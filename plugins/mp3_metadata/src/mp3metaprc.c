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

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

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

static OMX_ERRORTYPE release_out_buffer (mp3meta_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  if (NULL != ap_prc->p_out_hdr_)
    {
      tiz_check_omx_err (tiz_krn_release_buffer (
          tiz_get_krn (handleOf (ap_prc)),
          ARATELIA_MP3_METADATA_ERASER_PORT_INDEX, ap_prc->p_out_hdr_));
      ap_prc->p_out_hdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static inline OMX_BUFFERHEADERTYPE **get_header_ptr (mp3meta_prc_t *ap_prc)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  assert (NULL != ap_prc);
  pp_hdr = &(ap_prc->p_out_hdr_);
  assert (NULL != pp_hdr);
  return pp_hdr;
}

static OMX_BUFFERHEADERTYPE *get_header (mp3meta_prc_t *ap_prc)
{
  OMX_BUFFERHEADERTYPE *p_hdr = *(get_header_ptr (ap_prc));

  if (NULL == p_hdr)
    {
      if (OMX_ErrorNone
          == tiz_krn_claim_buffer (
                 tiz_get_krn (handleOf (ap_prc)),
                 ARATELIA_MP3_METADATA_ERASER_PORT_INDEX, 0, &p_hdr))
        {
          if (NULL != p_hdr)
            {
              TIZ_TRACE (handleOf (ap_prc),
                         "Claimed HEADER [%p] nFilledLen [%d]", p_hdr,
                         p_hdr->nFilledLen);
              ap_prc->p_out_hdr_ = p_hdr;
            }
        }
    }

  return p_hdr;
}

static inline bool buffers_available (mp3meta_prc_t *ap_prc)
{
  return (NULL != get_header (ap_prc));
}

static OMX_ERRORTYPE remove_metadata (mp3meta_prc_t *ap_prc)
{
  int ret = MPG123_OK;
  OMX_BUFFERHEADERTYPE *p_out = NULL;

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_mpg123_);

  p_out = get_header (ap_prc);
  assert (NULL != p_out);
  p_out->nFilledLen = 0;
  p_out->nFlags = 0;

  if (((ret = mpg123_framebyframe_next (ap_prc->p_mpg123_)) == MPG123_OK
       || MPG123_NEW_FORMAT == ret))
    {
      unsigned long header;
      unsigned char *bodydata;
      size_t bodybytes;
      if (mpg123_framedata (ap_prc->p_mpg123_, &header, &bodydata, &bodybytes)
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
          TIZ_TRACE (handleOf (ap_prc), "%zu: header 0x%08x, %zu body bytes",
                     ++ap_prc->counter_, header, bodybytes);
        }
    }
  else if (MPG123_DONE == ret)
    {
      TIZ_NOTICE (handleOf (ap_prc), "HEADER [%p] Adding OMX_BUFFERFLAG_EOS",
                  p_out);
      p_out->nFlags |= OMX_BUFFERFLAG_EOS;
      ap_prc->eos_ = true;
    }
  else if (MPG123_NEED_MORE == ret)
    {
      TIZ_WARN (handleOf (ap_prc), "ret=[MPG123_NEED_MORE] HEADER [%p]", p_out);
    }
  else
    {
      TIZ_ERROR (handleOf (ap_prc), "ret=[%d] HEADER [%p]", ret, p_out);
    }

  if (p_out->nFilledLen > 0 || ap_prc->eos_)
    {
      tiz_check_omx_err (release_out_buffer (ap_prc));
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
  p_prc->p_out_hdr_ = NULL;
  p_prc->p_uri_param_ = NULL;
  p_prc->counter_ = 0;
  p_prc->eos_ = false;
  p_prc->out_port_disabled_ = false;

  if (MPG123_OK != mpg123_init ())
    {
      TIZ_ERROR (handleOf (p_prc), "[%s] : initialising libmpg123.",
                 tiz_err_to_str (OMX_ErrorInsufficientResources));
    }

  return p_prc;
}

static void *mp3meta_prc_dtor (void *ap_obj)
{
  (void)mp3meta_prc_deallocate_resources (ap_obj);
  mpg123_exit ();
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

  if (NULL == (p_prc->p_mpg123_ = mpg123_new (NULL, &ret)))
    {
      TIZ_ERROR (handleOf (p_prc), "[%s] : creating the mpg123 handle.",
                 tiz_err_to_str (rc));
      goto end;
    }

  if (MPG123_OK != mpg123_param (p_prc->p_mpg123_, MPG123_ADD_FLAGS,
                                 MPG123_IGNORE_INFOFRAME | MPG123_SKIP_ID3V2, 0.))
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
  return release_out_buffer (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE mp3meta_prc_buffers_ready (const void *ap_obj)
{
  mp3meta_prc_t *p_prc = (mp3meta_prc_t *)ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_prc);

  while (buffers_available (p_prc) && OMX_ErrorNone == rc && !p_prc->eos_
         && !p_prc->out_port_disabled_)
    {
      rc = remove_metadata (p_prc);
    }

  return rc;
}

static OMX_ERRORTYPE mp3meta_prc_port_enable (const void *ap_prc, OMX_U32 a_pid)
{
  mp3meta_prc_t *p_prc = (mp3meta_prc_t *)ap_prc;
  assert (NULL != p_prc);
  assert (ARATELIA_MP3_METADATA_ERASER_PORT_INDEX == a_pid || OMX_ALL
                                                                     == a_pid);
  p_prc->out_port_disabled_ = false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE mp3meta_prc_port_disable (const void *ap_prc,
                                               OMX_U32 a_pid)
{
  mp3meta_prc_t *p_prc = (mp3meta_prc_t *)ap_prc;
  assert (NULL != p_prc);
  assert (ARATELIA_MP3_METADATA_ERASER_PORT_INDEX == a_pid || OMX_ALL
                                                                     == a_pid);
  p_prc->out_port_disabled_ = true;
  release_out_buffer (p_prc);
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
    = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "mp3metaprc_class", classOf (tizprc), sizeof(mp3meta_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl, ctor,
     /* TIZ_CLASS_COMMENT: class constructor */
     mp3meta_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
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
     tiz_srv_allocate_resources, mp3meta_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, mp3meta_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, mp3meta_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, mp3meta_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, mp3meta_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, mp3meta_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, mp3meta_prc_port_enable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, mp3meta_prc_port_disable,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return mp3metaprc;
}
