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
 * @file   tizcastd.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Chromecast access daemon implementation
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

#include <boost/bind.hpp>

#include <tizplatform.h>

#include <tizcasttypes.h>

#include "tizcastd.hpp"
#include "tizcastmgr.hpp"
#include "tizcastmgrtypes.hpp"

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
    p_cast_mgr_ (new tiz::castmgr::mgr (
        boost::bind (&tizcastd::cast_status, this, _1, _2),
        boost::bind (&tizcastd::media_status, this, _1, _2))),
    cc_name_or_ip_ ()
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing tizcastd...");
  p_cast_mgr_->init ();
}

tizcastd::~tizcastd ()
{
  if (p_cast_mgr_)
  {
    p_cast_mgr_->deinit ();
    delete p_cast_mgr_;
  }
}

int32_t tizcastd::connect (const std::string &name_or_ip)
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "connect");
  p_cast_mgr_->connect (name_or_ip);
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::disconnect ()
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "disconnect");
  p_cast_mgr_->disconnect ();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::load_url (const std::string &url,
                            const std::string &mime_type,
                            const std::string &title)
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "load_url");
  p_cast_mgr_->load_url (url, mime_type, title);
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::play ()
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "play");
  p_cast_mgr_->play ();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::stop ()
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "stop");
  p_cast_mgr_->stop ();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::pause ()
{
  p_cast_mgr_->pause ();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::volume_set (const int32_t & volume)
{
  p_cast_mgr_->volume_set (volume);
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::volume_up ()
{
  p_cast_mgr_->volume_up ();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::volume_down ()
{
  p_cast_mgr_->volume_down ();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::mute ()
{
  p_cast_mgr_->mute ();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::unmute ()
{
  p_cast_mgr_->unmute ();
  return TIZ_CAST_SUCCESS;
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
