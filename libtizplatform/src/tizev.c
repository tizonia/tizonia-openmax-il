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
 * @file   tizev.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia Platform - Event loop, async io and timers
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

#include "tizplatform.h"
#include "tizplatform_internal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.event"
#endif

#define TIZ_EV_THREAD_NAME "tizevloop"

struct tiz_event_io
{
  ev_io io;
  tiz_event_io_cb_f pf_cback;
  void *p_arg0;
  void *p_arg1;
  bool once;
  uint32_t id;
};

struct tiz_event_timer
{
  ev_timer timer;
  tiz_event_timer_cb_f pf_cback;
  void *p_arg0;
  void *p_arg1;
  uint32_t id;
};

struct tiz_event_stat
{
  ev_stat stat;
  tiz_event_stat_cb_f pf_cback;
  void *p_arg0;
  void *p_arg1;
  uint32_t id;
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
  tiz_rcfile_t *p_rcfile;
};

static pthread_once_t g_event_thread_once = PTHREAD_ONCE_INIT;
static tiz_loop_thread_t *gp_event_thread = NULL;

static void async_watcher_cback (struct ev_loop *ap_loop, ev_async *ap_watcher,
                                 int a_revents)
{
  (void)ap_loop;
  (void)ap_watcher;
  (void)a_revents;

  if (gp_event_thread)
    {
      if (ETIZEventLoopStateStopping == gp_event_thread->state)
        {
          ev_break (gp_event_thread->p_loop, EVBREAK_ONE);
        }
    }
}

static void io_watcher_cback (struct ev_loop *ap_loop, ev_io *ap_watcher,
                              int a_revents)
{
  tiz_event_io_t *p_io_event = (tiz_event_io_t *)ap_watcher;
  (void)ap_loop;

  if (gp_event_thread)
    {
      assert (NULL != p_io_event);
      assert (NULL != p_io_event->pf_cback);

      if (p_io_event->once)
        {
          ev_io_stop (gp_event_thread->p_loop, (ev_io *)p_io_event);
        }

      TIZ_LOG (TIZ_PRIORITY_TRACE, "io watcher cback");

      p_io_event->pf_cback (p_io_event->p_arg0, p_io_event, p_io_event->p_arg1,
                            p_io_event->id, ((ev_io *)p_io_event)->fd,
                            a_revents);
    }
}

static void timer_watcher_cback (struct ev_loop *ap_loop, ev_timer *ap_watcher,
                                 int a_revents)
{
  (void)ap_loop;
  (void)a_revents;

  if (gp_event_thread)
    {
      tiz_event_timer_t *p_timer_event = (tiz_event_timer_t *)ap_watcher;
      assert (NULL != p_timer_event);
      assert (NULL != p_timer_event->pf_cback);

      TIZ_LOG (TIZ_PRIORITY_TRACE, "timer watcher cback");

      p_timer_event->pf_cback (p_timer_event->p_arg0, p_timer_event,
                               p_timer_event->p_arg1, p_timer_event->id);
    }
}

static void stat_watcher_cback (struct ev_loop *ap_loop, ev_stat *ap_watcher,
                                int a_revents)
{
  (void)ap_loop;

  if (gp_event_thread)
    {
      tiz_event_stat_t *p_stat_event = (tiz_event_stat_t *)ap_watcher;
      assert (NULL != p_stat_event);
      assert (NULL != p_stat_event->pf_cback);

      TIZ_LOG (TIZ_PRIORITY_TRACE, "stat watcher cback");

      p_stat_event->pf_cback (p_stat_event->p_arg0, p_stat_event,
                              p_stat_event->p_arg1, p_stat_event->id,
                              a_revents);
    }
}

static void *event_loop_thread_func (void *p_arg)
{
  tiz_loop_thread_t *p_loop_thread = p_arg;
  struct ev_loop *p_loop = NULL;

  assert (NULL != p_loop_thread);

  p_loop = p_loop_thread->p_loop;
  assert (NULL != p_loop);

  (void)tiz_thread_setname (&(p_loop_thread->thread),
                            (const OMX_STRING)TIZ_EV_THREAD_NAME);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Entering the dispatcher...");
  tiz_sem_post (&(p_loop_thread->sem));

  ev_run (p_loop, 0);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Have left the dispatcher, thread exiting...");

  return NULL;
}

