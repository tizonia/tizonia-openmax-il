/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   cc_prc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL Chromecast renderer - base processor class
 * implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <tizplatform.h>

#include <tizport.h>
#include <tizport_decls.h>
#include <tizport-macros.h>
#include <tizkernel.h>

#include "cc_prc.h"
#include "cc_prc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.filterprc"
#endif

/* Forward declaration */
static OMX_ERRORTYPE
cc_prc_release_all_headers (cc_prc_t * ap_prc);

static OMX_ERRORTYPE
grow_internal_lists (cc_prc_t * ap_prc, const OMX_U32 a_pid)
{
  assert (ap_prc);
  assert (tiz_vector_length (ap_prc->p_hdrs_)
          == tiz_vector_length (ap_prc->p_disabled_flags_));
  assert (tiz_vector_length (ap_prc->p_hdrs_)
          == tiz_vector_length (ap_prc->p_port_dirs_));
  if (a_pid >= tiz_vector_length (ap_prc->p_hdrs_))
    {
      int i = 0;
      OMX_BUFFERHEADERTYPE * p_hdr = NULL;
      bool disabled_flag = false;
      OMX_DIRTYPE port_dir = OMX_DirMax;
      tiz_port_t * p_port
        = tiz_krn_get_port (tiz_get_krn (handleOf (ap_prc)), a_pid);
      assert (p_port);
      for (i = tiz_vector_length (ap_prc->p_hdrs_); i <= a_pid; ++i)
        {
          tiz_check_omx_ret_oom (
            tiz_vector_push_back (ap_prc->p_hdrs_, &p_hdr));
          disabled_flag = TIZ_PORT_IS_DISABLED (p_port);
          tiz_check_omx_ret_oom (
            tiz_vector_push_back (ap_prc->p_disabled_flags_, &disabled_flag));
          port_dir = tiz_port_dir (p_port);
          tiz_check_omx_ret_oom (
            tiz_vector_push_back (ap_prc->p_port_dirs_, &port_dir));
        }
    }
  return OMX_ErrorNone;
}

/*
 * filterprc
 */

static void *
cc_prc_ctor (void * ap_obj, va_list * app)
{
  cc_prc_t * p_prc = super_ctor (typeOf (ap_obj, "cc_prc"), ap_obj, app);
  assert (p_prc);

  tiz_check_omx_ret_null (
    tiz_vector_init (&(p_prc->p_hdrs_), sizeof (OMX_BUFFERHEADERTYPE *)));
  tiz_check_omx_ret_null (
    tiz_vector_init (&(p_prc->p_disabled_flags_), sizeof (bool)));
  tiz_check_omx_ret_null (
    tiz_vector_init (&(p_prc->p_port_dirs_), sizeof (OMX_DIRTYPE)));

  /* Initialise the internal lists */
  {
    OMX_U32 pid = 0;
    while (tiz_krn_get_port (tiz_get_krn (handleOf (p_prc)), pid))
      {
        grow_internal_lists (p_prc, pid);
        ++pid;
      }
  }

  p_prc->eos_ = false;
  return p_prc;
}

static void *
cc_prc_dtor (void * ap_obj)
{
  cc_prc_t * p_prc = ap_obj;
  assert (p_prc);
  tiz_vector_clear (p_prc->p_port_dirs_);
  tiz_vector_destroy (p_prc->p_port_dirs_);
  tiz_vector_clear (p_prc->p_disabled_flags_);
  tiz_vector_destroy (p_prc->p_disabled_flags_);
  tiz_vector_clear (p_prc->p_hdrs_);
  tiz_vector_destroy (p_prc->p_hdrs_);
  return super_dtor (typeOf (ap_obj, "cc_prc"), ap_obj);
}

static OMX_BUFFERHEADERTYPE **
cc_prc_get_header_ptr (cc_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE ** pp_hdr = NULL;
  assert (ap_prc);
  pp_hdr = tiz_vector_at (ap_prc->p_hdrs_, a_pid);
  assert (pp_hdr);
  return pp_hdr;
}

