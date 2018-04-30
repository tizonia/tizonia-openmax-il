/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   vorbisdprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Vorbis decoder processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <signal.h>

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <fishsound/constants.h>

#include <tizplatform.h>

#include <tizkernel.h>

#include "vorbisd.h"
#include "vorbisdprc.h"
#include "vorbisdprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.vorbis_decoder.prc"
#endif

/* This macros assume the existence of an "ap_prc" local variable */
#define bail_on_fish_error(expr, msg)                      \
  do                                                       \
    {                                                      \
      int fish_error = FISH_SOUND_OK;                      \
      if (FISH_SOUND_OK != (fish_error = (expr)))          \
        {                                                  \
          TIZ_ERROR (handleOf (ap_prc),                    \
                     "[OMX_ErrorInsufficientResources] : " \
                     "%s (fish error %d)",                 \
                     msg, fish_error);                     \
          goto end;                                        \
        }                                                  \
    }                                                      \
  while (0)

/* Forward declarations */
static OMX_ERRORTYPE
vorbisd_prc_deallocate_resources (void *);

static OMX_ERRORTYPE
alloc_temp_data_store (vorbisd_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);
  if (!ap_prc->p_store_)
    {
      port_def.nSize = (OMX_U32) sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
      port_def.nVersion.nVersion = OMX_VERSION;
      port_def.nPortIndex = ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX;

      tiz_check_omx (tiz_api_GetParameter (
        tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
        OMX_IndexParamPortDefinition, &port_def));

      assert (ap_prc->p_store_ == NULL);
      ap_prc->store_size_ = port_def.nBufferSize;
      tiz_check_null_ret_oom (
        (ap_prc->p_store_ = tiz_mem_alloc (ap_prc->store_size_)));
    }
  return OMX_ErrorNone;
}

static inline void
dealloc_temp_data_store (
  /*@special@ */ vorbisd_prc_t * ap_prc)
/*@releases ap_prc->p_store_@ */
/*@ensures isnull ap_prc->p_store_@ */
{
  assert (ap_prc);
  tiz_mem_free (ap_prc->p_store_);
  ap_prc->p_store_ = NULL;
  ap_prc->store_size_ = 0;
  ap_prc->store_offset_ = 0;
}

static inline OMX_U8 **
get_store_ptr (vorbisd_prc_t * ap_prc)
{
  assert (ap_prc);
  return &(ap_prc->p_store_);
}

static inline OMX_U32 *
get_store_size_ptr (vorbisd_prc_t * ap_prc)
{
  assert (ap_prc);
  return &(ap_prc->store_size_);
}

static inline OMX_U32 *
get_store_offset_ptr (vorbisd_prc_t * ap_prc)
{
  assert (ap_prc);
  return &(ap_prc->store_offset_);
}

static int
store_data (vorbisd_prc_t * ap_prc, const OMX_U8 * ap_data, OMX_U32 a_nbytes)
{
  OMX_U8 ** pp_store = NULL;
  OMX_U32 * p_offset = NULL;
  OMX_U32 * p_size = NULL;
  OMX_U32 nbytes_to_copy = 0;
  OMX_U32 nbytes_avail = 0;

  assert (ap_prc);
  assert (ap_data);

  pp_store = get_store_ptr (ap_prc);
  p_size = get_store_size_ptr (ap_prc);
  p_offset = get_store_offset_ptr (ap_prc);

  assert (pp_store && *pp_store);
  assert (p_size);
  assert (p_offset);

  nbytes_avail = *p_size - *p_offset;

  if (a_nbytes > nbytes_avail)
    {
      /* need to re-alloc */
      OMX_U8 * p_new_store = NULL;
      p_new_store = tiz_mem_realloc (*pp_store, *p_offset + a_nbytes);
      if (p_new_store)
        {
          *pp_store = p_new_store;
          *p_size = *p_offset + a_nbytes;
          nbytes_avail = *p_size - *p_offset;
          TIZ_TRACE (handleOf (ap_prc),
                     "Realloc'd data store "
                     "to new size [%d]",
                     *p_size);
        }
    }
  nbytes_to_copy = MIN (nbytes_avail, a_nbytes);
  memcpy (*pp_store + *p_offset, ap_data, nbytes_to_copy);
  *p_offset += nbytes_to_copy;

  TIZ_TRACE (handleOf (ap_prc), "bytes currently stored [%d]", *p_offset);

  return a_nbytes - nbytes_to_copy;
}

