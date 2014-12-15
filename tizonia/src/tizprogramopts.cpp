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
 * @file   tizprogramopts.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Program options parsing utility.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include <tizplatform.h>

#include "tizprogramopts.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.programopts"
#endif

namespace po = boost::program_options;

namespace
{
  const int TIZ_STREAMING_SERVER_DEFAULT_PORT = 8010;
  const int TIZ_MAX_BITRATE_MODES = 2;

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
}

tiz::programopts::programopts (int argc, char *argv[])
  : argc_ (argc),
    argv_ (argv),
    option_handlers_map_ (),
    vm_ (),
    general_ ("General options"),
    debug_ ("Debug options"),
    omx_ ("OpenMAX IL options"),
    server_ ("Audio streaming server options"),
    client_ ("Audio streaming client options"),
    spotify_ ("Spotify client options"),
    input_ ("Intput uris option"),
    positional_ (),
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
    bitrates_ (),
    bitrate_list_ (),
    sampling_rates_ (),
    sampling_rate_list_ (),
    uri_list_ (),
    spotify_user_ (),
    spotify_pass_ (),
    spotify_playlist_ (),
    spotify_playlist_container_ ()
{
  init_general_options ();
  init_debug_options ();
  init_omx_options ();
  init_streaming_server_options ();
  init_streaming_client_options ();
  init_spotify_options ();
  init_input_uri_option ();
}

int tiz::programopts::consume ()
{
#define PROGRAMOPTS_RETURN_IF_TRUE(expr) \
  do                                          \
  {                                           \
    if ((expr))                               \
    {                                         \
      return rc;                              \
    }                                         \
  } while (0)

  int rc = EXIT_FAILURE;
  try
  {
    bool done = false;

    parse_command_line (argc_, argv_);

    rc = consume_debug_options (done);
    PROGRAMOPTS_RETURN_IF_TRUE (done);

    rc = consume_general_options (done);
    PROGRAMOPTS_RETURN_IF_TRUE (done);

    rc = consume_omx_options (done);
    PROGRAMOPTS_RETURN_IF_TRUE (done);

    rc = consume_streaming_server_options (done);
    PROGRAMOPTS_RETURN_IF_TRUE (done);

    rc = consume_streaming_client_options (done);
    PROGRAMOPTS_RETURN_IF_TRUE (done);

    rc = consume_spotify_client_options (done);
    PROGRAMOPTS_RETURN_IF_TRUE (done);

    rc = consume_local_decode_options (done);
    PROGRAMOPTS_RETURN_IF_TRUE (done);

    print_usage ();
  }
  catch (std::exception &e)
  {
    std::cerr << e.what () << "\n\n";
    print_usage ();
  }
  return rc;
}

void tiz::programopts::print_version () const
{
  TIZ_PRINTF_BLU ("tizonia %s. Copyright (C) 2014 Juan A. Rubio\n", PACKAGE_VERSION);
  TIZ_PRINTF_BLU ("This software is part of Tizonia <http://tizonia.org>\n\n");
}

void tiz::programopts::print_usage () const
{
  print_version ();
  print_license ();
  // Note: We don't show here debug_ or input_ options, they are hidden from the
  // user.
  std::cout << general_ << "\n";
  std::cout << omx_ << "\n";
  std::cout << server_ << "\n";
  std::cout << spotify_ << "\n";
  // Note: We don't show the client_ options for now, but this may be needed in the future
  // std::cout << client_ << "\n";
}

