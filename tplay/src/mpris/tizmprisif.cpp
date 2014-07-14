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

// Object path, a.k.a. node
const char *control::mprisif::TPLAY_MPRIS_OBJECT_PATH = "/com/aratelia/tiz/tplay";

control::mprisif::mprisif (
    DBus::Connection& connection,
    mpris_mediaplayer2_props_t props,
    mpris_mediaplayer2_player_props_t player_props,
    mpris_callbacks_t cbacks)
  : DBus::ObjectAdaptor (connection, TPLAY_MPRIS_OBJECT_PATH),
    props_ (props),
    player_props_ (player_props),
    cbacks_ (cbacks)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing mprisif...");
  UpdateProps (props_);
  UpdatePlayerProps (player_props_);
}

void control::mprisif::on_set_property
(DBus::InterfaceAdaptor &interface, const std::string &property, const DBus::Variant &value)
{
  if (property == "Volumen")
  {
    std::cout << "'Message' has been changed\n";
  }
  if (property == "Data")
  {
    std::cout << "'Data' has been changed\n";
  }
}

void control::mprisif::Raise ()
{
  // No-op
}

void control::mprisif::Quit ()
{
  // No-op for now
}

void control::mprisif::Next ()
{
  cbacks_.next_ ();
}

void control::mprisif::Previous ()
{
  cbacks_.previous_ ();
}

void control::mprisif::Pause ()
{
  cbacks_.pause_ ();
}

void control::mprisif::PlayPause ()
{
  cbacks_.pause_ ();
}

void control::mprisif::Stop ()
{
  cbacks_.stop_ ();
}

void control::mprisif::Play ()
{
  cbacks_.play_ ();
}

void control::mprisif::Seek (const int64_t& Offset)
{
  // No-op for now
}

void control::mprisif::SetPosition (const ::DBus::Path& TrackId,
                                    const int64_t& Position)
{
  // No-op for now
}

void control::mprisif::OpenUri (const std::string& Uri)
{
  // No-op for now
}

void control::mprisif::UpdateProps (const mpris_mediaplayer2_props_t &props)
{
  CanQuit = props.can_quit_;
  CanRaise = props.can_raise_;
  HasTrackList = props.has_track_list_;
  Identity = props.identity_;
  SupportedUriSchemes = props.uri_schemes_;
  SupportedMimeTypes = props.mime_types_;
}

void control::mprisif::UpdatePlayerProps (
    const mpris_mediaplayer2_player_props_t &props)
{
  PlaybackStatus = props.playback_status_;
  LoopStatus = props.loop_status_;
  Rate = props.rate_;
  Shuffle = props.shuffle_;
  // Metadata = props.metadata_;
  Volume = props.volume_;
  Position = props.position_;
  MinimumRate = props.minimum_rate_;
  MaximumRate = props.maximum_rate_;
  CanGoNext = props.can_go_next_;
  CanGoPrevious = props.can_go_previous_;
  CanPlay = props.can_play_;
  CanPause = props.can_pause_;
  CanSeek = props.can_seek_;
  CanControl = props.can_control_;
}
