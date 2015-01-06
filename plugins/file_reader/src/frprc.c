/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>

#include <OMX_Core.h>

#include <tizplatform.h>

#include <tizkernel.h>

#include "fr.h"
#include "frprc_decls.h"
#include "frprc.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.file_reader.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE fr_prc_deallocate_resources (void *);

static inline void close_file (fr_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  if (NULL != ap_prc->p_file_)
    {
      fclose (ap_prc->p_file_);
      ap_prc->p_file_ = NULL;
    }
}

static inline void delete_uri (fr_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  tiz_mem_free (ap_prc->p_uri_param_);
  ap_prc->p_uri_param_ = NULL;
}

static inline void reset_stream_parameters (fr_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  ap_prc->counter_ = 0;
  ap_prc->eos_ = false;
  if (NULL != ap_prc->p_file_)
    {
      rewind (ap_prc->p_file_);
    }
}

static OMX_ERRORTYPE obtain_uri (fr_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const long pathname_max = PATH_MAX + NAME_MAX;

  assert (NULL != ap_prc);
  assert (NULL == ap_prc->p_uri_param_);

  ap_prc->p_uri_param_
      = tiz_mem_calloc (1, sizeof(OMX_PARAM_CONTENTURITYPE) + pathname_max + 1);

  if (NULL == ap_prc->p_uri_param_)
    {
      TIZ_ERROR (handleOf (ap_prc),
                 "Error allocating memory for the content uri struct");
      rc = OMX_ErrorInsufficientResources;
    }
  else
    {
      ap_prc->p_uri_param_->nSize = sizeof(OMX_PARAM_CONTENTURITYPE)
                                    + pathname_max + 1;
      ap_prc->p_uri_param_->nVersion.nVersion = OMX_VERSION;

      if (OMX_ErrorNone
          != (rc = tiz_api_GetParameter (
                  tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                  OMX_IndexParamContentURI, ap_prc->p_uri_param_)))
        {
          TIZ_ERROR (handleOf (ap_prc),
                     "[%s] : Error retrieving the URI param from port",
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

static OMX_ERRORTYPE read_into_buffer (const void *ap_obj,
                                       OMX_BUFFERHEADERTYPE *p_hdr)
{
  fr_prc_t *p_prc = (fr_prc_t *)ap_obj;
  assert (NULL != p_prc);

  if (p_prc->p_file_ && !(p_prc->eos_))
    {
      int bytes_read = 0;
      if (!(bytes_read
            = fread (p_hdr->pBuffer, 1, p_hdr->nAllocLen, p_prc->p_file_)))
        {
          if (feof (p_prc->p_file_))
            {
              TIZ_NOTICE (
                  handleOf (p_prc),
                  "End of file reached bytes_read=[%d] EOS in HEADER [%p]",
                  bytes_read, p_hdr);
              p_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
              p_prc->eos_ = true;
            }
          else
            {
              TIZ_ERROR (handleOf (p_prc), "An error occurred while reading");
              return OMX_ErrorInsufficientResources;
            }
        }

      p_hdr->nFilledLen = bytes_read;
      p_prc->counter_ += p_hdr->nFilledLen;

      TIZ_TRACE (handleOf (p_prc),
                 "Reading into HEADER [%p]...nFilledLen[%d] "
                 "counter [%d] bytes_read[%d]",
                 p_hdr, p_hdr->nFilledLen, p_prc->counter_, bytes_read);
    }

  return OMX_ErrorNone;
}

/*
 * frprc
 */

static void *fr_prc_ctor (void *ap_obj, va_list *app)
{
  fr_prc_t *p_prc = super_ctor (typeOf (ap_obj, "frprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_file_ = NULL;
  p_prc->p_uri_param_ = NULL;
  reset_stream_parameters (p_prc);
  return p_prc;
}

static void *fr_prc_dtor (void *ap_obj)
{
  (void)fr_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "frprc"), ap_obj);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE fr_prc_allocate_resources (void *ap_obj,
                                                OMX_U32 TIZ_UNUSED (a_pid))
{
  fr_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  assert (NULL == p_prc->p_uri_param_);
  assert (NULL == p_prc->p_file_);

  tiz_check_omx_err (obtain_uri (p_prc));

  if ((p_prc->p_file_
       = fopen ((const char *)p_prc->p_uri_param_->contentURI, "r")) == 0)
    {
      TIZ_ERROR (handleOf (p_prc), "Error opening file from URI (%s)",
                 strerror (errno));
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE fr_prc_deallocate_resources (void *ap_obj)
{
  close_file (ap_obj);
  delete_uri (ap_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE fr_prc_prepare_to_transfer (void *ap_obj,
                                                 OMX_U32 TIZ_UNUSED (a_pid))
{
  reset_stream_parameters (ap_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE fr_prc_transfer_and_process (void *ap_obj,
                                                  OMX_U32 TIZ_UNUSED (a_pid))
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE fr_prc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE fr_prc_buffers_ready (const void *ap_obj)
{
  const fr_prc_t *p_prc = ap_obj;

  assert (NULL != ap_obj);

  if (!p_prc->eos_)
    {
      OMX_BUFFERHEADERTYPE *p_hdr = NULL;
      tiz_check_omx_err (tiz_krn_claim_buffer (tiz_get_krn (handleOf (p_prc)),
                                               ARATELIA_FILE_READER_PORT_INDEX,
                                               0, &p_hdr));
      if (NULL != p_hdr)
        {
          TIZ_TRACE (handleOf (p_prc), "Claimed HEADER [%p]...nFilledLen [%d]",
                     p_hdr, p_hdr->nFilledLen);
          p_hdr->nOffset = 0;
          p_hdr->nFilledLen = 0;
          tiz_check_omx_err (read_into_buffer (p_prc, p_hdr));
          tiz_check_omx_err (
              tiz_krn_release_buffer (tiz_get_krn (handleOf (p_prc)),
                                      ARATELIA_FILE_READER_PORT_INDEX, p_hdr));
        }
    }

  return OMX_ErrorNone;
}

/*
 * fr_prc_class
 */

static void *fr_prc_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "frprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *fr_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *frprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizprc), "frprc_class", classOf (tizprc),
       sizeof(fr_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, fr_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value */
       0);
  return frprc_class;
}

void *fr_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *frprc_class = tiz_get_type (ap_hdl, "frprc_class");
  TIZ_LOG_CLASS (frprc_class);
  void *frprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (frprc_class, "frprc", tizprc, sizeof(fr_prc_t),
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
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return frprc;
}
