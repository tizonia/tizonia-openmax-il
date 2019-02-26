/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   tizspotify.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Spotify Web client library
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/lexical_cast.hpp>
#include <iostream>

#include "tizspotify.hpp"

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
        // Import the Tizonia Spotify proxy module
        bp::object py_main = bp::import ("__main__");

        // Retrieve the main module's namespace
        bp::object py_global = py_main.attr ("__dict__");

        // Check the existence of the 'spotipy' module
        bp::object ignored = exec (
            "import imp\n"
            "imp.find_module('spotipy')\n",
            py_global);

        // Check the existence of the 'fuzzywuzzy' module
        bp::object ignored2 = exec (
            "import imp\n"
            "imp.find_module('fuzzywuzzy')\n",
            py_global);

        rc = 0;
      }
    catch (bp::error_already_set &e)
      {
        PyErr_PrintEx (0);
        std::cerr << std::string (
            "\nPython modules 'spotifyapi' or 'fuzzywuzzy' not found."
            "\nPlease make sure these are installed correctly.\n");
      }
    catch (...)
      {
        std::cerr << std::string ("Unknown exception caught");
      }
    return rc;
  }

  void init_spotify (boost::python::object &py_main,
                     boost::python::object &py_global)
  {
    // Import the Spotify proxy module
    py_main = bp::import ("tizspotifyproxy");

    // Retrieve the main module's namespace
    py_global = py_main.attr ("__dict__");
  }

  void start_spotify (boost::python::object &py_global,
                      boost::python::object &py_spotify_proxy)
  {
    bp::object pyspotifyproxy = py_global["tizspotifyproxy"];
    py_spotify_proxy = pyspotifyproxy ();
  }
}  // namespace

tizspotify::tizspotify ()
  : current_uri_ (),
    current_track_index_ (),
    current_queue_length_ (),
    current_queue_length_as_int_ (0),
    current_track_title_ (),
    current_track_artist_ (),
    current_track_album_ (),
    current_track_release_date_ (),
    current_track_duration_ (),
    current_track_album_art_ (),
    current_track_uri_ (),
    current_track_artist_uri_ (),
    current_track_album_uri_ (),
    current_queue_progress_ ()
{
}

tizspotify::~tizspotify ()
{
}

int tizspotify::init ()
{
  int rc = 0;
  if (0 == (rc = check_deps ()))
    {
      try_catch_wrapper (init_spotify (py_main_, py_global_));
    }
  return rc;
}

int tizspotify::start ()
{
  int rc = 0;
  try_catch_wrapper (start_spotify (py_global_, py_spotify_proxy_));
  return rc;
}

void tizspotify::stop ()
{
  int rc = 0;
  // try_catch_wrapper (py_spotify_proxy_.attr ("logout")());
  (void)rc;
}

void tizspotify::deinit ()
{
  // boost::python doesn't support Py_Finalize() yet!
}

int tizspotify::play_tracks (const std::string &tracks)
{
  int rc = 0;
  try_catch_wrapper (
      py_spotify_proxy_.attr ("enqueue_tracks") (bp::object (tracks)));
  return rc;
}

int tizspotify::play_artist (const std::string &artist)
{
  int rc = 0;
  try_catch_wrapper (
      py_spotify_proxy_.attr ("enqueue_artist") (bp::object (artist)));
  return rc;
}

int tizspotify::play_album (const std::string &album)
{
  int rc = 0;
  try_catch_wrapper (
      py_spotify_proxy_.attr ("enqueue_album") (bp::object (album)));
  return rc;
}

int tizspotify::play_playlist (const std::string &playlist,
                               const std::string &owner)
{
  int rc = 0;
  try_catch_wrapper (py_spotify_proxy_.attr ("enqueue_playlist") (
      bp::object (playlist), bp::object (owner)));
  return rc;
}

int tizspotify::play_track_id (const std::string &track_id)
{
  int rc = 0;
  try_catch_wrapper (
      py_spotify_proxy_.attr ("enqueue_track_id") (bp::object (track_id)));
  return rc;
}

int tizspotify::play_artist_id (const std::string &artist_id)
{
  int rc = 0;
  try_catch_wrapper (
      py_spotify_proxy_.attr ("enqueue_artist_id") (bp::object (artist_id)));
  return rc;
}

