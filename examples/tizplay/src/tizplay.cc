/* -*-Mode: c++; -*- */
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
 * @file   tizplay.cc
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
#define TIZ_LOG_CATEGORY_NAME "tiz.play"
#endif

#include "OMX_Core.h"
#include "tizomxutil.h"
#include "tizgraph.h"
#include "tizgraphfactory.h"
#include "tizosal.h"

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

#include <termios.h>

static tizgraph *gp_running_graph = NULL;

static struct termios old_term, new_term;

void
init_termios(int echo)
{
  tcgetattr(0, &old_term); /* grab old terminal i/o settings */
  new_term = old_term; /* make new settings same as old settings */
  new_term.c_lflag &= ~ICANON; /* disable buffered i/o */
  new_term.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &new_term); /* use these new terminal i/o settings now */
}

void
reset_termios(void)
{
  tcsetattr(0, TCSANOW, &old_term);
}

char
getch_(int echo)
{
  char ch;
  init_termios (echo);
  ch = getchar();
  reset_termios ();
  return ch;
}

char
getch (void)
{
  /* Read 1 character without echo */
  return getch_(0);
}

void
tizplay_sig_term_hdlr (int sig)
{
  reset_termios ();
  exit (EXIT_FAILURE);
}

void
tizplay_sig_stp_hdlr (int sig)
{
  raise (SIGSTOP);
}

void
print_usage (void)
{
  printf ("Tizonia OpenMAX IL player version %s\n\n", PACKAGE_VERSION);
  printf ("usage: %s [-l] [-r] [-c] [-v] [-h] [FILE]\n", PACKAGE_NAME);
  printf ("options:\n");
  printf ("\t-l --list-components\t\t\tEnumerate all OpenMAX IL components.\n");
  printf
    ("\t-r --roles-of-comp <component>\t\tDisplay the roles found in <component>.\n");
  printf
    ("\t-c --comps-of-role <role>\t\tDisplay the components that implement <role>.\n");
  printf ("\t-v --version\t\t\t\tDisplay version info.\n");
  printf ("\t-h --help\t\t\t\tDisplay help.\n");
  printf ("\n");
  printf ("Example:\n");
  printf
    ("\t tizplay ~/Music (decodes every supported file in the '~/Music' folder)\n");
  printf ("\t    * Press [p] to skip to previous file.\n");
  printf ("\t    * Press [n] to skip to next file.\n");
  printf ("\t    * Press [SPACE] to pause playback.\n");
  printf ("\t    * Press [q] to quit.\n");
  printf ("\t    * Press [Ctrl-c] to terminate the player at any time.\n");
  printf ("\n");
}

OMX_ERRORTYPE
list_comps ()
{
  std::vector < std::string > components;
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  int index = 0;

  tizomxutil::init ();

  if (OMX_ErrorNoMore == (ret = tizomxutil::list_comps (components)))
    {
      BOOST_FOREACH (std::string component, components)
      {
        printf ("Component at index [%d] -> [%s]\n", index++,
                component.c_str ());
      }
    }

  tizomxutil::deinit ();

  if (ret == OMX_ErrorNoMore)
    {
      ret = OMX_ErrorNone;
    }

  return ret;
}

OMX_ERRORTYPE
roles_of_comp (OMX_STRING component)
{
  std::vector < std::string > roles;
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  int index = 0;

  tizomxutil::init ();

  if (OMX_ErrorNoMore == (ret = tizomxutil::roles_of_comp (component, roles)))
    {
      BOOST_FOREACH (std::string role, roles)
      {
        printf ("Component [%s] : role #%d -> [%s]\n", component, index++,
                role.c_str ());

      }
    }

  tizomxutil::deinit ();

  if (ret == OMX_ErrorNoMore)
    {
      ret = OMX_ErrorNone;
    }

  return ret;
}

OMX_ERRORTYPE
comps_of_role (OMX_STRING role)
{
  std::vector < std::string > components;
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  int index = 0;

  tizomxutil::init ();

  if (OMX_ErrorNoMore == (ret = tizomxutil::comps_of_role (role, components)))
    {
      BOOST_FOREACH (std::string component, components)
      {
        printf ("Role [%s] found in [%s]\n", role, component.c_str ());
      }
    }

  tizomxutil::deinit ();

  if (ret == OMX_ErrorNoMore)
    {
      ret = OMX_ErrorNone;
    }

  return ret;
}


struct pathname_of
{
  pathname_of (std::vector < std::string > &file_list):file_list_ (file_list)
  {
  }

  void operator () (const boost::filesystem::directory_entry & p) const
  {
    file_list_.push_back (p.path ().string ());
  }
  std::vector < std::string > &file_list_;
};

static OMX_ERRORTYPE
filter_unknown_media (std::vector < std::string > &file_list)
{
  std::vector < std::string >::iterator it = file_list.begin ();
  while (it != file_list.end ())
    {
      std::string extension (boost::filesystem::path (*it).extension ().
                             string ());
      if (extension.compare (".mp3") != 0)
        //&&
        // extension.compare (".ivf") != 0)
        {
          file_list.erase (it);
          // Restart the loop
          it = file_list.begin ();
        }
      else
        {
          ++it;
        }
    }

  return file_list.empty () ? OMX_ErrorContentURIError : OMX_ErrorNone;
}

static OMX_ERRORTYPE
verify_uri (const std::string & uri, std::vector < std::string > &file_list)
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
      std::for_each (boost::filesystem::directory_iterator
                     (boost::filesystem::path (uri)),
                     boost::filesystem::directory_iterator (),
                     pathname_of (file_list));
      return file_list.empty ()? OMX_ErrorContentURIError : OMX_ErrorNone;
    }

  return OMX_ErrorContentURIError;
}

