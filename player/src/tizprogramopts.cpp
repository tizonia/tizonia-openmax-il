/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizprogramopts.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Program options parsing utility.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/mem_fn.hpp>
#include <boost/bind.hpp>

#include <OMX_TizoniaExt.h>
#include <tizplatform.h>

#include "tizprogramopts.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.programopts"
#endif

#define PO_RETURN_IF_FAIL(expr) \
  do                            \
  {                             \
    if (!(expr))                \
    {                           \
      return EXIT_FAILURE;      \
    }                           \
  } while (0)

namespace po = boost::program_options;

namespace
{
  const int TIZ_STREAMING_SERVER_DEFAULT_PORT = 8010;
  const int TIZ_MAX_BITRATE_MODES = 2;

  struct program_option_is_defaulted
  {
    explicit program_option_is_defaulted (
        const boost::program_options::variables_map &vm)
      : vm_ (vm)
    {
    }

    bool operator()(const std::string &option) const
    {
      return vm_[option].defaulted ();
    }

  private:
    const boost::program_options::variables_map &vm_;
  };

  bool is_valid_sampling_rate (const int sampling_rate)
  {
    bool rc = false;
    switch (sampling_rate)
    {
      case 8000:
      case 11025:
      case 12000:
      case 16000:
      case 22050:
      case 24000:
      case 32000:
      case 44100:
      case 48000:
      case 96000:
      {
        rc = true;
        break;
      }
      default:
      {
        break;
      }
    };
    return rc;
  }

  bool is_valid_sampling_rate_list (
      const std::vector< std::string > &rate_strings, std::vector< int > &rates)
  {
    bool rc = true;
    rates.clear ();
    for (unsigned int i = 0; i < rate_strings.size () && rc; ++i)
    {
      rates.push_back (boost::lexical_cast< int >(rate_strings[i]));
      rc = is_valid_sampling_rate (rates[i]);
    }
    rc &= !rates.empty ();
    return rc;
  }

  bool is_valid_bitrate_list (const std::vector< std::string > &rate_strings)
  {
    bool rc = true;
    for (unsigned int i = 0; i < rate_strings.size (); ++i)
    {
      if (!(0 == rate_strings[i].compare ("CBR")
            || 0 == rate_strings[i].compare ("VBR")))
      {
        rc = false;
        break;
      }
    }
    rc &= !rate_strings.empty ();
    rc &= (rate_strings.size () <= TIZ_MAX_BITRATE_MODES);
    return rc;
  }

  bool omx_conflicting_options (const po::variables_map &vm, const char *opt1,
                                const char *opt2)
  {
    bool rc = false;
    if (vm.count (opt1) && !vm[opt1].defaulted () && vm.count (opt2)
        && !vm[opt2].defaulted ())
    {
      rc = true;
    }
    return rc;
  }

  void concat_option_lists (std::vector< std::string > &a,
                            const std::vector< std::string > &b)
  {
    a.insert (a.end (), b.begin (), b.end ());
  }

  // computes a - b = diff
  void diff_option_lists (const std::vector< std::string > &a,
                          const std::vector< std::string > &b,
                          std::vector< std::string > &diff)
  {
    std::set_difference (a.begin (), a.end (), b.begin (), b.end (),
                         std::back_inserter (diff));
#ifdef _DEBUG
    BOOST_FOREACH (std::string elem, a)
    {
      TIZ_PRINTF_DBG_RED ("a [%s]\n", elem.c_str ());
    }
    BOOST_FOREACH (std::string elem, b)
    {
      TIZ_PRINTF_DBG_RED ("b [%s]\n", elem.c_str ());
    }
    BOOST_FOREACH (std::string elem, diff)
    {
      TIZ_PRINTF_DBG_RED ("diff [%s]\n", elem.c_str ());
    }
#endif
  }

  void sort_option_list (std::vector< std::string > &a)
  {
    std::sort (a.begin (), a.end ());
  }

  bool is_valid_options_combination (
      std::vector< std::string > &valid_options,
      const std::vector< std::string > &given_options)
  {
    bool outcome = true;

    sort_option_list (valid_options);

    std::vector< std::string > diff;
    diff_option_lists (given_options, valid_options, diff);

    if (!diff.empty ())
    {
      outcome = false;
    }
    TIZ_PRINTF_DBG_RED ("outcome = [%s]\n",
                          outcome ? "SUCCESS" : "FAILURE");
    return outcome;
  }

  void retrieve_config_from_rc_file (const char *rc_section, const char *rc_key,
                                     std::string &container)
  {
    assert (rc_section);
    assert (rc_key);
    const char *p_key = tiz_rcfile_get_value (rc_section, rc_key);
    if (p_key)
    {
      container.assign (p_key);
    }
  }
}

