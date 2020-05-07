/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and
 * contributors
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
 * @file   tizsoundcloud.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple SoundCloud client library
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/lexical_cast.hpp>
#include <iostream>

#include "tizsoundcloud.hpp"

namespace bp = boost::python;

/* This macro assumes the existence of an "int rc" local variable */
#define try_catch_wrapper(expr)                                  \
  do                                                             \
    {                                                            \
      try                                                        \
        {                                                        \
          if (!rc)                                               \
            {                                                    \
              (expr);                                            \
            }                                                    \
        }                                                        \
      catch (bp::error_already_set & e)                          \
        {                                                        \
          PyErr_PrintEx (0);                                     \
          rc = 1;                                                \
        }                                                        \
      catch (const std::exception &e)                            \
        {                                                        \
          std::cerr << e.what ();                                \
          rc = 1;                                                \
        }                                                        \
      catch (...)                                                \
        {                                                        \
          std::cerr << std::string ("Unknown exception caught"); \
          rc = 1;                                                \
        }                                                        \
    }                                                            \
  while (0)

namespace
{
  int check_deps ()
  {
    int rc = 1;
    Py_Initialize ();

    try
      {
        // Import the Google Play Music proxy module
        bp::object py_main = bp::import ("__main__");

        // Retrieve the main module's namespace
        bp::object py_global = py_main.attr ("__dict__");

        // Check the existence of the 'soundcloud' module
        bp::object ignored = exec (
            "import importlib\n"
            "spec = importlib.util.find_spec('soundcloud')\n"
            "if not spec:\n raise ValueError\n",
            py_global);

        // Check the existence of the 'fuzzywuzzy' module
        bp::object ignored2 = exec (
            "import importlib\n"
            "spec = importlib.util.find_spec('fuzzywuzzy')\n"
            "if not spec:\n raise ValueError\n",
            py_global);

        rc = 0;
      }
    catch (bp::error_already_set &e)
      {
        PyErr_PrintEx (0);
        std::cerr << std::string (
            "\nPython modules 'soundcloud' or 'fuzzywuzzy' not found."
            "\nPlease make sure this is installed correctly.\n");
      }
    catch (...)
      {
        std::cerr << std::string ("Unknown exception caught");
      }
    return rc;
  }

  void init_soundcloud (boost::python::object &py_main,
                        boost::python::object &py_global)
  {
    // Import the SoundCloud proxy module
    py_main = bp::import ("tizsoundcloudproxy");

    // Retrieve the main module's namespace
    py_global = py_main.attr ("__dict__");
  }

  void start_soundcloud (boost::python::object &py_global,
                         boost::python::object &py_sc_proxy,
                         const std::string &oauth_token)
  {
    bp::object pysoundcloudproxy = py_global["tizsoundcloudproxy"];
    py_sc_proxy = pysoundcloudproxy (oauth_token.c_str ());
  }
}  // namespace

tizsoundcloud::tizsoundcloud (const std::string &oauth_token)
  : oauth_token_ (oauth_token),
    current_url_ (),
    current_track_index_ (),
    current_queue_length_ (),
    current_queue_length_as_int_ (0),
    current_user_ (),
    current_title_ (),
    current_duration_ (),
    current_track_year_ (),
    current_track_permalink_ (),
    current_track_license_ (),
    current_track_likes_ (),
    current_track_user_avatar_ (),
    current_queue_progress_ ()

{
}

tizsoundcloud::~tizsoundcloud ()
{
}

int tizsoundcloud::init ()
{
  int rc = 0;
  if (0 == (rc = check_deps ()))
    {
      try_catch_wrapper (init_soundcloud (py_main_, py_global_));
    }
  return rc;
}

int tizsoundcloud::start ()
{
  int rc = 0;
  try_catch_wrapper (start_soundcloud (py_global_, py_sc_proxy_, oauth_token_));
  return rc;
}

void tizsoundcloud::stop ()
{
  int rc = 0;
  // try_catch_wrapper (py_sc_proxy_.attr ("logout")());
  (void)rc;
}

void tizsoundcloud::deinit ()
{
  // boost::python doesn't support Py_Finalize() yet!
}

int tizsoundcloud::play_user_stream ()
{
  int rc = 0;
  try_catch_wrapper (py_sc_proxy_.attr ("enqueue_user_stream") ());
  return rc;
}

