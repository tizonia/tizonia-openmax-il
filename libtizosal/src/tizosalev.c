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
 * @file   tizosalev.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - Event loop, async io and timers
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <ev.h>

#include "tizosalev.h"
#include "tizosalthread.h"
#include "tizosalsync.h"
#include "tizosalmem.h"
#include "tizosallog.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.osal.event"
#endif

struct tiz_event_io
{
  ev_io io;
  tiz_event_io_cb_f pf_cback;
  OMX_HANDLETYPE p_hdl;
};

struct tiz_event_timer
{
  ev_timer timer;
  tiz_event_timer_cb_f pf_cback;
  OMX_HANDLETYPE p_hdl;
};

struct tiz_event_stat
{
  ev_stat stat;
  tiz_event_stat_cb_f pf_cback;
  OMX_HANDLETYPE p_hdl;
};

typedef enum tiz_loop_thread_state tiz_loop_thread_state_t;
enum tiz_loop_thread_state
{
  ETIZEventLoopStateInvalid = 0,
  ETIZEventLoopStateStarting,
  ETIZEventLoopStateStarted,
  ETIZEventLoopStateStopping,
  ETIZEventLoopStateStopped
};

typedef struct tiz_loop_thread tiz_loop_thread_t;
struct tiz_loop_thread
{
  tiz_thread_t thread;
  tiz_mutex_t mutex;
  tiz_sem_t sem;
  ev_async *p_async_watcher;
  struct ev_loop *p_loop;
  tiz_loop_thread_state_t state;
  OMX_S32 ref_count;
};

static pthread_once_t g_event_thread_once = PTHREAD_ONCE_INIT;
static tiz_loop_thread_t *gp_event_thread = NULL;

static void
async_watcher_cback (struct ev_loop *ap_loop, ev_async * ap_watcher,
                     int a_revents)
{

  if (NULL == gp_event_thread)
    {
      return;
    }

  if (ETIZEventLoopStateStopping == gp_event_thread->state)
    {
      ev_break (gp_event_thread->p_loop, EVBREAK_ONE);
    }
}

static void
io_watcher_cback (struct ev_loop *ap_loop, ev_io * ap_watcher, int a_revents)
{
  tiz_event_io_t *p_io_event = (tiz_event_io_t *) ap_watcher;

  if (NULL == gp_event_thread)
    {
      return;
    }

  assert (NULL != p_io_event);
  assert (NULL != p_io_event->pf_cback);

  TIZ_LOG (TIZ_LOG_TRACE, "io watcher cback");

  p_io_event->pf_cback (p_io_event, p_io_event->p_hdl,
                        ((ev_io *) p_io_event)->fd, a_revents);
}

static void
timer_watcher_cback (struct ev_loop *ap_loop, ev_timer * ap_watcher,
                     int a_revents)
{
  tiz_event_timer_t *p_timer_event = (tiz_event_timer_t *) ap_watcher;

  if (NULL == gp_event_thread)
    {
      return;
    }

  assert (NULL != p_timer_event);
  assert (NULL != p_timer_event->pf_cback);

  TIZ_LOG (TIZ_LOG_TRACE, "timer watcher cback");

  p_timer_event->pf_cback (p_timer_event, p_timer_event->p_hdl);
}

static void
stat_watcher_cback (struct ev_loop *ap_loop, ev_stat * ap_watcher,
                    int a_revents)
{
  tiz_event_stat_t *p_stat_event = (tiz_event_stat_t *) ap_watcher;

  if (NULL == gp_event_thread)
    {
      return;
    }

  assert (NULL != p_stat_event);
  assert (NULL != p_stat_event->pf_cback);

  TIZ_LOG (TIZ_LOG_TRACE, "stat watcher cback");

  p_stat_event->pf_cback (p_stat_event, p_stat_event->p_hdl, a_revents);
}

static void *
event_loop_thread_func (void *p_arg)
{
  tiz_loop_thread_t *p_loop_thread = p_arg;
  struct ev_loop *p_loop = NULL;

  assert (NULL != p_loop_thread);

  p_loop = p_loop_thread->p_loop;
  assert (NULL != p_loop);

  TIZ_LOG (TIZ_LOG_TRACE, "event loop [%p]", p_loop);

  TIZ_LOG (TIZ_LOG_TRACE, "Entering the dispatcher...");
  tiz_sem_post (&(p_loop_thread->sem));

  ev_run (p_loop, 0);

  TIZ_LOG (TIZ_LOG_TRACE, "Have left the dispatcher, thread exiting...");

  return NULL;
}

