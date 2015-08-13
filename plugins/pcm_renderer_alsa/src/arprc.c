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
          ;                                                \
        }                                                  \
    }                                                      \
  while (0)

/* Forward declaration */
static OMX_ERRORTYPE ar_prc_deallocate_resources (void *ap_obj);

static void log_alsa_pcm_state (ar_prc_t *ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_pcm_hdl)
    {
      snd_pcm_state_t state = snd_pcm_state (ap_prc->p_pcm_hdl);
      TIZ_DEBUG (handleOf (ap_prc), "ALSA PCM state : [%s]",
                 snd_pcm_state_name (state));
    }
}

/*@null@*/ static char *get_alsa_device (ar_prc_t *ap_prc)
{
  assert (ap_prc);

  if (!ap_prc->p_alsa_pcm_)
    {
      const char *p_alsa_pcm = tiz_rcfile_get_value (
          TIZ_RCFILE_PLUGINS_DATA_SECTION,
          "OMX.Aratelia.audio_renderer.alsa.pcm.alsa_device");

      if (p_alsa_pcm)
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
    }
  return (ap_prc->p_alsa_pcm_)
             ? ap_prc->p_alsa_pcm_
             : ARATELIA_AUDIO_RENDERER_DEFAULT_ALSA_DEVICE;
}

/*@null@*/ static char *get_alsa_mixer (ar_prc_t *ap_prc)
{
  assert (ap_prc);

  if (!ap_prc->p_alsa_mixer_)
    {
      const char *p_alsa_mixer = tiz_rcfile_get_value (
          TIZ_RCFILE_PLUGINS_DATA_SECTION,
          "OMX.Aratelia.audio_renderer.alsa.pcm.alsa_mixer");

      if (p_alsa_mixer)
        {
          TIZ_TRACE (handleOf (ap_prc), "Using ALSA mixer [%s]...", p_alsa_mixer);
          ap_prc->p_alsa_mixer_ = strndup (p_alsa_mixer, OMX_MAX_STRINGNAME_SIZE);
        }
      else
        {
          TIZ_TRACE (handleOf (ap_prc),
                     "No alsa mixer found in config file. Using [%s]...",
                     ARATELIA_AUDIO_RENDERER_DEFAULT_ALSA_MIXER);
        }
    }
  return (ap_prc->p_alsa_mixer_)
             ? ap_prc->p_alsa_mixer_
             : ARATELIA_AUDIO_RENDERER_DEFAULT_ALSA_MIXER;
}

static bool using_null_alsa_device (ar_prc_t *ap_prc)
{
  return (0 == strncmp (get_alsa_device (ap_prc),
                        ARATELIA_AUDIO_RENDERER_NULL_ALSA_DEVICE,
                        OMX_MAX_STRINGNAME_SIZE));
}

static inline OMX_ERRORTYPE start_io_watcher (ar_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);
  assert (ap_prc->p_ev_io_);
  if (!ap_prc->awaiting_io_ev_)
    {
      rc = tiz_srv_io_watcher_start (ap_prc, ap_prc->p_ev_io_);
    }
  ap_prc->awaiting_io_ev_ = true;
  TIZ_DEBUG (handleOf (ap_prc), "io watcher : [RUNNING]");
  return rc;
}

static inline OMX_ERRORTYPE stop_io_watcher (ar_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);
  if (ap_prc->p_ev_io_ && ap_prc->awaiting_io_ev_)
    {
      rc = tiz_srv_io_watcher_stop (ap_prc, ap_prc->p_ev_io_);
    }
  ap_prc->awaiting_io_ev_ = false;
  TIZ_DEBUG (handleOf (ap_prc), "io watcher : [STOPPED]");
  return rc;
}

