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
 * @file   arprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Audio Renderer Component processor class
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <byteswap.h>

#include <tizplatform.h>

#include <tizutils.h>
#include <tizkernel.h>

#include "ar.h"
#include "arprc.h"
#include "arprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.audio_renderer.prc"
#endif

/* This is for convenience. This macro assumes the existence of an
   "ap_prc" local variable */
#define bail_on_snd_mixer_error(expr)                           \
  do                                                            \
    {                                                           \
      int alsa_error = 0;                                       \
      if (0 != (alsa_error = (expr)))                           \
        {                                                       \
          TIZ_ERROR (handleOf (ap_prc),                         \
                     "[OMX_ErrorInsufficientResources] : "      \
                     "while accessing alsa master volume (%s)", \
                     snd_strerror (alsa_error));                \
          goto end;                                             \
        }                                                       \
    }                                                           \
  while (0)

/* This is for convenience. This macro assumes the existence of an
   "ap_prc" local variable */
#define bail_on_snd_pcm_error(expr)                        \
  do                                                       \
    {                                                      \
      int alsa_error = 0;                                  \
      if ((alsa_error = (expr)) < 0)                       \
        {                                                  \
          TIZ_ERROR (handleOf (ap_prc),                    \
                     "[OMX_ErrorInsufficientResources] : " \
                     "%s",                                 \
                     snd_strerror (alsa_error));           \
          return OMX_ErrorInsufficientResources;           \
        }                                                  \
    }                                                      \
  while (0)

#define log_alsa_pcm_state(ap_obj)                                         \
  do                                                                       \
    {                                                                      \
      const ar_prc_t * ap_prc = ap_obj;                                    \
      if (ap_prc && ap_prc->p_pcm_)                                        \
        {                                                                  \
          TIZ_DEBUG (handleOf (ap_prc), "ALSA PCM state : [%s]",           \
                     snd_pcm_state_name (snd_pcm_state (ap_prc->p_pcm_))); \
        }                                                                  \
    }                                                                      \
  while (0)

/* Forward declaration */
static OMX_ERRORTYPE
ar_prc_deallocate_resources (void * ap_prc);
static void
stop_eos_timer (ar_prc_t * ap_prc);

static void
alsa_error_handler (const char * file, int line, const char * function, int err,
                    const char * fmt, ...)
{
  char err_msg[128];
  int index, len;
  va_list arg;

#ifndef NDEBUG
  index = snprintf (err_msg, sizeof (err_msg), "ALSA lib %s:%i:(%s) ", file,
                    line, function);
#else
  index = snprintf (err_msg, sizeof (err_msg), "ALSA lib: ");
#endif
  if (index < 1 || index >= (int) sizeof (err_msg))
    {
      index = sizeof (err_msg) - 1;
      err_msg[index] = '\0';
      goto print_msg;
    }

  va_start (arg, fmt);
  if (index < sizeof (err_msg) - 1)
    {
      len = vsnprintf (err_msg + index, sizeof (err_msg) - index, fmt, arg);
      if (len < 1 || len >= (int) sizeof (err_msg) - index)
        len = sizeof (err_msg) - index - 1;
      index += len;
      err_msg[index] = '\0';
    }
  va_end (arg);
  if (err && index < sizeof (err_msg) - 1)
    {
      len = snprintf (err_msg + index, sizeof (err_msg) - index, ": %s",
                      snd_strerror (err));
      if (len < 1 || len >= (int) sizeof (err_msg) - index)
        len = sizeof (err_msg) - index - 1;
      index += len;
      err_msg[index] = '\0';
    }
print_msg:
  TIZ_LOG (TIZ_PRIORITY_ERROR, "%s", err_msg);
}

static void
check_alsa_support_pcm_format (ar_prc_t * ap_prc,
                               snd_pcm_format_t * ap_snd_pcm_format)
{
  int fmt = 0;
  snd_pcm_format_mask_t * fmask = NULL;
  bool is_supported_format = false;

  assert (ap_prc);
  assert (ap_snd_pcm_format);

  snd_pcm_format_mask_alloca (&fmask);
  snd_pcm_hw_params_get_format_mask (ap_prc->p_hw_params_, fmask);

  for (fmt = 0; fmt <= SND_PCM_FORMAT_LAST; ++fmt)
    {
      if (snd_pcm_format_mask_test (fmask, (snd_pcm_format_t) fmt))
        {
          const snd_pcm_format_t supported_format = snd_pcm_format_value (
            snd_pcm_format_name ((snd_pcm_format_t) fmt));
          TIZ_DEBUG (handleOf (ap_prc), "%s : byte order [%s]",
                     snd_pcm_format_name ((snd_pcm_format_t) fmt),
                     snd_pcm_format_little_endian (supported_format) == 1
                       ? "LITTLE"
                       : "BIG");
          if (supported_format == *ap_snd_pcm_format)
            {
              is_supported_format = true;
              TIZ_DEBUG (handleOf (ap_prc),
                         "%s : format supported by the alsa pcm",
                         snd_pcm_format_name ((snd_pcm_format_t) fmt));
              break;
            }
        }
    }

  if (!is_supported_format)
    {
      ap_prc->swap_byte_order_ = true;

      switch (*ap_snd_pcm_format)
        {
          case SND_PCM_FORMAT_FLOAT_LE:
            {
              *ap_snd_pcm_format = SND_PCM_FORMAT_FLOAT_BE;
            }
            break;
          case SND_PCM_FORMAT_FLOAT_BE:
            {
              *ap_snd_pcm_format = SND_PCM_FORMAT_FLOAT_LE;
            }
            break;
          case SND_PCM_FORMAT_S24:
            {
              *ap_snd_pcm_format = SND_PCM_FORMAT_S24_BE;
            }
            break;
          case SND_PCM_FORMAT_S24_BE:
            {
              *ap_snd_pcm_format = SND_PCM_FORMAT_S24;
            }
            break;
          case SND_PCM_FORMAT_S16:
            {
              *ap_snd_pcm_format = SND_PCM_FORMAT_S16_BE;
            }
            break;
          case SND_PCM_FORMAT_S16_BE:
            {
              *ap_snd_pcm_format = SND_PCM_FORMAT_S16;
            }
            break;
          default:
            {
            }
            break;
        };
    }
}

