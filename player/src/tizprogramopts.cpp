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
 * @file   tizprogramopts.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Program options parsing utility.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mem_fn.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/xpressive/xpressive.hpp>

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
    explicit program_option_is_defaulted (const po::variables_map &vm)
      : vm_ (vm)
    {
    }

    bool operator() (const std::string &option) const
    {
      return vm_[option].defaulted ();
    }

  private:
    const po::variables_map &vm_;
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
    for (uint32_t i = 0; i < rate_strings.size () && rc; ++i)
    {
      rates.push_back (boost::lexical_cast< int > (rate_strings[i]));
      rc = is_valid_sampling_rate (rates[i]);
    }
    rc &= !rates.empty ();
    return rc;
  }

  bool is_valid_bitrate_list (const std::vector< std::string > &rate_strings)
  {
    bool rc = true;
    for (uint32_t i = 0; i < rate_strings.size (); ++i)
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
    TIZ_PRINTF_DBG_RED ("outcome = [%s]\n", outcome ? "SUCCESS" : "FAILURE");
    return outcome;
  }

  void retrieve_string_from_rc_file (const char *rc_section, const char *rc_key,
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

  bool retrieve_bool_from_rc_file_if_found (const char *rc_section, const char *rc_key,
                                            bool &flag)
  {
    bool is_found = false;
    assert (rc_section);
    assert (rc_key);
    const char *p_key = tiz_rcfile_get_value (rc_section, rc_key);
    if (p_key)
    {
      std::string key;
      key.assign(p_key);
      if (0 == key.compare("true"))
        {
          flag = true;
          is_found = true;
        }
      else if (0 == key.compare("false"))
        {
          flag = false;
          is_found = true;
        }
    }
    return is_found;
  }

  bool retrieve_uint_from_rc_file (const char *rc_section, const char *rc_key,
                                   uint32_t &uint_val)
  {
    bool is_found = false;
    assert (rc_section);
    assert (rc_key);
    const char *p_key = tiz_rcfile_get_value (rc_section, rc_key);
    if (p_key)
    {
      uint_val = boost::lexical_cast< uint32_t >(p_key);
      is_found = true;
    }
    return is_found;
  }

  void retrieve_tizonia_uint_from_rc_file (const char *rc_key, uint32_t &uint_val)
  {
    if (0 == uint_val && rc_key)
    {
      uint32_t val = 0;
      if (retrieve_uint_from_rc_file ("tizonia", rc_key, val)
          && val > 0)
      {
        uint_val = val;
      }
    }
  }

// Workaround for 'implicit_option' behavioral change introduced in boost
// 1.59. See https://github.com/boostorg/program_options/issues/25
#if (BOOST_VERSION >= 105900) && (BOOST_VERSION < 106500)
  template < typename T >
  struct greedy_implicit_value_impl : public po::typed_value< T >
  {
    using base = po::typed_value< T >;
    greedy_implicit_value_impl (T *store_to) : po::typed_value< T > (store_to)
    {
    }
    greedy_implicit_value_impl () : base (nullptr)
    {
    }
    bool adjacent_tokens_only () const override
    {
      return false;
    }
    unsigned max_tokens () const override
    {
      return 1;
    }
  };
  template < typename T >
  po::typed_value< T > *greedy_implicit_value (T *store_to)
  {
    return new greedy_implicit_value_impl< T > (store_to);
  }
#else
#define greedy_implicit_value po::value
#endif
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
    tunein_ ("TuneIn options"),
    youtube_ ("YouTube options"),
    plex_ ("Plex options"),
    iheart_ ("iheart options"),
    chromecast_ ("Chromecast options"),
    proxy_ ("Proxy server options"),
    input_ ("Input urioption"),
    positional_ (),
    help_option_ ("help"),
    recurse_ (false),
    shuffle_ (false),
    daemon_ (false),
    chromecast_name_or_ip_ (),
    buffer_seconds_(0),
    proxy_server_(),
    proxy_user_(),
    proxy_password_(),
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
    spotify_owner_ (),
    spotify_recover_lost_token_(false),
    spotify_allow_explicit_tracks_(false),
    spotify_preferred_bitrate_ (0),
    spotify_tracks_ (),
    spotify_artist_ (),
    spotify_album_ (),
    spotify_playlist_ (),
    spotify_track_id_ (),
    spotify_artist_id_ (),
    spotify_album_id_ (),
    spotify_playlist_id_ (),
    spotify_related_artists_(),
    spotify_featured_playlist_(),
    spotify_new_releases_(),
    spotify_recommendations_by_track_id_(),
    spotify_recommendations_by_artist_id_(),
    spotify_recommendations_by_genre_(),
    spotify_user_liked_tracks_(),
    spotify_user_recent_tracks_(),
    spotify_user_top_tracks_(),
    spotify_user_top_artists_(),
    spotify_user_playlist_(),
    spotify_playlist_container_ (),
    spotify_playlist_type_(OMX_AUDIO_SpotifyPlaylistTypeUnknown),
    gmusic_user_ (),
    gmusic_pass_ (),
    gmusic_device_id_ (),
    gmusic_artist_ (),
    gmusic_album_ (),
    gmusic_playlist_ (),
    gmusic_station_ (),
    gmusic_genre_ (),
    gmusic_activity_ (),
    gmusic_promoted_ (),
    gmusic_tracks_ (),
    gmusic_podcast_ (),
    gmusic_library_ (),
    gmusic_free_station_ (),
    gmusic_feeling_lucky_station_ (),
    gmusic_additional_keywords_ (),
    gmusic_playlist_container_ (),
    gmusic_playlist_type_ (OMX_AUDIO_GmusicPlaylistTypeUnknown),
    gmusic_is_unlimited_search_ (false),
    gmusic_buffer_seconds_ (0),
    scloud_oauth_token_ (),
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
    scloud_buffer_seconds_ (0),
    tunein_search_ (),
    tunein_category_ (),
    tunein_local_ (),
    tunein_music_ (),
    tunein_talk_ (),
    tunein_sports_ (),
    tunein_location_ (),
    tunein_podcasts_ (),
    tunein_trending_ (),
    tunein_search_type_filter_(),
    tunein_keywords_ (),
    tunein_playlist_container_ (),
    tunein_playlist_type_ (OMX_AUDIO_TuneinPlaylistTypeUnknown),
    tunein_search_type_ (OMX_AUDIO_TuneinSearchTypeAll),
    tunein_buffer_seconds_ (0),
    youtube_audio_stream_ (),
    youtube_audio_playlist_ (),
    youtube_audio_mix_ (),
    youtube_audio_search_ (),
    youtube_audio_mix_search_ (),
    youtube_audio_channel_uploads_ (),
    youtube_audio_channel_playlist_ (),
    youtube_playlist_container_ (),
    youtube_playlist_type_ (OMX_AUDIO_YoutubePlaylistTypeUnknown),
    youtube_api_key_(),
    youtube_buffer_seconds_ (0),
    plex_base_url_ (),
    plex_token_ (),
    plex_section_ (),
    plex_audio_tracks_ (),
    plex_audio_artist_ (),
    plex_audio_album_ (),
    plex_audio_playlist_ (),
    plex_playlist_container_ (),
    plex_playlist_type_ (OMX_AUDIO_PlexPlaylistTypeUnknown),
    plex_buffer_seconds_ (0),
    iheart_search_(),
    iheart_keywords_(),
    iheart_playlist_container_(),
    iheart_playlist_type_ (OMX_AUDIO_IheartPlaylistTypeUnknown),
    iheart_buffer_seconds_(),
    consume_functions_ (),
    all_global_options_ (),
    all_debug_options_ (),
    all_omx_options_ (),
    all_streaming_server_options_ (),
    all_streaming_client_options_ (),
    all_spotify_client_options_ (),
    all_gmusic_client_options_ (),
    all_scloud_client_options_ (),
    all_tunein_client_options_ (),
    all_youtube_client_options_ (),
    all_plex_client_options_ (),
    all_iheart_client_options_ (),
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
  init_tunein_options ();
  init_youtube_options ();
  init_plex_options ();
  init_iheart_options ();
  init_input_uri_option ();
}

int tiz::programopts::consume ()
{
  int rc = EXIT_FAILURE;
  bool config_file_ok = (0 == tiz_rcfile_status ());
  uint32_t given_options_count = 0;
  std::string error_msg;

  try
  {
    bool done = false;
    given_options_count = parse_command_line (argc_, argv_);
    if (given_options_count && (config_file_ok || vm_.count ("help")))
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
      if (!config_file_ok)
      {
        error_msg.assign (
            "Unable to find a valid configuration file (tizonia.conf). \nUse "
            "'tizonia --help config'");
      }
      if (error_msg.empty ())
      {
        error_msg.assign ("Invalid combination of program options.");
      }
      print_version ();
      print_license ();
      TIZ_PRINTF_C01 ("%s", error_msg.c_str ());
      printf ("\n\n");
    }
  }

  return rc;
}

void tiz::programopts::print_version () const
{
  TIZ_PRINTF_C04 ("tizonia %s. Copyright (C) 2020 Juan A. Rubio and contributors",
                  PACKAGE_VERSION);
  TIZ_PRINTF_C04 (
      "This software is part of the Tizonia project <https://tizonia.org>");
  printf ("\n");
}

