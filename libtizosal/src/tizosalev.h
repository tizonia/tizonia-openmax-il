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
 * @file   tizosalev.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - Event loop, async io and timers
 *
 *
 */

#ifndef TIZOSALEV_H
#define TIZOSALEV_H

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @defgroup event Event loop utilities
   * @ingroup Tizonia-OSAL
   */

#include <OMX_Core.h>
#include <OMX_Types.h>

  /**
   * Handle to an io event
   * @ingroup event
   */
  typedef struct tiz_event_io tiz_event_io_t;

  /**
   * Handle to a timer event
   * @ingroup event
   */
  typedef struct tiz_event_timer tiz_event_timer_t;

  /**
   * Handle to a file status change event
   * @ingroup event
   */
  typedef struct tiz_event_stat tiz_event_stat_t;


  /**
   * Callback prototype for io events
   *
   * @param p_ev_io The io event being notified
   *
   * @param fd The associated file descriptor
   *
   * @param events The event types notified
   * 
   * @ingroup event
   */
  typedef void (*tiz_event_io_cb_f) (OMX_HANDLETYPE ap_hdl,
                                     tiz_event_io_t * ap_ev_io,
                                     int a_fd, int a_events);

  typedef void (*tiz_event_timer_cb_f) (OMX_HANDLETYPE ap_hdl,
                                        tiz_event_timer_t * ap_ev_timer,
                                        void * ap_arg);

  typedef void (*tiz_event_stat_cb_f) (OMX_HANDLETYPE ap_hdl,
                                       tiz_event_stat_t * ap_ev_stat,
                                       int a_events);

  typedef enum tiz_event_io_event
  {
    TIZ_EVENT_READ          = 0x01, /* ev_io detected read will not block */
    TIZ_EVENT_WRITE         = 0x02, /* ev_io detected write will not block */
    TIZ_EVENT_READ_OR_WRITE = 0x03,
    TIZ_EVENT_MAX
  } tiz_event_io_event_t;


  /**
   * Initialises a global event loop. The loop is hosted in its own thread
   * which is spawned the first time this function is called.  Ensure that the
   * first call to this function is serialized to avoid race conditions during
   * construction. Once the loop is fully constructed, subsequent calls to this
   * function simply increment a reference count and will be thread-safe.
   *
   * @ingroup event
   *
   * @return OMX_ErrorNone if success, OMX_ErrorInsufficientResources
   * otherwise.
   *
   */
  OMX_ERRORTYPE tiz_event_loop_init ();

  /**
   * Decrements the clients reference count and destroys the event loop if last
   * client. Since the last client's call destroys the hosting thread, it is
   * advisable to make sure that the last call is serialized.
   *
   * @ingroup event
   * 
   */
  void tiz_event_loop_destroy ();

  OMX_ERRORTYPE tiz_event_io_init (tiz_event_io_t ** app_ev_io,
                                   OMX_HANDLETYPE ap_hdl,
                                   tiz_event_io_cb_f ap_cback);

  void tiz_event_io_set (tiz_event_io_t * ap_ev_io,
                         int a_fd, tiz_event_io_event_t a_event);

  OMX_ERRORTYPE tiz_event_io_start (tiz_event_io_t * ap_ev_io);

  OMX_ERRORTYPE tiz_event_io_stop (tiz_event_io_t * ap_ev_io);

  void tiz_event_io_destroy (tiz_event_io_t * ap_ev_io);

  OMX_ERRORTYPE tiz_event_timer_init (tiz_event_timer_t ** app_ev_timer,
                                      OMX_HANDLETYPE ap_hdl,
                                      tiz_event_timer_cb_f ap_cback,
                                      void *ap_arg);

  void tiz_event_timer_set (tiz_event_timer_t * ap_ev_timer,
                            double a_after, double a_repeat);

  OMX_ERRORTYPE tiz_event_timer_start (tiz_event_timer_t * ap_ev_timer);

  OMX_ERRORTYPE tiz_event_timer_restart (tiz_event_timer_t * ap_ev_timer);

  OMX_ERRORTYPE tiz_event_timer_stop (tiz_event_timer_t * ap_ev_timer);

  void tiz_event_timer_destroy (tiz_event_timer_t * ap_ev_timer);

  OMX_ERRORTYPE tiz_event_stat_init (tiz_event_stat_t ** app_ev_stat,
                                     OMX_HANDLETYPE ap_hdl,
                                     tiz_event_stat_cb_f ap_cback);

  void tiz_event_stat_set (tiz_event_stat_t * ap_ev_stat,
                           const char *ap_path);

  OMX_ERRORTYPE tiz_event_stat_start (tiz_event_stat_t * ap_ev_stat);

  OMX_ERRORTYPE tiz_event_stat_stop (tiz_event_stat_t * ap_ev_stat);

  void tiz_event_stat_destroy (tiz_event_stat_t * ap_ev_stat);


#ifdef __cplusplus
}
#endif

#endif                          /* TIZOSALEV_H */
