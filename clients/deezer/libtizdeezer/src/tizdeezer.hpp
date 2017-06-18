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
 * @file   tizdeezer.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Deezer client library
 *
 *
 */

#ifndef TIZDEEZER_HPP
#define TIZDEEZER_HPP

#include <boost/python.hpp>

#include <string>

class tizdeezer
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
  tizdeezer (const std::string &user);
  ~tizdeezer ();

  int init ();
  int start ();
  void stop ();
  void deinit ();

  int play_tracks (const std::string &tracks);
  int play_album (const std::string &album);
  int play_artist (const std::string &artist);

  void clear_queue ();
  void set_playback_mode (const playback_mode mode);

  int next_track ();
  int prev_track ();

  size_t get_mp3_data (unsigned char **app_data);
  const char * get_current_track_artist ();
  const char * get_current_track_title ();
  const char * get_current_track_album ();
  const char * get_current_track_duration ();
  const char * get_current_track_file_size_mb ();
  int get_current_track_file_size_bytes ();

private:
  int get_current_track ();

private:
  std::string user_;
  std::string current_track_;
  std::string current_artist_;
  std::string current_title_;
  std::string current_album_;
  std::string current_duration_;
  std::string current_file_size_mb_;
  int current_file_size_bytes_;
  boost::python::object py_main_;
  boost::python::object py_global_;
  boost::python::object py_dz_proxy_;
};

#endif  // TIZDEEZER_HPP
