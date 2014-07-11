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
 * @file   tizplay.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - tplay: an audio player and streaming server
 * program
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE 30
#define FUSION_MAX_VECTOR_SIZE 20
#define SPIRIT_ARGUMENTS_LIMIT 20

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <vector>
#include <string>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>

#include <ostream>

#include <boost/version.hpp>

#include <taglib/taglib.h>

#include <tizplatform.h>
#include <OMX_Core.h>

#include "tizomxutil.hpp"
#include "tizdaemon.hpp"
#include "tizplaylist.hpp"
#include "tizgraph.hpp"
#include "tizdecgraphmgr.hpp"
#include "tizgraphfactory.hpp"
#include "tizhttpservmgr.hpp"
#include "tizhttpservconfig.hpp"
#include "tizhttpclntmgr.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play"
#endif

namespace  // unnamed namespace
{

  void print_usage (void)
  {
    printf ("Tizonia OpenMAX IL player version %s\n\n", PACKAGE_VERSION);
    printf (
        "usage: %s [-c role] [-d] [-h] [-l] [-v]\n"
        "\t     [-p port] [-r component] [-s]\n"
        "\t     [--bitrate-modes=comma-separated-list]\n"
        "\t     [--log-directory=DIR]\n"
        "\t     [--sampling-rates=comma-separated-list]\n"
        "\t     [--station-genre=string]\n"
        "\t     [--station-name=string]\n"
        "\t     [--shuffle]\n"
        "\t     <FILE/DIR>\n",
        PACKAGE_NAME);
    printf ("options:\n");
    printf (
        "\t-c --comps-of-role <role>\t\tDisplay the components that implement "
        "<role>.\n");
    printf ("\t-d --daemon\t\t\t\tRun in the background.\n");
    printf ("\t-h --help\t\t\t\tDisplay help.\n");
    printf (
        "\t-l --list-components\t\t\tEnumerate all OpenMAX IL components.\n");
    printf (
        "\t-p --port\t\t\t\tPort to be used for http streaming. Default: "
        "8010.\n");
    printf (
        "\t-r --roles-of-comp <component>\t\tDisplay the roles found in "
        "<component>.\n");
    printf ("\t-R --recurse\t\t\t\tRecursively process DIR.\n");
    printf (
        "\t   --log-directory\t\t\tA directory to be used for debug "
        "logging.\n");
    printf (
        "\t   --bitrate-modes\t\t\tA list of bitrate modes (CBR, VBR) that "
        "will be\n"
        "\t\t\t\t\t\tallowed in the playlist (http streaming only). Default: "
        "any.\n");
    printf (
        "\t   --sampling-rates\t\t\tA list of sampling rates that will be\n"
        "\t\t\t\t\t\tallowed in the playlist (http streaming only). Default: "
        "any.\n");
    printf ("\t   --shuffle\t\t\t\tShuffle the playlist.\n");
    printf (
        "\t   --station-genre\t\t\tSHOUTcast/ICEcast station genre "
        "(http streaming only).\n");
    printf (
        "\t   --station-name\t\t\tSHOUTcast/ICEcast station name "
        "(http streaming only).\n");
    printf (
        "\t-s --stream\t\t\t\tStream media via http using the "
        "SHOUTcast/ICEcast protocol.\n");
    printf ("\t-v --version\t\t\t\tDisplay version info.\n");
    printf ("\n");
    printf ("Examples:\n");
    printf ("\t tplay ~/Music\n\n");
    printf (
        "\t    * Decodes every supported file in the '~/Music' directory)\n");
    printf ("\t    * File formats currently supported for playback:\n");
    printf (
        "\t      * mp3, aac, (.aac only) flac (.flac, .ogg, .oga), opus "
        "(.opus, .ogg, .oga), "
        "vorbis (.ogg, .oga).\n");
    printf ("\t    * Key bindings:\n");
    printf ("\t      * [p] skip to previous file.\n");
    printf ("\t      * [n] skip to next file.\n");
    printf ("\t      * [SPACE] pause playback.\n");
    printf ("\t      * [+/-] increase/decrease volume.\n");
    printf ("\t      * [m] mute.\n");
    printf ("\t      * [q] quit.\n");
    printf ("\t      * [Ctrl-c] terminate the application at any time.\n");
    printf ("\n\t tplay --sampling-rates=44100,48000 -p 8011 -s ~/Music\n\n");
    printf ("\t    * This streams files from the '~/Music' directory.\n");
    printf ("\t    * File formats currently supported for streaming: mp3.\n");
    printf ("\t    * Sampling rates other than [44100,4800] are ignored.\n");
    printf ("\t    * Key bindings:\n");
    printf ("\t      * [q] quit.\n");
    printf ("\t      * [Ctrl-c] terminate the application at any time.\n");
    printf ("\n");
    printf ("Debug Info:\n");
    printf ("\t    * Boost [%s]\n", BOOST_LIB_VERSION);
    printf ("\t    * TagLib [%d.%d.%d]\n", TAGLIB_MAJOR_VERSION,
            TAGLIB_MINOR_VERSION, TAGLIB_PATCH_VERSION);
    printf ("\n");
  }


}  // unnamed namespace

