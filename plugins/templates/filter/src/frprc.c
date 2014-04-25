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
 * @file   frprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - File Reader processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <tizkernel.h>
#include <tizscheduler.h>
#include <tizplatform.h>

#include "fr.h"
#include "frprc_decls.h"
#include "frprc.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.file_reader.prc"
#endif

#define TIZ_FILE_READER_UPDATE_DISABLED_FLAG(prc,pid,bool_value)        \
  do                                                                    \
    {                                                                   \
      bool *p_port_disabled = NULL;                                     \
      p_port_disabled = get_port_disabled_ptr (prc, pid);               \
      assert (NULL != p_port_disabled);                                 \
      *p_port_disabled = bool_value;                                    \
    } while(0)

/* Forward declarations */
static OMX_ERRORTYPE fr_prc_deallocate_resources (void *);

static inline
OMX_BUFFERHEADERTYPE **get_header_ptr (fr_prc_t *ap_prc,
                                       const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  assert (NULL != ap_prc);
  assert (a_pid <= ARATELIA_FILE_READER_OUTPUT_PORT_INDEX);
  pp_hdr = (a_pid == ARATELIA_FILE_READER_INPUT_PORT_INDEX
                ? &(ap_prc->p_in_hdr_)
                : &(ap_prc->p_out_hdr_));
  assert (NULL != pp_hdr);
  return pp_hdr;
}

static inline bool
*get_port_disabled_ptr (fr_prc_t *ap_prc, const OMX_U32 a_pid)
{
  bool *p_port_disabled = NULL;
  assert (NULL != ap_prc);
  assert (a_pid <= ARATELIA_FILE_READER_OUTPUT_PORT_INDEX);
  p_port_disabled = (a_pid == ARATELIA_FILE_READER_INPUT_PORT_INDEX
                         ? &(ap_prc->in_port_disabled_)
                         : &(ap_prc->out_port_disabled_));
  assert (NULL != p_port_disabled);
  return p_port_disabled;
}

static OMX_BUFFERHEADERTYPE
*get_header (fr_prc_t *ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  bool port_disabled = *(get_port_disabled_ptr (ap_prc, a_pid));

  if (!port_disabled)
    {
      p_hdr = *(get_header_ptr (ap_prc, a_pid));
      if (NULL == p_hdr)
        {
          if (OMX_ErrorNone
              == tiz_krn_claim_buffer (tiz_get_krn (handleOf (ap_prc)), a_pid,
                                       0, &p_hdr))
            {
              if (NULL != p_hdr)
                {
                  TIZ_TRACE (handleOf (ap_prc),
                             "Claimed HEADER [%p] pid [%d] nFilledLen [%d]",
                             p_hdr, a_pid, p_hdr->nFilledLen);
                }
            }
        }
    }

  return p_hdr;
}

static void
update_disabled_flag (void *ap_prc, const OMX_U32 a_pid, const bool val)
{
  assert (ARATELIA_FILE_READER_INPUT_PORT_INDEX == a_pid
          || ARATELIA_FILE_READER_OUTPUT_PORT_INDEX == a_pid
          || OMX_ALL == a_pid);
  if (OMX_ALL == a_pid)
    {
      TIZ_FILE_READER_UPDATE_DISABLED_FLAG (ap_prc, ARATELIA_FILE_READER_INPUT_PORT_INDEX, val);
      TIZ_FILE_READER_UPDATE_DISABLED_FLAG (ap_prc, ARATELIA_FILE_READER_OUTPUT_PORT_INDEX, val);
    }
  else
    {
      TIZ_FILE_READER_UPDATE_DISABLED_FLAG (ap_prc, a_pid, val);
    }
}

