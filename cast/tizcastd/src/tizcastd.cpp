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
#include <vector>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

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
  : Tiz::DBus::ObjectAdaptor (a_connection, TIZ_CAST_DAEMON_PATH), p_cc_ctx_(NULL), devices_ ()
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing tizcastd...");
  int rc = tiz_chromecast_ctx_init (&(p_cc_ctx_));
  assert (0 == rc);
}

tizcastd::~tizcastd ()
{
  BOOST_FOREACH (const devices_pair_t &device, devices_)
  {
    tiz::castmgr::mgr *p_cast_mgr = device.second.p_cast_mgr_;
    dispose_mgr (p_cast_mgr);
  }
  tiz_chromecast_ctx_destroy (&(p_cc_ctx_));
}

int32_t tizcastd::connect (const std::vector< uint8_t > &uuid,
                           const std::string &name_or_ip)
{
  tiz::castmgr::mgr *p_cast_mgr = NULL;

  TIZ_LOG (TIZ_PRIORITY_NOTICE, "connect");
  if (devices_.count (name_or_ip))
  {
    p_cast_mgr = devices_[name_or_ip].p_cast_mgr_;
    assert (p_cast_mgr);
    dispose_mgr (p_cast_mgr);
    p_cast_mgr = NULL;
  }

  if (!p_cast_mgr)
  {
    p_cast_mgr = new tiz::castmgr::mgr (p_cc_ctx_,
        name_or_ip,
        boost::bind (&tizcastd::cast_status_forwarder, this, _1, _2, _3),
        boost::bind (&tizcastd::media_status_forwarder, this, _1, _2, _3),
        boost::bind (&tizcastd::error_status_forwarder, this, _1, _2, _3));
    assert (p_cast_mgr);
    p_cast_mgr->init ();

    std::pair< devices_map_t::iterator, bool > rv
        = devices_.insert (std::make_pair (
            name_or_ip, device_info (name_or_ip, uuid, p_cast_mgr)));
    if (rv.second)
    {
      char uuid_str[128];
      tiz_uuid_str (&(uuid[0]), uuid_str);
      TIZ_LOG (TIZ_PRIORITY_NOTICE,
               "'%s' : Successfully registered client with uuid [%s]...",
               name_or_ip.c_str (), uuid_str);
    }
  }
  p_cast_mgr->connect ();
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::disconnect (const std::vector< uint8_t > &uuid)
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "disconnect");
  dispose_mgr (get_mgr (uuid));
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::load_url (const std::vector< uint8_t > &uuid,
                            const std::string &url,
                            const std::string &mime_type,
                            const std::string &title,
                            const std::string &album_art)
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "load_url");
  tiz::castmgr::mgr *p_cast_mgr = get_mgr (uuid);
  if (p_cast_mgr)
  {
    p_cast_mgr->load_url (url, mime_type, title, album_art);
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::play (const std::vector< uint8_t > &uuid)
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "play");
  tiz::castmgr::mgr *p_cast_mgr = get_mgr (uuid);
  if (p_cast_mgr)
  {
    p_cast_mgr->play ();
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::stop (const std::vector< uint8_t > &uuid)
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "stop");
  tiz::castmgr::mgr *p_cast_mgr = get_mgr (uuid);
  if (p_cast_mgr)
  {
    p_cast_mgr->stop ();
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::pause (const std::vector< uint8_t > &uuid)
{
  tiz::castmgr::mgr *p_cast_mgr = get_mgr (uuid);
  if (p_cast_mgr)
  {
    p_cast_mgr->pause ();
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::volume_set (const std::vector< uint8_t > &uuid,
                              const int32_t &volume)
{
  tiz::castmgr::mgr *p_cast_mgr = get_mgr (uuid);
  if (p_cast_mgr)
  {
    p_cast_mgr->volume_set (volume);
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::volume_up (const std::vector< uint8_t > &uuid)
{
  tiz::castmgr::mgr *p_cast_mgr = get_mgr (uuid);
  if (p_cast_mgr)
  {
    p_cast_mgr->volume_up ();
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::volume_down (const std::vector< uint8_t > &uuid)
{
  tiz::castmgr::mgr *p_cast_mgr = get_mgr (uuid);
  if (p_cast_mgr)
  {
    p_cast_mgr->volume_down ();
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::mute (const std::vector< uint8_t > &uuid)
{
  tiz::castmgr::mgr *p_cast_mgr = get_mgr (uuid);
  if (p_cast_mgr)
  {
    p_cast_mgr->mute ();
  }
  return TIZ_CAST_SUCCESS;
}

int32_t tizcastd::unmute (const std::vector< uint8_t > &uuid)
{
  tiz::castmgr::mgr *p_cast_mgr = get_mgr (uuid);
  if (p_cast_mgr)
  {
    p_cast_mgr->unmute ();
  }
  return TIZ_CAST_SUCCESS;
}

void tizcastd::cast_status_forwarder (const std::string &name_or_ip,
                                      const uint32_t &status,
                                      const int32_t &volume)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "forwarding for manager [%s]",
           name_or_ip.c_str ());
  if (devices_.count (name_or_ip))
  {
    cast_status (devices_[name_or_ip].client_uuid_, status, volume);
    TIZ_LOG (TIZ_PRIORITY_TRACE, "forwarded for manager [%s]",
             name_or_ip.c_str ());
  }
}

void tizcastd::media_status_forwarder (const std::string &name_or_ip,
                                       const uint32_t &status,
                                       const int32_t &volume)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "forwarding for manager [%s]",
           name_or_ip.c_str ());
  if (devices_.count (name_or_ip))
  {
    media_status (devices_[name_or_ip].client_uuid_, status, volume);
    TIZ_LOG (TIZ_PRIORITY_TRACE, "forwarded for manager [%s]",
             name_or_ip.c_str ());
  }
}

void tizcastd::error_status_forwarder (const std::string &name_or_ip,
                                       const uint32_t &status,
                                       const std::string &error_str)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "termination signal from manager [%s]",
           name_or_ip.c_str ());
  if (devices_.count (name_or_ip))
  {
    error_status (devices_[name_or_ip].client_uuid_, status, error_str);
    // We don't remove the manager naw. We'll remove it the next time there is
    // an attempt to connect to its managed chromecast device.
  }
}

tiz::castmgr::mgr *tizcastd::get_mgr (std::vector< unsigned char > client_uuid)
{
  tiz::castmgr::mgr *p_cast_mgr = NULL;
  BOOST_FOREACH (const devices_pair_t &device, devices_)
  {
    if (device.second.client_uuid_ == client_uuid)
    {
      p_cast_mgr = device.second.p_cast_mgr_;
      break;
    }
  }
  return p_cast_mgr;
}

void tizcastd::dispose_mgr (tiz::castmgr::mgr *p_cast_mgr)
{
  if (p_cast_mgr)
  {
    p_cast_mgr->deinit ();
    devices_.erase (p_cast_mgr->get_name_or_ip ());
    delete p_cast_mgr;
    p_cast_mgr = NULL;
  }
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