OMX_BUFFERHEADERTYPE **
cc_prc_get_header_ptr (void * ap_obj, const OMX_U32 a_pid)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->get_header_ptr);
  return class->get_header_ptr (ap_obj, a_pid);
}

static OMX_BUFFERHEADERTYPE *
cc_prc_get_header (cc_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  bool port_disabled = *(cc_prc_get_port_disabled_ptr (ap_prc, a_pid));

  if (!port_disabled)
    {
      OMX_BUFFERHEADERTYPE ** pp_hdr = cc_prc_get_header_ptr (ap_prc, a_pid);
      p_hdr = *pp_hdr;
      if (!p_hdr
          && (OMX_ErrorNone
              == tiz_krn_claim_buffer (tiz_get_krn (handleOf (ap_prc)), a_pid,
                                       0, pp_hdr)))
        {
          p_hdr = *pp_hdr;
          if (p_hdr)
            {
              TIZ_TRACE (handleOf (ap_prc),
                         "Claimed HEADER [%p] pid [%d] nFilledLen [%d]", p_hdr,
                         a_pid, p_hdr->nFilledLen);
            }
        }
    }

  return p_hdr;
}

OMX_BUFFERHEADERTYPE *
cc_prc_get_header (void * ap_obj, const OMX_U32 a_pid)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->get_header);
  return class->get_header (ap_obj, a_pid);
}

static bool
cc_prc_headers_available (const cc_prc_t * ap_prc)
{
  cc_prc_t * p_prc = (cc_prc_t *) ap_prc;
  OMX_U32 in_hdrs = 0;
  OMX_U32 out_hdrs = 0;
  assert (p_prc);

  {
    OMX_S32 i = 0;
    const OMX_S32 nhdrs = tiz_vector_length (p_prc->p_hdrs_);
    assert (nhdrs == tiz_vector_length (p_prc->p_disabled_flags_));
    assert (nhdrs == tiz_vector_length (p_prc->p_port_dirs_));
    for (i = 0; i < nhdrs; ++i)
      {
        const OMX_DIRTYPE * p_dir = tiz_vector_at (p_prc->p_port_dirs_, i);
        assert (p_dir);
        if (cc_prc_get_header (p_prc, i))
          {
            assert (*p_dir != OMX_DirMax);
            if (*p_dir == OMX_DirInput)
              {
                ++in_hdrs;
              }
            else
              {
                ++out_hdrs;
              }
          }
      }
    TIZ_DEBUG (handleOf (ap_prc), "nhdrs = %d - in = %d - out = %d", nhdrs,
               in_hdrs, out_hdrs);
  }
  return (in_hdrs > 0 && out_hdrs > 0);
}

bool
cc_prc_headers_available (const void * ap_obj)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->headers_available);
  return class->headers_available (ap_obj);
}

static bool
cc_prc_output_headers_available (const cc_prc_t * ap_prc)
{
  cc_prc_t * p_prc = (cc_prc_t *) ap_prc;
  OMX_U32 out_hdrs = 0;
  assert (p_prc);

  {
    OMX_S32 i = 0;
    const OMX_S32 nhdrs = tiz_vector_length (p_prc->p_hdrs_);
    assert (nhdrs == tiz_vector_length (p_prc->p_disabled_flags_));
    assert (nhdrs == tiz_vector_length (p_prc->p_port_dirs_));
    for (i = 0; i < nhdrs; ++i)
      {
        const OMX_DIRTYPE * p_dir = tiz_vector_at (p_prc->p_port_dirs_, i);
        assert (p_dir);
        if (cc_prc_get_header (p_prc, i))
          {
            assert (*p_dir != OMX_DirMax);
            if (*p_dir == OMX_DirOutput)
              {
                ++out_hdrs;
              }
          }
      }
    TIZ_DEBUG (handleOf (ap_prc), "nhdrs = %d - out = %d", nhdrs, out_hdrs);
  }
  return (out_hdrs > 0);
}

