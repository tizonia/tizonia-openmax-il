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
 * @file   tizrmproxy.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Resource Manager client library
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <utility>

#include "tizrmtypes.h"
#include "tizrmproxy.hh"
#include "tizplatform.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.rm.proxy"
#endif

tizrmproxy::tizrmproxy (DBus::Connection &connection, const char *path,
                        const char *name)
  : DBus::ObjectProxy (connection, path, name), clients_ ()
{
}

tizrmproxy::~tizrmproxy ()
{
  // Check if there are clients
}

void *tizrmproxy::register_client (
    const char *ap_cname, const uint8_t uuid[], const uint32_t &grp_id,
    const uint32_t &grp_pri, tiz_rm_proxy_wait_complete_f apf_waitend,
    tiz_rm_proxy_preemption_req_f apf_preempt,
    tiz_rm_proxy_preemption_complete_f apf_preempt_end, void *ap_data)
{
  char uuid_str[128];
  std::vector< unsigned char > uuid_vec;
  uuid_vec.assign (&uuid[0], &uuid[0] + 128);

  std::pair< clients_map_t::iterator, bool > rv
    = clients_.insert (std::make_pair (
      uuid_vec, client_data (ap_cname, uuid_vec, grp_id, grp_pri, apf_waitend,
                             apf_preempt, apf_preempt_end, ap_data)));

  tiz_uuid_str (&(uuid_vec[0]), uuid_str);

  if (true == rv.second)
    {

      TIZ_LOG (TIZ_PRIORITY_TRACE, "'%s' : Registered with uuid [%s]...",
               ap_cname, uuid_str);
      return (void *)&(rv.first->first);
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Could not register client with uuid [%s]...",
           uuid_str);

  return NULL;
}

void tizrmproxy::unregister_client (const tiz_rm_t *ap_rm)
{
  int32_t rc = TIZ_RM_SUCCESS;
  char uuid_str[128];
  assert (ap_rm);
  const std::vector< unsigned char > *p_uuid_vec
      = static_cast< std::vector< unsigned char > * >(*ap_rm);
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
      TIZ_LOG (TIZ_PRIORITY_TRACE,
               "Relinquishing rm resources "
               "for client with uuid [%s]...",
               uuid_str);
      rc = relinquish_all (ap_rm);
      // Remove client from internal map
      clients_.erase (it);
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Unregistered client with uuid [%s]...rc [%d]",
           uuid_str, rc);
}

int32_t tizrmproxy::acquire (const tiz_rm_t *ap_rm, const uint32_t &rid,
                             const uint32_t &quantity)
{
  return invokerm (&com::aratelia::tiz::tizrmif_proxy::acquire, ap_rm, rid,
                   quantity);
}

int32_t tizrmproxy::release (const tiz_rm_t *ap_rm, const uint32_t &rid,
                             const uint32_t &quantity)
{
  return invokerm (&com::aratelia::tiz::tizrmif_proxy::release, ap_rm, rid,
                   quantity);
}

int32_t tizrmproxy::wait (const tiz_rm_t *ap_rm, const uint32_t &rid,
                          const uint32_t &quantity)
{
  return invokerm (&com::aratelia::tiz::tizrmif_proxy::wait, ap_rm, rid,
                   quantity);
}

int32_t tizrmproxy::cancel_wait (const tiz_rm_t *ap_rm, const uint32_t &rid,
                                 const uint32_t &quantity)
{
  return invokerm (&com::aratelia::tiz::tizrmif_proxy::cancel_wait, ap_rm, rid,
                   quantity);
}

int32_t tizrmproxy::relinquish_all (const tiz_rm_t *ap_rm)
{
  int32_t rc = TIZ_RM_SUCCESS;
  assert (ap_rm);
  const std::vector< unsigned char > *p_uuid_vec
      = static_cast< std::vector< unsigned char > * >(*ap_rm);
  assert (p_uuid_vec);

  if (clients_.count (*p_uuid_vec))
    {
      try
      {
        client_data &clnt = clients_[*p_uuid_vec];
        rc = com::aratelia::tiz::tizrmif_proxy::relinquish_all (clnt.cname_,
                                                                *p_uuid_vec);
      }
      catch (DBus::Error const &e)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "DBus error [%s]...", e.what ());
        rc = TIZ_RM_DBUS;
      }
      catch (std::exception const &e)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "Standard exception error [%s]...",
                 e.what ());
        rc = TIZ_RM_UNKNOWN;
      }
      catch (...)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "Uknonwn exception error...");
        rc = TIZ_RM_UNKNOWN;
      }
    }
  else
    {
      rc = TIZ_RM_MISUSE;
      char uuid_str[128];
      tiz_uuid_str (&((*p_uuid_vec)[0]), uuid_str);
      TIZ_LOG (TIZ_PRIORITY_TRACE,
               "Could not find the client with uuid [%s]...", uuid_str);
    }

  return rc;
}

