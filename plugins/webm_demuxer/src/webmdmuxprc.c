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
 * @file   webmdmuxprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - WebM Demuxer processor
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

#include "webmdmux.h"
#include "webmdmuxprc.h"
#include "webmdmuxprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.webm_demuxer.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE webmdmux_prc_deallocate_resources (void *);

static int ne_io_read (void *a_buffer, size_t a_length, void *a_userdata)
{
  return 0;
}

static int ne_io_seek (int64_t offset, int whence, void *userdata)
{
  return 0;
}

static int64_t ne_io_tell(void * userdata)
{
  return 0;
}

/* static OMX_ERRORTYPE init_demuxer (webmdmux_prc_t *ap_prc) */
/* { */
/*   OMX_ERRORTYPE rc = OMX_ErrorNone; */

/*   if (store_data (ap_prc)) */
/*     { */
/*       int op_error = 0; */
/*       OpusFileCallbacks op_cbacks = { read_cback, NULL, NULL, NULL }; */

/*       ap_prc->p_opus_dec_ */
/*           = op_open_callbacks (ap_prc, &op_cbacks, NULL, 0, &op_error); */

/*       if (0 != op_error) */
/*         { */
/*           TIZ_ERROR (handleOf (ap_prc), */
/*                      "Unable to open the opus file handle (op_error = %d).", */
/*                      op_error); */
/*           ap_prc->store_offset_ = 0; */
/*         } */
/*       else */
/*         { */
/*           TIZ_TRACE (handleOf (ap_prc), */
/*                      "demuxer_inited = TRUE - store_offset [%d]", */
/*                      ap_prc->store_offset_); */
/*           ap_prc->demuxer_inited_ = true; */
/*           tiz_buffer_advance (ap_prc->p_store_, ap_prc->store_offset_); */
/*           ap_prc->store_offset_ = 0; */
/*         } */
/*     } */
/*   else */
/*     { */
/*       rc = OMX_ErrorInsufficientResources; */
/*     } */

/*   return rc; */
/* } */

/* static OMX_ERRORTYPE transform_buffer (webmdmux_prc_t *ap_prc) */
/* { */
/*   OMX_ERRORTYPE rc = OMX_ErrorNone; */
/*   OMX_BUFFERHEADERTYPE *p_out = tiz_filter_prc_get_header ( */
/*       ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX); */

/*   if (!store_data (ap_prc)) */
/*     { */
/*       TIZ_ERROR (handleOf (ap_prc), */
/*                  "[OMX_ErrorInsufficientResources] : " */
/*                  "Could not store all the incoming data"); */
/*       return OMX_ErrorInsufficientResources; */
/*     } */

/*   if (tiz_buffer_available (ap_prc->p_store_) == 0 || NULL == p_out) */
/*     { */
/*       TIZ_TRACE (handleOf (ap_prc), "store bytes [%d] OUT HEADER [%p]", */
/*                  tiz_buffer_available (ap_prc->p_store_), p_out); */

/*       /\* Propagate the EOS flag to the next component *\/ */
/*       if (tiz_buffer_available (ap_prc->p_store_) == 0 && p_out */
/*           && tiz_filter_prc_is_eos (ap_prc)) */
/*         { */
/*           p_out->nFlags |= OMX_BUFFERFLAG_EOS; */
/*           tiz_filter_prc_release_header ( */
/*               ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX); */
/*           tiz_filter_prc_update_eos_flag (ap_prc, false); */
/*         } */
/*       return OMX_ErrorNotReady; */
/*     } */

/*   assert (ap_prc); */
/*   assert (ap_prc->p_opus_dec_); */

/*   { */
/*     unsigned char *p_pcm = p_out->pBuffer + p_out->nOffset; */
/*     const long len = p_out->nAllocLen; */
/*     int samples_read */
/*         = op_read_float_stereo (ap_prc->p_opus_dec_, (float *)p_pcm, len); */
/*     TIZ_TRACE (handleOf (ap_prc), "samples_read [%d] ", samples_read); */

/*     if (samples_read > 0) */
/*       { */
/*         p_out->nFilledLen = 2 * samples_read * sizeof(float); */
/*         (void)tiz_filter_prc_release_header ( */
/*             ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX); */
/*       } */
/*     else */
/*       { */
/*         switch (samples_read) */
/*           { */
/*             case OP_HOLE: */
/*               { */
/*                 TIZ_NOTICE (handleOf (ap_prc), */
/*                             "[OP_HOLE] : " */
/*                             "While decoding the input stream."); */
/*               } */
/*               break; */
/*             default: */
/*               { */
/*                 TIZ_ERROR (handleOf (ap_prc), */
/*                            "[OMX_ErrorStreamCorruptFatal] : " */
/*                            "While decoding the input stream."); */
/*                 rc = OMX_ErrorStreamCorruptFatal; */
/*               } */
/*           }; */
/*       } */
/*   } */