static inline void
clean_up_thread_data (tiz_loop_thread_t * ap_lp)
{
  if (NULL != ap_lp)
    {
      if (NULL != ap_lp->p_async_watcher)
        {
          tiz_mem_free (ap_lp->p_async_watcher);
          ap_lp->p_async_watcher = NULL;
        }

      if (NULL != ap_lp->p_loop)
        {
          ev_loop_destroy (ap_lp->p_loop);
          ap_lp->p_loop = NULL;
        }

      if (NULL != ap_lp->mutex)
        {
          (void) tiz_mutex_destroy (&(ap_lp->mutex));
          ap_lp->mutex = NULL;
        }

      if (NULL != ap_lp->sem)
        {
          (void) tiz_sem_destroy (&(ap_lp->sem));
          ap_lp->sem = NULL;
        }

      tiz_mem_free (gp_event_thread);
      gp_event_thread = NULL;
    }
}

static void
init_loop_thread ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  if (NULL == gp_event_thread)
    {
      gp_event_thread = (tiz_loop_thread_t *)
        tiz_mem_calloc (1, sizeof (tiz_loop_thread_t));

      if (NULL == gp_event_thread)
        {
          TIZ_LOG (TIZ_LOG_ERROR, "Error allocating thread data struct.");
          rc = OMX_ErrorInsufficientResources;
          goto end;
        }

      gp_event_thread->state = ETIZEventLoopStateStarting;

      if (NULL == (gp_event_thread->p_loop = ev_loop_new (EVFLAG_AUTO)))
        {
          TIZ_LOG (TIZ_LOG_ERROR, "Error instantiating ev_loop.");
          rc = OMX_ErrorInsufficientResources;
          goto end;
        }

      if (NULL == (gp_event_thread->p_async_watcher
                   = (ev_async *) tiz_mem_calloc (1, sizeof (ev_async))))
        {
          TIZ_LOG (TIZ_LOG_ERROR, "Error initializing async watcher.");
          rc = OMX_ErrorInsufficientResources;
          goto end;
        }

      if (OMX_ErrorNone != (tiz_mutex_init (&(gp_event_thread->mutex))))
        {
          TIZ_LOG (TIZ_LOG_ERROR, "Error initializing mutex.");
          rc = OMX_ErrorInsufficientResources;
          goto end;
        }

      if (OMX_ErrorNone != (tiz_sem_init (&(gp_event_thread->sem), 0)))
        {
          TIZ_LOG (TIZ_LOG_ERROR, "Error initializing sem.");
          rc = OMX_ErrorInsufficientResources;
          goto end;
        }

      ev_async_init (gp_event_thread->p_async_watcher, async_watcher_cback);
      ev_async_start (gp_event_thread->p_loop,
                      gp_event_thread->p_async_watcher);

      gp_event_thread->ref_count = 0;

      assert (gp_event_thread);
    }

end:

  if (OMX_ErrorNone == rc)
    {
      tiz_loop_thread_t *p_lp = gp_event_thread;
      p_lp->state = ETIZEventLoopStateStarted;
      /* Create event loop thread */
      tiz_thread_create (&(p_lp->thread), 0, 0, event_loop_thread_func, p_lp);
      TIZ_LOG (TIZ_LOG_TRACE, "Now in ETIZEventLoopStateStarted state...");

      tiz_mutex_lock (&(p_lp->mutex));
      /* This is to prevent the event loop from exiting when there are no
       * more active events */
      ev_ref (p_lp->p_loop);
      tiz_mutex_unlock (&(p_lp->mutex));
      tiz_sem_wait (&(p_lp->sem));
    }
  else
    {
      clean_up_thread_data (gp_event_thread);
      gp_event_thread = NULL;
    }

}

