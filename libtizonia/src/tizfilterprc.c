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
 * @file   tizfilterprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - filter processor class implementation
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

#include "tizfilterprc.h"
#include "tizfilterprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.filterprc"
#endif

/* Forward declaration */
static OMX_ERRORTYPE filter_prc_release_all_headers (tiz_filter_prc_t *ap_prc);

/*
 * filterprc
 */

static void *filter_prc_ctor (void *ap_obj, va_list *app)
{
  tiz_filter_prc_t *p_prc
      = super_ctor (typeOf (ap_obj, "tizfilterprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_in_hdr_ = NULL;
  p_prc->p_out_hdr_ = NULL;
  p_prc->eos_ = false;
  p_prc->in_port_disabled_ = false;
  p_prc->out_port_disabled_ = false;
  return p_prc;
}

static void *filter_prc_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tizfilterprc"), ap_obj);
}

static OMX_BUFFERHEADERTYPE **
filter_prc_get_header_ptr (tiz_filter_prc_t *ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  assert (NULL != ap_prc);
  assert (a_pid <= TIZ_FILTER_OUTPUT_PORT_INDEX);
  pp_hdr = (a_pid == TIZ_FILTER_INPUT_PORT_INDEX
                ? &(ap_prc->p_in_hdr_)
                : &(ap_prc->p_out_hdr_));
  assert (NULL != pp_hdr);
  return pp_hdr;
}

OMX_BUFFERHEADERTYPE **
tiz_filter_prc_get_header_ptr (void *ap_obj, const OMX_U32 a_pid)
{
  const tiz_filter_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->get_header_ptr);
  return class->get_header_ptr (ap_obj, a_pid);
}

static OMX_BUFFERHEADERTYPE *
filter_prc_get_header (tiz_filter_prc_t *ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  bool port_disabled = *(tiz_filter_prc_get_port_disabled_ptr (ap_prc, a_pid));

  if (!port_disabled)
    {
      OMX_BUFFERHEADERTYPE **pp_hdr = tiz_filter_prc_get_header_ptr (ap_prc, a_pid);
      p_hdr = *pp_hdr;
      if (NULL == p_hdr
          && (OMX_ErrorNone == tiz_krn_claim_buffer
              (tiz_get_krn (handleOf (ap_prc)), a_pid, 0, pp_hdr)))
        {
          p_hdr = *pp_hdr;
          if (NULL != p_hdr)
            {
              TIZ_TRACE (handleOf (ap_prc),
                         "Claimed HEADER [%p] pid [%d] nFilledLen [%d]",
                         p_hdr, a_pid, p_hdr->nFilledLen);
            }
        }
    }

  return p_hdr;
}

OMX_BUFFERHEADERTYPE *
tiz_filter_prc_get_header (void *ap_obj, const OMX_U32 a_pid)
{
  const tiz_filter_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->get_header);
  return class->get_header (ap_obj, a_pid);
}

static bool
filter_prc_headers_available (const tiz_filter_prc_t *ap_prc)
{
  bool rc = true;
  tiz_filter_prc_t *p_prc = (tiz_filter_prc_t *)ap_prc;
  rc &= (NULL != tiz_filter_prc_get_header (p_prc, TIZ_FILTER_INPUT_PORT_INDEX));
  rc &= (NULL != tiz_filter_prc_get_header (p_prc, TIZ_FILTER_OUTPUT_PORT_INDEX));
  return rc;
}

bool
tiz_filter_prc_headers_available (const void *ap_obj)
{
  const tiz_filter_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->headers_available);
  return class->headers_available (ap_obj);
}

static OMX_ERRORTYPE
filter_prc_release_header (tiz_filter_prc_t *ap_prc, const OMX_U32 a_pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  if (OMX_ALL == a_pid)
    {
      rc = filter_prc_release_all_headers (ap_prc);
    }
  else
    {
      OMX_BUFFERHEADERTYPE **pp_hdr = tiz_filter_prc_get_header_ptr (ap_prc, a_pid);
      OMX_BUFFERHEADERTYPE *p_hdr = NULL;

      assert (NULL != pp_hdr);
      p_hdr = *pp_hdr;

      if (NULL != p_hdr)
        {
          TIZ_TRACE (handleOf (ap_prc), "Releasing HEADER [%p] pid [%d] "
                     "nFilledLen [%d] nFlags [%d]",
                     p_hdr, a_pid, p_hdr->nFilledLen, p_hdr->nFlags);

          p_hdr->nOffset = 0;
          rc = tiz_krn_release_buffer
             (tiz_get_krn (handleOf (ap_prc)), a_pid, p_hdr);
          *pp_hdr = NULL;
        }
    }
  return rc;
}

OMX_ERRORTYPE
tiz_filter_prc_release_header (void *ap_obj, const OMX_U32 a_pid)
{
  const tiz_filter_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->release_header);
  return class->release_header (ap_obj, a_pid);
}

static OMX_ERRORTYPE
filter_prc_release_all_headers (tiz_filter_prc_t *ap_prc)
{
  tiz_check_omx_err
  (tiz_filter_prc_release_header (ap_prc, TIZ_FILTER_INPUT_PORT_INDEX));
  tiz_check_omx_err
  (tiz_filter_prc_release_header (ap_prc, TIZ_FILTER_OUTPUT_PORT_INDEX));
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_filter_prc_release_all_headers (void *ap_obj)
{
  const tiz_filter_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->release_all_headers);
  return class->release_all_headers (ap_obj);
}

