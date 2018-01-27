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
 * @file   tizcastclient.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL Chromecast daemon - client library (c++ implementation)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <utility>

#include "tizcasttypes.h"
#include "tizcastclient.hh"
#include "tizplatform.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.client.cc_api"
#endif

tizcastclient::tizcastclient (Tiz::DBus::Connection & connection,
                              const char * path, const char * name)
  : Tiz::DBus::ObjectProxy (connection, path, name), clients_ ()
{
}

tizcastclient::~tizcastclient ()
{
  // Check if there are clients
}

const tizcastclient::cast_client_id_ptr_t
tizcastclient::connect (const char * ap_device_name_or_ip, const uint8_t uuid[],
                        const tiz_cast_client_callbacks_t * ap_cbacks,
                        void * ap_data)
{
  int32_t rc = TIZ_CAST_SUCCESS;
  cast_client_id_ptr_t client_id = NULL;

  if ((client_id
       = register_client (ap_device_name_or_ip, uuid, ap_cbacks, ap_data)))
    {
      try
        {
          //         client_data & clnt = clients_[*p_uuid_vec];
          rc = com::aratelia::tiz::tizcastif_proxy::connect (
            ap_device_name_or_ip);
        }
      catch (Tiz::DBus::Error const & e)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "DBus error [%s]...", e.what ());
          rc = TIZ_CAST_DBUS;
        }
      catch (std::exception const & e)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Standard exception error [%s]...",
                   e.what ());
          rc = TIZ_CAST_UNKNOWN;
        }
      catch (...)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Uknonwn exception error...");
          rc = TIZ_CAST_UNKNOWN;
        }
      if (TIZ_CAST_SUCCESS != rc)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Connect error [%d]", rc);
          (void) unregister_client (client_id);
          client_id = NULL;
        }
    }
  else
    {
      rc = TIZ_CAST_MISUSE;
      char uuid_str[128];
      tiz_uuid_str (uuid, uuid_str);
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "Unable to connect the client with uuid [%s]...", uuid_str);
    }
  return client_id;
}

int32_t
tizcastclient::disconnect (const cast_client_id_ptr_t ap_cast_clnt)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::disconnect,
                     ap_cast_clnt);
}

int32_t
tizcastclient::load_url (const cast_client_id_ptr_t ap_cast_clnt,
                         const char * url, const char * mime_type,
                         const char * title)
{
  int32_t rc = TIZ_CAST_SUCCESS;
  assert (ap_cast_clnt);

  if (clients_.count (*ap_cast_clnt))
    {
      try
        {
          //         client_data & clnt = clients_[*ap_cast_clnt];
          rc = com::aratelia::tiz::tizcastif_proxy::load_url (url, mime_type,
                                                              title);
        }
      catch (Tiz::DBus::Error const & e)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "DBus error [%s]...", e.what ());
          rc = TIZ_CAST_DBUS;
        }
      catch (std::exception const & e)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Standard exception error [%s]...",
                   e.what ());
          rc = TIZ_CAST_UNKNOWN;
        }
      catch (...)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Uknonwn exception error...");
          rc = TIZ_CAST_UNKNOWN;
        }
    }
  else
    {
      rc = TIZ_CAST_MISUSE;
      char uuid_str[128];
      tiz_uuid_str (&((*ap_cast_clnt)[0]), uuid_str);
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "Could not find the client with uuid [%s]...", uuid_str);
    }
  return rc;
}

int32_t
tizcastclient::play (const cast_client_id_ptr_t ap_cast_clnt)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::play, ap_cast_clnt);
}

int32_t
tizcastclient::stop (const cast_client_id_ptr_t ap_cast_clnt)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::stop, ap_cast_clnt);
}

int32_t
tizcastclient::pause (const cast_client_id_ptr_t ap_cast_clnt)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::pause, ap_cast_clnt);
}

int32_t
tizcastclient::volume (const cast_client_id_ptr_t ap_cast_clnt, int a_volume)
{
  int32_t rc = TIZ_CAST_SUCCESS;
  assert (ap_cast_clnt);

  if (clients_.count (*ap_cast_clnt))
    {
      try
        {
          //         client_data & clnt = clients_[*ap_cast_clnt];
          rc = com::aratelia::tiz::tizcastif_proxy::volume (a_volume);
        }
      catch (Tiz::DBus::Error const & e)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "DBus error [%s]...", e.what ());
          rc = TIZ_CAST_DBUS;
        }
      catch (std::exception const & e)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Standard exception error [%s]...",
                   e.what ());
          rc = TIZ_CAST_UNKNOWN;
        }
      catch (...)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Uknonwn exception error...");
          rc = TIZ_CAST_UNKNOWN;
        }
    }
  else
    {
      rc = TIZ_CAST_MISUSE;
      char uuid_str[128];
      tiz_uuid_str (&((*ap_cast_clnt)[0]), uuid_str);
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "Could not find the client with uuid [%s]...", uuid_str);
    }
  return rc;
}

int32_t
tizcastclient::volume_up (const cast_client_id_ptr_t ap_cast_clnt)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::volume_up,
                     ap_cast_clnt);
}

int32_t
tizcastclient::volume_down (const cast_client_id_ptr_t ap_cast_clnt)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::volume_down,
                     ap_cast_clnt);
}

int32_t
tizcastclient::mute (const cast_client_id_ptr_t ap_cast_clnt)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::mute, ap_cast_clnt);
}

