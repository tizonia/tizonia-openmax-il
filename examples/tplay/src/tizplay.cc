/* -*-Mode: c++; -*- */
/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <vector>
#include <string>
#include <getopt.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>

#include <ostream>

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include <tizosal.h>
#include <OMX_Core.h>

#include "tizomxutil.h"
#include "tizplaylist.h"
#include "tizgraph.h"
#include "tizdecgraphmgr.h"
#include "tizgraphfactory.h"
#include "tizhttpservmgr.h"
#include "tizhttpservconfig.h"

static bool gb_daemon_mode = false;
static struct termios old_term, new_term;

namespace  // unnamed namespace
{

  void init_termios (int echo)
  {
    tcgetattr (0, &old_term);    /* grab old terminal i/o settings */
    new_term = old_term;         /* make new settings same as old settings */
    new_term.c_lflag &= ~ICANON; /* disable buffered i/o */
    new_term.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
    tcsetattr (0, TCSANOW,
               &new_term); /* use these new terminal i/o settings now */
  }

  void reset_termios (void)
  {
    tcsetattr (0, TCSANOW, &old_term);
  }

  char getch_ (int echo)
  {
    char ch;
    init_termios (echo);
    ch = getchar ();
    reset_termios ();
    return ch;
  }

  char getch (void)
  {
    /* Read 1 character without echo */
    return getch_ (0);
  }

  void tizplay_sig_term_hdlr (int sig)
  {
    if (!gb_daemon_mode)
    {
      reset_termios ();
    }
    exit (EXIT_FAILURE);
  }

  void tizplay_sig_stp_hdlr (int sig)
  {
    raise (SIGSTOP);
  }

  void tizplay_sig_pipe_hdlr (int sig)
  {
    // Simply ignore this one
  }

  void print_usage (void)
  {
    printf ("Tizonia OpenMAX IL player version %s\n\n", PACKAGE_VERSION);
    printf (
        "usage: %s [-c] [-d] [-h] [-l] [-p port] [-r] [-s] [--shuffle] [-v] "
        "[FILE/DIR]\n",
        PACKAGE_NAME);
    printf ("options:\n");
    printf (
        "\t-c --comps-of-role <role>\t\tDisplay the components that implement "
        "<role>.\n");
    printf ("\t-d --daemon\t\t\t\tRun in the background.\n");
    printf ("\t-h --help\t\t\t\tDisplay help.\n");
    printf (
        "\t-l --list-components\t\t\tEnumerate all OpenMAX IL components.\n");
    printf ("\t-p --port\t\t\t\tPort to be used for http streaming.\n");
    printf (
        "\t-r --roles-of-comp <component>\t\tDisplay the roles found in "
        "<component>.\n");
    printf ("\t-R --recurse\t\t\t\tRecursively process DIR.\n");
    printf ("\t   --shuffle\t\t\t\tShuffle the playlist.\n");
    printf (
        "\t-s --stream\t\t\t\tStream media via http. Default port is 8010.\n");
    printf ("\t-v --version\t\t\t\tDisplay version info.\n");
    printf ("\n");
    printf ("Examples:\n");
    printf (
        "\t tplay ~/Music (decodes every supported file in the '~/Music' "
        "directory)\n");
    printf ("\t    * Currently supported formats for playback:\n");
    printf (
        "\t      * mp3, flac (.flac, .ogg, .oga), opus (.opus, .ogg, .oga), "
        "vorbis (.ogg, .oga).\n");
    printf ("\t    * Key bindings:\n");
    printf ("\t      * [p] skip to previous file.\n");
    printf ("\t      * [n] skip to next file.\n");
    printf ("\t      * [SPACE] pause playback.\n");
    printf ("\t      * [+/-] increase/decrease volume.\n");
    printf ("\t      * [m] mute.\n");
    printf ("\t      * [q] quit.\n");
    printf ("\t      * [Ctrl-c] terminate the application at any time.\n");
    printf (
        "\n\t tplay -p 8011 -s ~/Music (streams every supported file in the "
        "'~/Music' directory)\n");
    printf ("\t    * Currently supported formats for streaming: mp3.\n");
    printf ("\t    * Key bindings:\n");
    printf ("\t      * [q] quit.\n");
    printf ("\t      * [Ctrl-c] terminate the application at any time.\n");
    printf ("\n");
  }

  void print_banner (void)
  {
    printf (
        "Tizonia OpenMAX IL player (v. %s).\n"
        "Copyright (C) 2011-2014 Juan A. Rubio\n\n",
        PACKAGE_VERSION);
  }

  OMX_ERRORTYPE
  list_comps ()
  {
    std::vector< std::string > components;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    print_banner ();

    tiz::omxutil::init ();

    if (OMX_ErrorNoMore == (ret = tiz::omxutil::list_comps (components)))
    {
      int index = 0;
      BOOST_FOREACH (std::string component, components)
      {
        printf ("Component at index [%d] -> [%s]\n", index++,
                component.c_str ());
      }
    }

    tiz::omxutil::deinit ();

    if (ret == OMX_ErrorNoMore)
    {
      ret = OMX_ErrorNone;
    }

    return ret;
  }

