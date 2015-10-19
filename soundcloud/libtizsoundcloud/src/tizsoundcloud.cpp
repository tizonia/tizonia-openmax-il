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
 * @file   tizsoundcloud.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple SoundCloud client library
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <boost/lexical_cast.hpp>

#include "tizsoundcloud.hpp"

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
  void init_soundcloud (boost::python::object &py_main,
                    boost::python::object &py_global)
  {
    Py_Initialize ();

    // Import the SoundCloud proxy module
    py_main = bp::import ("tizsoundcloudproxy");

    // Retrieve the main module's namespace
    py_global = py_main.attr ("__dict__");
  }

  void start_soundcloud (boost::python::object &py_global,
                     boost::python::object &py_gm_proxy,
                     const std::string &user, const std::string &pass)
  {
    bp::object pysoundcloudproxy = py_global["tizsoundcloudproxy"];
    py_gm_proxy
        = pysoundcloudproxy (user.c_str (), pass.c_str ());
  }
}

tizsoundcloud::tizsoundcloud (const std::string &user, const std::string &pass)
  : user_ (user), pass_ (pass)
{
}

tizsoundcloud::~tizsoundcloud ()
{
}

int tizsoundcloud::init ()
{
  int rc = 0;
  try_catch_wrapper (init_soundcloud (py_main_, py_global_));
  return rc;
}

int tizsoundcloud::start ()
{
  int rc = 0;
  try_catch_wrapper (
      start_soundcloud (py_global_, py_gm_proxy_, user_, pass_));
  return rc;
}

void tizsoundcloud::stop ()
{
  int rc = 0;
  // try_catch_wrapper (py_gm_proxy_.attr ("logout")());
  (void)rc;
}

void tizsoundcloud::deinit ()
{
  // boost::python doesn't support Py_Finalize() yet!
}

int tizsoundcloud::play_user_stream ()
{
  int rc = 0;
  try_catch_wrapper (py_gm_proxy_.attr ("enqueue_user_stream")());
  return rc;
}

int tizsoundcloud::play_user_playlist (const std::string &playlist)
{
  int rc = 0;
  try_catch_wrapper (py_gm_proxy_.attr ("enqueue_user_playlist")(bp::object (playlist)));
  return rc;
}

int tizsoundcloud::play_creator (const std::string &creator)
{
  int rc = 0;
  try_catch_wrapper (py_gm_proxy_.attr ("enqueue_creator")(bp::object (creator)));
  return rc;
}

int tizsoundcloud::play_tracks (const std::string &tracks)
{
  int rc = 0;
  try_catch_wrapper (py_gm_proxy_.attr ("enqueue_tracks")(bp::object (tracks)));
  return rc;
}

int tizsoundcloud::play_playlists (const std::string &playlists)
{
  int rc = 0;
  try_catch_wrapper (py_gm_proxy_.attr ("enqueue_playlists")(bp::object (playlists)));
  return rc;
}

int tizsoundcloud::play_genres (const std::string &genres)
{
  int rc = 0;
  try_catch_wrapper (py_gm_proxy_.attr ("enqueue_genres")(bp::object (genres)));
  return rc;
}

int tizsoundcloud::play_tags (const std::string &tags)
{
  int rc = 0;
  try_catch_wrapper (py_gm_proxy_.attr ("enqueue_tags")(bp::object (tags)));
  return rc;
}

const char *tizsoundcloud::get_next_url ()
{
  current_url_.clear ();
  try
    {
      const char *p_next_url
          = bp::extract< char const * >(py_gm_proxy_.attr ("next_url")());
      if (p_next_url && !get_current_track ())
        {
          current_url_.assign (p_next_url);
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
      const char *p_prev_url
          = bp::extract< char const * >(py_gm_proxy_.attr ("prev_url")());
      if (p_prev_url && !get_current_track ())
        {
          current_url_.assign (p_prev_url);
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
  return current_track_year_.empty ()
             ? NULL
             : current_track_year_.c_str ();
}

const char *tizsoundcloud::get_current_track_permalink ()
{
  return current_track_permalink_.empty ()
             ? NULL
             : current_track_permalink_.c_str ();
}

const char *tizsoundcloud::get_current_track_license ()
{
  return current_track_license_.empty ()
             ? NULL
             : current_track_license_.c_str ();
}

void tizsoundcloud::clear_queue ()
{
  int rc = 0;
  try_catch_wrapper (py_gm_proxy_.attr ("clear_queue")());
  (void)rc;
}

void tizsoundcloud::set_playback_mode (const playback_mode mode)
{
  int rc = 0;
  switch(mode)
    {
    case PlaybackModeNormal:
      {
        try_catch_wrapper (py_gm_proxy_.attr ("set_play_mode")("NORMAL"));
      }
      break;
    case PlaybackModeShuffle:
      {
        try_catch_wrapper (py_gm_proxy_.attr ("set_play_mode")("SHUFFLE"));
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

int tizsoundcloud::get_current_track ()
{
  int rc = 1;
  current_user_.clear ();
  current_title_.clear ();

  const bp::tuple &info1 = bp::extract< bp::tuple >(
      py_gm_proxy_.attr ("current_track_title_and_user")());
  const char *p_user = bp::extract< char const * >(info1[0]);
  const char *p_title = bp::extract< char const * >(info1[1]);

  if (p_user)
    {
      current_user_.assign (p_user);
    }
  if (p_title)
    {
      current_title_.assign (p_title);
    }

  int duration
      = bp::extract< int >(py_gm_proxy_.attr ("current_track_duration")());

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
          current_duration_.append (boost::lexical_cast< std::string >(hours));
          current_duration_.append ("h:");
        }

      if (minutes > 0)
        {
          current_duration_.append (
              boost::lexical_cast< std::string >(minutes));
          current_duration_.append ("m:");
        }
    }

  char seconds_str[3];
  sprintf (seconds_str, "%02i", seconds);
  current_duration_.append (seconds_str);
  current_duration_.append ("s");

  const int track_year = bp::extract< int >(py_gm_proxy_.attr ("current_track_year")());
  current_track_year_.assign (boost::lexical_cast< std::string >(track_year));

  const char *p_track_permalink = bp::extract< char const * >(
      py_gm_proxy_.attr ("current_track_permalink")());
  if (p_track_permalink)
    {
      current_track_permalink_.assign (p_track_permalink);
    }

  const char *p_track_license = bp::extract< char const * >(
      py_gm_proxy_.attr ("current_track_license")());
  if (p_track_license)
    {
      current_track_license_.assign (p_track_license);
    }

//   if (p_user || p_title)
//     {
      rc = 0;
//     }

  return rc;
}
