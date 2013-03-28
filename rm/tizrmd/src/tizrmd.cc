/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
 * @file   tizrmd.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Resource Manager Daemon implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include <algorithm>

#include "tizrmd.hh"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.rm.daemon"
#endif

// Bus name
static const char *TIZ_RM_DAEMON_NAME = "com.aratelia.tiz.tizrmd";

// Object path, a.k.a. node
static const char *TIZ_RM_DAEMON_PATH = "/com/aratelia/tiz/tizrmd";

tizrmd::tizrmd(DBus::Connection &a_connection, char const * ap_dbname)
  :
  DBus::ObjectAdaptor(a_connection, TIZ_RM_DAEMON_PATH),
  rmdb_(ap_dbname),
  waiters_()
{
  TIZ_LOG(TIZ_TRACE, "Constructing tizrmd...");
  rmdb_.connect();
}

tizrmd::~tizrmd()
{
  rmdb_.disconnect();
}

int32_t
tizrmd::acquire(const uint32_t &rid, const uint32_t &quantity,
                const std::string &cname, const std::vector< uint8_t > &uuid,
                const uint32_t& grpid, const uint32_t &pri)
{
  tizrm_error_t rc = TIZRM_SUCCESS;
  TIZ_LOG(TIZ_TRACE, "tizrmd::acquire : '%s': acquiring rid [%d] -"
            "quantity [%d] - grpid [%d] - pri [%d]...",
            cname.c_str(), rid, quantity, grpid, pri);

  // Reserve the resources now
  if (TIZRM_SUCCESS
      != (rc = rmdb_.acquire_resource(rid, quantity, cname, uuid, grpid, pri)))
    {
      TIZ_LOG(TIZ_TRACE, "tizrmd::acquire : '%s': "
                "Could not reserve [%d] "
                "units of resource [%d]", cname.c_str(), quantity, rid);

      if (TIZRM_NOT_ENOUGH_RESOURCE_AVAILABLE == rc)
        {
          tizrm_owners_list_t owners;
          tizrm_error_t fo_rc = TIZRM_SUCCESS;
          if (TIZRM_SUCCESS
              != (fo_rc = rmdb_.find_owners(rid, pri, owners)))
            {
              return fo_rc;
            }

          if (!owners.empty())
            {
              tizrm_owners_list_t::reverse_iterator rev_it  = owners.rbegin();
              tizrm_owners_list_t::reverse_iterator rend_it = owners.rend();
              uint32_t preemption_counter  = 0;
              uint32_t preemption_quantity = 0;
              tizrm_owners_list_t *p_signaled_owners
                = new tizrm_owners_list_t();

              while (rev_it != rend_it)
                {
                  tizrmowner &owner = *rev_it;
                  preemption_quantity += owner.quantity_;
                  preemption_counter++;
                  p_signaled_owners->push_back(owner);
                  if (preemption_quantity >= quantity)
                    {
                      // We had enough
                      break;
                    }
                }

              if (preemption_quantity >= quantity)
                {

                  // Notify the owners of the imminent preemption

                  rev_it  = owners.rbegin();
                  for (int i=0 ; i< preemption_counter; ++i)
                    {
                      tizrmowner &cur_owner = *rev_it++;
                      char uuid_str[129];
                      tiz_uuid_str(&(cur_owner.uuid_[0]), uuid_str);

                      TIZ_LOG(TIZ_TRACE, "tizrmd::acquire : Notifying "
                                "'%s' (uuid [%s]) of resource preemption "
                                "([%d] units of resource id [%d])",
                                cur_owner.cname_.c_str(), uuid_str,
                                cur_owner.quantity_,
                                rid);

                      // ... signal the owner component
                      preemption_req(cur_owner.rid_, cur_owner.uuid_);

                      // Store the owners info for when the acks are received
                      // TODO: Check rc
                      preemptions_.insert
                        (std::make_pair<tizrmowner, tizrmpreemptor>
                         (cur_owner, tizrmpreemptor(tizrmowner
                                                    (cname, uuid, grpid, pri,
                                                     rid, quantity),
                                                    p_signaled_owners)));

                      rc = TIZRM_PREEMPTION_IN_PROGRESS;

                    }
                }
              else
                {
                  delete p_signaled_owners;
                  p_signaled_owners = NULL;
                }
            }
          else
            {
              TIZ_LOG(TIZ_TRACE, "tizrmd::acquire : "
                        "No owners found with priority > [%d] - rid [%d] - "
                        "quantity [%d]", pri, rid, quantity);
            }
        }
    }

  return rc;

}

