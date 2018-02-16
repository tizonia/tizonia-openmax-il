/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   tizplex.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Plex audio client library
 *
 *
 */

#ifndef TIZPLEX_HPP
#define TIZPLEX_HPP

#include <boost/python.hpp>

#include <string>

class tizplex
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
  tizplex (const std::string &base_url, const std::string &auth_token);
  ~tizplex ();

  int init ();
  int start ();
  void stop ();
  void deinit ();

  int play_audio_tracks (const std::string &tracks);
  int play_audio_artist (const std::string &artist);
  int play_audio_album (const std::string &album);
  int play_audio_playlist (const std::string &playlist);

  void set_playback_mode (const playback_mode mode);
  void clear_queue ();
  const char *get_current_audio_track_index ();
  const char *get_current_queue_length ();
  const char *get_current_queue_progress ();

  const char *get_next_url (const bool a_remove_current_url);
  const char *get_prev_url (const bool a_remove_current_url);

  const char *get_current_audio_track_title ();
  const char *get_current_audio_track_artist ();
  const char *get_current_audio_track_album ();
  const char *get_current_audio_track_year ();
  const char *get_current_audio_track_file_size ();
  const char *get_current_audio_track_duration ();
  const char *get_current_audio_track_bitrate ();
  const char *get_current_audio_track_codec ();
  const char *get_current_audio_track_album_art ();

private:
  int get_current_track ();

private:
  std::string base_url_;
  std::string auth_token_;
  std::string current_url_;
  std::string current_track_index_;
  std::string current_queue_length_;
  std::string current_track_title_;
  std::string current_track_artist_;
  std::string current_track_album_;
  std::string current_track_year_;
  std::string current_track_file_size_;
  std::string current_track_duration_;
  std::string current_track_bitrate_;
  std::string current_track_codec_;
  std::string current_track_album_art_;
  std::string current_queue_progress_;
  boost::python::object py_main_;
  boost::python::object py_global_;
  boost::python::object py_plex_proxy_;
};

#endif  // TIZPLEX_HPP