static inline void
write_frame_float_ilv (float * to, const float * from, const int channels)
{
  int i = 0;
  assert (to);
  assert (from);
  for (i = 0; i < channels; ++i)
    {
      to[i] = from[i];
    }
}

static OMX_ERRORTYPE
update_pcm_mode (vorbisd_prc_t * ap_prc, const OMX_U32 a_samplerate,
                 const OMX_U32 a_channels)
{
  assert (ap_prc);
  if (a_samplerate != ap_prc->pcmmode_.nSamplingRate
      || a_channels != ap_prc->pcmmode_.nChannels)
    {
      TIZ_DEBUG (handleOf (ap_prc),
                 "Updating pcm mode : old samplerate [%d] new samplerate [%d]",
                 ap_prc->pcmmode_.nSamplingRate, a_samplerate);
      TIZ_DEBUG (handleOf (ap_prc),
                 "Updating pcm mode : old channels [%d] new channels [%d]",
                 ap_prc->pcmmode_.nChannels, a_channels);
      ap_prc->pcmmode_.nSamplingRate = a_samplerate;
      ap_prc->pcmmode_.nChannels = a_channels;
      tiz_check_omx (tiz_krn_SetParameter_internal (
        tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
        OMX_IndexParamAudioPcm, &(ap_prc->pcmmode_)));
      tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventPortSettingsChanged,
                           ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX,
                           OMX_IndexParamAudioPcm, /* the index of the
                                                      struct that has
                                                      been modififed */
                           NULL);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
store_metadata (vorbisd_prc_t * ap_prc, const char * ap_header_name,
                const char * ap_header_info)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_CONFIG_METADATAITEMTYPE * p_meta = NULL;
  size_t metadata_len = 0;
  size_t info_len = 0;

  assert (ap_prc);
  if (ap_header_name && ap_header_info)
    {
      info_len = strnlen (ap_header_info, OMX_MAX_STRINGNAME_SIZE - 1) + 1;
      metadata_len = sizeof (OMX_CONFIG_METADATAITEMTYPE) + info_len;

      if (!(p_meta
            = (OMX_CONFIG_METADATAITEMTYPE *) tiz_mem_calloc (1, metadata_len)))
        {
          rc = OMX_ErrorInsufficientResources;
        }
      else
        {
          const size_t name_len
            = strnlen (ap_header_name, OMX_MAX_STRINGNAME_SIZE - 1) + 1;
          strncpy ((char *) p_meta->nKey, ap_header_name, name_len - 1);
          p_meta->nKey[name_len - 1] = '\0';
          p_meta->nKeySizeUsed = name_len;

          strncpy ((char *) p_meta->nValue, ap_header_info, info_len - 1);
          p_meta->nValue[info_len - 1] = '\0';
          p_meta->nValueMaxSize = info_len;
          p_meta->nValueSizeUsed = info_len;

          p_meta->nSize = metadata_len;
          p_meta->nVersion.nVersion = OMX_VERSION;
          p_meta->eScopeMode = OMX_MetadataScopeAllLevels;
          p_meta->nScopeSpecifier = 0;
          p_meta->nMetadataItemIndex = 0;
          p_meta->eSearchMode = OMX_MetadataSearchValueSizeByIndex;
          p_meta->eKeyCharset = OMX_MetadataCharsetASCII;
          p_meta->eValueCharset = OMX_MetadataCharsetASCII;

          rc = tiz_krn_store_metadata (tiz_get_krn (handleOf (ap_prc)), p_meta);
        }
    }
  return rc;
}