void tiz::programopts::print_usage_extended () const
{
  print_usage ();
  print_examples ();
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

const std::vector< std::string > &tiz::programopts::spotify_playlist_container ()
{
  spotify_playlist_container_.clear ();
  spotify_playlist_container_.push_back (spotify_playlist_);
  return spotify_playlist_container_;
}

void tiz::programopts::print_license () const
{
  printf (
      "LGPLv3: GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl.html>\n"
      "This is free software: you are free to change and redistribute it.\n"
      "There is NO WARRANTY, to the extent permitted by law.\n\n");
}

void tiz::programopts::print_examples () const
{
  printf ("Examples:\n");
  printf (" tizonia ~/Music\n\n");
  printf ("    * Decodes every supported file in the '~/Music' directory)\n");
  printf ("    * File formats currently supported for playback:\n");
  printf (
      "      * mp3, aac, (.aac only) flac (.flac, .ogg, .oga), opus "
      "(.opus, .ogg, .oga), "
      "vorbis (.ogg, .oga).\n");
  printf ("    * Basic keys:\n");
  printf ("      * [p] skip to previous file.\n");
  printf ("      * [n] skip to next file.\n");
  printf ("      * [SPACE] pause playback.\n");
  printf ("      * [+/-] increase/decrease volume.\n");
  printf ("      * [m] mute.\n");
  printf ("      * [q] quit.\n");
  printf ("\n tizonia --sampling-rates=44100,48000 -p 8011 --stream ~/Music\n\n");
  printf ("    * This streams files from the '~/Music' directory.\n");
  printf ("    * File formats currently supported for streaming: mp3.\n");
  printf ("    * Sampling rates other than [44100,4800] are ignored.\n");
  printf ("    * Basic keys:\n");
  printf ("      * [q] quit.\n");
  printf ("\n");
}

void tiz::programopts::init_general_options ()
{
  general_.add_options ()
      /* TIZ_CLASS_COMMENT: This is to avoid the clang formatter messing up
         these lines*/
      ("help,h", "Print the usage message.")
      /* TIZ_CLASS_COMMENT: */
      ("version,v", "Print the version information.")
      /* TIZ_CLASS_COMMENT: */
      ("recurse,R", po::bool_switch(&recurse_)->default_value(false), "Recursively process the given folder.")
      /* TIZ_CLASS_COMMENT: */
      ("shuffle,S", po::bool_switch(&shuffle_)->default_value(false), "Shuffle the playlist.")
      /* TIZ_CLASS_COMMENT: */
      ("daemon,d", po::bool_switch(&daemon_)->default_value(false), "Run in the background.")
      /* TIZ_CLASS_COMMENT: */
    ;
}

void tiz::programopts::init_debug_options ()
{
  debug_.add_options ()
      /* TIZ_CLASS_COMMENT: This is to avoid the clang formatter messing up
         these lines*/
      ("log-directory", po::value (&log_dir_),
       "The directory to be used for the debug trace file.")
      ("debug-info", po::bool_switch(&debug_info_)->default_value(false),
       "Print debug-related information.")
      /* TIZ_CLASS_COMMENT: */
      ;
}

void tiz::programopts::init_omx_options ()
{
  omx_.add_options ()
      /* TIZ_CLASS_COMMENT: This is to avoid the clang formatter messing up
         these lines*/
      ("list-comp", "Enumerate all the OpenMAX IL components in the system.")
      /* TIZ_CLASS_COMMENT: */
      ("roles-of-comp", po::value (&comp_name_),
       "Display the OpenMAX IL roles found in component <arg>.")
      /* TIZ_CLASS_COMMENT: */
      ("comps-of-role", po::value (&role_name_),
       "Display the OpenMAX IL components that implement role <arg>.")
      /* TIZ_CLASS_COMMENT: */
      ;
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
       "TCP port used for SHOUTcast/ICEcast streaming. Default: 8010.")
      /* TIZ_CLASS_COMMENT: */
      ("station-name", po::value (&station_name_),
       "The SHOUTcast/ICEcast station name.")
      /* TIZ_CLASS_COMMENT: */
      ("station-genre", po::value (&station_genre_),
       "The SHOUTcast/ICEcast station genre.")
      /* TIZ_CLASS_COMMENT: */
      ("bitrate-modes", po::value (&bitrates_),
       "A comma-separated-list of "
       /* TIZ_CLASS_COMMENT: */
       "bitrate modes (e.g. 'CBR,VBR') that will be allowed in the playlist. "
       "Default: any.")
      /* TIZ_CLASS_COMMENT: */
      ("sampling-rates", po::value (&sampling_rates_),
       "A comma-separated-list "
       /* TIZ_CLASS_COMMENT: */
       "of sampling rates that will be allowed in the playlist. Default: any.")
      /* TIZ_CLASS_COMMENT: */
      ;

  // Give a default value to the bitrate list
  bitrates_ = std::string ("CBR,VBR");
  boost::split (bitrate_list_, bitrates_, boost::is_any_of (","));
}

