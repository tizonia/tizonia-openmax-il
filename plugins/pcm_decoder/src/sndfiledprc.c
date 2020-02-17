/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @brief  Tizonia - Sampled sound file decoder - processor class
 *implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
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
static OMX_ERRORTYPE
sndfiled_prc_deallocate_resources (void *);

static inline OMX_BUFFERHEADERTYPE *
get_in_hdr (sndfiled_prc_t * ap_prc)
{
  return tiz_filter_prc_get_header (ap_prc,
                                    ARATELIA_PCM_DECODER_INPUT_PORT_INDEX);
}

static inline OMX_BUFFERHEADERTYPE *
get_out_hdr (sndfiled_prc_t * ap_prc)
{
  return tiz_filter_prc_get_header (ap_prc,
                                    ARATELIA_PCM_DECODER_OUTPUT_PORT_INDEX);
}

OMX_ERRORTYPE
release_in_hdr (sndfiled_prc_t * ap_prc)
{
  OMX_BUFFERHEADERTYPE * p_in = get_in_hdr (ap_prc);
  assert (ap_prc);
  if (p_in)
    {
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          TIZ_TRACE (handleOf (ap_prc), "EOS flag received");
          /* Remember the EOS flag */
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          tiz_util_reset_eos_flag (p_in);
        }
      TIZ_TRACE (handleOf (ap_prc), "Releasing IN HEADER [%p]", p_in);
      tiz_filter_prc_release_header (ap_prc,
                                     ARATELIA_PCM_DECODER_INPUT_PORT_INDEX);
    }
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
release_out_hdr (sndfiled_prc_t * ap_prc)
{
  OMX_BUFFERHEADERTYPE * p_out = tiz_filter_prc_get_header (
    ap_prc, ARATELIA_PCM_DECODER_OUTPUT_PORT_INDEX);
  assert (ap_prc);
  if (p_out)
    {
      if (tiz_filter_prc_is_eos (ap_prc))
        {
          TIZ_TRACE (handleOf (ap_prc), "Propagating EOS flag");
          tiz_util_set_eos_flag (p_out);
        }
      TIZ_TRACE (handleOf (ap_prc),
                 "Releasing OUT HEADER [%p] nFilledLen [%d] nAllocLen [%d]",
                 p_out, p_out->nFilledLen, p_out->nAllocLen);
      tiz_filter_prc_release_header (ap_prc,
                                     ARATELIA_PCM_DECODER_OUTPUT_PORT_INDEX);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
allocate_temp_data_store (sndfiled_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);
  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_PCM_DECODER_INPUT_PORT_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));
  assert (ap_prc->p_store_ == NULL);
  return tiz_buffer_init (&(ap_prc->p_store_), port_def.nBufferSize);
}

static inline void
deallocate_temp_data_store (
  /*@special@ */ sndfiled_prc_t * ap_prc)
/*@releases ap_prc->p_store_@ */
/*@ensures isnull ap_prc->p_store_@ */
{
  assert (ap_prc);
  tiz_buffer_destroy (ap_prc->p_store_);
  ap_prc->p_store_ = NULL;
}

static bool
store_data (sndfiled_prc_t * ap_prc)
{
  bool rc = true;
  assert (ap_prc);

  if ((tiz_buffer_available (ap_prc->p_store_) - ap_prc->store_offset_)
      < ARATELIA_PCM_DECODER_PORT_MIN_INPUT_BUF_SIZE * 2)
    {
      OMX_BUFFERHEADERTYPE * p_in = tiz_filter_prc_get_header (
        ap_prc, ARATELIA_PCM_DECODER_INPUT_PORT_INDEX);

      assert (ap_prc);

      if (p_in)
        {
          if (tiz_buffer_push (ap_prc->p_store_, p_in->pBuffer + p_in->nOffset,
                               p_in->nFilledLen)
              == p_in->nFilledLen)
            {
              TIZ_TRACE (handleOf (ap_prc), "store bytes [%d]",
                         tiz_buffer_available (ap_prc->p_store_));
              release_in_hdr (ap_prc);
            }
          else
            {
              TIZ_ERROR (handleOf (ap_prc),
                         "[%s] : Unable to store all the data.",
                         tiz_err_to_str (rc));
              rc = false;
            }
        }
    }
  return rc;
}