int32_t
tizcastclient::unmute (const cast_client_id_ptr_t ap_cast_clnt)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::unmute,
                     ap_cast_clnt);
}

const tizcastclient::cast_client_id_ptr_t
tizcastclient::register_client (const char * ap_device_name_or_ip,
                                const uint8_t uuid[],
                                const tiz_cast_client_callbacks_t * ap_cbacks,
                                void * ap_data)
{
  char uuid_str[128];
  cast_client_id_t uuid_vec;
  uuid_vec.assign (&uuid[0], &uuid[0] + 128);

  tiz_uuid_str (uuid, uuid_str);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "'%s' : Registering client with uuid [%s]...",
           ap_device_name_or_ip, uuid_str);

  std::pair< clients_map_t::iterator, bool > rv = clients_.insert (
    std::make_pair (uuid_vec, client_data (ap_device_name_or_ip, uuid_vec,
                                           ap_cbacks, ap_data)));
  if (rv.second)
    {
      TIZ_LOG (TIZ_PRIORITY_NOTICE,
               "'%s' : Successfully registered client with uuid [%s]...",
               ap_device_name_or_ip, uuid_str);
      return (const cast_client_id_ptr_t) & (rv.first->first);
    }
  TIZ_LOG (TIZ_PRIORITY_ERROR, "Unable to register the clientwith uuid [%s]...",
           uuid_str);
  return NULL;
}

void
tizcastclient::unregister_client (const cast_client_id_ptr_t ap_cast_clnt)
{
  char uuid_str[128];
  assert (ap_cast_clnt);

  tiz_uuid_str (&((*ap_cast_clnt)[0]), uuid_str);

  clients_map_t::iterator it = clients_.find (*ap_cast_clnt);
  if (it != clients_.end ())
    {
      const int32_t cast_err = disconnect (ap_cast_clnt);
      if (TIZ_CAST_SUCCESS != cast_err)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR,
                   "While disconnecting from Tizonia's chromecat daemon");
        }
      // Remove client from internal map
      clients_.erase (it);
      TIZ_LOG (TIZ_PRIORITY_NOTICE, "Removed client with uuid [%s]...",
               uuid_str);
    }
}

void
tizcastclient::cast_status (const uint32_t & status)
{
  const tiz_cast_client_cast_status_t cast_status
    = static_cast< tiz_cast_client_cast_status_t > (status);

  // Notify the client of the cast status

  switch (cast_status)
    {
      case ETizCcCastStatusUnknown:
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "cast status [Unknown]");
        }
        break;
      case ETizCcCastStatusReadyToCast:
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "cast status [ReadyToCast]");
        }
        break;
      case ETizCcCastStatusNowCasting:
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "cast status [NowCasting]");
        }
        break;
      default:
        {
          assert (0);
        }
        break;
    }
  //   if (clients_.count (uuid))
  //     {
  //       uint32_t res = rid;
  //       client_data &data = clients_[uuid];

  //       TIZ_LOG (TIZ_PRIORITY_TRACE, "wait_complete on component  [%s]...",
  //                data.cname_.c_str ());

  //       data.pf_waitend_ (res, data.p_data_);
  //     }
}

void
tizcastclient::media_status (const uint32_t & status)
{
  const tiz_cast_client_media_status_t media_status
    = static_cast< tiz_cast_client_media_status_t > (status);

  // Notify the client of the media status

  switch (media_status)
    {
      case ETizCcMediaStatusUnknown:
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "media status [Unknown]");
        }
        break;
      case ETizCcMediaStatusIdle:
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "media status [Idle]");
        }
        break;
      case ETizCcMediaStatusBuffering:
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "media status [Buffering]");
        }
        break;
      case ETizCcMediaStatusPaused:
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "media status [Paused]");
        }
        break;
      case ETizCcMediaStatusPlaying:
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "media status [Playing]");
        }
        break;
      default:
        {
          assert (0);
        }
        break;
    }

  //   if (clients_.count (uuid))
  //     {
  //       uint32_t res = rid;
  //       client_data &data = clients_[uuid];

  //       TIZ_LOG (TIZ_PRIORITY_TRACE, "wait_complete on component  [%s]...",
  //                data.cname_.c_str ());

  //       data.pf_waitend_ (res, data.p_data_);
  //     }
}

int32_t
tizcastclient::invokecast (pmf_t a_pmf, const cast_client_id_ptr_t ap_cast_clnt)
{
  int32_t rc = TIZ_CAST_SUCCESS;
  assert (a_pmf);
  assert (ap_cast_clnt);

  if (clients_.count (*ap_cast_clnt))
    {
      try
        {
          // client_data & clnt = clients_[*ap_cast_clnt];
          rc = (this->*a_pmf) ();
        }
      catch (Tiz::DBus::Error const & e)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "DBus error [%s]...", e.what ());
          rc = TIZ_CAST_DBUS;
        }
      catch (std::exception const & e)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Standard exception error [%s]...",
                   e.what ());
          rc = TIZ_CAST_UNKNOWN;
        }
      catch (...)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Uknonwn exception error...");
          rc = TIZ_CAST_UNKNOWN;
        }
    }
  else
    {
      rc = TIZ_CAST_MISUSE;
      char uuid_str[128];
      tiz_uuid_str (&((*ap_cast_clnt)[0]), uuid_str);
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "Could not find the client with uuid [%s]...", uuid_str);
    }

  return rc;
}