bool
cc_prc_output_headers_available (const void * ap_obj)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->output_headers_available);
  return class->output_headers_available (ap_obj);
}

static OMX_ERRORTYPE
cc_prc_release_header (cc_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);

  if (OMX_ALL == a_pid)
    {
      rc = cc_prc_release_all_headers (ap_prc);
    }
  else
    {
      OMX_BUFFERHEADERTYPE ** pp_hdr = cc_prc_get_header_ptr (ap_prc, a_pid);
      OMX_BUFFERHEADERTYPE * p_hdr = NULL;

      assert (pp_hdr);
      p_hdr = *pp_hdr;

      if (p_hdr)
        {
          TIZ_TRACE (handleOf (ap_prc),
                     "Releasing HEADER [%p] pid [%d] "
                     "nFilledLen [%d] nFlags [%d]",
                     p_hdr, a_pid, p_hdr->nFilledLen, p_hdr->nFlags);

          p_hdr->nOffset = 0;
          rc = tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_prc)), a_pid,
                                       p_hdr);
          *pp_hdr = NULL;
        }
    }
  return rc;
}

OMX_ERRORTYPE
cc_prc_release_header (void * ap_obj, const OMX_U32 a_pid)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->release_header);
  return class->release_header (ap_obj, a_pid);
}

static OMX_ERRORTYPE
cc_prc_release_all_headers (cc_prc_t * ap_prc)
{
  cc_prc_t * p_prc = ap_prc;
  OMX_S32 i = 0;
  const OMX_S32 nhdrs = tiz_vector_length (p_prc->p_hdrs_);
  for (i = 0; i < nhdrs; ++i)
    {
      tiz_check_omx (cc_prc_release_header (ap_prc, i));
    }
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
cc_prc_release_all_headers (void * ap_obj)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->release_all_headers);
  return class->release_all_headers (ap_obj);
}

static bool *
cc_prc_get_port_disabled_ptr (cc_prc_t * ap_prc, const OMX_U32 a_pid)
{
  bool * p_port_disabled = NULL;
  assert (ap_prc);
  p_port_disabled = tiz_vector_at (ap_prc->p_disabled_flags_, a_pid);
  assert (p_port_disabled);
  TIZ_DEBUG (handleOf (ap_prc), "pid = [%u] disabled [%s]", a_pid,
             (*p_port_disabled ? "YES" : "NO"));
  return p_port_disabled;
}

bool *
cc_prc_get_port_disabled_ptr (void * ap_obj, const OMX_U32 a_pid)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->get_port_disabled_ptr);
  return class->get_port_disabled_ptr (ap_obj, a_pid);
}

static bool
cc_prc_is_port_disabled (cc_prc_t * ap_prc, const OMX_U32 a_pid)
{
  assert (ap_prc);
  return *(cc_prc_get_port_disabled_ptr (ap_prc, a_pid));
}

bool
cc_prc_is_port_disabled (void * ap_obj, const OMX_U32 a_pid)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->is_port_disabled);
  return class->is_port_disabled (ap_obj, a_pid);
}

static bool
cc_prc_is_port_enabled (cc_prc_t * ap_prc, const OMX_U32 a_pid)
{
  assert (ap_prc);
  return !(cc_prc_is_port_disabled (ap_prc, a_pid));
}

bool
cc_prc_is_port_enabled (void * ap_obj, const OMX_U32 a_pid)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->is_port_enabled);
  return class->is_port_enabled (ap_obj, a_pid);
}

static bool
cc_prc_is_eos (const cc_prc_t * ap_prc)
{
  assert (ap_prc);
  return ap_prc->eos_;
}

bool
cc_prc_is_eos (const void * ap_obj)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->is_eos);
  return class->is_eos (ap_obj);
}

static void
cc_prc_update_eos_flag (cc_prc_t * ap_prc, const bool flag)
{
  assert (ap_prc);
  ap_prc->eos_ = flag;
}

