/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   tizplayapp.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief tizonia app wrapper
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <limits.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <sys/utsname.h>

#include <cstdlib>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/version.hpp>

#include <MediaInfo/MediaInfo.h>
#include <taglib/taglib.h>

#include <OMX_Core.h>
#include <tizplatform.h>

#include "tizdaemon.hpp"
#include "tizgraphmgr.hpp"
#include "tizgraphtypes.hpp"
#include "tizomxutil.hpp"
#include <decoders/tizdecgraphmgr.hpp>
#include <httpclnt/tizhttpclntmgr.hpp>
#include <httpserv/tizhttpservconfig.hpp>
#include <httpserv/tizhttpservmgr.hpp>
#include <services/chromecast/tizchromecastconfig.hpp>
#include <services/chromecast/tizchromecastmgr.hpp>
#include <services/tunein/tiztuneinconfig.hpp>
#include <services/tunein/tiztuneinmgr.hpp>
#include <services/googlemusic/tizgmusicconfig.hpp>
#include <services/googlemusic/tizgmusicmgr.hpp>
#include <services/soundcloud/tizscloudconfig.hpp>
#include <services/soundcloud/tizscloudmgr.hpp>
#ifdef HAVE_LIBSPOTIFY
#include <services/spotify/tizspotifyconfig.hpp>
#include <services/spotify/tizspotifymgr.hpp>
#endif
#include <services/plex/tizplexconfig.hpp>
#include <services/plex/tizplexmgr.hpp>
#include <services/youtube/tizyoutubeconfig.hpp>
#include <services/youtube/tizyoutubemgr.hpp>
#include <services/iheart/tiziheartconfig.hpp>
#include <services/iheart/tiziheartmgr.hpp>

#include "tizplayapp.hpp"

#include <boost/python.hpp>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.app"
#endif

namespace bf = boost::filesystem;
namespace bp = boost::python;

#define APP_NAME "tizonia"
#define PYTHON_EXEC_TEMPLATE                        \
  "import pkg_resources\nprint('\t    * [MODULE', " \
  "pkg_resources.get_distribution('MODULE').version, ']')\n"

#define CHECK_PYTHON_MODULE_VER(mod)                                  \
  do                                                                  \
  {                                                                   \
    std::string exec_arg (PYTHON_EXEC_TEMPLATE);                      \
    boost::replace_all (exec_arg, "MODULE", mod);                     \
    try                                                               \
    {                                                                 \
      bp::object result = bp::exec (exec_arg.c_str (), py_global);    \
    }                                                                 \
    catch (bp::error_already_set & e)                                 \
    {                                                                 \
      PyErr_PrintEx (0);                                              \
      std::cerr << std::string ("\nPython module ") << mod            \
                << std::string (" not found.")                        \
                << std::string (" Please use pip3 to install it.\n"); \
    }                                                                 \
  } while (0)

namespace
{
  const int TIZ_MAX_BITRATE_MODES = 2;
  bool gb_daemon_mode = false;
  bool gb_termios_inited = false;
  struct termios old_term = (const struct termios){ 0 };
  struct termios new_term;

  enum ETIZPlayUserInput
  {
    ETIZPlayUserQuit,
    ETIZPlayUserNextFile,
    ETIZPlayUserPrevFile,
    ETIZPlayUserMax,
  };

  void player_init_termios (int echo)
  {
    tcgetattr (0, &old_term);    /* grab old terminal i/o settings */
    new_term = old_term;         /* make new settings same as old settings */
    new_term.c_lflag &= ~ICANON; /* disable buffered i/o */
    new_term.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
    tcsetattr (0, TCSANOW,
               &new_term); /* use these new terminal i/o settings now */
    gb_termios_inited = true;
  }

  void player_reset_termios (void)
  {
    if (gb_termios_inited)
    {
      gb_termios_inited = false;
      tcsetattr (0, TCSANOW, &old_term);
    }
  }

  char getch_ (int echo)
  {
    char ch;
    player_init_termios (echo);
    ch = (char)getchar ();
    player_reset_termios ();
    return ch;
  }

  char getch (void)
  {
    /* Read 1 character without echo */
    return getch_ (0);
  }

  void player_exit_failure ()
  {
    if (!gb_daemon_mode)
    {
      player_reset_termios ();
    }
    exit (EXIT_FAILURE);
  }

  void player_exit_success ()
  {
    if (!gb_daemon_mode)
    {
      player_reset_termios ();
    }
    exit (EXIT_SUCCESS);
  }

  void player_sig_term_hdlr (int sig)
  {
    printf ("\n\n");
    TIZ_PRINTF_C04 ("%s exiting (Ctrl-C).", APP_NAME);
    player_exit_success ();
  }

  void player_sig_stp_hdlr (int sig)
  {
    raise (SIGSTOP);
  }

