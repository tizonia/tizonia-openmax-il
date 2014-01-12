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
 * @file   mp3eprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Mp3 Encoder processor class
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mp3eprc.h"
#include "mp3eprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizosal.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.mp3_encoder.prc"
#endif

#define TIZ_LAME_MP3_ENC_MIN_BUFFER_SIZE 7200

static OMX_ERRORTYPE
release_buffers (const void *ap_obj)
{
  mp3e_prc_t *p_obj = (mp3e_prc_t *) ap_obj;
  void *p_krn = tiz_get_krn (handleOf (ap_obj));

  if (p_obj->p_inhdr_)
    {
      tiz_krn_release_buffer (p_krn, 0, p_obj->p_inhdr_);
      p_obj->p_inhdr_ = NULL;
    }

  if (p_obj->p_outhdr_)
    {
      tiz_krn_release_buffer (p_krn, 1, p_obj->p_outhdr_);
      p_obj->p_outhdr_ = NULL;
    }

  p_obj->frame_size_ = 0;
  if (false == p_obj->lame_flushed_)
    {
      if (NULL != p_obj->lame_)
        {
          OMX_U8 *p_buffer = NULL;

          if (NULL == (p_buffer = (OMX_U8 *) tiz_mem_alloc
                       ((size_t) TIZ_LAME_MP3_ENC_MIN_BUFFER_SIZE)))
            {
              return OMX_ErrorInsufficientResources;
            }

          (void) lame_encode_flush (p_obj->lame_, p_buffer,
                                    TIZ_LAME_MP3_ENC_MIN_BUFFER_SIZE);

          tiz_mem_free (p_buffer);
        }
      p_obj->lame_flushed_ = true;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
encode_buffer (const void *ap_obj)
{
  mp3e_prc_t *p_obj = (mp3e_prc_t *) ap_obj;
  int nsamples = 0;
  int encoded_bytes = 0;

  assert (p_obj->p_outhdr_);

  if (NULL != p_obj->p_inhdr_)
    {
      if ((p_obj->p_inhdr_->nFlags & OMX_BUFFERFLAG_EOS) != 0)
        {
          p_obj->eos_ = true;
        }

      nsamples = (p_obj->p_inhdr_->nFilledLen
                  / p_obj->pcmmode_.nChannels /
                  p_obj->pcmmode_.nBitPerSample) * 8;

      TIZ_TRACE (handleOf (ap_obj),
                "p_inhdr [%p] nsamples [%d] nFilledLen [%d] "
                "nChannels [%d] nBitPerSample [%d]"
                "nOffset [%d]", p_obj->p_inhdr_, nsamples,
                p_obj->p_inhdr_->nFilledLen, p_obj->pcmmode_.nChannels,
                p_obj->pcmmode_.nBitPerSample, p_obj->p_inhdr_->nOffset);

      if (0 > (encoded_bytes
               = lame_encode_buffer_interleaved
               (p_obj->lame_,
                (short int *) (p_obj->p_inhdr_->pBuffer
                               + p_obj->p_inhdr_->nOffset),
                nsamples,
                p_obj->p_outhdr_->pBuffer
                + p_obj->p_outhdr_->nOffset,
                p_obj->p_outhdr_->nAllocLen - p_obj->p_outhdr_->nFilledLen)))
        {
          if (encoded_bytes == -1)
            {
              TIZ_TRACE (handleOf (ap_obj),
                        "Output buffer is not big enough... [%d]...",
                        p_obj->p_outhdr_->nAllocLen -
                        p_obj->p_outhdr_->nFilledLen);
            }
          else
            {
              TIZ_TRACE (handleOf (ap_obj),
                        "Some error occurred during encoding [%d]...",
                        encoded_bytes);
            }

        }
      else
        {
          if (p_obj->frame_size_ == 0)
            {
              p_obj->frame_size_ = encoded_bytes;
            }

          p_obj->p_inhdr_->nFilledLen = 0;
          p_obj->p_outhdr_->nFilledLen += encoded_bytes;
          p_obj->p_outhdr_->nOffset += encoded_bytes;
        }

      if (p_obj->frame_size_ >
          (p_obj->p_outhdr_->nAllocLen - p_obj->p_outhdr_->nFilledLen)
          || encoded_bytes == -1)
        {
          void *p_krn = tiz_get_krn (handleOf (ap_obj));
          p_obj->p_outhdr_->nOffset = 0;
          tiz_krn_release_buffer (p_krn, 1, p_obj->p_outhdr_);
          p_obj->p_outhdr_ = NULL;
        }

    }                           /* if (NULL != p_obj->p_inhdr_) */

  if (true == p_obj->eos_)
    {
      if (NULL != p_obj->p_outhdr_)
        {
          /* may return one more mp3 frames */
          encoded_bytes = lame_encode_flush (p_obj->lame_,
                                             p_obj->p_outhdr_->pBuffer
                                             + p_obj->p_outhdr_->nOffset,
                                             p_obj->p_outhdr_->nAllocLen -
                                             p_obj->p_outhdr_->nFilledLen);
          p_obj->p_outhdr_->nFilledLen += encoded_bytes;
          p_obj->p_outhdr_->nOffset += encoded_bytes;
          p_obj->lame_flushed_ = true;
        }
    }

  return OMX_ErrorNone;
}

static void
lame_debugf (const char *format, va_list ap)
{
  /* TODO */
  (void) vfprintf (stdout, format, ap);
}

static bool
claim_input (const void *ap_obj)
{
  mp3e_prc_t *p_obj = (mp3e_prc_t *) ap_obj;
  tiz_pd_set_t ports;
  void *p_krn = tiz_get_krn (handleOf (ap_obj));

  TIZ_PD_ZERO (&ports);
  tiz_check_omx_err (tiz_krn_select (p_krn, 2, &ports));

  /* We need one input buffers */
  if (TIZ_PD_ISSET (0, &ports))
    {
      tiz_check_omx_err (tiz_krn_claim_buffer (p_krn, 0, 0,
                                                 &p_obj->p_inhdr_));
      return true;
    }

  return false;
}

static bool
claim_output (const void *ap_obj)
{
  mp3e_prc_t *p_obj = (mp3e_prc_t *) ap_obj;
  tiz_pd_set_t ports;
  void *p_krn = tiz_get_krn (handleOf (ap_obj));

  TIZ_PD_ZERO (&ports);
  tiz_check_omx_err (tiz_krn_select (p_krn, 2, &ports));

  /* We need one output buffers */
  if (TIZ_PD_ISSET (1, &ports))
    {
      tiz_check_omx_err (tiz_krn_claim_buffer (p_krn, 1, 0,
                                                 &p_obj->p_outhdr_));
      TIZ_TRACE (handleOf (ap_obj),
                "Claimed OUTPUT HEADER [%p] BUFFER [%p] nFilledLen [%d]...",
                p_obj->p_outhdr_,
                p_obj->p_outhdr_->pBuffer, p_obj->p_outhdr_->nFilledLen);
      p_obj->p_outhdr_->nFilledLen = 0;
      p_obj->p_outhdr_->nOffset = 0;
      return true;
    }

  return false;
}

static OMX_ERRORTYPE
set_lame_pcm_settings (void *ap_obj, OMX_HANDLETYPE ap_hdl, void *ap_krn)
{
  mp3e_prc_t *p_obj = ap_obj;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;

  assert (ap_obj);
  assert (ap_hdl);
  assert (ap_krn);

  /* Retrieve pcm params from port */
  p_obj->pcmmode_.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  p_obj->pcmmode_.nVersion.nVersion = OMX_VERSION;
  p_obj->pcmmode_.nPortIndex = 0;
  if (OMX_ErrorNone != (ret_val = tiz_api_GetParameter
                        (ap_krn, ap_hdl,
                         OMX_IndexParamAudioPcm, &p_obj->pcmmode_)))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : Error retrieving pcm params from port",
               tiz_err_to_str (ret_val));
      return ret_val;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nChannels = [%d] nBitPerSample = [%d] "
           "nSamplingRate = [%d] eNumData = [%d] eEndian = [%d] "
           "bInterleaved = [%s] ePCMMode = [%d]",
           p_obj->pcmmode_.nChannels,
           p_obj->pcmmode_.nBitPerSample,
           p_obj->pcmmode_.nSamplingRate,
           p_obj->pcmmode_.eNumData,
           p_obj->pcmmode_.eEndian,
           p_obj->pcmmode_.bInterleaved ? "OMX_TRUE" : "OMX_FALSE",
           p_obj->pcmmode_.ePCMMode);

  return ret_val;
}

