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
 * @file   tizplaybacksignals.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Media player signals
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizplaybacksignals.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.control.signals"
#endif

namespace control = tiz::control;

//
// playback_signals
//

control::playback_signals::playback_signals ()
  : PlaybackStatus_ (), LoopStatus_ (), Metadata_ (), Volume_ ()
{
}