  bool get_host_name_and_ip (std::string &host_name, std::string &ip_address,
                             std::string &error_msg)
  {
    bool outcome = true;  // we'll assume everything will be OK, or else,
                          // early-return in case it is not.
    char host_name_buf[HOST_NAME_MAX + 1] = "";
    struct ifaddrs *myaddrs = NULL;
    struct ifaddrs *ifa = NULL;
    void *in_addr;
    char ip_addr_buf[INET_ADDRSTRLEN + 1];

    if (gethostname (host_name_buf, sizeof (host_name_buf)) != 0)
    {
      error_msg.assign (strerror (errno));
      // Early return
      return false;
    }

    if (getifaddrs (&myaddrs) != 0)
    {
      error_msg.assign (strerror (errno));
      // Early return
      return false;
    }

    for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
    {
      if (ifa->ifa_addr == NULL)
      {
        continue;
      }
      if (!(ifa->ifa_flags & IFF_UP))
      {
        continue;
      }

      switch (ifa->ifa_addr->sa_family)
      {
        case AF_INET:
        {
          struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
          in_addr = &s4->sin_addr;
          break;
        }

        case AF_INET6:
        default:
        {
          continue;
        }
      }

      if (!inet_ntop (ifa->ifa_addr->sa_family, in_addr, ip_addr_buf,
                      sizeof (ip_addr_buf)))
      {
        error_msg.assign ("inet_ntop failed");
        return false;
      }

      // No need to iterate more
      break;
    }

    freeifaddrs (myaddrs);

    host_name.assign (host_name_buf);
    ip_address.assign (ip_addr_buf);

    return outcome;
  }

  ETIZPlayUserInput player_wait_for_user_input (
      tiz::graphmgr::mgr_ptr_t mgr_ptr)
  {
    int playlist_position = 0;
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
          printf("\nValue is : %d\n",ch[0]);
          switch (ch[0])
            {
            case '1':
              {
                playlist_position = (playlist_position * 10) + 1;
              }
              break;
            case '2':
              {
                playlist_position = (playlist_position * 10) + 2;
              }
              break;
            case '3':
              {
                playlist_position = (playlist_position * 10) + 3;
              }
              break;
            case '4':
              {
                playlist_position = (playlist_position * 10) + 4;
              }
              break;
            case '5':
              {
                playlist_position = (playlist_position * 10) + 5;
              }
              break;
            case '6':
              {
                playlist_position = (playlist_position * 10) + 6;
              }
              break;
            case '7':
              {
                playlist_position = (playlist_position * 10) + 7;
              }
              break;
            case '8':
              {
                playlist_position = (playlist_position * 10) + 8;
              }
              break;
            case '9':
              {
                playlist_position = (playlist_position * 10) + 9;
              }
              break;
            case '0':
              {
                if (playlist_position != 0)
                  {
                    playlist_position = (playlist_position * 10) + 0;
                  }
              }
              break;

            case 'q':
              return ETIZPlayUserQuit;

            case 27:
              {
                ch[0] = getch(); // skip
                printf("\n  Value is : %d\n",ch[0]);
                ch[0] = getch();
                printf("\n    Value is : %d\n",ch[0]);
                switch (ch[0])
                  {
                  case 68:  // key left
                    // seek
                    // printf ("Seek (left key) - not implemented\n");
                    break;

                  case 67:  // key right
                    // seek
                    // printf ("Seek (right key) - not implemented\n");
                    break;

                  case 65:  // key up
                    mgr_ptr->volume_step (1);
                    playlist_position = 0;
                    break;

                  case 66:  // key down
                    mgr_ptr->volume_step (-1);
                    playlist_position = 0;
                    break;

                  case 53:  // page up
                    ch[0] = getch(); // skip
                    printf("\n      Value is : %d\n",ch[0]);
                    mgr_ptr->next ();
                    playlist_position = 0;
                    break;

                  case 54:  // page down
                    ch[0] = getch(); // skip
                    printf("\n      Value is : %d\n",ch[0]);
                    mgr_ptr->prev ();
                    playlist_position = 0;
                    break;
                  default:
                    // printf("\n        (not)Value is : %d\n",ch[0]);
                    break;
                  }
              }
              break;

            case ' ':
              mgr_ptr->pause ();
              playlist_position = 0;
              break;

            case 'g':
              {
                if (playlist_position)
                  {
                    mgr_ptr->position (playlist_position);
                    playlist_position = 0;
                  }
              }
              break;

            case 'm':
              mgr_ptr->mute ();
              playlist_position = 0;
              break;

            case 'n':
              mgr_ptr->next ();
              playlist_position = 0;
              break;

            case 'p':
              mgr_ptr->prev ();
              playlist_position = 0;
              break;

            case '-':
              mgr_ptr->volume_step (-1);
              playlist_position = 0;
              break;

            case '+':
              mgr_ptr->volume_step (1);
              playlist_position = 0;
              break;

            default:
              // printf ("%d - not implemented\n", ch[0]);
              break;
            };
        }
    }
  }

  ETIZPlayUserInput player_wait_for_user_input_while_streaming (
      tiz::graphmgr::mgr_ptr_t mgr_ptr)
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
            return ETIZPlayUserQuit;

          case 'm':
            mgr_ptr->mute ();
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

  struct graphmgr_termination_cback
  {
    void operator() (OMX_ERRORTYPE code, std::string msg) const
    {
      if (OMX_ErrorNone != code)
      {
        printf ("\n");
        TIZ_PRINTF_C04 ("%s exiting (%s).", APP_NAME,
                        tiz_err_to_str (code));
        printf ("\n");
        boost::algorithm::trim(msg);
        TIZ_PRINTF_C01 (" %s", msg.c_str ());
        std::cout << std::endl;
        player_exit_failure ();
      }
      else
      {
        printf ("\n");
        printf ("\n");
        TIZ_PRINTF_C04 ("%s exiting (Quit).", APP_NAME);
        printf ("\n");
        printf ("\n");
        player_exit_success ();
      }
    }
  };
}

