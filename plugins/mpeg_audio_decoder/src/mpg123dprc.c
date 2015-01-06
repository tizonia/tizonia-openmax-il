/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   mpg123dprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - MPEG audio decoder processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <tizplatform.h>

#include <tizkernel.h>

#include "mpg123d.h"
#include "mpg123dprc.h"
#include "mpg123dprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.mpg123_decoder.prc"
#endif

/* This macro assumes the existence of an "ap_prc" local variable */
#define goto_end_on_mpg123_error(expr)                      \
  do                                                        \
    {                                                       \
      int mpg123_error = 0;                                 \
      if ((mpg123_error = (expr)) != MPG123_OK)             \
        {                                                   \
          TIZ_ERROR (handleOf (ap_prc),                     \
                     "[OMX_ErrorInsufficientResources] : "  \
                     "%s",                                  \
                     mpg123_plain_strerror (mpg123_error)); \
          goto end;                                         \
        }                                                   \
    }                                                       \
  while (0)

/* This macro assumes the existence of an "ap_prc" local variable */
#define goto_end_on_null(expr)                             \
  do                                                       \
    {                                                      \
      if (NULL == (expr))                                  \
        {                                                  \
          TIZ_ERROR (handleOf (ap_prc),                    \
                     "[OMX_ErrorInsufficientResources] : " \
                     "Expression returned NULL.");         \
          goto end;                                        \
        }                                                  \
    }                                                      \
  while (0)

/* Forward declarations */
static OMX_ERRORTYPE mpg123d_prc_deallocate_resources (void *);

static const char *mpeg_version_to_str (enum mpg123_version version)
{
  switch (version)
    {
      case MPG123_1_0:
        return "MPG123_1_0";
        break;
      case MPG123_2_0:
        return "MPG123_2_0";
        break;
      case MPG123_2_5:
        return "MPG123_3_0";
        break;
      default:
        break;
    };
  return "Unknown version";
}

static const char *mpeg_audio_mode_to_str (const enum mpg123_mode mode)
{
  switch (mode)
    {
      case MPG123_M_STEREO:
        return "MPG123_M_STEREO";
        break;
      case MPG123_M_JOINT:
        return "MPG123_M_JOINT";
        break;
      case MPG123_M_DUAL:
        return "MPG123_M_DUAL";
        break;
      case MPG123_M_MONO:
        return "MPG123_M_MONO";
        break;
      default:
        break;
    };
  return "Unknown mode";
}

static const char *mpeg_output_encoding_to_str (const int encoding)
{
  switch (encoding)
    {
      case MPG123_ENC_8:
        return "MPG123_ENC_8";
        break;
      case MPG123_ENC_16:
        return "MPG123_ENC_16";
        break;
      case MPG123_ENC_24:
        return "MPG123_ENC_24";
        break;
      case MPG123_ENC_32:
        return "MPG123_ENC_32";
        break;
      case MPG123_ENC_SIGNED:
        return "MPG123_ENC_SIGNED";
        break;
      case MPG123_ENC_FLOAT:
        return "MPG123_ENC_FLOAT";
        break;
      case MPG123_ENC_SIGNED_16:
        return "MPG123_ENC_SIGNED_16";
        break;
      case MPG123_ENC_UNSIGNED_16:
        return "MPG123_ENC_UNSIGNED_16";
        break;
      case MPG123_ENC_UNSIGNED_8:
        return "MPG123_ENC_UNSIGNED_8";
        break;
      case MPG123_ENC_SIGNED_8:
        return "MPG123_ENC_SIGNED_8";
        break;
      case MPG123_ENC_ULAW_8:
        return "MPG123_ENC_ULAW_8";
        break;
      case MPG123_ENC_ALAW_8:
        return "MPG123_ENC_ALAW_8";
        break;
      case MPG123_ENC_SIGNED_32:
        return "MPG123_ENC_SIGNED_32";
        break;
      case MPG123_ENC_UNSIGNED_32:
        return "MPG123_ENC_UNSIGNED_32";
        break;
      case MPG123_ENC_SIGNED_24:
        return "MPG123_ENC_SIGNED_24";
        break;
      case MPG123_ENC_UNSIGNED_24:
        return "MPG123_ENC_UNSIGNED_24";
        break;
      case MPG123_ENC_FLOAT_32:
        return "MPG123_ENC_FLOAT_32";
        break;
      case MPG123_ENC_FLOAT_64:
        return "MPG123_ENC_FLOAT_64";
        break;
      case MPG123_ENC_ANY:
        return "MPG123_ENC_ANY";
        break;
      default:
        break;
    };
  return "Unknown Encoding";
}