static bool *
filter_prc_get_port_disabled_ptr (tiz_filter_prc_t *ap_prc, const OMX_U32 a_pid)
{
  bool *p_port_disabled = NULL;
  assert (NULL != ap_prc);
  assert (a_pid <= TIZ_FILTER_OUTPUT_PORT_INDEX);
  p_port_disabled = (a_pid == TIZ_FILTER_INPUT_PORT_INDEX
                         ? &(ap_prc->in_port_disabled_)
                         : &(ap_prc->out_port_disabled_));
  assert (NULL != p_port_disabled);
  return p_port_disabled;
}

bool *
tiz_filter_prc_get_port_disabled_ptr (void *ap_obj, const OMX_U32 a_pid)
{
  const tiz_filter_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->get_port_disabled_ptr);
  return class->get_port_disabled_ptr (ap_obj, a_pid);
}

static bool
filter_prc_is_eos (const tiz_filter_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  return ap_prc->eos_;
}

bool
tiz_filter_prc_is_eos (const void *ap_obj)
{
  const tiz_filter_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->is_eos);
  return class->is_eos (ap_obj);
}

static void
filter_prc_update_eos_flag (tiz_filter_prc_t *ap_prc, const bool flag)
{
  assert (NULL != ap_prc);
  ap_prc->eos_ = flag;
}

void
tiz_filter_prc_update_eos_flag (void *ap_obj, const bool flag)
{
  const tiz_filter_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->update_eos_flag);
  class->update_eos_flag (ap_obj, flag);
}

static void
filter_prc_update_port_disabled_flag (tiz_filter_prc_t *ap_prc, const OMX_U32 a_pid, const bool flag)
{
  bool *p_port_disabled = tiz_filter_prc_get_port_disabled_ptr (ap_prc, a_pid);
  assert (NULL != ap_prc);
  assert (NULL != p_port_disabled);
  *p_port_disabled = flag;
}

void
tiz_filter_prc_update_port_disabled_flag (void *ap_obj, const OMX_U32 a_pid, const bool flag)
{
  const tiz_filter_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->update_port_disabled_flag);
  class->update_port_disabled_flag (ap_obj, a_pid, flag);
}

/*
 * filter_prc_class
 */

static void *filter_prc_class_ctor (void *ap_obj, va_list *app)
{
  tiz_filter_prc_class_t *p_obj
    = super_ctor (typeOf (ap_obj, "tizfilterprc_class"), ap_obj, app);
  typedef void (*voidf)();
  voidf selector = NULL;
  va_list ap;
  va_copy (ap, *app);

  /* NOTE: Start ignoring splint warnings in this section of code */
  /*@ignore@*/
  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf)tiz_filter_prc_get_header_ptr)
        {
          *(voidf *)&p_obj->get_header_ptr = method;
        }
      else if (selector == (voidf)tiz_filter_prc_get_header)
        {
          *(voidf *)&p_obj->get_header = method;
        }
      else if (selector == (voidf)tiz_filter_prc_headers_available)
        {
          *(voidf *)&p_obj->headers_available = method;
        }
      else if (selector == (voidf)tiz_filter_prc_release_header)
        {
          *(voidf *)&p_obj->release_header = method;
        }
      else if (selector == (voidf)tiz_filter_prc_release_all_headers)
        {
          *(voidf *)&p_obj->release_all_headers = method;
        }
      else if (selector == (voidf)tiz_filter_prc_get_port_disabled_ptr)
        {
          *(voidf *)&p_obj->get_port_disabled_ptr = method;
        }
      else if (selector == (voidf)tiz_filter_prc_is_eos)
        {
          *(voidf *)&p_obj->is_eos = method;
        }
      else if (selector == (voidf)tiz_filter_prc_update_eos_flag)
        {
          *(voidf *)&p_obj->update_eos_flag = method;
        }
      else if (selector == (voidf)tiz_filter_prc_update_port_disabled_flag)
        {
          *(voidf *)&p_obj->update_port_disabled_flag = method;
        }
    }
  /*@end@*/
  /* NOTE: Stop ignoring splint warnings in this section  */

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

void *tiz_filter_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *tizfilterprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "tizfilterprc_class", classOf (tizprc), sizeof(tiz_filter_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, filter_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizfilterprc_class;
}

void *tiz_filter_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *tizfilterprc_class = tiz_get_type (ap_hdl, "tizfilterprc_class");
  TIZ_LOG_CLASS (tizfilterprc_class);
  void *filterprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (tizfilterprc_class, "tizfilterprc", tizprc, sizeof(tiz_filter_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, filter_prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, filter_prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_filter_prc_get_header_ptr, filter_prc_get_header_ptr,
       /* TIZ_CLASS_COMMENT: */
       tiz_filter_prc_get_header, filter_prc_get_header,
       /* TIZ_CLASS_COMMENT: */
       tiz_filter_prc_headers_available, filter_prc_headers_available,
       /* TIZ_CLASS_COMMENT: */
       tiz_filter_prc_release_header, filter_prc_release_header,
       /* TIZ_CLASS_COMMENT: */
       tiz_filter_prc_release_all_headers, filter_prc_release_all_headers,
       /* TIZ_CLASS_COMMENT: */
       tiz_filter_prc_get_port_disabled_ptr, filter_prc_get_port_disabled_ptr,
       /* TIZ_CLASS_COMMENT: */
       tiz_filter_prc_is_eos, filter_prc_is_eos,
       /* TIZ_CLASS_COMMENT: */
       tiz_filter_prc_update_eos_flag, filter_prc_update_eos_flag,
       /* TIZ_CLASS_COMMENT: */
       tiz_filter_prc_update_port_disabled_flag, filter_prc_update_port_disabled_flag,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);

  return filterprc;
}