tiz::playapp::playapp (int argc, char *argv[]) : popts_ (argc, argv)
{
  tiz_log_init ();
}

tiz::playapp::~playapp ()
{
  (void)tiz_log_deinit ();
}

int tiz::playapp::run ()
{
  check_or_create_config_file ();
  set_option_handlers ();
  return popts_.consume ();
}

void tiz::playapp::check_or_create_config_file ()
{
  const std::string home_dir (std::getenv ("HOME"));
  if (!home_dir.empty ())
  {
    const std::string home_conf_dir (home_dir + "/.config/tizonia");
    const std::string home_conf_file (home_conf_dir + "/tizonia.conf");

    if (!bf::exists (home_conf_file))
    {
      // Canonical locations of tizonia.conf
      const std::string etc_conf_file ("/etc/tizonia/tizonia.conf"); // OLD location
      const std::string etc_xdg_conf_file ("/etc/xdg/tizonia/tizonia.conf"); // NEW location

      // STEP 1: Create $HOME/.config/tizonia, if it doesn't exist
      if (!bf::exists (home_conf_dir))
      {
        boost::system::error_code ec;
        bf::create_directories (home_conf_dir, ec);
        if (ec.value () != 0)
          {
            // Oops... if we can't create the home config dire, then we have no
            // business to do here. EARLY RETURN!!
            return;
          }
      }

      // STEP 2: See if tizonia.conf can be found under the SNAP directory
      const char *p_snap_env = std::getenv ("SNAP");
      if (p_snap_env)
      {
        const std::string snap_dir (p_snap_env);
        if (!snap_dir.empty ())
        {
          const std::string snap_conf_file_path (snap_dir + etc_xdg_conf_file);
          boost::system::error_code ec;
          bf::copy_file (snap_conf_file_path, home_conf_file, ec);
        }
      }

      // STEP 3: See if we can find tizonia.conf under one of $XDG_CONFIG_DIRS
      if (!bf::exists (home_conf_file))
      {
        char *p_env_str = std::getenv ("XDG_CONFIG_DIRS");
        if (p_env_str)
        {
          char *pch = NULL;
          pch = strtok (p_env_str, ":");
          while (pch != NULL && !bf::exists (home_conf_file))
          {
            boost::system::error_code ec;
            std::string xdg_file(pch);
            xdg_file.append(etc_xdg_conf_file);
            printf ("xdg_file : %s\n", xdg_file.c_str());
            bf::copy_file (xdg_file, home_conf_file, ec);
            if (ec.value () == 0)
            {
              break;
            }
            pch = strtok (NULL, ":");
          }
        }
      }

      // STEP 4: Try to find tizonia.conf under /etc/xdg
      if (!bf::exists (home_conf_file))
      {
        boost::system::error_code ec;
        bf::copy_file (etc_xdg_conf_file, home_conf_file, ec);
      }

      // STEP 5: Last chance. Try to find tizonia.conf under /etc
      if (!bf::exists (home_conf_file))
      {
        boost::system::error_code ec;
        bf::copy_file (etc_conf_file, home_conf_file, ec);
      }
    }  // if (!bf::exists (home_conf_file))
  }    // if (!home_dir.empty())
}