int tizsoundcloud::play_user_likes ()
{
  int rc = 0;
  try_catch_wrapper (py_sc_proxy_.attr ("enqueue_user_likes") ());
  return rc;
}

int tizsoundcloud::play_user_playlist (const std::string &playlist)
{
  int rc = 0;
  try_catch_wrapper (
      py_sc_proxy_.attr ("enqueue_user_playlist") (bp::object (playlist)));
  return rc;
}

int tizsoundcloud::play_creator (const std::string &creator)
{
  int rc = 0;
  try_catch_wrapper (
      py_sc_proxy_.attr ("enqueue_creator") (bp::object (creator)));
  return rc;
}

int tizsoundcloud::play_tracks (const std::string &tracks)
{
  int rc = 0;
  try_catch_wrapper (
      py_sc_proxy_.attr ("enqueue_tracks") (bp::object (tracks)));
  return rc;
}

int tizsoundcloud::play_playlists (const std::string &playlists)
{
  int rc = 0;
  try_catch_wrapper (
      py_sc_proxy_.attr ("enqueue_playlists") (bp::object (playlists)));
  return rc;
}

int tizsoundcloud::play_genres (const std::string &genres)
{
  int rc = 0;
  try_catch_wrapper (
      py_sc_proxy_.attr ("enqueue_genres") (bp::object (genres)));
  return rc;
}

int tizsoundcloud::play_tags (const std::string &tags)
{
  int rc = 0;
  try_catch_wrapper (py_sc_proxy_.attr ("enqueue_tags") (bp::object (tags)));
  return rc;
}

const char *tizsoundcloud::get_next_url ()
{
  current_url_.clear ();
  try
    {
      current_url_
          = bp::extract< std::string > (py_sc_proxy_.attr ("next_url") ());
      get_current_track ();
    }
  catch (bp::error_already_set &e)
    {
      PyErr_PrintEx (0);
    }
  catch (...)
    {
    }
  return current_url_.empty () ? NULL : current_url_.c_str ();
}

const char *tizsoundcloud::get_url (const int a_position)
{
  try
    {
      int queue_index = 0;
      int queue_length = 0;
      get_current_track_queue_index_and_length (queue_index, queue_length);
      current_url_.clear ();
      if (queue_length > 0 && a_position > 0 && queue_length >= a_position)
        {
          current_url_ = bp::extract< std::string > (
              py_sc_proxy_.attr ("get_url") (bp::object (a_position)));
          get_current_track ();
        }
    }
  catch (bp::error_already_set &e)
    {
      PyErr_PrintEx (0);
    }
  catch (...)
    {
    }
  return current_url_.empty () ? NULL : current_url_.c_str ();
}

const char *tizsoundcloud::get_prev_url ()
{
  current_url_.clear ();
  try
    {
      current_url_
          = bp::extract< std::string > (py_sc_proxy_.attr ("prev_url") ());
      get_current_track ();
    }
  catch (bp::error_already_set &e)
    {
      PyErr_PrintEx (0);
    }
  catch (...)
    {
    }
  return current_url_.empty () ? NULL : current_url_.c_str ();
}

const char *tizsoundcloud::get_current_audio_track_index ()
{
  return current_track_index_.empty () ? NULL : current_track_index_.c_str ();
}

const char *tizsoundcloud::get_current_queue_length ()
{
  return current_queue_length_.empty () ? NULL : current_queue_length_.c_str ();
}

int tizsoundcloud::get_current_queue_length_as_int ()
{
  int queue_index = 0;
  int queue_length = 0;
  get_current_track_queue_index_and_length (queue_index, queue_length);
  return current_queue_length_as_int_;
}

const char *tizsoundcloud::get_current_queue_progress ()
{
  current_queue_progress_.assign (get_current_audio_track_index ());
  current_queue_progress_.append (" of ");
  current_queue_progress_.append (get_current_queue_length ());
  return current_queue_progress_.c_str ();
}

const char *tizsoundcloud::get_current_track_user ()
{
  return current_user_.empty () ? NULL : current_user_.c_str ();
}

const char *tizsoundcloud::get_current_track_title ()
{
  return current_title_.empty () ? NULL : current_title_.c_str ();
}

const char *tizsoundcloud::get_current_track_duration ()
{
  return current_duration_.empty () ? NULL : current_duration_.c_str ();
}

const char *tizsoundcloud::get_current_track_year ()
{
  return current_track_year_.empty () ? NULL : current_track_year_.c_str ();
}