static OMX_ERRORTYPE
release_header (fr_prc_t *ap_prc, const OMX_U32 a_pid)
{
  assert (ARATELIA_FILE_READER_INPUT_PORT_INDEX == a_pid
          || ARATELIA_FILE_READER_OUTPUT_PORT_INDEX == a_pid);

  {
    OMX_BUFFERHEADERTYPE **pp_hdr = get_header_ptr (ap_prc, a_pid);
    OMX_BUFFERHEADERTYPE *p_hdr = NULL;

    p_hdr = *pp_hdr;
    if (NULL != p_hdr)
      {
        TIZ_TRACE (handleOf (ap_prc), "Releasing HEADER [%p] pid [%d] "
                   "nFilledLen [%d] nFlags [%d]",
                   p_hdr, a_pid, p_hdr->nFilledLen, p_hdr->nFlags);
        p_hdr->nOffset = 0;
        tiz_check_omx_err (tiz_krn_release_buffer
                           (tiz_get_krn (handleOf (ap_prc)), a_pid, p_hdr));
        *pp_hdr = NULL;
      }
  }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
release_all_headers (fr_prc_t *ap_prc)
{
  tiz_check_omx_err (release_header (ap_prc, ARATELIA_FILE_READER_INPUT_PORT_INDEX));
  tiz_check_omx_err (release_header (ap_prc, ARATELIA_FILE_READER_OUTPUT_PORT_INDEX));
  return OMX_ErrorNone;
}

static inline bool
buffers_available (fr_prc_t *ap_prc)
{
  bool rc = true;
  rc &= (NULL != get_header (ap_prc, ARATELIA_FILE_READER_INPUT_PORT_INDEX));
  rc &= (NULL != get_header (ap_prc, ARATELIA_FILE_READER_OUTPUT_PORT_INDEX));
  return rc;
}

static OMX_ERRORTYPE
transform_buffer (fr_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_in
      = get_header (ap_prc, ARATELIA_FILE_READER_INPUT_PORT_INDEX);
  OMX_BUFFERHEADERTYPE *p_out
      = get_header (ap_prc, ARATELIA_FILE_READER_OUTPUT_PORT_INDEX);

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
          TIZ_TRACE (handleOf (ap_prc), "Propagate EOS flag to output HEADER [%p]",
                     p_out);
          p_out->nFlags |= OMX_BUFFERFLAG_EOS;
          p_in->nFlags   = 0;
          tiz_check_omx_err
            (release_header (ap_prc, ARATELIA_FILE_READER_OUTPUT_PORT_INDEX));
        }
    }

  return rc;
}

/*
 * frprc
 */

static void *
fr_prc_ctor (void *ap_obj, va_list * app)
{
  fr_prc_t *p_prc           = super_ctor (typeOf (ap_obj, "frprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_in_hdr_          = NULL;
  p_prc->p_out_hdr_         = NULL;
  p_prc->eos_               = false;
  p_prc->in_port_disabled_  = false;
  p_prc->out_port_disabled_ = false;
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
  assert (NULL != p_prc);
  p_prc->eos_ = false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  assert (NULL == (*get_header_ptr (ap_obj, ARATELIA_FILE_READER_INPUT_PORT_INDEX)));
  assert (NULL == (*get_header_ptr (ap_obj, ARATELIA_FILE_READER_OUTPUT_PORT_INDEX)));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_stop_and_return (void *ap_obj)
{
  return release_all_headers (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
fr_prc_buffers_ready (const void *ap_obj)
{
  fr_prc_t *p_prc = (fr_prc_t *)ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);

  while (buffers_available (p_prc) && OMX_ErrorNone == rc)
    {
      rc = transform_buffer (p_prc);
    }

  return rc;
}

static OMX_ERRORTYPE
fr_prc_port_enable (const void *ap_obj, OMX_U32 a_pid)
{
  fr_prc_t *p_prc = (fr_prc_t *)ap_obj;
  bool flag = false;
  update_disabled_flag (p_prc, a_pid, flag);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_port_disable (const void *ap_obj, OMX_U32 a_pid)
{
  fr_prc_t *p_prc = (fr_prc_t *)ap_obj;
  bool flag = true;
  if (OMX_ALL == a_pid)
    {
      tiz_check_omx_err (release_all_headers (p_prc));
    }
  else
    {
      tiz_check_omx_err (release_header (p_prc, a_pid));
    }
  update_disabled_flag (p_prc, a_pid, flag);
  return OMX_ErrorNone;
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
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * frprc_class = factory_new (classOf (tizprc),
                                    "frprc_class",
                                    classOf (tizprc),
                                    sizeof (fr_prc_class_t),
                                    ap_tos, ap_hdl,
                                    ctor, fr_prc_class_ctor, 0);
  return frprc_class;
}

void *
fr_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * frprc_class = tiz_get_type (ap_hdl, "frprc_class");
  TIZ_LOG_CLASS (frprc_class);
  void * frprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (frprc_class, "frprc", tizprc, sizeof (fr_prc_t),
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