void tiz::playapp::set_option_handlers ()
{
  // debug-related program options
  popts_.set_option_handler (
      "log-directory", boost::bind (&tiz::playapp::unique_log_file, this));
  popts_.set_option_handler (
      "debug-info", boost::bind (&tiz::playapp::print_debug_info, this));
  // OMX-related program options
  popts_.set_option_handler ("comp-list",
                             boost::bind (&tiz::playapp::list_of_comps, this));
  popts_.set_option_handler ("roles-of-comp",
                             boost::bind (&tiz::playapp::roles_of_comp, this));
  popts_.set_option_handler ("comps-of-role",
                             boost::bind (&tiz::playapp::comp_of_role, this));
  // local audio decoding program options
  popts_.set_option_handler ("decode-local",
                             boost::bind (&tiz::playapp::decode_local, this));
  // streaming audio server program options
  popts_.set_option_handler ("serve-stream",
                             boost::bind (&tiz::playapp::serve_stream, this));
  // streaming audio client program options
  popts_.set_option_handler ("decode-stream",
                             boost::bind (&tiz::playapp::decode_stream, this));
  // spotify streaming client program options
  popts_.set_option_handler ("spotify-stream",
                             boost::bind (&tiz::playapp::spotify_stream, this));
  // Google music streaming client program options
  popts_.set_option_handler ("gmusic-stream",
                             boost::bind (&tiz::playapp::gmusic_stream, this));
  // SoundCloud music streaming client program options
  popts_.set_option_handler ("scloud-stream",
                             boost::bind (&tiz::playapp::scloud_stream, this));
  // Tunein internet radio directory streaming client program options
  popts_.set_option_handler ("tunein-stream",
                             boost::bind (&tiz::playapp::tunein_stream, this));
  // YouTube audio streaming client program options
  popts_.set_option_handler ("youtube-stream",
                             boost::bind (&tiz::playapp::youtube_stream, this));
  // Plex audio streaming client program options
  popts_.set_option_handler ("plex-stream",
                             boost::bind (&tiz::playapp::plex_stream, this));
  // Iheart audio streaming client program options
  popts_.set_option_handler ("iheart-stream",
                             boost::bind (&tiz::playapp::iheart_stream, this));
  // HTTP music streaming on Chromecast device
  popts_.set_option_handler (
      "http-stream-chromecast",
      boost::bind (&tiz::playapp::http_stream_chromecast, this));
  // Google music streaming on Chromecast device
  popts_.set_option_handler (
      "gmusic-stream-chromecast",
      boost::bind (&tiz::playapp::gmusic_stream_chromecast, this));
  // Soudcloud audio streaming on Chromecast device
  popts_.set_option_handler (
      "scloud-stream-chromecast",
      boost::bind (&tiz::playapp::scloud_stream_chromecast, this));
  // Tunein audio streaming on Chromecast device
  popts_.set_option_handler (
      "tunein-stream-chromecast",
      boost::bind (&tiz::playapp::tunein_stream_chromecast, this));
  // YouTube audio streaming on Chromecast device
  popts_.set_option_handler (
      "youtube-stream-chromecast",
      boost::bind (&tiz::playapp::youtube_stream_chromecast, this));
  // Plex audio streaming on Chromecast device
  popts_.set_option_handler (
      "plex-stream-chromecast",
      boost::bind (&tiz::playapp::plex_stream_chromecast, this));
  // Iheart audio streaming on Chromecast device
  popts_.set_option_handler (
      "iheart-stream-chromecast",
      boost::bind (&tiz::playapp::iheart_stream_chromecast, this));
}

