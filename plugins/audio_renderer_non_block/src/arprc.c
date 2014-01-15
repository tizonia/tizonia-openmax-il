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

#include "ar.h"
#include "arprc.h"
#include "arprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizosal.h"

#include <assert.h>
#include <errno.h>
#include <math.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.audio_renderer.prc"
#endif

/* This is for convenience. This macro assumes the existence of an "
   "ap_prc" local variable */
#define bail_on_alsa_error(expr)                                        \
  do {                                                                  \
    int alsa_error = 0;                                                 \
    if (0 != (alsa_error = (expr)))                                     \
      {                                                                 \
        OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;              \
        TIZ_ERROR (handleOf (ap_prc),                                   \
                   "[%s] Error while accessing alsa master volume (%s)", \
                   tiz_err_to_str (rc), snd_strerror (alsa_error));     \
        goto end;                                                       \
      }                                                                 \
  } while (0)

/*@null@*/ static char *
get_alsa_device (ar_prc_t * ap_prc)
{
  const char *p_alsa_pcm = NULL;

  assert (NULL != ap_prc);
  assert (NULL == ap_prc->p_alsa_pcm_);

  p_alsa_pcm
    = tiz_rcfile_get_value ("plugins-data",
                            "OMX.Aratelia.audio_renderer_nb.pcm.alsa_device");

  if (NULL != p_alsa_pcm)
    {
      TIZ_TRACE (handleOf (ap_prc), "Using ALSA pcm [%s]...", p_alsa_pcm);
      ap_prc->p_alsa_pcm_ = strndup (p_alsa_pcm, OMX_MAX_STRINGNAME_SIZE);
    }
  else
    {
      TIZ_TRACE (handleOf (ap_prc),
                 "No alsa device found in config file. Using [%s]...",
                 ARATELIA_AUDIO_RENDERER_DEFAULT_ALSA_DEVICE);
    }

  return (NULL != ap_prc->p_alsa_pcm_) ? ap_prc->p_alsa_pcm_
    : ARATELIA_AUDIO_RENDERER_DEFAULT_ALSA_DEVICE;
}

static inline OMX_ERRORTYPE
start_io_watcher (ar_prc_t * ap_prc)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_ev_io_);
  ap_prc->awaiting_io_ev_ = true;
  return tiz_event_io_start (ap_prc->p_ev_io_);
}

static inline OMX_ERRORTYPE
stop_io_watcher (ar_prc_t * ap_prc)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_ev_io_);
  ap_prc->awaiting_io_ev_ = false;
  return tiz_event_io_stop (ap_prc->p_ev_io_);
}