tiz::programopts::programopts (int argc, char *argv[])
  : argc_ (argc),
    argv_ (argv),
    option_handlers_map_ (),
    vm_ (),
    global_ ("Global options"),
    debug_ ("Debug options"),
    omx_ ("OpenMAX IL options"),
    server_ ("Audio streaming server options"),
    client_ ("Audio streaming client options"),
    spotify_ ("Spotify options (Spotify Premium required)"),
    gmusic_ ("Google Play Music options"),
    scloud_ ("SoundCloud options"),
    input_ ("Intput urioption"),
    positional_ (),
    help_option_ ("help"),
    recurse_ (false),
    shuffle_ (false),
    daemon_ (false),
    log_dir_ (),
    debug_info_ (false),
    comp_name_ (),
    role_name_ (),
    port_ (TIZ_STREAMING_SERVER_DEFAULT_PORT),
    station_name_ ("Tizonia Radio"),
    station_genre_ ("Unknown Genre"),
    no_icy_metadata_ (false),
    bitrates_ (),
    bitrate_list_ (),
    sampling_rates_ (),
    sampling_rate_list_ (),
    uri_list_ (),
    spotify_user_ (),
    spotify_pass_ (),
    spotify_playlist_ (),
    spotify_playlist_container_ (),
    gmusic_user_ (),
    gmusic_pass_ (),
    gmusic_device_id_ (),
    gmusic_artist_ (),
    gmusic_album_ (),
    gmusic_playlist_ (),
    gmusic_station_ (),
    gmusic_genre_ (),
    gmusic_promoted_ (),
    gmusic_feeling_lucky_station_ (),
    gmusic_playlist_container_ (),
    gmusic_playlist_type_ (OMX_AUDIO_GmusicPlaylistTypeUnknown),
    gmusic_is_all_access_search_ (false),
    scloud_user_ (),
    scloud_pass_ (),
    scloud_user_stream_ (),
    scloud_user_likes_ (),
    scloud_user_playlist_ (),
    scloud_creator_ (),
    scloud_tracks_ (),
    scloud_playlists_ (),
    scloud_genres_ (),
    scloud_tags_ (),
    scloud_playlist_container_ (),
    scloud_playlist_type_ (OMX_AUDIO_SoundCloudPlaylistTypeUnknown),
    consume_functions_ (),
    all_global_options_ (),
    all_debug_options_ (),
    all_omx_options_ (),
    all_streaming_server_options_ (),
    all_streaming_client_options_ (),
    all_spotify_client_options_ (),
    all_gmusic_client_options_ (),
    all_scloud_client_options_ (),
    all_input_uri_options_ (),
    all_given_options_ ()
{
  init_global_options ();
  init_debug_options ();
  init_omx_options ();
  init_streaming_server_options ();
  init_streaming_client_options ();
  init_spotify_options ();
  init_gmusic_options ();
  init_scloud_options ();
  init_input_uri_option ();
}

int tiz::programopts::consume ()
{
  int rc = EXIT_FAILURE;
  unsigned int given_options_count = 0;
  std::string error_msg;
  try
  {
    bool done = false;
    given_options_count = parse_command_line (argc_, argv_);
    if (given_options_count)
    {
      BOOST_FOREACH (consume_function_t consume_options, consume_functions_)
      {
        rc = consume_options (done, error_msg);
        if (done)
        {
          break;
        }
      }
    }
  }
  catch (std::exception &e)
  {
    error_msg.assign (e.what ());
  }

  if (EXIT_FAILURE == rc)
  {
    if (!given_options_count)
    {
      print_usage_help ();
    }
    else
    {
      if (error_msg.empty ())
      {
        error_msg.assign ("Invalid combination of program options.");
      }
      print_version ();
      print_license ();
      TIZ_PRINTF_RED ("%s\n\n", error_msg.c_str ());
    }
  }

  return rc;
}

void tiz::programopts::print_version () const
{
  TIZ_PRINTF_BLU ("tizonia %s. Copyright (C) 2015 Juan A. Rubio\n",
                  PACKAGE_VERSION);
  TIZ_PRINTF_BLU ("This software is part of the Tizonia project <http://tizonia.org>\n\n");
}

void tiz::programopts::print_usage_help () const
{
  print_version ();
  print_license ();
  std::cout << " " << "Help topics:" << "\n\n";
  std::cout << "  " << "global        Global options available with most features." << "\n";
  std::cout << "  " << "openmax       Various OpenMAX IL query options." << "\n";
  std::cout << "  " << "server        SHOUTcast/ICEcast streaming server options." << "\n";
  std::cout << "  " << "client        SHOUTcast/ICEcast streaming client options." << "\n";
  std::cout << "  " << "spotify       Spotify options." << "\n";
  std::cout << "  " << "googlemusic   Google Play Music options." << "\n";
  std::cout << "  " << "soundcloud    SoundCloud options." << "\n";
  std::cout << "  " << "keyboard      Keyboard control." << "\n";
  std::cout << "  " << "config        Configuration files." << "\n";
  std::cout << "  " << "examples      Some command-line examples." << "\n";

  std::cout << "\n" << "Use \"tizonia --help topic\"." << "\n";
}

void tiz::programopts::print_usage_feature (po::options_description &desc) const
{
  print_version ();
  print_license ();
  std::cout << desc << "\n";
}

void tiz::programopts::print_usage_keyboard () const
{
  print_version ();
  print_license ();
  printf ("Keyboard control:\n\n");
  printf ("   [p] skip to previous file.\n");
  printf ("   [n] skip to next file.\n");
  printf ("   [SPACE] pause playback.\n");
  printf ("   [+/-] increase/decrease volume.\n");
  printf ("   [m] mute.\n");
  printf ("   [q] quit.\n");
  printf ("\n");
}

void tiz::programopts::print_usage_config () const
{
}

void tiz::programopts::print_usage_examples () const
{
  print_version ();
  print_license ();
  printf ("Examples:\n\n");
  printf (" tizonia ~/Music\n\n");
  printf ("    * Decodes every supported file in the '~/Music' directory)\n");
  printf ("    * File formats currently supported for playback:\n");
  printf (
      "      * mp3, mp2, m2a, aac, (.aac only) flac (.flac, .ogg, .oga),\n"
      "        opus (.opus, .ogg, .oga), vorbis (.ogg, .oga), wav, aiff, aif.\n");
  printf (
      "\n tizonia --sampling-rates=44100,48000 -p 8011 --stream ~/Music\n\n");
  printf ("    * Streams files from the '~/Music' directory.\n");
  printf ("    * File formats currently supported for streaming: mp3.\n");
  printf ("    * Sampling rates other than [44100,4800] are ignored.\n");
  printf ("\n");
}

void tiz::programopts::set_option_handler (const std::string &option,
                                           const option_handler_t handler)
{
  option_handlers_map_.insert (
      std::make_pair< std::string, option_handler_t >(option, handler));
}

bool tiz::programopts::shuffle () const
{
  return shuffle_;
}

bool tiz::programopts::recurse () const
{
  return recurse_;
}

bool tiz::programopts::daemon () const
{
  return daemon_;
}

const std::string &tiz::programopts::log_dir () const
{
  return log_dir_;
}

