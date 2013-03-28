/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   tizdec.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - A sample decoder program
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.dec"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <vector>
#include <string>
#include <getopt.h>

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <ostream>

#include "OMX_Core.h"
#include "tizosal.h"
#include "tizomxutil.hh"
#include "tizgraph.hh"
#include "tizgraphfactory.hh"


static tizgraph *gp_running_graph = NULL;

void
tizdec_sig_term_hdlr(int sig)
{
  fprintf(stdout, "\nTizonia OpenMAX IL decoder... terminating.\n");
  exit(EXIT_FAILURE);
}

void
tizdec_sig_quit_hdlr(int sig)
{
  if (NULL != gp_running_graph)
    {
      gp_running_graph->signal();
    }
  else
    {
      raise (SIGQUIT);
    }
}

void
tizdec_sig_stp_hdlr(int sig)
{
  fprintf(stdout, "\nPausing playback. Use 'fg' or 'bg' to resume...\n");
  raise (SIGSTOP);
}

void print_usage(void)
{
  printf("Tizonia OpenMAX IL decoder version %s\n\n", PACKAGE_VERSION);
  printf("usage: %s [-l] [-r] [-c] [-v] [-d] <file_uri>\n", PACKAGE_NAME);
  printf("options:\n");
  printf("\t-l --list-components\t\t\tEnumerate all OpenMAX IL components\n");
  printf("\t-r --roles-of-comp <component>\t\tDisplay the roles found in <component>\n");
  printf("\t-c --comps-of-role <role>\t\tDisplay the components that implement <role>\n");
  printf("\t-d --decode <file_uri>\t\t\tDecode a mp3 or ivf (vp8) file\n");
  printf("\t-v --version\t\t\t\tDisplay version info\n");
  printf("\n");
  printf("Example:\n");
  printf("\t tizdec -d ~/Music (decodes every supported file in the '~/Music' folder)\n");
  printf("\t    * Press [Ctrl-\\] to skip to next song.\n");
  printf("\t    * Press [Ctrl-z] to \"pause\" the playback (type 'fg' or 'bg' to resume).\n");
  printf("\t    * Press [Ctrl-c] to terminate the decoder.\n");
  printf("\n");
}

OMX_ERRORTYPE
list_comps()
{
  std::vector<std::string> components;
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  int index = 0;

  tizomxutil::init();

  if (OMX_ErrorNoMore == (ret = tizomxutil::list_comps(components)))
    {
      BOOST_FOREACH( std::string component, components )
        {
          printf ("Component at index [%d] -> [%s]\n", index++,
                  component.c_str());
        }
    }

  tizomxutil::deinit();

  if (ret == OMX_ErrorNoMore)
    {
      ret = OMX_ErrorNone;
    }

  return ret;
}

OMX_ERRORTYPE
roles_of_comp(OMX_STRING component)
{
  std::vector<std::string> roles;
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  int index = 0;

  tizomxutil::init();

  if (OMX_ErrorNoMore == (ret = tizomxutil::roles_of_comp(component,
                                                          roles)))
    {
      BOOST_FOREACH( std::string role, roles )
        {
          printf ("Component [%s] : role #%d -> [%s]\n", component, index++,
                  role.c_str());

        }
    }

  tizomxutil::deinit();

  if (ret == OMX_ErrorNoMore)
    {
      ret = OMX_ErrorNone;
    }

  return ret;
}

OMX_ERRORTYPE
comps_of_role(OMX_STRING role)
{
  std::vector<std::string> components;
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  int index = 0;

  tizomxutil::init();

  if (OMX_ErrorNoMore == (ret = tizomxutil::comps_of_role(role,
                                                        components)))
    {
      BOOST_FOREACH( std::string component, components )
        {
          printf ("Role [%s] found in [%s]\n", role, component.c_str());
        }
    }

  tizomxutil::deinit();

  if (ret == OMX_ErrorNoMore)
    {
      ret = OMX_ErrorNone;
    }

  return ret;
}


struct pathname_of
{
  pathname_of(std::vector<std::string> &file_list)
    :
    file_list_(file_list){}

  void operator()(const boost::filesystem::directory_entry& p) const
  {
    file_list_.push_back(p.path().string());
  }
  std::vector<std::string> &file_list_;
};

static OMX_ERRORTYPE
filter_unknown_media (std::vector<std::string> &file_list)
{
  std::vector<std::string>::iterator it = file_list.begin();
  while (it != file_list.end())
    {
      std::string extension(boost::filesystem::path(*it).
                            extension().string());
      if (extension.compare (".mp3") != 0)
        //&&
        // extension.compare (".ivf") != 0)
        {
          file_list.erase(it);
          // Restart the loop
          it = file_list.begin();
        }
      else
        {
          ++it;
        }
    }

  return file_list.empty() ? OMX_ErrorContentURIError : OMX_ErrorNone;
}