static void
check_alsa_support_num_channels (ar_prc_t * ap_prc,
                                 unsigned int * ap_num_channels)
{
  unsigned int min_channels = 0;
  unsigned int max_channels = 0;

  assert (ap_prc);
  assert (ap_num_channels);

  snd_pcm_hw_params_get_channels_min (ap_prc->p_hw_params_, &min_channels);
  snd_pcm_hw_params_get_channels_max (ap_prc->p_hw_params_, &max_channels);
  TIZ_DEBUG (handleOf (ap_prc), "channels min = %d - channels max = %d]",
             min_channels, max_channels);

  if (ap_prc->pcmmode_.nChannels < min_channels)
    {
      *ap_num_channels = min_channels;
    }
  else if (ap_prc->pcmmode_.nChannels > max_channels)
    {
      *ap_num_channels = max_channels;
    }
  else
    {
      *ap_num_channels = ap_prc->pcmmode_.nChannels;
    }
}

static OMX_ERRORTYPE
retrieve_alsa_pcm_format_and_num_channels (ar_prc_t * ap_prc,
                                           snd_pcm_format_t * ap_snd_pcm_format,
                                           unsigned int * ap_num_channels)
{

  assert (ap_prc);
  assert (ap_snd_pcm_format);
  assert (ap_num_channels);
  assert (ap_prc->p_hw_params_);

  TIZ_INIT_OMX_PORT_STRUCT (ap_prc->pcmmode_,
                            ARATELIA_AUDIO_RENDERER_PORT_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamAudioPcm, &ap_prc->pcmmode_));

  if (ap_prc->pcmmode_.nBitPerSample == 24)
    {
      *ap_snd_pcm_format = ap_prc->pcmmode_.eEndian == OMX_EndianLittle
                             ? SND_PCM_FORMAT_S24
                             : SND_PCM_FORMAT_S24_BE;
    }
  /* NOTE: this is to allow float pcm streams coming from the the vorbis or
     opusfile decoders */
  else if (ap_prc->pcmmode_.nBitPerSample == 32)
    {
      *ap_snd_pcm_format = ap_prc->pcmmode_.eEndian == OMX_EndianLittle
                             ? SND_PCM_FORMAT_FLOAT_LE
                             : SND_PCM_FORMAT_FLOAT_BE;
    }
  else
    {
      *ap_snd_pcm_format = ap_prc->pcmmode_.eEndian == OMX_EndianLittle
                             ? SND_PCM_FORMAT_S16
                             : SND_PCM_FORMAT_S16_BE;
    }

  check_alsa_support_pcm_format (ap_prc, ap_snd_pcm_format);
  check_alsa_support_num_channels (ap_prc, ap_num_channels);

  TIZ_NOTICE (
    handleOf (ap_prc),
    "nChannels = [%d] nBitPerSample = [%d] "
    "nSamplingRate = [%d] eNumData = [%s] eEndian = [%s] "
    "bInterleaved = [%s] ePCMMode = [%d] snd_pcm_format = [%s] hw channels = "
    "[%d]",
    ap_prc->pcmmode_.nChannels, ap_prc->pcmmode_.nBitPerSample,
    ap_prc->pcmmode_.nSamplingRate,
    ap_prc->pcmmode_.eNumData == OMX_NumericalDataSigned ? "SIGNED"
                                                         : "UNSIGNED",
    ap_prc->pcmmode_.eEndian == OMX_EndianBig ? "BIG" : "LITTLE",
    ap_prc->pcmmode_.bInterleaved == OMX_TRUE ? "OMX_TRUE" : "OMX_FALSE",
    ap_prc->pcmmode_.ePCMMode, snd_pcm_format_name (*ap_snd_pcm_format),
    *ap_num_channels);

  return OMX_ErrorNone;
}

/*@null@*/ static char *
get_alsa_device (ar_prc_t * ap_prc)
{
  assert (ap_prc);

  if (!ap_prc->p_pcm_name_)
    {
      const char * p_alsa_pcm = tiz_rcfile_get_value (
        TIZ_RCFILE_PLUGINS_DATA_SECTION,
        "OMX.Aratelia.audio_renderer.alsa.pcm.alsa_device");

      if (p_alsa_pcm)
        {
          TIZ_TRACE (handleOf (ap_prc), "Using ALSA pcm [%s]...", p_alsa_pcm);
          ap_prc->p_pcm_name_ = strndup (p_alsa_pcm, OMX_MAX_STRINGNAME_SIZE);
        }
      else
        {
          TIZ_TRACE (handleOf (ap_prc),
                     "No alsa device found in config file. Using [%s]...",
                     ARATELIA_AUDIO_RENDERER_DEFAULT_ALSA_DEVICE);
        }
    }
  return (ap_prc->p_pcm_name_) ? ap_prc->p_pcm_name_
                               : ARATELIA_AUDIO_RENDERER_DEFAULT_ALSA_DEVICE;
}

/*@null@*/ static char *
get_alsa_mixer (ar_prc_t * ap_prc)
{
  assert (ap_prc);

  if (!ap_prc->p_mixer_name_)
    {
      const char * p_alsa_mixer = tiz_rcfile_get_value (
        TIZ_RCFILE_PLUGINS_DATA_SECTION,
        "OMX.Aratelia.audio_renderer.alsa.pcm.alsa_mixer");

      if (p_alsa_mixer)
        {
          TIZ_TRACE (handleOf (ap_prc), "Using ALSA mixer [%s]...",
                     p_alsa_mixer);
          ap_prc->p_mixer_name_
            = strndup (p_alsa_mixer, OMX_MAX_STRINGNAME_SIZE);
        }
      else
        {
          TIZ_TRACE (handleOf (ap_prc),
                     "No alsa mixer found in config file. Using [%s]...",
                     ARATELIA_AUDIO_RENDERER_DEFAULT_ALSA_MIXER);
        }
    }
  return (ap_prc->p_mixer_name_) ? ap_prc->p_mixer_name_
                                 : ARATELIA_AUDIO_RENDERER_DEFAULT_ALSA_MIXER;
}

