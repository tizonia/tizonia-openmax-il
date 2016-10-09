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
 * @file   webmdmuxfltprc.c
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

#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "webmdmux.h"
#include "webmdmuxfltprc.h"
#include "webmdmuxfltprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.webm_demuxer.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE webmdmuxflt_prc_deallocate_resources (void *);
static OMX_ERRORTYPE release_buffer (webmdmuxflt_prc_t *);
static OMX_ERRORTYPE prepare_for_port_auto_detection (webmdmuxflt_prc_t *ap_prc);

#define on_nestegg_error_ret_omx_oom(expr)                            \
  do                                                                  \
    {                                                                 \
      int nestegg_error = 0;                                          \
      if (0 != (nestegg_error = (expr)))                              \
        {                                                             \
          TIZ_ERROR (handleOf (p_prc),                                \
                     "[OMX_ErrorInsufficientResources] : while using" \
                     "libnestegg");                                   \
          return OMX_ErrorInsufficientResources;                      \
        }                                                             \
    }                                                                 \
  while (0)

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

static void update_cache_size (webmdmuxflt_prc_t *ap_prc)
{
  assert (ap_prc);
  assert (ap_prc->bitrate_ > 0);
  ap_prc->cache_bytes_ = ((ap_prc->bitrate_ * 1000) / 8)
                         * ARATELIA_WEBM_DEMUXER_DEFAULT_CACHE_SECONDS;
}

static OMX_ERRORTYPE release_buffer (webmdmuxflt_prc_t *ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_outhdr_)
    {
      TIZ_NOTICE (handleOf (ap_prc), "releasing HEADER [%p] nFilledLen [%d]",
                  ap_prc->p_outhdr_, ap_prc->p_outhdr_->nFilledLen);
      tiz_check_omx_err (tiz_krn_release_buffer (
          tiz_get_krn (handleOf (ap_prc)), ARATELIA_WEBM_DEMUXER_SOURCE_PORT_0_INDEX,
          ap_prc->p_outhdr_));
      ap_prc->p_outhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE prepare_for_port_auto_detection (webmdmuxflt_prc_t *ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_WEBM_DEMUXER_SOURCE_PORT_0_INDEX);
  tiz_check_omx_err (
      tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                            OMX_IndexParamPortDefinition, &port_def));
  ap_prc->audio_coding_type_ = port_def.format.audio.eEncoding;
  ap_prc->auto_detect_on_
      = (OMX_AUDIO_CodingAutoDetect == ap_prc->audio_coding_type_) ? true
                                                                   : false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE release_input_header (webmdmuxflt_prc_t *ap_prc)
{
  OMX_BUFFERHEADERTYPE *p_in = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX);

  assert (ap_prc);

  if (p_in)
    {
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          /* Let's propagate EOS flag to output */
          TIZ_TRACE (handleOf (ap_prc), "Propagating EOS flag to output");
          OMX_BUFFERHEADERTYPE *p_out = tiz_filter_prc_get_header (
              ap_prc, ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX);
          if (p_out)
            {
              p_out->nFlags |= OMX_BUFFERFLAG_EOS;
            }
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          p_in->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
        }
      tiz_filter_prc_release_header (ap_prc,
                                     ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX);
    }
  return OMX_ErrorNone;
}

static bool store_data (webmdmuxflt_prc_t *ap_prc)
{
  bool rc = true;

  if ((tiz_buffer_available (ap_prc->p_store_) - ap_prc->store_offset_)
      < ARATELIA_WEBM_DEMUXER_WEBM_PORT_MIN_BUF_SIZE)
    {
      OMX_BUFFERHEADERTYPE *p_in = tiz_filter_prc_get_header (
          ap_prc, ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX);

      assert (ap_prc);

      if (p_in)
        {
          TIZ_TRACE (handleOf (ap_prc), "store available [%d]",
                     tiz_buffer_available (ap_prc->p_store_));
          if (tiz_buffer_push (ap_prc->p_store_,
                                     p_in->pBuffer + p_in->nOffset,
                                     p_in->nFilledLen) < p_in->nFilledLen)
            {
              TIZ_ERROR (handleOf (ap_prc),
                         "[%s] : Unable to store all the data.",
                         tiz_err_to_str (rc));
              rc = false;
            }
          release_input_header (ap_prc);
        }
    }
  return rc;
}