bool tiz::programopts::debug_info () const
{
  return debug_info_;
}

const std::string &tiz::programopts::component_name () const
{
  return comp_name_;
}

const std::string &tiz::programopts::component_role () const
{
  return role_name_;
}

int tiz::programopts::port () const
{
  return port_;
}

const std::string &tiz::programopts::station_name () const
{
  return station_name_;
}

const std::string &tiz::programopts::station_genre () const
{
  return station_genre_;
}

bool tiz::programopts::icy_metadata () const
{
  return !no_icy_metadata_;
}

const std::string &tiz::programopts::bitrates () const
{
  return bitrates_;
}

const std::vector< std::string > &tiz::programopts::bitrate_list () const
{
  return bitrate_list_;
}

const std::string &tiz::programopts::sampling_rates () const
{
  return sampling_rates_;
}

const std::vector< int > &tiz::programopts::sampling_rate_list () const
{
  return sampling_rate_list_;
}

const std::vector< std::string > &tiz::programopts::uri_list () const
{
  return uri_list_;
}

const std::string &tiz::programopts::spotify_user () const
{
  return spotify_user_;
}

const std::string &tiz::programopts::spotify_password () const
{
  return spotify_pass_;
}

const std::vector< std::string > &
    tiz::programopts::spotify_playlist_container ()
{
  spotify_playlist_container_.clear ();
  spotify_playlist_container_.push_back (spotify_playlist_);
  return spotify_playlist_container_;
}

const std::string &tiz::programopts::gmusic_user () const
{
  return gmusic_user_;
}

const std::string &tiz::programopts::gmusic_password () const
{
  return gmusic_pass_;
}

const std::string &tiz::programopts::gmusic_device_id () const
{
  return gmusic_device_id_;
}

const std::vector< std::string > &
    tiz::programopts::gmusic_playlist_container ()
{
  gmusic_playlist_container_.clear ();
  if (!gmusic_artist_.empty ())
    {
      gmusic_playlist_container_.push_back (gmusic_artist_);
    }
  else if (!gmusic_album_.empty ())
    {
      gmusic_playlist_container_.push_back (gmusic_album_);
    }
  else if (!gmusic_playlist_.empty ())
    {
      gmusic_playlist_container_.push_back (gmusic_playlist_);
    }
  else if (!gmusic_station_.empty ())
    {
      gmusic_playlist_container_.push_back (gmusic_station_);
    }
  else if (!gmusic_genre_.empty ())
    {
      gmusic_playlist_container_.push_back (gmusic_genre_);
    }
  else if (!gmusic_promoted_.empty ())
    {
      // With gmusic promoted songs option, no playlist "name" is actually
      // required. But this helps keeping track of what is in the container.
      gmusic_playlist_container_.push_back (gmusic_promoted_);
    }
  else if (!gmusic_feeling_lucky_station_.empty ())
    {
      gmusic_playlist_container_.push_back (gmusic_feeling_lucky_station_);
    }
  else
    {
      assert (0);
    }
  return gmusic_playlist_container_;
}

OMX_TIZONIA_AUDIO_GMUSICPLAYLISTTYPE
tiz::programopts::gmusic_playlist_type ()
{
  if (!gmusic_artist_.empty ())
    {
      gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypeArtist;
    }
  else if (!gmusic_album_.empty ())
    {
      gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypeAlbum;
    }
  else if (!gmusic_playlist_.empty ())
    {
      gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypeUser;
    }
  else if (!gmusic_station_.empty () || !gmusic_feeling_lucky_station_.empty ())
    {
      gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypeStation;
    }
  else if (!gmusic_genre_.empty ())
    {
      gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypeGenre;
    }
  else if (!gmusic_promoted_.empty ())
    {
      gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypePromotedTracks;
    }
  else
    {
      assert (0);
    }

  return gmusic_playlist_type_;
}

bool tiz::programopts::gmusic_is_all_access_search () const
{
  return gmusic_is_all_access_search_;
}

const std::string &tiz::programopts::scloud_user () const
{
  return scloud_user_;
}

const std::string &tiz::programopts::scloud_password () const
{
  return scloud_pass_;
}

const std::vector< std::string > &
    tiz::programopts::scloud_playlist_container ()
{
  scloud_playlist_container_.clear ();
  if (!scloud_user_stream_.empty ())
    {
      scloud_playlist_container_.push_back (scloud_user_stream_);
    }
  else if (!scloud_user_likes_.empty ())
    {
      scloud_playlist_container_.push_back (scloud_user_likes_);
    }
  else if (!scloud_user_playlist_.empty ())
    {
      scloud_playlist_container_.push_back (scloud_user_playlist_);
    }
  else if (!scloud_creator_.empty ())
    {
      scloud_playlist_container_.push_back (scloud_creator_);
    }
  else if (!scloud_tracks_.empty ())
    {
      scloud_playlist_container_.push_back (scloud_tracks_);
    }
  else if (!scloud_playlists_.empty ())
    {
      scloud_playlist_container_.push_back (scloud_playlists_);
    }
  else if (!scloud_genres_.empty ())
    {
      scloud_playlist_container_.push_back (scloud_genres_);
    }
  else if (!scloud_tags_.empty ())
    {
      scloud_playlist_container_.push_back (scloud_tags_);
    }
  else
    {
      assert (0);
    }
  return scloud_playlist_container_;
}

