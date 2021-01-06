/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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
 * @file   tizchromecast.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Chromecast client library
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <unistd.h>

#include <boost/lexical_cast.hpp>
#include <iostream>

#include "tizchromecast.hpp"
#include "tizchromecastctx.hpp"

namespace bp = boost::python;

/* This macro assumes the existence of an "tiz_chromecast_error_t rc" local
 * variable */
#define try_catch_wrapper(expr)                                  \
  do                                                             \
    {                                                            \
      try                                                        \
        {                                                        \
          (expr);                                                \
        }                                                        \
      catch (bp::error_already_set & e)                          \
        {                                                        \
          PyErr_PrintEx (0);                                     \
          rc = ETizCcErrorConnectionError;                       \
        }                                                        \
      catch (const std::exception &e)                            \
        {                                                        \
          std::cerr << e.what ();                                \
          rc = ETizCcErrorConnectionError;                       \
        }                                                        \
      catch (...)                                                \
        {                                                        \
          std::cerr << std::string ("Unknown exception caught"); \
          rc = ETizCcErrorConnectionError;                       \
        }                                                        \
    }                                                            \
  while (0)

tizchromecast::tizchromecast (const tizchromecastctx &cc_ctx,
                              const std::string &name_or_ip,
                              const tiz_chromecast_callbacks_t *ap_cbacks,
                              void *ap_user_data)
  : cc_ctx_ (cc_ctx),
    name_or_ip_ (name_or_ip),
    cbacks_ (),
    p_user_data_ (ap_user_data)
{
  if (ap_cbacks)
    {
      cbacks_.pf_cast_status = ap_cbacks->pf_cast_status;
      cbacks_.pf_media_status = ap_cbacks->pf_media_status;
    }
}

tizchromecast::~tizchromecast ()
{
  cbacks_.pf_cast_status = NULL;
  cbacks_.pf_media_status = NULL;
}

tiz_chromecast_error_t tizchromecast::init ()
{
  return ETizCcErrorNoError;
}

tiz_chromecast_error_t tizchromecast::start ()
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;

  bp::object &py_cc_proxy = cc_ctx_.create_cc_proxy (name_or_ip_);

  if (ETizCcErrorNoError == rc)
    {
      typedef boost::function< void (std::string, float) > handler_fn1;
      typedef boost::function< void (std::string, int) > handler_fn2;
      handler_fn1 cast_status_handler (
          boost::bind (&tizchromecast::new_cast_status, this, boost:placeholders::_1, boost:placeholders::_2));
      handler_fn2 media_status_handler (
          boost::bind (&tizchromecast::new_media_status, this, boost:placeholders::_1, boost:placeholders::_2));
      try_catch_wrapper (py_cc_proxy.attr ("activate") (
          bp::make_function (cast_status_handler),
          bp::make_function (media_status_handler)));
    }
  // std::cout << "tizchromecast::start: rc " << rc << std::endl;
  return rc;
}

void tizchromecast::stop ()
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;
  if (cc_ctx_.cc_proxy_exists (name_or_ip_))
    {
      try_catch_wrapper (
          cc_ctx_.get_cc_proxy (name_or_ip_).attr ("deactivate") ());
      try_catch_wrapper (cc_ctx_.destroy_cc_proxy (name_or_ip_));
    }
  (void)rc;
}

void tizchromecast::deinit ()
{
  // boost::python doesn't support Py_Finalize() yet!
}

tiz_chromecast_error_t tizchromecast::poll_socket (int a_poll_time_ms)
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;

  if (cc_ctx_.cc_proxy_exists (name_or_ip_))
    {
      try_catch_wrapper (
          cc_ctx_.get_cc_proxy (name_or_ip_)
              .attr ("poll_socket") (bp::object (a_poll_time_ms)));
    }
  return rc;
}

tiz_chromecast_error_t tizchromecast::media_load (
    const std::string &url, const std::string &content_type,
    const std::string &title, const std::string &album_art)
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;
  // std::cout << "tizchromecast::media_load: " << url << std::endl;

  if (cc_ctx_.cc_proxy_exists (name_or_ip_))
    {
      try_catch_wrapper (cc_ctx_.get_cc_proxy (name_or_ip_)
                             .attr ("media_load") (
                                 bp::object (url), bp::object (content_type),
                                 bp::object (title), bp::object (album_art)));
    }
  return rc;
}

tiz_chromecast_error_t tizchromecast::media_play ()
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;
  // std::cout << "tizchromecast::media_play: " << std::endl;
  if (cc_ctx_.cc_proxy_exists (name_or_ip_))
    {
      try_catch_wrapper (
          cc_ctx_.get_cc_proxy (name_or_ip_).attr ("media_play") ());
    }
  return rc;
}

