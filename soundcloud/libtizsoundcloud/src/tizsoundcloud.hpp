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
  tizsoundcloud (const std::string &user, const std::string &pass);
  ~tizsoundcloud ();

  int init ();
  int start ();
  void stop ();
  void deinit ();

  int play_user_stream ();
  int play_user_playlist (const std::string &playlist);
  int play_creator (const std::string &creator);
  int play_tracks (const std::string &tracks);
  int play_playlists (const std::string &playlists);
  int play_genres (const std::string &genres);
  int play_tags (const std::string &tags);

  void clear_queue ();
  void set_playback_mode (const playback_mode mode);

  const char * get_next_url ();
  const char * get_prev_url ();
  const char * get_current_track_user ();
  const char * get_current_track_title ();
  const char * get_current_track_duration ();
  const char * get_current_track_year ();
  const char * get_current_track_permalink ();
  const char * get_current_track_license ();
  const char * get_current_track_likes ();

private:
  int get_current_track ();

private:
  std::string user_;
  std::string pass_;
  std::string current_url_;
  std::string current_user_;
  std::string current_title_;
  std::string current_duration_;
  std::string current_track_year_;
  std::string current_track_permalink_;
  std::string current_track_license_;
  std::string current_track_likes_;
  boost::python::object py_main_;
  boost::python::object py_global_;
  boost::python::object py_gm_proxy_;
};

#endif  // TIZSOUNDCLOUD_HPP