static OMX_ERRORTYPE
verify_uri (const std::string &uri,
            std::vector<std::string> &file_list)
{
  if (boost::filesystem::exists (uri)
      && boost::filesystem::is_regular_file (uri))
  {
    file_list.push_back (uri);
    return OMX_ErrorNone;
  }
  else if (boost::filesystem::exists (uri)
           && boost::filesystem::is_directory (uri))
  {
    std::for_each(boost::filesystem::directory_iterator
                  (boost::filesystem::path(uri)),
                  boost::filesystem::directory_iterator(),
                  pathname_of(file_list));
    return file_list.empty() ? OMX_ErrorContentURIError : OMX_ErrorNone;
  }

  return OMX_ErrorContentURIError;
}

OMX_ERRORTYPE
decode(const OMX_STRING uri)
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  std::vector<std::string> file_list;

  if (OMX_ErrorNone != verify_uri (uri, file_list))
    {
      fprintf(stderr, "File not found.\n");
      exit(EXIT_FAILURE);
    }

  if (OMX_ErrorNone != filter_unknown_media (file_list))
    {
      fprintf(stderr, "Unsupported media types.\n");
      exit(EXIT_FAILURE);
    }

  std::sort(file_list.begin(), file_list.end());

  tizgraph_ptr_t g_ptr(tizgraphfactory::create_graph(file_list[0].c_str()));
  if (!g_ptr)
    {
      // At this point we have removed all unsupported media, so we should
      // always have a graph object.
      fprintf(stderr, "Could not create a graph. Unsupported format.\n");
      exit(EXIT_FAILURE);
    }

  gp_running_graph = g_ptr.get();
  int list_size = file_list.size();
  for (int i=0; i < list_size; i++)
    {

      if (OMX_ErrorNone != (ret = g_ptr->load()))
        {
          fprintf(stderr, "Found error %s while loading the graph.\n",
                  tiz_err_to_str (ret));
          exit(EXIT_FAILURE);
        }

      if (OMX_ErrorNone != (ret = g_ptr->configure(i == 0 ? std::string()
                                                   : file_list[i])))
        {
          fprintf(stderr, "Could not configure a graph. Skipping file.\n");
          continue;
        }

      if (OMX_ErrorNone != (ret = g_ptr->execute()))
        {
          fprintf(stderr, "Found error %s while executing the graph.\n",
                  tiz_err_to_str (ret));
          exit(EXIT_FAILURE);
        }

      g_ptr->unload();
    }

  return ret;
}

int
main(int argc, char **argv)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int opt;
  int digit_optind = 0;

  if (argc < 2)
    {
      print_usage ();
      exit(EXIT_FAILURE);
    }

  signal(SIGTERM, tizdec_sig_term_hdlr);
  signal(SIGINT, tizdec_sig_term_hdlr);
  signal(SIGTSTP, tizdec_sig_stp_hdlr);
  signal(SIGQUIT, tizdec_sig_quit_hdlr);

  tiz_log_deinit ();

  TIZ_LOG(TIZ_TRACE, "Tizonia OpenMAX IL decoder...");

  while (1)
    {
      int this_option_optind = optind ? optind : 1;
      int option_index = 0;
      static struct option long_options[] =
        {
          {"list-components",  no_argument,       0,  'l' },
          {"roles-of-comp",  required_argument,       0,  'r' },
          {"comps-of-role",  required_argument,       0,  'c' },
          {"decode",  required_argument,       0,  'd' },
          {"version",  no_argument,       0,  'v' },
          {0,         0,                 0,  0 }
        };

      opt = getopt_long(argc, argv, "lr:c:d:v",
                      long_options, &option_index);
      if (opt == -1)
        break;

      switch (opt)
        {
        case 'l':
          {
            error = list_comps();
          }
          break;

        case 'r':
          {
            error = roles_of_comp(optarg);
          }
          break;

        case 'c':
          {
            error = comps_of_role(optarg);
          }
          break;

        case 'd':
          {
            printf("Tizonia OpenMAX IL decoder version %s\n",
                   PACKAGE_VERSION);
            error = decode(optarg);
          }
          break;

        case 'v':
          {
            printf("Tizonia OpenMAX IL decoder version %s\n",
                   PACKAGE_VERSION);
          }
          break;

        default:
            print_usage ();
            exit(EXIT_FAILURE);
        }
    }

  (void) tiz_log_deinit ();

  if (OMX_ErrorNone != error)
    {
      fprintf(stderr, "Error: %s\n", tiz_err_to_str (error));
      exit(EXIT_FAILURE);
    }

  exit(EXIT_SUCCESS);
}