static OMX_ERRORTYPE transform_buffer (webmdmuxflt_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_out = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX);

  if (!store_data (ap_prc))
    {
      TIZ_ERROR (handleOf (ap_prc),
                 "[OMX_ErrorInsufficientResources] : "
                 "Could not store all the incoming data");
      return OMX_ErrorInsufficientResources;
    }

  if (tiz_buffer_available (ap_prc->p_store_) == 0 || ! p_out)
    {
      TIZ_TRACE (handleOf (ap_prc), "store bytes [%d] OUT HEADER [%p]",
                 tiz_buffer_available (ap_prc->p_store_), p_out);

      /* Propagate the EOS flag to the next component */
      if (tiz_buffer_available (ap_prc->p_store_) == 0 && p_out
          && tiz_filter_prc_is_eos (ap_prc))
        {
          p_out->nFlags |= OMX_BUFFERFLAG_EOS;
          tiz_filter_prc_release_header (
              ap_prc, ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX);
          tiz_filter_prc_update_eos_flag (ap_prc, false);
        }
      return OMX_ErrorNotReady;
    }

  assert (ap_prc);
  assert (ap_prc->p_ne_ctx_);

  /* { */
/*     unsigned char *p_pcm = p_out->pBuffer + p_out->nOffset; */
/*     const long len = p_out->nAllocLen; */
/*     int samples_read */
/*         = op_read_float_stereo (ap_prc->p_opus_dec_, (float *)p_pcm, len); */
/*     TIZ_TRACE (handleOf (ap_prc), "samples_read [%d] ", samples_read); */

/*     if (samples_read > 0) */
/*       { */
/*         p_out->nFilledLen = 2 * samples_read * sizeof(float); */
/*         (void)tiz_filter_prc_release_header ( */
/*             ap_prc, ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX); */
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

  return rc;
}

static void reset_stream_parameters (webmdmuxflt_prc_t *ap_prc)
{
  assert (ap_prc);

  ap_prc->store_offset_ = 0;
  ap_prc->demuxer_inited_ = false;
  ap_prc->auto_detect_on_ = false;
  ap_prc->audio_coding_type_ = OMX_AUDIO_CodingUnused;
  ap_prc->bitrate_ = ARATELIA_WEBM_DEMUXER_DEFAULT_BIT_RATE_KBITS;

  tiz_buffer_clear (ap_prc->p_store_);

  update_cache_size (ap_prc);

  tiz_filter_prc_update_eos_flag (ap_prc, false);
}

static void reset_nestegg_object (webmdmuxflt_prc_t *ap_prc)
{
  assert (ap_prc);
  assert (!ap_prc->p_ne_ctx_);
  ap_prc->p_ne_ctx_ = NULL;
  ap_prc->ne_io_.read = ne_io_read;
  ap_prc->ne_io_.seek = ne_io_seek;
  ap_prc->ne_io_.tell = ne_io_tell;
  ap_prc->ne_io_.userdata = ap_prc;
}

static inline void deallocate_temp_data_store (
    /*@special@ */ webmdmuxflt_prc_t *ap_prc)
/*@releases ap_prc->p_store_@ */
/*@ensures isnull ap_prc->p_store_@ */
{
  assert (ap_prc);
  tiz_buffer_destroy (ap_prc->p_store_);
  ap_prc->p_store_ = NULL;
}

static OMX_ERRORTYPE allocate_temp_data_store (webmdmuxflt_prc_t *ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;

  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX);
  tiz_check_omx_err (
      tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                            OMX_IndexParamPortDefinition, &port_def));

  assert (ap_prc->p_store_ == NULL);
  return tiz_buffer_init (&(ap_prc->p_store_), port_def.nBufferSize);
}

static inline void deallocate_nestegg_object (
    /*@special@ */ webmdmuxflt_prc_t *ap_prc)
/*@releases ap_prc->p_ne_ctx_@ */
/*@ensures isnull ap_prc->p_ne_ctx_@ */
{
  assert (ap_prc);
  if (ap_prc->p_ne_ctx_)
    {
      nestegg_destroy (ap_prc->p_ne_ctx_);
      ap_prc->p_ne_ctx_ = NULL;
    }
}