static OMX_ERRORTYPE
release_buffers (ar_prc_t * ap_prc)
{
  assert (NULL != ap_prc);

  stop_io_watcher (ap_prc);

  if (ap_prc->p_inhdr_)
    {
      void *p_krn = tiz_get_krn (handleOf (ap_prc));
      tiz_check_omx_err (tiz_krn_release_buffer (p_krn, 0, ap_prc->p_inhdr_));
      ap_prc->p_inhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE
do_flush (ar_prc_t * ap_prc)
{
  assert (NULL != ap_prc);
  tiz_check_omx_err (stop_io_watcher (ap_prc));
  (void) snd_pcm_drop (ap_prc->p_pcm_hdl);
  /* Release any buffers held  */
  return release_buffers (ap_prc);
}

static float
sint_to_float (const int sample)
{
  float f;
  if (sample >= 0)
    f = sample / 32767.0;
  else
    f = sample / 32768.0;
  return f;
}

static int
float_to_sint (const float sample)
{
  float f;
  f = sample * 32767;
  if (f < -32768)
    f = -32768;
  if (f > 32767)
    f = 32767;
  return (int) f;
}

static void
adjust_gain (const ar_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_hdr,
             const snd_pcm_uframes_t a_num_samples)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_hdr);

  if (ARATELIA_AUDIO_RENDERER_DEFAULT_GAIN_VALUE != ap_prc->gain_)
    {
      int i;
      int gainadj = (int) (ap_prc->gain_ * 256.);
      float gain = pow (10., gainadj / 5120.);
      /*       gain = 3.1f; */
      /*       fprintf (stderr, "%f samples %ld\n", gain, a_num_samples); */
      OMX_S16 *pcm = (OMX_S16 *) (ap_hdr->pBuffer + ap_hdr->nOffset);
      for (i = 0; i < a_num_samples; i++)
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

static OMX_ERRORTYPE
get_alsa_master_volume (const ar_prc_t * ap_prc, long *ap_volume)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != ap_prc);
  assert (NULL != ap_volume);

  {
    const char *selem_name = "Master";
    long min, max, volume;
    snd_mixer_t *handle = NULL;
    snd_mixer_selem_id_t *sid = NULL;
    snd_mixer_elem_t *elem = NULL;

    rc = OMX_ErrorInsufficientResources;
    bail_on_alsa_error (snd_mixer_open (&handle, 0));
    bail_on_alsa_error (snd_mixer_attach (handle, ap_prc->p_alsa_pcm_));
    bail_on_alsa_error (snd_mixer_selem_register (handle, NULL, NULL));
    bail_on_alsa_error (snd_mixer_load (handle));

    snd_mixer_selem_id_alloca (&sid);
    snd_mixer_selem_id_set_index (sid, 0);
    snd_mixer_selem_id_set_name (sid, selem_name);
    elem = snd_mixer_find_selem (handle, sid);

    bail_on_alsa_error (snd_mixer_selem_get_playback_volume_range
                        (elem, &min, &max));
    bail_on_alsa_error (snd_mixer_selem_get_playback_volume
                        (elem, SND_MIXER_SCHN_FRONT_LEFT, &volume));

    *ap_volume = volume * 100 / max;
    bail_on_alsa_error (snd_mixer_close (handle));

    /* Everything well, restore the error code. */
    rc = OMX_ErrorNone;
  }
end:

  return rc;
}

static OMX_ERRORTYPE
set_component_volume (ar_prc_t * ap_prc)
{
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;

  assert (NULL != ap_prc);

  tiz_check_omx_err (get_alsa_master_volume (ap_prc, &(ap_prc->volume_)));

  volume.nSize = sizeof (OMX_AUDIO_CONFIG_VOLUMETYPE);
  volume.nVersion.nVersion = OMX_VERSION;
  volume.nPortIndex = 0;

  tiz_check_omx_err (tiz_api_GetConfig (tiz_get_krn (handleOf (ap_prc)),
                                        handleOf (ap_prc),
                                        OMX_IndexConfigAudioVolume, &volume));

  volume.sVolume.nValue = ap_prc->volume_;

  tiz_check_omx_err (tiz_api_SetConfig (tiz_get_krn (handleOf (ap_prc)),
                                        handleOf (ap_prc),
                                        OMX_IndexConfigAudioVolume, &volume));

  return OMX_ErrorNone;
}

static bool
set_alsa_master_volume (const ar_prc_t * ap_prc, const long volume)
{
  bool rc = false;
  assert (NULL != ap_prc);

  {
    const char *selem_name = "Master";
    long min, max;
    snd_mixer_t *handle = NULL;
    snd_mixer_selem_id_t *sid = NULL;
    snd_mixer_elem_t *elem = NULL;

    TIZ_TRACE (handleOf (ap_prc), "volume = %ld\n", volume);
    bail_on_alsa_error (snd_mixer_open (&handle, 0));
    bail_on_alsa_error (snd_mixer_attach (handle, ap_prc->p_alsa_pcm_));
    bail_on_alsa_error (snd_mixer_selem_register (handle, NULL, NULL));
    bail_on_alsa_error (snd_mixer_load (handle));

    snd_mixer_selem_id_alloca (&sid);
    snd_mixer_selem_id_set_index (sid, 0);
    snd_mixer_selem_id_set_name (sid, selem_name);
    elem = snd_mixer_find_selem (handle, sid);

    bail_on_alsa_error (snd_mixer_selem_get_playback_volume_range
                        (elem, &min, &max));
    bail_on_alsa_error (snd_mixer_selem_set_playback_volume_all
                        (elem, volume * max / 100));

    bail_on_alsa_error (snd_mixer_close (handle));

    /* Everything went well */
    rc = true;
  }

end:

  return rc;
}