int32_t tizrmproxy::preemption_conf (const tiz_rm_t *ap_rm, const uint32_t &rid,
                                     const uint32_t &quantity)
{
  return invokerm (&com::aratelia::tiz::tizrmif_proxy::preemption_conf, ap_rm,
                   rid, quantity);
}

void tizrmproxy::wait_complete (const uint32_t &rid,
                                const std::vector< uint8_t > &uuid)
{
  char uuid_str[128];
  tiz_uuid_str (&(uuid[0]), uuid_str);

  // Notify the client component that the wait for resources has ended

  TIZ_LOG (TIZ_PRIORITY_TRACE, "wait_complete on uuid [%s]...", uuid_str);

  if (clients_.count (uuid))
    {
      uint32_t res = rid;
      client_data &data = clients_[uuid];

      TIZ_LOG (TIZ_PRIORITY_TRACE, "wait_complete on component  [%s]...",
               data.cname_.c_str ());

      data.pf_waitend_ (res, data.p_data_);
    }
}

void tizrmproxy::preemption_req (const uint32_t &rid,
                                 const std::vector< uint8_t > &uuid)
{
  char uuid_str[128];
  tiz_uuid_str (&(uuid[0]), uuid_str);

  // Notify the owning component that a resource has been preempted

  TIZ_LOG (TIZ_PRIORITY_TRACE, "preemption_req on uuid [%s]...", uuid_str);

  if (clients_.count (uuid))
    {
      uint32_t res = rid;
      client_data &data = clients_[uuid];

      TIZ_LOG (TIZ_PRIORITY_TRACE, "preemption_req on component [%s]...",
               data.cname_.c_str ());

      data.pf_preempt_ (res, data.p_data_);
    }
}

void tizrmproxy::preemption_complete (const uint32_t &rid,
                                      const std::vector< uint8_t > &uuid)
{
  char uuid_str[128];
  tiz_uuid_str (&(uuid[0]), uuid_str);

  // Notify the owning component that a resource has been preempted

  TIZ_LOG (TIZ_PRIORITY_TRACE, "preemption_complete on uuid [%s]...", uuid_str);

  if (clients_.count (uuid))
    {
      uint32_t res = rid;
      client_data &data = clients_[uuid];

      TIZ_LOG (TIZ_PRIORITY_TRACE, "preemption_complete on component [%s]...",
               data.cname_.c_str ());

      data.pf_preempt_end_ (res, data.p_data_);
    }
}

int32_t tizrmproxy::invokerm (pmf_t a_pmf, const tiz_rm_t *ap_rm,
                              const uint32_t &rid, const uint32_t &quantity)
{
  int32_t rc = TIZ_RM_SUCCESS;
  assert (ap_rm);
  const std::vector< unsigned char > *p_uuid_vec
      = static_cast< std::vector< unsigned char > * >(*ap_rm);
  assert (p_uuid_vec);

  assert (a_pmf);

  if (clients_.count (*p_uuid_vec))
    {
      try
      {
        client_data &clnt = clients_[*p_uuid_vec];
        rc = (this->*a_pmf)(rid, quantity, clnt.cname_, *p_uuid_vec,
                            clnt.grp_id_, clnt.pri_);
      }
      catch (DBus::Error const &e)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "DBus error [%s]...", e.what ());
        rc = TIZ_RM_DBUS;
      }
      catch (std::exception const &e)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "Standard exception error [%s]...",
                 e.what ());
        rc = TIZ_RM_UNKNOWN;
      }
      catch (...)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "Uknonwn exception error...");
        rc = TIZ_RM_UNKNOWN;
      }
    }
  else
    {
      rc = TIZ_RM_MISUSE;
      char uuid_str[128];
      tiz_uuid_str (&((*p_uuid_vec)[0]), uuid_str);
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "Could not find the client with uuid [%s]...", uuid_str);
    }

  return rc;
}