int32_t
tizrmd::release(const uint32_t &rid, const uint32_t &quantity,
                const std::string &cname,
                const std::vector< uint8_t > &uuid,
                const uint32_t &grpid, const uint32_t &pri)
{
  tizrm_error_t ret_val = TIZRM_SUCCESS;
  TIZ_LOG(TIZ_TRACE, "tizrmd::release : '%s': releasing rid [%d] - "
            "quantity [%d]", cname.c_str(), rid, quantity);

  // Release the resources now...
  if (TIZRM_SUCCESS
      != (ret_val = rmdb_.release_resource(rid, quantity, cname, uuid,
                                           grpid, pri)))
    {
      TIZ_LOG(TIZ_TRACE, "tizrmd::release : "
                "'%s': Could not release [%d] "
                "units of resource [%d]", cname.c_str(), quantity, rid);
      return ret_val;
    }

  // Find a waiter who might want this resource...
  waitlist_t::iterator it_begin = waiters_.begin();
  waitlist_t::iterator it_end   = waiters_.end();
  waitlist_t::iterator it_next  = it_end;
  uint32_t rid_next = 0;
  for (waitlist_t::iterator it = it_begin; it != it_end ; ++it)
    {
      tizrmwaiter& waiter = *it;
      if (waiter.resid() == rid
          && rmdb_.resource_available(rid, waiter.quantity()))
        {
          ret_val = (tizrm_error_t)acquire(rid, quantity, waiter.cname(),
                                           waiter.uuid(),
                                           waiter.grpid(),
                                           waiter.pri());

          if (TIZRM_SUCCESS == ret_val)
            {
              // Signal the waiter
              TIZ_LOG(TIZ_TRACE, "tizrmd::release : "
                        "signalling waiter [%s] rid [%d] - "
                        "quantity [%d]", waiter.cname().c_str(),
                        rid, quantity);

              wait_complete(rid, waiter.uuid());

              // ... and remove it from the list
              waiters_.erase(it);
            }
          break;
        }
    }

  return ret_val;
}

int32_t
tizrmd::wait(const uint32_t &rid, const uint32_t &quantity,
             const std::string &cname, const std::vector< uint8_t > &uuid,
             const uint32_t &grpid, const uint32_t &pri)
{
  tizrm_error_t ret_val = TIZRM_SUCCESS;

  TIZ_LOG(TIZ_TRACE, "'%s': waiting for rid [%d] - "
            "quantity [%d]", cname.c_str(), rid, quantity);

  // Check that the component is provisioned and is allowed to access the
  // resource
  if (!rmdb_.comp_provisioned_with_resid(cname, rid))
    {
      TIZ_LOG(TIZ_TRACE, "tizrmd::wait : "
                "'%s': not provisioned...", cname.c_str());
      return TIZRM_COMPONENT_NOT_PROVISIONED;
    }

  // Check that the requested resource is provisioned...
  if (!rmdb_.resource_provisioned(rid))
    {
      TIZ_LOG(TIZ_TRACE, "tizrmd::wait : "
                "Resource [%d] not provisioned...", rid);
      return TIZRM_RESOURCE_NOT_PROVISIONED;
    }

  //...and that there isn't availability...
  if (rmdb_.resource_available(rid, quantity))
    {
      TIZ_LOG(TIZ_TRACE, "tizrmd::wait : "
                "Enough resource [%d] already available ...",
                rid);

      if (TIZRM_SUCCESS
          != (ret_val = rmdb_.acquire_resource(rid, quantity, cname,
                                               uuid, grpid, pri)))
        {
          TIZ_LOG(TIZ_TRACE, "tizrmd::wait : "
                    "'%s': Could not reserve [%d] "
                    "units of resource [%d]", cname.c_str(),
                    quantity, rid);
          return ret_val;
        }

      return TIZRM_WAIT_COMPLETE;
    }

  // No preemption occurs at this point. Preemption can only happen if the
  // client calls acquire

  TIZ_LOG(TIZ_TRACE, "tizrmd::wait : "
            "'%s' : Added to the waiting list", cname.c_str());

  // Now, add a waiter to the queue...
  waiters_.push_back(tizrmwaiter(rid, quantity, cname, uuid, grpid, pri));

  return TIZRM_SUCCESS;
}

int32_t
tizrmd::cancel_wait(const uint32_t &rid, const uint32_t &quantity,
                    const std::string &cname,
                    const std::vector< uint8_t > &uuid,
                    const uint32_t &grpid, const uint32_t &pri)
{
  // Remove the waiter from the queue...

  TIZ_LOG(TIZ_TRACE, "tizrmd::cancel_wait : "
            "'%s': Cancelling wait for [%d] "
            "units of resource [%d] - waiters [%d]",
            cname.c_str(), quantity, rid, waiters_.size());

  //std::remove_if(waiters_.begin(), waiters_.end(), remove_waiter_functor(uuid));

  waitlist_t::iterator it_begin = waiters_.begin();
  waitlist_t::iterator it_end = waiters_.end();
  for (waitlist_t::iterator it = it_begin; it != it_end; ++it)
    {
      if (it->uuid() == uuid)
        {
          waiters_.erase(it);
        }
    }

  TIZ_LOG(TIZ_TRACE, "tizrmd::cancel_wait : "
            "'%s': Cancelled wait for [%d] "
            "units of resource [%d] - waiters [%d]",
            cname.c_str(), quantity, rid, waiters_.size());

  return TIZRM_SUCCESS;
}