  OMX_ERRORTYPE
  roles_of_comp (OMX_STRING component)
  {
    std::vector< std::string > roles;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    print_banner ();

    tiz::omxutil::init ();

    if (OMX_ErrorNoMore
        == (ret = tiz::omxutil::roles_of_comp (component, roles)))
    {
      int index = 0;
      BOOST_FOREACH (std::string role, roles)
      {
        printf ("Component [%s] : role #%d -> [%s]\n", component, index++,
                role.c_str ());
      }
    }

    tiz::omxutil::deinit ();

    if (ret == OMX_ErrorNoMore)
    {
      ret = OMX_ErrorNone;
    }

    if (roles.empty ())
    {
      printf ("Component [%s] : No roles found.\n", component);
    }

    return ret;
  }

  OMX_ERRORTYPE
  comps_of_role (OMX_STRING role)
  {
    std::vector< std::string > components;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    print_banner ();

    tiz::omxutil::init ();

    if (OMX_ErrorNoMore
        == (ret = tiz::omxutil::comps_of_role (role, components)))
    {
      BOOST_FOREACH (std::string component, components)
      {
        printf ("Role [%s] found in [%s]\n", role, component.c_str ());
      }
    }

    tiz::omxutil::deinit ();

    if (ret == OMX_ErrorNoMore)
    {
      ret = OMX_ErrorNone;
    }

    if (components.empty ())
    {
      printf ("Role [%s] : No components found.\n", role);
    }

    return ret;
  }

  enum ETIZPlayUserInput
  {
    ETIZPlayUserStop,
    ETIZPlayUserNextFile,
    ETIZPlayUserPrevFile,
    ETIZPlayUserMax,
  };

  ETIZPlayUserInput wait_for_user_input (tiz::graphmgr::mgr_ptr_t mgr_ptr)
  {
    while (1)
    {
      if (gb_daemon_mode)
      {
        sleep (5000);
      }
      else
      {
        int ch[2];

        ch[0] = getch ();

        switch (ch[0])
        {
          case 'q':
            return ETIZPlayUserStop;

          case 68:  // key left
            // seek
            // printf ("Seek (left key) - not implemented\n");
            break;

          case 67:  // key right
            // seek
            // printf ("Seek (right key) - not implemented\n");
            break;

          case 65:  // key up
            // seek
            // printf ("Seek (up key) - not implemented\n");
            break;

          case 66:  // key down
            // seek
            // printf ("Seek (down key) - not implemented\n");
            break;

          case ' ':
            mgr_ptr->pause ();
            break;

          case 'm':
            mgr_ptr->mute ();
            break;

          case 'n':
            mgr_ptr->next ();
            break;

          case 'p':
            mgr_ptr->prev ();
            break;

          case '-':
            mgr_ptr->volume (-1);
            break;

          case '+':
            mgr_ptr->volume (1);
            break;

          default:
            //           printf ("%d - not implemented\n", ch[0]);
            break;
        };
      }
    }
  }

  ETIZPlayUserInput wait_for_user_input_while_streaming ()
  {
    while (1)
    {
      if (gb_daemon_mode)
      {
        sleep (5000);
      }
      else
      {
        int ch[2];

        ch[0] = getch ();

        switch (ch[0])
        {
          case 'q':
            return ETIZPlayUserStop;

          default:
            //           printf ("%d - not implemented\n", ch[0]);
            break;
        };
      }
    }
  }

  struct graph_error_functor
  {
    void operator()(OMX_ERRORTYPE a, std::string b) const
    {
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
      fprintf (stderr, "%s%s (%s).%s\n", KRED, b.c_str (), tiz_err_to_str (a),
               KNRM);
      exit (EXIT_FAILURE);
    }
  };

  OMX_ERRORTYPE
  decode (const std::string &uri, const bool shuffle_playlist,
          const bool recurse)
  {
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    uri_lst_t file_list;
    std::string error_msg;
    file_extension_lst_t extension_list;
    extension_list.insert (".mp3");
    extension_list.insert (".opus");
    extension_list.insert (".ogg");
    extension_list.insert (".oga");
    extension_list.insert (".flac");

    print_banner ();

    // Create a playlist
    if (!tizplaylist_t::assemble_play_list (uri, shuffle_playlist, recurse,
                                            extension_list, file_list,
                                            error_msg))
    {
      fprintf (stderr, "%s (%s).\n", error_msg.c_str (), uri.c_str ());
      exit (EXIT_FAILURE);
    }

    // Instantiate the decode manager
    tiz::graphmgr::mgr_ptr_t p_mgr
        = boost::make_shared< tiz::graphmgr::decodemgr >();

    // TODO: Check return codes
    p_mgr->init (file_list, graph_error_functor ());
    p_mgr->start ();

    while (ETIZPlayUserStop != wait_for_user_input (p_mgr))
    {
    }

    p_mgr->stop ();
    p_mgr->deinit ();

    return rc;
  }

