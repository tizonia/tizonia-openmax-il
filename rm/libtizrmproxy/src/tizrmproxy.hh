/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   tizrmproxy.hh
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Resource Manager client library
 *
 *
 */

#ifndef TIZRMPROXY_HH
#define TIZRMPROXY_HH

#include <map>
#include <string.h>

#include "tizrmproxy-dbus.hh"
#include "tizrmproxytypes.h"

class tizrmproxy
  : public com::aratelia::tiz::tizrmif_proxy,
    public DBus::IntrospectableProxy,
    public DBus::ObjectProxy
{

public:

  tizrmproxy(DBus::Connection &connection, const char *path, const char *name);

  ~tizrmproxy();

  void *register_client(const char * ap_cname, const uint8_t uuid[],
                        const uint32_t &grp_id, const uint32_t &grp_pri,
                        tizrm_proxy_wait_complete_f apf_waitend,
                        tizrm_proxy_preemption_req_f apf_preempt,
                        tizrm_proxy_preemption_complete_f apf_preempt_end,
                        void * ap_data);

  void unregister_client(const tizrm_t * ap_rm);

  // DBUS Methods

  int32_t acquire(const tizrm_t * ap_rm, const uint32_t &rid,
                  const uint32_t &quantity);

  int32_t release(const tizrm_t * ap_rm, const uint32_t &rid,
                  const uint32_t &quantity);

  int32_t wait(const tizrm_t * ap_rm, const uint32_t &rid,
               const uint32_t &quantity);

  int32_t cancel_wait(const tizrm_t * ap_rm, const uint32_t &rid,
                      const uint32_t &quantity);

  int32_t relinquish_all(const tizrm_t * ap_rm);

  int32_t preemption_conf(const tizrm_t * ap_rm, const uint32_t &rid,
                         const uint32_t &quantity);

private:

  // DBUS Signals

  void wait_complete(const uint32_t &rid, const std::vector< uint8_t >& uuid);

  void preemption_req(const uint32_t &rid, const std::vector< uint8_t >& uuid);

  void preemption_complete(const uint32_t &rid, const std::vector< uint8_t >& uuid);

private:

  struct client_data
  {
    client_data()
      :
      cname_(""),
      uuid_(),
      grp_id_(0),
      pri_(0),
      pf_waitend_(NULL),
      pf_preempt_(NULL),
      pf_preempt_end_(NULL),
      p_data_(NULL)
    {}

    client_data(const char * ap_cname, std::vector<unsigned char> uuid,
                const uint32_t &a_grp_id, const uint32_t &a_pri,
                tizrm_proxy_wait_complete_f apf_waitend,
                tizrm_proxy_preemption_req_f apf_preempt,
                tizrm_proxy_preemption_complete_f apf_preempt_end,
                void * ap_data)
      :
      cname_(ap_cname),
      uuid_(uuid),
      grp_id_(a_grp_id),
      pri_(a_pri),
      pf_waitend_(apf_waitend),
      pf_preempt_(apf_preempt),
      pf_preempt_end_(apf_preempt_end),
      p_data_(ap_data)
    {
    }

    bool operator<(const client_data &rhs) const
    {
      return (uuid_ < rhs.uuid_);
    }

    bool operator==(const client_data &rhs) const
    {
      return (uuid_ == rhs.uuid_);
    }

    // Data members
    std::string cname_;
    std::vector<unsigned char> uuid_;
    uint32_t grp_id_;
    uint32_t pri_;
    tizrm_proxy_wait_complete_f pf_waitend_;
    tizrm_proxy_preemption_req_f pf_preempt_;
    tizrm_proxy_preemption_complete_f pf_preempt_end_;
    void *p_data_;
  };

private:

  typedef std::map<std::vector<unsigned char>,client_data> clients_map_t;

  typedef int32_t (com::aratelia::tiz::tizrmif_proxy::*pmf_t)
  (const uint32_t &, const uint32_t &, const std::string &,
   const std::vector< uint8_t >&, const uint32_t &,
   const uint32_t &);

private:

  int32_t invokerm(pmf_t a_pmf, const tizrm_t * ap_rm, const uint32_t &,
                   const uint32_t &);

  using com::aratelia::tiz::tizrmif_proxy::acquire;
  using com::aratelia::tiz::tizrmif_proxy::release;
  using com::aratelia::tiz::tizrmif_proxy::wait;
  using com::aratelia::tiz::tizrmif_proxy::cancel_wait;
  using com::aratelia::tiz::tizrmif_proxy::preemption_conf;

private:

  clients_map_t clients_;

};

#endif // TIZRMPROXY_HH