void tiz::programopts::init_streaming_client_options ()
{
  client_.add_options ()
    /* TIZ_CLASS_COMMENT: */
    ("station-id", po::value (&station_name_),
     "Give a name/id to the remote stream.");
}

void tiz::programopts::init_spotify_options ()
{
  spotify_.add_options ()
    /* TIZ_CLASS_COMMENT: */
    ("spotify-user", po::value (&spotify_user_),
     "Spotify user's name.")
    /* TIZ_CLASS_COMMENT: */
    ("spotify-password", po::value (&spotify_pass_),
     "Spotify user's password.")
    /* TIZ_CLASS_COMMENT: */
    ("spotify-playlist", po::value (&spotify_playlist_),
     "Spotify playlist name.");
}

void tiz::programopts::init_input_uri_option ()
{
  input_.add_options ()
      /* TIZ_CLASS_COMMENT: */
      ("input-uris",
       po::value< std::vector< std::string > >(&uri_list_)->multitoken (),
       "input file")
      /* TIZ_CLASS_COMMENT: */
      ;
  positional_.add ("input-uris", -1);
}

void tiz::programopts::parse_command_line (int argc, char *argv[])
{
  // Declare an options description instance which will include
  // all the options
  po::options_description all ("All options");
  all.add (general_).add (debug_).add (omx_).add (server_).add (client_)
    .add (spotify_).add (input_);
  po::parsed_options parsed = po::command_line_parser (argc, argv)
                                  .options (all)
                                  .positional (positional_)
                                  .run ();
  po::store (parsed, vm_);
  po::notify (vm_);
  uri_list_ = po::collect_unrecognized (parsed.options, po::include_positional);
}

int tiz::programopts::consume_debug_options (bool &done)
{
  done = false;
  if (vm_.count ("log-directory"))
  {
    (void)call_handler (option_handlers_map_.find ("log-directory"));
  }
  if (vm_.count ("debug-info") && debug_info_)
  {
    (void)call_handler (option_handlers_map_.find ("debug-info"));
    done = true;
  }
  return EXIT_SUCCESS;
}

int tiz::programopts::consume_general_options (bool &done)
{
  int result = EXIT_SUCCESS;
  done = false;
  if (vm_.count ("help"))
  {
    print_usage_extended ();
    done = true;
  }
  else if (vm_.count ("version"))
  {
    print_version ();
    done = true;
  }
  return result;
}

int tiz::programopts::consume_omx_options (bool &done)
{
  int result = EXIT_FAILURE;
  done = false;

  if (vm_.count ("list-comp") || vm_.count ("roles-of-comp")
      || vm_.count ("comps-of-role"))
  {
    done = true;

    if (omx_conflicting_options (vm_, "list-comp", "roles-of-comp")
        || omx_conflicting_options (vm_, "list-comp", "comps-of-role")
        || omx_conflicting_options (vm_, "roles-of-comp", "comps-of-role"))
    {
      std::cerr << "Only one of '--list-comp', '--roles-of-comp', "
                   "'--comps-of-role' can be specified.";
      result = EXIT_FAILURE;
    }
    else
    {
      if (vm_.count ("list-comp"))
      {
        result = call_handler (option_handlers_map_.find ("list-comp"));
      }
      else if (vm_.count ("roles-of-comp"))
      {
        result = call_handler (option_handlers_map_.find ("roles-of-comp"));
      }
      else if (vm_.count ("comps-of-role"))
      {
        result = call_handler (option_handlers_map_.find ("comps-of-role"));
      }
      result = EXIT_SUCCESS;
    }
  }
  return result;
}

