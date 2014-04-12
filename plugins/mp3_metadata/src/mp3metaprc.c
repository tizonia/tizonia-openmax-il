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
 * @file   mp3metaprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Mp3 Metadata Eraser processor class
 *implementation
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

#include "mp3meta.h"
#include "mp3metaprc.h"
#include "mp3metaprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.mp3_metadata.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE mp3meta_prc_deallocate_resources (void *);

static inline void delete_uri (mp3meta_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  tiz_mem_free (ap_prc->p_uri_param_);
  ap_prc->p_uri_param_ = NULL;
}

static OMX_ERRORTYPE obtain_uri (mp3meta_prc_t *ap_prc)
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

static OMX_ERRORTYPE transform_buffer (mp3meta_prc_t *p_prc,
                                       OMX_BUFFERHEADERTYPE *p_out)
{
  int ret = MPG123_OK;

  assert (NULL != p_prc);
  assert (NULL != p_prc->p_mpg123_);
  assert (NULL != p_out);

  if (((ret = mpg123_framebyframe_next (p_prc->p_mpg123_)) == MPG123_OK
       || ret == MPG123_NEW_FORMAT))
    {
      unsigned long header;
      unsigned char *bodydata;
      size_t bodybytes;
      if (mpg123_framedata (p_prc->p_mpg123_, &header, &bodydata, &bodybytes)
          == MPG123_OK)
        {
          /* Need to extract the 4 header bytes from the native storage in the
           * correct order. */
          unsigned char hbuf[4];
          int i;
          for (i = 0; i < 4; ++i)
            {
              hbuf[i] = (unsigned char)((header >> ((3 - i) * 8)) & 0xff);
            }

          /* Now write out both header and data, fire and forget. */
          memcpy (p_out->pBuffer, hbuf, 4);
          memcpy (p_out->pBuffer + 4, bodydata, bodybytes);
          p_out->nFilledLen = 4 + bodybytes;
          TIZ_TRACE (handleOf (p_prc), "%zu: header 0x%08x, %zu body bytes",
                     ++p_prc->counter_, header, bodybytes);
          tiz_check_omx_err (tiz_krn_release_buffer (
              tiz_get_krn (handleOf (p_prc)),
              ARATELIA_MP3_METADATA_ERASER_OUTPUT_PORT_INDEX, p_out));
        }
    }

  if (ret == MPG123_DONE && NULL != p_out)
    {
      p_out->nFlags |= OMX_BUFFERFLAG_EOS;
      p_prc->eos_ = true;
      tiz_check_omx_err (tiz_krn_release_buffer (
          tiz_get_krn (handleOf (p_prc)),
          ARATELIA_MP3_METADATA_ERASER_OUTPUT_PORT_INDEX, p_out));
    }

  return OMX_ErrorNone;
}

/*
 * mp3metaprc
 */

