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
 * @file   tizmprisif.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  The MPRIS interface DBUS adaptor implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizplatform.h>

#include "tizmprisif.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.control.mprisif"
#endif

namespace control = tiz::control;

// Bus name
static const char *TPLAY_MPRIS_BUS_NAME = "com.aratelia.tiz.tplay";

// Object path, a.k.a. node
static const char *TPLAY_MPRIS_OBJECT_PATH = "/com/aratelia/tiz/tplay";

control::tizmprisif::tizmprisif (DBus::Connection &connection,
                                 mpris_mediaplayer2_props_ptr_t mp2_props_ptr,
                                 mpris_mediaplayer2_player_props_ptr_t mp2_player_props_ptr)
  : DBus::ObjectAdaptor (connection, TPLAY_MPRIS_OBJECT_PATH),
    props_ptr_ (props_ptr),
    player_props_ptr_ (player_props_ptr)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing tizmprisif...");
}

control::tizmprisif::~tizmprisif ()
{
}

void control::tizmprisif::Raise()
{
}

void control::tizmprisif::Quit()
{
}

void control::tizmprisif::Next()
{
}

void control::tizmprisif::Previous()
{
}

void control::tizmprisif::Pause()
{
}

void control::tizmprisif::PlayPause()
{
}

void control::tizmprisif::Stop()
{
}

void control::tizmprisif::Play()
{
}

void control::tizmprisif::Seek(const int64_t& Offset)
{
}

void control::tizmprisif::SetPosition(const ::DBus::Path& TrackId, const int64_t& Position)
{
}

void control::tizmprisif::OpenUri(const std::string& Uri)
{
}