static inline tiz_loop_thread_t *
get_loop_thread ()
{
  (void) pthread_once (&g_event_thread_once, init_loop_thread);

  if (NULL != gp_event_thread)
    {
      tiz_loop_thread_t *p_lp = gp_event_thread;
      tiz_mutex_lock (&(p_lp->mutex));
      p_lp->ref_count++;
      TIZ_LOG (TIZ_LOG_TRACE, "num clients = %d...", p_lp->ref_count);
      tiz_mutex_unlock (&(p_lp->mutex));
    }

  return gp_event_thread;
}

OMX_ERRORTYPE
tiz_event_loop_init ()
{
  if (NULL == get_loop_thread ())
    {
      TIZ_LOG (TIZ_LOG_ERROR, "[OMX_ErrorInsufficientResources] : "
               "Error retrieving the loop thread.");
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

void
tiz_event_loop_destroy ()
{
  if (NULL != gp_event_thread)
    {
      tiz_mutex_lock (&(gp_event_thread->mutex));
      TIZ_LOG (TIZ_LOG_TRACE, "num clients = %d.",
               gp_event_thread->ref_count);
      if (--(gp_event_thread->ref_count) == 0)
        {
          TIZ_LOG (TIZ_LOG_TRACE, "Last client: destroying event loop.");
          gp_event_thread->state = ETIZEventLoopStateStopping;
          ev_unref (gp_event_thread->p_loop);
          ev_async_send (gp_event_thread->p_loop,
                         gp_event_thread->p_async_watcher);
        }
      tiz_mutex_unlock (&(gp_event_thread->mutex));

      if (ETIZEventLoopStateStopping == gp_event_thread->state)
        {
          OMX_PTR p_result = NULL;
          tiz_thread_join (&(gp_event_thread->thread), &p_result);
          clean_up_thread_data (gp_event_thread);
          gp_event_thread = NULL;
        }
    }
}

/*
 * IO Event-related functions
 */

OMX_ERRORTYPE
tiz_event_io_init (tiz_event_io_t ** app_ev_io,
                   OMX_HANDLETYPE ap_hdl,
                   tiz_event_io_cb_f ap_cback)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_event_io_t *p_io_watcher = NULL;

  assert (NULL != app_ev_io);
  assert (NULL != ap_cback);
  assert (NULL != gp_event_thread);

  if (NULL == (p_io_watcher
               =
               (tiz_event_io_t *) tiz_mem_calloc (1,
                                                  sizeof (tiz_event_io_t))))
    {
      return OMX_ErrorInsufficientResources;
    }

  p_io_watcher->pf_cback = ap_cback;
  p_io_watcher->p_hdl    = ap_hdl;
  ev_init ((ev_io *) p_io_watcher, io_watcher_cback);

  *app_ev_io = p_io_watcher;

  return rc;
}

void
tiz_event_io_set (tiz_event_io_t * ap_ev_io,
                  int a_fd, tiz_event_io_event_t a_event)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_io);
  assert (a_fd > 0);
  assert (a_event < TIZ_EVENT_MAX);

  ev_io_set ((ev_io *) ap_ev_io, a_fd, a_event);
}

OMX_ERRORTYPE
tiz_event_io_start (tiz_event_io_t * ap_ev_io)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_io);

  pthread_mutex_lock (gp_event_thread->mutex);
  ev_io_start (gp_event_thread->p_loop, (ev_io *) ap_ev_io);
  ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
  pthread_mutex_unlock (gp_event_thread->mutex);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_event_io_stop (tiz_event_io_t * ap_ev_io)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_io);

  pthread_mutex_lock (gp_event_thread->mutex);
  ev_io_stop (gp_event_thread->p_loop, (ev_io *) ap_ev_io);
  ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
  pthread_mutex_unlock (gp_event_thread->mutex);

  return OMX_ErrorNone;
}

void
tiz_event_io_destroy (tiz_event_io_t * ap_ev_io)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_io);

  tiz_event_io_stop (ap_ev_io);
  tiz_mem_free (ap_ev_io);
}

/*
 * Timer Event-related functions
 */