OMX_TIZONIA_AUDIO_SOUNDCLOUDPLAYLISTTYPE
tiz::programopts::scloud_playlist_type ()
{
  if (!scloud_user_stream_.empty ())
    {
      scloud_playlist_type_ = OMX_AUDIO_SoundCloudPlaylistTypeUserStream;
    }
  else if (!scloud_user_likes_.empty ())
    {
      scloud_playlist_type_ = OMX_AUDIO_SoundCloudPlaylistTypeUserLikes;
    }
  else if (!scloud_user_playlist_.empty ())
    {
      scloud_playlist_type_ = OMX_AUDIO_SoundCloudPlaylistTypeUserPlaylist;
    }
  else if (!scloud_creator_.empty ())
    {
      scloud_playlist_type_ = OMX_AUDIO_SoundCloudPlaylistTypeCreator;
    }
  else if (!scloud_tracks_.empty ())
    {
      scloud_playlist_type_ = OMX_AUDIO_SoundCloudPlaylistTypeTracks;
    }
  else if (!scloud_playlists_.empty ())
    {
      scloud_playlist_type_ = OMX_AUDIO_SoundCloudPlaylistTypePlaylists;
    }
  else if (!scloud_genres_.empty ())
    {
      scloud_playlist_type_ = OMX_AUDIO_SoundCloudPlaylistTypeGenres;
    }
  else if (!scloud_tags_.empty ())
    {
      scloud_playlist_type_ = OMX_AUDIO_SoundCloudPlaylistTypeTags;
    }
  else
    {
      assert (0);
    }

  return scloud_playlist_type_;
}

void tiz::programopts::print_license () const
{
  TIZ_PRINTF_GRN (
      "GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl.html>\n"
      "This is free software: you are free to change and redistribute it.\n"
      "There is NO WARRANTY, to the extent permitted by law.\n\n");
}

void tiz::programopts::init_global_options ()
{
  global_.add_options ()
      /* TIZ_CLASS_COMMENT: This is to avoid the clang formatter messing up
         these lines*/
      ("help,h", po::value< std::string >(&help_option_)
                     ->implicit_value (std::string ("help")),
       "Print a usage message for a specific help topic (e.g. global, "
       "openmax, server, spotify, googlemusic, soundcloud, etc).")
      /* TIZ_CLASS_COMMENT: */
      ("version,v", "Print the version information.")
      /* TIZ_CLASS_COMMENT: */
      ("recurse,r", po::bool_switch (&recurse_)->default_value (false),
       "Recursively process a given path.")
      /* TIZ_CLASS_COMMENT: */
      ("shuffle,s", po::bool_switch (&shuffle_)->default_value (false),
       "Shuffle the playlist.")
      /* TIZ_CLASS_COMMENT: */
      ("daemon,d", po::bool_switch (&daemon_)->default_value (false),
       "Run in the background.")
      /* TIZ_CLASS_COMMENT: */
      ;
  register_consume_function (&tiz::programopts::consume_global_options);
  // TODO: help and version are not included. These should be moved out of
  // "global" and into its own category: "info"
  all_global_options_ = boost::assign::list_of ("recurse")("shuffle")("daemon");
}

void tiz::programopts::init_debug_options ()
{
  debug_.add_options ()
      /* TIZ_CLASS_COMMENT: This is to avoid the clang formatter messing up
         these lines*/
      ("log-directory", po::value (&log_dir_),
       "The directory to be used for the debug trace file.")(
          "debug-info", po::bool_switch (&debug_info_)->default_value (false),
          "Print debug-related information.")
      /* TIZ_CLASS_COMMENT: */
      ;
  register_consume_function (&tiz::programopts::consume_debug_options);
  all_debug_options_ = boost::assign::list_of ("log-directory")("debug-info");
}

void tiz::programopts::init_omx_options ()
{
  omx_.add_options ()
      /* TIZ_CLASS_COMMENT: This is to avoid the clang formatter messing up
         these lines*/
      ("comp-list,L", "Enumerate all the OpenMAX IL components in the system.")
      /* TIZ_CLASS_COMMENT: */
      ("roles-of-comp,R", po::value (&comp_name_),
       "Display the OpenMAX IL roles found in component <arg>.")
      /* TIZ_CLASS_COMMENT: */
      ("comps-of-role,C", po::value (&role_name_),
       "Display the OpenMAX IL components that implement role <arg>.")
      /* TIZ_CLASS_COMMENT: */
      ;
  register_consume_function (&tiz::programopts::consume_omx_options);
  all_omx_options_
      = boost::assign::list_of ("comp-list")("roles-of-comp")("comps-of-role");
}

void tiz::programopts::init_streaming_server_options ()
{
  server_.add_options ()
      /* TIZ_CLASS_COMMENT: This is to avoid the clang formatter messing up
         these lines*/
      ("server",
       "Stream media files using the SHOUTcast/ICEcast streaming protocol.")
      /* TIZ_CLASS_COMMENT: */
      ("port,p", po::value (&port_),
       "TCP port to be used for Icecast/SHOUTcast streaming. Default: 8010.")
      /* TIZ_CLASS_COMMENT: */
      ("station-name", po::value (&station_name_),
       "The Icecast/SHOUTcast station name. Optional.")
      /* TIZ_CLASS_COMMENT: */
      ("station-genre", po::value (&station_genre_),
       "The Icecast/SHOUTcast station genre. Optional.")
      /* TIZ_CLASS_COMMENT: */
      ("no-icy-metadata", po::bool_switch (&no_icy_metadata_),
       "Disables Icecast/SHOUTcast metadata in the stream.")
      /* TIZ_CLASS_COMMENT: */
      ("bitrate-modes", po::value (&bitrates_),
       "A comma-separated list of "
       /* TIZ_CLASS_COMMENT: */
       "bitrate modes (e.g. 'CBR,VBR'). Only media with these bitrate modes will be "
       "in the playlist. Default: any.")
      /* TIZ_CLASS_COMMENT: */
      ("sampling-rates", po::value (&sampling_rates_),
       "A comma-separated list "
       /* TIZ_CLASS_COMMENT: */
       "of sampling rates. Only media with these rates will in the "
       "playlist. Default: any.")
      /* TIZ_CLASS_COMMENT: */
      ;

  // Give a default value to the bitrate list
  bitrates_ = std::string ("CBR,VBR");
  boost::split (bitrate_list_, bitrates_, boost::is_any_of (","));
  register_consume_function (
      &tiz::programopts::consume_streaming_server_options);
  all_streaming_server_options_ = boost::assign::list_of ("server")("port")(
      "station-name")("station-genre")("no-icy-metadata")("bitrate-modes")(
      "sampling-rates");
}