OMX_ERRORTYPE
tiz::playapp::daemonize_if_requested () const
{
  gb_daemon_mode = popts_.daemon ();

  if (gb_daemon_mode)
  {
    TIZ_PRINTF_C04 ("Starting daemon.");
    printf ("\n");
    if (-1 == tiz::daemon::daemonize ())
    {
      TIZ_PRINTF_C01 ("Could not daemonize.");
      player_exit_failure ();
    }
  }

  signal (SIGTERM, player_sig_term_hdlr);
  signal (SIGPIPE, SIG_IGN);
  signal (SIGINT, player_sig_term_hdlr);
  signal (SIGTERM, player_sig_term_hdlr);

  if (!gb_daemon_mode)
  {
    signal (SIGTSTP, player_sig_stp_hdlr);
    signal (SIGQUIT, player_sig_term_hdlr);
  }
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz::playapp::unique_log_file () const
{
  const std::string &log_dir = popts_.log_dir ();
  tiz_log_set_unique_rolling_file (log_dir.c_str (), APP_NAME);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz::playapp::print_debug_info () const
{
  if (popts_.debug_info ())
  {
    struct utsname name;
    print_banner ();
    printf ("Debug Info:\n");
    if (!uname (&name))
    {
      printf ("\t    * [%s@%s-%s]\n", name.sysname, name.release, name.version);
    }
    printf ("\t    * [Boost %s]\n", BOOST_LIB_VERSION);
    printf ("\t    * [TagLib %d.%d.%d]\n", TAGLIB_MAJOR_VERSION,
            TAGLIB_MINOR_VERSION, TAGLIB_PATCH_VERSION);
    std::wstring wide (
        MediaInfoLib::MediaInfo::Option_Static (L"Info_Version"));
    printf ("\t    * [%s]\n",
            std::string (wide.begin (), wide.end ()).c_str ());
    {
      OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
      Py_Initialize ();

      try
        {
          // Import the Tizonia Plex proxy module
          bp::object py_main = bp::import ("__main__");

          // Retrieve the main module's namespace
          bp::object py_global = py_main.attr ("__dict__");

          std::vector< std::string > modules =  boost::assign::list_of ("gmusicapi")
            ("soundcloud")
            ("youtube-dl")
            ("pafy")
            ("pycountry")
            ("titlecase")
            ("pychromecast")
            ("plexapi")
            ("fuzzywuzzy")
            ("eventlet")
            ("python-Levenshtein")
            ("joblib")
            ("spotipy")
            ("youtube-dl");
          BOOST_FOREACH (std::string module, modules)
            {
              CHECK_PYTHON_MODULE_VER(module);
            }

          rc = OMX_ErrorNone;
        }
      catch (bp::error_already_set &e)
        {
          PyErr_PrintEx (0);
          std::cerr << std::string (
              "\nPython modules 'joblib' or 'fuzzywuzzy' not found."
              "\nPlease make sure these are installed correctly.\n");
        }
      catch (...)
        {
          std::cerr << std::string ("Unknown exception caught");
        }
      return rc;
    }
    printf ("\n");
  }
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz::playapp::list_of_comps () const
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
      printf ("Component at index [%d] -> [%s]\n", index++, component.c_str ());
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
tiz::playapp::roles_of_comp () const
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  const std::string component = popts_.component_name ();
  std::vector< std::string > roles;

  print_banner ();

  tiz::omxutil::init ();

  if (OMX_ErrorNoMore == (ret = tiz::omxutil::roles_of_comp (component, roles)))
  {
    int index = 0;
    BOOST_FOREACH (std::string role, roles)
    {
      printf ("Component [%s] : role #%d -> [%s]\n", component.c_str (),
              index++, role.c_str ());
    }
  }

  tiz::omxutil::deinit ();

  if (ret == OMX_ErrorNoMore)
  {
    ret = OMX_ErrorNone;
  }

  if (roles.empty ())
  {
    printf ("Component [%s] : No roles found.\n", component.c_str ());
  }

  return ret;
}

OMX_ERRORTYPE
tiz::playapp::comp_of_role () const
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  const std::string role = popts_.component_role ();
  std::vector< std::string > components;

  print_banner ();

  tiz::omxutil::init ();

  if (OMX_ErrorNoMore
      == (ret = tiz::omxutil::comps_of_role (
              const_cast< OMX_STRING > (role.c_str ()), components)))
  {
    BOOST_FOREACH (std::string component, components)
    {
      printf ("Role [%s] found in [%s]\n", role.c_str (), component.c_str ());
    }
  }

  tiz::omxutil::deinit ();

  if (ret == OMX_ErrorNoMore)
  {
    ret = OMX_ErrorNone;
  }

  if (components.empty ())
  {
    printf ("Role [%s] : No components found.\n", role.c_str ());
  }

  return ret;
}

OMX_ERRORTYPE
tiz::playapp::decode_local ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const uri_lst_t &uri_list = popts_.uri_list ();
  const bool shuffle = popts_.shuffle ();
  const bool recurse = popts_.recurse ();

  uri_lst_t file_list;
  std::string error_msg;

  print_banner ();

  file_extension_lst_t extension_list;
  // Add here the list of file extensions currently supported for playback
  extension_list.insert (".mp3");
  extension_list.insert (".mp2");
  extension_list.insert (".mpa");
  extension_list.insert (".m2a");
  extension_list.insert (".opus");
  extension_list.insert (".ogg");
  extension_list.insert (".oga");
  extension_list.insert (".flac");
  extension_list.insert (".aac");
  extension_list.insert (".wav");
  extension_list.insert (".aiff");
  extension_list.insert (".aif");

  // Create a playlist
  BOOST_FOREACH (std::string uri, uri_list)
  {
    if (!tizplaylist_t::assemble_play_list (
            uri, shuffle, recurse, extension_list, file_list, error_msg))
    {
      TIZ_PRINTF_C01 ("%s (%s).", error_msg.c_str (), uri.c_str ());
      player_exit_failure ();
    }
  }

  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (file_list));

  assert (playlist);

  // Instantiate the decode manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::decodemgr > ();

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::serve_stream ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const uri_lst_t &uri_list = popts_.uri_list ();
  const long int port = popts_.port ();
  const bool shuffle = popts_.shuffle ();
  const bool recurse = popts_.recurse ();
  const bool icy_metadata = popts_.icy_metadata ();
  const std::string &sampling_rates = popts_.sampling_rates ();
  const std::vector< int > &sampling_rate_list = popts_.sampling_rate_list ();
  const std::string &bitrates = popts_.bitrates ();
  const std::vector< std::string > &bitrate_list = popts_.bitrate_list ();
  const std::string &station_name = popts_.station_name ();
  const std::string &station_genre = popts_.station_genre ();

  print_banner ();

  uri_lst_t file_list;
  std::string hostname;
  std::string ip_address;
  std::string error_msg;
  file_extension_lst_t extension_list;
  extension_list.insert (".mp3");

  // Create a playlist
  BOOST_FOREACH (std::string uri, uri_list)
  {
    if (!tizplaylist_t::assemble_play_list (
            uri, shuffle, recurse, extension_list, file_list, error_msg))
    {
      TIZ_PRINTF_C01 ("%s (%s).", error_msg.c_str (), uri.c_str ());
      player_exit_failure ();
    }
  }

  (void)daemonize_if_requested ();

  // Retrieve the hostname and ip address
  if (!get_host_name_and_ip (hostname, ip_address, error_msg))
  {
    TIZ_PRINTF_C01 ("%s.", error_msg.c_str ());
    player_exit_failure ();
  }

  fprintf (stdout, "[%s]: Server streaming on http://%s:%ld\n",
           station_name.c_str (), hostname.c_str (), port);

  fprintf (stdout, "[%s]: Streaming media with sampling rates [%s].\n",
           station_name.c_str (),
           sampling_rates.empty () ? "ANY" : sampling_rates.c_str ());

  if (!bitrate_list.empty () || bitrate_list.size () == TIZ_MAX_BITRATE_MODES)
  {
    fprintf (stdout, "[%s]: Streaming media with bitrate modes [%s].\n",
             station_name.c_str (), bitrates.c_str ());
  }
  fprintf (stdout, "\n");

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (file_list));

  assert (playlist);
  playlist->print_info ();

  // Here we'll only process one encoding, that is mp3... so enable loop
  // playback to ensure that the graph does not stop to get back to the
  // manager at the end of the playlist.
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t config
      = boost::make_shared< tiz::graph::httpservconfig > (
          playlist, hostname, ip_address, port, sampling_rate_list,
          bitrate_list, station_name, station_genre, icy_metadata);

  // Instantiate the http streaming manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::httpservmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input_while_streaming (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::decode_stream ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const uri_lst_t &uri_list = popts_.uri_list ();
  const uint32_t unused_buffer_seconds = 0; // this is not used during casting

  (void)daemonize_if_requested ();
  print_banner ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t config
    = boost::make_shared< tiz::graph::config > (playlist, unused_buffer_seconds);

  // Instantiate the streaming client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::httpclntmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input_while_streaming (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

