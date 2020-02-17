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
 * @file   tizcastd.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia's Chromecast daemon implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <vector>

#include <boost/bind.hpp>

#include <tizplatform.h>

#include <tizcasttypes.h>

#include "tizcastd.hpp"
#include "tizcastworker.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.daemon"
#endif

Tiz::DBus::BusDispatcher dispatcher;

// Bus name
static const char *TIZ_CAST_DAEMON_NAME = "com.aratelia.tiz.tizcastd";

// Object path, a.k.a. node
static const char *TIZ_CAST_DAEMON_PATH = "/com/aratelia/tiz/tizcastd";

tizcastd::tizcastd (Tiz::DBus::Connection &a_connection)
  : Tiz::DBus::ObjectAdaptor (a_connection, TIZ_CAST_DAEMON_PATH),
    p_worker_ (NULL)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing tizcastd...");
  p_worker_ = new tiz::cast::worker (
      boost::bind (&tizcastd::cast_status_forwarder, this, _1, _2, _3),
      boost::bind (&tizcastd::media_status_forwarder, this, _1, _2, _3),
      boost::bind (&tizcastd::error_status_forwarder, this, _1, _2, _3));
  assert (p_worker_);
  p_worker_->init ();
}

tizcastd::~tizcastd ()
{
  if (p_worker_)
  {
    p_worker_->deinit ();
    delete p_worker_;
    p_worker_ = NULL;
  }
}

int32_t tizcastd::connect (const std::vector< uint8_t > &uuid,
                           const std::string &name_or_ip)
{
  if (p_worker_)
  {
    p_worker_->connect (uuid, name_or_ip);
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::disconnect (const std::vector< uint8_t > &uuid)
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "disconnect");
  if (p_worker_)
  {
    p_worker_->disconnect (uuid);
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::load_url (const std::vector< uint8_t > &uuid,
                            const std::string &url,
                            const std::string &mime_type,
                            const std::string &title,
                            const std::string &album_art)
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "load_url");
  if (p_worker_)
  {
    p_worker_->load_url (uuid, url, mime_type, title, album_art);
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::play (const std::vector< uint8_t > &uuid)
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "play");
  if (p_worker_)
  {
    p_worker_->play (uuid);
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::stop (const std::vector< uint8_t > &uuid)
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "stop");
  if (p_worker_)
  {
    p_worker_->stop (uuid);
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::pause (const std::vector< uint8_t > &uuid)
{
  if (p_worker_)
  {
    p_worker_->pause (uuid);
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::volume_set (const std::vector< uint8_t > &uuid,
                              const int32_t &volume)
{
  if (p_worker_)
  {
    p_worker_->volume_set (uuid, volume);
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::volume_up (const std::vector< uint8_t > &uuid)
{
  if (p_worker_)
  {
    p_worker_->volume_up (uuid);
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::volume_down (const std::vector< uint8_t > &uuid)
{
  if (p_worker_)
  {
    p_worker_->volume_down (uuid);
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::mute (const std::vector< uint8_t > &uuid)
{
  if (p_worker_)
  {
    p_worker_->mute (uuid);
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::unmute (const std::vector< uint8_t > &uuid)
{
  if (p_worker_)
  {
    p_worker_->unmute (uuid);
  }
  return TIZ_CAST_SUCCESS;
}

void tizcastd::cast_status_forwarder (const uuid_t &uuid,
                                      const uint32_t &status,
                                      const int32_t &volume)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "forwarding from manager");
  cast_status (uuid, status, volume);
}

void tizcastd::media_status_forwarder (const uuid_t &uuid,
                                       const uint32_t &status,
                                       const int32_t &volume)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "media status from manager");
  media_status (uuid, status, volume);
}

void tizcastd::error_status_forwarder (const uuid_t &uuid,
                                       const uint32_t &status,
                                       const std::string &error_str)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "error signal from manager");
  error_status (uuid, status, error_str);
}

static void tizcastd_sig_hdlr (int sig)
{
  dispatcher.leave ();
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Tizonia Chromecast daemon exiting...");
}

int main ()
{
  signal (SIGTERM, tizcastd_sig_hdlr);
  signal (SIGINT, tizcastd_sig_hdlr);

  tiz_log_init ();

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Tizonia Chromecast daemon starting...");

  Tiz::DBus::default_dispatcher = &dispatcher;
  Tiz::DBus::Connection conn = Tiz::DBus::Connection::SessionBus ();
  conn.request_name (TIZ_CAST_DAEMON_NAME);
  tizcastd server (conn);
  dispatcher.enter ();

  tiz_log_deinit ();
  return 0;
}
