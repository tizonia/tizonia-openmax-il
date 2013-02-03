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

#include "OMX_Core.h"

#include "tizosal.h"

#include "tizomxutil.hh"
#include "tizmp3graph.hh"

#include <boost/foreach.hpp>

void
tizdec_sig_hdlr(int sig)
{
  TIZ_LOG(TIZ_LOG_TRACE, "Tizonia OpenMAX IL decoder exiting...");
  fprintf(stderr, "\nInterrupted...\n");
  exit(EXIT_FAILURE);
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

OMX_ERRORTYPE
decode(const OMX_STRING file_uri)
{
  std::vector<std::string> components;
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  int index = 0;

  tizomxutil::init();

  tizmp3graph g;
  component_names_t comp_list;
  comp_list.push_back("OMX.Aratelia.file_reader.binary");
  comp_list.push_back("OMX.Aratelia.audio_decoder.mp3");
  comp_list.push_back("OMX.Aratelia.audio_renderer.pcm");

  if (OMX_ErrorNone != (ret = g.instantiate(comp_list)))
    {
      fprintf(stderr, "Found error %s while instantiating the graph.\n",
              tiz_err_to_str (ret));
      tizomxutil::deinit();
      exit(EXIT_FAILURE);
    }

  if (OMX_ErrorNone != (ret = g.configure(file_uri)))
    {
      fprintf(stderr, "Found error %s while configuring the graph.\n",
              tiz_err_to_str (ret));
      tizomxutil::deinit();
      exit(EXIT_FAILURE);
    }

  if (OMX_ErrorNone != (ret = g.execute()))
    {
      fprintf(stderr, "Found error %s while executing the graph.\n",
              tiz_err_to_str (ret));
      tizomxutil::deinit();
      exit(EXIT_FAILURE);
    }

  g.destroy();

  tizomxutil::deinit();

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

  signal(SIGTERM, tizdec_sig_hdlr);
  signal(SIGINT, tizdec_sig_hdlr);

  tiz_log_init();

  TIZ_LOG(TIZ_LOG_TRACE, "Tizonia OpenMAX IL decoder...");

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
            tiz_log_deinit();
            exit(EXIT_FAILURE);
        }
    }

  tiz_log_deinit();

  if (OMX_ErrorNone != error)
    {
      fprintf(stderr, "Error: %s\n", tiz_err_to_str (error));
      exit(EXIT_FAILURE);
    }

  exit(EXIT_SUCCESS);
}
