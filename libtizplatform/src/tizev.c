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

#include "tizplatform.h"
#include "tizplatform_internal.h"

#define EV_API_STATIC 1
#define EV_STANDALONE 1
#define EV_SIGNAL_ENABLE 1
#define EV_USE_EPOLL 1
#define EV_USE_POLL 0
#define EV_USE_SELECT 0
#define EV_CHECK_ENABLE 0
#define EV_PERIODIC_ENABLE 0
#define EV_CHILD_ENABLE 0
#define EV_EMBED_ENABLE 0
#define EV_IDLE_ENABLE 0
#define EV_CLEANUP_ENABLE 0
#define EV_PREPARE_ENABLE 0
#define EV_FORK_ENABLE 0
#define EV_VERIFY 1

#ifdef HAVE_SYSTEM_LIBEV
#include <ev.h>
#else
#include "ev/ev.c"
#endif

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.event"
#endif

#define TIZ_EVENT_LOOP_THREAD_NAME "evloop"

struct tiz_event_io
{
  ev_io io;
  tiz_event_io_cb_f pf_cback;
  void * p_arg0;
  void * p_arg1;
  bool once;
  uint32_t id;
  int fd;
  bool started;
};

struct tiz_event_timer
{
  ev_timer timer;
  tiz_event_timer_cb_f pf_cback;
  void * p_arg0;
  void * p_arg1;
  bool once;
  uint32_t id;
  bool started;
};

struct tiz_event_stat
{
  ev_stat stat;
  tiz_event_stat_cb_f pf_cback;
  void * p_arg0;
  void * p_arg1;
  uint32_t id;
  bool started;
};

typedef enum tiz_event_loop_state tiz_event_loop_state_t;
enum tiz_event_loop_state
{
  ETIZEventLoopStateInvalid = 0,
  ETIZEventLoopStateStarting,
  ETIZEventLoopStateStarted,
  ETIZEventLoopStateStopping,
  ETIZEventLoopStateStopped
};

typedef struct tiz_event_loop tiz_event_loop_t;
struct tiz_event_loop
{
  tiz_thread_t thread;
  tiz_mutex_t mutex;
  tiz_sem_t sem;
  tiz_pqueue_t * p_pq;
  tiz_soa_t * p_soa;
  ev_async * p_async_watcher;
  struct ev_loop * p_loop;
  tiz_event_loop_state_t state;
  tiz_rcfile_t * p_rcfile;
};

static pthread_once_t g_event_loop_once = PTHREAD_ONCE_INIT;
static tiz_event_loop_t * gp_event_loop = NULL;

typedef enum tiz_event_loop_msg_class tiz_event_loop_msg_class_t;
enum tiz_event_loop_msg_class
{
  ETIZEventLoopMsgIoStart = 0,
  ETIZEventLoopMsgIoStop,
  ETIZEventLoopMsgIoDestroy,
  ETIZEventLoopMsgIoAny,
  ETIZEventLoopMsgTimerStart,
  ETIZEventLoopMsgTimerRestart,
  ETIZEventLoopMsgTimerStop,
  ETIZEventLoopMsgTimerDestroy,
  ETIZEventLoopMsgTimerAny,
  ETIZEventLoopMsgStatStart,
  ETIZEventLoopMsgStatStop,
  ETIZEventLoopMsgStatDestroy,
  ETIZEventLoopMsgStatAny,
  ETIZEventLoopMsgMax,
};

typedef struct tiz_event_loop_msg_io tiz_event_loop_msg_io_t;
struct tiz_event_loop_msg_io
{
  tiz_event_io_t * p_ev_io;
  uint32_t id;
};

typedef struct tiz_event_loop_msg_timer tiz_event_loop_msg_timer_t;
struct tiz_event_loop_msg_timer
{
  tiz_event_timer_t * p_ev_timer;
  uint32_t id;
};

typedef struct tiz_event_loop_msg_stat tiz_event_loop_msg_stat_t;
struct tiz_event_loop_msg_stat
{
  tiz_event_stat_t * p_ev_stat;
  uint32_t id;
};

typedef struct tiz_event_loop_msg tiz_event_loop_msg_t;
struct tiz_event_loop_msg
{
  tiz_event_loop_msg_class_t class;
  OMX_S32 priority;
  union
  {
    tiz_event_loop_msg_io_t io;
    tiz_event_loop_msg_timer_t timer;
    tiz_event_loop_msg_stat_t stat;
  };
};

/* Forward declarations */
static OMX_ERRORTYPE
do_io_start (tiz_event_loop_msg_t *);
static OMX_ERRORTYPE
do_io_stop (tiz_event_loop_msg_t *);
static OMX_ERRORTYPE
do_io_destroy (tiz_event_loop_msg_t *);
static OMX_ERRORTYPE
do_timer_start (tiz_event_loop_msg_t *);
static OMX_ERRORTYPE
do_timer_restart (tiz_event_loop_msg_t *);
static OMX_ERRORTYPE
do_timer_stop (tiz_event_loop_msg_t *);
static OMX_ERRORTYPE
do_timer_destroy (tiz_event_loop_msg_t *);
static OMX_ERRORTYPE
do_stat_start (tiz_event_loop_msg_t *);
static OMX_ERRORTYPE
do_stat_stop (tiz_event_loop_msg_t *);
static OMX_ERRORTYPE
do_stat_destroy (tiz_event_loop_msg_t *);

typedef OMX_ERRORTYPE (*tiz_event_loop_msg_dispatch_f) (
  tiz_event_loop_msg_t * ap_msg);
static const tiz_event_loop_msg_dispatch_f tiz_event_loop_msg_to_fnt_tbl[] = {
  do_io_start,
  do_io_stop,
  do_io_destroy,
  NULL, /* ETIZEventLoopMsgIoAny, no handler */
  do_timer_start,
  do_timer_restart,
  do_timer_stop,
  do_timer_destroy,
  NULL, /* ETIZEventLoopMsgTimerAny, no handler */
  do_stat_start,
  do_stat_stop,
  do_stat_destroy,
  NULL, /* ETIZEventLoopMsgStatAny, no handler */
};

static void
dispatch_msg (tiz_event_loop_msg_t * ap_msg);