static void
store_stream_metadata (vorbisd_prc_t * ap_prc)
{
  assert (ap_prc);

  {
    char info[100];

    (void) tiz_krn_clear_metadata (tiz_get_krn (handleOf (ap_prc)));

    snprintf (info, 99, "%d Ch, %d Hz", ap_prc->fsinfo_.channels,
              ap_prc->fsinfo_.samplerate);
    info[99] = '\0';
    (void) store_metadata (ap_prc, "Vorbis Stream", info);
  }

  /* Signal that a new set of metadata items is available */
  (void) tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventIndexSettingChanged,
                              OMX_ALL, /* no particular port associated */
                              OMX_IndexConfigMetadataItem, /* index of the
                                                             struct that has
                                                             been modififed */
                              NULL);
}

static int
fishsound_decoded_callback (FishSound * ap_fsound, float * app_pcm[],
                            long frames, void * ap_user_data)
{
  int rc = FISH_SOUND_CONTINUE;
  vorbisd_prc_t * p_prc = ap_user_data;
  OMX_BUFFERHEADERTYPE * p_out = NULL;

  (void) ap_fsound;
  assert (app_pcm);
  assert (ap_user_data);

  TIZ_PRINTF_DBG_RED ("frames [%d] \n", frames);

  /* Possible return values are: */

  /* FISH_SOUND_CONTINUE Continue decoding */
  /* FISH_SOUND_STOP_OK Stop decoding immediately and return control to the
     fish_sound_decode() caller */
  /* FISH_SOUND_STOP_ERR Stop decoding immediately, purge buffered data, and
     return control to the fish_sound_decode() caller */

  if (!p_prc->started_)
    {
      p_prc->started_ = true;
      fish_sound_command (p_prc->p_fsnd_, FISH_SOUND_GET_INFO,
                          &(p_prc->fsinfo_), sizeof (FishSoundInfo));
      if (p_prc->fsinfo_.channels > 2
          || p_prc->fsinfo_.format != FISH_SOUND_VORBIS)
        {
          TIZ_ERROR (handleOf (p_prc),
                     "Supported Vorbis "
                     "streams up tp 2 channels only.");
          rc = FISH_SOUND_STOP_ERR;
          goto end;
        }
      TIZ_NOTICE (handleOf (p_prc), "Channels [%d] sampling rate [%d]",
                  p_prc->fsinfo_.channels, p_prc->fsinfo_.samplerate);
      store_stream_metadata (p_prc);
      (void) update_pcm_mode (p_prc, p_prc->fsinfo_.samplerate,
                              p_prc->fsinfo_.channels);
    }

  p_out = tiz_filter_prc_get_header (p_prc,
                                     ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX);
  if (!p_out)
    {
      TIZ_TRACE (handleOf (p_prc),
                 "No more output buffers "
                 "available at the moment");
      rc = FISH_SOUND_STOP_OK;
      goto end;
    }

  {
    /* write decoded PCM samples */
    size_t i = 0;
    size_t frame_len = sizeof (float) * p_prc->fsinfo_.channels;
    size_t frames_alloc = ((p_out->nAllocLen - p_out->nOffset) / frame_len);
    size_t frames_to_write = (frames > frames_alloc) ? frames_alloc : frames;
    size_t bytes_to_write = frames_to_write * frame_len;
    assert (p_out);

    for (i = 0; i < frames_to_write; ++i)
      {
        size_t frame_offset = i * frame_len;
        size_t float_offset = i * p_prc->fsinfo_.channels;
        float * out
          = (float *) (p_out->pBuffer + p_out->nOffset + frame_offset);
        write_frame_float_ilv (out, ((float *) app_pcm) + float_offset,
                               p_prc->fsinfo_.channels);
      }
    p_out->nFilledLen += bytes_to_write;
    p_out->nOffset += bytes_to_write;

    if (frames_to_write < frames)
      {
        /* Temporarily store the data until an omx buffer is
         * available */
        OMX_U32 nbytes_remaining = (frames - frames_to_write) * frame_len;
        TIZ_TRACE (handleOf (p_prc), "Need to store [%d] bytes",
                   nbytes_remaining);
        nbytes_remaining = store_data (
          p_prc, (OMX_U8 *) (app_pcm[frames_to_write * sizeof (float)]),
          nbytes_remaining);
      }

    if (tiz_filter_prc_is_eos (p_prc))
      {
        /* Propagate EOS flag to output */
        p_out->nFlags |= OMX_BUFFERFLAG_EOS;
        tiz_filter_prc_update_eos_flag (p_prc, false);
        TIZ_TRACE (handleOf (p_prc), "Propagating EOS flag to output");
      }
    /* TODO: Shouldn't ignore this rc */
    (void) tiz_filter_prc_release_header (
      p_prc, ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX);
    /* Let's process one input buffer at a time, for now */
    rc = FISH_SOUND_STOP_OK;
  }

end:

  return rc;
}