static OMX_ERRORTYPE release_header (ar_prc_t *ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_inhdr_)
    {
      ap_prc->p_inhdr_->nOffset = 0;
      tiz_check_omx_err (tiz_krn_release_buffer (
          tiz_get_krn (handleOf (ap_prc)), ARATELIA_AUDIO_RENDERER_PORT_INDEX,
          ap_prc->p_inhdr_));
      ap_prc->p_inhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE do_flush (ar_prc_t *ap_prc)
{
  assert (ap_prc);
  tiz_check_omx_err (stop_io_watcher (ap_prc));
  if (ap_prc->p_pcm_hdl)
    {
      (void)snd_pcm_drop (ap_prc->p_pcm_hdl);
    }
  /* Release any buffers held  */
  return release_header (ap_prc);
}

static float sint_to_float (const int a_sample)
{
  float f;
  if (a_sample >= 0)
    f = a_sample / 32767.0;
  else
    f = a_sample / 32768.0;
  return f;
}

static int float_to_sint (const float a_sample)
{
  float f;
  f = a_sample * 32767;
  if (f < -32768)
    f = -32768;
  if (f > 32767)
    f = 32767;
  return (int)f;
}

static void adjust_gain (const ar_prc_t *ap_prc, OMX_BUFFERHEADERTYPE *ap_hdr,
                         const snd_pcm_uframes_t a_num_samples)
{
  assert (ap_prc);
  assert (ap_hdr);

  if (ARATELIA_AUDIO_RENDERER_DEFAULT_GAIN_VALUE != ap_prc->gain_)
    {
      int i;
      int gainadj = (int)(ap_prc->gain_ * 256.);
      float gain = pow (10., gainadj / 5120.);
      /*       gain = 3.1f; */
      /*       fprintf (stderr, "%f samples %ld\n", gain, a_num_samples); */
      OMX_S16 *pcm = (OMX_S16 *)(ap_hdr->pBuffer + ap_hdr->nOffset);
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

static OMX_ERRORTYPE get_alsa_master_volume (ar_prc_t *ap_prc,
                                             long *ap_volume)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);
  assert (ap_volume);

  {
    const char *selem_name = get_alsa_mixer (ap_prc);
    long min, max, volume;
    snd_mixer_t *handle = NULL;
    snd_mixer_selem_id_t *sid = NULL;
    snd_mixer_elem_t *elem = NULL;

    rc = OMX_ErrorInsufficientResources;
    bail_on_snd_mixer_error (snd_mixer_open (&handle, 0));
    bail_on_snd_mixer_error (snd_mixer_attach (handle, ap_prc->p_alsa_pcm_));
    bail_on_snd_mixer_error (snd_mixer_selem_register (handle, NULL, NULL));
    bail_on_snd_mixer_error (snd_mixer_load (handle));

    snd_mixer_selem_id_alloca (&sid);
    snd_mixer_selem_id_set_index (sid, 0);
    snd_mixer_selem_id_set_name (sid, selem_name);
    elem = snd_mixer_find_selem (handle, sid);

    if (!elem)
      {
        TIZ_ERROR (handleOf (ap_prc), "[OMX_ErrorInsufficientResources] : "
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

static OMX_ERRORTYPE set_initial_component_volume (ar_prc_t *ap_prc)
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
      tiz_check_omx_err (get_alsa_master_volume (ap_prc, &(ap_prc->volume_)));

      TIZ_INIT_OMX_PORT_STRUCT (volume, ARATELIA_AUDIO_RENDERER_PORT_INDEX);
      tiz_check_omx_err (
          tiz_api_GetConfig (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                             OMX_IndexConfigAudioVolume, &volume));

      volume.sVolume.nValue = ap_prc->volume_;

      TIZ_TRACE (handleOf (ap_prc), "ap_prc->volume_ [%d]", ap_prc->volume_);

      tiz_check_omx_err (tiz_krn_SetConfig_internal (
          tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
          OMX_IndexConfigAudioVolume, &volume));
    }
  return OMX_ErrorNone;
}

static bool set_alsa_master_volume (ar_prc_t *ap_prc, const long a_volume)
{
  bool rc = false;
  assert (ap_prc);

  {
    const char *selem_name = get_alsa_mixer (ap_prc);
    long min, max;
    snd_mixer_t *handle = NULL;
    snd_mixer_selem_id_t *sid = NULL;
    snd_mixer_elem_t *elem = NULL;

    bail_on_snd_mixer_error (snd_mixer_open (&handle, 0));
    bail_on_snd_mixer_error (snd_mixer_attach (handle, ap_prc->p_alsa_pcm_));
    bail_on_snd_mixer_error (snd_mixer_selem_register (handle, NULL, NULL));
    bail_on_snd_mixer_error (snd_mixer_load (handle));

    snd_mixer_selem_id_alloca (&sid);
    snd_mixer_selem_id_set_index (sid, 0);
    snd_mixer_selem_id_set_name (sid, selem_name);
    elem = snd_mixer_find_selem (handle, sid);

    if (!elem)
      {
        TIZ_ERROR (handleOf (ap_prc), "[OMX_ErrorInsufficientResources] : "
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

static void toggle_mute (ar_prc_t *ap_prc, const bool a_mute)
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

static void set_volume (ar_prc_t *ap_prc, const long a_volume)
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

static void prepare_volume_ramp (ar_prc_t *ap_prc)
{
  assert (ap_prc);
  ap_prc->ramp_volume_ = ARATELIA_AUDIO_RENDERER_DEFAULT_VOLUME_VALUE;
  ap_prc->ramp_step_count_ = ARATELIA_AUDIO_RENDERER_DEFAULT_RAMP_STEP_COUNT;
  ap_prc->ramp_step_ = (double)ap_prc->ramp_volume_
                       / (double)ap_prc->ramp_step_count_;
  TIZ_TRACE (handleOf (ap_prc), "ramp_step_ = [%d] ramp_step_count_ = [%d]",
             ap_prc->ramp_step_, ap_prc->ramp_step_count_);
}

static OMX_ERRORTYPE start_volume_ramp (ar_prc_t *ap_prc)
{
  assert (ap_prc);
  assert (ap_prc->p_ev_timer_);
  ap_prc->ramp_volume_ = 0;
  TIZ_TRACE (handleOf (ap_prc), "ramp_volume_ = [%d]", ap_prc->ramp_volume_);
  tiz_check_omx_err (
      tiz_srv_timer_watcher_start (ap_prc, ap_prc->p_ev_timer_, 0.2, 0.2));
  return OMX_ErrorNone;
}

static void stop_volume_ramp (ar_prc_t *ap_prc)
{
  assert (ap_prc);
  if (ap_prc->p_ev_timer_)
    {
      (void)tiz_srv_timer_watcher_stop (ap_prc, ap_prc->p_ev_timer_);
    }
}

static OMX_ERRORTYPE apply_ramp_step (ar_prc_t *ap_prc)
{
  assert (ap_prc);
  if (ap_prc->ramp_step_count_-- > 0)
    {
      ap_prc->ramp_volume_ += ap_prc->ramp_step_;
      TIZ_TRACE (handleOf (ap_prc), "ramp_volume_ = [%d]",
                 ap_prc->ramp_volume_);
      set_volume (ap_prc, ap_prc->ramp_volume_);
    }
  else
    {
      stop_volume_ramp (ap_prc);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE render_buffer (ar_prc_t *ap_prc,
                                    OMX_BUFFERHEADERTYPE *ap_hdr)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  snd_pcm_sframes_t err = 0;
  snd_pcm_uframes_t samples = 0;
  unsigned long int step = 0;

  assert (ap_prc);
  assert (ap_hdr);

  step = (ap_prc->pcmmode.nBitPerSample / 8) * ap_prc->pcmmode.nChannels;
  assert (ap_hdr->nFilledLen > 0);
  samples = ap_hdr->nFilledLen / step;
  TIZ_TRACE (handleOf (ap_prc),
             "Rendering HEADER [%p]... step [%d], samples [%d] nFilledLen [%d]",
             ap_hdr, step, samples, ap_hdr->nFilledLen);
  TIZ_PRINTF_DBG_RED ("HEADER [%p] nFilledLen = [%d]\n", ap_hdr,
                      ap_hdr->nFilledLen);

  adjust_gain (ap_prc, ap_hdr, samples);

  while (samples > 0 && OMX_ErrorNone == rc)
    {
      err = snd_pcm_writei (ap_prc->p_pcm_hdl,
                            ap_hdr->pBuffer + ap_hdr->nOffset, samples);

      if (-EAGAIN == err)
        {
          /* got -EAGAIN, alsa's buffers must be full */
          rc = OMX_ErrorNoMore;
        }
      else if (err < 0)
        {
          /* This should handle -EINTR (interrupted system call), -EPIPE
           * (overrun or underrun) and -ESTRPIPE (stream is suspended) */
          err = snd_pcm_recover (ap_prc->p_pcm_hdl, (int)err, 0);
          if (err < 0)
            {
              TIZ_ERROR (handleOf (ap_prc), "snd_pcm_recover error: %s",
                         snd_strerror ((int)err));
              rc = OMX_ErrorUnderflow;
            }
        }
      else
        {
          ap_hdr->nOffset += err * step;
          ap_hdr->nFilledLen -= err * step;
          samples -= err;
        }
    }

  if (OMX_ErrorNone == rc)
    {
      ap_hdr->nFilledLen = 0;
    }
  return rc;
}

static OMX_BUFFERHEADERTYPE *get_header (ar_prc_t *ap_prc)
{
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  assert (ap_prc);

  if (!ap_prc->port_disabled_)
    {
      if (!ap_prc->p_inhdr_)
        {
          (void)tiz_krn_claim_buffer (tiz_get_krn (handleOf (ap_prc)),
                                      ARATELIA_AUDIO_RENDERER_PORT_INDEX, 0,
                                      &ap_prc->p_inhdr_);
          if (ap_prc->p_inhdr_)
            {
              TIZ_TRACE (handleOf (ap_prc),
                         "Claimed HEADER [%p]...nFilledLen [%d]",
                         ap_prc->p_inhdr_, ap_prc->p_inhdr_->nFilledLen);
            }
        }
      p_hdr = ap_prc->p_inhdr_;
    }
  return p_hdr;
}

static OMX_ERRORTYPE buffer_emptied (ar_prc_t *ap_prc)
{
  assert (ap_prc);
  assert (ap_prc->p_inhdr_);
  assert (ap_prc->p_inhdr_->nFilledLen == 0);

  TIZ_TRACE (handleOf (ap_prc), "Releasing HEADER [%p] emptied",
             ap_prc->p_inhdr_);

  if ((ap_prc->p_inhdr_->nFlags & OMX_BUFFERFLAG_EOS) != 0)
    {
      TIZ_DEBUG (handleOf (ap_prc), "OMX_BUFFERFLAG_EOS in HEADER [%p]",
                 ap_prc->p_inhdr_);
      tiz_srv_issue_event ((OMX_PTR)ap_prc, OMX_EventBufferFlag, 0,
                           ap_prc->p_inhdr_->nFlags, NULL);
    }

  return release_header (ap_prc);
}

static OMX_ERRORTYPE render_pcm_data (ar_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  while (((p_hdr = get_header (ap_prc))) && OMX_ErrorNone == rc)
    {
      if (p_hdr->nFilledLen > 0)
        {
          rc = render_buffer (ap_prc, p_hdr);
        }

      if (0 == p_hdr->nFilledLen)
        {
          tiz_check_omx_err (buffer_emptied (ap_prc));
          p_hdr = NULL;
        }
    }

  if (OMX_ErrorNoMore == rc)
    {
      TIZ_DEBUG (handleOf (ap_prc), "OMX_ErrorNoMore");
      tiz_check_omx_err (start_io_watcher (ap_prc));
      rc = OMX_ErrorNone;
    }

  return rc;
}

/*
 * arprc
 */

static void *ar_prc_ctor (void *ap_obj, va_list *app)
{
  ar_prc_t *p_obj = super_ctor (typeOf (ap_obj, "arprc"), ap_obj, app);
  p_obj->p_pcm_hdl = NULL;
  p_obj->p_hw_params = NULL;
  p_obj->p_alsa_pcm_ = NULL;
  p_obj->p_alsa_mixer_ = NULL;
  p_obj->descriptor_count_ = 0;
  p_obj->p_fds_ = NULL;
  p_obj->p_ev_io_ = NULL;
  p_obj->p_ev_timer_ = NULL;
  p_obj->p_inhdr_ = NULL;
  p_obj->port_disabled_ = false;
  p_obj->awaiting_io_ev_ = false;
  p_obj->gain_ = ARATELIA_AUDIO_RENDERER_DEFAULT_GAIN_VALUE;
  p_obj->volume_ = ARATELIA_AUDIO_RENDERER_DEFAULT_VOLUME_VALUE;
  p_obj->ramp_step_ = 0;
  p_obj->ramp_step_count_ = ARATELIA_AUDIO_RENDERER_DEFAULT_RAMP_STEP_COUNT;
  p_obj->ramp_volume_ = 0;
  return p_obj;
}

static void *ar_prc_dtor (void *ap_obj)
{
  (void)ar_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "arprc"), ap_obj);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE ar_prc_allocate_resources (void *ap_prc,
                                                OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t *p_prc = ap_prc;

  assert (p_prc);

  if (!p_prc->p_pcm_hdl)
    {
      char *p_device = get_alsa_device (p_prc);
      assert (p_device);

      /* Open a PCM in non-blocking mode */
      bail_on_snd_pcm_error (snd_pcm_open (&p_prc->p_pcm_hdl, p_device,
                                           SND_PCM_STREAM_PLAYBACK,
                                           SND_PCM_NONBLOCK));
      /* Allocate alsa's hardware parameter structure */
      bail_on_snd_pcm_error (snd_pcm_hw_params_malloc (&p_prc->p_hw_params));

      /* Get the alsa descriptors count */
      p_prc->descriptor_count_
          = snd_pcm_poll_descriptors_count (p_prc->p_pcm_hdl);
      if (p_prc->descriptor_count_ <= 0)
        {
          TIZ_ERROR (handleOf (p_prc),
                     "[OMX_ErrorInsufficientResources] : "
                     "Invalid poll descriptors count");
          return OMX_ErrorInsufficientResources;
        }

      /* Allocate space for the list of alsa fds */
      p_prc->p_fds_
          = tiz_mem_alloc (sizeof(struct pollfd) * p_prc->descriptor_count_);
      tiz_check_null_ret_oom (p_prc->p_fds_);

      /* This is to generate volume ramps when needed */
      tiz_check_omx_err (
          tiz_srv_timer_watcher_init (p_prc, &(p_prc->p_ev_timer_)));
    }

  assert (p_prc->p_pcm_hdl);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE ar_prc_prepare_to_transfer (void *ap_prc,
                                                 OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t *p_prc = ap_prc;
  assert (p_prc);

  if (p_prc->p_pcm_hdl)
    {
      snd_pcm_format_t snd_pcm_format;
      unsigned int snd_sampling_rate = 0;

      /* Fill params with a full configuration space for the PCM. */
      bail_on_snd_pcm_error (
          snd_pcm_hw_params_any (p_prc->p_pcm_hdl, p_prc->p_hw_params));

      TIZ_DEBUG (
          handleOf (p_prc), "Device can pause [%s] ",
          snd_pcm_hw_params_can_pause (p_prc->p_hw_params) == 0 ? "NO" : "YES");

      TIZ_DEBUG (handleOf (p_prc), "Device can resume [%s] ",
                 snd_pcm_hw_params_can_resume (p_prc->p_hw_params) == 0
                     ? "NO"
                     : "YES");

      /* Retrieve pcm params from port */
      TIZ_INIT_OMX_PORT_STRUCT (p_prc->pcmmode,
                                ARATELIA_AUDIO_RENDERER_PORT_INDEX);
      tiz_check_omx_err (tiz_api_GetParameter (
          tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
          OMX_IndexParamAudioPcm, &p_prc->pcmmode));

      TIZ_NOTICE (
          handleOf (p_prc),
          "nChannels = [%d] nBitPerSample = [%d] "
          "nSamplingRate = [%d] eNumData = [%d] eEndian = [%d] "
          "bInterleaved = [%s] ePCMMode = [%d]",
          p_prc->pcmmode.nChannels, p_prc->pcmmode.nBitPerSample,
          p_prc->pcmmode.nSamplingRate, p_prc->pcmmode.eNumData,
          p_prc->pcmmode.eEndian,
          p_prc->pcmmode.bInterleaved == OMX_TRUE ? "OMX_TRUE" : "OMX_FALSE",
          p_prc->pcmmode.ePCMMode);

      /* TODO : Add a function to properly encode snd_pcm_format */
      if (p_prc->pcmmode.nBitPerSample == 24)
        {
          snd_pcm_format = p_prc->pcmmode.eEndian == OMX_EndianLittle
                               ? SND_PCM_FORMAT_S24
                               : SND_PCM_FORMAT_S24_BE;
        }
      /* NOTE: this is a hack to allow float pcm streams coming from the the
         vorbis or opusfile decoders */
      else if (p_prc->pcmmode.nBitPerSample == 32)
        {
          snd_pcm_format = p_prc->pcmmode.eEndian == OMX_EndianLittle
                               ? SND_PCM_FORMAT_FLOAT_LE
                               : SND_PCM_FORMAT_FLOAT_BE;
        }
      else
        {
          /* p_prc->pcmmode.nBitPerSample == 16 */
          snd_pcm_format = p_prc->pcmmode.eEndian == OMX_EndianLittle
                               ? SND_PCM_FORMAT_S16
                               : SND_PCM_FORMAT_S16_BE;
        }

      /*       snd_sampling_rate = p_prc->pcmmode.bInterleaved == OMX_TRUE ? */
      /*         p_prc->pcmmode.nSamplingRate : p_prc->pcmmode.nSamplingRate *
       * 2; */
      snd_sampling_rate = p_prc->pcmmode.nSamplingRate;

      /* This sets the hardware and software parameters in a convenient way. */
      bail_on_snd_pcm_error (snd_pcm_set_params (
          p_prc->p_pcm_hdl, snd_pcm_format, SND_PCM_ACCESS_RW_INTERLEAVED,
          (unsigned int)p_prc->pcmmode.nChannels, snd_sampling_rate,
          0,     /* allow alsa-lib resampling */
          100000 /* overall latency in us */
          ));

      bail_on_snd_pcm_error (snd_pcm_poll_descriptors (
          p_prc->p_pcm_hdl, p_prc->p_fds_, p_prc->descriptor_count_));

      TIZ_DEBUG (handleOf (p_prc), "Poll descriptors : %d",
                 p_prc->descriptor_count_);

      /* Init the io watcher */
      tiz_check_omx_err (
          tiz_srv_io_watcher_init (p_prc, &(p_prc->p_ev_io_), p_prc->p_fds_->fd,
                                   TIZ_EVENT_READ_OR_WRITE, true));

      /* OK, now prepare the PCM for use */
      bail_on_snd_pcm_error (snd_pcm_prepare (p_prc->p_pcm_hdl));

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

static OMX_ERRORTYPE ar_prc_transfer_and_process (void *ap_obj,
                                                  OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t *p_prc = ap_obj;
  assert (p_prc);
  prepare_volume_ramp (p_prc);
  tiz_check_omx_err (start_volume_ramp (p_prc));
  tiz_check_omx_err (apply_ramp_step (p_prc));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE ar_prc_stop_and_return (void *ap_obj)
{
  stop_volume_ramp (ap_obj);
  return do_flush (ap_obj);
}

static OMX_ERRORTYPE ar_prc_deallocate_resources (void *ap_obj)
{
  ar_prc_t *p_prc = ap_obj;
  assert (p_prc);

  tiz_srv_timer_watcher_destroy (p_prc, p_prc->p_ev_timer_);
  p_prc->p_ev_timer_ = NULL;

  p_prc->descriptor_count_ = 0;
  tiz_mem_free (p_prc->p_fds_);
  p_prc->p_fds_ = NULL;

  tiz_srv_io_watcher_destroy (p_prc, p_prc->p_ev_io_);
  p_prc->p_ev_io_ = NULL;

  if (p_prc->p_hw_params)
    {
      snd_pcm_hw_params_free (p_prc->p_hw_params);
      (void)snd_pcm_close (p_prc->p_pcm_hdl);
      (void)snd_config_update_free_global ();
      p_prc->p_pcm_hdl = NULL;
      p_prc->p_hw_params = NULL;
    }

  tiz_mem_free (p_prc->p_alsa_pcm_);
  p_prc->p_alsa_pcm_ = NULL;

  tiz_mem_free (p_prc->p_alsa_mixer_);
  p_prc->p_alsa_mixer_ = NULL;

  return OMX_ErrorNone;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE ar_prc_buffers_ready (const void *ap_obj)
{
  ar_prc_t *p_prc = (ar_prc_t *)ap_obj;
  assert (p_prc);
  TIZ_TRACE (handleOf (p_prc),
             "Received buffer ready notification - "
             "awaiting_io_ev [%s]",
             p_prc->awaiting_io_ev_ ? "YES" : "NO");
  return render_pcm_data (p_prc);
}

static OMX_ERRORTYPE ar_prc_io_ready (void *ap_obj, tiz_event_io_t *ap_ev_io,
                                      int a_fd, int a_events)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  ar_prc_t *p_prc = ap_obj;
  TIZ_TRACE (handleOf (p_prc), "Received io event on fd [%d]", a_fd);
  if (p_prc->awaiting_io_ev_)
    {
      p_prc->awaiting_io_ev_ = false;
      rc = render_pcm_data (ap_obj);
    }
  return rc;
}

static OMX_ERRORTYPE ar_prc_timer_ready (void *ap_prc,
                                         tiz_event_timer_t *ap_ev_timer,
                                         void *ap_arg, const uint32_t a_id)
{
  TIZ_TRACE (handleOf (ap_prc), "Received timer event");
  return apply_ramp_step (ap_prc);
}

static OMX_ERRORTYPE ar_prc_pause (const void *ap_prc)
{
  ar_prc_t *p_prc = (ar_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int pause = 1;
  assert (p_prc);
  tiz_check_omx_err (stop_io_watcher (p_prc));
  bail_on_snd_pcm_error (snd_pcm_pause (p_prc->p_pcm_hdl, pause));
  TIZ_TRACE (handleOf (p_prc),
             "PAUSED ALSA device..."
             "awaiting_io_ev_ [%s]",
             p_prc->awaiting_io_ev_ ? "YES" : "NO");
  return rc;
}

static OMX_ERRORTYPE ar_prc_resume (const void *ap_prc)
{
  ar_prc_t *p_prc = (ar_prc_t *)ap_prc;
  int resume = 0;
  assert (p_prc);
  TIZ_TRACE (handleOf (p_prc), "RESUMING ALSA device...");
  bail_on_snd_pcm_error (snd_pcm_pause (p_prc->p_pcm_hdl, resume));
  tiz_check_omx_err (start_io_watcher (p_prc));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE ar_prc_port_flush (const void *ap_obj,
                                        OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t *p_prc = (ar_prc_t *)ap_obj;
  return do_flush (p_prc);
}

static OMX_ERRORTYPE ar_prc_port_disable (const void *ap_prc,
                                          OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t *p_prc = (ar_prc_t *)ap_prc;
  assert (p_prc);
  TIZ_PRINTF_DBG_BLU ("Received port disable\n");
  stop_volume_ramp (p_prc);
  p_prc->port_disabled_ = true;
  if (p_prc->p_pcm_hdl)
    {
      bail_on_snd_pcm_error (snd_pcm_drain (p_prc->p_pcm_hdl));
      tiz_check_omx_err (stop_io_watcher (p_prc));
    }
  /* Release any buffers held  */
  return release_header ((ar_prc_t *)ap_prc);
}

static OMX_ERRORTYPE ar_prc_port_enable (const void *ap_obj,
                                         OMX_U32 TIZ_UNUSED (a_pid))
{
  ar_prc_t *p_prc = (ar_prc_t *)ap_obj;
  assert (p_prc);
  TIZ_PRINTF_DBG_GRN ("Received port emable\n");
  p_prc->port_disabled_ = false;
  if (p_prc->p_pcm_hdl)
    {
      log_alsa_pcm_state (p_prc);
      tiz_check_omx_err (ar_prc_prepare_to_transfer (p_prc, OMX_ALL));
      tiz_check_omx_err (ar_prc_transfer_and_process (p_prc, OMX_ALL));
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE ar_prc_config_change (void *ap_obj, OMX_U32 a_pid,
                                           OMX_INDEXTYPE a_config_idx)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  ar_prc_t *p_prc = ap_obj;

  assert (ap_obj);

  if (ARATELIA_AUDIO_RENDERER_PORT_INDEX == a_pid)
    {
      if (OMX_IndexConfigAudioVolume == a_config_idx)
        {
          OMX_AUDIO_CONFIG_VOLUMETYPE volume;
          TIZ_INIT_OMX_PORT_STRUCT (volume, ARATELIA_AUDIO_RENDERER_PORT_INDEX);
          tiz_check_omx_err (tiz_api_GetConfig (
              tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
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
          tiz_check_omx_err (tiz_api_GetConfig (
              tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
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

static void *ar_prc_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "arprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *ar_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *arprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizprc), "arprc_class", classOf (tizprc),
       sizeof(ar_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, ar_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value */
       0);
  return arprc_class;
}

void *ar_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *arprc_class = tiz_get_type (ap_hdl, "arprc_class");
  TIZ_LOG_CLASS (arprc_class);
  void *arprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (arprc_class, "arprc", tizprc, sizeof(ar_prc_t),
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