/*   return rc; */
/* } */

static void
ne_log_cback(nestegg * ctx, unsigned int severity, char const * fmt, ...)
{
  va_list ap;
  char const * sev = NULL;

#if !defined(DEBUG)
  if (severity < NESTEGG_LOG_WARNING)
    return;
#endif

  switch (severity) {
  case NESTEGG_LOG_DEBUG:
    sev = "debug:   ";
    break;
  case NESTEGG_LOG_WARNING:
    sev = "warning: ";
    break;
  case NESTEGG_LOG_CRITICAL:
    sev = "critical:";
    break;
  default:
    sev = "unknown: ";
  }

  fprintf(stderr, "%p %s ", (void *) ctx, sev);

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fprintf(stderr, "\n");
}

/*
 * webmdmuxprc
 */

static void *
webmdmux_prc_ctor (void *ap_prc, va_list * app)
{
  webmdmux_prc_t *p_prc = super_ctor (typeOf (ap_prc, "webmdmuxprc"), ap_prc, app);
  p_prc->eos_ = false;
  p_prc->p_ne_ctx_ = NULL;
  p_prc->ne_io_.read = ne_io_read;
  p_prc->ne_io_.seek = ne_io_seek;
  p_prc->ne_io_.tell = ne_io_tell;
  p_prc->ne_io_.userdata = p_prc;
  return p_prc;
}

static void *
webmdmux_prc_dtor (void *ap_obj)
{
  (void) webmdmux_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "webmdmuxprc"), ap_obj);
}

/* static OMX_ERRORTYPE */
/* webmdmux_prc_read_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr) */
/* { */
/*   return OMX_ErrorNone; */
/* } */

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
webmdmux_prc_allocate_resources (void *ap_prc, OMX_U32 a_pid)
{
  webmdmux_prc_t *p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (!p_prc->p_ne_ctx_);
  if (0 != nestegg_init (&p_prc->p_ne_ctx_, p_prc->ne_io_, ne_log_cback, -1))
    {
      TIZ_ERROR (handleOf (ap_prc), "Error allocating the nestegg demuxer");
      rc = OMX_ErrorInsufficientResources;
    }
  return rc;
}

static OMX_ERRORTYPE
webmdmux_prc_deallocate_resources (void *ap_prc)
{
  webmdmux_prc_t *p_prc = ap_prc;
  if (p_prc->p_ne_ctx_)
    {
      nestegg_destroy(p_prc->p_ne_ctx_);
      p_prc->p_ne_ctx_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
webmdmux_prc_prepare_to_transfer (void *ap_prc, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
webmdmux_prc_transfer_and_process (void *ap_prc, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
webmdmux_prc_stop_and_return (void *ap_prc)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
webmdmux_prc_buffers_ready (const void *ap_prc)
{
/*   webmdmux_prc_t *p_prc = (webmdmux_prc_t *)ap_prc; */
  OMX_ERRORTYPE rc = OMX_ErrorNone;

/*   assert (ap_prc); */

/*   if (!p_prc->demuxer_inited_) */
/*     { */
/*       rc = init_demuxer (p_prc); */
/*     } */

/*   if (p_prc->demuxer_inited_ && OMX_ErrorNone == rc) */
/*     { */
/*       while (OMX_ErrorNone == rc) */
/*         { */
/*           rc = transform_buffer (p_prc); */
/*         } */
/*       if (OMX_ErrorNotReady == rc) */
/*         { */
/*           rc = OMX_ErrorNone; */
/*         } */
/*     } */

  return rc;
}

/*
 * webmdmux_prc_class
 */

static void *
webmdmux_prc_class_ctor (void *ap_prc, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_prc, "webmdmuxprc_class"), ap_prc, app);
}

/*
 * initialization
 */

void *
webmdmux_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * webmdmuxprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "webmdmuxprc_class", classOf (tizprc), sizeof (webmdmux_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, webmdmux_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return webmdmuxprc_class;
}

void *
webmdmux_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * webmdmuxprc_class = tiz_get_type (ap_hdl, "webmdmuxprc_class");
  TIZ_LOG_CLASS (webmdmuxprc_class);
  void * webmdmuxprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (webmdmuxprc_class, "webmdmuxprc", tizprc, sizeof (webmdmux_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, webmdmux_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, webmdmux_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, webmdmux_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, webmdmux_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, webmdmux_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, webmdmux_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, webmdmux_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, webmdmux_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return webmdmuxprc;
}
