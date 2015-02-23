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
 * @file   tizplaybackevents.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Media player signals
 *
 *
 */

#ifndef TIZPLAYBACKEVENTS_HPP
#define TIZPLAYBACKEVENTS_HPP

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/signals2/signal.hpp>

#include <tizplatform.h>

#include <tizgraphtypes.hpp>
#include <tizplaybackstatus.hpp>

namespace tiz
{
  namespace control
  {
    typedef class playback_events playback_events_t;
    class playback_events
    {
    public:
      typedef boost::signals2::signal<void (const playback_status_t status)> playback_status_event_t;
      typedef playback_status_event_t::slot_type playback_status_observer_t;

      typedef boost::signals2::signal<void (const loop_status_t status)> loop_status_event_t;
      typedef loop_status_event_t::slot_type loop_status_observer_t;

      typedef boost::signals2::signal<void (const track_metadata_map_t &metadata)> metadata_event_t;
      typedef metadata_event_t::slot_type metadata_observer_t;

      typedef boost::signals2::signal<void (const double &volume)> volume_event_t;
      typedef volume_event_t::slot_type volume_observer_t;

    public:
      playback_events ();

    public:
      playback_status_event_t playback_;
      loop_status_event_t loop_;
      metadata_event_t metadata_;
      volume_event_t volume_;
    };

    typedef boost::shared_ptr< playback_events_t >
        playback_events_ptr_t;
    typedef boost::scoped_ptr< playback_events_t >
        playback_events_scoped_ptr_t;
  }  // namespace control
}  // namespace tiz

#endif  // TIZPLAYBACKEVENTS_HPP
