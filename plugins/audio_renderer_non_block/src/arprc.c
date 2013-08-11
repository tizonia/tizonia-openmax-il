/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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

#include "arprc.h"
#include "arprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizosal.h"

#include <assert.h>
#include <errno.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.audio_renderer.prc"
#endif

#define TIZ_AR_ALSA_PCM_DEVICE "null"

/*@null@*/ static char *
get_alsa_device (ar_prc_t *ap_prc)
{
  const char *p_alsa_pcm = NULL;

  assert (NULL != ap_prc);
  assert (NULL == ap_prc->p_alsa_pcm_);

  p_alsa_pcm
    = tiz_rcfile_get_value ("plugins-data",
                            "OMX.Aratelia.audio_renderer_nb.pcm.alsa_device");

  if (NULL != p_alsa_pcm)
    {
      TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (ap_prc),
                "Using ALSA pcm [%s]...", p_alsa_pcm);
      ap_prc->p_alsa_pcm_ = strndup (p_alsa_pcm, OMX_MAX_STRINGNAME_SIZE);
    }
  else
    {
      TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (ap_prc),
               "No alsa device found in config file. Using [%s]...",
               TIZ_AR_ALSA_PCM_DEVICE);
    }

  return (NULL != ap_prc->p_alsa_pcm_) ? ap_prc->p_alsa_pcm_
    : TIZ_AR_ALSA_PCM_DEVICE;
}

static inline OMX_ERRORTYPE
start_io_watcher (ar_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_ev_io_);
  ap_prc->awaiting_io_ev_ = true;
  return tiz_event_io_start (ap_prc->p_ev_io_);
}

static inline OMX_ERRORTYPE
stop_io_watcher (ar_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_ev_io_);
  ap_prc->awaiting_io_ev_ = false;
  return tiz_event_io_stop (ap_prc->p_ev_io_);
}

static OMX_ERRORTYPE
release_buffers (ar_prc_t *ap_prc)
{
  assert (NULL != ap_prc);

  stop_io_watcher (ap_prc);

  if (ap_prc->p_inhdr_)
    {
      void *p_krn = tiz_get_krn (tiz_srv_get_hdl (ap_prc));
      tiz_check_omx_err (tiz_krn_release_buffer (p_krn, 0, ap_prc->p_inhdr_));
      ap_prc->p_inhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE
do_flush (ar_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  tiz_check_omx_err (stop_io_watcher (ap_prc));
  (void) snd_pcm_drop (ap_prc->p_pcm_hdl);
  /* Release any buffers held  */
  return release_buffers (ap_prc);
}

static OMX_ERRORTYPE
render_buffer (ar_prc_t *ap_prc, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  snd_pcm_sframes_t err = 0;
  snd_pcm_uframes_t samples = 0;
  unsigned long int step = 0;

  assert (NULL != ap_prc);
  assert (NULL != ap_hdr);
  
  TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (ap_prc),
           "Rendering HEADER [%p]...nFilledLen[%d] !!!", ap_hdr,
           ap_hdr->nFilledLen);

  step = (ap_prc->pcmmode.nBitPerSample / 8) * ap_prc->pcmmode.nChannels;
  assert (ap_hdr->nFilledLen >= ap_hdr->nOffset);
  samples = (ap_hdr->nFilledLen - ap_hdr->nOffset) / step;
  TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (ap_prc),
    "step [%d], samples [%d]", step, samples);

  while (samples > 0)
    {
      err = snd_pcm_writei (ap_prc->p_pcm_hdl,
                            ap_hdr->pBuffer + ap_hdr->nOffset,
                            samples);
      TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (ap_prc),
               "Rendering HEADER [%p]..." "err [%d] samples [%d] nOffset [%d]",
                ap_hdr, err, samples, ap_hdr->nOffset);

      if (-EAGAIN == err)
        {
          TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (ap_prc),
                    "Ring buffer must be full (got -EAGAIN)");
          return start_io_watcher (ap_prc);
        }

      if (err < 0)
        {
          TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (ap_prc),
                    "Rendering HEADER [%p]...underflow");
          err = snd_pcm_recover (ap_prc->p_pcm_hdl, (int) err, 0);
          if (err < 0)
            {
              TIZ_LOGN (TIZ_ERROR, tiz_srv_get_hdl (ap_prc),
                        "snd_pcm_recover error: %s",
                       snd_strerror ((int) err));
              break;
            }
        }

      ap_hdr->nOffset += err * step;
      samples -= err;
    }

  ap_hdr->nFilledLen = 0;

  return OMX_ErrorNone;
}

