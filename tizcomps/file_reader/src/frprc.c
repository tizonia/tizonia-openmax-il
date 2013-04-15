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
 * @file   frprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Binary Reader processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "OMX_Core.h"
#include "frprc.h"
#include "frprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizosal.h"

#include <assert.h>
#include <stdio.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.file_reader.prc"
#endif

/*
 * frprc
 */

static void *
fr_proc_ctor (void *ap_obj, va_list * app)
{
  fr_prc_t *p_obj = super_ctor (frprc, ap_obj, app);
  p_obj->p_file_ = NULL;
  p_obj->p_uri_param_ = NULL;
  p_obj->counter_ = 0;
  p_obj->eos_ = false;
  return p_obj;
}

static void *
fr_proc_dtor (void *ap_obj)
{
  fr_prc_t *p_obj = ap_obj;

  if (p_obj->p_file_)
    {
      fclose (p_obj->p_file_);
    }

  if (p_obj->p_uri_param_)
    {
      tiz_mem_free (p_obj->p_uri_param_);
    }

  return super_dtor (frprc, ap_obj);
}

static OMX_ERRORTYPE
fr_proc_read_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
{
  fr_prc_t *p_obj = (fr_prc_t *) ap_obj;
  int bytes_read = 0;

  if (p_obj->p_file_ && !(p_obj->eos_))
    {
      if (!(bytes_read
            = fread (p_hdr->pBuffer, 1, p_hdr->nAllocLen, p_obj->p_file_)))
        {
          if (feof (p_obj->p_file_))
            {
              TIZ_LOG (TIZ_NOTICE,
                       "End of file reached bytes_read=[%d]", bytes_read);
              p_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
              p_obj->eos_ = true;
            }
          else
            {
              TIZ_LOG (TIZ_ERROR, "An error occurred while reading");
              return OMX_ErrorInsufficientResources;
            }
        }

      p_hdr->nFilledLen = bytes_read;
      p_obj->counter_ += p_hdr->nFilledLen;
    }

  TIZ_LOG (TIZ_TRACE, "Reading into HEADER [%p]...nFilledLen[%d] "
           "counter [%d] bytes_read[%d]",
           p_hdr, p_hdr->nFilledLen, p_obj->counter_, bytes_read);

  return OMX_ErrorNone;

}

/*
 * from tiz_servant class
 */

static OMX_ERRORTYPE
fr_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  fr_prc_t *p_obj = ap_obj;
  const tiz_servant_t *p_parent = ap_obj;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;
  void *p_krn = tiz_get_krn (p_parent->p_hdl_);

  assert (ap_obj);

  if (!(p_obj->p_uri_param_))
    {
      p_obj->p_uri_param_ = tiz_mem_calloc
        (1, sizeof (OMX_PARAM_CONTENTURITYPE) + OMX_MAX_STRINGNAME_SIZE);

      if (NULL == p_obj->p_uri_param_)
        {
          TIZ_LOG (TIZ_ERROR, "Error allocating memory "
                   "for the content uri struct");
          return OMX_ErrorInsufficientResources;
        }

      p_obj->p_uri_param_->nSize = sizeof (OMX_PARAM_CONTENTURITYPE)
        + OMX_MAX_STRINGNAME_SIZE - 1;
      p_obj->p_uri_param_->nVersion.nVersion = OMX_VERSION;
    }

  if (OMX_ErrorNone != (ret_val = tiz_api_GetParameter
                        (p_krn,
                         p_parent->p_hdl_,
                         OMX_IndexParamContentURI, p_obj->p_uri_param_)))
    {
      TIZ_LOG (TIZ_ERROR, "Error retrieving URI param from port");
      return ret_val;
    }

  TIZ_LOG (TIZ_NOTICE, "Retrieved URI [%s]",
           p_obj->p_uri_param_->contentURI);

  if ((p_obj->p_file_
       = fopen ((const char *) p_obj->p_uri_param_->contentURI, "r")) == 0)
    {
      TIZ_LOG (TIZ_ERROR, "Error opening file from  URI string");
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_proc_deallocate_resources (void *ap_obj)
{
  fr_prc_t *p_obj = ap_obj;
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
fr_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  fr_prc_t *p_obj = ap_obj;
  assert (ap_obj);
  p_obj->counter_ = 0;
  p_obj->eos_ = false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  fr_prc_t *p_obj = ap_obj;
  assert (ap_obj);
  p_obj->counter_ = 0;
  p_obj->eos_ = false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_proc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tiz_proc class
 */

static OMX_ERRORTYPE
fr_proc_buffers_ready (const void *ap_obj)
{
  const fr_prc_t *p_obj = ap_obj;
  const tiz_servant_t *p_parent = ap_obj;
  tiz_pd_set_t ports;
  void *p_krn = tiz_get_krn (p_parent->p_hdl_);
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  if (p_obj->eos_ == false)
    {
      TIZ_PD_ZERO (&ports);

      tiz_check_omx_err (tiz_kernel_select (p_krn, 1, &ports));

      if (TIZ_PD_ISSET (0, &ports))
        {
          tiz_check_omx_err (tiz_kernel_claim_buffer (p_krn, 0, 0, &p_hdr));
          TIZ_LOG (TIZ_TRACE, "Claimed HEADER [%p]...", p_hdr);
          tiz_check_omx_err (fr_proc_read_buffer (ap_obj, p_hdr));
          tiz_kernel_relinquish_buffer (p_krn, 0, p_hdr);
        }
    }

  return OMX_ErrorNone;
}

/*
 * initialization
 */

const void *frprc;

void
fr_prc_init (void)
{
  if (!frprc)
    {
      tiz_proc_init ();
      frprc =
        factory_new
        (tizproc_class,
         "frprc",
         tizproc,
         sizeof (fr_prc_t),
         ctor, fr_proc_ctor,
         dtor, fr_proc_dtor,
         tiz_proc_buffers_ready, fr_proc_buffers_ready,
         tiz_servant_allocate_resources, fr_proc_allocate_resources,
         tiz_servant_deallocate_resources, fr_proc_deallocate_resources,
         tiz_servant_prepare_to_transfer, fr_proc_prepare_to_transfer,
         tiz_servant_transfer_and_process, fr_proc_transfer_and_process,
         tiz_servant_stop_and_return, fr_proc_stop_and_return, 0);
    }
}