void tiz::programopts::print_usage_help () const
{
  print_version ();
  print_license ();
  std::cout << " "
            << "Help topics:"
            << "\n\n";
  std::cout << "  "
            << "global        Global options available in combination with "
               "other features."
            << "\n";
  std::cout << "  "
            << "openmax       Various OpenMAX IL query options."
            << "\n";
  std::cout << "  "
            << "server        SHOUTcast/ICEcast streaming server options."
            << "\n";
  std::cout << "  "
            << "client        SHOUTcast/ICEcast streaming client options."
            << "\n";
#ifdef HAVE_LIBSPOTIFY
  std::cout << "  "
            << "spotify       Spotify options."
            << "\n";
#endif
  std::cout << "  "
            << "googlemusic   Google Play Music options."
            << "\n";
  std::cout << "  "
            << "soundcloud    SoundCloud options."
            << "\n";
  std::cout << "  "
            << "tunein        TuneIn options."
            << "\n";
  std::cout << "  "
            << "youtube       YouTube options."
            << "\n";
  std::cout << "  "
            << "plex          Plex options."
            << "\n";
  std::cout << "  "
            << "iheart        Iheart options."
            << "\n";
  std::cout << "  "
            << "chromecast    Chromecast options."
            << "\n";
  std::cout << "  "
            << "proxy         Proxy server options."
            << "\n";
  std::cout << "  "
            << "keyboard      Keyboard control."
            << "\n";
  std::cout << "  "
            << "config        Configuration files."
            << "\n";
  std::cout << "  "
            << "examples      Some command-line examples."
            << "\n";

  std::cout << "\n"
            << "Use \"tizonia --help topic\"."
            << "\n";
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
  print_version ();
  print_license ();
  printf ("Configuration file: 'tizonia.conf'\n\n");
  printf (
      "Tizonia creates its config file in one of the following locations when it\n"
      "first starts (add your user credentials here):\n");
  printf (
      "    - Debian or AUR packages: $HOME/.config/tizonia/tizonia.conf\n"
      "    - Snap package: $HOME/snap/tizonia/current/.config/tizonia/tizonia.conf\n");
  printf (
      "\nExample configuration files may also be found at \n"
      "    - /etc/xdg/tizonia/tizonia.conf or \n"
      "    - /snap/tizonia/current/etc/xdg/tizonia/tizonia.conf.\n");

//
//  TODO: Think about how the following information could be provided to the
//  user, since it is probably more for a developer or power user.
//
//   printf (
//       "Tizonia finds its config file in one of these locations (in this "
//       "order):\n");
//   printf (
//       "1.   A file pointed by the $TIZONIA_RC_FILE environment "
//       "variable.\n");
//   printf ("2.   $HOME/.config/tizonia/tizonia.conf\n");
//   printf (
//       "3.   A directory in $XDG_CONFIG_DIRS + "
//       "/tizonia/tizonia.conf\n");
//   printf ("4.                  /etc/tizonia/tizonia.conf\n\n");
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
      "        opus (.opus, .ogg, .oga), vorbis (.ogg, .oga), wav, aiff, "
      "aif.\n");
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
  option_handlers_map_.insert (std::make_pair (option, handler));
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

const std::string &tiz::programopts::chromecast_name_or_ip () const
{
  return chromecast_name_or_ip_;
}

const std::string &tiz::programopts::proxy_server () const
{
  return proxy_server_;
}

const std::string &tiz::programopts::proxy_user () const
{
  return proxy_user_;
}

const std::string &tiz::programopts::proxy_password () const
{
  return proxy_password_;
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

const std::string &tiz::programopts::spotify_owner () const
{
  return spotify_owner_;
}

bool tiz::programopts::spotify_recover_lost_token () const
{
  return spotify_recover_lost_token_;
}

bool tiz::programopts::spotify_allow_explicit_tracks () const
{
  return spotify_allow_explicit_tracks_;
}

uint32_t tiz::programopts::spotify_preferred_bitrate () const
{
  return spotify_preferred_bitrate_;
}

const std::vector< std::string >
    &tiz::programopts::spotify_playlist_container ()
{
  spotify_playlist_container_.clear ();
  if (!spotify_tracks_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_tracks_);
  }
  else if (!spotify_artist_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_artist_);
  }
  else if (!spotify_album_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_album_);
  }
  else if (!spotify_playlist_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_playlist_);
  }
  else if (!spotify_track_id_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_track_id_);
  }
  else if (!spotify_artist_id_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_artist_id_);
  }
  else if (!spotify_album_id_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_album_id_);
  }
  else if (!spotify_playlist_id_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_playlist_id_);
  }
  else if (!spotify_related_artists_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_related_artists_);
  }
  else if (!spotify_featured_playlist_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_featured_playlist_);
  }
  else if (!spotify_new_releases_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_new_releases_);
  }
  else if (!spotify_recommendations_by_track_id_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_recommendations_by_track_id_);
  }
  else if (!spotify_recommendations_by_artist_id_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_recommendations_by_artist_id_);
  }
  else if (!spotify_recommendations_by_genre_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_recommendations_by_genre_);
  }
  else if (!spotify_user_liked_tracks_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_user_liked_tracks_);
  }
  else if (!spotify_user_recent_tracks_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_user_recent_tracks_);
  }
  else if (!spotify_user_top_tracks_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_user_top_tracks_);
  }
  else if (!spotify_user_top_artists_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_user_top_artists_);
  }
  else if (!spotify_user_playlist_.empty ())
  {
    spotify_playlist_container_.push_back (spotify_user_playlist_);
  }
  else
  {
    assert (0);
  }
  return spotify_playlist_container_;
}

OMX_TIZONIA_AUDIO_SPOTIFYPLAYLISTTYPE
tiz::programopts::spotify_playlist_type ()
{
  if (!spotify_tracks_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeTracks;
  }
  else if (!spotify_artist_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeArtist;
  }
  else if (!spotify_album_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeAlbum;
  }
  else if (!spotify_playlist_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypePlaylist;
  }
  else if (!spotify_track_id_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeTrackId;
  }
  else if (!spotify_artist_id_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeArtistId;
  }
  else if (!spotify_album_id_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeAlbumId;
  }
  else if (!spotify_playlist_id_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypePlaylistId;
  }
  else if (!spotify_related_artists_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeRelatedArtists;
  }
  else if (!spotify_featured_playlist_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeFeaturedPlaylist;
  }
  else if (!spotify_new_releases_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeNewReleases;
  }
  else if (!spotify_recommendations_by_track_id_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeRecommendationsByTrackId;
  }
  else if (!spotify_recommendations_by_artist_id_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeRecommendationsByArtistId;
  }
  else if (!spotify_recommendations_by_genre_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeRecommendationsByGenre;
  }
  else if (!spotify_user_liked_tracks_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeUserLikedTracks;
  }
  else if (!spotify_user_recent_tracks_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeUserRecentTracks;
  }
  else if (!spotify_user_top_tracks_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeUserTopTracks;
  }
  else if (!spotify_user_top_artists_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeUserTopArtists;
  }
  else if (!spotify_user_playlist_.empty ())
  {
    spotify_playlist_type_ = OMX_AUDIO_SpotifyPlaylistTypeUserPlaylist;
  }

  return spotify_playlist_type_;
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

const std::vector< std::string > &tiz::programopts::gmusic_playlist_container ()
{
  gmusic_playlist_container_.clear ();
  if (!gmusic_library_.empty ())
  {
    // With gmusic library option, no playlist "name" is actually
    // required. But this helps keeping track of what is in the container.
    gmusic_playlist_container_.push_back (gmusic_library_);
  }
  else if (!gmusic_tracks_.empty ())
  {
    gmusic_playlist_container_.push_back (gmusic_tracks_);
  }
  else if (!gmusic_artist_.empty ())
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
  else if (!gmusic_activity_.empty ())
  {
    gmusic_playlist_container_.push_back (gmusic_activity_);
  }
  else if (!gmusic_podcast_.empty ())
  {
    gmusic_playlist_container_.push_back (gmusic_podcast_);
  }
  else if (!gmusic_free_station_.empty ())
  {
    gmusic_playlist_container_.push_back (gmusic_free_station_);
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
  if (!gmusic_library_.empty ())
  {
    gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypeLibrary;
  }
  else if (!gmusic_tracks_.empty ())
  {
    gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypeTracks;
  }
  else if (!gmusic_artist_.empty ())
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
  else if (!gmusic_activity_.empty ())
  {
    gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypeSituation;
  }
  else if (!gmusic_podcast_.empty ())
  {
    gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypePodcast;
  }
  else if (!gmusic_free_station_.empty ())
  {
    gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypeFreeStation;
  }
  else if (!gmusic_promoted_.empty ())
  {
    gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypePromotedTracks;
  }
  else
  {
    gmusic_playlist_type_ = OMX_AUDIO_GmusicPlaylistTypeUnknown;
  }

  return gmusic_playlist_type_;
}

const std::string &
tiz::programopts::gmusic_additional_keywords () const
{
  return gmusic_additional_keywords_;
}

bool tiz::programopts::gmusic_is_unlimited_search () const
{
  return gmusic_is_unlimited_search_;
}

uint32_t tiz::programopts::gmusic_buffer_seconds () const
{
  return buffer_seconds_ ? buffer_seconds_ : gmusic_buffer_seconds_;
}

const std::string &tiz::programopts::scloud_oauth_token () const
{
  return scloud_oauth_token_;
}

const std::vector< std::string > &tiz::programopts::scloud_playlist_container ()
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
    scloud_playlist_type_ = OMX_AUDIO_SoundCloudPlaylistTypeUnknown;
  }

  return scloud_playlist_type_;
}

uint32_t tiz::programopts::scloud_buffer_seconds () const
{
  return buffer_seconds_ ? buffer_seconds_ : scloud_buffer_seconds_;
}