#ifdef HAVE_LIBSPOTIFY
OMX_ERRORTYPE
tiz::playapp::spotify_stream ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const std::string user (popts_.spotify_user ());
  const std::string owner (popts_.spotify_owner ());
  std::string pass (popts_.spotify_password ());
  std::string proxy_server = popts_.proxy_server ();
  std::string proxy_user = popts_.proxy_user ();
  std::string proxy_pass = popts_.proxy_password ();
  const bool recover_lost_token = popts_.spotify_recover_lost_token ();
  const bool allow_explicit_tracks = popts_.spotify_allow_explicit_tracks ();
  const uint32_t preferred_bitrate = popts_.spotify_preferred_bitrate ();
  const uri_lst_t &uri_list = popts_.spotify_playlist_container ();
  const OMX_TIZONIA_AUDIO_SPOTIFYPLAYLISTTYPE playlist_type
      = popts_.spotify_playlist_type ();

  print_banner ();

  // If a username was supplied without a password, prompt for one
  if (!user.empty () && pass.empty ())
  {
    std::string msg (user);
    msg.append ("'s password:");
    pass.assign (getpass (msg.c_str ()));
    printf ("\n");
  }

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t config
    = boost::shared_ptr< tiz::graph::spotifyconfig > (new tiz::graph::spotifyconfig(
          playlist, user, pass, proxy_server, proxy_user, proxy_pass,
          playlist_type, owner, recover_lost_token, allow_explicit_tracks,
          preferred_bitrate));

  // Instantiate the streaming client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::spotifymgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}
#else
OMX_ERRORTYPE
tiz::playapp::spotify_stream ()
{
  return OMX_ErrorNone;
}
#endif

