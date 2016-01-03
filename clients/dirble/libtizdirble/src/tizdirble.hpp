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
 * @file   tizdirble.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Dirble client library
 *
 *
 */

#ifndef TIZDIRBLE_HPP
#define TIZDIRBLE_HPP

#include <boost/python.hpp>

#include <string>

class tizdirble
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
  tizdirble (const std::string &api_key);
  ~tizdirble ();

  int init ();
  int start ();
  void stop ();
  void deinit ();

  int play_popular_stations ();
  int play_stations (const std::string &query);
  int play_category (const std::string &category);
  int play_country (const std::string &country_code);

  void clear_queue ();
  void set_playback_mode (const playback_mode mode);

  const char * get_next_url (const bool a_remove_current_url);
  const char * get_prev_url (const bool a_remove_current_url);
  const char * get_current_station_name ();
  const char * get_current_station_country ();
  const char * get_current_station_category ();
  const char * get_current_station_website ();

private:
  int get_current_station ();

private:
  std::string api_key_;
  std::string current_url_;
  std::string current_station_name_;
  std::string current_station_country_;
  std::string current_station_category_;
  std::string current_station_website_;
  boost::python::object py_main_;
  boost::python::object py_global_;
  boost::python::object py_dirble_proxy_;
};

#endif  // TIZDIRBLE_HPP