int tizspotify::play_album_id (const std::string &album_id)
{
  int rc = 0;
  try_catch_wrapper (
      py_spotify_proxy_.attr ("enqueue_album_id") (bp::object (album_id)));
  return rc;
}

int tizspotify::play_playlist_id (const std::string &playlist_id,
                                  const std::string &owner)
{
  int rc = 0;
  try_catch_wrapper (py_spotify_proxy_.attr ("enqueue_playlist_id") (
      bp::object (playlist_id), bp::object (owner)));
  return rc;
}

int tizspotify::play_related_artists (const std::string &artist)
{
  int rc = 0;
  try_catch_wrapper (
      py_spotify_proxy_.attr ("enqueue_related_artists") (bp::object (artist)));
  return rc;
}

int tizspotify::play_featured_playlist (const std::string &playlist)
{
  int rc = 0;
  try_catch_wrapper (py_spotify_proxy_.attr ("enqueue_featured_playlist") (
      bp::object (playlist)));
  return rc;
}

int tizspotify::play_new_releases (const std::string &album)
{
  int rc = 0;
  try_catch_wrapper (py_spotify_proxy_.attr ("enqueue_new_releases") (
      bp::object (album)));
  return rc;
}

int tizspotify::play_recommendations_by_track_id (const std::string &track_id)
{
  int rc = 0;
  try_catch_wrapper (py_spotify_proxy_.attr ("enqueue_recommendations_by_track_id") (
      bp::object (track_id)));
  return rc;
}

int tizspotify::play_recommendations_by_artist_id (const std::string &artist_id)
{
  int rc = 0;
  try_catch_wrapper (py_spotify_proxy_.attr ("enqueue_recommendations_by_artist_id") (
      bp::object (artist_id)));
  return rc;
}

int tizspotify::play_recommendations_by_genre (const std::string &genre)
{
  int rc = 0;
  try_catch_wrapper (py_spotify_proxy_.attr ("enqueue_recommendations_by_genre") (
      bp::object (genre)));
  return rc;
}

const char *tizspotify::get_next_uri (const bool a_remove_current_uri)
{
  current_uri_.clear ();
  try
    {
      if (a_remove_current_uri)
        {
          py_spotify_proxy_.attr ("remove_current_uri") ();
        }
      current_uri_ = bp::extract< std::string > (
          py_spotify_proxy_.attr ("next_uri") ());
      get_current_track ();
    }
  catch (bp::error_already_set &e)
    {
      PyErr_PrintEx (0);
    }
  catch (...)
    {
    }
  return current_uri_.empty () ? NULL : current_uri_.c_str ();
}

const char *tizspotify::get_prev_uri (const bool a_remove_current_uri)
{
  current_uri_.clear ();
  try
    {
      if (a_remove_current_uri)
        {
          py_spotify_proxy_.attr ("remove_current_uri") ();
        }
      current_uri_ = bp::extract< std::string > (
          py_spotify_proxy_.attr ("prev_uri") ());
      get_current_track ();
    }
  catch (bp::error_already_set &e)
    {
      PyErr_PrintEx (0);
    }
  catch (...)
    {
    }
  return current_uri_.empty () ? NULL : current_uri_.c_str ();
}

void tizspotify::clear_queue ()
{
  int rc = 0;
  try_catch_wrapper (py_spotify_proxy_.attr ("clear_queue") ());
  (void)rc;
}

const char *tizspotify::get_current_track_index ()
{
  return current_track_index_.empty () ? NULL : current_track_index_.c_str ();
}

const char *tizspotify::get_current_queue_length ()
{
  return current_queue_length_.empty () ? NULL : current_queue_length_.c_str ();
}

int tizspotify::get_current_queue_length_as_int ()
{
  return current_queue_length_as_int_;
}

const char *tizspotify::get_current_queue_progress ()
{
  current_queue_progress_.assign (get_current_track_index ());
  current_queue_progress_.append (" of ");
  current_queue_progress_.append (get_current_queue_length ());
  return current_queue_progress_.c_str ();
}

