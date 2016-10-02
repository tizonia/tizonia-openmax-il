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
#include <string.h>

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
static OMX_ERRORTYPE release_buffer (webmdmux_prc_t *);

static int ne_io_read (void *a_buffer, size_t a_length, void *a_userdata)
{
  return 0;
}

static int ne_io_seek (int64_t offset, int whence, void *userdata)
{
  return 0;
}

static int64_t ne_io_tell (void *userdata)
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
/*                      "Unable to open the opus file handle (op_error = %d).",
 */
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

static void ne_log_cback (nestegg *ctx, unsigned int severity, char const *fmt,
                          ...)
{
  va_list ap;
  char const *sev = NULL;

#if !defined(DEBUG)
  if (severity < NESTEGG_LOG_WARNING)
    return;
#endif

  switch (severity)
    {
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

  fprintf (stderr, "%p %s ", (void *)ctx, sev);

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);

  fprintf (stderr, "\n");
}

static void update_cache_size (webmdmux_prc_t *ap_prc)
{
  assert (ap_prc);
  assert (ap_prc->bitrate_ > 0);
  ap_prc->cache_bytes_ = ((ap_prc->bitrate_ * 1000) / 8)
                         * ARATELIA_WEBM_DEMUXER_DEFAULT_CACHE_SECONDS;
  if (ap_prc->p_trans_)
    {
      tiz_urltrans_set_internal_buffer_size (ap_prc->p_trans_,
                                              ap_prc->cache_bytes_);
    }
}

static OMX_ERRORTYPE obtain_uri (webmdmux_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const long pathname_max = PATH_MAX + NAME_MAX;

  assert (ap_prc);
  assert (!ap_prc->p_uri_param_);

  ap_prc->p_uri_param_ = tiz_mem_calloc (
      1, sizeof (OMX_PARAM_CONTENTURITYPE) + pathname_max + 1);

  if (!ap_prc->p_uri_param_)
    {
      TIZ_ERROR (handleOf (ap_prc),
                 "Error allocating memory for the content uri struct");
      rc = OMX_ErrorInsufficientResources;
    }
  else
    {
      ap_prc->p_uri_param_->nSize
          = sizeof (OMX_PARAM_CONTENTURITYPE) + pathname_max + 1;
      ap_prc->p_uri_param_->nVersion.nVersion = OMX_VERSION;

      tiz_check_omx_err (tiz_api_GetParameter (
          tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
          OMX_IndexParamContentURI, ap_prc->p_uri_param_));
      TIZ_NOTICE (handleOf (ap_prc), "URI [%s]",
                  ap_prc->p_uri_param_->contentURI);
      /* Verify we are getting an http scheme */
      if (strncasecmp ((const char *)ap_prc->p_uri_param_->contentURI,
                       "http://", 7)
              != 0
          && strncasecmp ((const char *)ap_prc->p_uri_param_->contentURI,
                          "https://", 8)
                 != 0)
        {
          rc = OMX_ErrorContentURIError;
        }
    }

  return rc;
}

static void obtain_audio_encoding_from_headers (webmdmux_prc_t *ap_prc,
                                                const char *ap_header,
                                                const size_t a_size)
{
  (void)ap_prc;
  (void)ap_header;
  (void)a_size;
}

static inline void delete_uri (webmdmux_prc_t *ap_prc)
{
  assert (ap_prc);
  tiz_mem_free (ap_prc->p_uri_param_);
  ap_prc->p_uri_param_ = NULL;
}