typedef struct tiz_event_loop_msg_str tiz_event_loop_msg_str_t;
struct tiz_event_loop_msg_str
{
  tiz_event_loop_msg_class_t msg;
  OMX_STRING str;
};

static tiz_event_loop_msg_str_t tiz_event_loop_msg_to_str_tbl[] = {
  {ETIZEventLoopMsgIoStart, "ETIZEventLoopMsgIoStart"},
  {ETIZEventLoopMsgIoStop, "ETIZEventLoopMsgIoStop"},
  {ETIZEventLoopMsgIoDestroy, "ETIZEventLoopMsgIoDestroy"},
  {ETIZEventLoopMsgIoAny, "ETIZEventLoopMsgIoAny"},
  {ETIZEventLoopMsgTimerStart, "ETIZEventLoopMsgTimerStart"},
  {ETIZEventLoopMsgTimerRestart, "ETIZEventLoopMsgTimerRestart"},
  {ETIZEventLoopMsgTimerStop, "ETIZEventLoopMsgTimerStop"},
  {ETIZEventLoopMsgTimerDestroy, "ETIZEventLoopMsgTimerDestroy"},
  {ETIZEventLoopMsgTimerAny, "ETIZEventLoopMsgTimerAny"},
  {ETIZEventLoopMsgStatStart, "ETIZEventLoopMsgStatStart"},
  {ETIZEventLoopMsgStatStop, "ETIZEventLoopMsgStatStop"},
  {ETIZEventLoopMsgStatDestroy, "ETIZEventLoopMsgStatDestroy"},
  {ETIZEventLoopMsgStatAny, "ETIZEventLoopMsgStatAny"},
  {ETIZEventLoopMsgMax, "ETIZEventLoopMsgMax"},
};

static const OMX_STRING
tiz_event_loop_msg_to_str (const tiz_event_loop_msg_class_t a_msg)
{
  const OMX_S32 count = sizeof (tiz_event_loop_msg_to_str_tbl)
                        / sizeof (tiz_event_loop_msg_str_t);
  OMX_S32 i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_event_loop_msg_to_str_tbl[i].msg == a_msg)
        {
          return tiz_event_loop_msg_to_str_tbl[i].str;
        }
    }

  return "Unknown tizev message";
}

/* NOTE: Start ignoring splint warnings in this section of code */
/*@ignore@*/
static inline tiz_event_loop_msg_t *
init_event_loop_msg (tiz_event_loop_t * ap_event_loop,
                     tiz_event_loop_msg_class_t a_msg_class)
{
  tiz_event_loop_msg_t * p_msg = NULL;

  assert (ap_event_loop);
  assert (a_msg_class < ETIZEventLoopMsgMax);

  if (!(p_msg = (tiz_event_loop_msg_t *) tiz_soa_calloc (
          ap_event_loop->p_soa, sizeof (tiz_event_loop_msg_t))))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "[OMX_ErrorInsufficientResources] : "
               "Creating message [%s]",
               tiz_event_loop_msg_to_str (a_msg_class));
    }
  else
    {
      p_msg->class = a_msg_class;
      switch (a_msg_class)
        {
          case ETIZEventLoopMsgIoStart:
          case ETIZEventLoopMsgTimerStart:
          case ETIZEventLoopMsgTimerRestart:
          case ETIZEventLoopMsgStatStart:
            {
              /* Lowest priority */
              p_msg->priority = 2;
            }
            break;
          case ETIZEventLoopMsgIoStop:
          case ETIZEventLoopMsgTimerStop:
          case ETIZEventLoopMsgStatStop:
            {
              /* Medium priority */
              p_msg->priority = 1;
            }
            break;
          case ETIZEventLoopMsgIoDestroy:
          case ETIZEventLoopMsgTimerDestroy:
          case ETIZEventLoopMsgStatDestroy:
            {
              /* Highest priority */
              p_msg->priority = 0;
            }
            break;
          default:
            {
              assert (0);
            }
            break;
        };
    }

  return p_msg;
}
/*@end@*/
/* NOTE: Stop ignoring splint warnings in this section  */