static bool
using_null_alsa_device (ar_prc_t * ap_prc)
{
  return (0
          == strncmp (get_alsa_device (ap_prc),
                      ARATELIA_AUDIO_RENDERER_NULL_ALSA_DEVICE,
                      OMX_MAX_STRINGNAME_SIZE));
}

static inline OMX_ERRORTYPE
start_io_watcher (ar_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);
  assert (ap_prc->p_ev_io_);
  if (!ap_prc->awaiting_io_ev_)
    {
      rc = tiz_srv_io_watcher_start (ap_prc, ap_prc->p_ev_io_);
    }
  ap_prc->awaiting_io_ev_ = true;
  return rc;
}

static inline void
stop_io_watcher (ar_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->p_ev_io_ && ap_prc->awaiting_io_ev_)
    {
      OMX_ERRORTYPE rc = OMX_ErrorNone;
      rc = tiz_srv_io_watcher_stop (ap_prc, ap_prc->p_ev_io_);
      assert (OMX_ErrorNone == rc);
    }
  ap_prc->awaiting_io_ev_ = false;
}

static OMX_ERRORTYPE
release_header (ar_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_inhdr_)
    {
      ap_prc->p_inhdr_->nOffset = 0;
      tiz_check_omx (tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_prc)),
                                             ARATELIA_AUDIO_RENDERER_PORT_INDEX,
                                             ap_prc->p_inhdr_));
      ap_prc->p_inhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE
do_flush (ar_prc_t * ap_prc)
{
  assert (ap_prc);
  stop_io_watcher (ap_prc);
  stop_eos_timer (ap_prc);
  if (ap_prc->p_pcm_)
    {
      (void) snd_pcm_drop (ap_prc->p_pcm_);
    }
  /* Release any buffers held  */
  return release_header (ap_prc);
}

static float
sint_to_float (const int a_sample)
{
  float f;
  if (a_sample >= 0)
    {
      f = a_sample / 32767.0;
    }
  else
    {
      f = a_sample / 32768.0;
    }
  return f;
}

static int
float_to_sint (const float a_sample)
{
  float f;
  f = a_sample * 32767;
  if (f < -32768)
    {
      f = -32768;
    }
  if (f > 32767)
    {
      f = 32767;
    }
  return (int) f;
}

static void
adjust_gain (const ar_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_hdr,
             const snd_pcm_uframes_t a_samples_per_channel)
{
  assert (ap_prc);
  assert (ap_hdr);

  if (ARATELIA_AUDIO_RENDERER_DEFAULT_GAIN_VALUE != ap_prc->gain_)
    {
      int i;
      int gainadj = (int) (ap_prc->gain_ * 256.);
      float gain = pow (10., gainadj / 5120.);
      /*       gain = 3.1f; */
      /*       fprintf (stderr, "%f samples %ld\n", gain, a_samples_per_channel); */
      OMX_S16 * pcm = (OMX_S16 *) (ap_hdr->pBuffer + ap_hdr->nOffset);
      for (i = 0; i < a_samples_per_channel; i++)
        {
          float f = sint_to_float (*pcm);
          f *= gain;
          int v = float_to_sint (f);
          *(pcm++) = (v > 32767) ? 32767 : ((v < -32768) ? -32768 : v);
          f = sint_to_float (*pcm);
          f *= gain;
          v = float_to_sint (f);
          *(pcm++) = (v > 32767) ? 32767 : ((v < -32768) ? -32768 : v);
        }
    }
}

static void
swap_byte_order_s16 (const ar_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_hdr,
                     const int a_samples)
{
  assert (ap_prc);
  assert (ap_hdr);
  assert (ap_hdr->pBuffer);

  {
    OMX_S16 * p_pcm = (OMX_S16 *) (ap_hdr->pBuffer + ap_hdr->nOffset);
    int i = 0;
    for (i = 0; i < a_samples; ++i)
      {
        *p_pcm = bswap_16 (*p_pcm);
        p_pcm++;
      }
  }
}

static void
swap_byte_order (const ar_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  assert (ap_prc);
  assert (ap_hdr);

  if (ap_prc->swap_byte_order_ && !ap_hdr->nOffset)
    {
      const int bytes_per_sample = ap_prc->pcmmode_.nBitPerSample / 8;
      const int samples = ap_hdr->nFilledLen / bytes_per_sample;
      TIZ_DEBUG (handleOf (ap_prc),
                 "nBitPerSample = [%d] "
                 "nFilledLen = [%d] "
                 "samples = [%d]"
                 "nOffset = [%d]",
                 ap_prc->pcmmode_.nBitPerSample, ap_hdr->nFilledLen, samples,
                 ap_hdr->nOffset);

      switch (ap_prc->pcmmode_.nBitPerSample)
        {
          case 16:
            {
              swap_byte_order_s16 (ap_prc, ap_hdr, samples);
            }
            break;
          default:
            {
            }
            break;
        };
    }
}

