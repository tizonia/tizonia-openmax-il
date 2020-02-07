/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   tizcastclient.hh
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL Chromecast daemon - client library (c++ api)
 *
 */

#ifndef TIZCASTCLIENT_HH
#define TIZCASTCLIENT_HH

#include <map>
#include <string.h>

#include <tizcastclient-dbus.hh>

#include "tizcastclienttypes.h"

class tizcastclient : public com::aratelia::tiz::tizcastif_proxy,
                      public Tiz::DBus::IntrospectableProxy,
                      public Tiz::DBus::ObjectProxy
{
public:
  typedef std::vector< unsigned char > cast_client_id_t;
  typedef cast_client_id_t * cast_client_id_ptr_t;

public:
  tizcastclient (Tiz::DBus::Connection & connection, const char * path,
                 const char * name);

  ~tizcastclient ();

  // DBUS Methods

  const cast_client_id_ptr_t
  connect (const char * ap_device_name_or_ip, const uint8_t uuid[],
           const tiz_cast_client_callbacks_t * ap_cbacks, void * ap_data);

  int32_t
  disconnect (const cast_client_id_ptr_t ap_cast);

  int32_t
  load_url (const cast_client_id_ptr_t ap_cast_clnt, const char * url,
            const char * mime_type, const char * title, const char * album_art);

  int32_t
  play (const cast_client_id_ptr_t ap_cast_clnt);

  int32_t
  stop (const cast_client_id_ptr_t ap_cast_clnt);

  int32_t
  pause (const cast_client_id_ptr_t ap_cast_clnt);

  int32_t
  volume_set (const cast_client_id_ptr_t ap_cast_clnt, int a_volume);

  int32_t
  volume_get (const cast_client_id_ptr_t ap_cast_clnt, int * ap_volume);

  int32_t
  volume_up (const cast_client_id_ptr_t ap_cast_clnt);

  int32_t
  volume_down (const cast_client_id_ptr_t ap_cast_clnt);

  int32_t
  mute (const cast_client_id_ptr_t ap_cast_clnt);

  int32_t
  unmute (const cast_client_id_ptr_t ap_cast_clnt);

private:
  const cast_client_id_ptr_t
  register_client (const char * ap_device_name_or_ip, const uint8_t uuid[],
                   const tiz_cast_client_callbacks_t * ap_cbacks,
                   void * ap_data);

  void
  unregister_client (const cast_client_id_ptr_t ap_cast_clnt);

  void unregister_all_clients();

  // DBUS Signals
  void
  cast_status (const std::vector< uint8_t > & uuid, const uint32_t & status,
               const int32_t & volume);
  void
  media_status (const std::vector< uint8_t > & uuid, const uint32_t & status,
                const int32_t & volume);

  void
  error_status (const std::vector< uint8_t > & uuid, const uint32_t & status,
                const std::string & error_msg);

private:
  struct client_data
  {
    client_data () : cname_ (""), uuid_ (), p_data_ (NULL)
    {
      cbacks_.pf_cast_status = NULL;
      cbacks_.pf_media_status = NULL;
    }

    client_data (const char * ap_cname, std::vector< unsigned char > uuid,
                 const tiz_cast_client_callbacks_t * ap_cbacks, void * ap_data)
      : cname_ (ap_cname), uuid_ (uuid), p_data_ (ap_data)
    {
      assert (ap_cbacks);
      if (ap_cbacks)
        {
          cbacks_.pf_cast_status = ap_cbacks->pf_cast_status;
          cbacks_.pf_media_status = ap_cbacks->pf_media_status;
          cbacks_.pf_error_status = ap_cbacks->pf_error_status;
        }
    }

    bool
    operator< (const client_data & rhs) const
    {
      return (uuid_ < rhs.uuid_);
    }

    bool
    operator== (const client_data & rhs) const
    {
      return (uuid_ == rhs.uuid_);
    }

    // Data members
    std::string cname_;
    std::vector< unsigned char > uuid_;
    void * p_data_;
    tiz_cast_client_callbacks_t cbacks_;
  };

private:
  typedef std::map< cast_client_id_t, client_data > clients_map_t;

  typedef int32_t (com::aratelia::tiz::tizcastif_proxy::*pmf_t) (
    const cast_client_id_t &);

private:
  int32_t
  invokecast (pmf_t a_pmf, const cast_client_id_ptr_t ap_cast_clnt);

  using com::aratelia::tiz::tizcastif_proxy::load_url;
  using com::aratelia::tiz::tizcastif_proxy::mute;
  using com::aratelia::tiz::tizcastif_proxy::pause;
  using com::aratelia::tiz::tizcastif_proxy::play;
  using com::aratelia::tiz::tizcastif_proxy::stop;
  using com::aratelia::tiz::tizcastif_proxy::unmute;
  using com::aratelia::tiz::tizcastif_proxy::volume_down;
  using com::aratelia::tiz::tizcastif_proxy::volume_set;
  using com::aratelia::tiz::tizcastif_proxy::volume_up;

private:
  clients_map_t clients_;
};

#endif  // TIZCASTCLIENT_HH