OMX_ERRORTYPE release_in_hdr (mpg123d_prc_t *ap_prc)
{
  OMX_BUFFERHEADERTYPE *p_in = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_MPG123_DECODER_INPUT_PORT_INDEX);

  assert (NULL != ap_prc);

  if (p_in)
    {
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          TIZ_TRACE (handleOf (ap_prc), "EOS flag received");
          /* Remember the EOS flag */
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          tiz_util_reset_eos_flag (p_in);
        }
      TIZ_TRACE (handleOf (ap_prc), "Releasing IN HEADER [%p]", p_in);
      tiz_filter_prc_release_header (ap_prc,
                                     ARATELIA_MPG123_DECODER_INPUT_PORT_INDEX);
    }
  return OMX_ErrorNone;
}

static long get_mpg123_buffer_fill (mpg123d_prc_t *ap_prc)
{
  double fval;
  long buffer_fill;
  assert (NULL != ap_prc);
  (void)mpg123_getstate (ap_prc->p_mpg123_, MPG123_BUFFERFILL, &buffer_fill,
                         &fval);
  return buffer_fill;
}

OMX_ERRORTYPE release_out_hdr (mpg123d_prc_t *ap_prc)
{
  OMX_BUFFERHEADERTYPE *p_out = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_MPG123_DECODER_OUTPUT_PORT_INDEX);
  if (p_out)
    {
      if (tiz_filter_prc_is_eos (ap_prc)
          && (get_mpg123_buffer_fill (ap_prc) == 0
              || ap_prc->need_to_feed_more_))
        {
          TIZ_TRACE (handleOf (ap_prc), "Propagating EOS flag - fill [%ld]",
                     get_mpg123_buffer_fill (ap_prc));
          tiz_util_set_eos_flag (p_out);
        }
      TIZ_TRACE (handleOf (ap_prc),
                 "Releasing OUT HEADER [%p] nFilledLen [%d] nAllocLen [%d]",
                 p_out, p_out->nFilledLen, p_out->nAllocLen);
      tiz_filter_prc_release_header (ap_prc,
                                     ARATELIA_MPG123_DECODER_OUTPUT_PORT_INDEX);
    }
  return OMX_ErrorNone;
}

static void retrieve_stream_format (mpg123d_prc_t *ap_prc)
{
  struct mpg123_frameinfo mi;
  long rate;
  int channels;
  int encoding;

  (void)mpg123_info (ap_prc->p_mpg123_, &mi);
  TIZ_TRACE (handleOf (ap_prc),
             "stream info : version [%s] layer [%d] rate [%ld] mode [%s]",
             mpeg_version_to_str (mi.version), mi.layer, mi.rate,
             mpeg_audio_mode_to_str (mi.mode));

  (void)mpg123_getformat (ap_prc->p_mpg123_, &rate, &channels, &encoding);
  TIZ_TRACE (handleOf (ap_prc),
             "output format : rate [%ld] channels [%d] encoding [%s]", rate,
             channels, mpeg_output_encoding_to_str (encoding));
}

