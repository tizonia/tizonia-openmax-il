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
 * @file   sndfiledprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Sampled sound file decoder - processor class
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

#include "sndfiled.h"
#include "sndfiledprc.h"
#include "sndfiledprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.sndfile_decoder.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE sndfiled_prc_deallocate_resources (void *);

static OMX_ERRORTYPE allocate_temp_data_store (sndfiled_prc_t *ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;

  assert (NULL != ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_PCM_DECODER_INPUT_PORT_INDEX);
  tiz_check_omx_err (
      tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                            OMX_IndexParamPortDefinition, &port_def));

  assert (ap_prc->p_store_ == NULL);
  return tiz_buffer_init (&(ap_prc->p_store_), port_def.nBufferSize);
}

static inline void deallocate_temp_data_store (
    /*@special@ */ sndfiled_prc_t *ap_prc)
/*@releases ap_prc->p_store_@ */
/*@ensures isnull ap_prc->p_store_@ */
{
  assert (NULL != ap_prc);
  tiz_buffer_destroy (ap_prc->p_store_);
  ap_prc->p_store_ = NULL;
}

OMX_ERRORTYPE release_input_header (sndfiled_prc_t *ap_prc)
{
  OMX_BUFFERHEADERTYPE *p_in = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_PCM_DECODER_INPUT_PORT_INDEX);

  assert (NULL != ap_prc);

  if (p_in)
    {
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          /* Propagate EOS flag to output */
          TIZ_TRACE (handleOf (ap_prc), "Propagating EOS flag to output");
          OMX_BUFFERHEADERTYPE *p_out = tiz_filter_prc_get_header (
              ap_prc, ARATELIA_PCM_DECODER_OUTPUT_PORT_INDEX);
          if (p_out)
            {
              p_out->nFlags |= OMX_BUFFERFLAG_EOS;
            }
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          p_in->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
        }
      tiz_filter_prc_release_header (ap_prc,
                                     ARATELIA_PCM_DECODER_INPUT_PORT_INDEX);
    }
  return OMX_ErrorNone;
}

static bool store_data (sndfiled_prc_t *ap_prc)
{
  bool rc = true;

  if ((tiz_buffer_bytes_available (ap_prc->p_store_) - ap_prc->store_offset_)
      < ARATELIA_PCM_DECODER_PORT_MIN_INPUT_BUF_SIZE)
    {
      OMX_BUFFERHEADERTYPE *p_in = tiz_filter_prc_get_header (
          ap_prc, ARATELIA_PCM_DECODER_INPUT_PORT_INDEX);

      assert (NULL != ap_prc);

      if (p_in)
        {
          TIZ_TRACE (handleOf (ap_prc), "store available [%d]",
                     tiz_buffer_bytes_available (ap_prc->p_store_));
          if (tiz_buffer_store_data (ap_prc->p_store_,
                                     p_in->pBuffer + p_in->nOffset,
                                     p_in->nFilledLen) < p_in->nFilledLen)
            {
              TIZ_ERROR (handleOf (ap_prc),
                         "[%s] : Unable to store all the data.",
                         tiz_err_to_str (rc));
              rc = false;
            }
          release_input_header (ap_prc);
        }
    }
  return rc;
}

static sf_count_t sf_io_get_filelen (void *user_data)
{
  TIZ_TRACE (handleOf (user_data), "");
  return -1;
}

static sf_count_t sf_io_seek (sf_count_t offset, int whence, void *user_data)
{
  TIZ_TRACE (handleOf (user_data), "");
  return -1;
}

static sf_count_t sf_io_read (void *ap_ptr, sf_count_t count, void *user_data)
{
  sndfiled_prc_t *p_prc = (sndfiled_prc_t *)user_data;
  sf_count_t bytes_read = 0;

  assert (NULL != ap_ptr);
  assert (NULL != p_prc);

  (void)store_data (p_prc);

  TIZ_TRACE (
      handleOf (p_prc), "decoder_inited_ [%s] store bytes [%d] offset [%d]",
      (p_prc->decoder_inited_ ? "YES" : "NO"),
      tiz_buffer_bytes_available (p_prc->p_store_), p_prc->store_offset_);

  if (tiz_buffer_bytes_available (p_prc->p_store_) > 0)
    {
      bytes_read = MIN (count, tiz_buffer_bytes_available (p_prc->p_store_)
                               - p_prc->store_offset_);
      memcpy (ap_ptr,
              tiz_buffer_get_data (p_prc->p_store_) + p_prc->store_offset_,
              bytes_read);
      if (p_prc->decoder_inited_)
        {
          tiz_buffer_advance (p_prc->p_store_, bytes_read);
        }
      else
        {
          p_prc->store_offset_ += bytes_read;
        }
    }
  else
    {
      TIZ_TRACE (handleOf (p_prc), "Run out of compressed data");
    }
  return bytes_read;
}