static OMX_ERRORTYPE
get_alsa_master_volume (ar_prc_t * ap_prc, long * ap_volume)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);
  assert (ap_volume);

  {
    const char * selem_name = get_alsa_mixer (ap_prc);
    long min, max, volume;
    snd_mixer_t * handle = NULL;
    snd_mixer_selem_id_t * sid = NULL;
    snd_mixer_elem_t * elem = NULL;

    rc = OMX_ErrorInsufficientResources;
    bail_on_snd_mixer_error (snd_mixer_open (&handle, 0));
    bail_on_snd_mixer_error (snd_mixer_attach (handle, ap_prc->p_pcm_name_));
    bail_on_snd_mixer_error (snd_mixer_selem_register (handle, NULL, NULL));
    bail_on_snd_mixer_error (snd_mixer_load (handle));

    snd_mixer_selem_id_alloca (&sid);
    snd_mixer_selem_id_set_index (sid, 0);
    snd_mixer_selem_id_set_name (sid, selem_name);
    elem = snd_mixer_find_selem (handle, sid);

    if (!elem)
      {
        TIZ_ERROR (handleOf (ap_prc),
                   "[OMX_ErrorInsufficientResources] : "
                   "Unable to find mixer simple element.");
        goto end;
      }

    bail_on_snd_mixer_error (
      snd_mixer_selem_get_playback_volume_range (elem, &min, &max));
    bail_on_snd_mixer_error (snd_mixer_selem_get_playback_volume (
      elem, SND_MIXER_SCHN_FRONT_LEFT, &volume));

    *ap_volume = volume * 100 / max;
    bail_on_snd_mixer_error (snd_mixer_close (handle));

    /* Everything well, restore the error code. */
    rc = OMX_ErrorNone;
  }
end:

  return rc;
}

static OMX_ERRORTYPE
set_initial_component_volume (ar_prc_t * ap_prc)
{
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;

  assert (ap_prc);

  if (ARATELIA_AUDIO_RENDERER_DEFAULT_VOLUME_VALUE != ap_prc->volume_)
    {
      /* We want to do this only once, the first time that the component is
         move to Executing */
      return OMX_ErrorNone;
    }

  if (!using_null_alsa_device (ap_prc))
    {
      tiz_check_omx (get_alsa_master_volume (ap_prc, &(ap_prc->volume_)));

      TIZ_INIT_OMX_PORT_STRUCT (volume, ARATELIA_AUDIO_RENDERER_PORT_INDEX);
      tiz_check_omx (tiz_api_GetConfig (tiz_get_krn (handleOf (ap_prc)),
                                        handleOf (ap_prc),
                                        OMX_IndexConfigAudioVolume, &volume));

      volume.sVolume.nValue = ap_prc->volume_;

      TIZ_TRACE (handleOf (ap_prc), "ap_prc->volume_ [%d]", ap_prc->volume_);

      tiz_check_omx (tiz_krn_SetConfig_internal (
        tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
        OMX_IndexConfigAudioVolume, &volume));
    }
  return OMX_ErrorNone;
}

static bool
set_alsa_master_volume (ar_prc_t * ap_prc, const long a_volume)
{
  bool rc = false;
  assert (ap_prc);

  {
    const char * selem_name = get_alsa_mixer (ap_prc);
    long min, max;
    snd_mixer_t * handle = NULL;
    snd_mixer_selem_id_t * sid = NULL;
    snd_mixer_elem_t * elem = NULL;

    bail_on_snd_mixer_error (snd_mixer_open (&handle, 0));
    bail_on_snd_mixer_error (snd_mixer_attach (handle, ap_prc->p_pcm_name_));
    bail_on_snd_mixer_error (snd_mixer_selem_register (handle, NULL, NULL));
    bail_on_snd_mixer_error (snd_mixer_load (handle));

    snd_mixer_selem_id_alloca (&sid);
    snd_mixer_selem_id_set_index (sid, 0);
    snd_mixer_selem_id_set_name (sid, selem_name);
    elem = snd_mixer_find_selem (handle, sid);

    if (!elem)
      {
        TIZ_ERROR (handleOf (ap_prc),
                   "[OMX_ErrorInsufficientResources] : "
                   "Unable to find mixer simple element.");
        goto end;
      }

    bail_on_snd_mixer_error (
      snd_mixer_selem_get_playback_volume_range (elem, &min, &max));
    bail_on_snd_mixer_error (
      snd_mixer_selem_set_playback_volume_all (elem, a_volume * max / 100));

    bail_on_snd_mixer_error (snd_mixer_close (handle));

    /* Everything went well */
    rc = true;
  }

end:

  return rc;
}

static void
toggle_mute (ar_prc_t * ap_prc, const bool a_mute)
{
  assert (ap_prc);

  if (!using_null_alsa_device (ap_prc))
    {
      long new_volume = (a_mute ? 0 : ap_prc->volume_);
      TIZ_TRACE (handleOf (ap_prc), "new volume = %ld - ap_prc->volume_ [%d]",
                 new_volume, ap_prc->volume_);
      set_alsa_master_volume (ap_prc, new_volume);
    }
}

static void
set_volume (ar_prc_t * ap_prc, const long a_volume)
{
  if (!using_null_alsa_device (ap_prc))
    {
      if (set_alsa_master_volume (ap_prc, a_volume))
        {
          assert (ap_prc);
          ap_prc->volume_ = a_volume;
          TIZ_TRACE (handleOf (ap_prc), "ap_prc->volume_ = %ld",
                     ap_prc->volume_);
        }
    }
}

static void
prepare_volume_ramp (ar_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->ramp_enabled_)
    {
      ap_prc->ramp_volume_ = ARATELIA_AUDIO_RENDERER_DEFAULT_VOLUME_VALUE;
      ap_prc->ramp_step_count_
        = ARATELIA_AUDIO_RENDERER_DEFAULT_RAMP_STEP_COUNT;
      ap_prc->ramp_step_
        = (double) ap_prc->ramp_volume_ / (double) ap_prc->ramp_step_count_;
    }
}

static OMX_ERRORTYPE
start_volume_ramp (ar_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->ramp_enabled_)
    {
      assert (ap_prc->p_vol_ramp_timer_);
      ap_prc->ramp_volume_ = 0;
      tiz_check_omx (tiz_srv_timer_watcher_start (
        ap_prc, ap_prc->p_vol_ramp_timer_, 0.2, 0.2));
    }
  return OMX_ErrorNone;
}

