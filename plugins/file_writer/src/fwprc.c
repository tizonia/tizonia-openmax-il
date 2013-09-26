/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
 * @file   fwprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Binary Writer processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "OMX_Core.h"
#include "fwprc.h"
#include "fwprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizosal.h"

#include <assert.h>
#include <stdio.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.file_writer.prc"
#endif

/*
 * fwprc
 */

static void *
fw_proc_ctor (void *ap_obj, va_list * app)
{
  fw_prc_t *p_obj     = super_ctor (fwprc, ap_obj, app);
  p_obj->p_file_      = NULL;
  p_obj->p_uri_param_ = NULL;
  p_obj->counter_     = 0;
  p_obj->eos_         = false;
  return p_obj;
}

static void *
fw_proc_dtor (void *ap_obj)
{
  fw_prc_t *p_obj = ap_obj;

  if (p_obj->p_file_)
    {
      fclose (p_obj->p_file_);
    }

  if (p_obj->p_uri_param_)
    {
      tiz_mem_free (p_obj->p_uri_param_);
    }

  return super_dtor (fwprc, ap_obj);
}

static OMX_ERRORTYPE
fw_proc_write_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
{
  fw_prc_t *p_obj = (fw_prc_t *) ap_obj;
  int elems_written = 0;

  if (p_obj->p_file_ && !(p_obj->eos_) && p_hdr->nFilledLen > 0)
    {
      if (1 != (elems_written
                = fwrite (p_hdr->pBuffer + p_hdr->nOffset,
                          p_hdr->nFilledLen, 1, p_obj->p_file_)))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR,
                   "elems_written [%d] p_hdr->nFilledLen [%d]: "
                   "An error occurred while writing", elems_written,
                   p_hdr->nFilledLen);
          return OMX_ErrorInsufficientResources;
        }

      p_hdr->nFilledLen = 0;
      p_obj->counter_ += p_hdr->nFilledLen;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Writing data from HEADER [%p]...nFilledLen [%d] "
           "counter [%d] elems_written [%d]",
           p_hdr, p_hdr->nFilledLen, p_obj->counter_, elems_written);

  return OMX_ErrorNone;

}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
fw_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  fw_prc_t *p_obj = ap_obj;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;
  void *p_krn = tiz_get_krn (tiz_api_get_hdl (ap_obj));

  assert (ap_obj);

  if (!(p_obj->p_uri_param_))
    {
      p_obj->p_uri_param_ = tiz_mem_calloc
        (1, sizeof (OMX_PARAM_CONTENTURITYPE) + OMX_MAX_STRINGNAME_SIZE);

      if (NULL == p_obj->p_uri_param_)
        {
          TIZ_LOGN (TIZ_PRIORITY_ERROR, tiz_api_get_hdl (ap_obj),
                    "Error allocating memory "
                   "for the content uri struct");
          return OMX_ErrorInsufficientResources;
        }

      p_obj->p_uri_param_->nSize = sizeof (OMX_PARAM_CONTENTURITYPE)
        + OMX_MAX_STRINGNAME_SIZE - 1;
      p_obj->p_uri_param_->nVersion.nVersion = OMX_VERSION;
    }

  if (OMX_ErrorNone != (ret_val = tiz_api_GetParameter
                        (p_krn,
                         tiz_api_get_hdl (ap_obj),
                         OMX_IndexParamContentURI, p_obj->p_uri_param_)))
    {
      TIZ_LOGN (TIZ_PRIORITY_ERROR, tiz_api_get_hdl (ap_obj),
                "Error retrieving URI param from port");
      return ret_val;
    }

  TIZ_LOG (TIZ_PRIORITY_NOTICE, "Retrieved URI [%s]",
           p_obj->p_uri_param_->contentURI);

  if ((p_obj->p_file_
       = fopen ((const char *) p_obj->p_uri_param_->contentURI, "w")) == 0)
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "Error opening file from URI string");
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fw_proc_deallocate_resources (void *ap_obj)
{
  fw_prc_t *p_obj = ap_obj;
  assert (ap_obj);

  if (p_obj->p_file_)
    {
      fclose (p_obj->p_file_);
      p_obj->p_file_ = NULL;
    }

  tiz_mem_free (p_obj->p_uri_param_);
  p_obj->p_uri_param_ = NULL;

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fw_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  fw_prc_t *p_obj = ap_obj;
  assert (ap_obj);
  p_obj->counter_ = 0;
  p_obj->eos_ = false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fw_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  fw_prc_t *p_obj = ap_obj;
  assert (ap_obj);
  p_obj->counter_ = 0;
  p_obj->eos_ = false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fw_proc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
fw_proc_buffers_ready (const void *ap_obj)
{
  const fw_prc_t *p_obj = ap_obj;
  tiz_pd_set_t ports;
  void *p_krn = tiz_get_krn (tiz_api_get_hdl (ap_obj));
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  if (p_obj->eos_ == false)
    {
      TIZ_PD_ZERO (&ports);

      tiz_check_omx_err (tiz_krn_select (p_krn, 1, &ports));

      if (TIZ_PD_ISSET (0, &ports))
        {
          tiz_check_omx_err (tiz_krn_claim_buffer (p_krn, 0, 0, &p_hdr));
          TIZ_LOG (TIZ_PRIORITY_TRACE, "Claimed HEADER [%p]...", p_hdr);
          tiz_check_omx_err (fw_proc_write_buffer (ap_obj, p_hdr));
          if (p_hdr->nFlags & OMX_BUFFERFLAG_EOS)
            {
              TIZ_LOG (TIZ_PRIORITY_DEBUG,
                       "OMX_BUFFERFLAG_EOS in HEADER [%p]", p_hdr);
              tiz_srv_issue_event ((OMX_PTR) ap_obj,
                                      OMX_EventBufferFlag,
                                      0, p_hdr->nFlags, NULL);
            }
          tiz_check_omx_err (tiz_krn_release_buffer (p_krn, 0, p_hdr));
        }
    }

  return OMX_ErrorNone;
}

/*
 * initialization
 */

const void *fwprc;

OMX_ERRORTYPE
fw_prc_init (void)
{
  if (!fwprc)
    {
      tiz_check_omx_err_ret_oom (tiz_prc_init ());
      tiz_check_null_ret_oom
        (fwprc =
         factory_new
         (tizprc_class,
          "fwprc",
          tizprc,
          sizeof (fw_prc_t),
          ctor, fw_proc_ctor,
          dtor, fw_proc_dtor,
          tiz_prc_buffers_ready, fw_proc_buffers_ready,
          tiz_srv_allocate_resources, fw_proc_allocate_resources,
          tiz_srv_deallocate_resources, fw_proc_deallocate_resources,
          tiz_srv_prepare_to_transfer, fw_proc_prepare_to_transfer,
          tiz_srv_transfer_and_process, fw_proc_transfer_and_process,
          tiz_srv_stop_and_return, fw_proc_stop_and_return, 0));
    }
  return OMX_ErrorNone;
}
