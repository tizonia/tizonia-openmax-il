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
 * @file   tizcastclient_c.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL Chromecast daemon - client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizcastclient_c.h"
#include "tizcastclient.hh"
#include "tizplatform.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.client.c_api"
#endif

/* Bus name */
#define TIZ_CAST_DAEMON_NAME "com.aratelia.tiz.tizcastd"
/* Object path, a.k.a. node */
#define TIZ_CAST_DAEMON_PATH "/com/aratelia/tiz/tizcastd"

enum tiz_cast_state
{
  ETIZCastStateInvalid = 0,
  ETIZCastStateStarting,
  ETIZCastStateStarted,
  ETIZCastStateStopped
};
typedef enum tiz_cast_state tiz_cast_state_t;

struct tiz_cast_global
{
  tiz_thread_t thread;
  tiz_sem_t sem;
  tiz_queue_t * p_queue;
  tiz_cast_error_t error;
  tiz_cast_state_t state;
  OMX_S32 ref_count;
  Tiz::DBus::DefaultTimeout * p_dbustimeout;
  Tiz::DBus::BusDispatcher * p_dispatcher;
  Tiz::DBus::Connection * p_connection;
  tizcastclient * p_client;
};

typedef struct tiz_cast_global tiz_cast_global_t;

struct tiz_cast
{
  void * p_cast_id;
};

typedef tizcastclient::cast_client_id_ptr_t cast_client_id_ptr_t;

static inline tiz_cast_global_t *
get_global ();

static void *
cast_client_thread_func (void * p_arg)
{
  tiz_cast_global_t * p_cast = (tiz_cast_global_t *) (p_arg);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_cast [%08X]", p_cast);

  assert (p_cast);

  (void) tiz_thread_setname (&(p_cast->thread), (const OMX_STRING) "rmclient");

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Entering the dispatcher...");

  p_cast->p_dispatcher->enter ();

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Have left the dispatcher, thread exiting...");

  return NULL;
}

static inline tiz_cast_global_t *
get_global ()
{
  static tiz_cast_global_t * p_cast = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  /* For now, don't bother about thread safety of initialization of this */
  /* singleton. The current assumption is that this CAST client will be */
  /* initialized and deinitialized by the IL Core at init time. */

  if (!p_cast)
    {
      p_cast
        = (tiz_cast_global_t *) tiz_mem_calloc (1, sizeof (tiz_cast_global_t));

      if (!p_cast)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "While initializing cast client...");
          return NULL;
        }

      TIZ_LOG (TIZ_PRIORITY_TRACE, "Initializing rm [%p]...", p_cast);

      p_cast->p_client = NULL;

      if (OMX_ErrorNone != (rc = tiz_sem_init (&(p_cast->sem), 0)))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "While initializing cast client...");
          return NULL;
        }

      if (OMX_ErrorNone != (rc = tiz_queue_init (&(p_cast->p_queue), 10)))
        {
          return NULL;
        }

      p_cast->error = TIZ_CAST_SUCCESS;
      p_cast->state = ETIZCastStateStarting;
      p_cast->ref_count = 0;

      TIZ_LOG (TIZ_PRIORITY_TRACE, "Starting IL CAST client's thread [%p]...",
               p_cast);

      assert (p_cast);
    }

  return (rc == OMX_ErrorNone) ? p_cast : NULL;
}

static tiz_cast_error_t
stop_client ()
{
  tiz_cast_global_t * p_cast = get_global ();
  OMX_PTR p_result = NULL;
  assert (p_cast);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Stopping client's thread");

  tiz_thread_join (&(p_cast->thread), &p_result);

  delete p_cast->p_client;
  p_cast->p_client = NULL;
  delete p_cast->p_dbustimeout;
  p_cast->p_dbustimeout = NULL;
  delete p_cast->p_connection;
  p_cast->p_connection = NULL;
  delete p_cast->p_dispatcher;
  p_cast->p_dispatcher = NULL;

  tiz_queue_destroy (p_cast->p_queue);
  p_cast->p_queue = NULL;
  tiz_sem_destroy (&(p_cast->sem));

  /* NOTE: Do not free the tiz_cast_global_t structure. This will be reused in case
     the client is re-initialized */
  /* tiz_mem_free(p_cast); */

  return TIZ_CAST_SUCCESS;
}

