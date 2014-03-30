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
 * @file   webpdprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - WebP Decoder processor class
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <tizosal.h>

#include <tizkernel.h>

#include "webpdprc.h"
#include "webpdprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.webp_decoder.prc"
#endif

/*
 * webpdprc
 */

static void *
webpd_prc_ctor (void *ap_obj, va_list * app)
{
  webpd_prc_t *p_obj = super_ctor (typeOf (ap_obj, "webpdprc"), ap_obj, app);
  p_obj->pinhdr_ = 0;
  p_obj->pouthdr_ = 0;
  p_obj->eos_ = false;
  return p_obj;
}

static void *
webpd_prc_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "webpdprc"), ap_obj);
}

static OMX_ERRORTYPE
webpd_prc_transform_buffer (const void *ap_obj)
{
  webpd_prc_t *p_obj = (webpd_prc_t *) ap_obj;
  (void) p_obj;
  return OMX_ErrorNone;
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
webpd_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  webpd_prc_t *p_obj = ap_obj;
  assert (ap_obj);
  (void) p_obj;
  TIZ_TRACE (handleOf (ap_obj),
                 "Resource allocation complete..." "pid = [%d]", a_pid);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
webpd_prc_deallocate_resources (void *ap_obj)
{
  webpd_prc_t *p_obj = ap_obj;
  assert (ap_obj);
  (void) p_obj;
  TIZ_TRACE (handleOf (ap_obj),
                 "Resource deallocation complete...");
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
webpd_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  assert (ap_obj);
  TIZ_TRACE (handleOf (ap_obj),
           "Transfering buffers...pid [%d]", a_pid);
  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
webpd_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  assert (ap_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
webpd_prc_stop_and_return (void *ap_obj)
{
  webpd_prc_t *p_obj = ap_obj;
  assert (ap_obj);
  (void) p_obj;
  return OMX_ErrorNone;
}

/*
 * from tiz_prc class
 */

static bool
claim_input (const void *ap_obj)
{
  webpd_prc_t *p_prc = (webpd_prc_t *) ap_obj;
  bool rc = false;
  assert (NULL != p_prc);

  if (OMX_ErrorNone == tiz_krn_claim_buffer
      (tiz_get_krn (handleOf (p_prc)), 0, 0, &p_prc->pinhdr_))
    {
      if (NULL != p_prc->pinhdr_)
        {
          TIZ_TRACE (handleOf (p_prc),
                     "Claimed INPUT HEADER [%p]...", p_prc->pinhdr_);
          rc = true;
        }
    }

  return rc;
}

static bool
claim_output (const void *ap_obj)
{
  webpd_prc_t *p_prc = (webpd_prc_t *) ap_obj;
  bool rc = false;
  assert (NULL != p_prc);

  if (OMX_ErrorNone == tiz_krn_claim_buffer
      (tiz_get_krn (handleOf (p_prc)), 1, 0, &p_prc->pouthdr_))
    {
      if (NULL != p_prc->pouthdr_)
        {
          TIZ_TRACE (handleOf (p_prc),
                     "Claimed OUTPUT HEADER [%p] BUFFER [%p] "
                     "nFilledLen [%d]...", p_prc->pouthdr_,
                     p_prc->pouthdr_->pBuffer, p_prc->pouthdr_->nFilledLen);
          rc = true;
        }
    }

  return rc;
}

static OMX_ERRORTYPE
webpd_prc_buffers_ready (const void *ap_obj)
{
  webpd_prc_t *p_obj = (webpd_prc_t *) ap_obj;

  TIZ_TRACE (handleOf (ap_obj), "Buffers ready...");

  while (1)
    {

      if (!p_obj->pinhdr_)
        {
          if (!claim_input (ap_obj) || !p_obj->pinhdr_)
            {
              break;
            }
        }

      if (!p_obj->pouthdr_)
        {
          if (!claim_output (ap_obj))
            {
              break;
            }
        }

      tiz_check_omx_err (webpd_prc_transform_buffer (ap_obj));
      if (p_obj->pinhdr_ && (0 == p_obj->pinhdr_->nFilledLen))
        {
          p_obj->pinhdr_->nOffset = 0;
          tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_obj)), 0, p_obj->pinhdr_);
          p_obj->pinhdr_ = NULL;
        }
    }

  if (p_obj->eos_ && p_obj->pouthdr_)
    {
      /* EOS has been received and all the input data has been consumed
       * already, so its time to propagate the EOS flag */
      TIZ_TRACE (handleOf (ap_obj),
                "p_obj->eos OUTPUT HEADER [%p]...", p_obj->pouthdr_);
      p_obj->pouthdr_->nFlags |= OMX_BUFFERFLAG_EOS;
      tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_obj)), 1, p_obj->pouthdr_);
      p_obj->pouthdr_ = NULL;
    }

  return OMX_ErrorNone;
}

/*
 * webpd_prc_class
 */

static void *
webpd_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "webpdprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
webpd_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * webpdprc_class = factory_new (classOf (tizprc),
                                       "webpdprc_class",
                                       classOf (tizprc),
                                       sizeof (webpd_prc_class_t),
                                       ap_tos, ap_hdl,
                                       ctor, webpd_prc_class_ctor, 0);
  return webpdprc_class;
}

void *
webpd_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * webpdprc_class = tiz_get_type (ap_hdl, "webpdprc_class");
  TIZ_LOG_CLASS (webpdprc_class);
  void * webpdprc =
    factory_new
    (webpdprc_class,
     "webpdprc",
     tizprc,
     sizeof (webpd_prc_t),
     ap_tos, ap_hdl,
     ctor, webpd_prc_ctor,
     dtor, webpd_prc_dtor,
     tiz_prc_buffers_ready, webpd_prc_buffers_ready,
     tiz_srv_allocate_resources, webpd_prc_allocate_resources,
     tiz_srv_deallocate_resources, webpd_prc_deallocate_resources,
     tiz_srv_prepare_to_transfer, webpd_prc_prepare_to_transfer,
     tiz_srv_transfer_and_process, webpd_prc_transfer_and_process,
     tiz_srv_stop_and_return, webpd_prc_stop_and_return, 0);

  return webpdprc;
}