static sf_count_t sf_io_write (const void *ptr, sf_count_t count,
                               void *user_data)
{
  TIZ_TRACE (handleOf (user_data), "");
  return -1;
}

static sf_count_t sf_io_tell (void *user_data)
{
  TIZ_TRACE (handleOf (user_data), "");
  return -1;
}

static OMX_ERRORTYPE open_sf (sndfiled_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  if (store_data (ap_prc))
    {
      assert (NULL != ap_prc);
      if (!ap_prc->p_sf_)
    {
      ap_prc->p_sf_ = sf_open_virtual (&(ap_prc->sf_io_), SFM_READ,
                                       &(ap_prc->sf_info_), ap_prc);
      if (!ap_prc->p_sf_)
        {
          TIZ_ERROR (handleOf (ap_prc),
                     "Unable to open the sf handle");
          ap_prc->store_offset_ = 0;
        }
      else
      {
        SF_INFO *p = &(ap_prc->sf_info_);
        TIZ_TRACE (handleOf (ap_prc),
                   "decoder_inited = TRUE - store_offset [%d]",
                   ap_prc->store_offset_);

        ap_prc->decoder_inited_ = true;
        tiz_buffer_advance (ap_prc->p_store_, ap_prc->store_offset_);
        ap_prc->store_offset_ = 0;

        TIZ_TRACE (handleOf (ap_prc), "frames [%d]", p->frames);
        TIZ_TRACE (handleOf (ap_prc), "samplerate [%d]", p->samplerate);
        TIZ_TRACE (handleOf (ap_prc), "channels [%d]", p->channels);
        TIZ_TRACE (handleOf (ap_prc), "format [%d]", p->format);
        TIZ_TRACE (handleOf (ap_prc), "sections [%d]", p->sections);
        TIZ_TRACE (handleOf (ap_prc), "seekable [%d]", p->seekable);
      }
    }
    }
  else
    {
      rc = OMX_ErrorInsufficientResources;
    }

  return rc;
}

static OMX_ERRORTYPE transform_buffer (sndfiled_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_out = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_PCM_DECODER_OUTPUT_PORT_INDEX);

  if (!store_data (ap_prc))
    {
      TIZ_ERROR (handleOf (ap_prc),
                 "[OMX_ErrorInsufficientResources] : "
                 "Could not store all the incoming data");
      return OMX_ErrorInsufficientResources;
    }

  if (tiz_buffer_bytes_available (ap_prc->p_store_) == 0 || NULL == p_out)
    {
      TIZ_TRACE (handleOf (ap_prc), "store bytes [%d] OUT HEADER [%p]",
                 tiz_buffer_bytes_available (ap_prc->p_store_), p_out);

      /* Propagate the EOS flag to the next component */
      if (tiz_buffer_bytes_available (ap_prc->p_store_) == 0 && NULL != p_out
          && tiz_filter_prc_is_eos (ap_prc))
        {
          p_out->nFlags |= OMX_BUFFERFLAG_EOS;
          tiz_filter_prc_release_header (
              ap_prc, ARATELIA_PCM_DECODER_OUTPUT_PORT_INDEX);
          tiz_filter_prc_update_eos_flag (ap_prc, false);
        }
      return OMX_ErrorNotReady;
    }

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_sf_);

  {
    size_t frame_size = sizeof(int) * ap_prc->sf_info_.channels;
    sf_count_t read_frames = p_out->nAllocLen / frame_size;
    sf_count_t num_frames = sf_readf_int (
        ap_prc->p_sf_, (int *)(p_out->pBuffer + p_out->nOffset), read_frames);

    if (num_frames > 0)
      {
        p_out->nFilledLen = num_frames * frame_size;
        (void)tiz_filter_prc_release_header (
            ap_prc, ARATELIA_PCM_DECODER_OUTPUT_PORT_INDEX);
      }
    else
      {
        /* TODO */
      }
  }

  return rc;
}