int32_t
tizrmd::preemption_conf(const uint32_t &rid, const uint32_t &quantity,
                       const std::string &cname,
                       const std::vector< uint8_t > &uuid,
                       const uint32_t &grpid, const uint32_t &pri)
{
  tizrm_error_t ret_val = TIZRM_SUCCESS;
  preemptlist_t::iterator it
    = preemptions_.find(tizrmowner(cname, uuid, grpid, pri,
                                   rid, quantity));

  TIZ_LOG(TIZ_TRACE, "tizrmd::preemption_conf : "
            "'%s': resource id [%d] "
            "units of resource [%d]", cname.c_str(), rid, quantity);

  if (it != preemptions_.end())
    {
      // TODO: Sanity check the resource information received

      const tizrmowner &cur_owner = it->first;
      const tizrmowner &future_owner = it->second.preemptor_;
      tizrm_owners_list_t *p_cur_owners = it->second.p_owners_;

      // Release the resource...
      if (TIZRM_SUCCESS
          != (ret_val = rmdb_.release_resource(rid, quantity, cname, uuid,
                                               grpid, pri)))
        {
          TIZ_LOG(TIZ_TRACE, "tizrmd::preemption_conf : "
                    "'%s': Could not release [%d] "
                    "units of resource [%d]", cname.c_str(), quantity, rid);
          return ret_val;
        }

      // Check if this is the last owner to release the resource
      p_cur_owners->remove(cur_owner);
      if (p_cur_owners->empty())
        {

          delete p_cur_owners;
          //... now allocate the resource on behalf of the new owner
          if (TIZRM_SUCCESS
              != (ret_val
                  = rmdb_.acquire_resource(future_owner.rid_,
                                           future_owner.quantity_,
                                           future_owner.cname_,
                                           future_owner.uuid_,
                                           future_owner.grpid_,
                                           future_owner.pri_)))
            {
              TIZ_LOG(TIZ_TRACE, "tizrmd::preemption_conf : "
                        "'%s': Could not reserve [%d] "
                        "units of resource [%d]", future_owner.cname_.c_str(),
                        future_owner.quantity_, future_owner.rid_);
            }

          // ... and now signal the new owner...
          TIZ_LOG(TIZ_TRACE, "tizrmd::preemption_conf : "
                    "'%s': signalling completion - resource id [%d] "
                    "units of resource [%d]", future_owner.cname_.c_str(),
                    future_owner.rid_, future_owner.quantity_);

          preemption_complete(future_owner.rid_, future_owner.uuid_);
        }

      // Remove owner from the preemption list
      preemptions_.erase(it);

    }
  else
    {
      return TIZRM_MISUSE;
    }

  return TIZRM_SUCCESS;
}

int32_t
tizrmd::relinquish_all(const std::string &cname,
                       const std::vector<unsigned char> &uuid)
{
  tizrm_error_t ret_val = TIZRM_SUCCESS;

  TIZ_LOG(TIZ_TRACE, "tizrmd::relinquish_all: '%s' : "
            "Releasing all resources and resource requests",
            cname.c_str());

  // Release the resources now...
  ret_val = rmdb_.release_all(cname, uuid);

  TIZ_LOG(TIZ_TRACE, "'%s' : Released all resources - rc [%d]",
            cname.c_str(), ret_val);

  std::remove_if(waiters_.begin(), waiters_.end(), remove_waiter_functor(uuid));

  return ret_val;
}

DBus::BusDispatcher dispatcher;

void
tizrmd_sig_hdlr(int sig)
{
  dispatcher.leave();
  TIZ_LOG(TIZ_TRACE, "Tizonia IL RM daemon exiting...");
}

bool
find_rmdb_path(std::string & a_dbpath)
{
  bool rv = false;
  tiz_rcfile_t *p_rcfile = NULL;
  const char *p_rm_enabled = NULL;
  const char *p_rmdb_path = NULL;

  tiz_rcfile_open(&p_rcfile);

  p_rm_enabled = tiz_rcfile_get_value(p_rcfile, "resource-management",
                                     "enabled");
  p_rmdb_path = tiz_rcfile_get_value(p_rcfile, "resource-management",
                                     "rmdb");

  if (!p_rm_enabled || !p_rmdb_path
      || (0 != strncmp (p_rm_enabled, "true", 4)))
    {
      TIZ_LOG(TIZ_TRACE, "RM is disabled...");
    }
  else
    {
      TIZ_LOG(TIZ_TRACE, "RM db path [%s]...", p_rmdb_path);
      a_dbpath.assign(p_rmdb_path);
      rv = true;
    }

  tiz_rcfile_close(p_rcfile);

  return rv;
}

int
main()
{
  std::string rmdb_path;
  signal(SIGTERM, tizrmd_sig_hdlr);
  signal(SIGINT, tizrmd_sig_hdlr);

  tiz_log_init();

  TIZ_LOG(TIZ_TRACE, "Starting Tizonia IL RM daemon...");

  if (find_rmdb_path(rmdb_path))
    {
      DBus::default_dispatcher = &dispatcher;

      DBus::Connection conn = DBus::Connection::SessionBus();
      conn.request_name(TIZ_RM_DAEMON_NAME);

      tizrmd server(conn, rmdb_path.c_str());

      dispatcher.enter();

    }

  tiz_log_deinit ();

  return 0;
}