extern "C" tiz_cast_error_t
tiz_cast_client_init (tiz_cast_ptr_t * app_cast,
                      const char * ap_device_name_or_ip,
                      const OMX_UUIDTYPE * ap_uuid,
                      const tiz_cast_client_callbacks_t * ap_cbacks,
                      OMX_PTR ap_data)
{
  tiz_cast_error_t rc = TIZ_CAST_SUCCESS;
  tiz_cast_global_t * p_global = NULL;
  assert (app_cast);
  assert (ap_device_name_or_ip);
  assert (ap_uuid);
  assert (ap_cbacks);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Tizonia CAST client Init");

  if (!(p_global = get_global ()))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "Retrieving cast client");
      return TIZ_CAST_OOM;
    }

  if (ETIZCastStateStarting == p_global->state
      || ETIZCastStateStopped == p_global->state)
    {

      Tiz::DBus::_init_threading ();

      p_global->p_dispatcher = new Tiz::DBus::BusDispatcher ();
      Tiz::DBus::default_dispatcher = p_global->p_dispatcher;

      /* Increase DBus-C++ frequency */
      p_global->p_dbustimeout
        = new Tiz::DBus::DefaultTimeout (100, false, p_global->p_dispatcher);

      p_global->p_connection
        = new Tiz::DBus::Connection (Tiz::DBus::Connection::SessionBus ());
      p_global->p_client = new tizcastclient (
        *(p_global->p_connection), TIZ_CAST_DAEMON_PATH, TIZ_CAST_DAEMON_NAME);

      p_global->state = ETIZCastStateStarted;
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Now in ETIZCastStateStarted state...");

      /* Create IL Proxy thread */
      tiz_thread_create (&(p_global->thread), 0, 0, cast_client_thread_func,
                         p_global);

      pthread_yield ();
    }

  p_global->ref_count++;
  if (!(*app_cast = (tiz_cast_ptr_t) p_global->p_client->connect (
          ap_device_name_or_ip, *ap_uuid, ap_cbacks->pf_url_loaded, ap_data)))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "While registering cast client");
      rc = TIZ_CAST_OOM;
    }

  return rc;
}

extern "C" tiz_cast_error_t
tiz_cast_client_destroy (tiz_cast_t * ap_cast)
{
  tiz_cast_error_t rc = TIZ_CAST_SUCCESS;
  tiz_cast_global_t * p_cast = NULL;

  if (!ap_cast)
    {
      return TIZ_CAST_MISUSE;
    }

  if (!(p_cast = get_global ()))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "Retrieving cast client");
      return TIZ_CAST_OOM;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "IL CAST client destroy : ref_count [%d]",
           p_cast->ref_count);

  p_cast->p_client->disconnect ((cast_client_id_ptr_t) ap_cast);
  p_cast->ref_count--;

  if (0 == p_cast->ref_count)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Last reference, cleaning up...");

      TIZ_LOG (TIZ_PRIORITY_TRACE, "Will leave the dispatcher");

      p_cast->p_dispatcher->leave ();

      pthread_yield ();

      rc = stop_client ();

      p_cast->state = ETIZCastStateStopped;
    }

  return rc;
}

extern "C" OMX_S32
tiz_cast_client_version (const tiz_cast_t * ap_cast)
{
  tiz_cast_global_t * p_cast = NULL;
  if (!ap_cast)
    {
      return TIZ_CAST_MISUSE;
    }
  p_cast = get_global ();
  assert (p_cast);
  return p_cast->p_client->version ();
}