static OMX_ERRORTYPE
enqueue_io_msg (tiz_event_io_t * ap_ev_io, const uint32_t a_id,
                const tiz_event_loop_msg_class_t a_class)
{
  OMX_ERRORTYPE rc = OMX_ErrorUndefined;
  tiz_event_loop_msg_t * p_msg = NULL;
  tiz_event_loop_msg_io_t * p_msg_io = NULL;

  assert (ap_ev_io);
  assert (ETIZEventLoopMsgIoStart == a_class
          || ETIZEventLoopMsgIoStop == a_class
          || ETIZEventLoopMsgIoDestroy == a_class);

  tiz_check_omx (tiz_mutex_lock (&(gp_event_loop->mutex)));
  tiz_goto_end_on_null (
    (p_msg = init_event_loop_msg (gp_event_loop, (a_class))),
    "Failed to initialise the event loop");

  assert (p_msg);
  p_msg_io = &(p_msg->io);
  p_msg_io->p_ev_io = ap_ev_io;
  p_msg_io->id = a_id;
  tiz_goto_end_on_omx_err (
    (rc = tiz_pqueue_send (gp_event_loop->p_pq, p_msg, p_msg->priority)),
    "Failed to insert into the queue");
  tiz_check_omx (tiz_mutex_unlock (&(gp_event_loop->mutex)));
  ev_async_send (gp_event_loop->p_loop, gp_event_loop->p_async_watcher);

  /* All good */
  rc = OMX_ErrorNone;

 end:

  if (OMX_ErrorNone != rc)
    {
      tiz_check_omx (tiz_mutex_unlock (&(gp_event_loop->mutex)));
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
enqueue_timer_msg (tiz_event_timer_t * ap_ev_timer, const uint32_t a_id,
                   const tiz_event_loop_msg_class_t a_class)
{
  OMX_ERRORTYPE rc = OMX_ErrorUndefined;
  tiz_event_loop_msg_t * p_msg = NULL;
  tiz_event_loop_msg_timer_t * p_msg_timer = NULL;

  assert (ap_ev_timer);
  assert (ETIZEventLoopMsgTimerStart == a_class
          || ETIZEventLoopMsgTimerStop == a_class
          || ETIZEventLoopMsgTimerRestart == a_class
          || ETIZEventLoopMsgTimerDestroy == a_class);

  tiz_check_omx (tiz_mutex_lock (&(gp_event_loop->mutex)));
  tiz_goto_end_on_null (
    (p_msg = init_event_loop_msg (gp_event_loop, (a_class))),
    "Failed to initialise the event loop");

  assert (p_msg);
  p_msg_timer = &(p_msg->timer);
  p_msg_timer->p_ev_timer = ap_ev_timer;
  p_msg_timer->id = a_id;
  tiz_goto_end_on_omx_err (
    (rc = tiz_pqueue_send (gp_event_loop->p_pq, p_msg, p_msg->priority)),
    "Failed to insert into the queue");
  tiz_check_omx (tiz_mutex_unlock (&(gp_event_loop->mutex)));
  ev_async_send (gp_event_loop->p_loop, gp_event_loop->p_async_watcher);

  /* All good */
  rc = OMX_ErrorNone;

 end:

  if (OMX_ErrorNone != rc)
    {
      tiz_check_omx (tiz_mutex_unlock (&(gp_event_loop->mutex)));
    }

  return rc;
}

static OMX_ERRORTYPE
enqueue_stat_msg (tiz_event_stat_t * ap_ev_stat, const uint32_t a_id,
                  const tiz_event_loop_msg_class_t a_class)
{
  OMX_ERRORTYPE rc = OMX_ErrorUndefined;
  tiz_event_loop_msg_t * p_msg = NULL;
  tiz_event_loop_msg_stat_t * p_msg_stat = NULL;

  assert (ap_ev_stat);
  assert (ETIZEventLoopMsgStatStart == a_class
          || ETIZEventLoopMsgStatStop == a_class
          || ETIZEventLoopMsgStatDestroy == a_class);

  tiz_check_omx (tiz_mutex_lock (&(gp_event_loop->mutex)));
  tiz_goto_end_on_null ((p_msg = init_event_loop_msg (gp_event_loop, (a_class))),
                        "Failed to initialise the event loop");

  assert (p_msg);
  p_msg_stat = &(p_msg->stat);
  p_msg_stat->p_ev_stat = ap_ev_stat;
  p_msg_stat->id = a_id;
  tiz_goto_end_on_omx_err (
    (rc = tiz_pqueue_send (gp_event_loop->p_pq, p_msg, p_msg->priority)),
    "Failed to insert into the queue");
  tiz_check_omx (tiz_mutex_unlock (&(gp_event_loop->mutex)));
  ev_async_send (gp_event_loop->p_loop, gp_event_loop->p_async_watcher);

  /* All good */
  rc = OMX_ErrorNone;

 end:

  if (OMX_ErrorNone != rc)
    {
      tiz_check_omx (tiz_mutex_unlock (&(gp_event_loop->mutex)));
    }

  return OMX_ErrorNone;
}

static void
dispatch_msg (tiz_event_loop_msg_t * ap_msg)
{
  assert (ap_msg);
  assert (ap_msg->class < ETIZEventLoopMsgMax);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "msg [%p] class [%s]", ap_msg,
           tiz_event_loop_msg_to_str (ap_msg->class));

  (void) tiz_event_loop_msg_to_fnt_tbl[ap_msg->class](ap_msg);
}

static OMX_S32
pqueue_cmp (OMX_PTR ap_left, OMX_PTR ap_right)
{
  /* Not planning to use tiz_pqueue_remove or tiz_pqueue_removep */
  assert (0);
  return 1;
}

static OMX_BOOL
ev_io_msg_dequeue (void * ap_elem, OMX_S32 a_data1, void * ap_data2)
{
  OMX_BOOL rc = OMX_FALSE;
  tiz_event_loop_msg_t * p_msg = ap_elem;
  const tiz_event_loop_msg_class_t class_to_delete = a_data1;
  tiz_event_loop_msg_class_t elem_class = ETIZEventLoopMsgMax;
  bool elem_class_is_io = false;

  assert (p_msg);
  assert (ETIZEventLoopMsgIoStart == class_to_delete
          || ETIZEventLoopMsgIoStop == class_to_delete
          || ETIZEventLoopMsgIoDestroy == class_to_delete
          || ETIZEventLoopMsgIoAny == class_to_delete);

  elem_class = p_msg->class;
  elem_class_is_io = (ETIZEventLoopMsgIoStart == elem_class
                      || ETIZEventLoopMsgIoStop == elem_class
                      || ETIZEventLoopMsgIoDestroy == elem_class);

  if (class_to_delete == elem_class
      || (elem_class_is_io && class_to_delete == ETIZEventLoopMsgIoAny))
    {
      tiz_event_loop_msg_io_t * p_msg_io = NULL;
      tiz_event_io_t * p_ev_io = NULL;
      p_msg_io = &(p_msg->io);
      assert (p_msg_io);
      p_ev_io = p_msg_io->p_ev_io;
      assert (p_ev_io);
      if (ap_data2 == p_ev_io)
        {
          tiz_event_io_t * p_ev_io_needle = ap_data2;
          if (p_ev_io_needle->id == p_msg_io->id)
            {
              /* Found, return TRUE so that the msg will be removed from the
                 queue */
              rc = OMX_TRUE;
            }
        }
    }
  return rc;
}

static OMX_BOOL
ev_timer_msg_dequeue (void * ap_elem, OMX_S32 a_data1, void * ap_data2)
{
  OMX_BOOL rc = OMX_FALSE;
  tiz_event_loop_msg_t * p_msg = ap_elem;
  const tiz_event_loop_msg_class_t class_to_delete = a_data1;
  tiz_event_loop_msg_class_t elem_class = ETIZEventLoopMsgMax;
  bool elem_class_is_timer = false;

  assert (p_msg);
  assert (ETIZEventLoopMsgTimerStart == class_to_delete
          || ETIZEventLoopMsgTimerStop == class_to_delete
          || ETIZEventLoopMsgTimerDestroy == class_to_delete
          || ETIZEventLoopMsgTimerAny == class_to_delete);

  elem_class = p_msg->class;
  elem_class_is_timer = (ETIZEventLoopMsgTimerStart == elem_class
                         || ETIZEventLoopMsgTimerStop == elem_class
                         || ETIZEventLoopMsgTimerDestroy == elem_class);

  if (class_to_delete == p_msg->class || (elem_class_is_timer
                                          && class_to_delete
                                               == ETIZEventLoopMsgTimerAny))
    {
      tiz_event_loop_msg_timer_t * p_msg_timer = NULL;
      tiz_event_timer_t * p_ev_timer = NULL;
      p_msg_timer = &(p_msg->timer);
      assert (p_msg_timer);
      p_ev_timer = p_msg_timer->p_ev_timer;
      assert (p_ev_timer);
      if (ap_data2 == p_ev_timer)
        {
          tiz_event_timer_t * p_ev_timer_needle = ap_data2;
          if (p_ev_timer_needle->id == p_msg_timer->id)
            {
              /* Found, return TRUE so that the msg will be removed from the
                 queue */
              rc = OMX_TRUE;
            }
        }
    }
  return rc;
}

static OMX_BOOL
ev_stat_msg_dequeue (void * ap_elem, OMX_S32 a_data1, void * ap_data2)
{
  OMX_BOOL rc = OMX_FALSE;
  tiz_event_loop_msg_t * p_msg = ap_elem;
  const tiz_event_loop_msg_class_t class_to_delete = a_data1;
  tiz_event_loop_msg_class_t elem_class = ETIZEventLoopMsgMax;
  bool elem_class_is_stat = false;

  assert (p_msg);
  assert (ETIZEventLoopMsgStatStart == class_to_delete
          || ETIZEventLoopMsgStatStop == class_to_delete
          || ETIZEventLoopMsgStatDestroy == class_to_delete
          || ETIZEventLoopMsgStatAny == class_to_delete);

  elem_class = p_msg->class;
  elem_class_is_stat = (ETIZEventLoopMsgStatStart == elem_class
                        || ETIZEventLoopMsgStatStop == elem_class
                        || ETIZEventLoopMsgStatDestroy == elem_class);

  if (class_to_delete == p_msg->class || (elem_class_is_stat
                                          && class_to_delete
                                               == ETIZEventLoopMsgStatAny))
    {
      tiz_event_loop_msg_stat_t * p_msg_stat = NULL;
      tiz_event_stat_t * p_ev_stat = NULL;
      p_msg_stat = &(p_msg->stat);
      assert (p_msg_stat);
      p_ev_stat = p_msg_stat->p_ev_stat;
      assert (p_ev_stat);
      if (ap_data2 == p_ev_stat)
        {
          tiz_event_stat_t * p_ev_stat_needle = ap_data2;
          if (p_ev_stat_needle->id == p_msg_stat->id)
            {
              /* Found, return TRUE so that the msg will be removed from the
                 queue */
              rc = OMX_TRUE;
            }
        }
    }
  return rc;
}

static OMX_ERRORTYPE
do_io_start (tiz_event_loop_msg_t * ap_msg)
{
  tiz_event_loop_msg_io_t * p_msg_io = NULL;
  tiz_event_io_t * p_ev_io = NULL;

  assert (gp_event_loop);
  assert (ap_msg);
  assert (ETIZEventLoopStateStarted == gp_event_loop->state);

  p_msg_io = &(ap_msg->io);
  assert (p_msg_io);
  p_ev_io = p_msg_io->p_ev_io;
  assert (p_ev_io);
  /* debug: Verify that ids don't get repeated */
  if (p_ev_io->id != 0 && p_ev_io->id == p_msg_io->id)
    {
      assert (p_ev_io->id != p_msg_io->id);
    }
  p_ev_io->id = p_msg_io->id;
  /* debug: Verify that we are starting a watcher that was stopped */
  if (p_ev_io->started)
    {
      assert (!p_ev_io->started);
    }
  p_ev_io->started = true;
  ev_io_start (gp_event_loop->p_loop, (ev_io *) (p_ev_io));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_io_stop (tiz_event_loop_msg_t * ap_msg)
{
  tiz_event_loop_msg_io_t * p_msg_io = NULL;
  tiz_event_io_t * p_ev_io = NULL;

  assert (gp_event_loop);
  assert (ap_msg);
  assert (ETIZEventLoopStateStarted == gp_event_loop->state);

  p_msg_io = &(ap_msg->io);
  assert (p_msg_io);
  p_ev_io = p_msg_io->p_ev_io;
  assert (p_ev_io);
  if (p_ev_io->started)
    {
      /* The io watcher has been started, let's stop it */
      ev_io_stop (gp_event_loop->p_loop, (ev_io *) (p_ev_io));
      p_ev_io->started = false;
    }
  else
    {
      /* This io watcher hasn't been started, let's make sure there are no
         start requests left behind in the queue */
      const tiz_event_loop_msg_class_t class_to_be_deleted
        = ETIZEventLoopMsgIoStart;
      tiz_pqueue_remove_func (gp_event_loop->p_pq, ev_io_msg_dequeue,
                              (OMX_S32) class_to_be_deleted, p_ev_io);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_io_destroy (tiz_event_loop_msg_t * ap_msg)
{
  tiz_event_loop_msg_io_t * p_msg_io = NULL;
  tiz_event_io_t * p_ev_io = NULL;

  assert (gp_event_loop);
  assert (ap_msg);
  assert (ETIZEventLoopStateStarted == gp_event_loop->state);

  p_msg_io = &(ap_msg->io);
  assert (p_msg_io);
  p_ev_io = p_msg_io->p_ev_io;
  assert (p_ev_io);
  if (p_ev_io->started)
    {
      /* The io watcher has been started, let's stop it */
      ev_io_stop (gp_event_loop->p_loop, (ev_io *) (p_ev_io));
    }

  {
    /* Now remove any references to this watcher that might be present in the
       queue */
    tiz_event_loop_msg_class_t class_to_be_deleted = ETIZEventLoopMsgIoAny;
    tiz_pqueue_remove_func (gp_event_loop->p_pq, ev_io_msg_dequeue,
                            (OMX_S32) class_to_be_deleted, p_ev_io);
  }

  /* And now it should be safe to delete the io event */
  tiz_mem_free (p_ev_io);
  p_msg_io->p_ev_io = NULL;

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_timer_start (tiz_event_loop_msg_t * ap_msg)
{
  tiz_event_loop_msg_timer_t * p_msg_timer = NULL;
  tiz_event_timer_t * p_ev_timer = NULL;

  assert (gp_event_loop);
  assert (ap_msg);
  assert (ETIZEventLoopStateStarted == gp_event_loop->state);

  p_msg_timer = &(ap_msg->timer);
  assert (p_msg_timer);
  p_ev_timer = p_msg_timer->p_ev_timer;
  assert (p_ev_timer);
  /* debug: Verify that ids don't get repeated */
  if (p_ev_timer->id != 0 && p_ev_timer->id == p_msg_timer->id)
    {
      assert (p_ev_timer->id != p_msg_timer->id);
    }
  p_ev_timer->id = p_msg_timer->id;
  p_ev_timer->started = true;
  ev_timer_start (gp_event_loop->p_loop, (ev_timer *) (p_ev_timer));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_timer_restart (tiz_event_loop_msg_t * ap_msg)
{
  tiz_event_loop_msg_timer_t * p_msg_timer = NULL;
  tiz_event_timer_t * p_ev_timer = NULL;

  assert (gp_event_loop);
  assert (ap_msg);
  assert (ETIZEventLoopStateStarted == gp_event_loop->state);

  p_msg_timer = &(ap_msg->timer);
  assert (p_msg_timer);
  p_ev_timer = p_msg_timer->p_ev_timer;
  assert (p_ev_timer);
  /* debug: Verify that ids don't get repeated */
  if (p_ev_timer->id != 0 && p_ev_timer->id == p_msg_timer->id)
    {
      assert (p_ev_timer->id != p_msg_timer->id);
    }
  p_ev_timer->id = p_msg_timer->id;
  p_ev_timer->started = true;
  ev_timer_again (gp_event_loop->p_loop, (ev_timer *) (p_ev_timer));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_timer_stop (tiz_event_loop_msg_t * ap_msg)
{
  tiz_event_loop_msg_timer_t * p_msg_timer = NULL;
  tiz_event_timer_t * p_ev_timer = NULL;

  assert (gp_event_loop);
  assert (ap_msg);
  assert (ETIZEventLoopStateStarted == gp_event_loop->state);

  p_msg_timer = &(ap_msg->timer);
  assert (p_msg_timer);
  p_ev_timer = p_msg_timer->p_ev_timer;
  assert (p_ev_timer);
  if (p_ev_timer->started)
    {
      /* The timer watcher has been started, let's stop it */
      ev_timer_stop (gp_event_loop->p_loop, (ev_timer *) (p_ev_timer));
      p_ev_timer->started = false;
    }
  else
    {
      /* The timer watcher hasn't been started, let's make sure there are no
         start
         requests in the queue */
      const tiz_event_loop_msg_class_t class_to_be_deleted
        = ETIZEventLoopMsgTimerStart;
      tiz_pqueue_remove_func (gp_event_loop->p_pq, ev_timer_msg_dequeue,
                              (OMX_S32) class_to_be_deleted, p_ev_timer);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_timer_destroy (tiz_event_loop_msg_t * ap_msg)
{
  tiz_event_loop_msg_timer_t * p_msg_timer = NULL;
  tiz_event_timer_t * p_ev_timer = NULL;

  assert (gp_event_loop);
  assert (ap_msg);
  assert (ETIZEventLoopStateStarted == gp_event_loop->state);

  p_msg_timer = &(ap_msg->timer);
  assert (p_msg_timer);
  p_ev_timer = p_msg_timer->p_ev_timer;
  assert (p_ev_timer);
  if (p_ev_timer->started)
    {
      /* The timer watcher has been started, let's stop it */
      ev_timer_stop (gp_event_loop->p_loop, (ev_timer *) (p_ev_timer));
    }
  {
    /* Now remove any references to this watcher that might be present in the
       queue */
    tiz_event_loop_msg_class_t class_to_be_deleted = ETIZEventLoopMsgTimerAny;
    tiz_pqueue_remove_func (gp_event_loop->p_pq, ev_timer_msg_dequeue,
                            (OMX_S32) class_to_be_deleted, p_ev_timer);
  }

  /* And now it should be safe to delete the timer event */
  tiz_mem_free (p_ev_timer);
  p_msg_timer->p_ev_timer = NULL;

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_stat_start (tiz_event_loop_msg_t * ap_msg)
{
  tiz_event_loop_msg_stat_t * p_msg_stat = NULL;
  tiz_event_stat_t * p_ev_stat = NULL;

  assert (gp_event_loop);
  assert (ap_msg);
  assert (ETIZEventLoopStateStarted == gp_event_loop->state);

  p_msg_stat = &(ap_msg->stat);
  assert (p_msg_stat);
  p_ev_stat = p_msg_stat->p_ev_stat;
  assert (p_ev_stat);
  /* debug: Verify that ids don't get repeated */
  if (p_ev_stat->id != 0 && p_ev_stat->id == p_msg_stat->id)
    {
      assert (p_ev_stat->id != p_msg_stat->id);
    }
  p_ev_stat->id = p_msg_stat->id;
  /* debug: Verify that we are starting a watcher that was stopped */
  if (p_ev_stat->started)
    {
      assert (!p_ev_stat->started);
    }
  p_ev_stat->started = true;
  ev_stat_start (gp_event_loop->p_loop, (ev_stat *) (p_ev_stat));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_stat_stop (tiz_event_loop_msg_t * ap_msg)
{
  tiz_event_loop_msg_stat_t * p_msg_stat = NULL;
  tiz_event_stat_t * p_ev_stat = NULL;

  assert (gp_event_loop);
  assert (ap_msg);
  assert (ETIZEventLoopStateStarted == gp_event_loop->state);

  p_msg_stat = &(ap_msg->stat);
  assert (p_msg_stat);
  p_ev_stat = p_msg_stat->p_ev_stat;
  assert (p_ev_stat);
  if (p_ev_stat->started)
    {
      /* The stat watcher has been started, let's stop it */
      ev_stat_stop (gp_event_loop->p_loop, (ev_stat *) (p_ev_stat));
      p_ev_stat->started = false;
    }
  else
    {
      /* The stat watcher hasn't been started, let's make sure there are no
         start
         requests in the queue */
      const tiz_event_loop_msg_class_t class_to_be_deleted
        = ETIZEventLoopMsgStatStart;
      tiz_pqueue_remove_func (gp_event_loop->p_pq, ev_stat_msg_dequeue,
                              (OMX_S32) class_to_be_deleted, p_ev_stat);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_stat_destroy (tiz_event_loop_msg_t * ap_msg)
{
  tiz_event_loop_msg_stat_t * p_msg_stat = NULL;
  tiz_event_stat_t * p_ev_stat = NULL;

  assert (gp_event_loop);
  assert (ap_msg);
  assert (ETIZEventLoopStateStarted == gp_event_loop->state);

  p_msg_stat = &(ap_msg->stat);
  assert (p_msg_stat);
  p_ev_stat = p_msg_stat->p_ev_stat;
  assert (p_ev_stat);
  if (p_ev_stat->started)
    {
      /* The stat watcher has been started, let's stop it */
      ev_stat_stop (gp_event_loop->p_loop, (ev_stat *) (p_ev_stat));
    }

  {
    /* Now remove any references to this watcher that might be present in the
       queue */
    tiz_event_loop_msg_class_t class_to_be_deleted = ETIZEventLoopMsgStatAny;
    tiz_pqueue_remove_func (gp_event_loop->p_pq, ev_stat_msg_dequeue,
                            (OMX_S32) class_to_be_deleted, p_ev_stat);
  }

  /* And now it should be safe to delete the stat event */
  tiz_mem_free (p_msg_stat->p_ev_stat);
  p_msg_stat->p_ev_stat = NULL;
  return OMX_ErrorNone;
}

static void
async_watcher_cback (struct ev_loop * ap_loop, ev_async * ap_watcher,
                     int a_revents)
{
  (void) ap_loop;
  (void) ap_watcher;
  (void) a_revents;

  if (gp_event_loop)
    {
      if (ETIZEventLoopStateStopping == gp_event_loop->state)
        {
          ev_break (gp_event_loop->p_loop, EVBREAK_ONE);
        }
      else if (ETIZEventLoopStateStarted == gp_event_loop->state)
        {
          void * p_msg = NULL;

          /* Process all items from the queue */
          (void) tiz_mutex_lock (&(gp_event_loop->mutex));
          while (0 < tiz_pqueue_length (gp_event_loop->p_pq))
            {
              if (OMX_ErrorNone
                  != tiz_pqueue_receive (gp_event_loop->p_pq, &p_msg))
                {
                  break;
                }
              /* Process the message */
              dispatch_msg (p_msg);
              /* Delete the message */
              tiz_soa_free (gp_event_loop->p_soa, p_msg);
            }
          (void) tiz_mutex_unlock (&(gp_event_loop->mutex));
        }
    }
}

static void
io_watcher_cback (struct ev_loop * ap_loop, ev_io * ap_watcher, int a_revents)
{
  tiz_event_io_t * p_io_event = (tiz_event_io_t *) ap_watcher;
  (void) ap_loop;

  if (gp_event_loop)
    {
      assert (p_io_event);
      assert (p_io_event->pf_cback);

      if (p_io_event->once)
        {
          p_io_event->started = false;
          ev_io_stop (gp_event_loop->p_loop, (ev_io *) p_io_event);
        }
      p_io_event->pf_cback (p_io_event->p_arg0, p_io_event, p_io_event->p_arg1,
                            p_io_event->id, ((ev_io *) p_io_event)->fd,
                            a_revents);
    }
}

static void
timer_watcher_cback (struct ev_loop * ap_loop, ev_timer * ap_watcher,
                     int a_revents)
{
  (void) ap_loop;
  (void) a_revents;

  if (gp_event_loop)
    {
      tiz_event_timer_t * p_timer_event = (tiz_event_timer_t *) ap_watcher;
      assert (p_timer_event);
      assert (p_timer_event->pf_cback);
      p_timer_event->pf_cback (p_timer_event->p_arg0, p_timer_event,
                               p_timer_event->p_arg1, p_timer_event->id);
    }
}

static void
stat_watcher_cback (struct ev_loop * ap_loop, ev_stat * ap_watcher,
                    int a_revents)
{
  (void) ap_loop;

  if (gp_event_loop)
    {
      tiz_event_stat_t * p_stat_event = (tiz_event_stat_t *) ap_watcher;
      assert (p_stat_event);
      assert (p_stat_event->pf_cback);
      p_stat_event->pf_cback (p_stat_event->p_arg0, p_stat_event,
                              p_stat_event->p_arg1, p_stat_event->id,
                              a_revents);
    }
}

static void *
event_loop_thread_func (void * p_arg)
{
  tiz_event_loop_t * p_event_loop = p_arg;
  struct ev_loop * p_loop = NULL;

  assert (p_event_loop);

  p_loop = p_event_loop->p_loop;
  assert (p_loop);

  (void) tiz_thread_setname (&(p_event_loop->thread),
                             (const OMX_STRING) TIZ_EVENT_LOOP_THREAD_NAME);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Entering the dispatcher...");
  tiz_sem_post (&(p_event_loop->sem));

  ev_run (p_loop, 0);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Have left the dispatcher, thread exiting...");

  return NULL;
}

static inline void
clean_up_thread_data (tiz_event_loop_t * ap_lp)
{
  if (ap_lp)
    {
      if (ap_lp->p_async_watcher)
        {
          tiz_mem_free (ap_lp->p_async_watcher);
          ap_lp->p_async_watcher = NULL;
        }

      if (ap_lp->p_loop)
        {
          ev_loop_destroy (ap_lp->p_loop);
          ap_lp->p_loop = NULL;
        }

      if (ap_lp->mutex)
        {
          (void) tiz_mutex_destroy (&(ap_lp->mutex));
          ap_lp->mutex = NULL;
        }

      if (ap_lp->sem)
        {
          (void) tiz_sem_destroy (&(ap_lp->sem));
          ap_lp->sem = NULL;
        }

      if (ap_lp->p_pq)
        {
          tiz_pqueue_destroy (ap_lp->p_pq);
          ap_lp->p_pq = NULL;
        }

      if (ap_lp->p_soa)
        {
          tiz_soa_destroy (ap_lp->p_soa);
          ap_lp->p_soa = NULL;
        }

      tiz_mem_free (gp_event_loop);
      gp_event_loop = NULL;
    }
}

static void
child_event_loop_reset (void)
{
  /* Reset the once control */
  pthread_once_t once = PTHREAD_ONCE_INIT;
  memcpy (&g_event_loop_once, &once, sizeof (g_event_loop_once));
  gp_event_loop = NULL;
}

static void
init_event_loop_thread (void)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  if (!gp_event_loop)
    {
      /* Let's return OOM error if something goes wrong */
      rc = OMX_ErrorInsufficientResources;

      /* Register a handler to reset the pthread_once_t global variable to try
         to cope with the scenario of a process forking without exec. The idea
         is to make sure that the loop thread is re-created in the child
         process */
      pthread_atfork (NULL, NULL, child_event_loop_reset);

      tiz_goto_end_on_null (
        (gp_event_loop
         = (tiz_event_loop_t *) tiz_mem_calloc (1, sizeof (tiz_event_loop_t))),
        "Error allocating thread data struct.");

      gp_event_loop->state = ETIZEventLoopStateStarting;

      tiz_goto_end_on_omx_err (tiz_rcfile_init (&(gp_event_loop->p_rcfile)),
                           "Error opening configuration file.");

      tiz_goto_end_on_null ((gp_event_loop->p_loop = ev_loop_new (EVFLAG_AUTO)),
                            "Error instantiating ev_loop.");

      tiz_goto_end_on_null ((gp_event_loop->p_async_watcher
                            = (ev_async *) tiz_mem_calloc (1, sizeof (ev_async))),
                           "Error initializing async watcher.");

      tiz_goto_end_on_omx_err (tiz_mutex_init (&(gp_event_loop->mutex)),
                           "Error initializing mutex.");

      tiz_goto_end_on_omx_err (tiz_sem_init (&(gp_event_loop->sem), 0),
                           "Error initializing sem.");

      /* Init the small object allocator */
      tiz_goto_end_on_omx_err (tiz_soa_init (&(gp_event_loop->p_soa)),
                           "Error initializing the small object allocator.");

      /* Init the priority queue */
      tiz_goto_end_on_omx_err (
        tiz_pqueue_init (&gp_event_loop->p_pq, 2, &pqueue_cmp,
                         gp_event_loop->p_soa, TIZ_EVENT_LOOP_THREAD_NAME),
        "Error initializing pqueue.");

      /* All good */
      rc = OMX_ErrorNone;

      ev_async_init (gp_event_loop->p_async_watcher, async_watcher_cback);
      ev_async_start (gp_event_loop->p_loop, gp_event_loop->p_async_watcher);

      assert (gp_event_loop);
    }

end:

  if (OMX_ErrorNone == rc)
    {
      tiz_event_loop_t * p_lp = gp_event_loop;
      p_lp->state = ETIZEventLoopStateStarted;
      /* Create event loop thread */
      tiz_thread_create (&(p_lp->thread), 0, 0, event_loop_thread_func, p_lp);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Now in ETIZEventLoopStateStarted state...");

      (void) tiz_mutex_lock (&(p_lp->mutex));
      /* This is to prevent the event loop from exiting when there are no
       * more active events */
      ev_ref (p_lp->p_loop);
      (void) tiz_mutex_unlock (&(p_lp->mutex));
      tiz_sem_wait (&(p_lp->sem));
    }
  else
    {
      clean_up_thread_data (gp_event_loop);
      tiz_mem_free (gp_event_loop);
      gp_event_loop = NULL;
    }
}

static inline tiz_event_loop_t *
get_event_loop (void)
{
  (void) pthread_once (&g_event_loop_once, init_event_loop_thread);
  return gp_event_loop;
}

OMX_ERRORTYPE
tiz_event_loop_init (void)
{
  return get_event_loop () ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
}

void
tiz_event_loop_destroy (void)
{
  /* NOTE: If the thread is destroyed, it can't be recreated in the same
     process as it's been instantiated with pthread_once. */

  if (gp_event_loop)
    {
      (void) tiz_mutex_lock (&(gp_event_loop->mutex));
      TIZ_LOG (TIZ_PRIORITY_TRACE, "destroying event loop thread [%p].",
               gp_event_loop);
      gp_event_loop->state = ETIZEventLoopStateStopping;
      ev_unref (gp_event_loop->p_loop);
      ev_async_send (gp_event_loop->p_loop, gp_event_loop->p_async_watcher);
      (void) tiz_mutex_unlock (&(gp_event_loop->mutex));

      {
        OMX_PTR p_result = NULL;
        tiz_thread_join (&(gp_event_loop->thread), &p_result);
        clean_up_thread_data (gp_event_loop);
        tiz_mem_free (gp_event_loop);
        gp_event_loop = NULL;
      }
    }
}

/*
 * IO Event-related functions
 */

OMX_ERRORTYPE
tiz_event_io_init (tiz_event_io_t ** app_ev_io, void * ap_arg0,
                   tiz_event_io_cb_f ap_cback, void * ap_arg1)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  tiz_event_io_t * p_ev_io = NULL;

  assert (app_ev_io);
  assert (ap_cback);
  (void) get_event_loop ();

  if ((p_ev_io
       = (tiz_event_io_t *) tiz_mem_calloc (1, sizeof (tiz_event_io_t))))
    {
      p_ev_io->pf_cback = ap_cback;
      p_ev_io->p_arg0 = ap_arg0;
      p_ev_io->p_arg1 = ap_arg1;
      p_ev_io->once = false;
      p_ev_io->id = 0;
      p_ev_io->fd = -1;
      p_ev_io->started = false;
      ev_init ((ev_io *) p_ev_io, io_watcher_cback);
      rc = OMX_ErrorNone;
    }

  *app_ev_io = p_ev_io;

  return rc;
}

void
tiz_event_io_set (tiz_event_io_t * ap_ev_io, int a_fd,
                  tiz_event_io_event_t a_event, bool only_once)
{
  (void) get_event_loop ();
  assert (ap_ev_io);
  assert (a_fd > 0);
  assert (a_event < TIZ_EVENT_MAX);
  ap_ev_io->once = only_once;
  ap_ev_io->fd = a_fd;
  ev_io_set ((ev_io *) ap_ev_io, a_fd, a_event);
}

OMX_ERRORTYPE
tiz_event_io_start (tiz_event_io_t * ap_ev_io, const uint32_t a_id)
{
  assert (ap_ev_io);
  (void) get_event_loop ();
  return enqueue_io_msg (ap_ev_io, a_id, ETIZEventLoopMsgIoStart);
}

OMX_ERRORTYPE
tiz_event_io_stop (tiz_event_io_t * ap_ev_io)
{
  assert (ap_ev_io);
  (void) get_event_loop ();
  return enqueue_io_msg (ap_ev_io, ap_ev_io->id, ETIZEventLoopMsgIoStop);
}

bool
tiz_event_io_is_level_triggered (tiz_event_io_t * ap_ev_io)
{
  assert (ap_ev_io);
  return ap_ev_io->once;
}

void
tiz_event_io_destroy (tiz_event_io_t * ap_ev_io)
{
  if (ap_ev_io)
    {
      (void) get_event_loop ();
      (void) enqueue_io_msg (ap_ev_io, ap_ev_io->id, ETIZEventLoopMsgIoDestroy);
    }
}

/*
 * Timer Event-related functions
 */

OMX_ERRORTYPE
tiz_event_timer_init (tiz_event_timer_t ** app_ev_timer, void * ap_arg0,
                      tiz_event_timer_cb_f ap_cback, void * ap_arg1)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  tiz_event_timer_t * p_ev_timer = NULL;

  assert (app_ev_timer);
  assert (ap_cback);
  (void) get_event_loop ();

  if ((p_ev_timer
       = (tiz_event_timer_t *) tiz_mem_calloc (1, sizeof (tiz_event_timer_t))))
    {
      p_ev_timer->pf_cback = ap_cback;
      p_ev_timer->p_arg0 = ap_arg0;
      p_ev_timer->p_arg1 = ap_arg1;
      p_ev_timer->once = false;
      p_ev_timer->id = 0;
      p_ev_timer->started = false;
      ev_init ((ev_timer *) p_ev_timer, timer_watcher_cback);
      rc = OMX_ErrorNone;
    }

  *app_ev_timer = p_ev_timer;

  return rc;
}

void
tiz_event_timer_set (tiz_event_timer_t * ap_ev_timer, double a_after,
                     double a_repeat)
{
  assert (ap_ev_timer);
  (void) get_event_loop ();
  ap_ev_timer->once = a_repeat ? false : true;
  ev_timer_set ((ev_timer *) ap_ev_timer, a_after, a_repeat);
}

OMX_ERRORTYPE
tiz_event_timer_start (tiz_event_timer_t * ap_ev_timer, const uint32_t a_id)
{
  assert (ap_ev_timer);
  (void) get_event_loop ();
  return enqueue_timer_msg (ap_ev_timer, a_id, ETIZEventLoopMsgTimerStart);
}

OMX_ERRORTYPE
tiz_event_timer_restart (tiz_event_timer_t * ap_ev_timer, const uint32_t a_id)
{
  assert (ap_ev_timer);
  (void) get_event_loop ();
  return enqueue_timer_msg (ap_ev_timer, a_id, ETIZEventLoopMsgTimerRestart);
}

OMX_ERRORTYPE
tiz_event_timer_stop (tiz_event_timer_t * ap_ev_timer)
{
  assert (ap_ev_timer);
  (void) get_event_loop ();
  return enqueue_timer_msg (ap_ev_timer, ap_ev_timer->id,
                            ETIZEventLoopMsgTimerStop);
}

bool
tiz_event_timer_is_repeat (tiz_event_timer_t * ap_ev_timer)
{
  assert (ap_ev_timer);
  return !ap_ev_timer->once;
}

void
tiz_event_timer_destroy (tiz_event_timer_t * ap_ev_timer)
{
  if (ap_ev_timer)
    {
      (void) get_event_loop ();
      (void) enqueue_timer_msg (ap_ev_timer, ap_ev_timer->id,
                                ETIZEventLoopMsgTimerDestroy);
    }
}

/*
 * File status Event-related functions
 */

OMX_ERRORTYPE
tiz_event_stat_init (tiz_event_stat_t ** app_ev_stat, void * ap_arg0,
                     tiz_event_stat_cb_f ap_cback, void * ap_arg1)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  tiz_event_stat_t * p_ev_stat = NULL;

  assert (app_ev_stat);
  assert (ap_cback);
  (void) get_event_loop ();

  if ((p_ev_stat
       = (tiz_event_stat_t *) tiz_mem_calloc (1, sizeof (tiz_event_stat_t))))
    {
      p_ev_stat->pf_cback = ap_cback;
      p_ev_stat->p_arg0 = ap_arg0;
      p_ev_stat->p_arg1 = ap_arg1;
      p_ev_stat->id = 0;
      p_ev_stat->started = false;
      ev_init ((ev_stat *) p_ev_stat, stat_watcher_cback);
      rc = OMX_ErrorNone;
    }

  *app_ev_stat = p_ev_stat;

  return rc;
}

void
tiz_event_stat_set (tiz_event_stat_t * ap_ev_stat, const char * ap_path)
{
  (void) get_event_loop ();
  assert (ap_ev_stat);
  ev_stat_set ((ev_stat *) ap_ev_stat, ap_path, 0);
}

OMX_ERRORTYPE
tiz_event_stat_start (tiz_event_stat_t * ap_ev_stat, const uint32_t a_id)
{
  assert (ap_ev_stat);
  (void) get_event_loop ();
  return enqueue_stat_msg (ap_ev_stat, a_id, ETIZEventLoopMsgStatStart);
}

OMX_ERRORTYPE
tiz_event_stat_stop (tiz_event_stat_t * ap_ev_stat)
{
  assert (ap_ev_stat);
  (void) get_event_loop ();
  return enqueue_stat_msg (ap_ev_stat, ap_ev_stat->id,
                           ETIZEventLoopMsgStatStop);
}

void
tiz_event_stat_destroy (tiz_event_stat_t * ap_ev_stat)
{
  if (ap_ev_stat)
    {
      (void) get_event_loop ();
      (void) enqueue_stat_msg (ap_ev_stat, ap_ev_stat->id,
                               ETIZEventLoopMsgStatDestroy);
    }
}

tiz_rcfile_t *
tiz_rcfile_get_handle (void)
{
  tiz_event_loop_t * p_event_loop = get_event_loop ();
  return (p_event_loop && p_event_loop->p_rcfile) ? p_event_loop->p_rcfile
                                                  : NULL;
}
