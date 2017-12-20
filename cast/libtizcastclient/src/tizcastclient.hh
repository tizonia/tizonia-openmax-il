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
           tiz_cast_client_url_loaded_f apf_url_loaded, void * ap_data);

  int32_t
  disconnect (const cast_client_id_ptr_t ap_cast);

  int32_t
  load_url (const cast_client_id_ptr_t ap_cast_clnt, const char * url,
            const char * mime_type, const char * title);

  int32_t
  play (const cast_client_id_ptr_t ap_cast_clnt);

  int32_t
  stop (const cast_client_id_ptr_t ap_cast_clnt);

  int32_t
  pause (const cast_client_id_ptr_t ap_cast_clnt);

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
                   tiz_cast_client_url_loaded_f apf_url_loaded, void * ap_data);

  void
  unregister_client (const cast_client_id_ptr_t ap_cast_clnt);

  // DBUS Signals
  void
  url_loaded ();

private:
  struct client_data
  {
    client_data ()
      : cname_ (""), uuid_ (), pf_url_loaded_ (NULL), p_data_ (NULL)
    {
    }

    client_data (const char * ap_cname, std::vector< unsigned char > uuid,
                 tiz_cast_client_url_loaded_f apf_url_loaded, void * ap_data)
      : cname_ (ap_cname),
        uuid_ (uuid),
        pf_url_loaded_ (apf_url_loaded),
        p_data_ (ap_data)
    {
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
    tiz_cast_client_url_loaded_f pf_url_loaded_;
    void * p_data_;
  };

private:
  typedef std::map< std::vector< unsigned char >, client_data > clients_map_t;

  typedef int32_t (com::aratelia::tiz::tizcastif_proxy::*pmf_t) ();

private:
  int32_t
  invokecast (pmf_t a_pmf, const cast_client_id_ptr_t ap_cast_clnt);

  using com::aratelia::tiz::tizcastif_proxy::load_url;
  using com::aratelia::tiz::tizcastif_proxy::play;
  using com::aratelia::tiz::tizcastif_proxy::stop;
  using com::aratelia::tiz::tizcastif_proxy::pause;
  using com::aratelia::tiz::tizcastif_proxy::volume_up;
  using com::aratelia::tiz::tizcastif_proxy::volume_down;
  using com::aratelia::tiz::tizcastif_proxy::mute;
  using com::aratelia::tiz::tizcastif_proxy::unmute;

private:
  clients_map_t clients_;
};

#endif  // TIZCASTCLIENT_HH
