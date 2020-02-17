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
 * @file   tizcastmgrops.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia's Chromecast daemon - manager operations.
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

namespace cast = tiz::cast;

#define CAST_MGR_OPS_RECORD_ERROR(err, str)                              \
  do                                                                     \
  {                                                                      \
    error_msg_.assign (str);                                             \
    error_code_ = err;                                                   \
    TIZ_LOG (TIZ_PRIORITY_ERROR, "[%d] : %s", err, error_msg_.c_str ()); \
  } while (0)

#define CAST_MGR_OPS_BAIL_IF_ERROR(exp, str)         \
  do                                                 \
  {                                                  \
    tiz_chromecast_error_t rc_ = ETizCcErrorNoError; \
    if (ETizCcErrorNoError != (rc_ = (exp)))         \
    {                                                \
      CAST_MGR_OPS_RECORD_ERROR (rc_, str);          \
    }                                                \
  } while (0)

void cast::cc_cast_status_cback (void *ap_user_data,
                                 tiz_chromecast_cast_status_t a_status,
                                 int a_volume)
{
  tiz::cast::ops *p_ops = static_cast< tiz::cast::ops * > (ap_user_data);
  char uuid_str[128];
  tiz_uuid_str (&(p_ops->uuid ()[0]), uuid_str);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "cast status [%d] p_ops [%p] uuid [%s]",
           a_status, p_ops, uuid_str);
  assert (p_ops);
  p_ops->cast_received_cb_ ();
  p_ops->cast_cb_ (p_ops->uuid (), a_status, a_volume);
}

void cast::cc_media_status_cback (void *ap_user_data,
                                  tiz_chromecast_media_status_t a_status,
                                  int a_volume)
{
  tiz::cast::ops *p_ops = static_cast< tiz::cast::ops * > (ap_user_data);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "media status [%d]", a_status);
  assert (p_ops);
  p_ops->media_cb_ (p_ops->uuid (), a_status, a_volume);
}

//
// ops
//
cast::ops::ops (mgr *p_mgr, const tiz_chromecast_ctx_t *p_cc_ctx,
                cast_status_received_cback_t cast_received_cb,
                cast_status_cback_t cast_cb, media_status_cback_t media_cb,
                error_status_callback_t error_cb)
  : p_mgr_ (p_mgr),
    p_cc_ctx_ (p_cc_ctx),
    cast_received_cb_ (cast_received_cb),
    cast_cb_ (cast_cb),
    media_cb_ (media_cb),
    error_cb_ (error_cb),
    error_code_ (OMX_ErrorNone),
    error_msg_ (),
    p_cc_ (NULL)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

cast::ops::~ops ()
{
}

void cast::ops::deinit ()
{
  tiz_chromecast_destroy (p_cc_);
  p_cc_ = NULL;
}

cast::uuid_t cast::ops::uuid () const
{
  assert (p_mgr_);
  return p_mgr_->uuid ();
}

void cast::ops::do_connect (const std::string &name_or_ip)
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "do_connect");

  // NOTE: For now, we will handle just one client. In the future, this may
  // change.
  // Make sure a previous client has been disconnected
  if (p_cc_)
  {
    do_disconnect ();
  }

  tiz_chromecast_callbacks_t cc_cbacks
      = { tiz::cast::cc_cast_status_cback, tiz::cast::cc_media_status_cback };
  tiz_chromecast_error_t rc = tiz_chromecast_init (
      &(p_cc_), p_cc_ctx_, name_or_ip.c_str (), &cc_cbacks, this);
  std::string error_msg = tiz_chromecast_error_str (rc);
  CAST_MGR_OPS_BAIL_IF_ERROR (rc, error_msg);
}

void cast::ops::do_disconnect ()
{
  if (p_cc_)
  {
    tiz_chromecast_destroy (p_cc_);
    p_cc_ = NULL;
  }
}

void cast::ops::do_poll (int poll_time_ms)
{
  if (p_cc_)
  {
    CAST_MGR_OPS_BAIL_IF_ERROR (tiz_chromecast_poll (p_cc_, poll_time_ms),
                                "Unable to 'poll' the Chromecast socket");
  }
}

void cast::ops::do_load_url (const std::string &url,
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

void cast::ops::do_play ()
{
  if (p_cc_)
  {
    TIZ_LOG (TIZ_PRIORITY_NOTICE, "do_play");
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_play (p_cc_),
        "Unable to deliver 'play' to Chromecast device");
  }
}

void cast::ops::do_stop ()
{
  if (p_cc_)
  {
    TIZ_LOG (TIZ_PRIORITY_NOTICE, "do_stop");
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_stop (p_cc_),
        "Unable to deliver 'stop' to Chromecast device");
  }
}

void cast::ops::do_pause ()
{
  if (p_cc_)
  {
    TIZ_LOG (TIZ_PRIORITY_NOTICE, "do_pause");
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_pause (p_cc_),
        "Unable to deliver 'pause' to Chromecast device");
  }
}

void cast::ops::do_volume (int volume)
{
  if (p_cc_)
  {
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_volume (p_cc_, volume),
        "Unable to deliver 'volume up' to Chromecast device");
  }
}

void cast::ops::do_volume_up ()
{
  if (p_cc_)
  {
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_volume_up (p_cc_),
        "Unable to deliver 'volume up' to Chromecast device");
  }
}

void cast::ops::do_volume_down ()
{
  if (p_cc_)
  {
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_volume_down (p_cc_),
        "Unable to deliver 'volume down' to Chromecast device");
  }
}

void cast::ops::do_mute ()
{
  if (p_cc_)
  {
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_mute (p_cc_),
        "Unable to deliver 'mute' to Chromecast device");
  }
}

void cast::ops::do_unmute ()
{
  if (p_cc_)
  {
    CAST_MGR_OPS_BAIL_IF_ERROR (
        tiz_chromecast_unmute (p_cc_),
        "Unable to deliver 'unmute' to Chromecast device");
  }
}

void cast::ops::do_report_fatal_error (const int error, const std::string &msg)
{
  error_cb_ (uuid (), error, msg);
}

bool cast::ops::is_fatal_error (const int error, const std::string &msg)
{
  TIZ_LOG (TIZ_PRIORITY_ERROR, "[%d] : %s", error, msg.c_str ());
  return true;
}

int cast::ops::internal_error () const
{
  return error_code_;
}

std::string cast::ops::internal_error_msg () const
{
  return error_msg_;
}