void tiz::programopts::init_streaming_client_options ()
{
  client_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("station-id", po::value (&station_name_),
       "Give a name/id to the remote stream.");
  register_consume_function (
      &tiz::programopts::consume_streaming_client_options);
  all_streaming_client_options_ = boost::assign::list_of ("station-id");
}

void tiz::programopts::init_spotify_options ()
{
  spotify_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("spotify-user", po::value (&spotify_user_),
      "Spotify user name  (not required if provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-password", po::value (&spotify_pass_),
       "Spotify user password  (not required if provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-playlist", po::value (&spotify_playlist_),
       "A playlist from the user's library.");
  register_consume_function (&tiz::programopts::consume_spotify_client_options);
  all_spotify_client_options_ = boost::assign::list_of ("spotify-user")(
      "spotify-password")("spotify-playlist");
}

void tiz::programopts::init_gmusic_options ()
{
  gmusic_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-user", po::value (&gmusic_user_),
      "Google Play Music user name (not required if provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-password", po::value (&gmusic_pass_),
       "Google Play Music user's password (not required if provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-device-id", po::value (&gmusic_device_id_),
       "Google Play Music device id (not required if provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-artist", po::value (&gmusic_artist_),
       "Play tracks from the user's library by artist.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-album", po::value (&gmusic_album_),
       "Play an album from the user's library.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-playlist", po::value (&gmusic_playlist_),
       "A playlist from the user's library.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-all-access-station", po::value (&gmusic_station_),
       "Search and play All Access stations found in the user's library.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-all-access-album", po::value (&gmusic_album_),
       "Search and play All Access tracks by album (best match only).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-all-access-artist", po::value (&gmusic_artist_),
       "Search and play All Access tracks by artist (best match only).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-all-access-tracks", po::value (&gmusic_playlist_),
       "Search and play All Access tracks by name (50 first matches only).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-all-access-genre", po::value (&gmusic_genre_),
       "Search and play All Access tracks by genre.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-all-access-feeling-lucky-station",
       "Play the user's All Access 'I'm Feeling Lucky' station.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-all-access-promoted-tracks",
       "Play All Access promoted tracks.");

  register_consume_function (&tiz::programopts::consume_gmusic_client_options);
  all_gmusic_client_options_ = boost::assign::list_of ("gmusic-user")
    ("gmusic-password")("gmusic-device-id")("gmusic-artist")("gmusic-album")
    ("gmusic-playlist")("gmusic-all-access-station")("gmusic-all-access-album")
    ("gmusic-all-access-artist")("gmusic-all-access-tracks")("gmusic-all-access-genre")
    ("gmusic-all-access-feeling-lucky-station")("gmusic-all-access-promoted-tracks");
}

void tiz::programopts::init_scloud_options ()
{
  scloud_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("soundcloud-user", po::value (&scloud_user_),
       "SoundCloud user's name (not required if provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("soundcloud-password", po::value (&scloud_pass_),
       "SoundCloud user's password (not required if provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("soundcloud-user-stream",
       "Play the tracks currently listed in the user's stream.")
      /* TIZ_CLASS_COMMENT: */
      ("soundcloud-user-likes",
       "Play the tracks liked by the user.")
      /* TIZ_CLASS_COMMENT: */
      ("soundcloud-user-playlist", po::value (&scloud_user_playlist_),
       "Play a playlist from the user's collection.")
      /* TIZ_CLASS_COMMENT: */
      ("soundcloud-creator", po::value (&scloud_creator_),
       "Search and play the top 50 tracks from a creator.")
      /* TIZ_CLASS_COMMENT: */
      ("soundcloud-tracks", po::value (&scloud_tracks_),
       "Search and play tracks by title (50 first matches only).")
      /* TIZ_CLASS_COMMENT: */
      ("soundcloud-playlists", po::value (&scloud_playlists_),
       "Search and play playlists by title.")
      /* TIZ_CLASS_COMMENT: */
      ("soundcloud-genres", po::value (&scloud_genres_),
       "Search and play genres top tracks (arg is a command-separated list).")
      ("soundcloud-tags", po::value (&scloud_tags_),
       "Search and play tags top tracks (arg is a command-separated list).");

  register_consume_function (&tiz::programopts::consume_scloud_client_options);
  all_scloud_client_options_ = boost::assign::list_of ("soundcloud-user")
    ("soundcloud-password")("soundcloud-user-stream")("soundcloud-user-likes")
    ("soundcloud-user-playlist") ("soundcloud-creator")("soundcloud-tracks")
    ("soundcloud-playlists") ("soundcloud-genres")("soundcloud-tags");
}

void tiz::programopts::init_input_uri_option ()
{
  input_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("input-uris",
       po::value< std::vector< std::string > >(&uri_list_)->multitoken (),
       "input file");
  positional_.add ("input-uris", -1);
  register_consume_function (&tiz::programopts::consume_local_decode_options);
  all_input_uri_options_ = boost::assign::list_of ("input-uris");
}

unsigned int tiz::programopts::parse_command_line (int argc, char *argv[])
{
  // Declare an options description instance which will include
  // all the options
  po::options_description all ("All options");
  all.add (global_)
      .add (debug_)
      .add (omx_)
      .add (server_)
      .add (client_)
      .add (spotify_)
      .add (gmusic_)
      .add (scloud_)
      .add (input_);
  po::parsed_options parsed = po::command_line_parser (argc, argv)
                                  .options (all)
                                  .positional (positional_)
                                  .run ();
  po::store (parsed, vm_);
  po::notify (vm_);
  uri_list_ = po::collect_unrecognized (parsed.options, po::include_positional);

  // Copy the keys of the vm_ map into the all_given_options_ vector
  boost::copy (vm_ | boost::adaptors::map_keys,
               std::back_inserter (all_given_options_));

  // ... now remove all defaulted options from all_given_options_
  std::vector< std::string >::iterator it
      = std::remove_if (all_given_options_.begin (), all_given_options_.end (),
                        program_option_is_defaulted (vm_));
  all_given_options_.erase (it, all_given_options_.end ());

  // ... and finally sort the list to enable binary searching
  sort_option_list (all_given_options_);

  // ... finally, return the number of non-default options provided by the user
  return all_given_options_.size ();
}