static inline void clean_up_thread_data (tiz_loop_thread_t *ap_lp)
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
          (void)tiz_mutex_destroy (&(ap_lp->mutex));
          ap_lp->mutex = NULL;
        }

      if (NULL != ap_lp->sem)
        {
          (void)tiz_sem_destroy (&(ap_lp->sem));
          ap_lp->sem = NULL;
        }

      tiz_mem_free (gp_event_thread);
      gp_event_thread = NULL;
    }
}

static void child_event_loop_reset (void)
{
  /* Reset the once control */
  pthread_once_t once = PTHREAD_ONCE_INIT;
  memcpy (&g_event_thread_once, &once, sizeof(g_event_thread_once));
  gp_event_thread = NULL;
}

static void init_loop_thread (void)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  if (NULL == gp_event_thread)
    {
      /* Let's return OOM error if something goes wrong */
      rc = OMX_ErrorInsufficientResources;

      /* Register a handler to reset the pthread_once_t global variable to try
         to cope with the scenario of a process forking without exec. The idea
         is to make sure that the loop thread is re-created in the child
         process */
      pthread_atfork (NULL, NULL, child_event_loop_reset);

      gp_event_thread
          = (tiz_loop_thread_t *)tiz_mem_calloc (1, sizeof(tiz_loop_thread_t));

      if (NULL == gp_event_thread)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Error allocating thread data struct.");
          goto end;
        }

      gp_event_thread->state = ETIZEventLoopStateStarting;

      if (OMX_ErrorNone != (tiz_rcfile_init (&(gp_event_thread->p_rcfile))))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Error opening configuration file.");
          goto end;
        }

      if (NULL == (gp_event_thread->p_loop = ev_loop_new (EVFLAG_AUTO)))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Error instantiating ev_loop.");
          goto end;
        }

      if (NULL == (gp_event_thread->p_async_watcher
                   = (ev_async *)tiz_mem_calloc (1, sizeof(ev_async))))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Error initializing async watcher.");
          goto end;
        }

      if (OMX_ErrorNone != (tiz_mutex_init (&(gp_event_thread->mutex))))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Error initializing mutex.");
          goto end;
        }

      if (OMX_ErrorNone != (tiz_sem_init (&(gp_event_thread->sem), 0)))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Error initializing sem.");
          goto end;
        }

      /* All good */
      rc = OMX_ErrorNone;
      ;

      ev_async_init (gp_event_thread->p_async_watcher, async_watcher_cback);
      ev_async_start (gp_event_thread->p_loop,
                      gp_event_thread->p_async_watcher);

      assert (gp_event_thread);
    }

end:

  if (OMX_ErrorNone == rc)
    {
      tiz_loop_thread_t *p_lp = gp_event_thread;
      p_lp->state = ETIZEventLoopStateStarted;
      /* Create event loop thread */
      tiz_thread_create (&(p_lp->thread), 0, 0, event_loop_thread_func, p_lp);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Now in ETIZEventLoopStateStarted state...");

      (void)tiz_mutex_lock (&(p_lp->mutex));
      /* This is to prevent the event loop from exiting when there are no
       * more active events */
      ev_ref (p_lp->p_loop);
      (void)tiz_mutex_unlock (&(p_lp->mutex));
      tiz_sem_wait (&(p_lp->sem));
    }
  else
    {
      clean_up_thread_data (gp_event_thread);
      gp_event_thread = NULL;
    }
}

static inline tiz_loop_thread_t *get_loop_thread (void)
{
  (void)pthread_once (&g_event_thread_once, init_loop_thread);
  return gp_event_thread;
}

OMX_ERRORTYPE
tiz_event_loop_init (void)
{
  return get_loop_thread () ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
}