tiz_chromecast_error_t tizchromecast::media_stop ()
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;
  // std::cout << "tizchromecast::media_stop: " << std::endl;
  if (cc_ctx_.cc_proxy_exists (name_or_ip_))
    {
      try_catch_wrapper (
          cc_ctx_.get_cc_proxy (name_or_ip_).attr ("media_stop") ());
    }
  return rc;
}

tiz_chromecast_error_t tizchromecast::media_pause ()
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;
  if (cc_ctx_.cc_proxy_exists (name_or_ip_))
    {
      try_catch_wrapper (
          cc_ctx_.get_cc_proxy (name_or_ip_).attr ("media_pause") ());
    }
  return rc;
}

tiz_chromecast_error_t tizchromecast::media_volume (int volume)
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;
  if (cc_ctx_.cc_proxy_exists (name_or_ip_))
    {
      try_catch_wrapper (cc_ctx_.get_cc_proxy (name_or_ip_)
                             .attr ("media_vol") (bp::object (volume)));
    }
  return rc;
}

tiz_chromecast_error_t tizchromecast::media_volume_up ()
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;
  if (cc_ctx_.cc_proxy_exists (name_or_ip_))
    {
      try_catch_wrapper (
          cc_ctx_.get_cc_proxy (name_or_ip_).attr ("media_vol_up") ());
    }
  return rc;
}

tiz_chromecast_error_t tizchromecast::media_volume_down ()
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;
  if (cc_ctx_.cc_proxy_exists (name_or_ip_))
    {
      try_catch_wrapper (
          cc_ctx_.get_cc_proxy (name_or_ip_).attr ("media_vol_down") ());
    }
  return rc;
}

tiz_chromecast_error_t tizchromecast::media_mute ()
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;
  if (cc_ctx_.cc_proxy_exists (name_or_ip_))
    {
      try_catch_wrapper (
          cc_ctx_.get_cc_proxy (name_or_ip_).attr ("media_mute") ());
    }
  return rc;
}

tiz_chromecast_error_t tizchromecast::media_unmute ()
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;
  if (cc_ctx_.cc_proxy_exists (name_or_ip_))
    {
      try_catch_wrapper (
          cc_ctx_.get_cc_proxy (name_or_ip_).attr ("media_unmute") ());
    }
  return rc;
}

void tizchromecast::new_cast_status (const std::string &status,
                                     const float &volume)
{
  const int vol = volume * (float)100;
  // std::cout << "tizchromecast::new_cast_status: " << status << " volume "
  //             << volume << " pid " << getpid () << std::endl;
  if (!status.compare ("UNKNOWN"))
    {
      if (cbacks_.pf_cast_status)
        {
          cbacks_.pf_cast_status (p_user_data_, ETizCcCastStatusUnknown, vol);
        }
    }
  else if (!status.compare ("READY_TO_CAST"))
    {
      if (cbacks_.pf_cast_status)
        {
          cbacks_.pf_cast_status (p_user_data_, ETizCcCastStatusReadyToCast,
                                  vol);
        }
    }
  else if (!status.compare ("NOW_CASTING"))
    {
      if (cbacks_.pf_cast_status)
        {
          cbacks_.pf_cast_status (p_user_data_, ETizCcCastStatusNowCasting,
                                  vol);
        }
    }
  else
    {
      assert (0);
    }
}

void tizchromecast::new_media_status (const std::string &status,
                                      const int &volume)
{
  // std::cout << "tizchromecast::new_media_status: " << status << " volume "
  //           << volume << " pid " << getpid () << std::endl;
  if (!status.compare ("UNKNOWN"))
    {
      if (cbacks_.pf_media_status)
        {
          cbacks_.pf_media_status (p_user_data_, ETizCcMediaStatusUnknown,
                                   volume);
        }
    }
  else if (!status.compare ("IDLE"))
    {
      if (cbacks_.pf_media_status)
        {
          cbacks_.pf_media_status (p_user_data_, ETizCcMediaStatusIdle, volume);
        }
    }
  else if (!status.compare ("BUFFERING"))
    {
      if (cbacks_.pf_media_status)
        {
          cbacks_.pf_media_status (p_user_data_, ETizCcMediaStatusBuffering,
                                   volume);
        }
    }
  else if (!status.compare ("PAUSED"))
    {
      if (cbacks_.pf_media_status)
        {
          cbacks_.pf_media_status (p_user_data_, ETizCcMediaStatusPaused,
                                   volume);
        }
    }
  else if (!status.compare ("PLAYING"))
    {
      if (cbacks_.pf_media_status)
        {
          cbacks_.pf_media_status (p_user_data_, ETizCcMediaStatusPlaying,
                                   volume);
        }
    }
  else
    {
      assert (0);
    }
}