const std::vector< std::string > &tiz::programopts::tunein_playlist_container ()
{
  if (!tunein_search_.empty ())
  {
    tunein_playlist_container_.push_back (tunein_search_);
    BOOST_FOREACH (std::string keyword, tunein_keywords_)
    {
      tunein_playlist_container_.push_back (keyword);
    }
  }
  else if (!tunein_category_.empty ())
  {
    BOOST_FOREACH (std::string keyword, tunein_keywords_)
    {
      tunein_playlist_container_.push_back (keyword);
    }
  }
  else
  {
    assert (0);
  }
  return tunein_playlist_container_;
}

OMX_TIZONIA_AUDIO_TUNEINPLAYLISTTYPE
tiz::programopts::tunein_playlist_type ()
{
  if (!tunein_search_.empty ())
  {
    tunein_playlist_type_ = OMX_AUDIO_TuneinPlaylistTypeRadios;
  }
  else if (!tunein_category_.empty ())
  {
    tunein_playlist_type_ = OMX_AUDIO_TuneinPlaylistTypeCategory;
  }
  else
  {
    tunein_playlist_type_ = OMX_AUDIO_TuneinPlaylistTypeUnknown;
  }

  return tunein_playlist_type_;
}

OMX_TIZONIA_AUDIO_TUNEINSEARCHTYPE
tiz::programopts::tunein_search_type ()
{
  if (!tunein_search_type_filter_.compare ("stations"))
  {
    tunein_search_type_ = OMX_AUDIO_TuneinSearchTypeStations;
  }
  else if (!tunein_search_type_filter_.compare ("shows"))
  {
    tunein_search_type_ = OMX_AUDIO_TuneinSearchTypeShows;
  }
  else
  {
    tunein_search_type_ = OMX_AUDIO_TuneinSearchTypeAll;
  }

  return tunein_search_type_ ;
}

uint32_t tiz::programopts::tunein_buffer_seconds () const
{
  return buffer_seconds_ ? buffer_seconds_ : tunein_buffer_seconds_;
}

const std::vector< std::string >
    &tiz::programopts::youtube_playlist_container ()
{
  youtube_playlist_container_.clear ();
  if (!youtube_audio_stream_.empty ())
  {
    youtube_playlist_container_.push_back (youtube_audio_stream_);
  }
  else if (!youtube_audio_playlist_.empty ())
  {
    youtube_playlist_container_.push_back (youtube_audio_playlist_);
  }
  else if (!youtube_audio_mix_.empty ())
  {
    youtube_playlist_container_.push_back (youtube_audio_mix_);
  }
  else if (!youtube_audio_search_.empty ())
  {
    youtube_playlist_container_.push_back (youtube_audio_search_);
  }
  else if (!youtube_audio_mix_search_.empty ())
  {
    youtube_playlist_container_.push_back (youtube_audio_mix_search_);
  }
  else if (!youtube_audio_channel_uploads_.empty ())
  {
    youtube_playlist_container_.push_back (youtube_audio_channel_uploads_);
  }
  else if (!youtube_audio_channel_playlist_.empty ())
  {
    youtube_playlist_container_.push_back (youtube_audio_channel_playlist_);
  }
  else
  {
    assert (0);
  }
  return youtube_playlist_container_;
}

OMX_TIZONIA_AUDIO_YOUTUBEPLAYLISTTYPE
tiz::programopts::youtube_playlist_type ()
{
  if (!youtube_audio_stream_.empty ())
  {
    youtube_playlist_type_ = OMX_AUDIO_YoutubePlaylistTypeAudioStream;
  }
  else if (!youtube_audio_playlist_.empty ())
  {
    youtube_playlist_type_ = OMX_AUDIO_YoutubePlaylistTypeAudioPlaylist;
  }
  else if (!youtube_audio_mix_.empty ())
  {
    youtube_playlist_type_ = OMX_AUDIO_YoutubePlaylistTypeAudioMix;
  }
  else if (!youtube_audio_search_.empty ())
  {
    youtube_playlist_type_ = OMX_AUDIO_YoutubePlaylistTypeAudioSearch;
  }
  else if (!youtube_audio_mix_search_.empty ())
  {
    youtube_playlist_type_ = OMX_AUDIO_YoutubePlaylistTypeAudioMixSearch;
  }
  else if (!youtube_audio_channel_uploads_.empty ())
  {
    youtube_playlist_type_ = OMX_AUDIO_YoutubePlaylistTypeAudioChannelUploads;
  }
  else if (!youtube_audio_channel_playlist_.empty ())
  {
    youtube_playlist_type_ = OMX_AUDIO_YoutubePlaylistTypeAudioChannelPlaylist;
  }
  else
  {
    youtube_playlist_type_ = OMX_AUDIO_YoutubePlaylistTypeUnknown;
  }

  return youtube_playlist_type_;
}

const std::string &tiz::programopts::youtube_api_key () const
{
  return youtube_api_key_;
}

uint32_t tiz::programopts::youtube_buffer_seconds () const
{
  return buffer_seconds_ ? buffer_seconds_ : youtube_buffer_seconds_;
}

const std::string &tiz::programopts::plex_base_url () const
{
  return plex_base_url_;
}

const std::string &tiz::programopts::plex_token () const
{
  return plex_token_;
}

const std::string &tiz::programopts::plex_section () const
{
  return plex_section_;
}

const std::vector< std::string > &tiz::programopts::plex_playlist_container ()
{
  plex_playlist_container_.clear ();
  if (!plex_audio_tracks_.empty ())
  {
    plex_playlist_container_.push_back (plex_audio_tracks_);
  }
  else if (!plex_audio_artist_.empty ())
  {
    plex_playlist_container_.push_back (plex_audio_artist_);
  }
  else if (!plex_audio_album_.empty ())
  {
    plex_playlist_container_.push_back (plex_audio_album_);
  }
  else if (!plex_audio_playlist_.empty ())
  {
    plex_playlist_container_.push_back (plex_audio_playlist_);
  }
  else
  {
    assert (0);
  }
  return plex_playlist_container_;
}

OMX_TIZONIA_AUDIO_PLEXPLAYLISTTYPE
tiz::programopts::plex_playlist_type ()
{
  if (!plex_audio_tracks_.empty ())
  {
    plex_playlist_type_ = OMX_AUDIO_PlexPlaylistTypeAudioTracks;
  }
  else if (!plex_audio_artist_.empty ())
  {
    plex_playlist_type_ = OMX_AUDIO_PlexPlaylistTypeAudioArtist;
  }
  else if (!plex_audio_album_.empty ())
  {
    plex_playlist_type_ = OMX_AUDIO_PlexPlaylistTypeAudioAlbum;
  }
  else if (!plex_audio_playlist_.empty ())
  {
    plex_playlist_type_ = OMX_AUDIO_PlexPlaylistTypeAudioPlaylist;
  }
  else
  {
    plex_playlist_type_ = OMX_AUDIO_PlexPlaylistTypeUnknown;
  }

  return plex_playlist_type_;
}

uint32_t tiz::programopts::plex_buffer_seconds () const
{
  return buffer_seconds_ ? buffer_seconds_ : plex_buffer_seconds_;
}

const std::vector< std::string > &tiz::programopts::iheart_playlist_container ()
{
  iheart_playlist_container_.clear ();
  if (!iheart_search_.empty ())
  {
    iheart_playlist_container_.push_back (iheart_search_);
    BOOST_FOREACH (std::string keyword, iheart_keywords_)
    {
      iheart_playlist_container_.push_back (keyword);
    }
  }
  else
  {
    assert (0);
  }
  return iheart_playlist_container_;
}

OMX_TIZONIA_AUDIO_IHEARTPLAYLISTTYPE
tiz::programopts::iheart_playlist_type ()
{
  if (!iheart_search_.empty ())
  {
    iheart_playlist_type_ = OMX_AUDIO_IheartPlaylistTypeRadios;
  }
  else
  {
    iheart_playlist_type_ = OMX_AUDIO_IheartPlaylistTypeUnknown;
  }

  return iheart_playlist_type_;
}

uint32_t tiz::programopts::iheart_buffer_seconds () const
{
  return buffer_seconds_ ? buffer_seconds_ : iheart_buffer_seconds_;
}

void tiz::programopts::print_license () const
{
  TIZ_PRINTF_C02 (
      "GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl.html>");
  TIZ_PRINTF_C02 (
      "This is free software: you are free to change and redistribute it.");
  TIZ_PRINTF_C02 ("There is NO WARRANTY, to the extent permitted by law.");
  printf ("\n");
}

