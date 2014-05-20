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
 * @file   httpsrcprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - HTTP streaming client processor
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "httpsrc.h"
#include "httpsrcprc.h"
#include "httpsrcprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_source.prc"
#endif

/* static inline OMX_ERRORTYPE */
/* start_io_watcher (httpsrc_prc_t * ap_prc) */
/* { */
/*   assert (NULL != ap_prc); */
/*   assert (NULL != ap_prc->p_ev_io_); */
/*   ap_prc->awaiting_io_ev_ = true; */
/*   return tiz_event_io_start (ap_prc->p_ev_io_); */
/* } */

/* static inline OMX_ERRORTYPE */
/* stop_io_watcher (httpsrc_prc_t * ap_prc) */
/* { */
/*   OMX_ERRORTYPE rc = OMX_ErrorNone; */
/*   assert (NULL != ap_prc); */
/*   if (NULL != ap_prc->p_ev_io_) */
/*   { */
/*     rc = tiz_event_io_stop (ap_prc->p_ev_io_); */
/*   } */
/*   ap_prc->awaiting_io_ev_ = false; */
/*   return rc; */
/* } */

static OMX_ERRORTYPE
release_buffers (httpsrc_prc_t * ap_prc)
{
  assert (NULL != ap_prc);

/*   stop_io_watcher (ap_prc); */

  if (ap_prc->p_outhdr_)
    {
      void *p_krn = tiz_get_krn (handleOf (ap_prc));
      tiz_check_omx_err (tiz_krn_release_buffer (p_krn, 0, ap_prc->p_outhdr_));
      ap_prc->p_outhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE
do_flush (httpsrc_prc_t * ap_prc)
{
/*   assert (NULL != ap_prc); */
/*   tiz_check_omx_err (stop_io_watcher (ap_prc)); */
/*   if (NULL != ap_prc->p_pcm_hdl) */
/*     { */
/*       (void) snd_pcm_drop (ap_prc->p_pcm_hdl); */
/*     } */
  /* Release any buffers held  */
  return release_buffers (ap_prc);
}

/*
 * httpsrcprc
 */

static void *
httpsrc_prc_ctor (void *ap_obj, va_list * app)
{
  httpsrc_prc_t *p_obj = super_ctor (typeOf (ap_obj, "httpsrcprc"), ap_obj, app);
  p_obj->eos_ = false;
  return p_obj;
}

static void *
httpsrc_prc_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "httpsrcprc"), ap_obj);
}

static OMX_ERRORTYPE
httpsrc_prc_read_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
{
  return OMX_ErrorNone;
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
httpsrc_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
httpsrc_prc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
httpsrc_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
httpsrc_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
httpsrc_prc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
httpsrc_prc_buffers_ready (const void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
httpsrc_prc_io_ready (void *ap_obj,
                 tiz_event_io_t * ap_ev_io, int a_fd, int a_events)
{
  TIZ_TRACE (handleOf (ap_obj), "Received io event on fd [%d]", a_fd);
  stop_io_watcher (ap_obj);
  return render_pcm_data (ap_obj);
}

static OMX_ERRORTYPE
httpsrc_prc_pause (const void *ap_obj)
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *) ap_obj;
  assert (NULL != p_prc);
/*   if (p_prc->awaiting_io_ev_) */
/*     { */
/*       tiz_event_io_stop (p_prc->p_ev_io_); */
/*     } */
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
httpsrc_prc_resume (const void *ap_obj)
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *) ap_obj;
  assert (NULL != p_prc);
/*   if (p_prc->awaiting_io_ev_) */
/*     { */
/*       start_io_watcher (p_prc); */
/*     } */
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
httpsrc_prc_port_flush (const void *ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *) ap_obj;
  assert (NULL != p_prc);
  return do_flush (p_prc);
}

static OMX_ERRORTYPE
httpsrc_prc_port_disable (const void *ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *) ap_obj;
  assert (NULL != p_prc);
  /* Release any buffers held  */
  return release_buffers ((httpsrc_prc_t *) ap_obj);
}

static OMX_ERRORTYPE
httpsrc_prc_port_enable (const void *ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  /* TODO */
  return OMX_ErrorNone;
}

/*
 * httpsrc_prc_class
 */

static void *
httpsrc_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "httpsrcprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
httpsrc_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * httpsrcprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "httpsrcprc_class", classOf (tizprc), sizeof (httpsrc_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, httpsrc_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return httpsrcprc_class;
}

void *
httpsrc_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * httpsrcprc_class = tiz_get_type (ap_hdl, "httpsrcprc_class");
  TIZ_LOG_CLASS (httpsrcprc_class);
  void * httpsrcprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (httpsrcprc_class, "httpsrcprc", tizprc, sizeof (httpsrc_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, httpsrc_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, httpsrc_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, httpsrc_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, httpsrc_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, httpsrc_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, httpsrc_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, httpsrc_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, httpsrc_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_io_ready, httpsrc_prc_io_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_pause, httpsrc_prc_pause,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_resume, httpsrc_prc_resume,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, httpsrc_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, httpsrc_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, httpsrc_prc_port_enable,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return httpsrcprc;
}