static OMX_ERRORTYPE
set_lame_mp3_settings (void *ap_obj, OMX_HANDLETYPE ap_hdl, void *ap_krn)
{
  mp3e_prc_t *p_obj = ap_obj;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;
  int lame_mode = 0;

  assert (ap_obj);
  assert (ap_hdl);
  assert (ap_krn);

  /* Retrieve mp3 params from port */
  p_obj->mp3type_.nSize = sizeof (OMX_AUDIO_PARAM_MP3TYPE);
  p_obj->mp3type_.nVersion.nVersion = OMX_VERSION;
  p_obj->mp3type_.nPortIndex = 1;
  if (OMX_ErrorNone != (ret_val = tiz_api_GetParameter
                        (ap_krn, ap_hdl,
                         OMX_IndexParamAudioMp3, &p_obj->mp3type_)))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : Error retrieving mp3 params from port",
               tiz_err_to_str (ret_val));
      return ret_val;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nChannels = [%d] nBitRate = [%d] "
           "nSampleRate = [%d] nAudioBandWidth = [%d] eChannelMode = [%d] "
           "eFormat = [%d]",
           p_obj->mp3type_.nChannels,
           p_obj->mp3type_.nBitRate,
           p_obj->mp3type_.nSampleRate,
           p_obj->mp3type_.nAudioBandWidth,
           p_obj->mp3type_.eChannelMode, p_obj->mp3type_.eFormat);

  (void) lame_set_num_channels (p_obj->lame_, p_obj->mp3type_.nChannels);
  (void) lame_set_in_samplerate (p_obj->lame_, p_obj->mp3type_.nSampleRate);
  (void) lame_set_brate (p_obj->lame_, p_obj->mp3type_.nBitRate);

  switch (p_obj->mp3type_.eChannelMode)
    {
    case OMX_AUDIO_ChannelModeStereo:
      {
        lame_mode = 0;
      }
      break;
    case OMX_AUDIO_ChannelModeJointStereo:
      {
        lame_mode = 1;
      }
      break;
    case OMX_AUDIO_ChannelModeDual:
      {
        /* Not supported, default to 0 (stereo) */
        lame_mode = 0;
      }
      break;
    case OMX_AUDIO_ChannelModeMono:
      {
        lame_mode = 3;
      }
      break;

    default:
      {
        lame_mode = 3;
      }
    };

  (void) lame_set_mode (p_obj->lame_, lame_mode);
  (void) lame_set_quality (p_obj->lame_, 2);    /* 2=high  5 = medium  7=low */

  return ret_val;
}