int tiz::programopts::consume_streaming_server_options (bool &done)
{
#define PROGRAMOPTS_RETURN_IF_FAIL(expr) \
  do                                     \
  {                                      \
    if (!(expr))                         \
    {                                    \
      return EXIT_FAILURE;               \
    }                                    \
  } while (0)

  int result = EXIT_SUCCESS;
  done = false;

  if (vm_.count ("server"))
  {
    done = true;

    PROGRAMOPTS_RETURN_IF_FAIL (verify_port_argument ());
    PROGRAMOPTS_RETURN_IF_FAIL (verify_bitrates_argument ());
    PROGRAMOPTS_RETURN_IF_FAIL (verify_sampling_rates_argument ());
    result = consume_input_file_uris_option ();
    if (EXIT_SUCCESS == result)
    {
      result = call_handler (option_handlers_map_.find ("serve-stream"));
    }
  }
  return result;
}

int tiz::programopts::consume_streaming_client_options (bool &done)
{
  int result = EXIT_FAILURE;
  done = false;
  result = consume_input_http_uris_option ();
  if (EXIT_SUCCESS == result)
  {
    done = true;
    result = call_handler (option_handlers_map_.find ("decode-stream"));
  }
  return result;
}

int tiz::programopts::consume_spotify_client_options (bool &done)
{
  int result = EXIT_FAILURE;
  done = false;

  if (vm_.count ("spotify-user") || vm_.count ("spotify-password")
      || vm_.count ("spotify-playlist"))
  {
    done = true;

    if (!vm_.count ("spotify-user") && vm_.count ("spotify-password"))
    {
      std::cerr << "Need to provide a Spotify user name." << "\n";
      result = EXIT_FAILURE;
    }
    else if (!vm_.count ("spotify-playlist"))
    {
      std::cerr << "At least one playlist must be specified." << "\n";
      result = EXIT_FAILURE;
    }
    else
    {
      result = call_handler (option_handlers_map_.find ("spotify-stream"));
    }
  }
  return result;
}

int tiz::programopts::consume_local_decode_options (bool &done)
{
  int result = EXIT_FAILURE;
  done = false;
  result = consume_input_file_uris_option ();
  if (EXIT_SUCCESS == result)
  {
    done = true;
    result = call_handler (option_handlers_map_.find ("decode-local"));
  }
  return result;
}

int tiz::programopts::consume_input_file_uris_option ()
{
  int result = EXIT_FAILURE;
  if (vm_.count ("input-uris"))
  {
    result = EXIT_SUCCESS;
    uri_list_ = vm_["input-uris"].as< std::vector< std::string > >();
  }

  return result;
}

int tiz::programopts::consume_input_http_uris_option ()
{
  int result = EXIT_FAILURE;
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
    result = all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  return result;
}

bool tiz::programopts::verify_port_argument () const
{
  bool rc = true;
  if (vm_.count ("port"))
  {
    if (port_ <= 1024)
    {
      rc = false;
      std::cerr << "Invalid argument : " << port_ << "\n";
      std::cerr << "Please provide a port number in the range "
                   "[1025-65535]"
                << "\n";
    }
  }
  return rc;
}

bool tiz::programopts::verify_bitrates_argument ()
{
  bool rc = true;
  if (vm_.count ("bitrate-modes"))
  {
    boost::split (bitrate_list_, bitrates_, boost::is_any_of (","));
    if (!is_valid_bitrate_list (bitrate_list_))
    {
      rc = false;
      std::cerr << "Invalid argument : " << bitrates_ << "\n";
      std::cerr << "Valid bitrate mode values : [CBR,VBR]\n";
    }
  }
  return rc;
}

bool tiz::programopts::verify_sampling_rates_argument ()
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
      std::cerr << "Invalid argument : " << sampling_rates_ << "\n";
      std::cerr << "Valid sampling rate values :"
                << "\n";
      std::cerr
          << "[8000,11025,12000,16000,22050,24000,32000,44100,48000,96000]\n";
    }
  }
  return rc;
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
