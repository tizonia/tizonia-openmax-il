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
 * @file   tizyoutube.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple YouTube audio client library
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/lexical_cast.hpp>
#include <iostream>

#include "tizyoutube.hpp"

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

        bp::object ignored = exec (
            "import imp\n"
            "imp.find_module('pafy')\n",
            py_global);

        bp::object ignored2 = exec (
            "import imp\n"
            "imp.find_module('youtube_dl')\n",
            py_global);
        rc = 0;
      }
    catch (bp::error_already_set &e)
      {
        PyErr_PrintEx (0);
        std::cerr << std::string (
            "\nPython modules 'pafy and/or 'youtube-dl' not found."
            "\nPlease make sure these are installed correctly.\n");
      }
    catch (...)
      {
        std::cerr << std::string ("Unknown exception caught");
      }
    return rc;
  }

  void init_youtube (boost::python::object &py_main,
                     boost::python::object &py_global)
  {
    // Import the YouTube proxy module
    py_main = bp::import ("tizyoutubeproxy");

    // Retrieve the main module's namespace
    py_global = py_main.attr ("__dict__");
  }

  void start_youtube (boost::python::object &py_global,
                      boost::python::object &py_yt_proxy)
  {
    bp::object pyyoutubeproxy = py_global["tizyoutubeproxy"];
    py_yt_proxy = pyyoutubeproxy ();
  }
}

tizyoutube::tizyoutube ()
  : current_url_ (),
    current_stream_index_ (),
    current_queue_length_ (),
    current_stream_title_ (),
    current_stream_author_ (),
    current_stream_file_size_ (),
    current_stream_duration_ (),
    current_stream_bitrate_ (),
    current_stream_view_count_ (),
    current_stream_description_ (),
    current_stream_file_extension_ (),
    current_stream_video_id_ (),
    current_stream_published_ ()
{
}

tizyoutube::~tizyoutube ()
{
}

int tizyoutube::init ()
{
  int rc = 0;
  if (0 == (rc = check_deps ()))
    {
      try_catch_wrapper (init_youtube (py_main_, py_global_));
    }
  return rc;
}

int tizyoutube::start ()
{
  int rc = 0;
  try_catch_wrapper (start_youtube (py_global_, py_yt_proxy_));
  return rc;
}

void tizyoutube::stop ()
{
  int rc = 0;
  // try_catch_wrapper (py_yt_proxy_.attr ("logout")());
  (void)rc;
}

void tizyoutube::deinit ()
{
  // boost::python doesn't support Py_Finalize() yet!
}

int tizyoutube::play_audio_stream (const std::string &url_or_id)
{
  int rc = 0;
  try_catch_wrapper (
      py_yt_proxy_.attr ("enqueue_audio_stream") (bp::object (url_or_id)));
  return rc;
}

int tizyoutube::play_audio_playlist (const std::string &url_or_id)
{
  int rc = 0;
  try_catch_wrapper (
      py_yt_proxy_.attr ("enqueue_audio_playlist") (bp::object (url_or_id)));
  return rc;
}

int tizyoutube::play_audio_mix (const std::string &url_or_id)
{
  int rc = 0;
  try_catch_wrapper (
      py_yt_proxy_.attr ("enqueue_audio_mix") (bp::object (url_or_id)));
  return rc;
}

int tizyoutube::play_audio_search (const std::string &search)
{
  int rc = 0;
  try_catch_wrapper (
      py_yt_proxy_.attr ("enqueue_audio_search") (bp::object (search)));
  return rc;
}

int tizyoutube::play_audio_mix_search (const std::string &search)
{
  int rc = 0;
  try_catch_wrapper (
      py_yt_proxy_.attr ("enqueue_audio_mix_search") (bp::object (search)));
  return rc;
}