void
cc_prc_update_eos_flag (void * ap_obj, const bool flag)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->update_eos_flag);
  class->update_eos_flag (ap_obj, flag);
}

static void
cc_prc_update_port_disabled_flag (cc_prc_t * ap_prc, const OMX_U32 a_pid,
                                  const bool flag)
{
  bool * p_port_disabled = cc_prc_get_port_disabled_ptr (ap_prc, a_pid);
  assert (ap_prc);
  assert (p_port_disabled);
  *p_port_disabled = flag;
}

void
cc_prc_update_port_disabled_flag (void * ap_obj, const OMX_U32 a_pid,
                                  const bool flag)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->update_port_disabled_flag);
  class->update_port_disabled_flag (ap_obj, a_pid, flag);
}

/*
 * cc_prc_class
 */

static void *
cc_prc_class_ctor (void * ap_obj, va_list * app)
{
  cc_prc_class_t * p_obj
    = super_ctor (typeOf (ap_obj, "cc_prc_class"), ap_obj, app);
  typedef void (*voidf) ();
  voidf selector = NULL;
  va_list ap;
  va_copy (ap, *app);

  /* NOTE: Start ignoring splint warnings in this section of code */
  /*@ignore@*/
  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) cc_prc_get_header_ptr)
        {
          *(voidf *) &p_obj->get_header_ptr = method;
        }
      else if (selector == (voidf) cc_prc_get_header)
        {
          *(voidf *) &p_obj->get_header = method;
        }
      else if (selector == (voidf) cc_prc_headers_available)
        {
          *(voidf *) &p_obj->headers_available = method;
        }
      else if (selector == (voidf) cc_prc_output_headers_available)
        {
          *(voidf *) &p_obj->output_headers_available = method;
        }
      else if (selector == (voidf) cc_prc_release_header)
        {
          *(voidf *) &p_obj->release_header = method;
        }
      else if (selector == (voidf) cc_prc_release_all_headers)
        {
          *(voidf *) &p_obj->release_all_headers = method;
        }
      else if (selector == (voidf) cc_prc_get_port_disabled_ptr)
        {
          *(voidf *) &p_obj->get_port_disabled_ptr = method;
        }
      else if (selector == (voidf) cc_prc_is_port_disabled)
        {
          *(voidf *) &p_obj->is_port_disabled = method;
        }
      else if (selector == (voidf) cc_prc_is_port_enabled)
        {
          *(voidf *) &p_obj->is_port_enabled = method;
        }
      else if (selector == (voidf) cc_prc_is_eos)
        {
          *(voidf *) &p_obj->is_eos = method;
        }
      else if (selector == (voidf) cc_prc_update_eos_flag)
        {
          *(voidf *) &p_obj->update_eos_flag = method;
        }
      else if (selector == (voidf) cc_prc_update_port_disabled_flag)
        {
          *(voidf *) &p_obj->update_port_disabled_flag = method;
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

void *
cc_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * cc_prc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "cc_prc_class", classOf (tizprc),
     sizeof (cc_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return cc_prc_class;
}

void *
cc_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * cc_prc_class = tiz_get_type (ap_hdl, "cc_prc_class");
  TIZ_LOG_CLASS (cc_prc_class);
  void * filterprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_prc_class, "cc_prc", tizprc, sizeof (cc_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, cc_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_header_ptr, cc_prc_get_header_ptr,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_header, cc_prc_get_header,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_headers_available, cc_prc_headers_available,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_output_headers_available, cc_prc_output_headers_available,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_release_header, cc_prc_release_header,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_release_all_headers, cc_prc_release_all_headers,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_port_disabled_ptr, cc_prc_get_port_disabled_ptr,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_is_port_disabled, cc_prc_is_port_disabled,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_is_port_enabled, cc_prc_is_port_enabled,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_is_eos, cc_prc_is_eos,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_update_eos_flag, cc_prc_update_eos_flag,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_update_port_disabled_flag, cc_prc_update_port_disabled_flag,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return filterprc;
}
