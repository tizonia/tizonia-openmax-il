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
 * @brief  Tizonia OpenMAX IL - Chromecast Daemon client library library
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
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.client"
#endif

tizcastclient::tizcastclient (DBus::Connection & connection, const char * path,
                              const char * name)
  : DBus::ObjectProxy (connection, path, name), clients_ ()
{
}

tizcastclient::~tizcastclient ()
{
  // Check if there are clients
}

void *
tizcastclient::register_client (const char * ap_cname, const uint8_t uuid[],
                                tiz_cast_client_url_loaded_f apf_url_loaded,
                                void * ap_data)
{
  char uuid_str[128];
  std::vector< unsigned char > uuid_vec;
  uuid_vec.assign (&uuid[0], &uuid[0] + 128);

  std::pair< clients_map_t::iterator, bool > rv
    = clients_.insert (std::make_pair (
      uuid_vec, client_data (ap_cname, uuid_vec, apf_url_loaded, ap_data)));

  tiz_uuid_str (&(uuid_vec[0]), uuid_str);

  if (true == rv.second)
    {

      TIZ_LOG (TIZ_PRIORITY_TRACE, "'%s' : Registered with uuid [%s]...",
               ap_cname, uuid_str);
      return (void *) &(rv.first->first);
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Could not register client with uuid [%s]...",
           uuid_str);

  return NULL;
}

void
tizcastclient::unregister_client (const tiz_cast_t * ap_cast)
{
  int32_t rc = TIZ_CAST_SUCCESS;
  char uuid_str[128];
  assert (ap_cast);
  const std::vector< unsigned char > * p_uuid_vec
    = static_cast< std::vector< unsigned char > * > (*ap_cast);
  assert (p_uuid_vec);

  tiz_uuid_str (&((*p_uuid_vec)[0]), uuid_str);

  TIZ_LOG (TIZ_PRIORITY_TRACE,
           "Unregistering "
           "client with uuid [%s]...",
           uuid_str);

  clients_map_t::iterator it = clients_.find (*p_uuid_vec);
  if (it != clients_.end ())
    {
      // Release all resources currently allocated with the RM and cancel all
      // outstanding resource requests
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Removing client with uuid [%s]...",
               uuid_str);
      // Remove client from internal map
      clients_.erase (it);
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Unregistered client with uuid [%s]...rc [%d]",
           uuid_str, rc);
}

int32_t
tizcastclient::load_url (const tiz_cast_t * ap_cast, const char * url,
                         const char * mime_type, const char * title)
{
  int32_t rc = TIZ_CAST_SUCCESS;
  assert (ap_cast);
  const std::vector< unsigned char > * p_uuid_vec
    = static_cast< std::vector< unsigned char > * > (*ap_cast);
  assert (p_uuid_vec);

  if (clients_.count (*p_uuid_vec))
    {
      try
        {
          //         client_data & clnt = clients_[*p_uuid_vec];
          rc = com::aratelia::tiz::tizcastif_proxy::load_url (url, mime_type,
                                                              title);
        }
      catch (DBus::Error const & e)
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
      tiz_uuid_str (&((*p_uuid_vec)[0]), uuid_str);
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "Could not find the client with uuid [%s]...", uuid_str);
    }
  return rc;
}

int32_t
tizcastclient::play (const tiz_cast_t * ap_cast)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::play, ap_cast);
}

int32_t
tizcastclient::stop (const tiz_cast_t * ap_cast)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::stop, ap_cast);
}

int32_t
tizcastclient::pause (const tiz_cast_t * ap_cast)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::pause, ap_cast);
}

int32_t
tizcastclient::volume_up (const tiz_cast_t * ap_cast)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::volume_up, ap_cast);
}

int32_t
tizcastclient::volume_down (const tiz_cast_t * ap_cast)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::volume_down,
                     ap_cast);
}

int32_t
tizcastclient::mute (const tiz_cast_t * ap_cast)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::mute, ap_cast);
}

int32_t
tizcastclient::unmute (const tiz_cast_t * ap_cast)
{
  return invokecast (&com::aratelia::tiz::tizcastif_proxy::unmute, ap_cast);
}

void tizcastclient::url_loaded ()
{
  // Notify the client that the url has been loaded

  TIZ_LOG (TIZ_PRIORITY_TRACE, "url loaded...");

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
tizcastclient::invokecast (pmf_t a_pmf, const tiz_cast_t * ap_cast)
{
  int32_t rc = TIZ_CAST_SUCCESS;
  assert (ap_cast);
  const std::vector< unsigned char > * p_uuid_vec
    = static_cast< std::vector< unsigned char > * > (*ap_cast);
  assert (p_uuid_vec);

  assert (a_pmf);

  if (clients_.count (*p_uuid_vec))
    {
      try
        {
          // client_data & clnt = clients_[*p_uuid_vec];
          rc = (this->*a_pmf) ();
        }
      catch (DBus::Error const & e)
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
      tiz_uuid_str (&((*p_uuid_vec)[0]), uuid_str);
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "Could not find the client with uuid [%s]...", uuid_str);
    }

  return rc;
}