OMX_ERRORTYPE
tiz::playapp::gmusic_stream ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const std::string user (popts_.gmusic_user ());
  std::string pass (popts_.gmusic_password ());
  std::string device_id (popts_.gmusic_device_id ());
  std::string additional_keywords (popts_.gmusic_additional_keywords ());
  const uri_lst_t &uri_list = popts_.gmusic_playlist_container ();
  const OMX_TIZONIA_AUDIO_GMUSICPLAYLISTTYPE playlist_type
      = popts_.gmusic_playlist_type ();
  const bool is_unlimited_search = popts_.gmusic_is_unlimited_search ();
  const uint32_t buffer_seconds = popts_.gmusic_buffer_seconds ();

  print_banner ();

  // If a username was supplied without a password, prompt for one
  if (!user.empty () && pass.empty ())
  {
    std::string msg (user);
    msg.append ("'s password:");
    pass.assign (getpass (msg.c_str ()));
    printf ("\n");
  }

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t config = boost::make_shared< tiz::graph::gmusicconfig > (
      playlist, buffer_seconds, user, pass, device_id, playlist_type,
      additional_keywords, is_unlimited_search);

  // Instantiate the streaming client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::gmusicmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::scloud_stream ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const std::string token (popts_.scloud_oauth_token ());
  const uri_lst_t &uri_list = popts_.scloud_playlist_container ();
  const OMX_TIZONIA_AUDIO_SOUNDCLOUDPLAYLISTTYPE playlist_type
      = popts_.scloud_playlist_type ();
  const uint32_t buffer_seconds = popts_.scloud_buffer_seconds ();

  print_banner ();

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t config = boost::make_shared< tiz::graph::scloudconfig > (
      playlist, buffer_seconds, token, playlist_type);

  // Instantiate the streaming client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::scloudmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::tunein_stream ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const std::string api_key;
  const uri_lst_t &uri_list = popts_.tunein_playlist_container ();
  const OMX_TIZONIA_AUDIO_TUNEINPLAYLISTTYPE playlist_type
      = popts_.tunein_playlist_type ();
  const OMX_TIZONIA_AUDIO_TUNEINSEARCHTYPE search_type
      = popts_.tunein_search_type ();
  const uint32_t buffer_seconds = popts_.tunein_buffer_seconds ();

  print_banner ();

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t config = boost::make_shared< tiz::graph::tuneinconfig > (
      playlist, buffer_seconds, api_key, playlist_type, search_type);

  // Instantiate the streaming client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::tuneinmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::youtube_stream ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const uri_lst_t &uri_list = popts_.youtube_playlist_container ();
  const OMX_TIZONIA_AUDIO_YOUTUBEPLAYLISTTYPE playlist_type
      = popts_.youtube_playlist_type ();
  const std::string api_key = popts_.youtube_api_key ();
  const uint32_t buffer_seconds = popts_.youtube_buffer_seconds ();

  print_banner ();

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t config
      = boost::make_shared< tiz::graph::youtubeconfig > (
          api_key, playlist, buffer_seconds, playlist_type);

  // Instantiate the streaming client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::youtubemgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::plex_stream ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const std::string base_url (popts_.plex_base_url ());
  const std::string token (popts_.plex_token ());
  const std::string section (popts_.plex_section ());
  const uri_lst_t &uri_list = popts_.plex_playlist_container ();
  const OMX_TIZONIA_AUDIO_PLEXPLAYLISTTYPE playlist_type
      = popts_.plex_playlist_type ();
  const uint32_t buffer_seconds = popts_.plex_buffer_seconds ();

  print_banner ();

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t config = boost::make_shared< tiz::graph::plexconfig > (
      playlist, buffer_seconds, base_url, token, section, playlist_type);

  // Instantiate the streaming client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::plexmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::iheart_stream ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const std::string api_key;
  const uri_lst_t &uri_list = popts_.iheart_playlist_container ();
  const OMX_TIZONIA_AUDIO_IHEARTPLAYLISTTYPE playlist_type
      = popts_.iheart_playlist_type ();
  const uint32_t buffer_seconds = popts_.iheart_buffer_seconds ();

  print_banner ();

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t config = boost::make_shared< tiz::graph::iheartconfig > (
      playlist, buffer_seconds, api_key, playlist_type);

  // Instantiate the streaming client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::iheartmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::http_stream_chromecast ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const uri_lst_t &uri_list = popts_.uri_list ();
  const std::string cc_name_or_ip (popts_.chromecast_name_or_ip ());
  const uint32_t unused_buffer_seconds = 0; // this is not used during casting

  (void)daemonize_if_requested ();
  print_banner ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t service_config
      = boost::make_shared< tiz::graph::config > (playlist,
                                                  unused_buffer_seconds);

  tizgraphconfig_ptr_t config
      = boost::make_shared< tiz::graph::chromecastconfig > (
          cc_name_or_ip, service_config,
          tiz::graph::chromecastconfig::ConfigHttpStreaming);

  // Instantiate the chromecast client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::chromecastmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::gmusic_stream_chromecast ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const std::string user (popts_.gmusic_user ());
  std::string pass (popts_.gmusic_password ());
  std::string device_id (popts_.gmusic_device_id ());
  std::string additional_keywords (popts_.gmusic_additional_keywords ());
  const uri_lst_t &uri_list = popts_.gmusic_playlist_container ();
  const OMX_TIZONIA_AUDIO_GMUSICPLAYLISTTYPE playlist_type
      = popts_.gmusic_playlist_type ();
  const bool is_unlimited_search = popts_.gmusic_is_unlimited_search ();
  const std::string cc_name_or_ip (popts_.chromecast_name_or_ip ());
  const uint32_t unused_buffer_seconds = 0; // this is not used during casting

  print_banner ();

  // If a username was supplied without a password, prompt for one
  if (!user.empty () && pass.empty ())
  {
    std::string msg (user);
    msg.append ("'s password:");
    pass.assign (getpass (msg.c_str ()));
    printf ("\n");
  }

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t service_config
      = boost::make_shared< tiz::graph::gmusicconfig > (
          playlist, unused_buffer_seconds, user, pass, device_id, playlist_type,
          additional_keywords, is_unlimited_search);

  tizgraphconfig_ptr_t config
      = boost::make_shared< tiz::graph::chromecastconfig > (
          cc_name_or_ip, service_config,
          tiz::graph::chromecastconfig::ConfigGoogleMusic);

  // Instantiate the chromecast client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::chromecastmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::scloud_stream_chromecast ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const std::string token (popts_.scloud_oauth_token ());
  const uri_lst_t &uri_list = popts_.scloud_playlist_container ();
  const OMX_TIZONIA_AUDIO_SOUNDCLOUDPLAYLISTTYPE playlist_type
      = popts_.scloud_playlist_type ();
  const std::string cc_name_or_ip (popts_.chromecast_name_or_ip ());
  const uint32_t unused_buffer_seconds = 0; // this is not used during casting

  print_banner ();

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t service_config
      = boost::make_shared< tiz::graph::scloudconfig > (
          playlist, unused_buffer_seconds, token, playlist_type);

  tizgraphconfig_ptr_t config
      = boost::make_shared< tiz::graph::chromecastconfig > (
          cc_name_or_ip, service_config,
          tiz::graph::chromecastconfig::ConfigSoundCloud);

  // Instantiate the chromecast client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::chromecastmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::tunein_stream_chromecast ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const std::string api_key;
  const uri_lst_t &uri_list = popts_.tunein_playlist_container ();
  const OMX_TIZONIA_AUDIO_TUNEINPLAYLISTTYPE playlist_type
      = popts_.tunein_playlist_type ();
  const OMX_TIZONIA_AUDIO_TUNEINSEARCHTYPE search_type
      = popts_.tunein_search_type ();
  const std::string cc_name_or_ip (popts_.chromecast_name_or_ip ());
  const uint32_t unused_buffer_seconds = 0; // this is not used during casting

  print_banner ();

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t service_config
      = boost::make_shared< tiz::graph::tuneinconfig > (
          playlist, unused_buffer_seconds, api_key, playlist_type, search_type);

  tizgraphconfig_ptr_t config
      = boost::make_shared< tiz::graph::chromecastconfig > (
          cc_name_or_ip, service_config,
          tiz::graph::chromecastconfig::ConfigTunein);

  // Instantiate the chromecast client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::chromecastmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::youtube_stream_chromecast ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const std::string cc_name_or_ip (popts_.chromecast_name_or_ip ());
  const uri_lst_t &uri_list = popts_.youtube_playlist_container ();
  const OMX_TIZONIA_AUDIO_YOUTUBEPLAYLISTTYPE playlist_type
      = popts_.youtube_playlist_type ();
  const std::string api_key = popts_.youtube_api_key ();
  const uint32_t unused_buffer_seconds = 0; // this is not used during casting

  print_banner ();

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t service_config
      = boost::make_shared< tiz::graph::youtubeconfig > (
          api_key, playlist, unused_buffer_seconds, playlist_type);

  tizgraphconfig_ptr_t config
      = boost::make_shared< tiz::graph::chromecastconfig > (
          cc_name_or_ip, service_config,
          tiz::graph::chromecastconfig::ConfigYouTube);

  // Instantiate the chromecast client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::chromecastmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::plex_stream_chromecast ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const std::string base_url (popts_.plex_base_url ());
  const std::string token (popts_.plex_token ());
  const std::string section (popts_.plex_section ());
  const uri_lst_t &uri_list = popts_.plex_playlist_container ();
  const OMX_TIZONIA_AUDIO_PLEXPLAYLISTTYPE playlist_type
      = popts_.plex_playlist_type ();
  const std::string cc_name_or_ip (popts_.chromecast_name_or_ip ());
  const uint32_t unused_buffer_seconds = 0; // this is not used during casting

  print_banner ();

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t service_config
      = boost::make_shared< tiz::graph::plexconfig > (
          playlist, unused_buffer_seconds, base_url, token, section,
          playlist_type);

  tizgraphconfig_ptr_t config
      = boost::make_shared< tiz::graph::chromecastconfig > (
          cc_name_or_ip, service_config,
          tiz::graph::chromecastconfig::ConfigPlex);

  // Instantiate the chromecast client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::chromecastmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

