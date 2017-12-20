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

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.daemon"
#endif

Tiz::DBus::BusDispatcher dispatcher;

// Bus name
static const char *TIZ_CAST_DAEMON_NAME = "com.aratelia.tiz.tizcastd";

// Object path, a.k.a. node
static const char *TIZ_CAST_DAEMON_PATH = "/com/aratelia/tiz/tizcastd";

static void cc_new_media_status_cback (void *ap_user_data)
{
  // TODO
}

tizcastd::tizcastd (Tiz::DBus::Connection &a_connection)
  : Tiz::DBus::ObjectAdaptor (a_connection, TIZ_CAST_DAEMON_PATH),
    p_cc_ (NULL),
    cc_name_or_ip_ ()
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing tizcastd...");
}

tizcastd::~tizcastd ()
{
  tiz_chromecast_destroy (p_cc_);
  p_cc_ = NULL;
}

int32_t tizcastd::connect (const std::string &name_or_ip)
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  // Make sure a previous client has been disconnected
  disconnect ();
  if (0 != tiz_chromecast_init (&(p_cc_), name_or_ip.c_str(),
                                cc_new_media_status_cback,
                                this))
    {
      outcome = TIZ_CAST_MISUSE;
    }
  return outcome;
}

int32_t tizcastd::disconnect ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (p_cc_)
    {
      tiz_chromecast_destroy (p_cc_);
      p_cc_ = NULL;
    }
  return outcome;
}

int32_t tizcastd::load_url (const std::string &url,
                            const std::string &mime_type,
                            const std::string &title)
{

  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_load_url (p_cc_, url.c_str (), mime_type.c_str (),
                                    title.c_str ()))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While loading url : [%s]", url.c_str ());
      outcome = TIZ_CAST_URL_LOAD_FAILURE;
    }
  return outcome;
}

int32_t tizcastd::play ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_play (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking play");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

int32_t tizcastd::stop ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_stop (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking stop");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

int32_t tizcastd::pause ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_pause (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking pause");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

int32_t tizcastd::volume_up ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_volume_up (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking volume up");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

int32_t tizcastd::volume_down ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_volume_down (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking volume down");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

int32_t tizcastd::mute ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_mute (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking mute");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

int32_t tizcastd::unmute ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_unmute (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking unmute");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
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
