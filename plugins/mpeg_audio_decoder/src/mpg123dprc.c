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
 * @file   mpg123dprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - MPEG audio decoder processor class implementation
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
#define TIZ_LOG_CATEGORY_NAME "tiz.mp3_decoder.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE mpg123d_prc_deallocate_resources (void *);

static OMX_ERRORTYPE
transform_buffer (mpg123d_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_in
      = tiz_filter_prc_get_header (ap_prc, ARATELIA_MPG123_DECODER_INPUT_PORT_INDEX);
  OMX_BUFFERHEADERTYPE *p_out
      = tiz_filter_prc_get_header (ap_prc, ARATELIA_MPG123_DECODER_OUTPUT_PORT_INDEX);

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
          TIZ_TRACE (handleOf (ap_prc), "Propagate EOS flag to output HEADER [%p]",
                     p_out);
          p_out->nFlags |= OMX_BUFFERFLAG_EOS;
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          p_in->nFlags   = 0;
          tiz_check_omx_err
            (tiz_filter_prc_release_header (ap_prc, ARATELIA_MPG123_DECODER_OUTPUT_PORT_INDEX));
        }
    }

  return rc;
}

static void reset_stream_parameters (mpg123d_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  tiz_filter_prc_update_eos_flag (ap_prc, false);
}

/*
 * mpg123dprc
 */

static void *
mpg123d_prc_ctor (void *ap_obj, va_list * app)
{
  mpg123d_prc_t *p_prc = super_ctor (typeOf (ap_obj, "mpg123dprc"), ap_obj, app);
  assert (NULL != p_prc);
  return p_prc;
}

static void *
mpg123d_prc_dtor (void *ap_obj)
{
  (void) mpg123d_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "mpg123dprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
mpg123d_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mpg123d_prc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mpg123d_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  mpg123d_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  reset_stream_parameters (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mpg123d_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mpg123d_prc_stop_and_return (void *ap_obj)
{
  return tiz_filter_prc_release_all_headers (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
mpg123d_prc_buffers_ready (const void *ap_prc)
{
  mpg123d_prc_t *p_prc = (mpg123d_prc_t *)ap_prc;
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

static OMX_ERRORTYPE
mpg123d_prc_port_enable (const void *ap_prc, OMX_U32 a_pid)
{
  mpg123d_prc_t *p_prc = (mpg123d_prc_t *)ap_prc;
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, false);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mpg123d_prc_port_disable (const void *ap_prc, OMX_U32 a_pid)
{
  mpg123d_prc_t *p_prc = (mpg123d_prc_t *)ap_prc;
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
 * mpg123d_prc_class
 */

static void *
mpg123d_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "mpg123dprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
mpg123d_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * mpg123dprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizfilterprc), "mpg123dprc_class", classOf (tizfilterprc), sizeof (mpg123d_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, mpg123d_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return mpg123dprc_class;
}

void *
mpg123d_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * mpg123dprc_class = tiz_get_type (ap_hdl, "mpg123dprc_class");
  TIZ_LOG_CLASS (mpg123dprc_class);
  void * mpg123dprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (mpg123dprc_class, "mpg123dprc", tizfilterprc, sizeof (mpg123d_prc_t),
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
     tiz_prc_port_enable, mpg123d_prc_port_enable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, mpg123d_prc_port_disable,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return mpg123dprc;
}