enum ETIZPlayUserInput
{
  ETIZPlayUserStop,
  ETIZPlayUserNextFile,
  ETIZPlayUserPrevFile,
  ETIZPlayUserMax,
};

static ETIZPlayUserInput
wait_for_user_input (tizgraph_ptr_t graph_ptr)
{
  ETIZPlayUserInput input = ETIZPlayUserMax;
  while (1)
    {
      int ch[2];

      ch[0] = getch ();

      switch (ch[0])
        {
        case 'q':
          return ETIZPlayUserStop;

        case 68:            // key left
          // seek
          printf ("Seek (left key) - not implemented\n");
          break;

        case 67:            // key right
          // seek
          printf ("Seek (right key) - not implemented\n");
          break;

        case 65: // key up
          // seek
          printf ("Seek (up key) - not implemented\n");
          break;

        case 66: // key down
          // seek
          printf ("Seek (down key) - not implemented\n");
          break;

        case ' ':
          graph_ptr->pause ();
          break;

        case 'n':
          return ETIZPlayUserNextFile;

        case 'p':
          return ETIZPlayUserPrevFile;

        case '-':
          printf ("Vol down - not implemented\n");
          //Volume
          break;

        case '+':
          printf ("Vol up - not implemented\n");
          //Volume
          break;

        default:
//           printf ("%d - not implemented\n", ch[0]);
          break;
        };
    }

}

static OMX_ERRORTYPE
decode (const OMX_STRING uri)
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  std::vector < std::string > file_list;

  if (OMX_ErrorNone != verify_uri (uri, file_list))
    {
      fprintf (stderr, "File not found.\n");
      exit (EXIT_FAILURE);
    }

  if (OMX_ErrorNone != filter_unknown_media (file_list))
    {
      fprintf (stderr, "Unsupported media types.\n");
      exit (EXIT_FAILURE);
    }

  std::sort (file_list.begin (), file_list.end ());

  tizgraph_ptr_t g_ptr (tizgraphfactory::create_graph (file_list[0].c_str ()));
  if (!g_ptr)
    {
      // At this point we have removed all unsupported media, so we should
      // always have a graph object.
      fprintf (stderr, "Could not create a graph. Unsupported format.\n");
      exit (EXIT_FAILURE);
    }

  gp_running_graph = g_ptr.get ();
  int list_size    = file_list.size ();
  ETIZPlayUserInput user = ETIZPlayUserMax;
  bool config_first_time = true;
  for (int i = 0; i < list_size && user != ETIZPlayUserStop; i++)
    {
      if (OMX_ErrorNone != (ret = g_ptr->load ()))
        {
          fprintf (stderr, "Found error %s while loading the graph.\n",
                   tiz_err_to_str (ret));
          exit (EXIT_FAILURE);
        }

      if (OMX_ErrorNone != (ret = g_ptr->configure (config_first_time
                                                    ? std::string ()
                                                    : file_list[i])))
        {
          fprintf (stderr, "Could not configure a graph. Skipping file.\n");
          g_ptr->unload ();
          continue;
        }

      config_first_time = false;

      if (OMX_ErrorNone != (ret = g_ptr->execute ()))
        {
          fprintf (stderr, "Found error %s while executing the graph.\n",
                   tiz_err_to_str (ret));
          exit (EXIT_FAILURE);
        }

      user = wait_for_user_input (g_ptr);
      if (ETIZPlayUserPrevFile == user)
        {
          i   -= 2;
          if (i < -1)
            i  = -1;
        }
      g_ptr->unload ();
    }

  return ret;
}

int
main (int argc, char **argv)
{
  OMX_ERRORTYPE error = OMX_ErrorMax;
  int opt;
  int digit_optind = 0;

  if (argc < 2)
    {
      print_usage ();
      exit (EXIT_FAILURE);
    }

  signal (SIGTERM, tizplay_sig_term_hdlr);
  signal (SIGINT, tizplay_sig_term_hdlr);
  signal (SIGTSTP, tizplay_sig_stp_hdlr);
  signal (SIGQUIT, tizplay_sig_term_hdlr);

  tiz_log_init ();

  TIZ_LOG (TIZ_TRACE, "Tizonia OpenMAX IL player...");

  while (1)
    {
      int this_option_optind = optind ? optind : 1;
      int option_index = 0;
      static struct option long_options[] = {
        {"list-components", no_argument, 0, 'l'},
        {"roles-of-comp", required_argument, 0, 'r'},
        {"comps-of-role", required_argument, 0, 'c'},
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
      };

      opt = getopt_long (argc, argv, "lr:c:vh", long_options, &option_index);
      if (opt == -1)
        break;

      switch (opt)
        {

        case 0:
          break;

        case 'l':
          {
            error = list_comps ();
          }
          break;

        case 'r':
          {
            error = roles_of_comp (optarg);
          }
          break;

        case 'c':
          {
            error = comps_of_role (optarg);
          }
          break;

        case 'v':
          {
            error = OMX_ErrorNone;
            printf ("Tizonia OpenMAX IL player version %s\n",
                    PACKAGE_VERSION);
          }
          break;

        case 'h':
          {
            error = OMX_ErrorNone;
            print_usage ();
          }
          break;

        default:
          print_usage ();
          exit (EXIT_FAILURE);
        }
    }

  if (OMX_ErrorNone == error)
    {
      exit (EXIT_SUCCESS);
    }

  if (optind >= argc)
    {
      print_usage ();
      exit (EXIT_FAILURE);
    }

  if (OMX_ErrorMax == error)
    {
      error = decode (argv[optind]);
    }
  
  (void) tiz_log_deinit ();

  if (OMX_ErrorNone != error)
    {
      exit (EXIT_FAILURE);
    }

  exit (EXIT_SUCCESS);
}
