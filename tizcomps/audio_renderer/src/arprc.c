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

#include <assert.h>

#include "tizkernel.h"
#include "tizscheduler.h"

#include "arprc.h"
#include "arprc_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.audio_renderer.prc"
#endif

#define ALSA_PLAYBACK_DEVICE "null"

/*
 * arprc
 */

static void *
ar_proc_ctor (void *ap_obj, va_list * app)
{
  struct arprc *p_obj = super_ctor (arprc, ap_obj, app);
  TIZ_LOG (TIZ_LOG_TRACE, "Constructing arprc...[%p]", p_obj);
  return p_obj;
}

static void *
ar_proc_dtor (void *ap_obj)
{
  struct arprc *p_obj = ap_obj;
  TIZ_LOG (TIZ_LOG_TRACE, "Destructing arprc...[%p]", p_obj);
  return super_dtor (arprc, ap_obj);
}

static OMX_ERRORTYPE
ar_proc_render_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
{
  const struct arprc *p_obj = ap_obj;
  int err, offset = 0;
  int samples, step;

  TIZ_LOG (TIZ_LOG_TRACE,
             "Rendering HEADER [%p]...nFilledLen[%d] !!!", p_hdr,
             p_hdr->nFilledLen);

  step = (p_obj->pcmmode.nBitPerSample / 8) * p_obj->pcmmode.nChannels;
  samples = p_hdr->nFilledLen / step;
  TIZ_LOG (TIZ_LOG_TRACE, "step [%d], samples [%d]", step, samples);

  while (samples)
    {
      err = snd_pcm_writei (p_obj->p_playback_hdl,
                            p_hdr->pBuffer + p_hdr->nOffset + offset,
                            samples);
      TIZ_LOG (TIZ_LOG_TRACE,
                 "Rendering HEADER [%p]..." "err [%d] samples [%d]",
                 p_hdr, err, samples);
      if (-EAGAIN == err)
        {
          TIZ_LOG (TIZ_LOG_TRACE, "Rendering HEADER [%p]...-EAGAIN");
          continue;
        }

      if (err < 0)
        {
          TIZ_LOG (TIZ_LOG_TRACE,
                     "Rendering HEADER [%p]...underflow");
          err = snd_pcm_recover (p_obj->p_playback_hdl, err, 0);
          if (err < 0)
            {
              TIZ_LOG (TIZ_LOG_TRACE, "snd_pcm_recover error: %s", 
                         snd_strerror (err));
              break;
            }
        }

      offset += err * step;
      samples -= err;

    }

  p_hdr->nFilledLen = 0;

  return OMX_ErrorNone;

}

/*
 * from tizservant class
 */