void tiz::programopts::init_global_options ()
{
  global_.add_options ()
      /* TIZ_CLASS_COMMENT: This is to avoid the clang formatter messing up
         these lines*/
      ("help,h",
       greedy_implicit_value< std::string > (&help_option_)
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
      ("cast,c", po::value (&chromecast_name_or_ip_),
       "Cast to a Chromecast device (arg: device name or ip address). "
       "Available in combination with Google Play Music, SoundCloud, "
       "YouTube, TuneIn and HTTP radio stations.")
      /* TIZ_CLASS_COMMENT: */
      ("buffer-seconds,b", po::value (&buffer_seconds_),
       "Size of the buffer (in seconds) to be used while downloading streams. "
       "Increase in case of cuts in gmusic, scloud, youtube or plex.")
      /* TIZ_CLASS_COMMENT: */
      ("proxy-server", po::value (&proxy_server_),
       "Url to the proxy server that should be used. (not required if provided "
       "via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("proxy-user", po::value (&proxy_user_),
       "User name to be used during proxy server authentication (not required "
       "if provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("proxy-password", po::value (&proxy_password_),
       "Password to be used during proxy server authentication (not required "
       "if provided via config file).");

  register_consume_function (&tiz::programopts::consume_global_options);
  // TODO: help and version are not included. These should be moved out of
  // "global" and into its own category: "info"
  all_global_options_
      = boost::assign::list_of ("recurse") ("shuffle") ("daemon") ("cast") (
            "buffer-seconds") ("proxy-server") ("proxy-user") ("proxy-password")
            .convert_to_container< std::vector< std::string > > ();

  // Even though --cast is a global option, we also initialise here a
  // 'chromecast' option description for the purpose of displaying it with the
  // --help command, which provides a little more visibility.
  chromecast_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("cast,c", po::value (&chromecast_name_or_ip_),
       "Cast to a Chromecast device (arg: device name, 'friendly' name or ip address). "
       "Available in combination with Google Play Music, SoundCloud, "
       "YouTube, Plex and HTTP radio stations.");

  // We also initialise here a 'proxy' option description for the purpose
  // of displaying it with the --help command.
  proxy_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("proxy-server", po::value (&proxy_server_),
       "Url to the proxy server that should be used (only works with the "
       "Spotify service at the moment, not required if provided "
       "via config file). The format is protocol://<host>:port (where protocol "
       "is http/https/socks4/socks5")
      /* TIZ_CLASS_COMMENT: */
      ("proxy-user", po::value (&proxy_user_),
       "User name to be used during proxy server authentication (only works "
       "with the Spotify service at the moment, not required "
       "if provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("proxy-password", po::value (&proxy_password_),
       "Password to be used during proxy server authentication (only works with "
       "the Spotify service at the moment, not required "
       "if provided via config file).");
}

void tiz::programopts::init_debug_options ()
{
  debug_.add_options ()
      /* TIZ_CLASS_COMMENT: This is to avoid the clang formatter messing up
         these lines*/
      ("log-directory", po::value (&log_dir_),
       "The directory to be used for the debug trace file.") (
          "debug-info", po::bool_switch (&debug_info_)->default_value (false),
          "Print debug-related information.")
      /* TIZ_CLASS_COMMENT: */
      ;
  register_consume_function (&tiz::programopts::consume_debug_options);
  all_debug_options_
      = boost::assign::list_of ("log-directory") ("debug-info")
            .convert_to_container< std::vector< std::string > > ();
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
      = boost::assign::list_of ("comp-list") ("roles-of-comp") ("comps-of-role")
            .convert_to_container< std::vector< std::string > > ();
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
       "TCP port to be used for Icecast/SHOUTcast streaming. Optional. "
       "Default: 8010.")
      /* TIZ_CLASS_COMMENT: */
      ("station-name", po::value (&station_name_),
       "The Icecast/SHOUTcast station name. Optional.")
      /* TIZ_CLASS_COMMENT: */
      ("station-genre", po::value (&station_genre_),
       "The Icecast/SHOUTcast station genre. Optional.")
      /* TIZ_CLASS_COMMENT: */
      ("no-icy-metadata", po::bool_switch (&no_icy_metadata_),
       "Disables Icecast/SHOUTcast metadata in the stream. Optional.")
      /* TIZ_CLASS_COMMENT: */
      ("bitrate-modes", po::value (&bitrates_),
       "A comma-separated list of "
       "bitrate modes (e.g. 'CBR,VBR'). Only media with these bitrate modes "
       "will be streamed. Optional. Default: any.")
      /* TIZ_CLASS_COMMENT: */
      ("sampling-rates", po::value (&sampling_rates_),
       "A comma-separated list of sampling rates. Only media with these rates "
       "will be streamed."
       "Optional. Default: any.");

  // Give a default value to the bitrate list
  bitrates_ = std::string ("CBR,VBR");
  boost::split (bitrate_list_, bitrates_, boost::is_any_of (","));
  register_consume_function (
      &tiz::programopts::consume_streaming_server_options);
  all_streaming_server_options_
      = boost::assign::list_of ("server") ("port") ("station-name") (
            "station-genre") ("no-icy-metadata") ("bitrate-modes") (
            "sampling-rates")
            .convert_to_container< std::vector< std::string > > ();
}

void tiz::programopts::init_streaming_client_options ()
{
  client_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("station-id", po::value (&station_name_),
       "Give a name/id to the remote stream. Optional.");
  register_consume_function (
      &tiz::programopts::consume_streaming_client_options);
  all_streaming_client_options_
      = boost::assign::list_of ("station-id")
            .convert_to_container< std::vector< std::string > > ();
}

void tiz::programopts::init_spotify_options ()
{
#ifdef HAVE_LIBSPOTIFY
  spotify_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("spotify-user", po::value (&spotify_user_),
       "Spotify user name  (not required if provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-password", po::value (&spotify_pass_),
       "Spotify user password  (not required if provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-owner", po::value (&spotify_owner_),
       "The owner of the playlist  (this is optional: use in conjunction with "
       "--spotify-playlist or --spotify-playlist-id).")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-recover-lost-token",
       po::bool_switch (&spotify_recover_lost_token_)->default_value (false),
       "Allow Tizonia to recover the play token and keep playing after a "
       "spurious token loss (default: false).")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-allow-explicit-tracks",
       po::bool_switch (&spotify_allow_explicit_tracks_)->default_value (false),
       "Allow Tizonia to play explicit tracks from Spotify (default: false).")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-preferred-bitrate", po::value (&spotify_preferred_bitrate_),
       "Preferred Spotify bitrate (kbps) (320, 160 or 96; default: 320).")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-tracks", po::value (&spotify_tracks_),
       "Search and play from Spotify by track name.")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-artist", po::value (&spotify_artist_),
       "Search and play from Spotify by artist name.")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-album", po::value (&spotify_album_),
       "Search and play from Spotify by album name.")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-playlist", po::value (&spotify_playlist_),
       "Search and play public playlists (owner is assumed the current user, "
       "unless --spotify-owner is provided).")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-track-id", po::value (&spotify_track_id_),
       "Play from Spotify by track ID, URI or URL.")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-artist-id", po::value (&spotify_artist_id_),
       "Play from Spotify by artist ID, URI or URL.")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-album-id", po::value (&spotify_album_id_),
       "Play from Spotify by album ID, URI or URL.")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-playlist-id", po::value (&spotify_playlist_id_),
       "Play public playlists from Spotify by ID, URI or URL "
       "(owner is assumed the current user, "
       "unless --spotify-owner is provided).")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-related-artists", po::value (&spotify_related_artists_),
       "Search and play from Spotify the top songs from "
       "a selection of related artists.")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-featured-playlist", po::value (&spotify_featured_playlist_),
       "Search and play a featured playlist from Spotify.")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-new-releases", po::value (&spotify_new_releases_),
       "Search and play a newly released album from Spotify.")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-recommendations-by-track-id",
       po::value (&spotify_recommendations_by_track_id_),
       "Play Spotify recommendations by track ID, URI or URL")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-recommendations-by-artist-id",
       po::value (&spotify_recommendations_by_artist_id_),
       "Play Spotify recommendations by artist ID, URI or URL.")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-recommendations-by-genre",
       po::value (&spotify_recommendations_by_genre_),
       "Play Spotify recommendations by genre name.")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-user-liked-tracks", "Play the user's liked tracks")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-user-recent-tracks",
       "Play the user's most recently played tracks")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-user-top-tracks", "Play the user's top tracks")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-user-top-artists",
       "Play top tracks from the user's top artists")
      /* TIZ_CLASS_COMMENT: */
      ("spotify-user-playlist", po::value (&spotify_user_playlist_),
       "Play a playlist from the users private library, including Daily Mixes "
       "and Discover Weekly");

  register_consume_function (&tiz::programopts::consume_spotify_client_options);
  all_spotify_client_options_
      = boost::assign::list_of ("spotify-user") ("spotify-password") (
            "spotify-owner") ("spotify-recover-lost-token") (
            "spotify-allow-explicit-tracks") ("spotify-preferred-bitrate") (
            "spotify-tracks") ("spotify-artist") ("spotify-album") (
            "spotify-playlist") ("spotify-track-id") ("spotify-artist-id") (
            "spotify-album-id") ("spotify-playlist-id") (
            "spotify-related-artists") ("spotify-featured-playlist") (
            "spotify-new-releases") ("spotify-recommendations-by-track-id") (
            "spotify-recommendations-by-artist-id") (
            "spotify-recommendations-by-genre") ("spotify-user-liked-tracks") (
            "spotify-user-recent-tracks") ("spotify-user-top-tracks") (
            "spotify-user-top-artists") ("spotify-user-playlist")
            .convert_to_container< std::vector< std::string > > ();
#endif
}

