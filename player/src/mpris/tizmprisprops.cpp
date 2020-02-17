/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   tizmprisprops.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  MPRIS interface - property containers implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizmprisprops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.control.props"
#endif

namespace control = tiz::control;

//
// mpris_mediaplayer2_props
//

control::mpris_mediaplayer2_props::mpris_mediaplayer2_props (
    bool can_quit, bool can_raise, bool has_track_list, std::string identity,
    std::vector< std::string > uri_schemes,
    std::vector< std::string > mime_types)
  : can_quit_ (can_quit),
    can_raise_ (can_raise),
    has_track_list_ (has_track_list),
    identity_ (identity),
    uri_schemes_ (uri_schemes),
    mime_types_ (mime_types)
{
}

//
// mpris_mediaplayer2_player_props
//
control::mpris_mediaplayer2_player_props::mpris_mediaplayer2_player_props (
    std::string playback_status, std::string loop_status, double rate,
    bool shuffle, const track_metadata_map_t &metadata, double volume,
    int64_t position, double minimum_rate, double maximum_rate,
    bool can_go_next, bool can_go_previous, bool can_play, bool can_pause,
    bool can_seek, bool can_control)
  : playback_status_ (playback_status),
    loop_status_ (loop_status),
    rate_ (rate),
    shuffle_ (shuffle),
    metadata_ (metadata),
    volume_ (volume),
    position_ (position),
    minimum_rate_ (minimum_rate),
    maximum_rate_ (maximum_rate),
    can_go_next_ (can_go_next),
    can_go_previous_ (can_go_previous),
    can_play_ (can_play),
    can_pause_ (can_pause),
    can_seek_ (can_seek),
    can_control_ (can_control)
{
}