static OMX_ERRORTYPE release_buffer (webmdmux_prc_t *ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_outhdr_)
    {
      TIZ_NOTICE (handleOf (ap_prc), "releasing HEADER [%p] nFilledLen [%d]",
                  ap_prc->p_outhdr_, ap_prc->p_outhdr_->nFilledLen);
      tiz_check_omx_err (tiz_krn_release_buffer (
          tiz_get_krn (handleOf (ap_prc)), ARATELIA_WEBM_DEMUXER_PORT_INDEX,
          ap_prc->p_outhdr_));
      ap_prc->p_outhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static void buffer_filled (OMX_BUFFERHEADERTYPE *ap_hdr, void *ap_arg)
{
  webmdmux_prc_t *p_prc = ap_arg;
  assert (p_prc);
  assert (ap_hdr);
  assert (p_prc->p_outhdr_ == ap_hdr);
  ap_hdr->nOffset = 0;
  (void)release_buffer (p_prc);
}

static OMX_BUFFERHEADERTYPE *buffer_emptied (OMX_PTR ap_arg)
{
  webmdmux_prc_t *p_prc = ap_arg;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  assert (p_prc);

  if (!p_prc->port_disabled_)
    {
      if (p_prc->p_outhdr_)
        {
          p_hdr = p_prc->p_outhdr_;
        }
      else
        {
          if (OMX_ErrorNone
              == (tiz_krn_claim_buffer (tiz_get_krn (handleOf (p_prc)),
                                        ARATELIA_WEBM_DEMUXER_PORT_INDEX, 0,
                                        &p_prc->p_outhdr_)))
            {
              if (p_prc->p_outhdr_)
                {
                  TIZ_TRACE (handleOf (p_prc),
                             "Claimed HEADER [%p]...nFilledLen [%d]",
                             p_prc->p_outhdr_, p_prc->p_outhdr_->nFilledLen);
                  p_hdr = p_prc->p_outhdr_;
                }
            }
        }
    }
  return p_hdr;
}

static void header_available (OMX_PTR ap_arg, const void *ap_ptr,
                              const size_t a_nbytes)
{
  webmdmux_prc_t *p_prc = ap_arg;
  assert (p_prc);
  assert (ap_ptr);

  if (p_prc->auto_detect_on_)
    {
      obtain_audio_encoding_from_headers (p_prc, ap_ptr, a_nbytes);
    }
}

static bool data_available (OMX_PTR ap_arg, const void *ap_ptr,
                            const size_t a_nbytes)
{
  webmdmux_prc_t *p_prc = ap_arg;
  bool pause_needed = false;
  assert (p_prc);
  assert (ap_ptr);

  /*   if (p_prc->auto_detect_on_ && a_nbytes > 0) */
  /*     { */
  /*       p_prc->auto_detect_on_ = false; */

  /*       /\* This will pause the http transfer *\/ */
  /*       pause_needed = true; */

  /*       if (OMX_AUDIO_CodingOGA == p_prc->audio_coding_type_) */
  /*         { */
  /*           /\* Try to identify the actual codec from the ogg stream *\/ */
  /*           p_prc->audio_coding_type_ */
  /*             = identify_ogg_codec (p_prc, (unsigned char *)ap_ptr,
   * a_nbytes); */
  /*           if (OMX_AUDIO_CodingUnused != p_prc->audio_coding_type_) */
  /*             { */
  /*               set_audio_coding_on_port (p_prc); */
  /*               set_audio_info_on_port (p_prc); */
  /*             } */
  /*         } */
  /*       /\* And now trigger the OMX_EventPortFormatDetected and */
  /*          OMX_EventPortSettingsChanged events or a */
  /*          OMX_ErrorFormatNotDetected event *\/ */
  /*       send_port_auto_detect_events (p_prc); */
  /*     } */
  return pause_needed;
}

static bool connection_lost (OMX_PTR ap_arg)
{
  /*   webmdmux_prc_t *p_prc = ap_arg; */
  /*   assert (p_prc); */
  /*   prepare_for_port_auto_detection (p_prc); */
  /* Return true to indicate that the automatic reconnection procedure needs to
     be started */
  return true;
}

/*
 * webmdmuxprc
 */

static void *webmdmux_prc_ctor (void *ap_prc, va_list *app)
{
  webmdmux_prc_t *p_prc
      = super_ctor (typeOf (ap_prc, "webmdmuxprc"), ap_prc, app);
  assert (p_prc);
  p_prc->p_outhdr_ = NULL;
  p_prc->p_uri_param_ = NULL;
  p_prc->p_trans_ = NULL;
  p_prc->eos_ = false;
  p_prc->port_disabled_ = false;
  p_prc->auto_detect_on_ = false;
  p_prc->audio_coding_type_ = OMX_AUDIO_CodingUnused;
  p_prc->bitrate_ = ARATELIA_WEBM_DEMUXER_DEFAULT_BIT_RATE_KBITS;
  update_cache_size (p_prc);
  p_prc->p_ne_ctx_ = NULL;
  p_prc->ne_io_.read = ne_io_read;
  p_prc->ne_io_.seek = ne_io_seek;
  p_prc->ne_io_.tell = ne_io_tell;
  p_prc->ne_io_.userdata = p_prc;
  return p_prc;
}

static void *webmdmux_prc_dtor (void *ap_obj)
{
  (void)webmdmux_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "webmdmuxprc"), ap_obj);
}

