/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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


#include <iostream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/join.hpp>

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
        // Import Tizonia YouTube proxy module
        bp::object py_main = bp::import ("__main__");

        // Retrieve the main module's namespace
        bp::object py_global = py_main.attr ("__dict__");

        // Check the existence of the 'pafy' module
        bp::object ignored = exec (
            "import importlib\n"
            "spec = importlib.util.find_spec('pafy')\n"
            "if not spec:\n raise ValueError\n",
            py_global);

        // Check the existence of the 'youtube_dl' module
        bp::object ignored2 = exec (
            "import importlib\n"
            "spec = importlib.util.find_spec('youtube_dl')\n"
            "if not spec:\n raise ValueError\n",
            py_global);

        // Check the existence of the 'joblib' module
        bp::object ignored3 = exec (
            "import importlib\n"
            "spec = importlib.util.find_spec('joblib')\n"
            "if not spec:\n raise ValueError\n",
            py_global);

        // Check the existence of the 'fuzzywuzzy' module
        bp::object ignored4 = exec (
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
            "\nPython modules 'pafy', 'youtube-dl', 'joblib' or 'fuzzywuzzy' not found."
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
                      boost::python::object &py_yt_proxy,
                      const std::string &api_key)
  {
    bp::object pyyoutubeproxy = py_global["tizyoutubeproxy"];
    py_yt_proxy = pyyoutubeproxy (api_key.c_str());
  }
}

tizyoutube::tizyoutube (const std::string &api_key)
  : api_key_ (api_key),
    current_url_ (),
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
    current_stream_published_ (),
    current_queue_progress_ ()
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
  try_catch_wrapper (start_youtube (py_global_, py_yt_proxy_, api_key_));
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

int tizyoutube::play_audio_channel_uploads (const std::string &channel)
{
  int rc = 0;
  try_catch_wrapper (
      py_yt_proxy_.attr ("enqueue_audio_channel_uploads") (bp::object (channel)));
  return rc;
}

int tizyoutube::play_audio_channel_playlist (
    const std::string &channel_and_playlist)
{
  int rc = 0;
  std::string ch_and_pl_trim = channel_and_playlist;
  std::vector< std::string > strs;
  boost::algorithm::trim_all (ch_and_pl_trim);
  boost::split (strs, ch_and_pl_trim, boost::is_any_of (" "));
  if (strs.size () <= 1)
    {
      rc = 1;
    }
  else
    {
      std::string channel = strs[0];
      strs.erase (strs.begin ());
      std::string playlist = boost::algorithm::join (strs, " ");
      try_catch_wrapper (py_yt_proxy_.attr ("enqueue_audio_channel_playlist") (
          bp::object (channel), bp::object (playlist)));
    }
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
      current_url_ = bp::extract< std::string > (py_yt_proxy_.attr ("next_url") ());
      get_current_stream ();
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
      current_url_ = bp::extract< std::string > (py_yt_proxy_.attr ("prev_url") ());
      get_current_stream ();
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
  current_queue_progress_.assign (get_current_audio_stream_index ());
  current_queue_progress_.append (" of ");
  current_queue_progress_.append (get_current_queue_length ());
  return current_queue_progress_.c_str ();
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

void tizyoutube::get_current_stream ()
{
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

  current_stream_title_ = bp::extract< std::string > (
      py_yt_proxy_.attr ("current_audio_stream_title") ());

  current_stream_author_ = bp::extract< std::string > (
      py_yt_proxy_.attr ("current_audio_stream_author") ());

  const int file_size = bp::extract< int > (
      py_yt_proxy_.attr ("current_audio_stream_file_size") ());
  current_stream_file_size_.assign (
      boost::lexical_cast< std::string > (file_size / (1024 * 1024)));
  current_stream_file_size_.append (" MiB");

  std::string duration = bp::extract< std::string > (
      py_yt_proxy_.attr ("current_audio_stream_duration") ());
  if (duration.length())
    {
      std::string value = duration;
      std::vector< std::string > strs;
      boost::split (strs, value, boost::is_any_of (":"));
      std::reverse(strs.begin(), strs.end());
      size_t num_non_empty = 0;
      for (size_t i = 0; i < strs.size (); ++i)
        {
          if (0 == i)
            {
              ++num_non_empty;
              strs[i].append ("s");
            }
          if (1 == i)
            {
              ++num_non_empty;
              strs[i].append ("m");
            }
          if (2 == i)
            {
              if (0 == boost::lexical_cast< unsigned long > (strs[i]))
                {
                  strs[i].clear ();
                }
              else
                {
                  ++num_non_empty;
                  strs[i].append ("h");
                }
            }
        }

      for (size_t i = 0; i < num_non_empty; ++i)
        {
          current_stream_duration_ =  strs[i] + current_stream_duration_;
          if ((num_non_empty - 1) != i)
            {
              current_stream_duration_ = ":" + current_stream_duration_;
            }
        }
    }

  current_stream_bitrate_ = bp::extract< std::string > (
      py_yt_proxy_.attr ("current_audio_stream_bitrate") ());

  const int view_count = bp::extract< int > (
      py_yt_proxy_.attr ("current_audio_stream_view_count") ());
  current_stream_view_count_.assign (
      boost::lexical_cast< std::string > (view_count));

  std::string description = bp::extract< std::string > (
      py_yt_proxy_.attr ("current_audio_stream_description") ());
  if (description.length())
    {
      current_stream_description_ = description;
      current_stream_description_.erase (
          std::remove (current_stream_description_.begin (),
                       current_stream_description_.end (), '\n'),
          current_stream_description_.end ());
      current_stream_description_.erase (
          std::remove (current_stream_description_.begin (),
                       current_stream_description_.end (), '\r'),
          current_stream_description_.end ());
    }

  current_stream_file_extension_ = bp::extract< std::string > (
      py_yt_proxy_.attr ("current_audio_stream_file_extension") ());

  current_stream_video_id_ = bp::extract< std::string > (
      py_yt_proxy_.attr ("current_audio_stream_video_id") ());

  current_stream_published_ = bp::extract< std::string > (
      py_yt_proxy_.attr ("current_audio_stream_published") ());

}