static OMX_ERRORTYPE
init_vorbis_decoder (vorbisd_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  assert (ap_prc);

  if (!ap_prc->p_fsnd_)
    {
      ap_prc->p_fsnd_ = fish_sound_new (FISH_SOUND_DECODE, &(ap_prc->fsinfo_));
      tiz_check_null_ret_oom (ap_prc->p_fsnd_);

      bail_on_fish_error (
        fish_sound_set_decoded_float_ilv (ap_prc->p_fsnd_,
                                          fishsound_decoded_callback, ap_prc),
        "Could not set the 'decoded' callback.");
    }

  rc = OMX_ErrorNone;

end:

  if (OMX_ErrorInsufficientResources == rc && ap_prc->p_fsnd_)
    {
      fish_sound_delete (ap_prc->p_fsnd_);
      ap_prc->p_fsnd_ = NULL;
    }

  return rc;
}

static OMX_ERRORTYPE
transform_buffer (vorbisd_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE * p_in = tiz_filter_prc_get_header (
    ap_prc, ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX);
  OMX_BUFFERHEADERTYPE * p_out = tiz_filter_prc_get_header (
    ap_prc, ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX);

  if (!p_in || !p_out)
    {
      TIZ_TRACE (handleOf (ap_prc), "IN HEADER [%p] OUT HEADER [%p]", p_in,
                 p_out);
      return OMX_ErrorNone;
    }

  assert (ap_prc);

  TIZ_TRACE (handleOf (ap_prc), "HEADER [%p] nFilledLen [%d] nFlags [%d] ",
             p_in, p_in->nFilledLen, p_in->nFlags);

  if (0 == p_in->nFilledLen)
    {
      TIZ_TRACE (handleOf (ap_prc), "HEADER [%p] nFlags [%d] is empty", p_in,
                 p_in->nFlags);
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          /* Inmediately propagate EOS flag to output */
          TIZ_TRACE (handleOf (ap_prc), "Let's propagate EOS flag to output");
          p_out->nFlags |= OMX_BUFFERFLAG_EOS;
          p_in->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
          tiz_check_omx (tiz_filter_prc_release_header (
            ap_prc, ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX));
        }
    }
  /*   raise(SIGTRAP); */

  if (p_in->nFilledLen > 0)
    {
      unsigned char * p_data = p_in->pBuffer + p_in->nOffset;
      const long len = p_in->nFilledLen;
      long bytes_consumed = fish_sound_decode (ap_prc->p_fsnd_, p_data, len);
      TIZ_TRACE (handleOf (ap_prc), "p_in->nFilledLen [%d] ", p_in->nFilledLen);
      TIZ_TRACE (handleOf (ap_prc), "bytes_consumed [%d] ", bytes_consumed);

      if (bytes_consumed >= 0)
        {
          assert (p_in->nFilledLen >= bytes_consumed);
          p_in->nFilledLen = 0;
          p_in->nOffset = 0;
        }
      else
        {
          switch (bytes_consumed)
            {
              case FISH_SOUND_STOP_ERR:
                {
                  /* Decoding was stopped by a FishSoundDecode* */
                  /* callback returning FISH_SOUND_STOP_ERR before any input
                   * bytes were consumed. */
                  /* This will occur when PCM is decoded from previously
                   * buffered input, and */
                  /* stopping is immediately requested. */
                  TIZ_ERROR (handleOf (ap_prc),
                             "[FISH_SOUND_ERR_STOP_ERR] : "
                             "While decoding the input stream.");
                  rc = OMX_ErrorStreamCorruptFatal;
                }
                break;
              case FISH_SOUND_ERR_OUT_OF_MEMORY:
                {
                  TIZ_ERROR (handleOf (ap_prc),
                             "[FISH_SOUND_ERR_OUT_OF_MEMORY] : "
                             "While decoding the input stream.");
                  rc = OMX_ErrorInsufficientResources;
                }
                break;
              case FISH_SOUND_ERR_BAD:
                {
                  TIZ_ERROR (handleOf (ap_prc),
                             "[FISH_SOUND_ERR_BAD] : "
                             "While decoding the input stream.");
                  rc = OMX_ErrorStreamCorruptFatal;
                }
                break;
              default:
                {
                  TIZ_ERROR (handleOf (ap_prc),
                             "[%s] : "
                             "While decoding the input stream.",
                             bytes_consumed);
                  assert (0);
                  rc = OMX_ErrorStreamCorruptFatal;
                }
            };
        }
    }

  if (0 == p_in->nFilledLen)
    {
      TIZ_TRACE (handleOf (ap_prc), "HEADER [%p] nFlags [%d] is empty", p_in,
                 p_in->nFlags);
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          /* Let's propagate EOS flag to output */
          TIZ_TRACE (handleOf (ap_prc), "Let's propagate EOS flag to output");
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          p_in->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
        }
      rc = tiz_filter_prc_release_header (
        ap_prc, ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX);
    }

  return rc;
}