static void
toggle_mute (ar_prc_t * ap_prc, const bool a_mute)
{
  assert (NULL != ap_prc);
  {
    long new_volume = (a_mute ? 0 : ap_prc->volume_);
    TIZ_TRACE (handleOf (ap_prc), "volume = %ld\n", new_volume);
    set_alsa_master_volume (ap_prc, new_volume);
  }
}

static void
set_volume (ar_prc_t * ap_prc, const long volume)
{
  if (set_alsa_master_volume (ap_prc, volume))
    {
      assert (NULL != ap_prc);
      ap_prc->volume_ = volume;
    }
}

static OMX_ERRORTYPE
render_buffer (ar_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  snd_pcm_sframes_t err = 0;
  snd_pcm_uframes_t samples = 0;
  unsigned long int step = 0;

  assert (NULL != ap_prc);
  assert (NULL != ap_hdr);

  TIZ_TRACE (handleOf (ap_prc),
             "Rendering HEADER [%p]...nFilledLen[%d] nOffset [%d]!!!", ap_hdr,
             ap_hdr->nFilledLen, ap_hdr->nOffset);

  step = (ap_prc->pcmmode.nBitPerSample / 8) * ap_prc->pcmmode.nChannels;
  assert (ap_hdr->nFilledLen > 0);
  samples = ap_hdr->nFilledLen / step;
  TIZ_TRACE (handleOf (ap_prc),
             "step [%d], samples [%d] nFilledLen [%d]",
             step, samples, ap_hdr->nFilledLen);

  adjust_gain (ap_prc, ap_hdr, samples);

  while (samples > 0)
    {
      err = snd_pcm_writei (ap_prc->p_pcm_hdl,
                            ap_hdr->pBuffer + ap_hdr->nOffset, samples);
      TIZ_TRACE (handleOf (ap_prc),
                 "Rendering HEADER [%p]...err [%d] samples [%d] nOffset [%d]",
                 ap_hdr, err, samples, ap_hdr->nOffset);

      if (-EAGAIN == err)
        {
          TIZ_ERROR (handleOf (ap_prc),
                     "Ring buffer must be full (got -EAGAIN)");
          return start_io_watcher (ap_prc);
        }

      if (err < 0)
        {
          /* This should handle -EINTR (interrupted system call), -EPIPE
           * (overrun or underrun) and -ESTRPIPE (stream is suspended) */
          TIZ_ERROR (handleOf (ap_prc),
                     "Trying to recover the stream state...");
          err = snd_pcm_recover (ap_prc->p_pcm_hdl, (int) err, 0);
          if (err < 0)
            {
              TIZ_ERROR (handleOf (ap_prc),
                         "snd_pcm_recover error: %s",
                         snd_strerror ((int) err));
              break;
            }
        }
      ap_hdr->nOffset += err * step;
      ap_hdr->nFilledLen -= err * step;
      samples -= err;
    }

  ap_hdr->nFilledLen = 0;

  return OMX_ErrorNone;
}

static OMX_BUFFERHEADERTYPE *
buffer_needed (ar_prc_t * ap_prc)
{
  assert (NULL != ap_prc);

  if (false == ap_prc->port_disabled_)
    {
      if (NULL != ap_prc->p_inhdr_ && ap_prc->p_inhdr_->nFilledLen > 0)
        {
          return ap_prc->p_inhdr_;
        }
      else
        {
          tiz_pd_set_t ports;
          void *p_krn = NULL;

          p_krn = tiz_get_krn (handleOf (ap_prc));

          TIZ_PD_ZERO (&ports);
          if (OMX_ErrorNone == tiz_krn_select (p_krn, 1, &ports))
            {
              if (TIZ_PD_ISSET (0, &ports))
                {
                  if (OMX_ErrorNone == tiz_krn_claim_buffer
                      (p_krn, 0, 0, &ap_prc->p_inhdr_))
                    {
                      TIZ_TRACE (handleOf (ap_prc),
                                 "Claimed HEADER [%p]...nFilledLen [%d]",
                                 ap_prc->p_inhdr_,
                                 ap_prc->p_inhdr_->nFilledLen);
                      return ap_prc->p_inhdr_;
                    }
                }
            }
        }
    }

  ap_prc->awaiting_buffers_ = true;
  return NULL;
}

