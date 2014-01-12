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
 * @file   tizpcmport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - pcmport class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizpcmport.h"
#include "tizpcmport_decls.h"
#include "tizutils.h"

#include "tizosal.h"

#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.pcmport"
#endif

/*
 * tizpcmport class
 */

static void *
pcmport_ctor (void *ap_obj, va_list * app)
{
  tiz_pcmport_t *p_obj = super_ctor (typeOf (ap_obj, "tizpcmport"), ap_obj, app);
  tiz_port_t *p_base = ap_obj;
  OMX_AUDIO_PARAM_PCMMODETYPE *p_pcmmode = NULL;
  OMX_AUDIO_CONFIG_VOLUMETYPE *p_volume = NULL;
  OMX_AUDIO_CONFIG_MUTETYPE *p_mute = NULL;

  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexParamAudioPcm));
  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexConfigAudioVolume));
  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexConfigAudioMute));

  /* Initialize the OMX_AUDIO_PARAM_PCMMODETYPE structure */
  if ((p_pcmmode = va_arg (*app, OMX_AUDIO_PARAM_PCMMODETYPE *)))
    {
      int i = 0;
      (void) tiz_mem_set (&p_obj->pcmmode_, 0, sizeof p_obj->pcmmode_);
      p_obj->pcmmode_ = *p_pcmmode;
      for (i = 0; i < OMX_AUDIO_MAXCHANNELS; ++i)
        {
          p_obj->pcmmode_.eChannelMapping[i] = p_pcmmode->eChannelMapping[i];
        }

      TIZ_TRACE (handleOf (ap_obj),
                "nChannels = [%d]", p_obj->pcmmode_.nChannels);
      TIZ_TRACE (handleOf (ap_obj),
                "nBitPerSample = [%d]",
                p_obj->pcmmode_.nBitPerSample);
      TIZ_TRACE (handleOf (ap_obj),
                "nSamplingRate = [%d]",
                p_obj->pcmmode_.nSamplingRate);
    }

  /* Initialize the OMX_AUDIO_CONFIG_VOLUMETYPE structure */
  if ((p_volume = va_arg (*app, OMX_AUDIO_CONFIG_VOLUMETYPE *)))
    {
      p_obj->volume_ = *p_volume;
    }

  /* Initialize the OMX_AUDIO_CONFIG_MUTETYPE structure */
  if ((p_mute = va_arg (*app, OMX_AUDIO_CONFIG_MUTETYPE *)))
    {
      p_obj->mute_ = *p_mute;
    }

  /* TODO: Extract this from the va_list */
  p_base->portdef_.eDomain = OMX_PortDomainAudio;
  /* NOTE: MIME type is gone in 1.2 */
  p_base->portdef_.format.audio.pNativeRender = 0;
  p_base->portdef_.format.audio.bFlagErrorConcealment = OMX_FALSE;
  p_base->portdef_.format.audio.eEncoding = OMX_AUDIO_CodingPCM;

  return p_obj;

}