static OMX_ERRORTYPE obtain_track_info (webmdmuxflt_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  unsigned int tracks = 0;
  uint64_t duration = 0;
  int nestegg_rc = 0;
  int type;
  unsigned int i, j;
  unsigned int data_items = 0;
  unsigned char * codec_data, * ptr;
  size_t length;
  nestegg_video_params vparams;
  nestegg_audio_params aparams;

  assert(ap_prc);

  (void)nestegg_track_count(ap_prc->p_ne_ctx_, &tracks);
  nestegg_rc = nestegg_duration(ap_prc->p_ne_ctx_, &duration);

  if (nestegg_rc == 0)
    {
      TIZ_DEBUG (handleOf (ap_prc), "media has %u tracks and duration %fs",
                 tracks, duration / 1e9);
    }
  else
    {
      TIZ_DEBUG (handleOf (ap_prc),
                 "media has %u tracks and unknown duration\n", tracks);
    }

  for (i = 0; i < tracks; ++i)
    {
      type = nestegg_track_type (ap_prc->p_ne_ctx_, i);
      TIZ_DEBUG (handleOf (ap_prc), "track %u: type: %d codec: %d", i, type,
                 nestegg_track_codec_id (ap_prc->p_ne_ctx_, i));
      nestegg_track_codec_data_count (ap_prc->p_ne_ctx_, i, &data_items);
      for (j = 0; j < data_items; ++j)
        {
          nestegg_track_codec_data (ap_prc->p_ne_ctx_, i, j, &codec_data,
                                    &length);
          TIZ_DEBUG (handleOf (ap_prc), " (%p, %u)", codec_data, (unsigned int)length);
        }
      if (type == NESTEGG_TRACK_VIDEO)
        {
          nestegg_track_video_params (ap_prc->p_ne_ctx_, i, &vparams);
          TIZ_DEBUG (handleOf (ap_prc), " video: %ux%u (d: %ux%u %ux%ux%ux%u)",
                   vparams.width, vparams.height, vparams.display_width,
                   vparams.display_height, vparams.crop_top, vparams.crop_left,
                   vparams.crop_bottom, vparams.crop_right);
        }
      else if (type == NESTEGG_TRACK_AUDIO)
        {
          nestegg_track_audio_params (ap_prc->p_ne_ctx_, i, &aparams);
          TIZ_DEBUG (handleOf (ap_prc), " audio: %.2fhz %u bit %u channels", aparams.rate,
                   aparams.depth, aparams.channels);
        }
    }
  return rc;
}

static OMX_ERRORTYPE allocate_nestegg_object (webmdmuxflt_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  if (store_data (ap_prc))
    {
      int nestegg_rc = 0;
      assert (!p_prc->p_ne_ctx_);

      nestegg_rc
          = nestegg_init (&ap_prc->p_ne_ctx_, ap_prc->ne_io_, ne_log_cback, -1);

      if (0 != nestegg_rc)
        {
          TIZ_ERROR (handleOf (ap_prc),
                     "Unable to open the nestegg demuxer handle (error = %d).",
                     nestegg_rc);
          deallocate_nestegg_object(ap_prc);
          ap_prc->store_offset_ = 0;
        }
      else
        {
          TIZ_TRACE (handleOf (ap_prc),
                     "decoder_inited = TRUE - store_offset [%d]",
                     ap_prc->store_offset_);
          obtain_track_info(ap_prc);
          ap_prc->demuxer_inited_ = true;
          tiz_buffer_advance (ap_prc->p_store_, ap_prc->store_offset_);
          ap_prc->store_offset_ = 0;
        }
    }
  else
    {
      rc = OMX_ErrorInsufficientResources;
    }

  return rc;
}

/*
 * webmdmuxfltprc
 */

static void *webmdmuxflt_prc_ctor (void *ap_prc, va_list *app)
{
  webmdmuxflt_prc_t *p_prc
      = super_ctor (typeOf (ap_prc, "webmdmuxfltprc"), ap_prc, app);
  assert (p_prc);

  p_prc->p_outhdr_ = NULL;
  p_prc->p_store_ = NULL;

  reset_stream_parameters (p_prc);
  reset_nestegg_object (p_prc);

  return p_prc;
}

static void *webmdmuxflt_prc_dtor (void *ap_obj)
{
  (void)webmdmuxflt_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "webmdmuxfltprc"), ap_obj);
}

/* static OMX_ERRORTYPE */
/* webmdmuxflt_prc_read_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
 */
/* { */
/*   return OMX_ErrorNone; */
/* } */

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE webmdmuxflt_prc_allocate_resources (void *ap_prc,
                                                      OMX_U32 a_pid)
{
  webmdmuxflt_prc_t *p_prc = ap_prc;
  assert (p_prc);
  return allocate_temp_data_store (p_prc);
}

