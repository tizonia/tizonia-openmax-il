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
 * @file   fwprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Binary file writer processor
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>

#include <OMX_Core.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "fw.h"
#include "fwprc_decls.h"
#include "fwprc.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.file_writer.prc"
#endif

static OMX_ERRORTYPE
obtain_uri (fw_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const long pathname_max = PATH_MAX + NAME_MAX;

  assert (ap_prc);
  assert (NULL == ap_prc->p_uri_param_);

  ap_prc->p_uri_param_ = tiz_mem_calloc
    (1, sizeof (OMX_PARAM_CONTENTURITYPE) + pathname_max + 1);

  if (NULL == ap_prc->p_uri_param_)
    {
      TIZ_ERROR (handleOf (ap_prc),
                "Error allocating memory for the content uri struct");
      rc = OMX_ErrorInsufficientResources;
    }
  else
    {
      ap_prc->p_uri_param_->nSize = sizeof (OMX_PARAM_CONTENTURITYPE)
        + pathname_max + 1;
      ap_prc->p_uri_param_->nVersion.nVersion = OMX_VERSION;

      if (OMX_ErrorNone != (rc = tiz_api_GetParameter
                            (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                             OMX_IndexParamContentURI, ap_prc->p_uri_param_)))
        {
          TIZ_ERROR (handleOf (ap_prc),
                     "[%s] : Error retrieving URI param from port",
                     tiz_err_to_str (rc));
        }
      else
        {
          TIZ_NOTICE (handleOf (ap_prc), "URI [%s]",
                      ap_prc->p_uri_param_->contentURI);
        }
    }

  return rc;
}

/*
 * fwprc
 */

static void *
fw_proc_ctor (void *ap_obj, va_list * app)
{
  fw_prc_t *p_prc     = super_ctor (typeOf (ap_obj, "fwprc"), ap_obj, app);
  assert (p_prc);
  p_prc->p_file_      = NULL;
  p_prc->p_uri_param_ = NULL;
  p_prc->counter_     = 0;
  p_prc->eos_         = false;
  return p_prc;
}

static void *
fw_proc_dtor (void *ap_obj)
{
  fw_prc_t *p_prc = ap_obj;
  assert (p_prc);

  if (p_prc->p_file_)
    {
      fclose (p_prc->p_file_);
    }

  if (p_prc->p_uri_param_)
    {
      tiz_mem_free (p_prc->p_uri_param_);
    }

  return super_dtor (typeOf (ap_obj, "fwprc"), ap_obj);
}

static OMX_ERRORTYPE
fw_proc_write_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
{
  fw_prc_t *p_prc = (fw_prc_t *) ap_obj;
  assert (p_prc);

  if (p_prc->p_file_ && !(p_prc->eos_) && p_hdr->nFilledLen > 0)
    {
      int elems_written = 0;
      if (1 != (elems_written
                = fwrite (p_hdr->pBuffer + p_hdr->nOffset,
                          p_hdr->nFilledLen, 1, p_prc->p_file_)))
        {
          TIZ_ERROR (handleOf (p_prc),
                   "elems_written [%d] p_hdr->nFilledLen [%d]: "
                   "An error occurred while writing", elems_written,
                   p_hdr->nFilledLen);
          return OMX_ErrorInsufficientResources;
        }

      p_hdr->nFilledLen = 0;
      p_prc->counter_ += p_hdr->nFilledLen;

      TIZ_TRACE (handleOf (p_prc), "Writing data from HEADER [%p]...nFilledLen [%d] "
                 "counter [%d] elems_written [%d]",
                 p_hdr, p_hdr->nFilledLen, p_prc->counter_, elems_written);
    }

  return OMX_ErrorNone;
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
fw_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  fw_prc_t *p_prc = ap_obj;
  assert (ap_obj);

  tiz_check_omx_err (obtain_uri (p_prc));

  if ((p_prc->p_file_
       = fopen ((const char *) p_prc->p_uri_param_->contentURI, "w")) == 0)
    {
      TIZ_ERROR (handleOf (p_prc),
                "Error opening file from URI (%s)", strerror (errno));
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fw_proc_deallocate_resources (void *ap_obj)
{
  fw_prc_t *p_prc = ap_obj;
  assert (ap_obj);

  if (p_prc->p_file_)
    {
      fclose (p_prc->p_file_);
      p_prc->p_file_ = NULL;
    }

  tiz_mem_free (p_prc->p_uri_param_);
  p_prc->p_uri_param_ = NULL;

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fw_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  fw_prc_t *p_prc = ap_obj;
  assert (ap_obj);
  p_prc->counter_ = 0;
  p_prc->eos_ = false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fw_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  fw_prc_t *p_prc = ap_obj;
  assert (ap_obj);
  p_prc->counter_ = 0;
  p_prc->eos_ = false;
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
  const fw_prc_t *p_prc = ap_obj;

  if (!p_prc->eos_)
    {
      OMX_BUFFERHEADERTYPE *p_hdr = NULL;
      tiz_check_omx_err (tiz_krn_claim_buffer (tiz_get_krn (handleOf (p_prc)),
                                               ARATELIA_FILE_WRITER_PORT_INDEX,
                                               0, &p_hdr));
      if (p_hdr)
        {
          TIZ_TRACE (handleOf (p_prc), "Claimed HEADER [%p]...", p_hdr);
          tiz_check_omx_err (fw_proc_write_buffer (p_prc, p_hdr));
          if (p_hdr->nFlags & OMX_BUFFERFLAG_EOS)
            {
              TIZ_DEBUG (handleOf (p_prc),
                         "OMX_BUFFERFLAG_EOS in HEADER [%p]", p_hdr);
              tiz_srv_issue_event ((OMX_PTR) p_prc,
                                   OMX_EventBufferFlag,
                                   ARATELIA_FILE_WRITER_PORT_INDEX,
                                   p_hdr->nFlags, NULL);
            }
          tiz_check_omx_err (tiz_krn_release_buffer (tiz_get_krn (handleOf (p_prc)),
                                                     ARATELIA_FILE_WRITER_PORT_INDEX,
                                                     p_hdr));
        }
    }

  return OMX_ErrorNone;
}

/*
 * fw_prc_class
 */

static void *
fw_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "fwprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
fw_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * fwprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "fwprc_class", classOf (tizprc), sizeof (fw_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, fw_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value */
     0);
  return fwprc_class;
}

void *
fw_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * fwprc_class = tiz_get_type (ap_hdl, "fwprc_class");
  TIZ_LOG_CLASS (fwprc_class);
  void * fwprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (fwprc_class, "fwprc", tizprc, sizeof (fw_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, fw_proc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, fw_proc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, fw_proc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, fw_proc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, fw_proc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, fw_proc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, fw_proc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, fw_proc_buffers_ready,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return fwprc;
}
