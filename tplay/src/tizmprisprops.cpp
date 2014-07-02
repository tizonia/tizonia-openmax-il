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
control::mpris_mediaplayer2_props::mpris_mediaplayer2_props ()
{
}

control::mpris_mediaplayer2_props::~mpris_mediaplayer2_props ()
{
}

//
// mpris_mediaplayer2_player_props
//
control::mpris_mediaplayer2_player_props::mpris_mediaplayer2_player_props ()
{
}

control::mpris_mediaplayer2_player_props::~mpris_mediaplayer2_player_props ()
{
}