const char *tizsoundcloud::get_current_track_permalink ()
{
  return current_track_permalink_.empty () ? NULL
                                           : current_track_permalink_.c_str ();
}

const char *tizsoundcloud::get_current_track_license ()
{
  return current_track_license_.empty () ? NULL
                                         : current_track_license_.c_str ();
}

const char *tizsoundcloud::get_current_track_likes ()
{
  return current_track_likes_.empty () ? NULL : current_track_likes_.c_str ();
}

const char *tizsoundcloud::get_current_track_user_avatar ()
{
  return current_track_user_avatar_.empty ()
             ? NULL
             : current_track_user_avatar_.c_str ();
}

void tizsoundcloud::clear_queue ()
{
  int rc = 0;
  try_catch_wrapper (py_sc_proxy_.attr ("clear_queue") ());
  (void)rc;
}

void tizsoundcloud::set_playback_mode (const playback_mode mode)
{
  int rc = 0;
  switch (mode)
    {
      case PlaybackModeNormal:
        {
          try_catch_wrapper (py_sc_proxy_.attr ("set_play_mode") ("NORMAL"));
        }
        break;
      case PlaybackModeShuffle:
        {
          try_catch_wrapper (py_sc_proxy_.attr ("set_play_mode") ("SHUFFLE"));
        }
        break;
      default:
        {
          assert (0);
        }
        break;
    };
  (void)rc;
}

void tizsoundcloud::get_current_track ()
{
  current_track_index_.clear ();
  current_queue_length_.clear ();
  current_user_.clear ();
  current_title_.clear ();

  const bp::tuple &queue_info = bp::extract< bp::tuple > (
      py_sc_proxy_.attr ("current_track_queue_index_and_queue_length") ());
  const int queue_index = bp::extract< int > (queue_info[0]);
  const int queue_length = bp::extract< int > (queue_info[1]);
  current_track_index_.assign (
      boost::lexical_cast< std::string > (queue_index));
  current_queue_length_.assign (
      boost::lexical_cast< std::string > (queue_length));

  const bp::tuple &info1 = bp::extract< bp::tuple > (
      py_sc_proxy_.attr ("current_track_title_and_user") ());
  current_title_ = bp::extract< std::string > (info1[0]);
  current_user_ = bp::extract< std::string > (info1[1]);

  int duration
      = bp::extract< int > (py_sc_proxy_.attr ("current_track_duration") ());

  int seconds = 0;
  current_duration_.clear ();
  if (duration)
    {
      duration /= 1000;
      seconds = duration % 60;
      int minutes = (duration - seconds) / 60;
      int hours = 0;
      if (minutes >= 60)
        {
          int total_minutes = minutes;
          minutes = total_minutes % 60;
          hours = (total_minutes - minutes) / 60;
        }

      if (hours > 0)
        {
          current_duration_.append (boost::lexical_cast< std::string > (hours));
          current_duration_.append ("h:");
        }

      if (minutes > 0)
        {
          current_duration_.append (
              boost::lexical_cast< std::string > (minutes));
          current_duration_.append ("m:");
        }
    }

  char seconds_str[6];
  sprintf (seconds_str, "%02i", seconds);
  current_duration_.append (seconds_str);
  current_duration_.append ("s");

  const int track_year
      = bp::extract< int > (py_sc_proxy_.attr ("current_track_year") ());
  current_track_year_.assign (boost::lexical_cast< std::string > (track_year));

  current_track_permalink_ = bp::extract< std::string > (
      py_sc_proxy_.attr ("current_track_permalink") ());

  current_track_license_ = bp::extract< std::string > (
      py_sc_proxy_.attr ("current_track_license") ());

  const int track_likes
      = bp::extract< int > (py_sc_proxy_.attr ("current_track_likes") ());
  current_track_likes_.assign (
      boost::lexical_cast< std::string > (track_likes));

  current_track_user_avatar_ = bp::extract< std::string > (
      py_sc_proxy_.attr ("current_track_user_avatar") ());
}

void tizsoundcloud::get_current_track_queue_index_and_length (int &queue_index,
                                                              int &queue_length)
{
  const bp::tuple &queue_info = bp::extract< bp::tuple > (
      py_sc_proxy_.attr ("current_track_queue_index_and_queue_length") ());
  queue_index = bp::extract< int > (queue_info[0]);
  queue_length = bp::extract< int > (queue_info[1]);
  current_queue_length_as_int_ = queue_length;
}
