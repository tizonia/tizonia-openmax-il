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
 * @file   tizrmproxy_c.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Resource Manager client library (c wrapper impl)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizrmproxy_c.h"
#include "tizrmproxy.hh"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.rm.proxy_c"
#endif

/* Bus name */
#define TIZ_RM_DAEMON_NAME "com.aratelia.tiz.tizrmd"
/* Object path, a.k.a. node */
#define TIZ_RM_DAEMON_PATH "/com/aratelia/tiz/tizrmd"

enum tizrm_state
  {
    ETIZRmStateInvalid = 0,
    ETIZRmStateStarting,
    ETIZRmStateStarted,
    ETIZRmStateStopped
  };

typedef enum tizrm_state tizrm_state_t;

struct tizrm
{
  tiz_thread_t thread;
  tiz_sem_t sem;
  tiz_queue_t *p_queue;
  tizrm_error_t error;
  tizrm_state_t state;
  OMX_S32 ref_count;
  DBus::DefaultTimeout *p_dbustimeout;
  DBus::BusDispatcher *p_dispatcher;
  DBus::Connection *p_connection;
  tizrmproxy *p_proxy;
};

typedef struct tizrm tizrm_int_t;

static inline tizrm_int_t* get_rm();

static void *
il_rmproxy_thread_func(void *p_arg)
{
  tizrm_int_t *p_rm = (tizrm_int_t*)(p_arg);

  TIZ_LOG(TIZ_LOG_TRACE, "p_rm [%08X]", p_rm);

  assert(p_rm);

  TIZ_LOG(TIZ_LOG_TRACE, "Entering the dispatcher...");

  p_rm->p_dispatcher->enter();


  TIZ_LOG(TIZ_LOG_TRACE, "Have left the dispatcher, thread exiting...");

  return NULL;
}

static inline tizrm_int_t*
get_rm()
{
  static tizrm_int_t *p_rm = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  /* For now, don't bother about thread safety of initialization of this */
  /* singleton. The current assumption is that this RM proxy will be */
  /* initialized and deinitialized by the IL Core at init time. */

  /* TODO: Fix error handling!! */

  if(!p_rm)
    {
      p_rm = (tizrm_int_t*)
        tiz_mem_calloc(1, sizeof(tizrm_int_t));

      if (!p_rm)
        {
          return NULL;
        }

      TIZ_LOG(TIZ_LOG_TRACE, "Initializing rm [%p]...", p_rm);

      p_rm->p_proxy = NULL;

      if (OMX_ErrorNone
          != (rc = tiz_sem_init(&(p_rm->sem), 0)))
        {
          TIZ_LOG(TIZ_LOG_TRACE, "Error Initializing rm...");
          return NULL;
        }

      if (OMX_ErrorNone
          != (rc = tiz_queue_init(&(p_rm->p_queue), 10)))
        {
          return NULL;
        }

      p_rm->error = TIZRM_SUCCESS;
      p_rm->state = ETIZRmStateStarting;
      p_rm->ref_count = 0;

      TIZ_LOG(TIZ_LOG_TRACE, "Initialization success...");

      TIZ_LOG(TIZ_LOG_TRACE, "Starting IL RM proxy's thread [%p]...",
                p_rm);

      assert(p_rm);

    }

  return (rc == OMX_ErrorNone) ? p_rm : NULL;

}

static tizrm_error_t
stop_proxy()
{
  tizrm_int_t *p_rm = get_rm();
  OMX_PTR p_result = NULL;
  assert(p_rm);

  TIZ_LOG(TIZ_LOG_TRACE, "Stopping proxy's thread");

  tiz_thread_join(&(p_rm->thread), &p_result);

  delete p_rm->p_proxy;
  p_rm->p_proxy = NULL;
  delete p_rm->p_dbustimeout;
  p_rm->p_dbustimeout = NULL;
  delete p_rm->p_connection;
  p_rm->p_connection = NULL;
  delete p_rm->p_dispatcher;
  p_rm->p_dispatcher = NULL;

  tiz_queue_destroy(p_rm->p_queue);
  p_rm->p_queue = NULL;
  tiz_sem_destroy(&(p_rm->sem));

  /* NOTE: Do not free the tizrm_int_t structure. This will be reused in case
     the proxy is re-initialized */
  /* tiz_mem_free(p_rm); */

  return TIZRM_SUCCESS;

}

extern "C" tizrm_error_t
tizrm_proxy_init(tizrm_t * ap_rm, const OMX_STRING ap_name,
                 const OMX_UUIDTYPE * ap_uuid,
                 const OMX_PRIORITYMGMTTYPE * ap_pri,
                 const tizrm_proxy_callbacks_t * ap_cbacks,
                 OMX_PTR ap_data)
{
  tizrm_error_t rc = TIZRM_SUCCESS;
  tizrm_int_t *p_rm = NULL;

  TIZ_LOG(TIZ_LOG_TRACE, "IL RM Proxy Init");
  assert(ap_rm);
  assert(ap_name);
  assert(ap_uuid);
  assert(ap_pri);
  assert(ap_cbacks);
  assert(ap_cbacks->pf_waitend);
  assert(ap_cbacks->pf_preempt);
  assert(ap_cbacks->pf_preempt_end);

  if (NULL == (p_rm = get_rm()))
    {
      TIZ_LOG(TIZ_LOG_TRACE, "Error retrieving proxy");
      return TIZRM_OOM;
    }

  if (ETIZRmStateStarting == p_rm->state
      || ETIZRmStateStopped == p_rm->state)
    {

      DBus::_init_threading();
      
      p_rm->p_dispatcher    = new DBus::BusDispatcher();
      DBus::default_dispatcher = p_rm->p_dispatcher;

      /* Increase DBus-C++ frequency */
      p_rm->p_dbustimeout =
        new DBus::DefaultTimeout(100, false, p_rm->p_dispatcher);

      p_rm->p_connection =
        new DBus::Connection(DBus::Connection::SessionBus());
      p_rm->p_proxy      = new tizrmproxy(*(p_rm->p_connection),
                                          TIZ_RM_DAEMON_PATH,
                                          TIZ_RM_DAEMON_NAME);

      p_rm->state = ETIZRmStateStarted;
      TIZ_LOG(TIZ_LOG_TRACE, "Now in ETIZRmStateStarted state...");

      /* Create IL Proxy thread */
      tiz_thread_create(&(p_rm->thread),
                          0,
                          0,
                          il_rmproxy_thread_func,
                          p_rm);

      pthread_yield();
    }

  p_rm->ref_count++;
  if (NULL == (* ap_rm
               = p_rm->p_proxy->register_client(ap_name,
                                                * ap_uuid,
                                                ap_pri->nGroupID,
                                                ap_pri->nGroupPriority,
                                                ap_cbacks->pf_waitend,
                                                ap_cbacks->pf_preempt,
                                                ap_cbacks->pf_preempt_end,
                                                ap_data)))
    {
      TIZ_LOG(TIZ_LOG_TRACE, "Error registering proxy");
      rc = TIZRM_OOM;
    }


  return rc;

}