static void reset_stream_parameters (sndfiled_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  ap_prc->decoder_inited_ = false;
  tiz_buffer_clear (ap_prc->p_store_);
  ap_prc->store_offset_ = 0;
  tiz_filter_prc_update_eos_flag (ap_prc, false);
}

/*
 * sndfiledprc
 */

static void *sndfiled_prc_ctor (void *ap_obj, va_list *app)
{
  sndfiled_prc_t *p_prc
      = super_ctor (typeOf (ap_obj, "sndfiledprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_sf_ = NULL;
  p_prc->sf_info_.format = 0;
  p_prc->sf_io_.get_filelen = sf_io_get_filelen;
  p_prc->sf_io_.seek = sf_io_seek;
  p_prc->sf_io_.read = sf_io_read;
  p_prc->sf_io_.write = sf_io_write;
  p_prc->sf_io_.tell = sf_io_tell;
  reset_stream_parameters (p_prc);
  return p_prc;
}

static void *sndfiled_prc_dtor (void *ap_obj)
{
  (void)sndfiled_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "sndfiledprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE sndfiled_prc_allocate_resources (void *ap_obj,
                                                      OMX_U32 a_pid)
{
  return allocate_temp_data_store (ap_obj);
}

static OMX_ERRORTYPE sndfiled_prc_deallocate_resources (void *ap_obj)
{
  sndfiled_prc_t *p_prc = ap_obj;
  deallocate_temp_data_store (p_prc);
  assert (NULL != p_prc);
  sf_close (p_prc->p_sf_);
  p_prc->p_sf_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE sndfiled_prc_prepare_to_transfer (void *ap_obj,
                                                       OMX_U32 a_pid)
{
  reset_stream_parameters (ap_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE sndfiled_prc_transfer_and_process (void *ap_obj,
                                                        OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE sndfiled_prc_stop_and_return (void *ap_obj)
{
  return tiz_filter_prc_release_all_headers (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE sndfiled_prc_buffers_ready (const void *ap_prc)
{
  sndfiled_prc_t *p_prc = (sndfiled_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_prc);

  if (!p_prc->decoder_inited_)
    {
      rc = open_sf (p_prc);
    }

  if (p_prc->decoder_inited_ && OMX_ErrorNone == rc)
    {
      while (OMX_ErrorNone == rc)
        {
          rc = transform_buffer (p_prc);
        }
      if (OMX_ErrorNotReady == rc)
        {
          rc = OMX_ErrorNone;
        }
    }

  return rc;
}

static OMX_ERRORTYPE sndfiled_prc_port_enable (const void *ap_prc,
                                               OMX_U32 a_pid)
{
  sndfiled_prc_t *p_prc = (sndfiled_prc_t *)ap_prc;
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, false);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE sndfiled_prc_port_disable (const void *ap_prc,
                                                OMX_U32 a_pid)
{
  sndfiled_prc_t *p_prc = (sndfiled_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  if (OMX_ALL == a_pid)
    {
      rc = tiz_filter_prc_release_all_headers (p_prc);
    }
  else
    {
      rc = tiz_filter_prc_release_header (p_prc, a_pid);
    }
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, true);
  return rc;
}

/*
 * sndfiled_prc_class
 */

static void *sndfiled_prc_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "sndfiledprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *sndfiled_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void *sndfiledprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizfilterprc), "sndfiledprc_class", classOf (tizfilterprc),
       sizeof(sndfiled_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, sndfiled_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return sndfiledprc_class;
}

void *sndfiled_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void *sndfiledprc_class = tiz_get_type (ap_hdl, "sndfiledprc_class");
  TIZ_LOG_CLASS (sndfiledprc_class);
  void *sndfiledprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (sndfiledprc_class, "sndfiledprc", tizfilterprc, sizeof(sndfiled_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, sndfiled_prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, sndfiled_prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, sndfiled_prc_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, sndfiled_prc_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, sndfiled_prc_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, sndfiled_prc_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, sndfiled_prc_stop_and_return,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, sndfiled_prc_buffers_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_enable, sndfiled_prc_port_enable,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_disable, sndfiled_prc_port_disable,
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return sndfiledprc;
}
