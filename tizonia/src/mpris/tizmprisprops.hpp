/* -*-Mode: c++; -*- */
/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   tizmprisprops.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  MPRIS interface - property containers
 *
 *
 */

#ifndef TIZMPRISPROPS_HPP
#define TIZMPRISPROPS_HPP

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <tizplatform.h>

namespace tiz
{
  namespace control
  {
    typedef class mpris_mediaplayer2_props mpris_mediaplayer2_props_t;
    class mpris_mediaplayer2_props
    {
    public:
      mpris_mediaplayer2_props (bool can_quit, bool can_raise,
                                bool has_track_list, std::string identity,
                                std::vector< std::string > uri_schemes,
                                std::vector< std::string > mime_types);
    public:
      bool can_quit_;
      bool can_raise_;
      bool has_track_list_;
      std::string identity_;
      std::vector< std::string > uri_schemes_;
      std::vector< std::string > mime_types_;
    };

    typedef boost::shared_ptr< mpris_mediaplayer2_props_t >
        mpris_mediaplayer2_props_ptr_t;
    typedef boost::scoped_ptr< mpris_mediaplayer2_props_t >
        mpris_mediaplayer2_props_scoped_ptr_t;

    typedef class mpris_mediaplayer2_player_props
        mpris_mediaplayer2_player_props_t;
    class mpris_mediaplayer2_player_props
    {
    public:
      mpris_mediaplayer2_player_props (
          std::string playback_status, std::string loop_status, double rate,
          bool shuffle, std::map< std::string, std::string > metadata,
          double volume, int64_t position, double minimum_rate,
          double maximum_rate, bool can_go_next, bool can_go_previous,
          bool can_play, bool can_pause, bool can_seek, bool can_control);

    public:
      std::string playback_status_;
      std::string loop_status_;
      double rate_;
      bool shuffle_;
      std::map< std::string, std::string > metadata_;
      double volume_;
      int64_t position_;
      double minimum_rate_;
      double maximum_rate_;
      bool can_go_next_;
      bool can_go_previous_;
      bool can_play_;
      bool can_pause_;
      bool can_seek_;
      bool can_control_;
    };

    typedef boost::shared_ptr< mpris_mediaplayer2_player_props_t >
        mpris_mediaplayer2_player_props_ptr_t;
    typedef boost::scoped_ptr< mpris_mediaplayer2_player_props_t >
        mpris_mediaplayer2_player_props_scoped_ptr_t;

  }  // namespace control
}  // namespace tiz

#endif  // TIZMPRISPROPS_HPP