extern "C" tizrm_error_t
tizrm_proxy_destroy(tizrm_t * ap_rm)
{
  tizrm_error_t rc = TIZRM_SUCCESS;
  tizrm_int_t *p_rm = NULL;

  if (!ap_rm)
    {
      return TIZRM_MISUSE;
    }


  if (NULL == (p_rm = get_rm()))
    {
      TIZ_LOG(TIZ_LOG_TRACE, "Error retrieving proxy");
      return TIZRM_OOM;
    }

  TIZ_LOG(TIZ_LOG_TRACE, "IL RM Proxy destroy : ref_count [%d]", p_rm->ref_count);

  p_rm->p_proxy->unregister_client(ap_rm);
  p_rm->ref_count--;

  if (0 == p_rm->ref_count)
    {
      TIZ_LOG(TIZ_LOG_TRACE, "Last reference, cleaning up...");

      TIZ_LOG(TIZ_LOG_TRACE, "Will leave the dispatcher");

      p_rm->p_dispatcher->leave();

      pthread_yield();

      rc = stop_proxy();

      p_rm->state = ETIZRmStateStopped;

    }

  return rc;
}

extern "C" OMX_S32
tizrm_proxy_version(const tizrm_t * ap_rm)
{
  tizrm_int_t *p_rm = NULL;
  if (!ap_rm)
    {
      return TIZRM_MISUSE;
    }

  p_rm = get_rm();
  assert(p_rm);

  return p_rm->p_proxy->Version();
}

extern "C" tizrm_error_t
tizrm_proxy_acquire(const tizrm_t * ap_rm, OMX_U32 a_rid, OMX_U32 a_quantity)
{
  tizrm_int_t *p_rm = NULL;
  if (!ap_rm)
    {
      return TIZRM_MISUSE;
    }

  p_rm = get_rm();
  assert(p_rm);

  TIZ_LOG(TIZ_LOG_TRACE, "tizrm_proxy_acquire");

  return (tizrm_error_t)p_rm->p_proxy->acquire(ap_rm, a_rid, a_quantity);
}

extern "C" tizrm_error_t
tizrm_proxy_release(const tizrm_t * ap_rm, OMX_U32 a_rid, OMX_U32 a_quantity)
{
  tizrm_int_t *p_rm = NULL;
  if (!ap_rm)
    {
      return TIZRM_MISUSE;
    }

  p_rm = get_rm();
  assert(p_rm);

  TIZ_LOG(TIZ_LOG_TRACE, "tizrm_proxy_release");
  return (tizrm_error_t)p_rm->p_proxy->release(ap_rm, a_rid, a_quantity);
}

extern "C" tizrm_error_t
tizrm_proxy_wait(const tizrm_t * ap_rm, OMX_U32 a_rid, OMX_U32 a_quantity)
{
  tizrm_int_t *p_rm = NULL;
  if (!ap_rm)
    {
      return TIZRM_MISUSE;
    }

  p_rm = get_rm();
  assert(p_rm);

  TIZ_LOG(TIZ_LOG_TRACE, "tizrm_proxy_wait");
  return (tizrm_error_t)p_rm->p_proxy->wait(ap_rm, a_rid, a_quantity);
}

extern "C" tizrm_error_t
tizrm_proxy_cancel_wait(const tizrm_t * ap_rm, OMX_U32 a_rid, OMX_U32 a_quantity)
{
  tizrm_int_t *p_rm = NULL;
  if (!ap_rm)
    {
      return TIZRM_MISUSE;
    }

  p_rm = get_rm();
  assert(p_rm);

  TIZ_LOG(TIZ_LOG_TRACE, "tizrm_proxy_cancel_wait");
  return (tizrm_error_t)p_rm->p_proxy->cancel_wait(ap_rm, a_rid, a_quantity);
}

extern "C" tizrm_error_t
tizrm_proxy_preemption_conf(const tizrm_t * ap_rm, OMX_U32 a_rid, OMX_U32 a_quantity)
{
  tizrm_int_t *p_rm = NULL;
  if (!ap_rm)
    {
      return TIZRM_MISUSE;
    }

  p_rm = get_rm();
  assert(p_rm);

  TIZ_LOG(TIZ_LOG_TRACE, "tizrm_proxy_preemption_conf");
  return (tizrm_error_t)p_rm->p_proxy->preemption_conf(ap_rm, a_rid, a_quantity);
}