  OMX_ERRORTYPE
  stream (const std::string &uri, const long int port,
          const bool shuffle_playlist, const bool recurse)
  {
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    uri_lst_t file_list;
    char hostname[120] = "";
    std::string ip_address;
    std::string error_msg;
    file_extension_lst_t extension_list;
    extension_list.insert (".mp3");

    print_banner ();

    if (!tizplaylist_t::assemble_play_list (uri, shuffle_playlist, recurse,
                                            extension_list, file_list,
                                            error_msg))
    {
      fprintf (stderr, "%s (%s).\n", error_msg.c_str (), uri.c_str ());
      exit (EXIT_FAILURE);
    }

    // Retrieve teh hostname
    // TODO: Error handling
    if (0 == gethostname (hostname, sizeof(hostname)))
    {
      struct hostent *p_hostent = gethostbyname (hostname);
      struct in_addr ip_addr = *(struct in_addr *)(p_hostent->h_addr);
      ip_address = inet_ntoa (ip_addr);
      fprintf (stdout, "Streaming from http://%s:%ld\n\n", hostname, port);
    }

    tizgraphconfig_ptr_t config
        = boost::make_shared< tiz::graph::httpservconfig >(file_list, hostname,
                                                           ip_address, port);

    // Instantiate the http streaming manager
    tiz::graphmgr::mgr_ptr_t p_mgr
        = boost::make_shared< tiz::graphmgr::httpservmgr >(config);

    // TODO: Check return codes
    p_mgr->init (file_list, graph_error_functor ());
    p_mgr->start ();

    while (ETIZPlayUserStop != wait_for_user_input_while_streaming ())
    {
    }

    p_mgr->stop ();
    p_mgr->deinit ();

    return rc;
  }

}  // unnamed namespace

int main (int argc, char **argv)
{
  OMX_ERRORTYPE error = OMX_ErrorMax;
  int opt;
  long int srv_port = 8010;  // default port for http streaming
  std::string media;
  bool shuffle_playlist = false;
  bool recurse = false;

  if (argc < 2)
  {
    print_usage ();
    exit (EXIT_FAILURE);
  }

  while (1)
  {
    int option_index = 0;
    static struct option long_options[]
        = { { "list-components", no_argument, 0, 'l' },
            { "roles-of-comp", required_argument, 0, 'r' },
            { "comps-of-role", required_argument, 0, 'c' },
            { "daemon", no_argument, 0, 'd' },
            { "shuffle", no_argument, 0, 1 },
            { "stream", required_argument, 0, 's' },
            { "port", required_argument, 0, 'p' },
            { "recurse", no_argument, 0, 'R' },
            { "version", no_argument, 0, 'v' },
            { "help", no_argument, 0, 'h' },
            { 0, 0, 0, 0 } };

    opt = getopt_long (argc, argv, "lr:c:d1p:s:Rvh", long_options,
                       &option_index);
    if (opt == -1)
      break;

    switch (opt)
    {

      case 0:
        break;

      case 1:
      {
        shuffle_playlist = true;
      }
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

      case 'd':
      {
        gb_daemon_mode = true;
      }
      break;

      case 'p':
      {
        char *p_end = NULL;
        srv_port = strtol (optarg, &p_end, 10);
        if ((p_end != NULL && *p_end != '\0') || srv_port < 1024)
        {
          fprintf (stderr,
                   "Please provide a port number in the range "
                   "[1024-65535] - %ld\n",
                   srv_port);
          exit (EXIT_FAILURE);
        }
      }
      break;

      case 's':
      {
        media = optarg;
      }
      break;

      case 'R':
      {
        recurse = true;
      }
      break;

      case 'v':
      {
        error = OMX_ErrorNone;
        print_banner ();
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

  if (gb_daemon_mode)
  {
    fprintf (stdout,
             "Tizonia OpenMAX IL player version %s "
             "running as a daemon (PID %d).\n",
             PACKAGE_VERSION, getpid ());
    if (-1 == daemon (1, 0))
    {
      fprintf (stderr, "Could not daemon.\n");
      exit (EXIT_FAILURE);
    }
  }

  signal (SIGTERM, tizplay_sig_term_hdlr);
  signal (SIGINT, tizplay_sig_term_hdlr);
  signal (SIGTSTP, tizplay_sig_stp_hdlr);
  signal (SIGQUIT, tizplay_sig_term_hdlr);
  signal (SIGPIPE, tizplay_sig_pipe_hdlr);

  tiz_log_init ();
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Tizonia OpenMAX IL player...");

  if (!media.empty ())
  {
    error = stream (media.c_str (), srv_port, shuffle_playlist, recurse);
    exit (EXIT_SUCCESS);
  }

  if (OMX_ErrorMax == error)
  {
    media = argv[optind] == NULL ? "" : argv[optind];
    error = decode (media, shuffle_playlist, recurse);
  }

  (void)tiz_log_deinit ();

  if (OMX_ErrorNone != error)
  {
    exit (EXIT_FAILURE);
  }

  exit (EXIT_SUCCESS);
}
