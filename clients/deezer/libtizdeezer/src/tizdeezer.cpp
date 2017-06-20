/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   tizdeezer.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Deezer client library
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/lexical_cast.hpp>
#include <iostream>

#include "tizdeezer.hpp"

namespace bp = boost::python;

/* This macro assumes the existence of an "int rc" local variable */
#define try_catch_wrapper(expr)                                  \
  do                                                             \
    {                                                            \
      try                                                        \
        {                                                        \
          (expr);                                                \
        }                                                        \
      catch (bp::error_already_set & e)                          \
        {                                                        \
          PyErr_PrintEx (0);                                     \
          rc = 1;                                                \
        }                                                        \
      catch (const std::exception &e)                            \
        {                                                        \
          std::cerr << e.what ();                                \
        }                                                        \
      catch (...)                                                \
        {                                                        \
          std::cerr << std::string ("Unknown exception caught"); \
        }                                                        \
    }                                                            \
  while (0)

namespace
{
  void init_deezer (boost::python::object &py_main,
                    boost::python::object &py_global)
  {
    Py_Initialize ();

    // Import the Deezer proxy module
    py_main = bp::import ("tizdeezerproxy");

    // Retrieve the main module's namespace
    py_global = py_main.attr ("__dict__");
  }

  void start_deezer (boost::python::object &py_global,
                     boost::python::object &py_dz_proxy,
                     const std::string &user)
  {
    bp::object pydeezerproxy = py_global["tizdeezerproxy"];
    py_dz_proxy = pydeezerproxy (user.c_str ());
  }
}

tizdeezer::tizdeezer (const std::string &user)
  : user_ (user),
    current_track_ (),
    current_artist_ (),
    current_title_ (),
    current_album_ (),
    current_duration_ (),
    current_file_size_mb_ (),
    current_file_size_bytes_(0)

{
}

tizdeezer::~tizdeezer ()
{
}

int tizdeezer::init ()
{
  int rc = 0;
  try_catch_wrapper (init_deezer (py_main_, py_global_));
  return rc;
}

int tizdeezer::start ()
{
  int rc = 0;
  printf("user_ %s\n", user_.c_str());
  try_catch_wrapper (start_deezer (py_global_, py_dz_proxy_, user_));
  return rc;
}

void tizdeezer::stop ()
{
  int rc = 0;
  // try_catch_wrapper (py_dz_proxy_.attr ("logout")());
  (void)rc;
}

void tizdeezer::deinit ()
{
  // boost::python doesn't support Py_Finalize() yet!
}

int tizdeezer::play_tracks (const std::string &tracks)
{
  int rc = 0;
  try_catch_wrapper (
      py_dz_proxy_.attr ("enqueue_tracks") (bp::object (tracks)));
  return rc;
}

int tizdeezer::play_album (const std::string &album)
{
  int rc = 0;
  try_catch_wrapper (py_dz_proxy_.attr ("enqueue_album") (bp::object (album)));
  return rc;
}

int tizdeezer::play_artist (const std::string &artist)
{
  int rc = 0;
  try_catch_wrapper (
      py_dz_proxy_.attr ("enqueue_artist") (bp::object (artist)));
  return rc;
}

int tizdeezer::play_mix (const std::string &mix)
{
  int rc = 0;
  try_catch_wrapper (
      py_dz_proxy_.attr ("enqueue_mix") (bp::object (mix)));
  return rc;
}

int tizdeezer::play_playlist (const std::string &playlist)
{
  int rc = 0;
  try_catch_wrapper (
      py_dz_proxy_.attr ("enqueue_playlist") (bp::object (playlist)));
  return rc;
}

int tizdeezer::play_user_flow ()
{
  int rc = 0;
  try_catch_wrapper (py_dz_proxy_.attr ("enqueue_user_flow") ());
  return rc;
}

int tizdeezer::next_track ()
{
  current_track_.clear ();
  try
    {
      const char *p_next_track
          = bp::extract< char const * > (py_dz_proxy_.attr ("next_track") ());
      if (p_next_track)
        {
          current_track_.assign (p_next_track);
        }
      if (get_current_track ())
        {
          current_track_.clear ();
        }
    }
  catch (bp::error_already_set &e)
    {
      PyErr_PrintEx (0);
    }
  catch (...)
    {
    }
  return current_track_.empty () ? EXIT_FAILURE : EXIT_SUCCESS;
}

