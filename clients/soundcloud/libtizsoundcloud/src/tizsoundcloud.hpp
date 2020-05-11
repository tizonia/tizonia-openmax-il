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
 * @file   tizsoundcloud.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple SoundCloud client library
 *
 *
 */

#ifndef TIZSOUNDCLOUD_HPP
#define TIZSOUNDCLOUD_HPP

#include <boost/python.hpp>

#include <string>

class tizsoundcloud
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
  tizsoundcloud (const std::string &oauth_token);
  ~tizsoundcloud ();

  int init ();
  int start ();
  void stop ();
  void deinit ();

  int play_user_stream ();
  int play_user_likes ();
  int play_user_playlist (const std::string &playlist);
  int play_creator (const std::string &creator);
  int play_tracks (const std::string &tracks);
  int play_playlists (const std::string &playlists);
  int play_genres (const std::string &genres);
  int play_tags (const std::string &tags);

  void clear_queue ();
  void print_queue ();

  const char *get_current_audio_track_index ();
  const char *get_current_queue_length ();
  int get_current_queue_length_as_int ();
  const char *get_current_queue_progress ();
  void set_playback_mode (const playback_mode mode);

  const char *get_url (const int a_position);
  const char *get_next_url ();
  const char *get_prev_url ();
  const char *get_current_track_user ();
  const char *get_current_track_title ();
  const char *get_current_track_duration ();
  const char *get_current_track_year ();
  const char *get_current_track_permalink ();
  const char *get_current_track_license ();
  const char *get_current_track_likes ();
  const char *get_current_track_user_avatar ();

private:
  void get_current_track ();
  void get_current_track_queue_index_and_length(int &index, int &length);

private:
  std::string oauth_token_;
  std::string current_url_;
  std::string current_track_index_;
  std::string current_queue_length_;
  int current_queue_length_as_int_;
  std::string current_user_;
  std::string current_title_;
  std::string current_duration_;
  std::string current_track_year_;
  std::string current_track_permalink_;
  std::string current_track_license_;
  std::string current_track_likes_;
  std::string current_track_user_avatar_;
  std::string current_queue_progress_;
  boost::python::object py_main_;
  boost::python::object py_global_;
  boost::python::object py_sc_proxy_;
};

#endif  // TIZSOUNDCLOUD_HPP
