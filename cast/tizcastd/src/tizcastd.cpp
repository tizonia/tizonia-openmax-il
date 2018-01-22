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
 * @brief  Tizonia OpenMAX IL - Chromecast access daemon implementation
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

#include <tizplatform.h>


#include <tizcasttypes.h>

#include "tizcastd.hpp"
#include "tizcastmgr.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.daemon"
#endif

Tiz::DBus::BusDispatcher dispatcher;

// Bus name
static const char *TIZ_CAST_DAEMON_NAME = "com.aratelia.tiz.tizcastd";

// Object path, a.k.a. node
static const char *TIZ_CAST_DAEMON_PATH = "/com/aratelia/tiz/tizcastd";

tizcastd::tizcastd (Tiz::DBus::Connection &a_connection,
                    tiz::castmgr::mgr &mgr)
  : Tiz::DBus::ObjectAdaptor (a_connection, TIZ_CAST_DAEMON_PATH),
    cast_mgr_(mgr),
    cc_name_or_ip_ ()
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing tizcastd...");
}

tizcastd::~tizcastd ()
{
}

int32_t tizcastd::connect (const std::string &name_or_ip)
{
  cast_mgr_.connect(name_or_ip);
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::disconnect ()
{
  cast_mgr_.disconnect();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::load_url (const std::string &url,
                            const std::string &mime_type,
                            const std::string &title)
{
  cast_mgr_.load_url(url, mime_type, title);
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::play ()
{
  cast_mgr_.play();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::stop ()
{
  cast_mgr_.stop();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::pause ()
{
  cast_mgr_.pause();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::volume_up ()
{
  cast_mgr_.volume_up();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::volume_down ()
{
  cast_mgr_.volume_down();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::mute ()
{
  cast_mgr_.mute();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::unmute ()
{
  cast_mgr_.unmute();
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

  tiz::castmgr::mgr mgr;
  if (OMX_ErrorNone == mgr.init())
    {
      Tiz::DBus::default_dispatcher = &dispatcher;
      Tiz::DBus::Connection conn = Tiz::DBus::Connection::SessionBus ();
      conn.request_name (TIZ_CAST_DAEMON_NAME);
      mgr.start();
      tizcastd server (conn, mgr);
      dispatcher.enter ();
      mgr.quit();
      mgr.deinit();
    }
  tiz_log_deinit ();

  return 0;
}