static sf_count_t
sf_io_get_filelen (void * user_data)
{
  /* We don't know the size of the stream */
  return SF_COUNT_MAX;
}

static sf_count_t
sf_io_seek (sf_count_t offset, int whence, void * user_data)
{
  /* The stream is not seekable */
  return EBADF;
}

static sf_count_t
sf_io_read (void * ap_ptr, sf_count_t count, void * user_data)
{
  sndfiled_prc_t * p_prc = (sndfiled_prc_t *) user_data;
  sf_count_t bytes_read = 0;

  assert (ap_ptr);
  assert (p_prc);

  if (!tiz_filter_prc_is_eos (p_prc) && store_data (p_prc)
      && tiz_buffer_available (p_prc->p_store_) > 0)
    {
      TIZ_TRACE (handleOf (p_prc),
                 "count [%d] decoder_inited_ [%s] store bytes [%d] offset [%d]",
                 count, (p_prc->decoder_inited_ ? "YES" : "NO"),
                 tiz_buffer_available (p_prc->p_store_), p_prc->store_offset_);
      bytes_read = MIN (
        count, tiz_buffer_available (p_prc->p_store_) - p_prc->store_offset_);
      memcpy (ap_ptr, tiz_buffer_get (p_prc->p_store_) + p_prc->store_offset_,
              bytes_read);
      if (p_prc->decoder_inited_)
        {
          tiz_buffer_advance (p_prc->p_store_,
                              bytes_read + p_prc->store_offset_);
          p_prc->store_offset_ = 0;
        }
      else
        {
          p_prc->store_offset_ += bytes_read;
        }
    }
  TIZ_TRACE (handleOf (p_prc), "Satisfied callback ? [%s]",
             (bytes_read == count ? "YES" : "NO"));
  return bytes_read;
}

static sf_count_t
sf_io_write (const void * ptr, sf_count_t count, void * user_data)
{
  /* Nothing to write */
  return -1;
}

static sf_count_t
sf_io_tell (void * user_data)
{
  sndfiled_prc_t * p_prc = (sndfiled_prc_t *) user_data;
  assert (p_prc);
  return p_prc->store_offset_;
}

static OMX_ERRORTYPE
open_sf (sndfiled_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);

  if (store_data (ap_prc) && tiz_buffer_available (ap_prc->p_store_) > 0)
    {
      if (!ap_prc->p_sf_)
        {
          ap_prc->sf_info_.format |= SF_ENDIAN_LITTLE;
          ap_prc->p_sf_ = sf_open_virtual (&(ap_prc->sf_io_), SFM_READ,
                                           &(ap_prc->sf_info_), ap_prc);
          if (!ap_prc->p_sf_)
            {
              TIZ_ERROR (handleOf (ap_prc),
                         "Unable to open the sf handle : [%s]",
                         sf_strerror (NULL));
              ap_prc->store_offset_ = 0;
            }
          else
            {
              SF_INFO * p = &(ap_prc->sf_info_);
              ap_prc->decoder_inited_ = true;
              tiz_buffer_advance (ap_prc->p_store_, ap_prc->store_offset_);

              TIZ_TRACE (handleOf (ap_prc), "frames [%d]", p->frames);
              TIZ_TRACE (handleOf (ap_prc), "samplerate [%d]", p->samplerate);
              TIZ_TRACE (handleOf (ap_prc), "channels [%d]", p->channels);
              TIZ_TRACE (handleOf (ap_prc), "format [%d]", p->format);
              TIZ_TRACE (handleOf (ap_prc), "sections [%d]", p->sections);
              TIZ_TRACE (handleOf (ap_prc), "seekable [%d]", p->seekable);
            }
        }
    }

  return rc;
}