static void
reset_stream_parameters (vorbisd_prc_t * ap_prc)
{
  assert (ap_prc);
  ap_prc->started_ = false;
  if (ap_prc->p_fsnd_)
    {
      fish_sound_reset (ap_prc->p_fsnd_);
    }
  tiz_mem_set (&(ap_prc->fsinfo_), 0, sizeof (FishSoundInfo));
  if (ap_prc->p_store_)
    {
      tiz_mem_set (ap_prc->p_store_, 0, ap_prc->store_size_);
      ap_prc->store_offset_ = 0;
    }
}

static inline OMX_ERRORTYPE
do_flush (vorbisd_prc_t * ap_prc, OMX_U32 a_pid)
{
  assert (ap_prc);
  if (OMX_ALL == a_pid || ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX == a_pid)
    {
      reset_stream_parameters (ap_prc);
    }
  /* Release any buffers held  */
  return tiz_filter_prc_release_header (ap_prc, a_pid);
}

/*
 * vorbisdprc
 */

static void *
vorbisd_prc_ctor (void * ap_obj, va_list * app)
{
  vorbisd_prc_t * p_prc
    = super_ctor (typeOf (ap_obj, "vorbisdprc"), ap_obj, app);
  assert (p_prc);
  p_prc->p_fsnd_ = NULL;
  p_prc->started_ = false;
  p_prc->p_store_ = NULL;
  p_prc->store_size_ = 0;
  p_prc->store_offset_ = 0;
  return p_prc;
}

static void *
vorbisd_prc_dtor (void * ap_obj)
{
  (void) vorbisd_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "vorbisdprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
vorbisd_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  tiz_check_omx (alloc_temp_data_store (ap_obj));
  return init_vorbis_decoder (ap_obj);
}