/* static OMX_ERRORTYPE */
/* webmdmux_prc_read_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
 */
/* { */
/*   return OMX_ErrorNone; */
/* } */

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE webmdmux_prc_allocate_resources (void *ap_prc,
                                                      OMX_U32 a_pid)
{
  webmdmux_prc_t *p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  assert (!p_prc->p_ne_ctx_);
  assert (!p_prc->p_uri_param_);

  tiz_check_omx_err (obtain_uri (p_prc));

  if (0 != nestegg_init (&p_prc->p_ne_ctx_, p_prc->ne_io_, ne_log_cback, -1))
    {
      TIZ_ERROR (handleOf (ap_prc), "Error allocating the nestegg demuxer");
      rc = OMX_ErrorInsufficientResources;
    }

  if (OMX_ErrorNone == rc)
    {
      const tiz_urltrans_buffer_cbacks_t buffer_cbacks
          = { buffer_filled, buffer_emptied };
      const tiz_urltrans_info_cbacks_t info_cbacks
          = { header_available, data_available, connection_lost };
      const tiz_urltrans_event_io_cbacks_t io_cbacks
          = { tiz_srv_io_watcher_init, tiz_srv_io_watcher_destroy,
              tiz_srv_io_watcher_start, tiz_srv_io_watcher_stop };
      const tiz_urltrans_event_timer_cbacks_t timer_cbacks
          = { tiz_srv_timer_watcher_init, tiz_srv_timer_watcher_destroy,
              tiz_srv_timer_watcher_start, tiz_srv_timer_watcher_stop,
              tiz_srv_timer_watcher_restart };
      rc = tiz_urltrans_init (&(p_prc->p_trans_), p_prc, p_prc->p_uri_param_,
                              ARATELIA_WEBM_DEMUXER_COMPONENT_NAME,
                              ARATELIA_WEBM_DEMUXER_PORT_MIN_BUF_SIZE,
                              ARATELIA_WEBM_DEMUXER_DEFAULT_RECONNECT_TIMEOUT,
                              buffer_cbacks, info_cbacks, io_cbacks,
                              timer_cbacks);
    }
  return rc;
}

static OMX_ERRORTYPE webmdmux_prc_deallocate_resources (void *ap_prc)
{
  webmdmux_prc_t *p_prc = ap_prc;
  assert (p_prc);
  tiz_urltrans_destroy (p_prc->p_trans_);
  p_prc->p_trans_ = NULL;
  delete_uri (p_prc);
  if (p_prc->p_ne_ctx_)
    {
      nestegg_destroy (p_prc->p_ne_ctx_);
      p_prc->p_ne_ctx_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE webmdmux_prc_prepare_to_transfer (void *ap_prc,
                                                       OMX_U32 a_pid)
{
  webmdmux_prc_t *p_prc = ap_prc;
  assert (ap_prc);
  p_prc->eos_ = false;
  tiz_urltrans_cancel (p_prc->p_trans_);
  tiz_urltrans_set_internal_buffer_size (p_prc->p_trans_, p_prc->cache_bytes_);
  /*   return prepare_for_port_auto_detection (p_prc); */
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE webmdmux_prc_transfer_and_process (void *ap_prc,
                                                        OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE webmdmux_prc_stop_and_return (void *ap_prc)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE webmdmux_prc_buffers_ready (const void *ap_prc)
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

static void *webmdmux_prc_class_ctor (void *ap_prc, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_prc, "webmdmuxprc_class"), ap_prc, app);
}

/*
 * initialization
 */

void *webmdmux_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *webmdmuxprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizprc), "webmdmuxprc_class", classOf (tizprc),
       sizeof (webmdmux_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, webmdmux_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return webmdmuxprc_class;
}

void *webmdmux_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *webmdmuxprc_class = tiz_get_type (ap_hdl, "webmdmuxprc_class");
  TIZ_LOG_CLASS (webmdmuxprc_class);
  void *webmdmuxprc = factory_new
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