void tiz::programopts::init_gmusic_options ()
{
  gmusic_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-user", po::value (&gmusic_user_),
       "Google Play Music user name (not required if provided via config "
       "file).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-password", po::value (&gmusic_pass_),
       "Google Play Music user's password (not required if provided via config "
       "file).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-device-id", po::value (&gmusic_device_id_),
       "Google Play Music device id (not required if provided via config "
       "file).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-additional-keywords", po::value (&gmusic_additional_keywords_),
       "Additional search keywords (this is optional: use in conjunction with"
       "--gmusic-unlimited-activity).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-library", "Play all tracks from the user's library.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-tracks", po::value (&gmusic_tracks_),
       "Play tracks from the user's library by track name.")
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
      ("gmusic-podcast", po::value (&gmusic_podcast_),
       "Search and play Google Play Music podcasts (only available in the US "
       "and Canada).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-station", po::value (&gmusic_free_station_),
       "Search and play Google Play Music free stations.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-unlimited-station", po::value (&gmusic_station_),
       "Search and play Google Play Music Unlimited stations found in the "
       "user's library.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-unlimited-album", po::value (&gmusic_album_),
       "Search and play Google Play Music Unlimited tracks by album (best "
       "match only).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-unlimited-artist", po::value (&gmusic_artist_),
       "Search and play Google Play Music Unlimited tracks by artist (best "
       "match only).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-unlimited-tracks", po::value (&gmusic_tracks_),
       "Search and play Google Play Music Unlimited tracks by name (50 first "
       "matches only).")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-unlimited-playlist", po::value (&gmusic_playlist_),
       "Search and play Google Play Music Unlimited playlists by name.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-unlimited-genre", po::value (&gmusic_genre_),
       "Search and play Google Play Music Unlimited tracks by genre.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-unlimited-activity", po::value (&gmusic_activity_),
       "Search and play Google Play Music Unlimited tracks by activity.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-unlimited-feeling-lucky-station",
       "Play the user's Google Play Music Unlimited 'I'm Feeling Lucky' "
       "station.")
      /* TIZ_CLASS_COMMENT: */
      ("gmusic-unlimited-promoted-tracks",
       "Play Google Play Music Unlimited promoted tracks.");

  register_consume_function (&tiz::programopts::consume_gmusic_client_options);
  all_gmusic_client_options_
      = boost::assign::list_of ("gmusic-user") ("gmusic-password") (
            "gmusic-device-id") ("gmusic-additional-keywords") (
            "gmusic-library") ("gmusic-tracks") ("gmusic-artist") (
            "gmusic-album") ("gmusic-playlist") ("gmusic-podcast") (
            "gmusic-station") ("gmusic-unlimited-station") (
            "gmusic-unlimited-album") ("gmusic-unlimited-artist") (
            "gmusic-unlimited-tracks") ("gmusic-unlimited-playlist") (
            "gmusic-unlimited-genre") ("gmusic-unlimited-activity") (
            "gmusic-unlimited-feeling-lucky-station") (
            "gmusic-unlimited-promoted-tracks")
            .convert_to_container< std::vector< std::string > > ();
}

void tiz::programopts::init_scloud_options ()
{
  scloud_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("soundcloud-oauth-token", po::value (&scloud_oauth_token_),
       "SoundCloud user OAuth token (not required if provided via config "
       "file).")
      /* TIZ_CLASS_COMMENT: */
      ("soundcloud-user-stream",
       "Play the tracks currently listed in the user's stream.")
      /* TIZ_CLASS_COMMENT: */
      ("soundcloud-user-likes", "Play the tracks liked by the user.")
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
       "Search and play genres top tracks (arg is a command-separated list).") (
          "soundcloud-tags", po::value (&scloud_tags_),
          "Search and play tags top tracks (arg is a command-separated list).");

  register_consume_function (&tiz::programopts::consume_scloud_client_options);
  all_scloud_client_options_
      = boost::assign::list_of ("soundcloud-oauth-token") (
            "soundcloud-user-stream") ("soundcloud-user-likes") (
            "soundcloud-user-playlist") ("soundcloud-creator") (
            "soundcloud-tracks") ("soundcloud-playlists") (
            "soundcloud-genres") ("soundcloud-tags")
            .convert_to_container< std::vector< std::string > > ();
}

void tiz::programopts::init_tunein_options ()
{
  tunein_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("tunein-search", po::value (&tunein_search_),
       "TuneIn global station/podcast search.")
      /* TIZ_CLASS_COMMENT: */
      ("tunein-local", po::value (&tunein_local_),
       "TuneIn 'local' category search.")
      /* TIZ_CLASS_COMMENT: */
      ("tunein-music", po::value (&tunein_music_),
       "TuneIn 'music' category search.")
      /* TIZ_CLASS_COMMENT: */
      ("tunein-talk", po::value (&tunein_talk_),
       "TuneIn 'talk' category search.")
      /* TIZ_CLASS_COMMENT: */
      ("tunein-sports", po::value (&tunein_sports_),
       "TuneIn 'sports' category search.")
      /* TIZ_CLASS_COMMENT: */
      ("tunein-location", po::value (&tunein_location_),
       "TuneIn 'location' category search.")
      /* TIZ_CLASS_COMMENT: */
      ("tunein-podcasts", po::value (&tunein_podcasts_),
       "TuneIn 'podcasts' category search.")
      /* TIZ_CLASS_COMMENT: */
      ("tunein-trending", po::value (&tunein_trending_)->implicit_value (""),
       "TuneIn 'trending' category search.")
      /* TIZ_CLASS_COMMENT: */
      ("tunein-type", po::value (&tunein_search_type_filter_),
       "Narrow down the search to specific type: 'stations', 'shows', or 'all' "
       "(default: all). "
       "Optional.")
      /* TIZ_CLASS_COMMENT: */
      ("tunein-keywords",
       po::value< std::vector< std::string > > (&tunein_keywords_)
           ->multitoken ()
           ->composing (),
       "Additional keywords that may be used in conjunction with the TuneIn "
       "options. Optional (may be repeated).");

  register_consume_function (&tiz::programopts::consume_tunein_client_options);
  all_tunein_client_options_
      = boost::assign::list_of ("tunein-search") ("tunein-type") (
            "tunein-local") ("tunein-music") ("tunein-talk") ("tunein-sports") (
            "tunein-location") ("tunein-podcasts") ("tunein-trending") (
            "tunein-keywords")
            .convert_to_container< std::vector< std::string > > ();
}

void tiz::programopts::init_youtube_options ()
{
  youtube_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("youtube-api-key", po::value (&youtube_api_key_),
       "The user's YouTube DATA API v3 key (Optional but RECOMMENDED)")
      /* TIZ_CLASS_COMMENT: */
      ("youtube-audio-stream", po::value (&youtube_audio_stream_),
       "Play a YouTube audio stream from a video url or video id.")
      /* TIZ_CLASS_COMMENT: */
      ("youtube-audio-playlist", po::value (&youtube_audio_playlist_),
       "Play a YouTube audio playlist from a playlist url or playlist id.")
      /* TIZ_CLASS_COMMENT: */
      ("youtube-audio-mix", po::value (&youtube_audio_mix_),
       "Play a YouTube mix from a video url or video id.")
      /* TIZ_CLASS_COMMENT: */
      ("youtube-audio-search", po::value (&youtube_audio_search_),
       "Search and play YouTube audio streams.")
      /* TIZ_CLASS_COMMENT: */
      ("youtube-audio-mix-search", po::value (&youtube_audio_mix_search_),
       "Play a YouTube mix from a search term.")
      /* TIZ_CLASS_COMMENT: */
      ("youtube-audio-channel-uploads", po::value (&youtube_audio_channel_uploads_),
       "Play all videos uploaded to a YouTube channel (arg = channel url or name).")
      /* TIZ_CLASS_COMMENT: */
      ("youtube-audio-channel-playlist", po::value (&youtube_audio_channel_playlist_),
       "Play a playlist from particular YouTube channel (arg = '<channel-name[space]playlist-name>').");

  register_consume_function (&tiz::programopts::consume_youtube_client_options);
  all_youtube_client_options_
      = boost::assign::list_of ("youtube-api-key") ("youtube-audio-stream") (
            "youtube-audio-playlist") ("youtube-audio-mix") (
            "youtube-audio-search") ("youtube-audio-mix-search") (
            "youtube-audio-channel-uploads") ("youtube-audio-channel-playlist")
            .convert_to_container< std::vector< std::string > > ();
}

void tiz::programopts::init_plex_options ()
{
  plex_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("plex-server-base-url", po::value (&plex_base_url_),
       "Plex server base URL (e.g. 'http://plexserver:32400'. Not required if "
       "provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("plex-auth-token", po::value (&plex_token_),
       "Plex account authentication token (not required if provided via config "
       "file).")
      /* TIZ_CLASS_COMMENT: */
      ("plex-music-section", po::value (&plex_section_),
       "Name of the Plex music section (needed if different from 'Music'; "
       "may be provided via config file).")
      /* TIZ_CLASS_COMMENT: */
      ("plex-audio-tracks", po::value (&plex_audio_tracks_),
       "Search and play audio tracks from a Plex server.")
      /* TIZ_CLASS_COMMENT: */
      ("plex-audio-artist", po::value (&plex_audio_artist_),
       "Search and play an artist's audio tracks from a Plex server.")
      /* TIZ_CLASS_COMMENT: */
      ("plex-audio-album", po::value (&plex_audio_album_),
       "Search and play a music album from a Plex server.")
      /* TIZ_CLASS_COMMENT: */
      ("plex-audio-playlist", po::value (&plex_audio_playlist_),
       "Search and play playlists from a Plex server.");

  register_consume_function (&tiz::programopts::consume_plex_client_options);
  all_plex_client_options_
      = boost::assign::list_of ("plex-server-base-url") ("plex-auth-token") (
            "plex-music-section") ("plex-audio-tracks") ("plex-audio-artist") (
            "plex-audio-album") ("plex-audio-playlist")
            .convert_to_container< std::vector< std::string > > ();
}

void tiz::programopts::init_iheart_options ()
{
  iheart_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("iheart-search", po::value (&iheart_search_),
       "iheart station search.")
          /* TIZ_CLASS_COMMENT: */
      ("iheart-keywords",
       po::value< std::vector< std::string > > (&iheart_keywords_)
           ->multitoken ()
           ->composing (),
       "Additional keywords that may be used in conjunction with the iheart "
       "search option. Optional (may be repeated).");

  register_consume_function (&tiz::programopts::consume_iheart_client_options);
  all_iheart_client_options_
      = boost::assign::list_of ("iheart-search") ("iheart-keywords")
            .convert_to_container< std::vector< std::string > > ();
}

void tiz::programopts::init_input_uri_option ()
{
  input_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("input-uris",
       po::value< std::vector< std::string > > (&uri_list_)->multitoken (),
       "input file");
  positional_.add ("input-uris", -1);
  register_consume_function (&tiz::programopts::consume_local_decode_options);
  all_input_uri_options_
      = boost::assign::list_of ("input-uris")
            .convert_to_container< std::vector< std::string > > ();
}

