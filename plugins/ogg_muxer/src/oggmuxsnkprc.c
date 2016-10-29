/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   oggmuxsnkprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Ogg muxer sink processor
 *
 * NOTE: This processor implementation is just a skeleton for now!
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "oggmux.h"
#include "oggmuxsnkprc.h"
#include "oggmuxsnkprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.ogg_muxer.sink.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE
oggmuxsnk_prc_deallocate_resources (void *);
static OMX_ERRORTYPE
release_buffer (oggmuxsnk_prc_t *);
static OMX_ERRORTYPE
prepare_for_port_auto_detection (oggmuxsnk_prc_t * ap_prc);

static void
update_cache_size (oggmuxsnk_prc_t * ap_prc)
{
  assert (ap_prc);
  assert (ap_prc->bitrate_ > 0);
  ap_prc->cache_bytes_ = ((ap_prc->bitrate_ * 1000) / 8)
                         * ARATELIA_OGG_MUXER_DEFAULT_CACHE_SECONDS;
  if (ap_prc->p_trans_)
    {
      tiz_urltrans_set_internal_buffer_size (ap_prc->p_trans_,
                                             ap_prc->cache_bytes_);
    }
}

static OMX_ERRORTYPE
obtain_uri (oggmuxsnk_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const long pathname_max = PATH_MAX + NAME_MAX;

  assert (ap_prc);
  assert (!ap_prc->p_uri_);

  ap_prc->p_uri_
    = tiz_mem_calloc (1, sizeof (OMX_PARAM_CONTENTURITYPE) + pathname_max + 1);

  if (!ap_prc->p_uri_)
    {
      TIZ_ERROR (handleOf (ap_prc),
                 "Error allocating memory for the content uri struct");
      rc = OMX_ErrorInsufficientResources;
    }
  else
    {
      ap_prc->p_uri_->nSize
        = sizeof (OMX_PARAM_CONTENTURITYPE) + pathname_max + 1;
      ap_prc->p_uri_->nVersion.nVersion = OMX_VERSION;

      tiz_check_omx_err (tiz_api_GetParameter (
        tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
        OMX_IndexParamContentURI, ap_prc->p_uri_));
      TIZ_NOTICE (handleOf (ap_prc), "URI [%s]",
                  ap_prc->p_uri_->contentURI);
      /* Verify we are getting an http scheme */
      if (strncasecmp ((const char *) ap_prc->p_uri_->contentURI,
                       "http://", 7)
            != 0
          && strncasecmp ((const char *) ap_prc->p_uri_->contentURI,
                          "https://", 8)
               != 0)
        {
          rc = OMX_ErrorContentURIError;
        }
    }

  return rc;
}

static inline void
delete_uri (oggmuxsnk_prc_t * ap_prc)
{
  assert (ap_prc);
  tiz_mem_free (ap_prc->p_uri_);
  ap_prc->p_uri_ = NULL;
}

static OMX_ERRORTYPE
release_buffer (oggmuxsnk_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_outhdr_)
    {
      TIZ_NOTICE (handleOf (ap_prc), "releasing HEADER [%p] nFilledLen [%d]",
                  ap_prc->p_outhdr_, ap_prc->p_outhdr_->nFilledLen);
      tiz_check_omx_err (tiz_krn_release_buffer (
        tiz_get_krn (handleOf (ap_prc)),
        ARATELIA_OGG_MUXER_SINK_PORT_0_INDEX, ap_prc->p_outhdr_));
      ap_prc->p_outhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
prepare_for_port_auto_detection (oggmuxsnk_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def,
                            ARATELIA_OGG_MUXER_SINK_PORT_0_INDEX);
  tiz_check_omx_err (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));
  ap_prc->audio_coding_type_ = port_def.format.audio.eEncoding;
  ap_prc->auto_detect_on_
    = (OMX_AUDIO_CodingAutoDetect == ap_prc->audio_coding_type_) ? true : false;
  return OMX_ErrorNone;
}

/*
 * oggmuxsnkprc
 */

static void *
oggmuxsnk_prc_ctor (void * ap_prc, va_list * app)
{
  oggmuxsnk_prc_t * p_prc
    = super_ctor (typeOf (ap_prc, "oggmuxsnkprc"), ap_prc, app);
  assert (p_prc);
  p_prc->p_outhdr_ = NULL;
  p_prc->p_uri_ = NULL;
  p_prc->p_trans_ = NULL;
  p_prc->eos_ = false;
  p_prc->port_disabled_ = false;
  p_prc->uri_changed_ = false;
  p_prc->auto_detect_on_ = false;
  p_prc->audio_coding_type_ = OMX_AUDIO_CodingUnused;
  p_prc->bitrate_ = ARATELIA_OGG_MUXER_DEFAULT_BIT_RATE_KBITS;
  update_cache_size (p_prc);
  return p_prc;
}