int tiz::programopts::consume_debug_options (bool &done, std::string &msg)
{
  int rc = EXIT_FAILURE;
  done = false;
  if (vm_.count ("log-directory"))
  {
    (void)call_handler (option_handlers_map_.find ("log-directory"));
    rc = EXIT_SUCCESS;
  }
  if (vm_.count ("debug-info") && debug_info_)
  {
    (void)call_handler (option_handlers_map_.find ("debug-info"));
    rc = EXIT_SUCCESS;
    done = true;
  }
  TIZ_PRINTF_DBG_RED ("debug-opts ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_global_options (bool &done,
                                               std::string & /* msg */)
{
  int rc = EXIT_FAILURE;
  done = false;
  if (vm_.count ("help"))
  {
    done = true;
    rc = EXIT_SUCCESS;
    if (0 == help_option_.compare ("global"))
      {
        print_usage_feature (global_);
      }
    else if (0 == help_option_.compare ("openmax"))
      {
        print_usage_feature (omx_);
      }
    else if (0 == help_option_.compare ("server"))
      {
        print_usage_feature (server_);
      }
    else if (0 == help_option_.compare ("client"))
      {
        print_usage_feature (client_);
      }
    else if (0 == help_option_.compare ("spotify"))
      {
        print_usage_feature (spotify_);
      }
    else if (0 == help_option_.compare ("googlemusic"))
      {
        print_usage_feature (gmusic_);
      }
    else if (0 == help_option_.compare ("soundcloud"))
      {
        print_usage_feature (scloud_);
      }
    else if (0 == help_option_.compare ("keyboard"))
      {
        print_usage_keyboard ();
      }
    else if (0 == help_option_.compare ("config"))
      {
        print_usage_config ();
      }
    else if (0 == help_option_.compare ("examples"))
      {
        print_usage_examples ();
      }
    else
      {
        print_usage_help ();
      }
  }
  else if (vm_.count ("version"))
  {
    print_version ();
    done = true;
    rc = EXIT_SUCCESS;
  }
  TIZ_PRINTF_DBG_RED ("global-opts ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_omx_options (bool &done, std::string &msg)
{
  int rc = EXIT_FAILURE;
  done = false;

  if (validate_omx_options ())
  {
    done = true;

    if (omx_conflicting_options (vm_, "comp-list", "roles-of-comp")
        || omx_conflicting_options (vm_, "comp-list", "comps-of-role")
        || omx_conflicting_options (vm_, "roles-of-comp", "comps-of-role"))
    {
      msg.assign (
          "Only one of '--comp-list', '--roles-of-comp', "
          "'--comps-of-role' can be specified.");
      rc = EXIT_FAILURE;
    }
    else
    {
      if (vm_.count ("comp-list"))
      {
        rc = call_handler (option_handlers_map_.find ("comp-list"));
      }
      else if (vm_.count ("roles-of-comp"))
      {
        rc = call_handler (option_handlers_map_.find ("roles-of-comp"));
      }
      else if (vm_.count ("comps-of-role"))
      {
        rc = call_handler (option_handlers_map_.find ("comps-of-role"));
      }
      rc = EXIT_SUCCESS;
    }
  }
  TIZ_PRINTF_DBG_RED ("omx-opts ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_streaming_server_options (bool &done,
                                                        std::string &msg)
{
  int rc = EXIT_FAILURE;
  done = false;

  if (validate_streaming_server_options ())
  {
    done = true;
    PO_RETURN_IF_FAIL (validate_port_argument (msg));
    PO_RETURN_IF_FAIL (validate_bitrates_argument (msg));
    PO_RETURN_IF_FAIL (validate_sampling_rates_argument (msg));
    rc = consume_input_file_uris_option ();
    if (EXIT_SUCCESS == rc)
    {
      rc = call_handler (option_handlers_map_.find ("serve-stream"));
    }
  }
  TIZ_PRINTF_DBG_RED ("serve-stream ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_streaming_client_options (bool &done,
                                                        std::string &msg)
{
  int rc = EXIT_FAILURE;
  done = false;
  rc = consume_input_http_uris_option ();
  if (EXIT_SUCCESS == rc)
  {
    done = true;
    rc = call_handler (option_handlers_map_.find ("decode-stream"));
  }
  TIZ_PRINTF_DBG_RED ("streaming-client ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_spotify_client_options (bool &done,
                                                      std::string &msg)
{
  int rc = EXIT_FAILURE;
  done = false;

  if (validate_spotify_client_options ())
  {
    done = true;

    if (spotify_user_.empty ())
      {
        retrieve_config_from_rc_file ("tizonia", "spotify.user", spotify_user_);
      }
    if (spotify_pass_.empty ())
      {
        retrieve_config_from_rc_file ("tizonia", "spotify.password", spotify_pass_);
      }

    if (spotify_user_.empty ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "Need to provide a Spotify user name.";
      msg.assign (oss.str ());
    }
    else if (!vm_.count ("spotify-playlist"))
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "A playlist must be specified.";
      msg.assign (oss.str ());
    }
    else
    {
      rc = call_handler (option_handlers_map_.find ("spotify-stream"));
    }
  }
  TIZ_PRINTF_DBG_RED ("spotify ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_gmusic_client_options (bool &done,
                                                     std::string &msg)
{
  int rc = EXIT_FAILURE;
  done = false;

  if (validate_gmusic_client_options ())
  {
    done = true;

    const int playlist_option_count = vm_.count ("gmusic-artist")
      + vm_.count ("gmusic-album") + vm_.count ("gmusic-playlist")
      + vm_.count ("gmusic-all-access-station") + vm_.count ("gmusic-all-access-album")
      + vm_.count ("gmusic-all-access-artist") + vm_.count ("gmusic-all-access-tracks")
      + vm_.count ("gmusic-all-access-genre") + vm_.count ("gmusic-all-access-feeling-lucky-station")
      + vm_.count ("gmusic-all-access-promoted-tracks") ;

    if (gmusic_user_.empty ())
      {
        retrieve_config_from_rc_file ("tizonia", "gmusic.user", gmusic_user_);
      }
    if (gmusic_pass_.empty ())
      {
        retrieve_config_from_rc_file ("tizonia", "gmusic.password", gmusic_pass_);
      }
    if (gmusic_device_id_.empty ())
      {
        retrieve_config_from_rc_file ("tizonia", "gmusic.device_id", gmusic_device_id_);
      }

    if (vm_.count ("gmusic-all-access-promoted-tracks"))
      {
        // This is not going to be used by the client code, but will help
        // in gmusic_playlist_type() to decide which playlist type value is returned.
        gmusic_promoted_.assign ("All Access promoted tracks");
      }

    if (vm_.count ("gmusic-all-access-feeling-lucky-station"))
      {
        gmusic_feeling_lucky_station_.assign ("I'm Feeling Lucky");
      }

      if (vm_.count ("gmusic-all-access-station")
          || vm_.count ("gmusic-all-access-album")
          || vm_.count ("gmusic-all-access-artist")
          || vm_.count ("gmusic-all-access-tracks")
          || vm_.count ("gmusic-all-access-genre"))
      {
        gmusic_is_all_access_search_ = true;
      }

    if (gmusic_user_.empty ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "Need to provide a Google Play Music user name.";
      msg.assign (oss.str ());
    }
    else if (gmusic_device_id_.empty ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "A device id must be provided.";
      msg.assign (oss.str ());
    }
    else if (playlist_option_count > 1)
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "Only one playlist type must be specified.";
      msg.assign (oss.str ());
    }
    else if (!playlist_option_count)
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "A playlist must be specified.";
      msg.assign (oss.str ());
    }
    else
    {
      rc = call_handler (option_handlers_map_.find ("gmusic-stream"));
    }
  }
  TIZ_PRINTF_DBG_RED ("gmusic ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_scloud_client_options (bool &done,
                                                     std::string &msg)
{
  int rc = EXIT_FAILURE;
  done = false;

  if (validate_scloud_client_options ())
  {
    done = true;

    const int playlist_option_count = vm_.count ("soundcloud-user-stream")
      + vm_.count ("soundcloud-user-likes") + vm_.count ("soundcloud-user-playlist")
      + vm_.count ("soundcloud-creator") + vm_.count ("soundcloud-tracks")
      + vm_.count("soundcloud-playlists") + vm_.count ("soundcloud-genres")
      + vm_.count ("soundcloud-tags");

    if (scloud_user_.empty ())
      {
        retrieve_config_from_rc_file ("tizonia", "soundcloud.user", scloud_user_);
      }
    if (scloud_pass_.empty ())
      {
        retrieve_config_from_rc_file ("tizonia", "soundcloud.password", scloud_pass_);
      }

    if (vm_.count ("soundcloud-user-stream"))
      {
        // This is not going to be used by the client code, but will help
        // in sclound_playlist_type() to decide which playlist type value is returned.
        scloud_user_stream_.assign ("SoundCloud user stream");
      }

    if (vm_.count ("soundcloud-user-likes"))
      {
        // This is not going to be used by the client code, but will help
        // in sclound_playlist_type() to decide which playlist type value is returned.
        scloud_user_likes_.assign ("SoundCloud user likes");
      }

    if (scloud_user_.empty ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "Need to provide a SoundCloud user name.";
      msg.assign (oss.str ());
    }
    else if (playlist_option_count > 1)
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "Only one playlist type must be specified.";
      msg.assign (oss.str ());
    }
    else if (!playlist_option_count)
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "A playlist must be specified.";
      msg.assign (oss.str ());
    }
    else
    {
      rc = call_handler (option_handlers_map_.find ("scloud-stream"));
    }
  }
  TIZ_PRINTF_DBG_RED ("scloud ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_local_decode_options (bool &done,
                                                    std::string &msg)
{
  int rc = EXIT_FAILURE;
  done = false;
  if (EXIT_SUCCESS == consume_input_file_uris_option ())
  {
    rc = EXIT_SUCCESS;
    done = true;
    rc = call_handler (option_handlers_map_.find ("decode-local"));
  }
  TIZ_PRINTF_DBG_RED ("decode-local ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_input_file_uris_option ()
{
  int rc = EXIT_FAILURE;
  if (vm_.count ("input-uris"))
  {
    rc = EXIT_SUCCESS;
    uri_list_ = vm_["input-uris"].as< std::vector< std::string > >();
  }
  return rc;
}

int tiz::programopts::consume_input_http_uris_option ()
{
  int rc = EXIT_FAILURE;
  if (vm_.count ("input-uris"))
  {
    uri_list_ = vm_["input-uris"].as< std::vector< std::string > >();
    bool all_ok = true;
    BOOST_FOREACH (std::string uri, uri_list_)
    {
      boost::xpressive::sregex http_s
          = boost::xpressive::sregex::compile ("^https?://");
      boost::xpressive::smatch what;
      if (!boost::xpressive::regex_search (uri, what, http_s))
      {
        all_ok = false;
        break;
      }
    }
    rc = all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  return rc;
}

bool tiz::programopts::validate_omx_options () const
{
  bool outcome = false;
  const unsigned int omx_opts_count = vm_.count ("comp-list")
                                      + vm_.count ("roles-of-comp")
                                      + vm_.count ("comps-of-role");

  std::vector< std::string > all_valid_options = all_omx_options_;
  concat_option_lists (all_valid_options, all_global_options_);
  concat_option_lists (all_valid_options, all_debug_options_);

  if (omx_opts_count > 0 && omx_opts_count <= 3
      && is_valid_options_combination (all_valid_options, all_given_options_))
  {
    outcome = true;
  }
  return outcome;
}

bool tiz::programopts::validate_streaming_server_options () const
{
  bool outcome = false;

  std::vector< std::string > all_valid_options = all_streaming_server_options_;
  concat_option_lists (all_valid_options, all_global_options_);
  concat_option_lists (all_valid_options, all_debug_options_);
  concat_option_lists (all_valid_options, all_input_uri_options_);

  if (vm_.count ("server")
      && is_valid_options_combination (all_valid_options, all_given_options_))
  {
    outcome = true;
  }
  return outcome;
}

bool tiz::programopts::validate_spotify_client_options () const
{
  bool outcome = false;
  unsigned int spotify_opts_count
      = vm_.count ("spotify-user") + vm_.count ("spotify-password")
        + vm_.count ("spotify-playlist") + vm_.count ("log-directory");

  std::vector< std::string > all_valid_options = all_spotify_client_options_;
  concat_option_lists (all_valid_options, all_global_options_);
  concat_option_lists (all_valid_options, all_debug_options_);

  if (spotify_opts_count > 0
      && is_valid_options_combination (all_valid_options, all_given_options_))
  {
    outcome = true;
  }
  return outcome;
}

bool tiz::programopts::validate_gmusic_client_options () const
{
  bool outcome = false;
  unsigned int gmusic_opts_count
      = vm_.count ("gmusic-user") + vm_.count ("gmusic-password")
        + vm_.count ("gmusic-device-id") + vm_.count ("gmusic-artist")
        + vm_.count ("gmusic-album") + vm_.count ("gmusic-playlist")
        + vm_.count ("gmusic-all-access-station") + vm_.count ("gmusic-all-access-album")
        + vm_.count ("gmusic-all-access-artist") + vm_.count ("gmusic-all-access-tracks")
        + vm_.count ("gmusic-all-access-genre") + vm_.count ("gmusic-all-access-feeling-lucky-station")
        + vm_.count ("gmusic-all-access-promoted-tracks")
        + vm_.count ("log-directory");

  std::vector< std::string > all_valid_options = all_gmusic_client_options_;
  concat_option_lists (all_valid_options, all_global_options_);
  concat_option_lists (all_valid_options, all_debug_options_);

  if (gmusic_opts_count > 0
      && is_valid_options_combination (all_valid_options, all_given_options_))
  {
    outcome = true;
  }
  TIZ_PRINTF_DBG_RED ("outcome = [%s]\n",
                      outcome ? "SUCCESS" : "FAILURE");
  return outcome;
}

bool tiz::programopts::validate_scloud_client_options () const
{
  bool outcome = false;
  unsigned int scloud_opts_count
      = vm_.count ("soundcloud-user") + vm_.count ("soundcloud-password")
        + vm_.count ("soundcloud-user-stream") + vm_.count ("soundcloud-user-likes")
        + vm_.count ("soundcloud-user-playlist") + vm_.count ("soundcloud-creator")
        + vm_.count ("soundcloud-tracks") + vm_.count ("soundcloud-playlists")
        + vm_.count ("soundcloud-genres") + vm_.count ("soundcloud-tags")
        + vm_.count ("log-directory");

  std::vector< std::string > all_valid_options = all_scloud_client_options_;
  concat_option_lists (all_valid_options, all_global_options_);
  concat_option_lists (all_valid_options, all_debug_options_);

  if (scloud_opts_count > 0
      && is_valid_options_combination (all_valid_options, all_given_options_))
  {
    outcome = true;
  }
  TIZ_PRINTF_DBG_RED ("outcome = [%s]\n",
                      outcome ? "SUCCESS" : "FAILURE");
  return outcome;
}

bool tiz::programopts::validate_port_argument (std::string &msg) const
{
  bool rc = true;
  if (vm_.count ("port"))
  {
    if (port_ <= 1024)
    {
      rc = false;
      std::ostringstream oss;
      oss << "Invalid argument : " << port_ << "\n"
          << "Please provide a port number in the range [1025-65535]";
      msg.assign (oss.str ());
    }
  }
  return rc;
}

bool tiz::programopts::validate_bitrates_argument (std::string &msg)
{
  bool rc = true;
  if (vm_.count ("bitrate-modes"))
  {
    boost::split (bitrate_list_, bitrates_, boost::is_any_of (","));
    if (!is_valid_bitrate_list (bitrate_list_))
    {
      rc = false;
      std::ostringstream oss;
      oss << "Invalid argument : " << bitrates_ << "\n"
          << "Valid bitrate mode values : [CBR,VBR].";
      msg.assign (oss.str ());
    }
  }
  return rc;
}

bool tiz::programopts::validate_sampling_rates_argument (std::string &msg)
{
  bool rc = true;
  if (vm_.count ("sampling-rates"))
  {
    std::vector< std::string > sampling_rate_str_list;
    boost::split (sampling_rate_str_list, sampling_rates_,
                  boost::is_any_of (","));
    if (!is_valid_sampling_rate_list (sampling_rate_str_list,
                                      sampling_rate_list_))
    {
      rc = false;
      std::ostringstream oss;
      oss << "Invalid argument : " << sampling_rates_ << "\n"
          << "Valid sampling rate values :\n"
          << "[8000,11025,12000,16000,22050,24000,32000,44100,48000,96000].";
      msg.assign (oss.str ());
    }
  }
  return rc;
}

void tiz::programopts::register_consume_function (const consume_mem_fn_t cf)
{
  consume_functions_.push_back (boost::bind (boost::mem_fn (cf), this, _1, _2));
}

int tiz::programopts::call_handler (
    const option_handlers_map_t::const_iterator &handler_it)
{
  int result = EXIT_FAILURE;
  if (handler_it != option_handlers_map_.end ())
  {
    OMX_ERRORTYPE rc = handler_it->second ();
    if (OMX_ErrorNone == rc)
    {
      result = EXIT_SUCCESS;
    }
  }
  return result;
}
