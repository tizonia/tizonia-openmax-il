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
 * @file   tizspotify.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Spotify Web client library
 *
 *
 */

#ifndef TIZSPOTIFY_HPP
#define TIZSPOTIFY_HPP

#include <boost/python.hpp>

#include <string>

class tizspotify
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
  tizspotify ();
  ~tizspotify ();

  int init ();
  int start ();
  void stop ();
  void deinit ();

  int play_tracks (const std::string &tracks);
  int play_artist (const std::string &artist);
  int play_album (const std::string &album);
  int play_playlist (const std::string &playlist, const std::string &owner);
  int play_track_id (const std::string &track_id);
  int play_artist_id (const std::string &artist_id);
  int play_album_id (const std::string &album_id);
  int play_playlist_id (const std::string &playlist_id,
                        const std::string &owner);

  void set_playback_mode (const playback_mode mode);
  void clear_queue ();
  const char *get_current_track_index ();
  const char *get_current_queue_length ();
  int get_current_queue_length_as_int ();
  const char *get_current_queue_progress ();

  const char *get_next_uri (const bool a_remove_current_uri);
  const char *get_prev_uri (const bool a_remove_current_uri);

  const char *get_current_track_title ();
  const char *get_current_track_artist ();
  const char *get_current_track_album ();
  const char *get_current_track_release_date ();
  const char *get_current_track_duration ();
  const char *get_current_track_album_art ();

private:
  int get_current_track ();

private:
  std::string current_uri_;
  std::string current_track_index_;
  std::string current_queue_length_;
  int current_queue_length_as_int_;
  std::string current_track_title_;
  std::string current_track_artist_;
  std::string current_track_album_;
  std::string current_track_release_date_;
  std::string current_track_duration_;
  std::string current_track_album_art_;
  std::string current_queue_progress_;
  boost::python::object py_main_;
  boost::python::object py_global_;
  boost::python::object py_spotify_proxy_;
};

#endif  // TIZSPOTIFY_HPP