static OMX_ERRORTYPE webmdmuxflt_prc_deallocate_resources (void *ap_prc)
{
  webmdmuxflt_prc_t *p_prc = ap_prc;
  assert (p_prc);
  deallocate_temp_data_store (p_prc);
  deallocate_nestegg_object (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE webmdmuxflt_prc_prepare_to_transfer (void *ap_prc,
                                                       OMX_U32 a_pid)
{
  webmdmuxflt_prc_t *p_prc = ap_prc;
  assert (ap_prc);
  return prepare_for_port_auto_detection (p_prc);
}

static OMX_ERRORTYPE webmdmuxflt_prc_transfer_and_process (void *ap_prc,
                                                        OMX_U32 a_pid)
{
  webmdmuxflt_prc_t *p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (p_prc->auto_detect_on_)
    {
    }
  return rc;
}

static OMX_ERRORTYPE webmdmuxflt_prc_stop_and_return (void *ap_prc)
{
  webmdmuxflt_prc_t *p_prc = ap_prc;
  assert (p_prc);
  return release_buffer (p_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE webmdmuxflt_prc_buffers_ready (const void *ap_prc)
{
  webmdmuxflt_prc_t *p_prc = (webmdmuxflt_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_prc);

  if (!p_prc->demuxer_inited_)
    {
      rc = allocate_nestegg_object (p_prc);
    }

  if (p_prc->demuxer_inited_ && OMX_ErrorNone == rc)
    {
      while (OMX_ErrorNone == rc)
        {
          rc = transform_buffer (p_prc);
        }
      if (OMX_ErrorNotReady == rc)
        {
          rc = OMX_ErrorNone;
        }
    }
  return rc;
}


static OMX_ERRORTYPE webmdmuxflt_prc_pause (const void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE webmdmuxflt_prc_resume (const void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE webmdmuxflt_prc_port_flush (const void *ap_prc,
                                                OMX_U32 a_pid)
{
  webmdmuxflt_prc_t *p_prc = (webmdmuxflt_prc_t *)ap_prc;
  reset_stream_parameters (p_prc);
  return tiz_filter_prc_release_header (p_prc, a_pid);
}

static OMX_ERRORTYPE webmdmuxflt_prc_port_disable (const void *ap_prc,
                                                 OMX_U32 a_pid)
{
  webmdmuxflt_prc_t *p_prc = (webmdmuxflt_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = tiz_filter_prc_release_header (p_prc, a_pid);
  reset_stream_parameters (p_prc);
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, true);
  return rc;
}

static OMX_ERRORTYPE webmdmuxflt_prc_port_enable (const void *ap_prc,
                                                OMX_U32 a_pid)
{
  webmdmuxflt_prc_t *p_prc = (webmdmuxflt_prc_t *)ap_prc;
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, false);
  return OMX_ErrorNone;
}

/*
 * webmdmuxflt_prc_class
 */

static void *webmdmuxflt_prc_class_ctor (void *ap_prc, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_prc, "webmdmuxfltprc_class"), ap_prc, app);
}

/*
 * initialization
 */

void *webmdmuxflt_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void *webmdmuxfltprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizfilterprc), "webmdmuxfltprc_class", classOf (tizfilterprc),
       sizeof (webmdmuxflt_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, webmdmuxflt_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return webmdmuxfltprc_class;
}

void *webmdmuxflt_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void *webmdmuxfltprc_class = tiz_get_type (ap_hdl, "webmdmuxfltprc_class");
  TIZ_LOG_CLASS (webmdmuxfltprc_class);
  void *webmdmuxfltprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (webmdmuxfltprc_class, "webmdmuxfltprc", tizfilterprc, sizeof (webmdmuxflt_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, webmdmuxflt_prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, webmdmuxflt_prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, webmdmuxflt_prc_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, webmdmuxflt_prc_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, webmdmuxflt_prc_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, webmdmuxflt_prc_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, webmdmuxflt_prc_stop_and_return,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, webmdmuxflt_prc_buffers_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_pause, webmdmuxflt_prc_pause,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_resume, webmdmuxflt_prc_resume,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_flush, webmdmuxflt_prc_port_flush,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_disable, webmdmuxflt_prc_port_disable,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_enable, webmdmuxflt_prc_port_enable,
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return webmdmuxfltprc;
}