static void *
oggmuxsnk_prc_dtor (void * ap_obj)
{
  (void) oggmuxsnk_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "oggmuxsnkprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
oggmuxsnk_prc_allocate_resources (void * ap_prc, OMX_U32 a_pid)
{
  oggmuxsnk_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  assert (!p_prc->p_uri_);
  tiz_check_omx_err (obtain_uri (p_prc));
  return rc;
}

static OMX_ERRORTYPE
oggmuxsnk_prc_deallocate_resources (void * ap_prc)
{
  oggmuxsnk_prc_t * p_prc = ap_prc;
  assert (p_prc);
  tiz_urltrans_destroy (p_prc->p_trans_);
  p_prc->p_trans_ = NULL;
  delete_uri (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggmuxsnk_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid)
{
  oggmuxsnk_prc_t * p_prc = ap_prc;
  assert (ap_prc);
  p_prc->eos_ = false;
  tiz_urltrans_cancel (p_prc->p_trans_);
  tiz_urltrans_set_internal_buffer_size (p_prc->p_trans_, p_prc->cache_bytes_);
  return prepare_for_port_auto_detection (p_prc);
}

static OMX_ERRORTYPE
oggmuxsnk_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid)
{
  oggmuxsnk_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (p_prc->auto_detect_on_)
    {
      rc = tiz_urltrans_start (p_prc->p_trans_);
    }
  return rc;
}

static OMX_ERRORTYPE
oggmuxsnk_prc_stop_and_return (void * ap_prc)
{
  oggmuxsnk_prc_t * p_prc = ap_prc;
  assert (p_prc);
  if (p_prc->p_trans_)
    {
      tiz_urltrans_pause (p_prc->p_trans_);
      tiz_urltrans_flush_buffer (p_prc->p_trans_);
    }
  return release_buffer (p_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
oggmuxsnk_prc_buffers_ready (const void * ap_prc)
{
  oggmuxsnk_prc_t * p_prc = (oggmuxsnk_prc_t *) ap_prc;
  assert (p_prc);
  return tiz_urltrans_on_buffers_ready (p_prc->p_trans_);
}

static OMX_ERRORTYPE
oggmuxsnk_prc_io_ready (void * ap_prc, tiz_event_io_t * ap_ev_io, int a_fd,
                          int a_events)
{
  oggmuxsnk_prc_t * p_prc = ap_prc;
  assert (p_prc);
  return tiz_urltrans_on_io_ready (p_prc->p_trans_, ap_ev_io, a_fd, a_events);
}

static OMX_ERRORTYPE
oggmuxsnk_prc_timer_ready (void * ap_prc, tiz_event_timer_t * ap_ev_timer,
                             void * ap_arg, const uint32_t a_id)
{
  oggmuxsnk_prc_t * p_prc = ap_prc;
  assert (p_prc);
  return tiz_urltrans_on_timer_ready (p_prc->p_trans_, ap_ev_timer);
}

static OMX_ERRORTYPE
oggmuxsnk_prc_pause (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggmuxsnk_prc_resume (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggmuxsnk_prc_port_flush (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  oggmuxsnk_prc_t * p_prc = (oggmuxsnk_prc_t *) ap_obj;
  if (p_prc->p_trans_)
    {
      tiz_urltrans_flush_buffer (p_prc->p_trans_);
    }
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE
oggmuxsnk_prc_port_disable (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  oggmuxsnk_prc_t * p_prc = (oggmuxsnk_prc_t *) ap_obj;
  assert (p_prc);
  TIZ_PRINTF_DBG_RED ("Disabling port was disabled? [%s]\n",
                      p_prc->port_disabled_ ? "YES" : "NO");
  p_prc->port_disabled_ = true;
  if (p_prc->p_trans_)
    {
      tiz_urltrans_pause (p_prc->p_trans_);
      tiz_urltrans_flush_buffer (p_prc->p_trans_);
    }
  /* Release any buffers held  */
  return release_buffer ((oggmuxsnk_prc_t *) ap_obj);
}

static OMX_ERRORTYPE
oggmuxsnk_prc_port_enable (const void * ap_prc, OMX_U32 a_pid)
{
  oggmuxsnk_prc_t * p_prc = (oggmuxsnk_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  TIZ_PRINTF_DBG_RED ("Enabling port was disabled? [%s]\n",
                      p_prc->port_disabled_ ? "YES" : "NO");
  if (p_prc->port_disabled_)
    {
      p_prc->port_disabled_ = false;
      if (!p_prc->uri_changed_)
        {
          rc = tiz_urltrans_unpause (p_prc->p_trans_);
        }
      else
        {
          p_prc->uri_changed_ = false;
          rc = tiz_urltrans_start (p_prc->p_trans_);
        }
    }
  return rc;
}

/*
 * oggmuxsnk_prc_class
 */

static void *
oggmuxsnk_prc_class_ctor (void * ap_prc, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_prc, "oggmuxsnkprc_class"), ap_prc, app);
}

/*
 * initialization
 */

void *
oggmuxsnk_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * oggmuxsnkprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "oggmuxsnkprc_class", classOf (tizprc),
     sizeof (oggmuxsnk_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, oggmuxsnk_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return oggmuxsnkprc_class;
}

void *
oggmuxsnk_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * oggmuxsnkprc_class = tiz_get_type (ap_hdl, "oggmuxsnkprc_class");
  TIZ_LOG_CLASS (oggmuxsnkprc_class);
  void * oggmuxsnkprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (oggmuxsnkprc_class, "oggmuxsnkprc", tizprc, sizeof (oggmuxsnk_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, oggmuxsnk_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, oggmuxsnk_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, oggmuxsnk_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, oggmuxsnk_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, oggmuxsnk_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, oggmuxsnk_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, oggmuxsnk_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_io_ready, oggmuxsnk_prc_io_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_timer_ready, oggmuxsnk_prc_timer_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, oggmuxsnk_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_pause, oggmuxsnk_prc_pause,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_resume, oggmuxsnk_prc_resume,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, oggmuxsnk_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, oggmuxsnk_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, oggmuxsnk_prc_port_enable,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return oggmuxsnkprc;
}