static OMX_ERRORTYPE
transform_buffer (sndfiled_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNotReady;
  OMX_BUFFERHEADERTYPE * p_out = get_out_hdr (ap_prc);

  (void) store_data (ap_prc);

  assert (ap_prc);
  assert (ap_prc->p_sf_);

  if (p_out && tiz_buffer_available (ap_prc->p_store_) > 0)
    {
      size_t frame_size = sizeof (short int) * ap_prc->sf_info_.channels;
      sf_count_t read_frames = p_out->nAllocLen / frame_size;
      sf_count_t num_frames = sf_readf_short (
        ap_prc->p_sf_, (short int *) (p_out->pBuffer + p_out->nOffset),
        read_frames);
      p_out->nFilledLen = num_frames * frame_size;
      if (num_frames > 0 || tiz_filter_prc_is_eos (ap_prc))
        {
          (void) release_out_hdr (ap_prc);
        }
      rc = num_frames > 0 ? OMX_ErrorNone : OMX_ErrorNotReady;
    }
  return rc;
}

static void
reset_stream_parameters (sndfiled_prc_t * ap_prc)
{
  assert (ap_prc);
  ap_prc->decoder_inited_ = false;
  tiz_buffer_clear (ap_prc->p_store_);
  ap_prc->store_offset_ = 0;
  tiz_filter_prc_update_eos_flag (ap_prc, false);
}

/*
 * sndfiledprc
 */

static void *
sndfiled_prc_ctor (void * ap_obj, va_list * app)
{
  sndfiled_prc_t * p_prc
    = super_ctor (typeOf (ap_obj, "sndfiledprc"), ap_obj, app);
  assert (p_prc);
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

static void *
sndfiled_prc_dtor (void * ap_obj)
{
  (void) sndfiled_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "sndfiledprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
sndfiled_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  return allocate_temp_data_store (ap_obj);
}

static OMX_ERRORTYPE
sndfiled_prc_deallocate_resources (void * ap_obj)
{
  sndfiled_prc_t * p_prc = ap_obj;
  deallocate_temp_data_store (p_prc);
  assert (p_prc);
  sf_close (p_prc->p_sf_);
  p_prc->p_sf_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sndfiled_prc_prepare_to_transfer (void * ap_obj, OMX_U32 a_pid)
{
  reset_stream_parameters (ap_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sndfiled_prc_transfer_and_process (void * ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sndfiled_prc_stop_and_return (void * ap_obj)
{
  return tiz_filter_prc_release_all_headers (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
sndfiled_prc_buffers_ready (const void * ap_prc)
{
  sndfiled_prc_t * p_prc = (sndfiled_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_prc);

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

static OMX_ERRORTYPE
sndfiled_prc_port_enable (const void * ap_prc, OMX_U32 a_pid)
{
  sndfiled_prc_t * p_prc = (sndfiled_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = sndfiled_prc_allocate_resources (p_prc, OMX_ALL);
  if (OMX_ErrorNone == rc)
    {
      rc = sndfiled_prc_prepare_to_transfer (p_prc, OMX_ALL);
    }
  return rc;
}

static OMX_ERRORTYPE
sndfiled_prc_port_disable (const void * ap_prc, OMX_U32 a_pid)
{
  sndfiled_prc_t * p_prc = (sndfiled_prc_t *) ap_prc;
  (void) sndfiled_prc_deallocate_resources (p_prc);
  return tiz_filter_prc_release_header (p_prc, a_pid);
}

/*
 * sndfiled_prc_class
 */

static void *
sndfiled_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "sndfiledprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
sndfiled_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * sndfiledprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizfilterprc), "sndfiledprc_class", classOf (tizfilterprc),
     sizeof (sndfiled_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, sndfiled_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return sndfiledprc_class;
}

void *
sndfiled_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * sndfiledprc_class = tiz_get_type (ap_hdl, "sndfiledprc_class");
  TIZ_LOG_CLASS (sndfiledprc_class);
  void * sndfiledprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (sndfiledprc_class, "sndfiledprc", tizfilterprc, sizeof (sndfiled_prc_t),
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