extern "C" tiz_cast_error_t
tiz_cast_client_load_url (const tiz_cast_t * ap_cast, const char * url,
                          const char * mime_type, const char * title)
{
  tiz_cast_global_t * p_cast = NULL;
  if (!ap_cast)
    {
      return TIZ_CAST_MISUSE;
    }
  p_cast = get_global ();
  assert (p_cast);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_cast_client_load_url");
  return (tiz_cast_error_t) p_cast->p_client->load_url (
    (cast_client_id_ptr_t) ap_cast, url, mime_type, title);
}

extern "C" tiz_cast_error_t
tiz_cast_client_play (const tiz_cast_t * ap_cast)
{
  tiz_cast_global_t * p_cast = NULL;
  if (!ap_cast)
    {
      return TIZ_CAST_MISUSE;
    }
  p_cast = get_global ();
  assert (p_cast);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_cast_client_play");
  return (tiz_cast_error_t) p_cast->p_client->play (
    (cast_client_id_ptr_t) ap_cast);
}

extern "C" tiz_cast_error_t
tiz_cast_client_stop (const tiz_cast_t * ap_cast)
{
  tiz_cast_global_t * p_cast = NULL;
  if (!ap_cast)
    {
      return TIZ_CAST_MISUSE;
    }

  p_cast = get_global ();
  assert (p_cast);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_cast_client_stop");
  return (tiz_cast_error_t) p_cast->p_client->stop (
    (cast_client_id_ptr_t) ap_cast);
}

extern "C" tiz_cast_error_t
tiz_cast_client_pause (const tiz_cast_t * ap_cast)
{
  tiz_cast_global_t * p_cast = NULL;
  if (!ap_cast)
    {
      return TIZ_CAST_MISUSE;
    }

  p_cast = get_global ();
  assert (p_cast);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_cast_client_pause");
  return (tiz_cast_error_t) p_cast->p_client->pause (
    (cast_client_id_ptr_t) ap_cast);
}

extern "C" tiz_cast_error_t
tiz_cast_client_volume_up (const tiz_cast_t * ap_cast)
{
  tiz_cast_global_t * p_cast = NULL;
  if (!ap_cast)
    {
      return TIZ_CAST_MISUSE;
    }

  p_cast = get_global ();
  assert (p_cast);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_cast_client_volume_up");
  return (tiz_cast_error_t) p_cast->p_client->volume_up (
    (cast_client_id_ptr_t) ap_cast);
}

extern "C" tiz_cast_error_t
tiz_cast_client_volume_down (const tiz_cast_t * ap_cast)
{
  tiz_cast_global_t * p_cast = NULL;
  if (!ap_cast)
    {
      return TIZ_CAST_MISUSE;
    }

  p_cast = get_global ();
  assert (p_cast);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_cast_client_volume_down");
  return (tiz_cast_error_t) p_cast->p_client->volume_down (
    (cast_client_id_ptr_t) ap_cast);
}

extern "C" tiz_cast_error_t
tiz_cast_client_mute (const tiz_cast_t * ap_cast)
{
  tiz_cast_global_t * p_cast = NULL;
  if (!ap_cast)
    {
      return TIZ_CAST_MISUSE;
    }

  p_cast = get_global ();
  assert (p_cast);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_cast_client_mute");
  return (tiz_cast_error_t) p_cast->p_client->mute (
    (cast_client_id_ptr_t) ap_cast);
}

extern "C" tiz_cast_error_t
tiz_cast_client_unmute (const tiz_cast_t * ap_cast)
{
  tiz_cast_global_t * p_cast = NULL;
  if (!ap_cast)
    {
      return TIZ_CAST_MISUSE;
    }

  p_cast = get_global ();
  assert (p_cast);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_cast_client_unmute");
  return (tiz_cast_error_t) p_cast->p_client->unmute (
    (cast_client_id_ptr_t) ap_cast);
}
