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
 * @file   tizyoutube.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple YouTube audio client library
 *
 *
 */

#ifndef TIZYOUTUBE_HPP
#define TIZYOUTUBE_HPP

#include <boost/python.hpp>

#include <string>

class tizyoutube
{
public:
  /**
   * Various playback modes that control the playback queue.
   */
  enum playback_mode
  {
    PlaybackModeNormal,
    PlaybackModeShuffle,
    PlaybackModeMax
  };

public:
  tizyoutube (const std::string &api_key);
  ~tizyoutube ();

  int init ();
  int start ();
  void stop ();
  void deinit ();

  int play_audio_stream (const std::string &url_or_id);
  int play_audio_playlist (const std::string &url_or_id);
  int play_audio_mix (const std::string &url_or_id);
  int play_audio_search (const std::string &search);
  int play_audio_mix_search (const std::string &search);
  int play_audio_channel_uploads (const std::string &channel);
  int play_audio_channel_playlist (const std::string &channel_and_playlist);

  void set_playback_mode (const playback_mode mode);
  void clear_queue ();
  void print_queue ();

  const char *get_current_audio_stream_index ();
  const char *get_current_queue_length ();
  int get_current_queue_length_as_int ();
  const char *get_current_queue_progress ();

  const char *get_url (const int a_position);
  const char *get_next_url (const bool a_remove_current_url);
  const char *get_prev_url (const bool a_remove_current_url);

  const char *get_current_audio_stream_title ();
  const char *get_current_audio_stream_author ();
  const char *get_current_audio_stream_file_size ();
  const char *get_current_audio_stream_duration ();
  const char *get_current_audio_stream_bitrate ();
  const char *get_current_audio_stream_view_count ();
  const char *get_current_audio_stream_description ();
  const char *get_current_audio_stream_file_extension ();
  const char *get_current_audio_stream_video_id ();
  const char *get_current_audio_stream_published ();

private:
  void get_current_stream ();
  void get_current_stream_queue_index_and_length (int &index, int &length);

private:
  std::string api_key_;
  std::string current_url_;
  std::string current_stream_index_;
  std::string current_queue_length_;
  int current_queue_length_as_int_;
  std::string current_stream_title_;
  std::string current_stream_author_;
  std::string current_stream_file_size_;
  std::string current_stream_duration_;
  std::string current_stream_bitrate_;
  std::string current_stream_view_count_;
  std::string current_stream_description_;
  std::string current_stream_file_extension_;
  std::string current_stream_video_id_;
  std::string current_stream_published_;
  std::string current_queue_progress_;
  boost::python::object py_main_;
  boost::python::object py_global_;
  boost::python::object py_yt_proxy_;
};

#endif  // TIZYOUTUBE_HPP