static OMX_ERRORTYPE consume_decoded_data (mpg123d_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_out = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_MPG123_DECODER_OUTPUT_PORT_INDEX);

  if (p_out)
    {
      size_t bytes_decoded;
      const int ret
          = mpg123_read (ap_prc->p_mpg123_, TIZ_OMX_BUF_PTR (p_out),
                         TIZ_OMX_BUF_ALLOC_LEN (p_out), &bytes_decoded);
      switch (ret)
        {
          case MPG123_OK:
            ap_prc->need_to_feed_more_ = false;
            break;
          case MPG123_NEED_MORE:
            ap_prc->need_to_feed_more_ = true;
            break;
          default:
            {
              TIZ_ERROR (handleOf (ap_prc),
                         "[OMX_ErrorInsufficientResources] : "
                         "mpg123_read error : [%s]",
                         mpg123_plain_strerror (ret));
              rc = OMX_ErrorInsufficientResources;
            }
            break;
        };

      if (OMX_ErrorNone == rc)
        {
          p_out->nFilledLen = bytes_decoded;
          if (p_out->nFilledLen > 0)
            {
              rc = release_out_hdr (ap_prc);
            }
        }
    }
  return rc;
}

static bool need_to_feed_more_data (mpg123d_prc_t *ap_prc)
{
  bool rc = false;
  if (get_mpg123_buffer_fill (ap_prc)
      < ARATELIA_MPG123_DECODER_BUF_FILL_THRESHOLD)
    {
      rc = true;
    }
  return rc;
}

static bool may_consume_more_data (mpg123d_prc_t *ap_prc)
{
  bool rc = false;
  assert (NULL != ap_prc);
  rc = !(ap_prc->need_to_feed_more_)
       && (NULL != tiz_filter_prc_get_header (
                       ap_prc, ARATELIA_MPG123_DECODER_OUTPUT_PORT_INDEX));
  return rc;
}

static OMX_ERRORTYPE feed_encoded_data (mpg123d_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_in = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_MPG123_DECODER_INPUT_PORT_INDEX);

  assert (NULL != ap_prc);

  if (p_in)
    {
      const int ret = mpg123_feed (ap_prc->p_mpg123_, TIZ_OMX_BUF_PTR (p_in),
                                   TIZ_OMX_BUF_FILL_LEN (p_in));
      if (ret != MPG123_OK)
        {
          TIZ_ERROR (
              handleOf (ap_prc),
              "[OMX_ErrorInsufficientResources] : mpg123_feed error : [%s]",
              mpg123_plain_strerror (ret));
          rc = OMX_ErrorInsufficientResources;
        }
      else
        {
          p_in->nFilledLen = 0;
          rc = release_in_hdr (ap_prc);
        }
    }
  return rc;
}

static OMX_ERRORTYPE decode_stream (mpg123d_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  do
    {
      if (need_to_feed_more_data (ap_prc))
        {
          tiz_check_omx_err (feed_encoded_data (ap_prc));
        }
      tiz_check_omx_err (consume_decoded_data (ap_prc));
    }
  while (may_consume_more_data (ap_prc));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE query_format (mpg123d_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_in = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_MPG123_DECODER_INPUT_PORT_INDEX);

  if (p_in)
    {
      int mpg123_ret = 0;
      size_t bytes_decoded = 0;
      assert (NULL != ap_prc);
      assert (NULL != ap_prc->p_mpg123_);

      mpg123_ret = mpg123_decode (ap_prc->p_mpg123_, TIZ_OMX_BUF_PTR (p_in),
                                  TIZ_OMX_BUF_FILL_LEN (p_in), NULL, 0,
                                  &bytes_decoded);
      p_in->nFilledLen = 0;

      if (MPG123_NEW_FORMAT == mpg123_ret)
        {
          TIZ_TRACE (handleOf (ap_prc), "Found new format");
          ap_prc->found_format_ = true;
          retrieve_stream_format (ap_prc);
          rc = consume_decoded_data (ap_prc);
        }
      (void)release_in_hdr (ap_prc);
    }

  return rc;
}

static void reset_stream_parameters (mpg123d_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  ap_prc->found_format_ = false;
  ap_prc->need_to_feed_more_ = true;
  tiz_filter_prc_update_eos_flag (ap_prc, false);
}

/*
 * mpg123dprc
 */

