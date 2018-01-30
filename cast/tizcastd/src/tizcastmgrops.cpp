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
 * @file   tizcastmgrops.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph manager operations
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tizmacros.h>
#include <tizplatform.h>

#include "tizcastmgr.hpp"
#include "tizcastmgrops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.mgr.ops"
#endif

namespace castmgr = tiz::castmgr;

#define CAST_MGR_OPS_RECORD_ERROR(err, str)                              \
  do                                                                     \
  {                                                                      \
    error_msg_.assign (str);                                             \
    error_code_ = err;                                                   \
    TIZ_LOG (TIZ_PRIORITY_ERROR, "[%d] : %s", err, error_msg_.c_str ()); \
  } while (0)

#define CAST_MGR_OPS_BAIL_IF_ERROR(exp, str) \
  do                                         \
  {                                          \
    int rc_ = 0;                             \
    if (0 != (rc_ = (exp)))                  \
    {                                        \
      CAST_MGR_OPS_RECORD_ERROR (rc_, str);  \
    }                                        \
  } while (0)

void castmgr::cc_cast_status_cback (void *ap_user_data,
                                    tiz_chromecast_cast_status_t a_status,
                                    int a_volume)
{
  tiz::castmgr::ops *p_ops = static_cast< tiz::castmgr::ops * > (ap_user_data);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "cast status [%d]", a_status);
  assert (p_ops);
  p_ops->cast_received_cb_ ();
  p_ops->cast_cb_ (p_ops->device_name_or_ip (), a_status, a_volume);
}

void castmgr::cc_media_status_cback (void *ap_user_data,
                                     tiz_chromecast_media_status_t a_status,
                                     int a_volume)
{
  tiz::castmgr::ops *p_ops = static_cast< tiz::castmgr::ops * > (ap_user_data);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "media status [%d]", a_status);
  assert (p_ops);
  p_ops->media_cb_ (p_ops->device_name_or_ip (), a_status, a_volume);
}

//
// ops
//
castmgr::ops::ops (mgr *p_mgr, cast_status_received_cback_t cast_received_cb,
                   cast_status_cback_t cast_cb, media_status_cback_t media_cb)
  : p_mgr_ (p_mgr),
    cast_received_cb_ (cast_received_cb),
    cast_cb_ (cast_cb),
    media_cb_ (media_cb),
    error_code_ (OMX_ErrorNone),
    error_msg_ (),
    p_cc_ (NULL)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

castmgr::ops::~ops ()
{
}

void castmgr::ops::deinit ()
{
  tiz_chromecast_destroy (p_cc_);
  p_cc_ = NULL;

  //   termination_cback_ (OMX_ErrorNone, "");
}

void castmgr::ops::do_connect (const std::string &name_or_ip)
{
  // Make sure a previous client has been disconnected
  // disconnect ();
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "do_connect");
  tiz_chromecast_callbacks_t cc_cbacks
      = { tiz::castmgr::cc_cast_status_cback,
          tiz::castmgr::cc_media_status_cback };
  CAST_MGR_OPS_BAIL_IF_ERROR (
      tiz_chromecast_init (&(p_cc_), name_or_ip.c_str (), &cc_cbacks, this),
      "Unable to initialize the Chromecast client library");
}

void castmgr::ops::do_disconnect ()
{
  if (p_cc_)
  {
    tiz_chromecast_destroy (p_cc_);
    p_cc_ = NULL;
  }
}

void castmgr::ops::do_poll (int poll_time_ms)
{
  if (p_cc_)
  {
    CAST_MGR_OPS_BAIL_IF_ERROR (tiz_chromecast_poll (p_cc_, poll_time_ms),
                                "Unable to 'poll' the Chromecast socket");
  }
}

void castmgr::ops::do_load_url (const std::string &url,
                                const std::string &mime_type,
                                const std::string &title,
                                const std::string &album_art)
{
  if (p_cc_)
  {
    TIZ_LOG (TIZ_PRIORITY_NOTICE, "do_load_url");
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_load_url (p_cc_, url.c_str (), mime_type.c_str (),
                                 title.c_str (), album_art.c_str ()),
        "Unable to load the URL into the Chromecast device");
  }
}

void castmgr::ops::do_play ()
{
  if (p_cc_)
  {
    TIZ_LOG (TIZ_PRIORITY_NOTICE, "do_play");
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_play (p_cc_),
        "Unable to deliver 'play' to Chromecast device");
  }
}

void castmgr::ops::do_stop ()
{
  if (p_cc_)
  {
    TIZ_LOG (TIZ_PRIORITY_NOTICE, "do_stop");
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_stop (p_cc_),
        "Unable to deliver 'stop' to Chromecast device");
  }
}

void castmgr::ops::do_pause ()
{
  if (p_cc_)
  {
    TIZ_LOG (TIZ_PRIORITY_NOTICE, "do_pause");
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_pause (p_cc_),
        "Unable to deliver 'pause' to Chromecast device");
  }
}

void castmgr::ops::do_volume (int volume)
{
  if (p_cc_)
  {
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_volume (p_cc_, volume),
        "Unable to deliver 'volume up' to Chromecast device");
  }
}

void castmgr::ops::do_volume_up ()
{
  if (p_cc_)
  {
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_volume_up (p_cc_),
        "Unable to deliver 'volume up' to Chromecast device");
  }
}

void castmgr::ops::do_volume_down ()
{
  if (p_cc_)
  {
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_volume_down (p_cc_),
        "Unable to deliver 'volume down' to Chromecast device");
  }
}

void castmgr::ops::do_mute ()
{
  if (p_cc_)
  {
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_mute (p_cc_),
        "Unable to deliver 'mute' to Chromecast device");
  }
}

void castmgr::ops::do_unmute ()
{
  if (p_cc_)
  {
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_unmute (p_cc_),
        "Unable to deliver 'unmute' to Chromecast device");
  }
}

void castmgr::ops::do_report_fatal_error (const int error,
                                          const std::string &msg)
{
  //   termination_cback_ (error, msg);
}

bool castmgr::ops::is_fatal_error (const int error, const std::string &msg)
{
  TIZ_LOG (TIZ_PRIORITY_ERROR, "[%d] : %s", error, msg.c_str ());
  // This is a generic implementation. We use here some common understanding of
  // fatal errors. Each manager cast may decide to use its own list of fatal
  // errors.
  // return tiz::cast::util::is_fatal_error (error);
  return true;
}

int castmgr::ops::internal_error () const
{
  return error_code_;
}

std::string castmgr::ops::internal_error_msg () const
{
  return error_msg_;
}

std::string castmgr::ops::device_name_or_ip ()
{
  assert (p_mgr_);
  return p_mgr_->name_or_ip_;
}