static void *
pcmport_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tizpcmport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
pcmport_GetParameter (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_pcmport_t *p_obj = ap_obj;

  TIZ_TRACE (ap_hdl, "GetParameter [%s]...",
            tiz_idx_to_str (a_index));
  assert (NULL != ap_obj);

  switch (a_index)
    {
    case OMX_IndexParamAudioPcm:
      {
        int i = 0;
        OMX_AUDIO_PARAM_PCMMODETYPE *p_pcmmode
          = (OMX_AUDIO_PARAM_PCMMODETYPE *) ap_struct;
        *p_pcmmode = p_obj->pcmmode_;
        for (i = 0; i < OMX_AUDIO_MAXCHANNELS; ++i)
        {
          p_pcmmode->eChannelMapping[i] = p_obj->pcmmode_.eChannelMapping[i];
        }
        break;
      }

    default:
      {
        /* Try the parent's indexes */
        return super_GetParameter (typeOf (ap_obj, "tizpcmport"),
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
pcmport_SetParameter (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_pcmport_t *p_obj = (tiz_pcmport_t *) ap_obj;

  TIZ_TRACE (ap_hdl, "PORT [%d] SetParameter [%s]...",
            tiz_port_dir (p_obj), tiz_idx_to_str (a_index));
  assert (NULL != ap_obj);

  switch (a_index)
    {
    case OMX_IndexParamAudioPcm:
      {
        OMX_S32 i = 0;
        const OMX_AUDIO_PARAM_PCMMODETYPE *p_pcmmode
          = (OMX_AUDIO_PARAM_PCMMODETYPE *) ap_struct;

        switch (p_pcmmode->nSamplingRate)
          {
          case 8000:
          case 11025:
          case 12000:
          case 16000:
          case 22050:
          case 24000:
          case 32000:
          case 44100:
          case 48000:
            {
              break;
            }
          default:
            {
              TIZ_ERROR (ap_hdl, "[OMX_ErrorBadParameter] : PORT [%d] "
                        "SetParameter [%s]... Invalid sampling rate [%d]",
                        tiz_port_dir (p_obj), tiz_idx_to_str (a_index),
                        p_pcmmode->nSamplingRate);
              return OMX_ErrorBadParameter;
            }
          };

        switch (p_pcmmode->nBitPerSample)
          {
          case 8:
          case 16:
            {
              break;
            }
          default:
            {
              TIZ_ERROR (ap_hdl, "[OMX_ErrorBadParameter] : PORT [%d] "
                        "SetParameter [%s]... Invalid bits per sample [%d]",
                        tiz_port_dir (p_obj), tiz_idx_to_str (a_index),
                        p_pcmmode->nBitPerSample);
              return OMX_ErrorBadParameter;
            }
          };

        /* Do now allow changes to sampling rate, num of channels or bits per
         * sample if this is a slave output port */
        {
          const tiz_port_t *p_base = ap_obj;

          if ((OMX_DirOutput == p_base->portdef_.eDir)
              && (p_base->opts_.mos_port != (OMX_U32) -1)
              && (p_base->opts_.mos_port != p_base->portdef_.nPortIndex))
/*               && (p_obj->pcmmode_.nChannels != p_pcmmode->nChannels */
/*                   || p_obj->pcmmode_.nBitPerSample != p_pcmmode->nBitPerSample */
/*                   || p_obj->pcmmode_.nSamplingRate != */
/*                   p_pcmmode->nSamplingRate)) */
            {
              TIZ_ERROR (ap_hdl, "[OMX_ErrorBadParameter] : PORT [%d] "
                        "SetParameter [OMX_IndexParamAudioPcm]... "
                        "Slave port, cannot update sample rate "
                        "bits per sample or number of channels", tiz_port_dir (p_obj));
              return OMX_ErrorBadParameter;
            }
        }

        /* Apply the new default values */
        p_obj->pcmmode_.nChannels     = p_pcmmode->nChannels;
        p_obj->pcmmode_.eNumData      = p_pcmmode->eNumData;
        p_obj->pcmmode_.eEndian       = p_pcmmode->eEndian;
        p_obj->pcmmode_.bInterleaved  = p_pcmmode->bInterleaved;
        p_obj->pcmmode_.nBitPerSample = p_pcmmode->nBitPerSample;
        p_obj->pcmmode_.nSamplingRate = p_pcmmode->nSamplingRate;
        p_obj->pcmmode_.ePCMMode      = p_pcmmode->ePCMMode;

        for (i = 0; i < OMX_AUDIO_MAXCHANNELS; ++i)
          {
            p_obj->pcmmode_.eChannelMapping[i]
              = p_pcmmode->eChannelMapping[i];
          }

      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_SetParameter (typeOf (ap_obj, "tizpcmport"),
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
pcmport_GetConfig (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_pcmport_t *p_obj = ap_obj;

  switch (a_index)
    {

    case OMX_IndexConfigAudioVolume:
      {
        OMX_AUDIO_CONFIG_VOLUMETYPE *p_volume
          = (OMX_AUDIO_CONFIG_VOLUMETYPE *) ap_struct;

        *p_volume = p_obj->volume_;
      }
      break;

    case OMX_IndexConfigAudioMute:
      {
        OMX_AUDIO_CONFIG_MUTETYPE *p_mute
          = (OMX_AUDIO_CONFIG_MUTETYPE *) ap_struct;

        *p_mute = p_obj->mute_;
      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_GetConfig (typeOf (ap_obj, "tizpcmport"),
                                ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
pcmport_SetConfig (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_pcmport_t *p_obj = (tiz_pcmport_t *) ap_obj;

  TIZ_TRACE (ap_hdl, "PORT [%d] SetConfig [%s]...",
            tiz_port_dir (p_obj), tiz_idx_to_str (a_index));
  assert (NULL != ap_obj);

  switch (a_index)
    {

    case OMX_IndexConfigAudioVolume:
      {
        /* TODO: Delegate this to the processor */
        const OMX_AUDIO_CONFIG_VOLUMETYPE *p_volume
          = (OMX_AUDIO_CONFIG_VOLUMETYPE *) ap_struct;

        /* Apply the new default values */
        if (p_obj->volume_.bLinear != p_volume->bLinear ||
            p_obj->volume_.sVolume.nValue != p_volume->sVolume.nValue ||
            p_obj->volume_.sVolume.nMin != p_volume->sVolume.nMin ||
            p_obj->volume_.sVolume.nMax != p_volume->sVolume.nMax)
          {
            p_obj->volume_.bLinear = p_volume->bLinear;
            p_obj->volume_.sVolume.nValue = p_volume->sVolume.nValue;
            p_obj->volume_.sVolume.nMin = p_volume->sVolume.nMin;
            p_obj->volume_.sVolume.nMax = p_volume->sVolume.nMax;
          }

      }
      break;

    case OMX_IndexConfigAudioMute:
      {

        /* TODO: Delegate this to the processor */
        const OMX_AUDIO_CONFIG_MUTETYPE *p_mute
          = (OMX_AUDIO_CONFIG_MUTETYPE *) ap_struct;

        if (p_obj->mute_.bMute != p_mute->bMute)
          {
            p_obj->mute_.bMute = p_mute->bMute;
          }

      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_SetConfig (typeOf (ap_obj, "tizpcmport"),
                                ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
  pcmport_set_portdef_format
  (void *ap_obj, const OMX_PARAM_PORTDEFINITIONTYPE * ap_pdef)
{
  /* TODO */
  return OMX_ErrorNone;
}

static bool
pcmport_check_tunnel_compat (const void *ap_obj,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t *p_obj = (tiz_port_t *) ap_obj;

  assert (NULL != ap_this_def);
  assert (NULL != ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_ERROR (handleOf (ap_obj),
                "PORT [%d] : Audio domain not found, instead found domain [%d]",
                p_obj->pid_, ap_other_def->eDomain);
      return false;
    }

  /* INFO: */
  /* This is not specified in the spec, but a binary audio reader */
  /* could use OMX_AUDIO_CodingUnused as a means to singal "any" format */

  if (ap_other_def->format.audio.eEncoding != OMX_AUDIO_CodingUnused)
    {
      if (ap_other_def->format.audio.eEncoding != OMX_AUDIO_CodingPCM)
        {
          TIZ_ERROR (handleOf (ap_obj),
                    "PORT [%d] : "
                    "PCM encoding not found, instead foudn encoding [%d]",
                    p_obj->pid_, ap_other_def->format.audio.eEncoding);
          return false;
        }
    }

  TIZ_TRACE (handleOf (ap_obj),
            "PORT [%d] check_tunnel_compat [OK]", p_obj->pid_);

  return true;
}

static OMX_ERRORTYPE
pcmport_apply_slaving_behaviour (void *ap_obj, void *ap_mos_port,
                                 const OMX_INDEXTYPE a_index,
                                 const OMX_PTR ap_struct,
                                 tiz_vector_t * ap_changed_idxs)
{
  tiz_pcmport_t *p_obj = ap_obj;
  tiz_port_t *p_base = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  /* OpenMAX IL 1.2 Section 3.5 : Slaving behaviour for nSamplingRate and
   * nChannels, both in OMX_AUDIO_PARAM_PCMMODETYPE. nBufferSize may also
   * change if the current value is not big enough to fit 5 ms of PCM audio
   * data - see OpenMAX IL 1.2 section 4.1.2. */

  assert (NULL != p_obj);
  assert (NULL != ap_struct);
  assert (NULL != ap_changed_idxs);

  {
    OMX_U32 new_min_buf_sz = 0;
    OMX_U32 new_rate = p_obj->pcmmode_.nSamplingRate;
    OMX_U32 new_channels = p_obj->pcmmode_.nChannels;
    OMX_U32 new_bps = p_obj->pcmmode_.nBitPerSample;

    switch (a_index)
      {
      case OMX_IndexParamAudioPcm:
        {
          const OMX_AUDIO_PARAM_PCMMODETYPE *p_pcmmode = ap_struct;
          new_rate = p_pcmmode->nSamplingRate;
          new_channels = p_pcmmode->nChannels;
          new_bps = p_pcmmode->nBitPerSample;
          /* min buffer size = At least 5ms or pcm data */
          new_min_buf_sz = ((new_rate * new_bps * new_channels) / 8000) * 5;

          TIZ_TRACE (handleOf (ap_obj),
                    "OMX_IndexParamAudioPcm : new sampling rate[%d] "
                    "new num channels[%d] new_bps[%d]", new_rate, new_channels,
                    new_bps);
        }
        break;

      case OMX_IndexParamAudioMp3:
        {
          const OMX_AUDIO_PARAM_MP3TYPE *p_mp3type = ap_struct;
          new_rate = p_mp3type->nSampleRate;
          new_channels = p_mp3type->nChannels;
          /* min buffer size = At least 5ms or pcm data */
          new_min_buf_sz = ((new_rate * new_bps * new_channels) / 8000) * 5;

          TIZ_TRACE (handleOf (ap_obj),
                    "OMX_IndexParamAudioMp3 : new sampling rate[%d] "
                    "new num channels[%d] new_bps[%d]", new_rate, new_channels,
                    new_bps);
        }
        break;

      case OMX_IndexParamAudioAac:
        {
          const OMX_AUDIO_PARAM_AACPROFILETYPE *p_aactype = ap_struct;
          new_rate = p_aactype->nSampleRate;
          new_channels = p_aactype->nChannels;
          /* min buffer size = At least 5ms or pcm data */
          new_min_buf_sz = ((new_rate * new_bps * new_channels) / 8000) * 5;

          TIZ_TRACE (handleOf (ap_obj),
                    "OMX_IndexParamAudioAac : new sampling rate[%d] "
                    "new num channels[%d] new_bps[%d]", new_rate, new_channels,
                    new_bps);
        }
        break;

      case OMX_IndexParamAudioVorbis:
        {
          const OMX_AUDIO_PARAM_VORBISTYPE *p_vortype = ap_struct;
          new_rate = p_vortype->nSampleRate;
          new_channels = p_vortype->nChannels;
          /* min buffer size = At least 5ms or pcm data */
          new_min_buf_sz = ((new_rate * new_bps * new_channels) / 8000) * 5;

          TIZ_TRACE (handleOf (ap_obj),
                    "OMX_IndexParamAudioVorbis : new sampling rate[%d] "
                    "new num channels[%d] new_bps[%d]", new_rate, new_channels,
                    new_bps);
        }
        break;

      case OMX_IndexParamAudioWma:
        {
          const OMX_AUDIO_PARAM_WMATYPE *p_wmatype = ap_struct;
          new_rate = p_wmatype->nSamplingRate;
          new_channels = p_wmatype->nChannels;
          /* min buffer size = At least 5ms or pcm data */
          new_min_buf_sz = ((new_rate * new_bps * new_channels) / 8000) * 5;

          TIZ_TRACE (handleOf (ap_obj),
                    "OMX_IndexParamAudioWma : new sampling rate[%d] "
                    "new num channels[%d] new_bps[%d]", new_rate, new_channels,
                    new_bps);
        }
        break;

      case OMX_IndexParamAudioRa:
        {
          const OMX_AUDIO_PARAM_RATYPE *p_ratype = ap_struct;
          new_rate = p_ratype->nSamplingRate;
          new_channels = p_ratype->nChannels;
          /* min buffer size = At least 5ms or pcm data */
          new_min_buf_sz = ((new_rate * new_bps * new_channels) / 8000) * 5;

          TIZ_TRACE (handleOf (ap_obj),
                    "OMX_IndexParamAudioRa : new sampling rate[%d] "
                    "new num channels[%d] new_bps[%d]", new_rate, new_channels,
                    new_bps);
        }
        break;

      case OMX_IndexParamAudioSbc:
        {
          const OMX_AUDIO_PARAM_SBCTYPE *p_sbctype = ap_struct;
          new_rate = p_sbctype->nSampleRate;
          new_channels = p_sbctype->nChannels;
          /* min buffer size = At least 5ms or pcm data */
          new_min_buf_sz = ((new_rate * new_bps * new_channels) / 8000) * 5;

          TIZ_TRACE (handleOf (ap_obj),
                    "OMX_IndexParamAudioSbc : new sampling rate[%d] "
                    "new num channels[%d] new_bps[%d]", new_rate, new_channels,
                    new_bps);
        }
        break;

      case OMX_IndexParamAudioAdpcm:
        {
          const OMX_AUDIO_PARAM_ADPCMTYPE *p_adpcmtype = ap_struct;
          new_rate = p_adpcmtype->nSampleRate;
          new_channels = p_adpcmtype->nChannels;
          /* min buffer size = At least 5ms or pcm data */
          new_min_buf_sz = ((new_rate * new_bps * new_channels) / 8000) * 5;

          TIZ_TRACE (handleOf (ap_obj),
                    "OMX_IndexParamAudioAdpcm : new sampling rate[%d] "
                    "new num channels[%d] new_bps[%d]", new_rate, new_channels,
                    new_bps);
        }
        break;

      default:
        {
          if (OMX_TizoniaIndexParamAudioOpus == a_index)
            {
              const OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE *p_opustype = ap_struct;
              new_rate = p_opustype->nSampleRate;
              new_channels = p_opustype->nChannels;

              TIZ_TRACE (handleOf (ap_obj),
                         "OMX_IndexParamAudioOpus : new sampling rate[%d] "
                         "new num channels[%d]", new_rate, new_channels);
            }

          else if (OMX_TizoniaIndexParamAudioFlac == a_index)
            {
              const OMX_TIZONIA_AUDIO_PARAM_FLACTYPE *p_flactype = ap_struct;
              new_rate = p_flactype->nSampleRate;
              new_channels = p_flactype->nChannels;

              TIZ_TRACE (handleOf (ap_obj),
                         "OMX_TizoniaIndexParamAudioFlac : new sampling rate[%d] "
                         "new num channels[%d]", new_rate, new_channels);
            }
        }
      };

    if ((p_obj->pcmmode_.nSamplingRate != new_rate)
        || (p_obj->pcmmode_.nChannels != new_channels)
        || (p_obj->pcmmode_.nBitPerSample != new_bps))
      {
        OMX_INDEXTYPE id = OMX_IndexParamAudioPcm;

        p_obj->pcmmode_.nSamplingRate = new_rate;
        p_obj->pcmmode_.nChannels = new_channels;
        p_obj->pcmmode_.nBitPerSample = new_bps;

        tiz_check_omx_err_ret_oom
          (tiz_vector_push_back (ap_changed_idxs, &id));

        TIZ_TRACE (handleOf (ap_obj),
                  "original PORT [%d] this PORT [%d] : [%s] -> "
                  "changed [OMX_IndexParamAudioPcm]...",
                  tiz_port_index (ap_mos_port),
                  p_base->portdef_.nPortIndex, tiz_idx_to_str (a_index));
      }

    /* Also update the port's minimum buffer size, if needed */
    if (new_min_buf_sz > p_base->portdef_.nBufferSize)
      {
        OMX_INDEXTYPE id = OMX_IndexParamPortDefinition;

        p_base->portdef_.nBufferSize = new_min_buf_sz;
        tiz_check_omx_err_ret_oom
          (tiz_vector_push_back (ap_changed_idxs, &id));

        TIZ_TRACE (handleOf (ap_obj),
                  "original PORT [%d] this PORT [%d] : [%s] -> "
                  "changed [OMX_IndexParamPortDefinition] nBufferSize [%d]...",
                  tiz_port_index (ap_mos_port),
                  p_base->portdef_.nPortIndex,
                  tiz_idx_to_str (a_index), p_base->portdef_.nBufferSize);
      }

  }
  return rc;
}

/*
 * tizpcmport_class
 */

static void *
pcmport_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizpcmport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_pcmport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizaudioport = tiz_get_type (ap_hdl, "tizaudioport");
  void * tizpcmport_class = factory_new (classOf (tizaudioport),
                                         "tizpcmport_class",
                                         classOf (tizaudioport),
                                         sizeof (tiz_pcmport_class_t),
                                         ap_tos, ap_hdl,
                                         ctor, pcmport_class_ctor, 0);
  return tizpcmport_class; 
}

void *
tiz_pcmport_init (void * ap_tos, void * ap_hdl)
{
  void * tizaudioport = tiz_get_type (ap_hdl, "tizaudioport");
  void * tizpcmport_class = tiz_get_type (ap_hdl, "tizpcmport_class");
  TIZ_LOG_CLASS (tizpcmport_class);
  void * tizpcmport =
    factory_new
    (tizpcmport_class,
     "tizpcmport",
     tizaudioport,
     sizeof (tiz_pcmport_t),
     ap_tos, ap_hdl,
     ctor, pcmport_ctor,
     dtor, pcmport_dtor,
     tiz_api_GetParameter, pcmport_GetParameter,
     tiz_api_SetParameter, pcmport_SetParameter,
     tiz_api_GetConfig, pcmport_GetConfig,
     tiz_api_SetConfig, pcmport_SetConfig,
     tiz_port_set_portdef_format, pcmport_set_portdef_format,
     tiz_port_check_tunnel_compat, pcmport_check_tunnel_compat,
     tiz_port_apply_slaving_behaviour, pcmport_apply_slaving_behaviour, 0);

  return tizpcmport;
}
