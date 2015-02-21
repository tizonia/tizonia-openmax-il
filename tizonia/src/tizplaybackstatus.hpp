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
 * @file   tizplaybackstatus.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Playback Status
 *
 *
 */

#ifndef TIZPLAYBACKSTATUS_HPP
#define TIZPLAYBACKSTATUS_HPP

namespace tiz
{
  namespace control
  {
    enum playback_status_t
      {
        // A track is currently playing.
        Playing,
        // A track is currently paused.
        Paused,
        // There is no track currently playing.
        Stopped
      };

    enum loop_status_t
      {
        // The playback will stop when there are no more tracks to play.
        None,
        // The current track will start again from the begining once it has finished playing
        Track,
        // The playback loops through a list of tracks
        Playlist
      };
  }  // namespace control
}  // namespace tiz

#endif  // TIZPLAYBACKSTATUS_HPP