void tizspotify::set_playback_mode (const playback_mode mode)
{
  int rc = 0;
  switch (mode)
    {
      case PlaybackModeNormal:
        {
          try_catch_wrapper (
              py_spotify_proxy_.attr ("set_play_mode") ("NORMAL"));
        }
        break;
      case PlaybackModeShuffle:
        {
          try_catch_wrapper (
              py_spotify_proxy_.attr ("set_play_mode") ("SHUFFLE"));
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

const char *tizspotify::get_current_track_title ()
{
  return current_track_title_.empty () ? NULL : current_track_title_.c_str ();
}

const char *tizspotify::get_current_track_artist ()
{
  return current_track_artist_.empty () ? NULL : current_track_artist_.c_str ();
}

const char *tizspotify::get_current_track_album ()
{
  return current_track_album_.empty () ? NULL : current_track_album_.c_str ();
}

const char *tizspotify::get_current_track_release_date ()
{
  return current_track_release_date_.empty ()
             ? NULL
             : current_track_release_date_.c_str ();
}

const char *tizspotify::get_current_track_duration ()
{
  return current_track_duration_.empty () ? NULL
                                          : current_track_duration_.c_str ();
}

const char *tizspotify::get_current_track_album_art ()
{
  return current_track_album_art_.empty () ? NULL
                                           : current_track_album_art_.c_str ();
}

const char *tizspotify::get_current_track_uri ()
{
  return current_track_uri_.empty () ? NULL : current_track_uri_.c_str ();
}

const char *tizspotify::get_current_track_artist_uri ()
{
  return current_track_artist_uri_.empty () ? NULL : current_track_artist_uri_.c_str ();
}

const char *tizspotify::get_current_track_album_uri ()
{
  return current_track_album_uri_.empty () ? NULL : current_track_album_uri_.c_str ();
}

void tizspotify::get_current_track ()
{
  current_track_index_.clear ();
  current_queue_length_.clear ();
  current_track_title_.clear ();
  current_track_artist_.clear ();
  current_track_album_.clear ();
  current_track_release_date_.clear ();
  current_track_duration_.clear ();
  current_track_album_art_.clear ();
  current_track_uri_.clear ();
  current_track_artist_uri_.clear ();
  current_track_album_uri_.clear ();

  const bp::tuple &queue_info = bp::extract< bp::tuple > (
      py_spotify_proxy_.attr ("current_track_queue_index_and_queue_length") ());
  const int queue_index = bp::extract< int > (queue_info[0]);
  const int queue_length = bp::extract< int > (queue_info[1]);
  current_track_index_.assign (
      boost::lexical_cast< std::string > (queue_index));
  current_queue_length_as_int_ = queue_length;
  current_queue_length_.assign (
      boost::lexical_cast< std::string > (queue_length));
  current_track_title_ = bp::extract< std::string > (
      py_spotify_proxy_.attr ("current_track_title") ());

  current_track_artist_ = bp::extract< std::string > (
      py_spotify_proxy_.attr ("current_track_artist") ());

  current_track_album_ = bp::extract< std::string > (
      py_spotify_proxy_.attr ("current_track_album") ());

  current_track_release_date_ = bp::extract< std::string > (
      py_spotify_proxy_.attr ("current_track_release_date") ());

  const int duration = bp::extract< int > (
      py_spotify_proxy_.attr ("current_track_duration") ());
  int seconds = duration % 60;
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
      current_track_duration_.assign (
          boost::lexical_cast< std::string > (hours));
      current_track_duration_.append ("h:");
    }

  if (minutes > 0)
    {
      current_track_duration_.append (
          boost::lexical_cast< std::string > (minutes));
      current_track_duration_.append ("m:");
    }

  char seconds_str[10];
  if (0 == minutes && 0 == hours)
    {
      sprintf (seconds_str, "%01i", seconds);
    }
  else
    {
      sprintf (seconds_str, "%02i", seconds);
    }
  current_track_duration_.append (seconds_str);
  current_track_duration_.append ("s");

  current_track_album_art_ = bp::extract< std::string > (
      py_spotify_proxy_.attr ("current_track_album_art") ());

  current_track_uri_ = bp::extract< std::string > (
      py_spotify_proxy_.attr ("current_track_uri") ());

  current_track_artist_uri_ = bp::extract< std::string > (
      py_spotify_proxy_.attr ("current_track_artist_uri") ());

  current_track_album_uri_ = bp::extract< std::string > (
      py_spotify_proxy_.attr ("current_track_album_uri") ());
}
