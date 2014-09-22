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
 * @file   sndfiledprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Sampled sound file decoder - processor class
 *implementation
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

#include "sndfiled.h"
#include "sndfiledprc.h"
#include "sndfiledprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.sndfile_decoder.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE sndfiled_prc_deallocate_resources (void *);

static sf_count_t sf_io_get_filelen (void *user_data)
{
  TIZ_TRACE (handleOf (user_data), "");
  return -1;
}

static sf_count_t sf_io_seek (sf_count_t offset, int whence, void *user_data)
{
  TIZ_TRACE (handleOf (user_data), "");
  return -1;
}

static sf_count_t sf_io_read (void *ptr, sf_count_t count, void *user_data)
{
  sndfiled_prc_t *p_prc = (sndfiled_prc_t *)user_data;
  sf_count_t total_bytes = 0;
  assert (NULL != p_prc);

  TIZ_TRACE (handleOf (p_prc), "");

  return total_bytes;
}

static sf_count_t sf_io_write (const void *ptr, sf_count_t count,
                               void *user_data)
{
  TIZ_TRACE (handleOf (user_data), "");
  return -1;
}

static sf_count_t sf_io_tell (void *user_data)
{
  TIZ_TRACE (handleOf (user_data), "");
  return -1;
}

static OMX_ERRORTYPE transform_buffer (sndfiled_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_in = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_PCM_DECODER_INPUT_PORT_INDEX);
  OMX_BUFFERHEADERTYPE *p_out = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_PCM_DECODER_OUTPUT_PORT_INDEX);

  if (NULL == p_in || NULL == p_out)
    {
      TIZ_TRACE (handleOf (ap_prc), "IN HEADER [%p] OUT HEADER [%p]", p_in,
                 p_out);
      return OMX_ErrorNone;
    }

  assert (NULL != ap_prc);

  if (0 == p_in->nFilledLen)
    {
      TIZ_TRACE (handleOf (ap_prc), "HEADER [%p] nFlags [%d] is empty", p_in,
                 p_in->nFlags);
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          /* Inmediately propagate EOS flag to output */
          TIZ_TRACE (handleOf (ap_prc),
                     "Propagate EOS flag to output HEADER [%p]", p_out);
          p_out->nFlags |= OMX_BUFFERFLAG_EOS;
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          p_in->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
          tiz_check_omx_err (tiz_filter_prc_release_header (
              ap_prc, ARATELIA_PCM_DECODER_OUTPUT_PORT_INDEX));
        }
    }

  return rc;
}

static void reset_stream_parameters (sndfiled_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  tiz_filter_prc_update_eos_flag (ap_prc, false);
}

/*
 * sndfiledprc
 */

static void *sndfiled_prc_ctor (void *ap_obj, va_list *app)
{
  sndfiled_prc_t *p_prc
      = super_ctor (typeOf (ap_obj, "sndfiledprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_sf_ = NULL;
  p_prc->sf_info_.format = 0;
  p_prc->sf_io_.get_filelen = sf_io_get_filelen;
  p_prc->sf_io_.seek = sf_io_seek;
  p_prc->sf_io_.read = sf_io_read;
  p_prc->sf_io_.write = sf_io_write;
  p_prc->sf_io_.tell = sf_io_tell;
  return p_prc;
}

static void *sndfiled_prc_dtor (void *ap_obj)
{
  (void)sndfiled_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "sndfiledprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE sndfiled_prc_allocate_resources (void *ap_obj,
                                                      OMX_U32 a_pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  sndfiled_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  p_prc->p_sf_ = sf_open_virtual(&(p_prc->sf_io_), SFM_READ, &(p_prc->sf_info_), p_prc);
  if (NULL == p_prc->p_sf_)
    {
      rc = OMX_ErrorInsufficientResources;
    }

  return rc;
}

static OMX_ERRORTYPE sndfiled_prc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE sndfiled_prc_prepare_to_transfer (void *ap_obj,
                                                       OMX_U32 a_pid)
{
  sndfiled_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  reset_stream_parameters (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE sndfiled_prc_transfer_and_process (void *ap_obj,
                                                        OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE sndfiled_prc_stop_and_return (void *ap_obj)
{
  return tiz_filter_prc_release_all_headers (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE sndfiled_prc_buffers_ready (const void *ap_prc)
{
  sndfiled_prc_t *p_prc = (sndfiled_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_prc);

  TIZ_TRACE (handleOf (p_prc), "eos [%s] ",
             tiz_filter_prc_is_eos (p_prc) ? "YES" : "NO");
  while (tiz_filter_prc_headers_available (p_prc) && OMX_ErrorNone == rc)
    {
      rc = transform_buffer (p_prc);
    }

  return rc;
}

static OMX_ERRORTYPE sndfiled_prc_port_enable (const void *ap_prc,
                                               OMX_U32 a_pid)
{
  sndfiled_prc_t *p_prc = (sndfiled_prc_t *)ap_prc;
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, false);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE sndfiled_prc_port_disable (const void *ap_prc,
                                                OMX_U32 a_pid)
{
  sndfiled_prc_t *p_prc = (sndfiled_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  if (OMX_ALL == a_pid)
    {
      rc = tiz_filter_prc_release_all_headers (p_prc);
    }
  else
    {
      rc = tiz_filter_prc_release_header (p_prc, a_pid);
    }
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, true);
  return rc;
}

/*
 * sndfiled_prc_class
 */

static void *sndfiled_prc_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "sndfiledprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *sndfiled_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void *sndfiledprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizfilterprc), "sndfiledprc_class", classOf (tizfilterprc),
       sizeof(sndfiled_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, sndfiled_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return sndfiledprc_class;
}

void *sndfiled_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void *sndfiledprc_class = tiz_get_type (ap_hdl, "sndfiledprc_class");
  TIZ_LOG_CLASS (sndfiledprc_class);
  void *sndfiledprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (sndfiledprc_class, "sndfiledprc", tizfilterprc, sizeof(sndfiled_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, sndfiled_prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, sndfiled_prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, sndfiled_prc_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, sndfiled_prc_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, sndfiled_prc_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, sndfiled_prc_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, sndfiled_prc_stop_and_return,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, sndfiled_prc_buffers_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_enable, sndfiled_prc_port_enable,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_disable, sndfiled_prc_port_disable,
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return sndfiledprc;
}