int tizdeezer::prev_track ()
{
  current_track_.clear ();
  try
    {
      const char *p_prev_track
          = bp::extract< char const * > (py_dz_proxy_.attr ("prev_track") ());
      if (p_prev_track)
        {
          current_track_.assign (p_prev_track);
        }
      if (get_current_track ())
        {
          current_track_.clear ();
        }
    }
  catch (bp::error_already_set &e)
    {
      PyErr_PrintEx (0);
    }
  catch (...)
    {
    }
  return current_track_.empty () ? EXIT_FAILURE : EXIT_SUCCESS;
}

size_t tizdeezer::get_mp3_data (unsigned char **app_data)
{
  int size = 0;
  if (app_data)
    {
      try
        {
          const bp::tuple &info1 = bp::extract< bp::tuple > (
              py_dz_proxy_.attr ("stream_current_track") ());
          const char *p_data = bp::extract< char const * > (info1[0]);
          *app_data = (unsigned char *)p_data;
          size = bp::extract< int > (info1[1]);
        }
      catch (bp::error_already_set &e)
        {
          PyErr_PrintEx (0);
        }
      catch (...)
        {
        }
    }
  return (size_t)size;
}

const char *tizdeezer::get_current_track_artist ()
{
  return current_artist_.empty () ? NULL : current_artist_.c_str ();
}

const char *tizdeezer::get_current_track_title ()
{
  return current_title_.empty () ? NULL : current_title_.c_str ();
}

const char *tizdeezer::get_current_track_album ()
{
  return current_album_.empty () ? NULL : current_album_.c_str ();
}

const char *tizdeezer::get_current_track_duration ()
{
  return current_duration_.empty () ? NULL : current_duration_.c_str ();
}

const char *tizdeezer::get_current_track_file_size_mb ()
{
  return current_file_size_mb_.empty () ? NULL : current_file_size_mb_.c_str ();
}

int tizdeezer::get_current_track_file_size_bytes ()
{
  return current_file_size_bytes_;
}

void tizdeezer::clear_queue ()
{
  int rc = 0;
  try_catch_wrapper (py_dz_proxy_.attr ("clear_queue") ());
  (void)rc;
}

void tizdeezer::set_playback_mode (const playback_mode mode)
{
  int rc = 0;
  switch (mode)
    {
      case PlaybackModeNormal:
        {
          try_catch_wrapper (py_dz_proxy_.attr ("set_play_mode") ("NORMAL"));
        }
        break;
      case PlaybackModeShuffle:
        {
          try_catch_wrapper (py_dz_proxy_.attr ("set_play_mode") ("SHUFFLE"));
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

int tizdeezer::get_current_track ()
{
  int rc = EXIT_FAILURE;
  current_title_.clear ();
  current_artist_.clear ();

  const bp::tuple &info1 = bp::extract< bp::tuple > (
      py_dz_proxy_.attr ("current_track_title_and_artist") ());
  const char *p_title = bp::extract< char const * > (info1[0]);
  const char *p_artist = bp::extract< char const * > (info1[1]);

  if (p_artist)
    {
      current_artist_.assign (p_artist);
    }
  if (p_title)
    {
      current_title_.assign (p_title);
    }

  const bp::tuple &info2 = bp::extract< bp::tuple > (
      py_dz_proxy_.attr ("current_track_album_and_duration") ());

  const char *p_album = bp::extract< char const * > (info2[0]);
  int duration = bp::extract< int > (info2[1]);
  if (p_album)
    {
      current_album_.assign (p_album);
    }

  int seconds = 0;
  current_duration_.clear ();
  if (duration)
    {
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

  char seconds_str[3];
  sprintf (seconds_str, "%02i", seconds);
  current_duration_.append (seconds_str);
  current_duration_.append ("s");

  const int file_size
      = bp::extract< int > (py_dz_proxy_.attr ("current_track_file_size") ());
  current_file_size_mb_.assign (
      boost::lexical_cast< std::string > (file_size / (1024 * 1024)));
  current_file_size_mb_.append (" MiB");
  current_file_size_bytes_ = file_size;

  if (p_artist || p_title)
    {
      rc = EXIT_SUCCESS;
    }

  return rc;
}