OMX_ERRORTYPE
tiz_event_timer_init (tiz_event_timer_t ** app_ev_timer,
                      OMX_HANDLETYPE ap_hdl,
                      tiz_event_timer_cb_f ap_cback)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_event_timer_t *p_timer_watcher = NULL;

  assert (NULL != app_ev_timer);
  assert (NULL != ap_cback);
  assert (NULL != gp_event_thread);

  if (NULL == (p_timer_watcher = (tiz_event_timer_t *)
               tiz_mem_calloc (1, sizeof (tiz_event_timer_t))))
    {
      return OMX_ErrorInsufficientResources;
    }

  p_timer_watcher->pf_cback = ap_cback;
  p_timer_watcher->p_hdl    = ap_hdl;
  ev_init ((ev_timer *) p_timer_watcher, timer_watcher_cback);

  *app_ev_timer = p_timer_watcher;

  return rc;
}

void
tiz_event_timer_set (tiz_event_timer_t * ap_ev_timer,
                     double a_after, double a_repeat)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_timer);

  ev_timer_set ((ev_timer *) ap_ev_timer, a_after, a_repeat);
}

OMX_ERRORTYPE
tiz_event_timer_start (tiz_event_timer_t * ap_ev_timer)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_timer);

  pthread_mutex_lock (gp_event_thread->mutex);
  ev_timer_start (gp_event_thread->p_loop, (ev_timer *) ap_ev_timer);
  ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
  pthread_mutex_unlock (gp_event_thread->mutex);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_event_timer_restart (tiz_event_timer_t * ap_ev_timer)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_timer);

  pthread_mutex_lock (gp_event_thread->mutex);
  ev_timer_again (gp_event_thread->p_loop, (ev_timer *) ap_ev_timer);
  pthread_mutex_unlock (gp_event_thread->mutex);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_event_timer_stop (tiz_event_timer_t * ap_ev_timer)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_timer);

  pthread_mutex_lock (gp_event_thread->mutex);
  ev_timer_stop (gp_event_thread->p_loop, (ev_timer *) ap_ev_timer);
  ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
  pthread_mutex_unlock (gp_event_thread->mutex);

  return OMX_ErrorNone;
}

void
tiz_event_timer_destroy (tiz_event_timer_t * ap_ev_timer)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_timer);

  tiz_event_timer_stop (ap_ev_timer);
  tiz_mem_free (ap_ev_timer);
}

/*
 * File status Event-related functions
 */

OMX_ERRORTYPE
tiz_event_stat_init (tiz_event_stat_t ** app_ev_stat,
                     OMX_HANDLETYPE ap_hdl,
                     tiz_event_stat_cb_f ap_cback)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_event_stat_t *p_stat_watcher = NULL;

  assert (NULL != app_ev_stat);
  assert (NULL != ap_cback);
  assert (NULL != gp_event_thread);

  if (NULL == (p_stat_watcher = (tiz_event_stat_t *)
               tiz_mem_calloc (1, sizeof (tiz_event_stat_t))))
    {
      return OMX_ErrorInsufficientResources;
    }

  p_stat_watcher->pf_cback = ap_cback;
  p_stat_watcher->p_hdl    = ap_hdl;
  ev_init ((ev_stat *) p_stat_watcher, stat_watcher_cback);

  *app_ev_stat = p_stat_watcher;

  return rc;
}

void
tiz_event_stat_set (tiz_event_stat_t * ap_ev_stat,
                    const char *ap_path)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_stat);

  ev_stat_set ((ev_stat *) ap_ev_stat, ap_path, 0);
}

OMX_ERRORTYPE
tiz_event_stat_start (tiz_event_stat_t * ap_ev_stat)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_stat);

  pthread_mutex_lock (gp_event_thread->mutex);
  ev_stat_start (gp_event_thread->p_loop, (ev_stat *) ap_ev_stat);
  ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
  pthread_mutex_unlock (gp_event_thread->mutex);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_event_stat_stop (tiz_event_stat_t * ap_ev_stat)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_stat);

  pthread_mutex_lock (gp_event_thread->mutex);
  ev_stat_stop (gp_event_thread->p_loop, (ev_stat *) ap_ev_stat);
  ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
  pthread_mutex_unlock (gp_event_thread->mutex);

  return OMX_ErrorNone;
}

void
tiz_event_stat_destroy (tiz_event_stat_t * ap_ev_stat)
{
  assert (NULL != gp_event_thread);
  assert (NULL != ap_ev_stat);

  tiz_event_stat_stop (ap_ev_stat);
  tiz_mem_free (ap_ev_stat);
}