uint32_t tiz::programopts::parse_command_line (int argc, char *argv[])
{
  // Declare an options description instance which will include
  // all the options
  po::options_description all ("All options");
  all.add (global_)
      .add (debug_)
      .add (omx_)
      .add (server_)
      .add (client_)
#ifdef HAVE_LIBSPOTIFY
      .add (spotify_)
#endif
      .add (gmusic_)
      .add (scloud_)
      .add (tunein_)
      .add (youtube_)
      .add (plex_)
      .add (iheart_)
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
#ifdef HAVE_LIBSPOTIFY
    else if (0 == help_option_.compare ("spotify"))
    {
      print_usage_feature (spotify_);
    }
#endif
    else if (0 == help_option_.compare ("googlemusic"))
    {
      print_usage_feature (gmusic_);
    }
    else if (0 == help_option_.compare ("soundcloud"))
    {
      print_usage_feature (scloud_);
    }
    else if (0 == help_option_.compare ("tunein"))
    {
      print_usage_feature (tunein_);
    }
    else if (0 == help_option_.compare ("youtube"))
    {
      print_usage_feature (youtube_);
    }
    else if (0 == help_option_.compare ("plex"))
    {
      print_usage_feature (plex_);
    }
    else if (0 == help_option_.compare ("iheart"))
    {
      print_usage_feature (iheart_);
    }
    else if (0 == help_option_.compare ("chromecast"))
    {
      print_usage_feature (chromecast_);
    }
    else if (0 == help_option_.compare ("proxy"))
    {
      print_usage_feature (proxy_);
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

  if (proxy_server_.empty ())
  {
    retrieve_string_from_rc_file ("tizonia", "proxy.server", proxy_server_);
  }
  if (proxy_user_.empty ())
  {
    retrieve_string_from_rc_file ("tizonia", "proxy.user_name", proxy_user_);
  }
  if (proxy_password_.empty ())
  {
    retrieve_string_from_rc_file ("tizonia", "proxy.user_password",
                                  proxy_password_);
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
    if (chromecast_name_or_ip_.empty ())
    {
      rc = call_handler (option_handlers_map_.find ("decode-stream"));
    }
    else
    {
      rc = call_handler (option_handlers_map_.find ("http-stream-chromecast"));
    }
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

    const int playlist_option_count
        = vm_.count ("spotify-tracks") + vm_.count ("spotify-artist")
          + vm_.count ("spotify-album") + vm_.count ("spotify-playlist")
          + vm_.count ("spotify-track-id") + vm_.count ("spotify-artist-id")
          + vm_.count ("spotify-album-id") + vm_.count ("spotify-playlist-id")
          + vm_.count ("spotify-related-artists")
          + vm_.count ("spotify-featured-playlist")
          + vm_.count ("spotify-new-releases")
          + vm_.count ("spotify-recommendations-by-track-id")
          + vm_.count ("spotify-recommendations-by-artist-id")
          + vm_.count ("spotify-recommendations-by-genre")
          + vm_.count ("spotify-user-liked-tracks")
          + vm_.count ("spotify-user-recent-tracks")
          + vm_.count ("spotify-user-top-tracks")
          + vm_.count ("spotify-user-top-artists")
          + vm_.count ("spotify-user-playlist");

    if (spotify_user_.empty ())
    {
      retrieve_string_from_rc_file ("tizonia", "spotify.user", spotify_user_);
    }
    if (spotify_pass_.empty ())
    {
      retrieve_string_from_rc_file ("tizonia", "spotify.password",
                                    spotify_pass_);
    }
    if (spotify_owner_.empty ())
    {
      spotify_owner_ = spotify_user_;
    }

    // This is to find out if the spotify-recover-lost-token flag has  been
    // provided on the command line, and the retrieve from the config file.
    // See https://stackoverflow.com/questions/32150230/boost-program-options-bool-always-true
    bool recover_token_option_provided
        = (std::find (all_given_options_.begin (), all_given_options_.end (),
                      std::string ("spotify-recover-lost-token"))
           != all_given_options_.end ());
    if (!recover_token_option_provided)
    {
      if (!retrieve_bool_from_rc_file_if_found ("tizonia", "spotify.recover_lost_token",
                                                spotify_recover_lost_token_))
        {
          // Just make sure we always default this to false when the flag is
          // not configured in tizonia.conf.
          spotify_recover_lost_token_ = false;
        }
    }

    // This is to find out if the spotify-allow-explicit-tracks flag has  been
    // provided on the command line, and then retrieve from the config file.
    // See https://stackoverflow.com/questions/32150230/boost-program-options-bool-always-true
    bool allow_explicit_tracks_provided
        = (std::find (all_given_options_.begin (), all_given_options_.end (),
                      std::string ("spotify-allow-explicit-tracks"))
           != all_given_options_.end ());
    if (!allow_explicit_tracks_provided)
    {
      if (!retrieve_bool_from_rc_file_if_found ("tizonia", "spotify.allow_explicit_tracks",
                                                spotify_allow_explicit_tracks_))
        {
          // Just make sure we always default this to false when the flag is
          // not configured in tizonia.conf.
          spotify_allow_explicit_tracks_ = false;
        }
    }

    if (!spotify_preferred_bitrate_)
    {
      retrieve_tizonia_uint_from_rc_file ("spotify.preferred_bitrate",
                                          spotify_preferred_bitrate_);
    }

    if (vm_.count ("spotify-user-liked-tracks"))
    {
      spotify_user_liked_tracks_.assign ("liked tracks");
    }
    else if (vm_.count ("spotify-user-recent-tracks"))
    {
      spotify_user_recent_tracks_.assign ("recent tracks");
    }
    else if (vm_.count ("spotify-user-top-tracks"))
    {
      spotify_user_top_tracks_.assign ("top tracks");
    }
    else if (vm_.count ("spotify-user-top-artists"))
    {
      spotify_user_top_artists_.assign ("top artists");
    }

    if (spotify_user_.empty ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "Need to provide a Spotify user name.";
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
    else if (OMX_AUDIO_SpotifyPlaylistTypePlaylist != spotify_playlist_type ()
             && OMX_AUDIO_SpotifyPlaylistTypePlaylistId != spotify_playlist_type ()
             && vm_.count ("spotify-owner"))
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "The --spotify-owner option can only be used in conjunction with\n"
          << " --spotify-playlist and --spotify-playlist-id";
      msg.assign (oss.str ());
    }
    else if (OMX_AUDIO_SpotifyPlaylistTypeUnknown == spotify_playlist_type ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "A playlist value must be specified.";
      msg.assign (oss.str ());
    }
    else
    {
      if (chromecast_name_or_ip_.empty ())
      {
        rc = call_handler (option_handlers_map_.find ("spotify-stream"));
      }
      else
      {
        rc = EXIT_FAILURE;
        std::ostringstream oss;
        oss << "The --cast option is currently not available with Spotify.";
        msg.assign (oss.str ());
      }
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

    const int playlist_option_count
        = vm_.count ("gmusic-library") + vm_.count ("gmusic-tracks")
          + vm_.count ("gmusic-artist") + vm_.count ("gmusic-album")
          + vm_.count ("gmusic-playlist") + vm_.count ("gmusic-podcast")
          + vm_.count ("gmusic-station")
          + vm_.count ("gmusic-unlimited-station")
          + vm_.count ("gmusic-unlimited-album")
          + vm_.count ("gmusic-unlimited-artist")
          + vm_.count ("gmusic-unlimited-tracks")
          + vm_.count ("gmusic-unlimited-playlist")
          + vm_.count ("gmusic-unlimited-genre")
          + vm_.count ("gmusic-unlimited-activity")
          + vm_.count ("gmusic-unlimited-feeling-lucky-station")
          + vm_.count ("gmusic-unlimited-promoted-tracks");

    if (gmusic_user_.empty ())
    {
      retrieve_string_from_rc_file ("tizonia", "gmusic.user", gmusic_user_);
    }
    if (gmusic_pass_.empty ())
    {
      retrieve_string_from_rc_file ("tizonia", "gmusic.password", gmusic_pass_);
    }
    if (gmusic_device_id_.empty ())
    {
      retrieve_string_from_rc_file ("tizonia", "gmusic.device_id",
                                    gmusic_device_id_);
    }
    if (!buffer_seconds_)
    {
      retrieve_tizonia_uint_from_rc_file ("gmusic.buffer_seconds",
                                          gmusic_buffer_seconds_);
    }

    if (vm_.count ("gmusic-library"))
    {
      // This is not going to be used by the client code, but will help
      // in gmusic_playlist_type() to decide which playlist type value is
      // returned.
      gmusic_library_.assign ("Google Play Music full library playback");
    }

    if (vm_.count ("gmusic-unlimited-promoted-tracks"))
    {
      // This is not going to be used by the client code, but will help
      // in gmusic_playlist_type() to decide which playlist type value is
      // returned.
      gmusic_promoted_.assign ("Google Play Music Unlimited promoted tracks");
    }

    if (vm_.count ("gmusic-unlimited-feeling-lucky-station"))
    {
      gmusic_feeling_lucky_station_.assign ("I'm Feeling Lucky");
    }

    if (vm_.count ("gmusic-unlimited-station")
        || vm_.count ("gmusic-unlimited-album")
        || vm_.count ("gmusic-unlimited-artist")
        || vm_.count ("gmusic-unlimited-tracks")
        || vm_.count ("gmusic-unlimited-playlist")
        || vm_.count ("gmusic-unlimited-genre")
        || vm_.count ("gmusic-unlimited-activity"))
    {
      gmusic_is_unlimited_search_ = true;
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
    else if (OMX_AUDIO_GmusicPlaylistTypeSituation != gmusic_playlist_type ()
             && vm_.count ("gmusic-additional-keywords"))
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "The --gmusic-additional-keywords option can only be used in conjunction with\n"
          << " --gmusic-unlimited-activity";
      msg.assign (oss.str ());
    }
    else if (OMX_AUDIO_GmusicPlaylistTypeUnknown == gmusic_playlist_type ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "A playlist value must be specified.";
      msg.assign (oss.str ());
    }
    else
    {
      if (chromecast_name_or_ip_.empty ())
      {
        rc = call_handler (option_handlers_map_.find ("gmusic-stream"));
      }
      else
      {
        rc = call_handler (
            option_handlers_map_.find ("gmusic-stream-chromecast"));
      }
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

    const int playlist_option_count
        = vm_.count ("soundcloud-user-stream")
          + vm_.count ("soundcloud-user-likes")
          + vm_.count ("soundcloud-user-playlist")
          + vm_.count ("soundcloud-creator") + vm_.count ("soundcloud-tracks")
          + vm_.count ("soundcloud-playlists") + vm_.count ("soundcloud-genres")
          + vm_.count ("soundcloud-tags");

    if (scloud_oauth_token_.empty ())
    {
      retrieve_string_from_rc_file ("tizonia", "soundcloud.oauth_token",
                                    scloud_oauth_token_);
    }
    if (!buffer_seconds_)
    {
      retrieve_tizonia_uint_from_rc_file ("soundcloud.buffer_seconds",
                                          scloud_buffer_seconds_);
    }

    if (vm_.count ("soundcloud-user-stream"))
    {
      // This is not going to be used by the client code, but will help
      // in sclound_playlist_type() to decide which playlist type value is
      // returned.
      scloud_user_stream_.assign ("SoundCloud user stream");
    }

    if (vm_.count ("soundcloud-user-likes"))
    {
      // This is not going to be used by the client code, but will help
      // in sclound_playlist_type() to decide which playlist type value is
      // returned.
      scloud_user_likes_.assign ("SoundCloud user likes");
    }

    if (scloud_oauth_token_.empty ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "Need to provide a SoundCloud user OAuth token.";
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
    else if (OMX_AUDIO_SoundCloudPlaylistTypeUnknown == scloud_playlist_type ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "A playlist value must be specified.";
      msg.assign (oss.str ());
    }
    else
    {
      if (chromecast_name_or_ip_.empty ())
      {
        rc = call_handler (option_handlers_map_.find ("scloud-stream"));
      }
      else
      {
        rc = call_handler (
            option_handlers_map_.find ("scloud-stream-chromecast"));
      }
    }
  }
  TIZ_PRINTF_DBG_RED ("scloud ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_tunein_client_options (bool &done,
                                                     std::string &msg)
{
  int rc = EXIT_FAILURE;
  done = false;

  if (validate_tunein_client_options ())
  {
    done = true;

    const int playlist_option_count
        = vm_.count ("tunein-search") + vm_.count ("tunein-local")
          + vm_.count ("tunein-music") + vm_.count ("tunein-talk")
          + vm_.count ("tunein-sports") + vm_.count ("tunein-location")
          + vm_.count ("tunein-podcasts") + vm_.count ("tunein-trending");

    if (!buffer_seconds_)
    {
      retrieve_tizonia_uint_from_rc_file ("tunein.buffer_seconds",
                                          tunein_buffer_seconds_);
    }

    if (vm_.count ("tunein-local"))
    {
      tunein_category_.assign ("local");
      tunein_playlist_container_.push_back (tunein_category_);
      tunein_playlist_container_.push_back (tunein_local_);
    }
    else if (vm_.count ("tunein-music"))
    {
      tunein_category_.assign ("music");
      tunein_playlist_container_.push_back (tunein_category_);
      tunein_playlist_container_.push_back (tunein_music_);
    }
    else if (vm_.count ("tunein-talk"))
    {
      tunein_category_.assign ("talk");
      tunein_playlist_container_.push_back (tunein_category_);
      tunein_playlist_container_.push_back (tunein_talk_);
    }
    else if (vm_.count ("tunein-sports"))
    {
      tunein_category_.assign ("sports");
      tunein_playlist_container_.push_back (tunein_category_);
      tunein_playlist_container_.push_back (tunein_sports_);
    }
    else if (vm_.count ("tunein-location"))
    {
      tunein_category_.assign ("location");
      tunein_playlist_container_.push_back (tunein_category_);
      tunein_playlist_container_.push_back (tunein_location_);
    }
    else if (vm_.count ("tunein-podcasts"))
    {
      tunein_category_.assign ("podcasts");
      tunein_playlist_container_.push_back (tunein_category_);
      tunein_playlist_container_.push_back (tunein_podcasts_);
    }
    else if (vm_.count ("tunein-trending"))
    {
      tunein_category_.assign ("trending");
      tunein_playlist_container_.push_back (tunein_category_);
      tunein_playlist_container_.push_back (tunein_trending_);
    }

    if (!vm_.count ("tunein-type"))
    {
      tunein_search_type_filter_.assign ("all");
    }

    if (playlist_option_count > 1)
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
    else if (OMX_AUDIO_TuneinPlaylistTypeUnknown == tunein_playlist_type ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "A playlist value must be specified.";
      msg.assign (oss.str ());
    }
    else if (!(0 == tunein_search_type_filter_.compare ("all")
               || 0 == tunein_search_type_filter_.compare ("stations")
               || 0 == tunein_search_type_filter_.compare ("shows")))
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "tunein-type: only one of 'stations', 'shows', or 'all' must be "
             "specified.";
      msg.assign (oss.str ());
    }
    else
    {
      if (chromecast_name_or_ip_.empty ())
      {
        rc = call_handler (option_handlers_map_.find ("tunein-stream"));
      }
      else
      {
        rc = call_handler (
            option_handlers_map_.find ("tunein-stream-chromecast"));
      }
    }
  }
  TIZ_PRINTF_DBG_RED ("tunein ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_youtube_client_options (bool &done,
                                                      std::string &msg)
{
  int rc = EXIT_FAILURE;
  done = false;

  if (validate_youtube_client_options ())
  {
    done = true;

    const int playlist_option_count = vm_.count ("youtube-audio-stream")
                                      + vm_.count ("youtube-audio-playlist")
                                      + vm_.count ("youtube-audio-mix")
                                      + vm_.count ("youtube-audio-search")
                                      + vm_.count ("youtube-audio-mix-search")
                                      + vm_.count ("youtube-audio-channel-uploads")
                                      + vm_.count ("youtube-audio-channel-playlist");

    if (youtube_api_key_.empty())
    {
      retrieve_string_from_rc_file ("tizonia", "youtube.api_key",
                                    youtube_api_key_);
    }

    if (!buffer_seconds_)
    {
      retrieve_tizonia_uint_from_rc_file ("youtube.buffer_seconds",
                                          youtube_buffer_seconds_);
    }

    if (playlist_option_count > 1)
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
      oss << "A playlist type must be specified.";
      msg.assign (oss.str ());
    }
    else if (OMX_AUDIO_YoutubePlaylistTypeUnknown == youtube_playlist_type ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "A playlist value must be specified.";
      msg.assign (oss.str ());
    }
    else
    {
      if (chromecast_name_or_ip_.empty ())
      {
        rc = call_handler (option_handlers_map_.find ("youtube-stream"));
      }
      else
      {
        rc = call_handler (
            option_handlers_map_.find ("youtube-stream-chromecast"));
      }
    }
  }
  TIZ_PRINTF_DBG_RED ("youtube ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_plex_client_options (bool &done, std::string &msg)
{
  int rc = EXIT_FAILURE;
  done = false;

  if (validate_plex_client_options ())
  {
    done = true;

    const int playlist_option_count
        = vm_.count ("plex-audio-tracks") + vm_.count ("plex-audio-artist")
          + vm_.count ("plex-audio-album") + vm_.count ("plex-audio-playlist");

    if (plex_base_url_.empty ())
    {
      retrieve_string_from_rc_file ("tizonia", "plex.base_url",
                                    plex_base_url_);
    }

    if (plex_token_.empty ())
    {
      retrieve_string_from_rc_file ("tizonia", "plex.auth_token",
                                    plex_token_);
    }

    if (plex_section_.empty ())
    {
      std::string music_section_name;
      retrieve_string_from_rc_file ("tizonia", "plex.music_section_name",
                                    music_section_name);
      if (!music_section_name.empty ())
      {
        plex_section_.assign (music_section_name);
      }
      else
      {
        plex_section_.assign ("Music");
      }
    }

    if (!buffer_seconds_)
    {
      retrieve_tizonia_uint_from_rc_file ("plex.buffer_seconds",
                                          plex_buffer_seconds_);
    }

    if (playlist_option_count > 1)
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
      oss << "A playlist type must be specified.";
      msg.assign (oss.str ());
    }
    else if (OMX_AUDIO_PlexPlaylistTypeUnknown == plex_playlist_type ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "A playlist value must be specified.";
      msg.assign (oss.str ());
    }
    else
    {
      if (chromecast_name_or_ip_.empty ())
      {
        rc = call_handler (option_handlers_map_.find ("plex-stream"));
      }
      else
      {
        // TODO: Normal Plex stream URLs don't work on chromecast devices
        //  rc = call_handler (
        //              option_handlers_map_.find ("plex-stream-chromecast"));
        rc = EXIT_FAILURE;
        std::ostringstream oss;
        oss << "The --cast option is currently not available with Plex.";
        msg.assign (oss.str ());
      }
    }
  }
  TIZ_PRINTF_DBG_RED ("plex ; rc = [%s]\n",
                      rc == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
  return rc;
}

int tiz::programopts::consume_iheart_client_options (bool &done,
                                                     std::string &msg)
{
  int rc = EXIT_FAILURE;
  done = false;

  if (validate_iheart_client_options ())
  {
    done = true;

    const int playlist_option_count
        = vm_.count ("iheart-search");

    if (!buffer_seconds_)
    {
      retrieve_tizonia_uint_from_rc_file ("iheart.buffer_seconds",
                                          iheart_buffer_seconds_);
    }

    if (playlist_option_count > 1)
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
    else if (OMX_AUDIO_IheartPlaylistTypeUnknown == iheart_playlist_type ())
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "A playlist value must be specified.";
      msg.assign (oss.str ());
    }
    else
    {
      if (chromecast_name_or_ip_.empty ())
      {
        rc = call_handler (option_handlers_map_.find ("iheart-stream"));
      }
      else
      {
        rc = call_handler (
            option_handlers_map_.find ("iheart-stream-chromecast"));
      }
    }
  }
  TIZ_PRINTF_DBG_RED ("iheart ; rc = [%s]\n",
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
    if (chromecast_name_or_ip_.empty ())
    {
      rc = EXIT_SUCCESS;
      done = true;
      rc = call_handler (option_handlers_map_.find ("decode-local"));
    }
    else
    {
      rc = EXIT_FAILURE;
      std::ostringstream oss;
      oss << "The --cast option is currently not available with local media.";
      msg.assign (oss.str ());
    }
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
    uri_list_ = vm_["input-uris"].as< std::vector< std::string > > ();
  }
  return rc;
}

int tiz::programopts::consume_input_http_uris_option ()
{
  int rc = EXIT_FAILURE;
  if (vm_.count ("input-uris"))
  {
    uri_list_ = vm_["input-uris"].as< std::vector< std::string > > ();
    bool all_ok = true;
    BOOST_FOREACH (std::string uri, uri_list_)
    {
      std::transform(uri.begin(), uri.end(), uri.begin(), ::tolower);
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
  const uint32_t omx_opts_count = vm_.count ("comp-list")
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
  uint32_t spotify_opts_count
      = vm_.count ("spotify-user") + vm_.count ("spotify-password")
        + vm_.count ("spotify-owner") + vm_.count ("spotify-recover-lost-token")
        + vm_.count ("spotify-allow-explicit-tracks")
        + vm_.count ("spotify-tracks") + vm_.count ("spotify-artist")
        + vm_.count ("spotify-album") + vm_.count ("spotify-playlist")
        + vm_.count ("spotify-track-id") + vm_.count ("spotify-artist-id")
        + vm_.count ("spotify-album-id") + vm_.count ("spotify-playlist-id")
        + vm_.count ("spotify-related-artists")
        + vm_.count ("spotify-featured-playlist")
        + vm_.count ("spotify-new-releases")
        + vm_.count ("spotify-recommendations-by-track-id")
        + vm_.count ("spotify-recommendations-by-artist-id")
        + vm_.count ("spotify-recommendations-by-genre")
        + vm_.count ("spotify-user-liked-tracks")
        + vm_.count ("spotify-user-recent-tracks")
        + vm_.count ("spotify-user-top-tracks")
        + vm_.count ("spotify-user-top-artists")
        + vm_.count ("spotify-user-playlist") + vm_.count ("log-directory");

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
  uint32_t gmusic_opts_count
      = vm_.count ("gmusic-user") + vm_.count ("gmusic-password")
        + vm_.count ("gmusic-device-id")
        + vm_.count ("gmusic-additional-keywords")
        + vm_.count ("gmusic-library") + vm_.count ("gmusic-tracks")
        + vm_.count ("gmusic-artist") + vm_.count ("gmusic-album")
        + vm_.count ("gmusic-playlist") + vm_.count ("gmusic-podcast")
        + vm_.count ("gmusic-station") + vm_.count ("gmusic-unlimited-station")
        + vm_.count ("gmusic-unlimited-album")
        + vm_.count ("gmusic-unlimited-artist")
        + vm_.count ("gmusic-unlimited-tracks")
        + vm_.count ("gmusic-unlimited-playlist")
        + vm_.count ("gmusic-unlimited-genre")
        + vm_.count ("gmusic-unlimited-activity")
        + vm_.count ("gmusic-unlimited-feeling-lucky-station")
        + vm_.count ("gmusic-unlimited-promoted-tracks")
        + vm_.count ("log-directory");

  std::vector< std::string > all_valid_options = all_gmusic_client_options_;
  concat_option_lists (all_valid_options, all_global_options_);
  concat_option_lists (all_valid_options, all_debug_options_);

  if (gmusic_opts_count > 0
      && is_valid_options_combination (all_valid_options, all_given_options_))
  {
    outcome = true;
  }
  TIZ_PRINTF_DBG_RED ("outcome = [%s]\n", outcome ? "SUCCESS" : "FAILURE");
  return outcome;
}

bool tiz::programopts::validate_scloud_client_options () const
{
  bool outcome = false;
  uint32_t scloud_opts_count
      = vm_.count ("soundcloud-oauth-token")
        + vm_.count ("soundcloud-user-stream")
        + vm_.count ("soundcloud-user-likes")
        + vm_.count ("soundcloud-user-playlist")
        + vm_.count ("soundcloud-creator") + vm_.count ("soundcloud-tracks")
        + vm_.count ("soundcloud-playlists") + vm_.count ("soundcloud-genres")
        + vm_.count ("soundcloud-tags") + vm_.count ("log-directory");

  std::vector< std::string > all_valid_options = all_scloud_client_options_;
  concat_option_lists (all_valid_options, all_global_options_);
  concat_option_lists (all_valid_options, all_debug_options_);

  if (scloud_opts_count > 0
      && is_valid_options_combination (all_valid_options, all_given_options_))
  {
    outcome = true;
  }
  TIZ_PRINTF_DBG_RED ("outcome = [%s]\n", outcome ? "SUCCESS" : "FAILURE");
  return outcome;
}

bool tiz::programopts::validate_tunein_client_options () const
{
  bool outcome = false;
  uint32_t tunein_opts_count
      = vm_.count ("tunein-search") + vm_.count ("tunein-type")
        + vm_.count ("tunein-local") + vm_.count ("tunein-music")
        + vm_.count ("tunein-talk") + vm_.count ("tunein-sports")
        + vm_.count ("tunein-location") + vm_.count ("tunein-podcasts")
        + vm_.count ("tunein-trending") + vm_.count ("log-directory");

  std::vector< std::string > all_valid_options = all_tunein_client_options_;
  concat_option_lists (all_valid_options, all_global_options_);
  concat_option_lists (all_valid_options, all_debug_options_);

  if (tunein_opts_count > 0
      && is_valid_options_combination (all_valid_options, all_given_options_))
  {
    outcome = true;
  }
  TIZ_PRINTF_DBG_RED ("outcome = [%s]\n", outcome ? "SUCCESS" : "FAILURE");
  return outcome;
}

bool tiz::programopts::validate_youtube_client_options () const
{
  bool outcome = false;
  uint32_t youtube_opts_count
      = vm_.count ("youtube-api-key") + vm_.count ("youtube-audio-stream")
        + vm_.count ("youtube-audio-playlist") + vm_.count ("youtube-audio-mix")
        + vm_.count ("youtube-audio-search")
        + vm_.count ("youtube-audio-mix-search")
        + vm_.count ("youtube-audio-channel-uploads")
        + vm_.count ("youtube-audio-channel-playlist");

  std::vector< std::string > all_valid_options = all_youtube_client_options_;
  concat_option_lists (all_valid_options, all_global_options_);
  concat_option_lists (all_valid_options, all_debug_options_);

  if (youtube_opts_count > 0
      && is_valid_options_combination (all_valid_options, all_given_options_))
  {
    outcome = true;
  }
  TIZ_PRINTF_DBG_RED ("outcome = [%s]\n", outcome ? "SUCCESS" : "FAILURE");
  return outcome;
}

bool tiz::programopts::validate_plex_client_options () const
{
  bool outcome = false;
  uint32_t plex_opts_count
      = vm_.count ("plex-server-base-url") + vm_.count ("plex-auth-token")
        + vm_.count ("plex-music-section") + vm_.count ("plex-audio-tracks")
        + vm_.count ("plex-audio-artist") + vm_.count ("plex-audio-album")
        + vm_.count ("plex-audio-playlist");

  std::vector< std::string > all_valid_options = all_plex_client_options_;
  concat_option_lists (all_valid_options, all_global_options_);
  concat_option_lists (all_valid_options, all_debug_options_);

  if (plex_opts_count > 0
      && is_valid_options_combination (all_valid_options, all_given_options_))
  {
    outcome = true;
  }
  TIZ_PRINTF_DBG_RED ("outcome = [%s]\n", outcome ? "SUCCESS" : "FAILURE");
  return outcome;
}

bool tiz::programopts::validate_iheart_client_options () const
{
  bool outcome = false;
  uint32_t iheart_opts_count = vm_.count ("iheart-search");

  std::vector< std::string > all_valid_options = all_iheart_client_options_;
  concat_option_lists (all_valid_options, all_global_options_);
  concat_option_lists (all_valid_options, all_debug_options_);

  if (iheart_opts_count > 0
      && is_valid_options_combination (all_valid_options, all_given_options_))
  {
    outcome = true;
  }
  TIZ_PRINTF_DBG_RED ("outcome = [%s]\n", outcome ? "SUCCESS" : "FAILURE");
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