const char *tizyoutube::get_next_url (const bool a_remove_current_url)
{
  current_url_.clear ();
  try
    {
      if (a_remove_current_url)
        {
          py_yt_proxy_.attr ("remove_current_url") ();
        }
      const char *p_next_url
          = bp::extract< char const * > (py_yt_proxy_.attr ("next_url") ());
      current_url_.assign (p_next_url);
      if (!p_next_url || get_current_stream ())
        {
          current_url_.clear ();
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

const char *tizyoutube::get_prev_url (const bool a_remove_current_url)
{
  current_url_.clear ();
  try
    {
      if (a_remove_current_url)
        {
          py_yt_proxy_.attr ("remove_current_url") ();
        }
      const char *p_prev_url
          = bp::extract< char const * > (py_yt_proxy_.attr ("prev_url") ());
      current_url_.assign (p_prev_url);
      if (!p_prev_url || get_current_stream ())
        {
          current_url_.clear ();
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

void tizyoutube::clear_queue ()
{
  int rc = 0;
  try_catch_wrapper (py_yt_proxy_.attr ("clear_queue") ());
  (void)rc;
}

const char *tizyoutube::get_current_audio_stream_index ()
{
  return current_stream_index_.empty () ? NULL : current_stream_index_.c_str ();
}

const char *tizyoutube::get_current_queue_length ()
{
  return current_queue_length_.empty () ? NULL : current_queue_length_.c_str ();
}

const char *tizyoutube::get_current_queue_progress ()
{
  std::string output (get_current_audio_stream_index ());
  output.append (" of ");
  output.append (get_current_queue_length ());
  return output.c_str ();
}

void tizyoutube::set_playback_mode (const playback_mode mode)
{
  int rc = 0;
  switch (mode)
    {
      case PlaybackModeNormal:
        {
          try_catch_wrapper (py_yt_proxy_.attr ("set_play_mode") ("NORMAL"));
        }
        break;
      case PlaybackModeShuffle:
        {
          try_catch_wrapper (py_yt_proxy_.attr ("set_play_mode") ("SHUFFLE"));
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

const char *tizyoutube::get_current_audio_stream_title ()
{
  return current_stream_title_.empty () ? NULL : current_stream_title_.c_str ();
}

const char *tizyoutube::get_current_audio_stream_author ()
{
  return current_stream_author_.empty () ? NULL
                                         : current_stream_author_.c_str ();
}

const char *tizyoutube::get_current_audio_stream_file_size ()
{
  return current_stream_file_size_.empty ()
             ? NULL
             : current_stream_file_size_.c_str ();
}

const char *tizyoutube::get_current_audio_stream_duration ()
{
  return current_stream_duration_.empty () ? NULL
                                           : current_stream_duration_.c_str ();
}

const char *tizyoutube::get_current_audio_stream_bitrate ()
{
  return current_stream_bitrate_.empty () ? NULL
                                          : current_stream_bitrate_.c_str ();
}

const char *tizyoutube::get_current_audio_stream_view_count ()
{
  return current_stream_view_count_.empty ()
             ? NULL
             : current_stream_view_count_.c_str ();
}

const char *tizyoutube::get_current_audio_stream_description ()
{
  return current_stream_description_.empty ()
             ? NULL
             : current_stream_description_.c_str ();
}

const char *tizyoutube::get_current_audio_stream_file_extension ()
{
  return current_stream_file_extension_.empty ()
             ? NULL
             : current_stream_file_extension_.c_str ();
}

const char *tizyoutube::get_current_audio_stream_video_id ()
{
  return current_stream_video_id_.empty () ? NULL
                                           : current_stream_video_id_.c_str ();
}

const char *tizyoutube::get_current_audio_stream_published ()
{
  return current_stream_published_.empty ()
             ? NULL
             : current_stream_published_.c_str ();
}

int tizyoutube::get_current_stream ()
{
  int rc = 0;
  current_stream_index_.clear ();
  current_queue_length_.clear ();
  current_stream_title_.clear ();
  current_stream_author_.clear ();
  current_stream_file_size_.clear ();
  current_stream_duration_.clear ();
  current_stream_bitrate_.clear ();
  current_stream_view_count_.clear ();
  current_stream_description_.clear ();
  current_stream_file_extension_.clear ();
  current_stream_video_id_.clear ();
  current_stream_published_.clear ();

  const bp::tuple &queue_info = bp::extract< bp::tuple > (py_yt_proxy_.attr (
      "current_audio_stream_queue_index_and_queue_length") ());
  const int queue_index = bp::extract< int > (queue_info[0]);
  const int queue_length = bp::extract< int > (queue_info[1]);
  current_stream_index_.assign (
      boost::lexical_cast< std::string > (queue_index));
  current_queue_length_.assign (
      boost::lexical_cast< std::string > (queue_length));

  const char *p_title = bp::extract< char const * > (
      py_yt_proxy_.attr ("current_audio_stream_title") ());
  if (p_title)
    {
      current_stream_title_.assign (p_title);
    }

  const char *p_author = bp::extract< char const * > (
      py_yt_proxy_.attr ("current_audio_stream_author") ());
  if (p_author)
    {
      current_stream_author_.assign (p_author);
    }

  const int file_size = bp::extract< int > (
      py_yt_proxy_.attr ("current_audio_stream_file_size") ());
  current_stream_file_size_.assign (
      boost::lexical_cast< std::string > (file_size / (1024 * 1024)));
  current_stream_file_size_.append (" MiB");

  const char *p_duration = bp::extract< char const * > (
      py_yt_proxy_.attr ("current_audio_stream_duration") ());
  if (p_duration)
    {
      current_stream_duration_.assign (p_duration);
    }

  const char *p_bitrate = bp::extract< char const * > (
      py_yt_proxy_.attr ("current_audio_stream_bitrate") ());
  if (p_bitrate)
    {
      current_stream_bitrate_.assign (p_bitrate);
    }

  const int view_count = bp::extract< int > (
      py_yt_proxy_.attr ("current_audio_stream_view_count") ());
  current_stream_view_count_.assign (
      boost::lexical_cast< std::string > (view_count));

  const char *p_description = bp::extract< char const * > (
      py_yt_proxy_.attr ("current_audio_stream_description") ());
  if (p_description)
    {
      current_stream_description_.assign (p_description);
      current_stream_description_.erase (
          std::remove (current_stream_description_.begin (),
                       current_stream_description_.end (), '\n'),
          current_stream_description_.end ());
      current_stream_description_.erase (
          std::remove (current_stream_description_.begin (),
                       current_stream_description_.end (), '\r'),
          current_stream_description_.end ());
    }

  const char *p_file_extension = bp::extract< char const * > (
      py_yt_proxy_.attr ("current_audio_stream_file_extension") ());
  if (p_file_extension)
    {
      current_stream_file_extension_.assign (p_file_extension);
    }

  const char *p_video_id = bp::extract< char const * > (
      py_yt_proxy_.attr ("current_audio_stream_video_id") ());
  if (p_video_id)
    {
      current_stream_video_id_.assign (p_video_id);
    }

  const char *p_published = bp::extract< char const * > (
      py_yt_proxy_.attr ("current_audio_stream_published") ());
  if (p_published)
    {
      current_stream_published_.assign (p_published);
    }

  return rc;
}
