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
 * @file   tizchromecast.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Chromecast client library
 *
 *
 */

#ifndef TIZCHROMECAST_HPP
#define TIZCHROMECAST_HPP

#include <boost/python.hpp>

#include <string>

class tizchromecast
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
  tizchromecast (const std::string &oauth_token);
  ~tizchromecast ();

  int init ();
  int start ();
  void stop ();
  void deinit ();

  void set_playback_mode (const playback_mode mode);

private:
  int get_current_track ();

private:
  std::string oauth_token_;
  boost::python::object py_main_;
  boost::python::object py_global_;
  boost::python::object py_gm_proxy_;
};

#endif  // TIZCHROMECAST_HPP