int main (int argc, char **argv)
{
  OMX_ERRORTYPE error = OMX_ErrorMax;
  int opt;
  long int srv_port = 8010;  // default port for http streaming
  std::vector< int > sampling_rate_list;
  std::vector< std::string > sampling_rate_str_list;
  std::vector< std::string > bitrate_mode_list;
  std::string media;
  std::string station_name ("Tizonia Radio");
  std::string station_genre ("Unknown Genre");
  std::string log_dir;
  bool shuffle_playlist = false;
  bool recurse = false;
  bool streaming_server = false;

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
            { "sampling-rates", required_argument, 0, 2 },
            { "station-name", required_argument, 0, 3 },
            { "station-genre", required_argument, 0, 4 },
            { "bitrate-modes", required_argument, 0, 5 },
            { "log-directory", required_argument, 0, 6 },
            { "stream", required_argument, 0, 's' },
            { "port", required_argument, 0, 'p' },
            { "recurse", no_argument, 0, 'R' },
            { "version", no_argument, 0, 'v' },
            { "help", no_argument, 0, 'h' },
            { 0, 0, 0, 0 } };

    opt = getopt_long (argc, argv, "lr:c:d123p:s:Rvh", long_options,
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

      case 2:
      {
        char *p_end = NULL;
        std::string sampling_rates_str (optarg);
        boost::split (sampling_rate_str_list, sampling_rates_str,
                      boost::is_any_of (","));
        if (!valid_sampling_rate_list (sampling_rate_str_list,
                                       sampling_rate_list))
        {
          fprintf (
              stderr,
              "Invalid argument : %s .\nValid sampling rates values :\n"
              "[8000,11025,12000,16000,22050,24000,32000,44100,48000,96000]\n",
              optarg);
          exit (EXIT_FAILURE);
        }
      }
      break;

      case 3:
      {
        station_name = optarg;
      }
      break;

      case 4:
      {
        station_genre = optarg;
      }
      break;

      case 5:
      {
        char *p_end = NULL;
        std::string bitrates_str (optarg);
        boost::split (bitrate_mode_list, bitrates_str, boost::is_any_of (","));
        if (!valid_bitrate_list (bitrate_mode_list))
        {
          fprintf (stderr,
                   "Invalid argument : %s .\nValid bitrate type values :\n"
                   "[CBR,VBR]\n",
                   optarg);
          exit (EXIT_FAILURE);
        }
      }
      break;

      case 6:
      {
        DIR *dir = opendir (optarg);
        if (NULL == dir)
        {
          fprintf (stderr, "Invalid argument : %s (%s).\n", optarg,
                   strerror (errno));
          exit (EXIT_FAILURE);
        }
        else
        {
          closedir (dir);
          dir = NULL;
          log_dir = optarg;
        }
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
        if ((p_end != NULL && *p_end != '\0') || srv_port <= 1024)
        {
          fprintf (stderr,
                   "Please provide a port number in the range "
                   "[1025-65535] - (%s)\n",
                   optarg);
          exit (EXIT_FAILURE);
        }
      }
      break;

      case 's':
      {
        media = optarg;
        streaming_server = true;
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
             "Tizonia OpenMAX IL player version %s. "
             "Starting daemon.\n",
             PACKAGE_VERSION);
    if (-1 == tiz::daemon::daemonize ())
    {
      fprintf (stderr, "Could not daemonize.\n");
      exit (EXIT_FAILURE);
    }
  }

  signal (SIGTERM, tizplay_sig_term_hdlr);
  signal (SIGPIPE, SIG_IGN);
  signal (SIGINT, tizplay_sig_term_hdlr);
  signal (SIGTERM, tizplay_sig_term_hdlr);

  if (!gb_daemon_mode)
  {
    signal (SIGTSTP, tizplay_sig_stp_hdlr);
    signal (SIGQUIT, tizplay_sig_term_hdlr);
  }

  tiz_log_init ();
  if (log_dir.length () > 0)
  {
    tiz_log_set_unique_rolling_file (log_dir.c_str (), PACKAGE_NAME);
  }
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Tizonia OpenMAX IL player...");

  if (streaming_server && !media.empty ())
  {
    error = stream (media.c_str (), srv_port, shuffle_playlist, recurse,
                    sampling_rate_str_list, sampling_rate_list,
                    bitrate_mode_list, station_name, station_genre);
    exit (EXIT_SUCCESS);
  }

  if (OMX_ErrorMax == error)
  {
    media = argv[optind] == NULL ? "" : argv[optind];

    // TODO: This WIP
    std::string uri_scheme = media.substr (0, 7);
    if (uri_scheme.size () >= 7 && uri_scheme.compare ("http://") == 0)
    {
      error = decode_stream (media);
    }
    else
    {
      error = decode (media, shuffle_playlist, recurse);
    }
  }

  (void)tiz_log_deinit ();

  if (OMX_ErrorNone != error)
  {
    exit (EXIT_FAILURE);
  }

  exit (EXIT_SUCCESS);
}