static void
stop_volume_ramp (ar_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->ramp_enabled_)
    {
      if (ap_prc->p_vol_ramp_timer_)
        {
          (void) tiz_srv_timer_watcher_stop (ap_prc, ap_prc->p_vol_ramp_timer_);
        }
    }
}

static OMX_ERRORTYPE
start_eos_timer (ar_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  snd_pcm_sframes_t avail = 0;
  snd_pcm_sframes_t delay = 0;

  assert (ap_prc);
  assert (ap_prc->p_eos_timer_);
  assert (ap_prc->p_pcm_);

  if ((ap_prc->nflags_ & OMX_BUFFERFLAG_EOS) != 0)
    {
      bail_on_snd_pcm_error (
        snd_pcm_avail_delay (ap_prc->p_pcm_, &avail, &delay));
      rc = tiz_srv_timer_watcher_start (
        ap_prc, ap_prc->p_eos_timer_,
        (double) (avail + delay) / (double) ap_prc->pcmmode_.nSamplingRate, 0);
    }
  return rc;
}

static void
stop_eos_timer (ar_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->p_eos_timer_ && (ap_prc->nflags_ & OMX_BUFFERFLAG_EOS) != 0)
    {
      (void) tiz_srv_timer_watcher_stop (ap_prc, ap_prc->p_eos_timer_);
    }
}