OMX_ERRORTYPE
tiz::playapp::iheart_stream_chromecast ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const bool shuffle = popts_.shuffle ();
  const std::string api_key;
  const uri_lst_t &uri_list = popts_.iheart_playlist_container ();
  const OMX_TIZONIA_AUDIO_IHEARTPLAYLISTTYPE playlist_type
      = popts_.iheart_playlist_type ();
  const std::string cc_name_or_ip (popts_.chromecast_name_or_ip ());
  const uint32_t unused_buffer_seconds = 0; // this is not used during casting

  print_banner ();

  // daemon support
  (void)daemonize_if_requested ();

  tizplaylist_ptr_t playlist
      = boost::make_shared< tiz::playlist > (tiz::playlist (uri_list, shuffle));

  assert (playlist);
  playlist->set_loop_playback (true);

  tizgraphconfig_ptr_t service_config
      = boost::make_shared< tiz::graph::iheartconfig > (
          playlist, unused_buffer_seconds, api_key, playlist_type);

  tizgraphconfig_ptr_t config
      = boost::make_shared< tiz::graph::chromecastconfig > (
          cc_name_or_ip, service_config,
          tiz::graph::chromecastconfig::ConfigIheart);

  // Instantiate the chromecast client manager
  tiz::graphmgr::mgr_ptr_t p_mgr
      = boost::make_shared< tiz::graphmgr::chromecastmgr > (config);

  // TODO: Check return codes
  p_mgr->init (playlist, graphmgr_termination_cback ());
  p_mgr->start ();

  while (ETIZPlayUserQuit != player_wait_for_user_input (p_mgr))
  {
  }

  p_mgr->quit ();
  p_mgr->deinit ();

  return rc;
}

void tiz::playapp::print_banner () const
{
  popts_.print_version ();
}