static OMX_ERRORTYPE
ar_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  struct arprc *p_obj = ap_obj;
  int err;
  assert (ap_obj);

  if (!(p_obj->p_playback_hdl))
    {
      if ((err = snd_pcm_open (&p_obj->p_playback_hdl, ALSA_PLAYBACK_DEVICE,
                               SND_PCM_STREAM_PLAYBACK, 0)) < 0)
        {
          TIZ_LOG (TIZ_LOG_TRACE, "cannot open audio device %s (%s)",
                     ALSA_PLAYBACK_DEVICE, snd_strerror (err));
          return OMX_ErrorInsufficientResources;
        }

      if ((err = snd_pcm_hw_params_malloc (&p_obj->p_hw_params)) < 0)
        {
          TIZ_LOG (TIZ_LOG_TRACE,
                     "cannot allocate hardware parameter structure" " (%s)",
                     snd_strerror (err));
          return OMX_ErrorInsufficientResources;
        }
    }

  if ((err = snd_pcm_hw_params_any (p_obj->p_playback_hdl,
                                    p_obj->p_hw_params)) < 0)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "cannot initialize hardware parameter "
                 "structure (%s)", snd_strerror (err));
      return OMX_ErrorInsufficientResources;
    }

  TIZ_LOG (TIZ_LOG_TRACE, "Resource allocation complete... "
             "arprc = [%p]!!!", p_obj);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_proc_deallocate_resources (void *ap_obj)
{
  struct arprc *p_obj = ap_obj;
  assert (ap_obj);

  if (p_obj->p_hw_params)
    {
      snd_pcm_hw_params_free (p_obj->p_hw_params);
      snd_pcm_close (p_obj->p_playback_hdl);
      p_obj->p_playback_hdl = NULL;
      p_obj->p_hw_params = NULL;
    }

  TIZ_LOG (TIZ_LOG_TRACE, "Resource deallocation complete..."
             "arprc = [%p]!!!", p_obj);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ar_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  struct arprc *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;
  void *p_krn = tiz_get_krn (p_parent->p_hdl_);
  int err;
  snd_pcm_format_t snd_pcm_format;
  assert (ap_obj);

  TIZ_LOG (TIZ_LOG_TRACE, "pid [%d]", a_pid);

  /* Retrieve pcm params from port */
  p_obj->pcmmode.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  p_obj->pcmmode.nVersion.nVersion = OMX_VERSION;
  p_obj->pcmmode.nPortIndex = 0; /* port index */
  if (OMX_ErrorNone != (ret_val = tizapi_GetParameter
                        (p_krn,
                         p_parent->p_hdl_,
                         OMX_IndexParamAudioPcm, &p_obj->pcmmode)))
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Error retrieving pcm params from port");
      return ret_val;
    }

  TIZ_LOG (TIZ_LOG_TRACE, "nChannels = [%d] nBitPerSample = [%d] "
             "nSamplingRate = [%d] eNumData = [%d] eEndian = [%d] "
             "bInterleaved = [%s] ePCMMode = [%d]",
             p_obj->pcmmode.nChannels,
             p_obj->pcmmode.nBitPerSample,
             p_obj->pcmmode.nSamplingRate,
             p_obj->pcmmode.eNumData,
             p_obj->pcmmode.eEndian,
             p_obj->pcmmode.bInterleaved ? "OMX_TRUE" : "OMX_FALSE",
             p_obj->pcmmode.ePCMMode);

  /* TODO : Add function to encode properly encode snd_pcm_format */
  snd_pcm_format = p_obj->pcmmode.eEndian == OMX_EndianLittle ?
    SND_PCM_FORMAT_S16 : SND_PCM_FORMAT_S16_BE;

  if ((err = snd_pcm_set_params (p_obj->p_playback_hdl,
                                 snd_pcm_format,
                                 SND_PCM_ACCESS_RW_INTERLEAVED,
                                 p_obj->pcmmode.nChannels,
                                 p_obj->pcmmode.nSamplingRate,
                                 1, 100 * 1000)) < 0)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Didn' work...p_obj = [ERROR]!!!");
    }

  if ((err = snd_pcm_prepare (p_obj->p_playback_hdl)) < 0)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Cannot prepare audio interface for use "
                 "(%s)", snd_strerror (err));
      return OMX_ErrorInsufficientResources;
    }

  TIZ_LOG (TIZ_LOG_TRACE, "Transfering buffers...p_obj = [%p]!!!",
             p_obj);

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
ar_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  struct arprc *p_obj = ap_obj;
  assert (ap_obj);

  TIZ_LOG (TIZ_LOG_TRACE, "Awaiting buffers...p_obj = [%p]!!!", p_obj);

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
ar_proc_stop_and_return (void *ap_obj)
{
  struct arprc *p_obj = ap_obj;
  assert (ap_obj);

  TIZ_LOG (TIZ_LOG_TRACE, "Stopped buffer transfer...p_obj = [%p]!!!",
             p_obj);

  return OMX_ErrorNone;
}

/*
 * from tizproc class
 */

static OMX_ERRORTYPE
ar_proc_buffers_ready (const void *ap_obj)
{
  const struct tizservant *p_parent = ap_obj;
  tiz_pd_set_t ports;
  void *p_krn = tiz_get_krn (p_parent->p_hdl_);
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  TIZ_LOG (TIZ_LOG_TRACE, "Buffers ready...");

  TIZ_PD_ZERO (&ports);

  TIZ_UTIL_TEST_ERR (tizkernel_select (p_krn, 1, &ports));

  if (TIZ_PD_ISSET (0, &ports))
    {
      TIZ_UTIL_TEST_ERR (tizkernel_claim_buffer (p_krn, 0, 0, &p_hdr));
      TIZ_LOG (TIZ_LOG_TRACE, "Claimed HEADER [%p]...", p_hdr);
      TIZ_UTIL_TEST_ERR (ar_proc_render_buffer (ap_obj, p_hdr));
      if (p_hdr->nFlags & OMX_BUFFERFLAG_EOS)
        {
          TIZ_LOG (TIZ_LOG_TRACE,
                     "OMX_BUFFERFLAG_EOS in HEADER [%p]", p_hdr);
          tizservant_issue_event ((OMX_PTR) ap_obj,
                                  OMX_EventBufferFlag,
                                  0, p_hdr->nFlags, NULL);
        }
      tizkernel_relinquish_buffer (p_krn, 0, p_hdr);
    }

  return OMX_ErrorNone;
}

/*
 * initialization
 */

const void *arprc;

void
init_arprc (void)
{

  if (!arprc)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Initializing arprc...");
      init_tizproc ();
      arprc =
        factory_new
        (tizproc_class,
         "arprc",
         tizproc,
         sizeof (struct arprc),
         ctor, ar_proc_ctor,
         dtor, ar_proc_dtor,
         tizservant_allocate_resources, ar_proc_allocate_resources,
         tizservant_deallocate_resources, ar_proc_deallocate_resources,
         tizservant_prepare_to_transfer, ar_proc_prepare_to_transfer,
         tizservant_transfer_and_process, ar_proc_transfer_and_process,
         tizservant_stop_and_return, ar_proc_stop_and_return,
         tizproc_buffers_ready, ar_proc_buffers_ready,
         0);
    }

}