void tiz_event_loop_destroy (void)
{
  /* NOTE: If the thread is destroyed, it can't be recreated in the same
     process as it's been instantiated with pthread_once. */

  if (NULL != gp_event_thread)
    {
      (void)tiz_mutex_lock (&(gp_event_thread->mutex));
      TIZ_LOG (TIZ_PRIORITY_TRACE, "destroying event loop thread [%p].",
               gp_event_thread);
      gp_event_thread->state = ETIZEventLoopStateStopping;
      ev_unref (gp_event_thread->p_loop);
      ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
      (void)tiz_mutex_unlock (&(gp_event_thread->mutex));

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
tiz_event_io_init (tiz_event_io_t **app_ev_io, void *ap_arg0,
                   tiz_event_io_cb_f ap_cback, void *ap_arg1)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  tiz_event_io_t *p_io_watcher = NULL;

  assert (NULL != app_ev_io);
  assert (NULL != ap_cback);
  (void)get_loop_thread ();

  if (NULL != (p_io_watcher
               = (tiz_event_io_t *)tiz_mem_calloc (1, sizeof(tiz_event_io_t))))
    {
      p_io_watcher->pf_cback = ap_cback;
      p_io_watcher->p_arg0 = ap_arg0;
      p_io_watcher->p_arg1 = ap_arg1;
      p_io_watcher->once = false;
      p_io_watcher->id = 0;
      ev_init ((ev_io *)p_io_watcher, io_watcher_cback);
      rc = OMX_ErrorNone;
    }

  *app_ev_io = p_io_watcher;

  return rc;
}

void tiz_event_io_set (tiz_event_io_t *ap_ev_io, int a_fd,
                       tiz_event_io_event_t a_event, bool only_once)
{
  (void)get_loop_thread ();
  assert (NULL != ap_ev_io);
  assert (a_fd > 0);
  assert (a_event < TIZ_EVENT_MAX);
  ap_ev_io->once = only_once;
  ev_io_set ((ev_io *)ap_ev_io, a_fd, a_event);
}

OMX_ERRORTYPE
tiz_event_io_start (tiz_event_io_t *ap_ev_io, const uint32_t a_id)
{
  (void)get_loop_thread ();
  assert (NULL != ap_ev_io);
  tiz_check_omx_err (tiz_mutex_lock (&(gp_event_thread->mutex)));
  ap_ev_io->id = a_id;
  ev_io_start (gp_event_thread->p_loop, (ev_io *)ap_ev_io);
  ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
  tiz_check_omx_err (tiz_mutex_unlock (&(gp_event_thread->mutex)));
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_event_io_stop (tiz_event_io_t *ap_ev_io)
{
  (void)get_loop_thread ();
  assert (NULL != ap_ev_io);
  tiz_check_omx_err (tiz_mutex_lock (&(gp_event_thread->mutex)));
  ev_io_stop (gp_event_thread->p_loop, (ev_io *)ap_ev_io);
  ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
  tiz_check_omx_err (tiz_mutex_unlock (&(gp_event_thread->mutex)));
  return OMX_ErrorNone;
}

bool tiz_event_io_is_level_triggered (tiz_event_io_t *ap_ev_io)
{
  assert (NULL != ap_ev_io);
  return ap_ev_io->once;
}

void tiz_event_io_destroy (tiz_event_io_t *ap_ev_io)
{
  if (NULL != ap_ev_io)
    {
      (void)get_loop_thread ();
      tiz_event_io_stop (ap_ev_io);
      tiz_mem_free (ap_ev_io);
    }
}

/*
 * Timer Event-related functions
 */

OMX_ERRORTYPE
tiz_event_timer_init (tiz_event_timer_t **app_ev_timer, void *ap_arg0,
                      tiz_event_timer_cb_f ap_cback, void *ap_arg1)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  tiz_event_timer_t *p_timer_watcher = NULL;

  assert (NULL != app_ev_timer);
  assert (NULL != ap_cback);
  (void)get_loop_thread ();

  if (NULL != (p_timer_watcher = (tiz_event_timer_t *)tiz_mem_calloc (
                   1, sizeof(tiz_event_timer_t))))
    {
      p_timer_watcher->pf_cback = ap_cback;
      p_timer_watcher->p_arg0 = ap_arg0;
      p_timer_watcher->p_arg1 = ap_arg1;
      p_timer_watcher->id = 0;
      ev_init ((ev_timer *)p_timer_watcher, timer_watcher_cback);
      rc = OMX_ErrorNone;
    }

  *app_ev_timer = p_timer_watcher;

  return rc;
}

void tiz_event_timer_set (tiz_event_timer_t *ap_ev_timer, double a_after,
                          double a_repeat)
{
  (void)get_loop_thread ();
  assert (NULL != ap_ev_timer);
  ev_timer_set ((ev_timer *)ap_ev_timer, a_after, a_repeat);
}

OMX_ERRORTYPE
tiz_event_timer_start (tiz_event_timer_t *ap_ev_timer, const uint32_t a_id)
{
  (void)get_loop_thread ();
  assert (NULL != ap_ev_timer);
  tiz_check_omx_err (tiz_mutex_lock (&(gp_event_thread->mutex)));
  ap_ev_timer->id = a_id;
  ev_timer_start (gp_event_thread->p_loop, (ev_timer *)ap_ev_timer);
  ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
  tiz_check_omx_err (tiz_mutex_unlock (&(gp_event_thread->mutex)));
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_event_timer_restart (tiz_event_timer_t *ap_ev_timer, const uint32_t a_id)
{
  (void)get_loop_thread ();
  assert (NULL != ap_ev_timer);
  tiz_check_omx_err (tiz_mutex_lock (&(gp_event_thread->mutex)));
  ap_ev_timer->id = a_id;
  ev_timer_again (gp_event_thread->p_loop, (ev_timer *)ap_ev_timer);
  tiz_check_omx_err (tiz_mutex_unlock (&(gp_event_thread->mutex)));
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_event_timer_stop (tiz_event_timer_t *ap_ev_timer)
{
  (void)get_loop_thread ();
  assert (NULL != ap_ev_timer);
  tiz_check_omx_err (tiz_mutex_lock (&(gp_event_thread->mutex)));
  ev_timer_stop (gp_event_thread->p_loop, (ev_timer *)ap_ev_timer);
  ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
  tiz_check_omx_err (tiz_mutex_unlock (&(gp_event_thread->mutex)));
  return OMX_ErrorNone;
}

void tiz_event_timer_destroy (tiz_event_timer_t *ap_ev_timer)
{
  if (NULL != ap_ev_timer)
    {
      (void)get_loop_thread ();
      tiz_event_timer_stop (ap_ev_timer);
      tiz_mem_free (ap_ev_timer);
    }
}

/*
 * File status Event-related functions
 */

OMX_ERRORTYPE
tiz_event_stat_init (tiz_event_stat_t **app_ev_stat, void *ap_arg0,
                     tiz_event_stat_cb_f ap_cback, void *ap_arg1)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  tiz_event_stat_t *p_stat_watcher = NULL;

  assert (NULL != app_ev_stat);
  assert (NULL != ap_cback);
  (void)get_loop_thread ();

  if (NULL != (p_stat_watcher = (tiz_event_stat_t *)tiz_mem_calloc (
                   1, sizeof(tiz_event_stat_t))))
    {
      p_stat_watcher->pf_cback = ap_cback;
      p_stat_watcher->p_arg0 = ap_arg0;
      p_stat_watcher->p_arg1 = ap_arg1;
      p_stat_watcher->id = 0;
      ev_init ((ev_stat *)p_stat_watcher, stat_watcher_cback);
      rc = OMX_ErrorNone;
    }

  *app_ev_stat = p_stat_watcher;

  return rc;
}

void tiz_event_stat_set (tiz_event_stat_t *ap_ev_stat, const char *ap_path)
{
  (void)get_loop_thread ();
  assert (NULL != ap_ev_stat);
  ev_stat_set ((ev_stat *)ap_ev_stat, ap_path, 0);
}

OMX_ERRORTYPE
tiz_event_stat_start (tiz_event_stat_t *ap_ev_stat, const uint32_t a_id)
{
  (void)get_loop_thread ();
  assert (NULL != ap_ev_stat);
  tiz_check_omx_err (tiz_mutex_lock (&(gp_event_thread->mutex)));
  ap_ev_stat->id = a_id;
  ev_stat_start (gp_event_thread->p_loop, (ev_stat *)ap_ev_stat);
  ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
  tiz_check_omx_err (tiz_mutex_unlock (&(gp_event_thread->mutex)));
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_event_stat_stop (tiz_event_stat_t *ap_ev_stat)
{
  (void)get_loop_thread ();
  assert (NULL != ap_ev_stat);
  tiz_check_omx_err (tiz_mutex_lock (&(gp_event_thread->mutex)));
  ev_stat_stop (gp_event_thread->p_loop, (ev_stat *)ap_ev_stat);
  ev_async_send (gp_event_thread->p_loop, gp_event_thread->p_async_watcher);
  tiz_check_omx_err (tiz_mutex_unlock (&(gp_event_thread->mutex)));
  return OMX_ErrorNone;
}

void tiz_event_stat_destroy (tiz_event_stat_t *ap_ev_stat)
{
  (void)get_loop_thread ();
  assert (NULL != ap_ev_stat);
  tiz_event_stat_stop (ap_ev_stat);
  tiz_mem_free (ap_ev_stat);
}

tiz_rcfile_t *tiz_rcfile_get_handle (void)
{
  tiz_loop_thread_t *p_event_thread = get_loop_thread ();
  return (p_event_thread && p_event_thread->p_rcfile) ? p_event_thread->p_rcfile
                                                      : NULL;
}