static OMX_ERRORTYPE
apply_ramp_step (ar_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->ramp_enabled_)
    {
      if (ap_prc->ramp_step_count_-- > 0)
        {
          ap_prc->ramp_volume_ += ap_prc->ramp_step_;
          set_volume (ap_prc, ap_prc->ramp_volume_);
        }
      else
        {
          stop_volume_ramp (ap_prc);
        }
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
arrange_samples_buffer (ar_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_hdr,
                        unsigned long int a_sample_size,
                        unsigned long int a_step,
                        snd_pcm_uframes_t a_samples_per_channel,
                        const void ** app_buffer)
{
  OMX_U8 * p_hdr_buf = NULL;
  assert (ap_prc);
  assert (app_buffer);
  assert (ap_hdr);
  assert (a_samples_per_channel > 0);

  p_hdr_buf = ap_hdr->pBuffer + ap_hdr->nOffset;

  if (ap_prc->pcmmode_.nChannels < ap_prc->num_channels_supported_)
    {
      snd_pcm_uframes_t i = 0;
      tiz_buffer_clear (ap_prc->p_sample_buf_);
      while (i < a_samples_per_channel)
        {
          int j = 0;
          while (j < ap_prc->num_channels_supported_)
            {
              if (a_sample_size
                  != tiz_buffer_push (ap_prc->p_sample_buf_,
                                      p_hdr_buf + (a_step * i), a_sample_size))
                {
                  TIZ_ERROR (handleOf (ap_prc),
                             "Unable to copy all sample data into the buffer");
                  /* Early return */
                  return OMX_ErrorInsufficientResources;
                }
              j += 1;
            }
          i += 1;
        }
      *app_buffer = tiz_buffer_get (ap_prc->p_sample_buf_);
      TIZ_DEBUG (
        handleOf (ap_prc),
        "a_samples_per_channel [%u] tiz_buffer [%p] avail [%d] offset [%d]",
        a_samples_per_channel, *app_buffer,
        tiz_buffer_available (ap_prc->p_sample_buf_),
        tiz_buffer_offset (ap_prc->p_sample_buf_));
    }
  else
    {
      *app_buffer = p_hdr_buf;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
render_buffer (ar_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  unsigned long int sample_size = 0;
  unsigned long int step = 0;
  snd_pcm_uframes_t samples_per_channel = 0;

  assert (ap_prc);
  assert (ap_hdr);

  sample_size = ap_prc->pcmmode_.nBitPerSample / 8;
  step = sample_size * ap_prc->pcmmode_.nChannels;
  assert (ap_hdr->nFilledLen > 0);
  samples_per_channel = ap_hdr->nFilledLen / step;

  adjust_gain (ap_prc, ap_hdr, samples_per_channel);
  swap_byte_order (ap_prc, ap_hdr);

  while (samples_per_channel > 0 && OMX_ErrorNone == rc)
    {
      const void * p_buffer = NULL;
      snd_pcm_sframes_t err = 0;

      tiz_check_omx (arrange_samples_buffer (ap_prc, ap_hdr, sample_size, step,
                                             samples_per_channel, &p_buffer));

      err = snd_pcm_writei (ap_prc->p_pcm_, p_buffer, samples_per_channel);

      if (-EAGAIN == err)
        {
          /* got -EAGAIN, alsa buffers are full */
          rc = OMX_ErrorNoMore;
        }
      else if (err < 0)
        {
          /* This should handle -EINTR (interrupted system call), -EPIPE
           * (overrun or underrun) and -ESTRPIPE (stream is suspended) */
          err = snd_pcm_recover (ap_prc->p_pcm_, (int) err, 0);
          if (err < 0)
            {
              TIZ_ERROR (handleOf (ap_prc), "snd_pcm_recover error: %s",
                         snd_strerror ((int) err));
              rc = OMX_ErrorUnderflow;
            }
        }
      else
        {
          ap_hdr->nOffset += err * step;
          ap_hdr->nFilledLen -= err * step;
          samples_per_channel -= err;
        }
    }

  return rc;
}

static OMX_BUFFERHEADERTYPE *
get_header (ar_prc_t * ap_prc)
{
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  assert (ap_prc);

  if (!ap_prc->port_disabled_)
    {
      if (!ap_prc->p_inhdr_)
        {
          (void) tiz_krn_claim_buffer (tiz_get_krn (handleOf (ap_prc)),
                                       ARATELIA_AUDIO_RENDERER_PORT_INDEX, 0,
                                       &ap_prc->p_inhdr_);
          if (ap_prc->p_inhdr_)
            {
              TIZ_TRACE (handleOf (ap_prc),
                         "Claimed HEADER [%p]...nFilledLen [%d]",
                         ap_prc->p_inhdr_, ap_prc->p_inhdr_->nFilledLen);
            }
          else
            {
              TIZ_TRACE (handleOf (ap_prc), "No INPUT headers available");
            }
        }
      else
        {
          TIZ_TRACE (handleOf (ap_prc), "Using HEADER [%p]...nFilledLen [%d]",
                     ap_prc->p_inhdr_, ap_prc->p_inhdr_->nFilledLen);
        }
      p_hdr = ap_prc->p_inhdr_;
    }
  return p_hdr;
}

static OMX_ERRORTYPE
buffer_emptied (ar_prc_t * ap_prc)
{
  assert (ap_prc);
  assert (ap_prc->p_inhdr_);
  assert (ap_prc->p_inhdr_->nFilledLen == 0);

  TIZ_TRACE (handleOf (ap_prc), "Releasing HEADER [%p] emptied",
             ap_prc->p_inhdr_);

  if ((ap_prc->p_inhdr_->nFlags & OMX_BUFFERFLAG_EOS) != 0)
    {
      TIZ_TRACE (handleOf (ap_prc), "Received EOS");
      /* Record the fact that EOS shown up. We'll signal it to the client on a
         timer event */
      ap_prc->nflags_ = ap_prc->p_inhdr_->nFlags;
      tiz_check_omx (start_eos_timer (ap_prc));
    }

  return release_header (ap_prc);
}

static OMX_ERRORTYPE
render_pcm_data (ar_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;

  while (OMX_ErrorNone == rc && (p_hdr = get_header (ap_prc)))
    {
      if (p_hdr->nFilledLen > 0)
        {
          rc = render_buffer (ap_prc, p_hdr);
        }

      if (0 == p_hdr->nFilledLen)
        {
          tiz_check_omx (buffer_emptied (ap_prc));
          p_hdr = NULL;
        }
    }

  if (OMX_ErrorNoMore == rc)
    {
      rc = start_io_watcher (ap_prc);
    }

  return rc;
}

/*
 * arprc
 */

static void *
ar_prc_ctor (void * ap_prc, va_list * app)
{
  ar_prc_t * p_prc = super_ctor (typeOf (ap_prc, "arprc"), ap_prc, app);
  p_prc->p_pcm_ = NULL;
  p_prc->p_hw_params_ = NULL;
  p_prc->p_pcm_name_ = NULL;
  p_prc->p_mixer_name_ = NULL;
  p_prc->swap_byte_order_ = false;
  p_prc->num_channels_supported_ = 0;
  p_prc->p_sample_buf_ = NULL;
  p_prc->descriptor_count_ = 0;
  p_prc->p_fds_ = NULL;
  p_prc->p_ev_io_ = NULL;
  p_prc->p_vol_ramp_timer_ = NULL;
  p_prc->p_eos_timer_ = NULL;
  p_prc->p_inhdr_ = NULL;
  p_prc->port_disabled_ = false;
  p_prc->awaiting_io_ev_ = false;
  p_prc->nflags_ = 0;
  p_prc->gain_ = ARATELIA_AUDIO_RENDERER_DEFAULT_GAIN_VALUE;
  p_prc->volume_ = ARATELIA_AUDIO_RENDERER_DEFAULT_VOLUME_VALUE;
  p_prc->ramp_enabled_ = false;
  p_prc->ramp_step_ = 0;
  p_prc->ramp_step_count_ = ARATELIA_AUDIO_RENDERER_DEFAULT_RAMP_STEP_COUNT;
  p_prc->ramp_volume_ = 0;
  return p_prc;
}

static void *
ar_prc_dtor (void * ap_prc)
{
  (void) ar_prc_deallocate_resources (ap_prc);
  return super_dtor (typeOf (ap_prc, "arprc"), ap_prc);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
ar_prc_allocate_resources (void * ap_prc, OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t * p_prc = ap_prc;

  assert (p_prc);

  tiz_check_omx (tiz_buffer_init (
    &p_prc->p_sample_buf_, ARATELIA_AUDIO_RENDERER_PORT_MIN_BUF_SIZE * 2));

  snd_lib_error_set_handler (alsa_error_handler);

  if (!p_prc->p_pcm_)
    {
      char * p_device = get_alsa_device (p_prc);
      assert (p_device);

      /* Open a PCM in non-blocking mode */
      bail_on_snd_pcm_error (snd_pcm_open (
        &p_prc->p_pcm_, p_device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK));
      /* Allocate alsa's hardware parameter structure */
      bail_on_snd_pcm_error (snd_pcm_hw_params_malloc (&p_prc->p_hw_params_));

      /* Get the alsa descriptors count */
      p_prc->descriptor_count_ = snd_pcm_poll_descriptors_count (p_prc->p_pcm_);
      if (p_prc->descriptor_count_ <= 0)
        {
          TIZ_ERROR (handleOf (p_prc),
                     "[OMX_ErrorInsufficientResources] : "
                     "Invalid poll descriptors count");
          return OMX_ErrorInsufficientResources;
        }

      /* Allocate space for the list of alsa fds */
      p_prc->p_fds_
        = tiz_mem_alloc (sizeof (struct pollfd) * p_prc->descriptor_count_);
      tiz_check_null_ret_oom (p_prc->p_fds_);

      /* This is to generate volume ramps when needed */
      if (p_prc->ramp_enabled_)
        {
          tiz_check_omx (
            tiz_srv_timer_watcher_init (p_prc, &(p_prc->p_vol_ramp_timer_)));
        }

      /* This is to produce accurate EOS flag events */
      tiz_check_omx (
        tiz_srv_timer_watcher_init (p_prc, &(p_prc->p_eos_timer_)));
    }

  assert (p_prc->p_pcm_);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_prepare_to_transfer (void * ap_prc, OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t * p_prc = ap_prc;
  assert (p_prc);
  p_prc->nflags_ = 0;

  if (p_prc->p_pcm_)
    {
      snd_pcm_format_t snd_pcm_format;

      p_prc->swap_byte_order_ = false;
      p_prc->num_channels_supported_ = 0;

      log_alsa_pcm_state (p_prc);

      /* Fill params with a full configuration space for the PCM. */
      bail_on_snd_pcm_error (
        snd_pcm_hw_params_any (p_prc->p_pcm_, p_prc->p_hw_params_));

      /* Retrieve pcm params from the alsa pcm device and the omx port */
      tiz_check_omx (retrieve_alsa_pcm_format_and_num_channels (
        p_prc, &snd_pcm_format, &p_prc->num_channels_supported_));

      /* This sets the hardware and software parameters in a convenient way. */
      bail_on_snd_pcm_error (snd_pcm_set_params (
        p_prc->p_pcm_, snd_pcm_format, SND_PCM_ACCESS_RW_INTERLEAVED,
        (unsigned int) p_prc->num_channels_supported_,
        p_prc->pcmmode_.nSamplingRate, 0, /* allow alsa-lib resampling */
        100000                            /* overall latency in us */
        ));

      bail_on_snd_pcm_error (snd_pcm_poll_descriptors (
        p_prc->p_pcm_, p_prc->p_fds_, p_prc->descriptor_count_));

      TIZ_DEBUG (handleOf (p_prc), "Poll descriptors : %d",
                 p_prc->descriptor_count_);

      /* Init the io watcher */
      tiz_check_omx (tiz_srv_io_watcher_init (p_prc, &(p_prc->p_ev_io_),
                                              p_prc->p_fds_->fd,
                                              TIZ_EVENT_READ_OR_WRITE, true));

      /* OK, now prepare the PCM for use */
      bail_on_snd_pcm_error (snd_pcm_prepare (p_prc->p_pcm_));

      /* Internally store the initial volume, so that the internal OMX volume
         struct reflects the current value of ALSA's master volume. */
      if (OMX_ErrorNone != set_initial_component_volume (p_prc))
        {
          /* Volume control might not be supported by the current alsa device,
             not a big deal, simply log a message. */
          TIZ_NOTICE (handleOf (p_prc), "Could not set the component's volume");
        }
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_transfer_and_process (void * ap_prc, OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t * p_prc = ap_prc;
  assert (p_prc);
  log_alsa_pcm_state (p_prc);
  prepare_volume_ramp (p_prc);
  tiz_check_omx (start_volume_ramp (p_prc));
  tiz_check_omx (apply_ramp_step (p_prc));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_stop_and_return (void * ap_prc)
{
  log_alsa_pcm_state (ap_prc);
  stop_volume_ramp (ap_prc);
  stop_eos_timer (ap_prc);
  return do_flush (ap_prc);
}

static OMX_ERRORTYPE
ar_prc_deallocate_resources (void * ap_prc)
{
  ar_prc_t * p_prc = ap_prc;
  assert (p_prc);

  tiz_srv_timer_watcher_destroy (p_prc, p_prc->p_eos_timer_);
  p_prc->p_eos_timer_ = NULL;

  if (p_prc->ramp_enabled_)
    {
      tiz_srv_timer_watcher_destroy (p_prc, p_prc->p_vol_ramp_timer_);
      p_prc->p_vol_ramp_timer_ = NULL;
    }

  p_prc->descriptor_count_ = 0;
  tiz_mem_free (p_prc->p_fds_);
  p_prc->p_fds_ = NULL;

  tiz_srv_io_watcher_destroy (p_prc, p_prc->p_ev_io_);
  p_prc->p_ev_io_ = NULL;

  if (p_prc->p_hw_params_)
    {
      snd_pcm_hw_params_free (p_prc->p_hw_params_);
      (void) snd_pcm_close (p_prc->p_pcm_);
      (void) snd_config_update_free_global ();
      p_prc->p_pcm_ = NULL;
      p_prc->p_hw_params_ = NULL;
    }

  tiz_buffer_destroy (p_prc->p_sample_buf_);
  p_prc->p_sample_buf_ = NULL;

  tiz_mem_free (p_prc->p_pcm_name_);
  p_prc->p_pcm_name_ = NULL;

  tiz_mem_free (p_prc->p_mixer_name_);
  p_prc->p_mixer_name_ = NULL;

  return OMX_ErrorNone;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
ar_prc_buffers_ready (const void * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  ar_prc_t * p_prc = (ar_prc_t *) ap_prc;
  assert (p_prc);
  TIZ_TRACE (handleOf (p_prc),
             "Received buffer ready notification - "
             "awaiting_io_ev [%s]",
             p_prc->awaiting_io_ev_ ? "YES" : "NO");
  if (!p_prc->awaiting_io_ev_)
    {
      rc = render_pcm_data (p_prc);
    }
  return rc;
}

static OMX_ERRORTYPE
ar_prc_io_ready (void * ap_prc, tiz_event_io_t * ap_ev_io, int a_fd,
                 int a_events)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  ar_prc_t * p_prc = ap_prc;
  if (p_prc->awaiting_io_ev_)
    {
      p_prc->awaiting_io_ev_ = false;
      rc = render_pcm_data (ap_prc);
    }
  return rc;
}

static OMX_ERRORTYPE
ar_prc_timer_ready (void * ap_prc, tiz_event_timer_t * ap_ev_timer,
                    void * ap_arg, const uint32_t a_id)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  ar_prc_t * p_prc = ap_prc;
  assert (p_prc);

  if (ap_ev_timer == p_prc->p_eos_timer_
      && (p_prc->nflags_ & OMX_BUFFERFLAG_EOS) != 0)
    {
      p_prc->nflags_ = 0;
      tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventBufferFlag, 0,
                           p_prc->nflags_, NULL);
    }
  else if (ap_ev_timer == p_prc->p_vol_ramp_timer_)
    {
      rc = apply_ramp_step (ap_prc);
    }
  else
    {
      assert (0);
    }
  return rc;
}

static OMX_ERRORTYPE
ar_prc_pause (const void * ap_prc)
{
  ar_prc_t * p_prc = (ar_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int pause = 1;
  assert (p_prc);
  log_alsa_pcm_state (p_prc);
  stop_io_watcher (p_prc);
  stop_eos_timer (p_prc);
  if (snd_pcm_hw_params_can_pause (p_prc->p_hw_params_))
    {
      bail_on_snd_pcm_error (snd_pcm_pause (p_prc->p_pcm_, pause));
    }
  else
    {
      bail_on_snd_pcm_error (snd_pcm_drop (p_prc->p_pcm_));
    }
  return rc;
}

static OMX_ERRORTYPE
ar_prc_resume (const void * ap_prc)
{
  ar_prc_t * p_prc = (ar_prc_t *) ap_prc;
  int resume = 0;
  assert (p_prc);
  start_eos_timer (p_prc);
  log_alsa_pcm_state (p_prc);
  if (snd_pcm_hw_params_can_pause (p_prc->p_hw_params_))
    {
      bail_on_snd_pcm_error (snd_pcm_pause (p_prc->p_pcm_, resume));
    }
  else
    {
      bail_on_snd_pcm_error (snd_pcm_prepare (p_prc->p_pcm_));
    }
  tiz_check_omx (start_io_watcher (p_prc));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_port_flush (const void * ap_prc, OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t * p_prc = (ar_prc_t *) ap_prc;
  return do_flush (p_prc);
}

static OMX_ERRORTYPE
ar_prc_port_disable (const void * ap_prc, OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t * p_prc = (ar_prc_t *) ap_prc;
  assert (p_prc);
  log_alsa_pcm_state (p_prc);
  stop_volume_ramp (p_prc);
  p_prc->port_disabled_ = true;
  if (p_prc->p_pcm_)
    {
      /* Try to drain the PCM...*/
      if (snd_pcm_drain (p_prc->p_pcm_))
        {
          /* ... or else drop all samples. */
          (void) snd_pcm_drop (p_prc->p_pcm_);
        }
      stop_io_watcher (p_prc);
      stop_eos_timer (p_prc);
    }
  /* Release any buffers held  */
  return release_header ((ar_prc_t *) ap_prc);
}

static OMX_ERRORTYPE
ar_prc_port_enable (const void * ap_prc, OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t * p_prc = (ar_prc_t *) ap_prc;
  assert (p_prc);
  log_alsa_pcm_state (p_prc);
  p_prc->port_disabled_ = false;
  if (p_prc->p_pcm_)
    {
      tiz_check_omx (ar_prc_prepare_to_transfer (p_prc, OMX_ALL));
      tiz_check_omx (ar_prc_transfer_and_process (p_prc, OMX_ALL));
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_config_change (void * ap_prc, OMX_U32 a_pid, OMX_INDEXTYPE a_config_idx)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  ar_prc_t * p_prc = ap_prc;

  assert (ap_prc);

  if (ARATELIA_AUDIO_RENDERER_PORT_INDEX == a_pid)
    {
      if (OMX_IndexConfigAudioVolume == a_config_idx)
        {
          OMX_AUDIO_CONFIG_VOLUMETYPE volume;
          TIZ_INIT_OMX_PORT_STRUCT (volume, ARATELIA_AUDIO_RENDERER_PORT_INDEX);
          tiz_check_omx (
            tiz_api_GetConfig (tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
                               OMX_IndexConfigAudioVolume, &volume));
          TIZ_TRACE (
            handleOf (p_prc),
            "[OMX_IndexConfigAudioVolume] : volume.sVolume.nValue = %ld",
            volume.sVolume.nValue);
          if (volume.sVolume.nValue <= ARATELIA_AUDIO_RENDERER_MAX_VOLUME_VALUE
              && volume.sVolume.nValue
                   >= ARATELIA_AUDIO_RENDERER_MIN_VOLUME_VALUE)
            {
              /* TODO: Volume should be done by adjusting the gain, not ALSA's
               * master volume! */
              set_volume (p_prc, volume.sVolume.nValue);
            }
        }
      else if (OMX_IndexConfigAudioMute == a_config_idx)
        {
          OMX_AUDIO_CONFIG_MUTETYPE mute;
          TIZ_INIT_OMX_PORT_STRUCT (mute, ARATELIA_AUDIO_RENDERER_PORT_INDEX);
          tiz_check_omx (tiz_api_GetConfig (tiz_get_krn (handleOf (p_prc)),
                                            handleOf (p_prc),
                                            OMX_IndexConfigAudioMute, &mute));
          /* TODO: Volume should be done by adjusting the gain, not ALSA's
           * master volume! */
          TIZ_TRACE (handleOf (p_prc),
                     "[OMX_IndexConfigAudioMute] : bMute = [%s]",
                     (mute.bMute == OMX_FALSE ? "FALSE" : "TRUE"));
          toggle_mute (p_prc, mute.bMute == OMX_TRUE ? true : false);
        }
    }
  return rc;
}

/*
 * ar_prc_class
 */

static void *
ar_prc_class_ctor (void * ap_prc, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_prc, "arprc_class"), ap_prc, app);
}

/*
 * initialization
 */

void *
ar_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * arprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "arprc_class", classOf (tizprc), sizeof (ar_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, ar_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value */
     0);
  return arprc_class;
}

void *
ar_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * arprc_class = tiz_get_type (ap_hdl, "arprc_class");
  TIZ_LOG_CLASS (arprc_class);
  void * arprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (arprc_class, "arprc", tizprc, sizeof (ar_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, ar_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, ar_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, ar_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, ar_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, ar_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, ar_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, ar_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_io_ready, ar_prc_io_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_timer_ready, ar_prc_timer_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, ar_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_pause, ar_prc_pause,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_resume, ar_prc_resume,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, ar_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, ar_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, ar_prc_port_enable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_config_change, ar_prc_config_change,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return arprc;
}