/*
 * mp3eprc
 */

static void *
mp3e_proc_ctor (void *ap_obj, va_list * app)
{
  mp3e_prc_t *p_obj = super_ctor (typeOf (ap_obj, "mp3eprc"), ap_obj, app);
  p_obj->lame_ = NULL;
  p_obj->frame_size_ = 0;
  p_obj->p_inhdr_ = 0;
  p_obj->p_outhdr_ = 0;
  p_obj->eos_ = false;
  p_obj->lame_flushed_ = true;
  return p_obj;
}

static void *
mp3e_proc_dtor (void *ap_obj)
{
  mp3e_prc_t *p_obj = ap_obj;

  if (NULL != p_obj->lame_)
    {
      lame_close (p_obj->lame_);
      p_obj->lame_ = NULL;
    }

  return super_dtor (typeOf (ap_obj, "mp3eprc"), ap_obj);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
mp3e_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  mp3e_prc_t *p_obj = ap_obj;
  assert (ap_obj);

  if (NULL == (p_obj->lame_ = lame_init ()))
    {
      TIZ_ERROR (handleOf (ap_obj),
                "[OMX_ErrorInsufficientResources] : "
                "lame encoder initialization error");
      return OMX_ErrorInsufficientResources;
    }

  TIZ_TRACE (handleOf (ap_obj),
            "lame encoder version [%s]", get_lame_version ());

  (void) lame_set_errorf (p_obj->lame_, lame_debugf);
  (void) lame_set_debugf (p_obj->lame_, lame_debugf);
  (void) lame_set_msgf (p_obj->lame_, lame_debugf);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3e_proc_deallocate_resources (void *ap_obj)
{
  mp3e_prc_t *p_obj = ap_obj;
  assert (ap_obj);

  if (NULL != p_obj->lame_)
    {
      lame_close (p_obj->lame_);
      p_obj->lame_ = NULL;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3e_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  mp3e_prc_t *p_obj = ap_obj;
  void *p_krn = NULL;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;

  assert (ap_obj);

  p_krn = tiz_get_krn (handleOf (ap_obj));
  assert (p_krn);

  if (NULL == p_obj->lame_)
    {
      return OMX_ErrorNone;
    }

  if (OMX_ErrorNone != (ret_val = set_lame_mp3_settings (p_obj,
                                                         handleOf (ap_obj),
                                                         p_krn)))
    {
      return ret_val;
    }

  if (OMX_ErrorNone != (ret_val = set_lame_pcm_settings (p_obj,
                                                         handleOf (ap_obj),
                                                         p_krn)))
    {
      return ret_val;
    }

  if (-1 == lame_init_params (p_obj->lame_))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorInsufficientResources] : "
               "Error returned by lame during initialization.");
      return OMX_ErrorInsufficientResources;
    }

  p_obj->lame_flushed_ = false;

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3e_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  assert (ap_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3e_proc_stop_and_return (void *ap_obj)
{
  mp3e_prc_t *p_obj = ap_obj;
  return release_buffers (p_obj);
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
mp3e_proc_buffers_ready (const void *ap_obj)
{
  mp3e_prc_t *p_obj = (mp3e_prc_t *) ap_obj;
  void *p_krn = tiz_get_krn (handleOf (ap_obj));

  while (1)
    {

      if (!p_obj->p_inhdr_)
        {
          if (!claim_input (ap_obj) || (!p_obj->p_inhdr_))
            {
              break;
            }
        }

      if (!p_obj->p_outhdr_)
        {
          if (!claim_output (ap_obj))
            {
              break;
            }
        }

      tiz_check_omx_err (encode_buffer (ap_obj));
      if (p_obj->p_inhdr_ && (0 == p_obj->p_inhdr_->nFilledLen))
        {
          p_obj->p_inhdr_->nOffset = 0;
          tiz_krn_release_buffer (p_krn, 0, p_obj->p_inhdr_);
          p_obj->p_inhdr_ = NULL;
        }
    }

  if (p_obj->eos_ && p_obj->lame_flushed_ && p_obj->p_outhdr_)
    {
      /* EOS has been received and all the input data has been consumed
       * already, so its time to propagate the EOS flag */
      TIZ_TRACE (handleOf (ap_obj),
                "p_obj->eos OUTPUT HEADER [%p]...", p_obj->p_outhdr_);
      p_obj->p_outhdr_->nFlags |= OMX_BUFFERFLAG_EOS;
      tiz_krn_release_buffer (p_krn, 1, p_obj->p_outhdr_);
      p_obj->p_outhdr_ = NULL;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3e_proc_port_flush (const void *ap_obj, OMX_U32 a_pid)
{
  mp3e_prc_t *p_obj = (mp3e_prc_t *) ap_obj;
  /* Release all buffers, regardless of the port this is received on */
  return release_buffers (p_obj);
}

static OMX_ERRORTYPE
mp3e_proc_port_disable (const void *ap_obj, OMX_U32 a_pid)
{
  mp3e_prc_t *p_obj = (mp3e_prc_t *) ap_obj;
  /* Release all buffers, regardless of the port this is received on */
  return release_buffers (p_obj);
}

static OMX_ERRORTYPE
mp3e_proc_port_enable (const void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

/*
 * mp3e_prc_class
 */

static void *
mp3e_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "mp3eprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
mp3e_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * mp3eprc_class = factory_new (classOf (tizprc),
                                      "mp3eprc_class",
                                      classOf (tizprc),
                                      sizeof (mp3e_prc_class_t),
                                      ap_tos, ap_hdl,
                                      ctor, mp3e_prc_class_ctor, 0);
  return mp3eprc_class;
}

void *
mp3e_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * mp3eprc_class = tiz_get_type (ap_hdl, "mp3eprc_class");
  TIZ_LOG_CLASS (mp3eprc_class);
  void * mp3eprc =
    factory_new
    (mp3eprc_class,
     "mp3eprc",
     tizprc,
     sizeof (mp3e_prc_t),
     ap_tos, ap_hdl,
     ctor, mp3e_proc_ctor,
     dtor, mp3e_proc_dtor,
     tiz_srv_allocate_resources, mp3e_proc_allocate_resources,
     tiz_srv_deallocate_resources, mp3e_proc_deallocate_resources,
     tiz_srv_prepare_to_transfer, mp3e_proc_prepare_to_transfer,
     tiz_srv_transfer_and_process, mp3e_proc_transfer_and_process,
     tiz_srv_stop_and_return, mp3e_proc_stop_and_return,
     tiz_prc_buffers_ready, mp3e_proc_buffers_ready,
     tiz_prc_port_flush, mp3e_proc_port_flush,
     tiz_prc_port_disable, mp3e_proc_port_disable,
     tiz_prc_port_enable, mp3e_proc_port_enable, 0);

  return mp3eprc;
}