static void *mpg123d_prc_ctor (void *ap_obj, va_list *app)
{
  mpg123d_prc_t *p_prc
      = super_ctor (typeOf (ap_obj, "mpg123dprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_mpg123_ = NULL;
  reset_stream_parameters (p_prc);
  if (MPG123_OK != mpg123_init ())
    {
      TIZ_ERROR (handleOf (p_prc), "[%s] : initialising libmpg123.",
                 tiz_err_to_str (OMX_ErrorInsufficientResources));
    }
  return p_prc;
}

static void *mpg123d_prc_dtor (void *ap_obj)
{
  (void)mpg123d_prc_deallocate_resources (ap_obj);
  mpg123_exit ();
  return super_dtor (typeOf (ap_obj, "mpg123dprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE mpg123d_prc_allocate_resources (void *ap_prc,
                                                     OMX_U32 a_pid)
{
  mpg123d_prc_t *p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  int ret = 0;

  assert (NULL != p_prc);

  p_prc->p_mpg123_ = mpg123_new (NULL, &ret);
  goto_end_on_mpg123_error (ret);

  ret = mpg123_open_feed (p_prc->p_mpg123_);
  goto_end_on_mpg123_error (ret);

  /* Everything went well  */
  rc = OMX_ErrorNone;

end:

  if (OMX_ErrorInsufficientResources == rc)
    {
      mpg123_delete (p_prc->p_mpg123_); /* Closes, too. */
      p_prc->p_mpg123_ = NULL;
    }

  return rc;
}

static OMX_ERRORTYPE mpg123d_prc_deallocate_resources (void *ap_obj)
{
  mpg123d_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  mpg123_delete (p_prc->p_mpg123_); /* Closes, too. */
  p_prc->p_mpg123_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE mpg123d_prc_prepare_to_transfer (void *ap_obj,
                                                      OMX_U32 a_pid)
{
  mpg123d_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  reset_stream_parameters (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE mpg123d_prc_transfer_and_process (void *ap_obj,
                                                       OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE mpg123d_prc_stop_and_return (void *ap_obj)
{
  return tiz_filter_prc_release_all_headers (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE mpg123d_prc_buffers_ready (const void *ap_prc)
{
  mpg123d_prc_t *p_prc = (mpg123d_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_prc);

  rc = !p_prc->found_format_ ? query_format (p_prc) : decode_stream (p_prc);

  return rc;
}

static OMX_ERRORTYPE mpg123d_proc_port_flush (const void *ap_prc, OMX_U32 a_pid)
{
  mpg123d_prc_t *p_prc = (mpg123d_prc_t *)ap_prc;
  reset_stream_parameters (p_prc);
  return tiz_filter_prc_release_header (p_prc, a_pid);
}

static OMX_ERRORTYPE mpg123d_prc_port_enable (const void *ap_prc, OMX_U32 a_pid)
{
  mpg123d_prc_t *p_prc = (mpg123d_prc_t *)ap_prc;
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, false);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE mpg123d_prc_port_disable (const void *ap_prc,
                                               OMX_U32 a_pid)
{
  mpg123d_prc_t *p_prc = (mpg123d_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = tiz_filter_prc_release_header (p_prc, a_pid);
  reset_stream_parameters (p_prc);
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, true);
  return rc;
}

/*
 * mpg123d_prc_class
 */

static void *mpg123d_prc_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "mpg123dprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *mpg123d_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void *mpg123dprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizfilterprc), "mpg123dprc_class", classOf (tizfilterprc),
       sizeof(mpg123d_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, mpg123d_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return mpg123dprc_class;
}

void *mpg123d_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void *mpg123dprc_class = tiz_get_type (ap_hdl, "mpg123dprc_class");
  TIZ_LOG_CLASS (mpg123dprc_class);
  void *mpg123dprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (mpg123dprc_class, "mpg123dprc", tizfilterprc, sizeof(mpg123d_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, mpg123d_prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, mpg123d_prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, mpg123d_prc_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, mpg123d_prc_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, mpg123d_prc_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, mpg123d_prc_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, mpg123d_prc_stop_and_return,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, mpg123d_prc_buffers_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_flush, mpg123d_proc_port_flush,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_enable, mpg123d_prc_port_enable,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_disable, mpg123d_prc_port_disable,
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return mpg123dprc;
}
