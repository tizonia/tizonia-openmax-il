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
 * @file   frprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - File Reader processor class implementation
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

#include "fr.h"
#include "frprc.h"
#include "frprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.file_reader.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE fr_prc_deallocate_resources (void *);

static OMX_ERRORTYPE
transform_buffer (fr_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_in
      = tiz_filter_prc_get_header (ap_prc, ARATELIA_FILE_READER_INPUT_PORT_INDEX);
  OMX_BUFFERHEADERTYPE *p_out
      = tiz_filter_prc_get_header (ap_prc, ARATELIA_FILE_READER_OUTPUT_PORT_INDEX);

  if (NULL == p_in || NULL == p_out)
    {
      TIZ_TRACE (handleOf (ap_prc), "IN HEADER [%p] OUT HEADER [%p]", p_in,
                 p_out);
      return OMX_ErrorNone;
    }

  assert (ap_prc);

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
          tiz_check_omx
            (tiz_filter_prc_release_header (ap_prc, ARATELIA_FILE_READER_OUTPUT_PORT_INDEX));
        }
    }

  return rc;
}

static void reset_stream_parameters (fr_prc_t *ap_prc)
{
  assert (ap_prc);
  tiz_filter_prc_update_eos_flag (ap_prc, false);
}

/*
 * frprc
 */

static void *
fr_prc_ctor (void *ap_obj, va_list * app)
{
  fr_prc_t *p_prc = super_ctor (typeOf (ap_obj, "frprc"), ap_obj, app);
  assert (p_prc);
  return p_prc;
}

static void *
fr_prc_dtor (void *ap_obj)
{
  (void) fr_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "frprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
fr_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  fr_prc_t *p_prc = ap_obj;
  assert (p_prc);
  reset_stream_parameters (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_stop_and_return (void *ap_obj)
{
  return tiz_filter_prc_release_all_headers (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
fr_prc_buffers_ready (const void *ap_prc)
{
  fr_prc_t *p_prc = (fr_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_prc);

  TIZ_TRACE (handleOf (p_prc), "eos [%s] ",
             tiz_filter_prc_is_eos (p_prc) ? "YES" : "NO");
  while (tiz_filter_prc_headers_available (p_prc) && OMX_ErrorNone == rc)
    {
      rc = transform_buffer (p_prc);
    }

  return rc;
}

static OMX_ERRORTYPE
fr_prc_port_enable (const void *ap_prc, OMX_U32 a_pid)
{
  fr_prc_t *p_prc = (fr_prc_t *)ap_prc;
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, false);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_port_disable (const void *ap_prc, OMX_U32 a_pid)
{
  fr_prc_t *p_prc = (fr_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = tiz_filter_prc_release_header (p_prc, a_pid);
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, true);
  return rc;
}

/*
 * fr_prc_class
 */

static void *
fr_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "frprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
fr_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * frprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizfilterprc), "frprc_class", classOf (tizfilterprc), sizeof (fr_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, fr_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return frprc_class;
}

void *
fr_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * frprc_class = tiz_get_type (ap_hdl, "frprc_class");
  TIZ_LOG_CLASS (frprc_class);
  void * frprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (frprc_class, "frprc", tizfilterprc, sizeof (fr_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, fr_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, fr_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, fr_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, fr_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, fr_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, fr_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, fr_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, fr_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, fr_prc_port_enable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, fr_prc_port_disable,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return frprc;
}