static void *mp3meta_prc_ctor (void *ap_obj, va_list *app)
{
  mp3meta_prc_t *p_prc
      = super_ctor (typeOf (ap_obj, "mp3metaprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_mpg123_ = NULL;
  p_prc->p_uri_param_ = NULL;
  p_prc->counter_ = 0;
  p_prc->eos_ = false;
  p_prc->out_port_disabled_ = false;
  return p_prc;
}

static void *mp3meta_prc_dtor (void *ap_obj)
{
  (void)mp3meta_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "mp3metaprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE mp3meta_prc_allocate_resources (void *ap_obj,
                                                     OMX_U32 a_pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  mp3meta_prc_t *p_prc = ap_obj;
  int ret = 0;

  assert (NULL != p_prc);
  assert (NULL == p_prc->p_uri_param_);

  tiz_check_omx_err (obtain_uri (p_prc));

  assert (NULL != p_prc->p_uri_param_);

  if (MPG123_OK != mpg123_init ())
    {
      TIZ_ERROR (handleOf (p_prc), "[%s] : initialising libmpg123.",
                 tiz_err_to_str (rc));
      goto end;
    }

  if (NULL == (p_prc->p_mpg123_ = mpg123_new (NULL, &ret)))
    {
      TIZ_ERROR (handleOf (p_prc), "[%s] : creating the mpg123 handle.",
                 tiz_err_to_str (rc));
      goto end;
    }

  if (MPG123_OK != mpg123_param (p_prc->p_mpg123_, MPG123_REMOVE_FLAGS,
                                 MPG123_IGNORE_INFOFRAME, 0.))
    {
      TIZ_ERROR (handleOf (p_prc), "[%s] : setting mpg123 handle params.",
                 tiz_err_to_str (rc));
      goto end;
    }

  if (MPG123_OK != mpg123_open (p_prc->p_mpg123_,
                                (const char *)p_prc->p_uri_param_->contentURI))
    {
      TIZ_ERROR (handleOf (p_prc),
                 "[%s] : opening mpg123 from filesystem path (%s).",
                 tiz_err_to_str (rc), mpg123_strerror (p_prc->p_mpg123_));
      goto end;
    }

  /* Everything went well  */
  rc = OMX_ErrorNone;

end:

  if (OMX_ErrorInsufficientResources == rc)
    {
      mpg123_delete (p_prc->p_mpg123_); /* Closes, too. */
      p_prc->p_mpg123_ = NULL;
    }

  return rc;
}

static OMX_ERRORTYPE mp3meta_prc_deallocate_resources (void *ap_obj)
{
  mp3meta_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  delete_uri (ap_obj);
  mpg123_delete (p_prc->p_mpg123_); /* Closes, too. */
  p_prc->p_mpg123_ = NULL;
  mpg123_exit ();
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE mp3meta_prc_prepare_to_transfer (void *ap_obj,
                                                      OMX_U32 a_pid)
{
  mp3meta_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  p_prc->counter_ = 0;
  p_prc->eos_ = false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE mp3meta_prc_transfer_and_process (void *ap_obj,
                                                       OMX_U32 a_pid)
{
  mp3meta_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  p_prc->counter_ = 0;
  p_prc->eos_ = false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE mp3meta_prc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE mp3meta_prc_buffers_ready (const void *ap_obj)
{
  mp3meta_prc_t *p_prc = (mp3meta_prc_t *)ap_obj;

  assert (NULL != p_prc);

  if (!p_prc->eos_)
    {
      OMX_BUFFERHEADERTYPE *p_hdr = NULL;
      tiz_check_omx_err (tiz_krn_claim_buffer (
          tiz_get_krn (handleOf (p_prc)),
          ARATELIA_MP3_METADATA_ERASER_OUTPUT_PORT_INDEX, 0, &p_hdr));
      if (NULL != p_hdr)
        {
          TIZ_TRACE (handleOf (p_prc), "Claimed HEADER [%p]...nFilledLen [%d]",
                     p_hdr, p_hdr->nFilledLen);
          tiz_check_omx_err (transform_buffer (p_prc, p_hdr));
        }
    }

  return OMX_ErrorNone;
}

/*
 * mp3meta_prc_class
 */

static void *mp3meta_prc_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "mp3metaprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *mp3meta_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *mp3metaprc_class
      = factory_new (classOf (tizprc), "mp3metaprc_class", classOf (tizprc),
                     sizeof(mp3meta_prc_class_t), ap_tos, ap_hdl, ctor,
                     mp3meta_prc_class_ctor, 0);
  return mp3metaprc_class;
}

void *mp3meta_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *mp3metaprc_class = tiz_get_type (ap_hdl, "mp3metaprc_class");
  TIZ_LOG_CLASS (mp3metaprc_class);
  void *mp3metaprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (mp3metaprc_class, "mp3metaprc", tizprc, sizeof(mp3meta_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, mp3meta_prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, mp3meta_prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, mp3meta_prc_buffers_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, mp3meta_prc_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, mp3meta_prc_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, mp3meta_prc_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, mp3meta_prc_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, mp3meta_prc_stop_and_return,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);

  return mp3metaprc;
}