static OMX_BUFFERHEADERTYPE *
buffer_needed (ar_prc_t *ap_prc)
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

          p_krn = tiz_get_krn (tiz_srv_get_hdl (ap_prc));

          TIZ_PD_ZERO (&ports);
          if (OMX_ErrorNone == tiz_krn_select (p_krn, 1, &ports))
            {
              if (TIZ_PD_ISSET (0, &ports))
                {
                  if (OMX_ErrorNone == tiz_krn_claim_buffer
                      (p_krn, 0, 0, &ap_prc->p_inhdr_))
                    {
                      TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (ap_prc),
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
buffer_emptied (ar_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  assert (ap_prc->p_inhdr_->nFilledLen == 0);

  TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (ap_prc), "HEADER [%p] emptied",
            ap_prc->p_inhdr_);

  ap_prc->p_inhdr_->nOffset = 0;

  if ((ap_prc->p_inhdr_->nFlags & OMX_BUFFERFLAG_EOS) != 0)
    {
      TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (ap_prc),
                "OMX_BUFFERFLAG_EOS in HEADER [%p]",
                ap_prc->p_inhdr_);
      tiz_srv_issue_event ((OMX_PTR) ap_prc,
                           OMX_EventBufferFlag, 0,
                           ap_prc->p_inhdr_->nFlags, NULL);
    }

  tiz_check_omx_err (tiz_krn_release_buffer (tiz_get_krn
                                             (tiz_srv_get_hdl (ap_prc)),
                                             0, ap_prc->p_inhdr_));
  ap_prc->p_inhdr_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
render_pcm_data (ar_prc_t *ap_prc)
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

      TIZ_LOGN (TIZ_ERROR, tiz_srv_get_hdl (ap_prc),
                "awaiting_io_ev_ [%s]",
                ap_prc->awaiting_io_ev_ ? "YES" : "NO");

      if (0 == p_hdr->nFilledLen)
        {
          if (OMX_ErrorNone != (rc = buffer_emptied (ap_prc)))
            {
              TIZ_LOGN (TIZ_ERROR, tiz_srv_get_hdl (ap_prc),
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
  ar_prc_t *p_obj          = super_ctor (arprc, ap_obj, app);
  p_obj->p_pcm_hdl         = NULL;
  p_obj->p_hw_params       = NULL;
  p_obj->p_alsa_pcm_       = NULL;
  p_obj->descriptor_count_ = 0;
  p_obj->p_fds_            = NULL;
  p_obj->p_ev_io_          = NULL;
  p_obj->p_inhdr_          = NULL;
  p_obj->port_disabled_    = false;
  p_obj->awaiting_buffers_ = true;
  p_obj->awaiting_io_ev_   = false;
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

  return super_dtor (arprc, ap_obj);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
ar_prc_allocate_resources (void *ap_obj, OMX_U32 TIZ_UNUSED(a_pid))
{
  ar_prc_t *p_prc = ap_obj;
  OMX_HANDLETYPE p_hdl = tiz_srv_get_hdl (p_prc);
  int err = 0;

  assert (NULL != ap_obj);

  if (NULL == p_prc->p_pcm_hdl)
    {
      OMX_ERRORTYPE rc = OMX_ErrorNone;
      char * p_device = get_alsa_device (p_prc);
      assert (NULL != p_device);

      /* Open a PCM in non-blocking mode */
      if ((err =
           snd_pcm_open (&p_prc->p_pcm_hdl, p_device,
                         SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0)
        {
          TIZ_LOGN (TIZ_ERROR, p_hdl,
                    "cannot open audio device %s (%s)",
                   TIZ_AR_ALSA_PCM_DEVICE, snd_strerror (err));
          return OMX_ErrorInsufficientResources;
        }

      if ((err = snd_pcm_hw_params_malloc (&p_prc->p_hw_params)) < 0)
        {
          TIZ_LOGN (TIZ_ERROR, p_hdl,
                    "cannot allocate hardware parameter structure" " (%s)",
                    snd_strerror (err));
          return OMX_ErrorInsufficientResources;
        }

      p_prc->descriptor_count_ = snd_pcm_poll_descriptors_count (p_prc->p_pcm_hdl);
      if (p_prc->descriptor_count_ <= 0)
        {
          TIZ_LOGN (TIZ_ERROR, p_hdl,
                    "Invalid poll descriptors count");
          return OMX_ErrorInsufficientResources;
        }

      p_prc->p_fds_ = tiz_mem_alloc (sizeof (struct pollfd) * p_prc->descriptor_count_);
      if (p_prc->p_fds_ == NULL)
        {
          TIZ_LOGN (TIZ_ERROR, p_hdl,
                    "Could not allocate poll file descriptors\n");
          return -ENOMEM;
        }

      if (OMX_ErrorNone !=
          (rc = tiz_event_io_init (&(p_prc->p_ev_io_), p_hdl,
                                   tiz_comp_event_io)))
        {
          TIZ_LOGN (TIZ_ERROR, p_hdl, "[%s] : Error initializing "
                    "the PCM io event", tiz_err_to_str (rc));
          return OMX_ErrorInsufficientResources;
        }
    }

  assert (NULL != p_prc->p_pcm_hdl);

  /* Fill params with a full configuration space for the PCM. */
  if ((err = snd_pcm_hw_params_any (p_prc->p_pcm_hdl,
                                    p_prc->p_hw_params)) < 0)
    {
      TIZ_LOGN (TIZ_ERROR, p_hdl, "cannot initialize hardware parameter "
               "structure (%s)", snd_strerror (err));
      return OMX_ErrorInsufficientResources;
    }

  TIZ_LOGN (TIZ_DEBUG, p_hdl, "Device can pause [%s] ",
           snd_pcm_hw_params_can_pause(p_prc->p_hw_params) == 0 ?
           "NO" : "YES");

  TIZ_LOGN (TIZ_DEBUG, p_hdl, "Device can resume [%s] ",
           snd_pcm_hw_params_can_resume(p_prc->p_hw_params) == 0 ?
           "NO" : "YES");

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_prepare_to_transfer (void *ap_obj, OMX_U32 TIZ_UNUSED(a_pid))
{
  ar_prc_t *p_obj = ap_obj;

  assert (NULL != ap_obj);

  if (NULL != p_obj->p_pcm_hdl)
    {
      OMX_ERRORTYPE rc = OMX_ErrorNone;
      snd_pcm_format_t snd_pcm_format;
      OMX_HANDLETYPE p_hdl = tiz_srv_get_hdl (p_obj);
      void *p_krn = tiz_get_krn (p_hdl);
      int err = 0;

      /* Retrieve pcm params from port */
      p_obj->pcmmode.nSize = (OMX_U32) sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
      p_obj->pcmmode.nVersion.nVersion = (OMX_U32) OMX_VERSION;
      p_obj->pcmmode.nPortIndex = 0;    /* port index */
      if (OMX_ErrorNone != (rc = tiz_api_GetParameter
                            (p_krn, p_hdl,
                             OMX_IndexParamAudioPcm, &p_obj->pcmmode)))
        {
          TIZ_LOGN (TIZ_ERROR, p_hdl, "Error retrieving pcm params from port");
          return rc;
        }

      TIZ_LOGN (TIZ_NOTICE, p_hdl, "nChannels = [%d] nBitPerSample = [%d] "
               "nSamplingRate = [%d] eNumData = [%d] eEndian = [%d] "
               "bInterleaved = [%s] ePCMMode = [%d]",
               p_obj->pcmmode.nChannels,
               p_obj->pcmmode.nBitPerSample,
               p_obj->pcmmode.nSamplingRate,
               p_obj->pcmmode.eNumData,
               p_obj->pcmmode.eEndian,
               p_obj->pcmmode.bInterleaved == OMX_TRUE ? "OMX_TRUE" : "OMX_FALSE",
               p_obj->pcmmode.ePCMMode);

      /* TODO : Add function to properly encode snd_pcm_format */
      snd_pcm_format = p_obj->pcmmode.eEndian == OMX_EndianLittle ?
        SND_PCM_FORMAT_S16 : SND_PCM_FORMAT_S16_BE;

      /* This sets the hardware and software parameters in a convenient way. */
      if ((err = snd_pcm_set_params (p_obj->p_pcm_hdl,
                                     snd_pcm_format,
                                     SND_PCM_ACCESS_RW_INTERLEAVED,
                                     (unsigned int) p_obj->pcmmode.nChannels,
                                     (unsigned int) p_obj->pcmmode.nSamplingRate,
                                     1, /* allow alsa-lib resampling */
                                     100 * 1000 /* overall latency in us */
                                     )) < 0)
        {
          TIZ_LOGN (TIZ_ERROR, p_hdl, "Could not set the PCM params!");
          return OMX_ErrorInsufficientResources;
        }

      if ((err = snd_pcm_poll_descriptors(p_obj->p_pcm_hdl, p_obj->p_fds_,
                                          p_obj->descriptor_count_)) < 0)
        {
          TIZ_LOGN (TIZ_ERROR, p_hdl,
                    "Unable to obtain poll descriptors for playback: %s",
                    snd_strerror(err));
          return OMX_ErrorInsufficientResources;
        }

      TIZ_LOGN (TIZ_DEBUG, p_hdl, "Poll descriptors : %d",
                   p_obj->descriptor_count_);

      tiz_event_io_set (p_obj->p_ev_io_, p_obj->p_fds_->fd,
                        TIZ_EVENT_READ_OR_WRITE, true);

      /* OK, now prepare the PCM for use */
      if ((err = snd_pcm_prepare (p_obj->p_pcm_hdl)) < 0)
        {
          TIZ_LOGN (TIZ_ERROR, p_hdl,
                    "Could not prepare audio interface for use (%s)",
                    snd_strerror (err));
          return OMX_ErrorInsufficientResources;
        }
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_transfer_and_process (void * ap_obj,
                              OMX_U32 TIZ_UNUSED(a_pid))
{
  ar_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  p_prc->awaiting_buffers_ = true;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_stop_and_return (void * ap_obj)
{
  ar_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (p_prc),
            "stop_and_return");
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
      (void)snd_pcm_close (p_prc->p_pcm_hdl);
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
  TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (p_prc),
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
  TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (p_prc),
            "Received io event on fd [%d]", a_fd);
  stop_io_watcher (p_prc);
  return render_pcm_data (p_prc);
}

static OMX_ERRORTYPE
ar_prc_pause (const void *ap_obj)
{
  ar_prc_t *p_prc = (ar_prc_t *) ap_obj;
  int       pause = 1;
  assert (NULL != p_prc);
  snd_pcm_pause (p_prc->p_pcm_hdl, pause);
  TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (p_prc),
            "PAUSED ALSA device..."
            "awaiting_io_ev_ [%s]",
            p_prc->awaiting_io_ev_ ? "YES" : "NO");
  if (p_prc->awaiting_io_ev_ == true)
    {
      tiz_event_io_stop (p_prc->p_ev_io_);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_resume (const void *ap_obj)
{
  ar_prc_t *p_prc  = (ar_prc_t *) ap_obj;
  int       resume = 0;
  assert (NULL != p_prc);
  TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (p_prc),
            "RESUMING ALSA device...");
  snd_pcm_pause (p_prc->p_pcm_hdl, resume);
  if (p_prc->awaiting_io_ev_ == true)
    {
      start_io_watcher (p_prc);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_prc_port_flush (const void *ap_obj, OMX_U32 TIZ_UNUSED(a_pid))
{
  ar_prc_t *p_prc = (ar_prc_t *) ap_obj;
  assert (NULL != p_prc);
  return do_flush (p_prc);
}

static OMX_ERRORTYPE
ar_prc_port_disable (const void *ap_obj, OMX_U32 TIZ_UNUSED(a_pid))
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

/*
 * initialization
 */

const void *arprc;

OMX_ERRORTYPE
ar_prc_init (void)
{
  if (!arprc)
    {
      tiz_check_omx_err_ret_oom (tiz_prc_init ());
      tiz_check_null_ret_oom
        (arprc = 
         factory_new
         (tizprc_class,
          "arprc",
          tizprc,
          sizeof (ar_prc_t),
          ctor, ar_prc_ctor,
          dtor, ar_prc_dtor,
          tiz_srv_allocate_resources, ar_prc_allocate_resources,
          tiz_srv_deallocate_resources, ar_prc_deallocate_resources,
          tiz_srv_prepare_to_transfer, ar_prc_prepare_to_transfer,
          tiz_srv_transfer_and_process, ar_prc_transfer_and_process,
          tiz_srv_stop_and_return, ar_prc_stop_and_return,
          tiz_prc_buffers_ready, ar_prc_buffers_ready,
          tiz_prc_io_ready, ar_prc_io_ready,
          tiz_prc_pause, ar_prc_pause,
          tiz_prc_resume, ar_prc_resume,
          tiz_prc_port_flush, ar_prc_port_flush,
          tiz_prc_port_disable, ar_prc_port_disable,
          tiz_prc_port_enable, ar_prc_port_enable,
          0));
    }
  return OMX_ErrorNone;
}