static OMX_ERRORTYPE
buffer_emptied (ar_prc_t * ap_prc)
{
  assert (NULL != ap_prc);
  assert (ap_prc->p_inhdr_->nFilledLen == 0);

  TIZ_TRACE (handleOf (ap_prc), "HEADER [%p] emptied", ap_prc->p_inhdr_);

  ap_prc->p_inhdr_->nOffset = 0;

  if ((ap_prc->p_inhdr_->nFlags & OMX_BUFFERFLAG_EOS) != 0)
    {
      TIZ_TRACE (handleOf (ap_prc),
                 "OMX_BUFFERFLAG_EOS in HEADER [%p]", ap_prc->p_inhdr_);
      tiz_srv_issue_event ((OMX_PTR) ap_prc,
                           OMX_EventBufferFlag, 0,
                           ap_prc->p_inhdr_->nFlags, NULL);
    }

  tiz_check_omx_err (tiz_krn_release_buffer (tiz_get_krn
                                             (handleOf (ap_prc)),
                                             0, ap_prc->p_inhdr_));
  ap_prc->p_inhdr_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
render_pcm_data (ar_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  while ((NULL != (p_hdr = buffer_needed (ap_prc)))
         && ap_prc->awaiting_io_ev_ == false)
    {
      if (p_hdr->nFilledLen > 0)
        {
          tiz_check_omx_err (render_buffer (ap_prc, p_hdr));
        }

      TIZ_TRACE (handleOf (ap_prc),
                 "awaiting_io_ev_ [%s]",
                 ap_prc->awaiting_io_ev_ ? "YES" : "NO");

      if (0 == p_hdr->nFilledLen)
        {
          if (OMX_ErrorNone != (rc = buffer_emptied (ap_prc)))
            {
              TIZ_ERROR (handleOf (ap_prc),
                         "[%s] Error while returning bufffer",
                         tiz_err_to_str (rc));
              return rc;
            }
        }
    }

  return OMX_ErrorNone;
}

/*
 * arprc
 */

static void *
ar_prc_ctor (void *ap_obj, va_list * app)
{
  ar_prc_t *p_obj = super_ctor (typeOf (ap_obj, "arprc"), ap_obj, app);
  p_obj->p_pcm_hdl = NULL;
  p_obj->p_hw_params = NULL;
  p_obj->p_alsa_pcm_ = NULL;
  p_obj->descriptor_count_ = 0;
  p_obj->p_fds_ = NULL;
  p_obj->p_ev_io_ = NULL;
  p_obj->p_inhdr_ = NULL;
  p_obj->port_disabled_ = false;
  p_obj->awaiting_buffers_ = true;
  p_obj->awaiting_io_ev_ = false;
  p_obj->gain_ = ARATELIA_AUDIO_RENDERER_DEFAULT_GAIN_VALUE;
  p_obj->volume_ = ARATELIA_AUDIO_RENDERER_DEFAULT_VOLUME_VALUE;
  return p_obj;
}

static void *
ar_prc_dtor (void *ap_obj)
{
  ar_prc_t *p_obj = ap_obj;

  tiz_mem_free (p_obj->p_alsa_pcm_);

  if (NULL != p_obj->p_hw_params)
    {
      snd_pcm_hw_params_free (p_obj->p_hw_params);
    }

  if (NULL != p_obj->p_pcm_hdl)
    {
      (void) snd_pcm_close (p_obj->p_pcm_hdl);
    }

  tiz_mem_free (p_obj->p_fds_);

  if (NULL != p_obj->p_ev_io_)
    {
      tiz_event_io_destroy (p_obj->p_ev_io_);
    }

  return super_dtor (typeOf (ap_obj, "arprc"), ap_obj);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
ar_prc_allocate_resources (void *ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t *p_prc = ap_obj;
  OMX_HANDLETYPE p_hdl = handleOf (p_prc);
  int err = 0;

  assert (NULL != ap_obj);

  if (NULL == p_prc->p_pcm_hdl)
    {
      OMX_ERRORTYPE rc = OMX_ErrorNone;
      char *p_device = get_alsa_device (p_prc);
      assert (NULL != p_device);

      /* Open a PCM in non-blocking mode */
      if ((err =
           snd_pcm_open (&p_prc->p_pcm_hdl, p_device,
                         SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0)
        {
          TIZ_ERROR (p_hdl, "[OMX_ErrorInsufficientResources] : "
                     "cannot open audio device %s (%s)",
                     p_device,
                     snd_strerror (err));
          return OMX_ErrorInsufficientResources;
        }

      if ((err = snd_pcm_hw_params_malloc (&p_prc->p_hw_params)) < 0)
        {
          TIZ_ERROR (p_hdl, "[OMX_ErrorInsufficientResources] : "
                     "cannot allocate hardware parameter structure" " (%s)",
                     snd_strerror (err));
          return OMX_ErrorInsufficientResources;
        }

      p_prc->descriptor_count_ =
        snd_pcm_poll_descriptors_count (p_prc->p_pcm_hdl);
      if (p_prc->descriptor_count_ <= 0)
        {
          TIZ_ERROR (p_hdl, "[OMX_ErrorInsufficientResources] : "
                     "Invalid poll descriptors count");
          return OMX_ErrorInsufficientResources;
        }

      p_prc->p_fds_ =
        tiz_mem_alloc (sizeof (struct pollfd) * p_prc->descriptor_count_);
      if (p_prc->p_fds_ == NULL)
        {
          TIZ_ERROR (p_hdl, "[OMX_ErrorInsufficientResources] : "
                     "Could not allocate poll file descriptors\n");
          return OMX_ErrorInsufficientResources;
        }

      if (OMX_ErrorNone !=
          (rc = tiz_event_io_init (&(p_prc->p_ev_io_), p_hdl,
                                   tiz_comp_event_io)))
        {
          TIZ_ERROR (p_hdl, "[OMX_ErrorInsufficientResources] : "
                     "Error initializing the PCM io event (was %s)",
                     tiz_err_to_str (rc));
          return OMX_ErrorInsufficientResources;
        }
    }

  assert (NULL != p_prc->p_pcm_hdl);

  /* Fill params with a full configuration space for the PCM. */
  if ((err = snd_pcm_hw_params_any (p_prc->p_pcm_hdl, p_prc->p_hw_params)) < 0)
    {
      TIZ_ERROR (p_hdl, "[OMX_ErrorInsufficientResources] : "
                 "cannot initialize hardware parameter structure (%s)",
                 snd_strerror (err));
      return OMX_ErrorInsufficientResources;
    }

  TIZ_DEBUG (p_hdl, "Device can pause [%s] ",
             snd_pcm_hw_params_can_pause (p_prc->p_hw_params) == 0 ?
             "NO" : "YES");

  TIZ_DEBUG (p_hdl, "Device can resume [%s] ",
             snd_pcm_hw_params_can_resume (p_prc->p_hw_params) == 0 ?
             "NO" : "YES");

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_prepare_to_transfer (void *ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t *p_prc = ap_obj;

  assert (NULL != ap_obj);

  if (NULL != p_prc->p_pcm_hdl)
    {
      OMX_ERRORTYPE rc = OMX_ErrorNone;
      snd_pcm_format_t snd_pcm_format;
      OMX_HANDLETYPE p_hdl = handleOf (p_prc);
      void *p_krn = tiz_get_krn (p_hdl);
      int err = 0;
      unsigned int snd_sampling_rate = 0;

      /* Retrieve pcm params from port */
      p_prc->pcmmode.nSize = (OMX_U32) sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
      p_prc->pcmmode.nVersion.nVersion = (OMX_U32) OMX_VERSION;
      p_prc->pcmmode.nPortIndex = 0;    /* port index */
      if (OMX_ErrorNone != (rc = tiz_api_GetParameter
                            (p_krn, p_hdl,
                             OMX_IndexParamAudioPcm, &p_prc->pcmmode)))
        {
          TIZ_ERROR (p_hdl, "[%s] : Error retrieving "
                     "pcm params from port", tiz_err_to_str (rc));
          return rc;
        }

      TIZ_NOTICE (p_hdl, "nChannels = [%d] nBitPerSample = [%d] "
                  "nSamplingRate = [%d] eNumData = [%d] eEndian = [%d] "
                  "bInterleaved = [%s] ePCMMode = [%d]",
                  p_prc->pcmmode.nChannels,
                  p_prc->pcmmode.nBitPerSample,
                  p_prc->pcmmode.nSamplingRate,
                  p_prc->pcmmode.eNumData,
                  p_prc->pcmmode.eEndian,
                  p_prc->pcmmode.bInterleaved ==
                  OMX_TRUE ? "OMX_TRUE" : "OMX_FALSE",
                  p_prc->pcmmode.ePCMMode);

      /* TODO : Add function to properly encode snd_pcm_format */
      snd_pcm_format = p_prc->pcmmode.eEndian == OMX_EndianLittle ?
        SND_PCM_FORMAT_S16 : SND_PCM_FORMAT_S16_BE;

      snd_sampling_rate = p_prc->pcmmode.bInterleaved == OMX_TRUE ?
        p_prc->pcmmode.nSamplingRate : p_prc->pcmmode.nSamplingRate * 2;

      /* This sets the hardware and software parameters in a convenient way. */
      if ((err = snd_pcm_set_params (p_prc->p_pcm_hdl,
                                     snd_pcm_format, SND_PCM_ACCESS_RW_INTERLEAVED,
                                     (unsigned int) p_prc->pcmmode.nChannels,
                                     snd_sampling_rate, 1,    /* allow alsa-lib resampling */
                                     100000     /* overall latency in us */
           )) < 0)
        {
          TIZ_ERROR (p_hdl, "[OMX_ErrorInsufficientResources] : "
                     "Could not set the PCM params (%s)!",
                     snd_strerror ((int) err));
          return OMX_ErrorInsufficientResources;
        }

      if ((err = snd_pcm_poll_descriptors (p_prc->p_pcm_hdl, p_prc->p_fds_,
                                           p_prc->descriptor_count_)) < 0)
        {
          TIZ_ERROR (p_hdl, "[OMX_ErrorInsufficientResources] : "
                     "Unable to obtain poll descriptors for playback: %s",
                     snd_strerror (err));
          return OMX_ErrorInsufficientResources;
        }

      TIZ_DEBUG (p_hdl, "Poll descriptors : %d", p_prc->descriptor_count_);

      tiz_event_io_set (p_prc->p_ev_io_, p_prc->p_fds_->fd,
                        TIZ_EVENT_READ_OR_WRITE, true);

      /* OK, now prepare the PCM for use */
      if ((err = snd_pcm_prepare (p_prc->p_pcm_hdl)) < 0)
        {
          TIZ_ERROR (p_hdl, "[OMX_ErrorInsufficientResources] : "
                     "Could not prepare audio interface for use (%s)",
                     snd_strerror (err));
          return OMX_ErrorInsufficientResources;
        }

      /* Internally store the initial volume, so that the internal OMX volume
         struct reflects the current value of ALSA's master volume. */
      if (OMX_ErrorNone != set_component_volume (p_prc))
        {
          /* Volume control might not be supported by the current alsa device,
             not a big deal, simply log a message. */
          TIZ_NOTICE (p_hdl, "Could not set the component's volume");
        }
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_transfer_and_process (void *ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  p_prc->awaiting_buffers_ = true;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_stop_and_return (void *ap_obj)
{
  ar_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  return do_flush (p_prc);
}

static OMX_ERRORTYPE
ar_prc_deallocate_resources (void *ap_obj)
{
  ar_prc_t *p_prc = ap_obj;
  assert (NULL != ap_obj);

  p_prc->descriptor_count_ = 0;
  tiz_mem_free (p_prc->p_fds_);
  p_prc->p_fds_ = NULL;

  if (NULL != p_prc->p_ev_io_)
    {
      tiz_event_io_destroy (p_prc->p_ev_io_);
      p_prc->p_ev_io_ = NULL;
    }

  if (NULL != p_prc->p_hw_params)
    {
      snd_pcm_hw_params_free (p_prc->p_hw_params);
      (void) snd_pcm_close (p_prc->p_pcm_hdl);
      (void) snd_config_update_free_global ();
      p_prc->p_pcm_hdl = NULL;
      p_prc->p_hw_params = NULL;
    }

  tiz_mem_free (p_prc->p_alsa_pcm_);
  p_prc->p_alsa_pcm_ = NULL;

  return OMX_ErrorNone;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
ar_prc_buffers_ready (const void *ap_obj)
{
  ar_prc_t *p_prc = (ar_prc_t *) ap_obj;
  TIZ_TRACE (handleOf (p_prc),
             "Received buffer ready notification - awaiting_buffers [%s] "
             "awaiting_io_ev [%s]",
             p_prc->awaiting_buffers_ ? "YES" : "NO",
             p_prc->awaiting_io_ev_ ? "YES" : "NO");
  if (p_prc->awaiting_buffers_ == true && p_prc->awaiting_io_ev_ == false)
    {
      p_prc->awaiting_buffers_ = false;
      return render_pcm_data (p_prc);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_io_ready (void *ap_obj,
                 tiz_event_io_t * ap_ev_io, int a_fd, int a_events)
{
  ar_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  TIZ_TRACE (handleOf (p_prc), "Received io event on fd [%d]", a_fd);
  stop_io_watcher (p_prc);
  return render_pcm_data (p_prc);
}

static OMX_ERRORTYPE
ar_prc_pause (const void *ap_obj)
{
  ar_prc_t *p_prc = (ar_prc_t *) ap_obj;
  int pause = 1;
  assert (NULL != p_prc);
  snd_pcm_pause (p_prc->p_pcm_hdl, pause);
  TIZ_TRACE (handleOf (p_prc),
             "PAUSED ALSA device..."
             "awaiting_io_ev_ [%s]", p_prc->awaiting_io_ev_ ? "YES" : "NO");
  if (p_prc->awaiting_io_ev_ == true)
    {
      tiz_event_io_stop (p_prc->p_ev_io_);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_resume (const void *ap_obj)
{
  ar_prc_t *p_prc = (ar_prc_t *) ap_obj;
  int resume = 0;
  assert (NULL != p_prc);
  TIZ_TRACE (handleOf (p_prc), "RESUMING ALSA device...");
  snd_pcm_pause (p_prc->p_pcm_hdl, resume);
  if (p_prc->awaiting_io_ev_ == true)
    {
      start_io_watcher (p_prc);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_port_flush (const void *ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t *p_prc = (ar_prc_t *) ap_obj;
  assert (NULL != p_prc);
  return do_flush (p_prc);
}

static OMX_ERRORTYPE
ar_prc_port_disable (const void *ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t *p_prc = (ar_prc_t *) ap_obj;
  assert (NULL != p_prc);
  snd_pcm_drop (p_prc->p_pcm_hdl);
  /* Release any buffers held  */
  return release_buffers ((ar_prc_t *) ap_obj);
}

static OMX_ERRORTYPE
ar_prc_port_enable (const void *ap_obj, OMX_U32 a_pid)
{
  /* TODO */
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_config_change (void *ap_obj, OMX_U32 a_pid, OMX_INDEXTYPE a_config_idx)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  ar_prc_t *p_prc = ap_obj;

  assert (NULL != ap_obj);

  if (ARATELIA_AUDIO_RENDERER_PORT_INDEX == a_pid)
    {
      if (OMX_IndexConfigAudioVolume == a_config_idx)
        {
          OMX_AUDIO_CONFIG_VOLUMETYPE volume;
          TIZ_INIT_OMX_PORT_STRUCT (volume, ARATELIA_AUDIO_RENDERER_PORT_INDEX);
          if (OMX_ErrorNone
              != (rc =
                  tiz_api_GetConfig (tiz_get_krn (handleOf (p_prc)),
                                     handleOf (p_prc), OMX_IndexConfigAudioVolume,
                                     &volume)))
            {
              TIZ_ERROR (handleOf (p_prc), "[%s] : Error retrieving "
                         "OMX_IndexConfigAudioVolume from port",
                         tiz_err_to_str (rc));
            }
          else
            {
              TIZ_TRACE (handleOf (p_prc), "volume.sVolume.nValue = %ld\n",
                         volume.sVolume.nValue);
              if (volume.sVolume.nValue <= ARATELIA_AUDIO_RENDERER_MAX_VOLUME_VALUE
                  && volume.sVolume.nValue >=
                  ARATELIA_AUDIO_RENDERER_MIN_VOLUME_VALUE)
                {
                  /* TODO: Volume should be done by adjusting the gain, not ALSA's
                   * master volume! */
                  set_volume (p_prc, volume.sVolume.nValue);
                }
            }
        }
      else if (OMX_IndexConfigAudioMute == a_config_idx)
        {
          OMX_AUDIO_CONFIG_MUTETYPE mute;
          TIZ_INIT_OMX_PORT_STRUCT (mute, ARATELIA_AUDIO_RENDERER_PORT_INDEX);
          if (OMX_ErrorNone
              != (rc =
                  tiz_api_GetConfig (tiz_get_krn (handleOf (p_prc)),
                                     handleOf (p_prc), OMX_IndexConfigAudioMute,
                                     &mute)))
            {
              TIZ_ERROR (handleOf (p_prc), "[%s] : Error retrieving "
                         "OMX_IndexConfigAudioMute from port",
                         tiz_err_to_str (rc));
            }
          else
            {
              /* TODO: Volume should be done by adjusting the gain, not ALSA's
               * master volume! */
              TIZ_TRACE (handleOf (p_prc), "bMute = [%s]\n",
                         (mute.bMute == OMX_FALSE ? "FALSE" : "TRUE"));
              toggle_mute (p_prc, mute.bMute == OMX_TRUE ? true : false);
            }
        }
    }
  return rc;
}

/*
 * ar_prc_class
 */

static void *
ar_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "arprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
ar_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *arprc_class = factory_new (classOf (tizprc),
                                   "arprc_class",
                                   classOf (tizprc),
                                   sizeof (ar_prc_class_t),
                                   ap_tos, ap_hdl,
                                   ctor, ar_prc_class_ctor, 0);
  return arprc_class;
}

void *
ar_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *arprc_class = tiz_get_type (ap_hdl, "arprc_class");
  TIZ_LOG_CLASS (arprc_class);
  void *arprc = factory_new (arprc_class,
                             "arprc",
                             tizprc,
                             sizeof (ar_prc_t),
                             ap_tos, ap_hdl,
                             ctor, ar_prc_ctor,
                             dtor, ar_prc_dtor,
                             tiz_srv_allocate_resources,
                             ar_prc_allocate_resources,
                             tiz_srv_deallocate_resources,
                             ar_prc_deallocate_resources,
                             tiz_srv_prepare_to_transfer,
                             ar_prc_prepare_to_transfer,
                             tiz_srv_transfer_and_process,
                             ar_prc_transfer_and_process,
                             tiz_srv_stop_and_return, ar_prc_stop_and_return,
                             tiz_prc_buffers_ready, ar_prc_buffers_ready,
                             tiz_prc_io_ready, ar_prc_io_ready,
                             tiz_prc_pause, ar_prc_pause,
                             tiz_prc_resume, ar_prc_resume,
                             tiz_prc_port_flush, ar_prc_port_flush,
                             tiz_prc_port_disable, ar_prc_port_disable,
                             tiz_prc_port_enable, ar_prc_port_enable,
                             tiz_prc_config_change, ar_prc_config_change,
                             0);

  return arprc;
}