static OMX_ERRORTYPE
vorbisd_prc_deallocate_resources (void * ap_obj)
{
  vorbisd_prc_t * p_prc = ap_obj;
  assert (p_prc);
  if (p_prc->p_fsnd_)
    {
      fish_sound_delete (p_prc->p_fsnd_);
      p_prc->p_fsnd_ = NULL;
    }
  dealloc_temp_data_store (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vorbisd_prc_prepare_to_transfer (void * ap_obj, OMX_U32 a_pid)
{
  vorbisd_prc_t * p_prc = ap_obj;
  assert (p_prc);
  TIZ_INIT_OMX_PORT_STRUCT (p_prc->pcmmode_,
                            ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX);
  tiz_check_omx (tiz_api_GetParameter (tiz_get_krn (handleOf (p_prc)),
                                       handleOf (p_prc), OMX_IndexParamAudioPcm,
                                       &(p_prc->pcmmode_)));
  TIZ_TRACE (handleOf (p_prc),
             "sample rate renderer = [%d] channels renderer = [%d]",
             p_prc->pcmmode_.nSamplingRate, p_prc->pcmmode_.nChannels);
  reset_stream_parameters (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vorbisd_prc_transfer_and_process (void * ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vorbisd_prc_stop_and_return (void * ap_obj)
{
  return tiz_filter_prc_release_all_headers (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
vorbisd_prc_buffers_ready (const void * ap_obj)
{
  vorbisd_prc_t * p_prc = (vorbisd_prc_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_prc);

  TIZ_TRACE (handleOf (p_prc), "eos [%s] avail [%s]",
             tiz_filter_prc_is_eos (p_prc) ? "YES" : "NO",
             tiz_filter_prc_headers_available (p_prc) ? "YES" : "NO");
  while (tiz_filter_prc_headers_available (p_prc) && OMX_ErrorNone == rc)
    {
      rc = transform_buffer (p_prc);
    }

  if (tiz_filter_prc_is_eos (p_prc))
    {
      OMX_BUFFERHEADERTYPE * p_out = tiz_filter_prc_get_header (
        p_prc, ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX);
      if (p_out)
        {
          p_out->nFlags |= OMX_BUFFERFLAG_EOS;
          tiz_filter_prc_update_eos_flag (p_prc, false);
          tiz_check_omx (tiz_filter_prc_release_header (
            p_prc, ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX));
        }
    }
  return rc;
}

static OMX_ERRORTYPE
vorbisd_prc_port_flush (const void * ap_obj, OMX_U32 a_pid)
{
  vorbisd_prc_t * p_obj = (vorbisd_prc_t *) ap_obj;
  return do_flush (p_obj, a_pid);
}

static OMX_ERRORTYPE
vorbisd_prc_port_disable (const void * ap_prc, OMX_U32 a_pid)
{
  vorbisd_prc_t * p_prc = (vorbisd_prc_t *) ap_prc;
  assert (p_prc);
  if (OMX_ALL == a_pid || ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX == a_pid)
    {
      reset_stream_parameters (p_prc);
    }
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, true);
  return tiz_filter_prc_release_header (p_prc, a_pid);
}

static OMX_ERRORTYPE
vorbisd_prc_port_enable (const void * ap_obj, OMX_U32 a_pid)
{
  vorbisd_prc_t * p_prc = (vorbisd_prc_t *) ap_obj;
  assert (p_prc);
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, false);
  if (OMX_ALL == a_pid || ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX == a_pid)
    {
      (void) vorbisd_prc_allocate_resources (p_prc, a_pid);
    }
  return OMX_ErrorNone;
}

/*
 * vorbisd_prc_class
 */

static void *
vorbisd_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "vorbisdprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
vorbisd_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * vorbisdprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizfilterprc), "vorbisdprc_class", classOf (tizfilterprc),
     sizeof (vorbisd_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, vorbisd_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return vorbisdprc_class;
}

void *
vorbisd_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * vorbisdprc_class = tiz_get_type (ap_hdl, "vorbisdprc_class");
  TIZ_LOG_CLASS (vorbisdprc_class);
  void * vorbisdprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (vorbisdprc_class, "vorbisdprc", tizfilterprc, sizeof (vorbisd_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, vorbisd_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, vorbisd_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, vorbisd_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, vorbisd_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, vorbisd_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, vorbisd_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, vorbisd_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, vorbisd_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, vorbisd_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, vorbisd_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, vorbisd_prc_port_enable,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return vorbisdprc;
}
