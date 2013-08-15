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
 * @file   tizmp3port.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  mp3port class implementation
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizmp3port.h"
#include "tizmp3port_decls.h"

#include "tizutils.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.mp3port"
#endif

/*
 * tizmp3port class
 */

static void *
mp3port_ctor (void *ap_obj, va_list * app)
{
  tiz_mp3port_t *p_obj = super_ctor (tizmp3port, ap_obj, app);
  tiz_port_t *p_base = ap_obj;
  OMX_AUDIO_PARAM_MP3TYPE *p_mp3mode = NULL;
  tiz_port_register_index (p_obj, OMX_IndexParamAudioMp3);

  /* Initialize the OMX_AUDIO_PARAM_MP3TYPE structure */
  if ((p_mp3mode = va_arg (*app, OMX_AUDIO_PARAM_MP3TYPE *)))
    {
      p_obj->mp3type_ = *p_mp3mode;
    }

  p_base->portdef_.eDomain = OMX_PortDomainAudio;
  p_base->portdef_.format.audio.pNativeRender = 0;
  /* TODO: MIME type */
  p_base->portdef_.format.audio.bFlagErrorConcealment = OMX_FALSE;
  p_base->portdef_.format.audio.eEncoding = OMX_AUDIO_CodingMP3;

  return p_obj;

}

static void *
mp3port_dtor (void *ap_obj)
{
  return super_dtor (tizmp3port, ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
mp3port_GetParameter (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_mp3port_t *p_obj = ap_obj;

  TIZ_LOG (TIZ_TRACE, "GetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamAudioMp3:
      {
        OMX_AUDIO_PARAM_MP3TYPE *p_mp3mode
          = (OMX_AUDIO_PARAM_MP3TYPE *) ap_struct;
        *p_mp3mode = p_obj->mp3type_;
        break;
      }

    default:
      {
        /* Try the parent's indexes */
        return super_GetParameter (tizmp3port,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3port_SetParameter (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_mp3port_t *p_obj = (tiz_mp3port_t *) ap_obj;

  switch (a_index)
    {

    case OMX_IndexParamAudioMp3:
      {
        const OMX_AUDIO_PARAM_MP3TYPE *p_mp3type
          = (OMX_AUDIO_PARAM_MP3TYPE *) ap_struct;

        switch (p_mp3type->nSampleRate)
          {
          case 16000:           /* MPEG-2 */
          case 24000:           /* MPEG-2 */
          case 22050:           /* MPEG-2 */
          case 32000:           /* MPEG-1 */
          case 44100:           /* MPEG-1 */
          case 48000:           /* MPEG-1 */
            {
              break;
            }
          default:
            {
              TIZ_LOGN (TIZ_TRACE, ap_hdl,
                        "[%s] : OMX_ErrorBadParameter : "
                        "Sample rate not supported [%d]. "
                        "Returning...", tiz_idx_to_str (a_index),
                        p_mp3type->nSampleRate);
              return OMX_ErrorBadParameter;
            }
          };

        /* Do now allow changes to sampling rate or num of channels if this is
         * a slave output port */
        {
          const tiz_port_t *p_base = ap_obj;

          if ((OMX_DirOutput == p_base->portdef_.eDir)
              && (p_base->opts_.mos_port != -1)
              && (p_base->opts_.mos_port != p_base->portdef_.nPortIndex)
              && (p_obj->mp3type_.nChannels != p_mp3type->nChannels
                  || p_obj->mp3type_.nSampleRate != p_mp3type->nSampleRate))
            {
              TIZ_LOGN (TIZ_ERROR, ap_hdl,
                        "[OMX_ErrorBadParameter] : PORT [%d] "
                        "SetParameter [OMX_IndexParamAudioMp3]... "
                        "Slave port, cannot update sample rate "
                        "or number of channels", tiz_port_dir (p_obj));
              return OMX_ErrorBadParameter;
            }
        }

        /* Apply the new default values */
        if (p_obj->mp3type_.nChannels != p_mp3type->nChannels ||
            p_obj->mp3type_.nBitRate != p_mp3type->nBitRate ||
            p_obj->mp3type_.nSampleRate != p_mp3type->nSampleRate ||
            p_obj->mp3type_.nAudioBandWidth != p_mp3type->nAudioBandWidth ||
            p_obj->mp3type_.eChannelMode != p_mp3type->eChannelMode ||
            p_obj->mp3type_.eFormat != p_mp3type->eFormat)
          {
            p_obj->mp3type_.nChannels = p_mp3type->nChannels;
            p_obj->mp3type_.nBitRate = p_mp3type->nBitRate;
            p_obj->mp3type_.nSampleRate = p_mp3type->nSampleRate;
            p_obj->mp3type_.nAudioBandWidth = p_mp3type->nAudioBandWidth;
            p_obj->mp3type_.eChannelMode = p_mp3type->eChannelMode;
            p_obj->mp3type_.eFormat = p_mp3type->eFormat;
          }

      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_SetParameter (tizmp3port,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
  mp3port_set_portdef_format
  (void *ap_obj, const OMX_PARAM_PORTDEFINITIONTYPE * ap_pdef)
{
  /* TODO */
  return OMX_ErrorNone;
}

static OMX_BOOL
mp3port_check_tunnel_compat (const void *ap_obj,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t *p_obj = (tiz_port_t *) ap_obj;

  assert (ap_this_def);
  assert (ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_LOG (TIZ_TRACE,
               "port [%d] check_tunnel_compat : "
               "Audio domain not found, instead found domain [%d]",
               p_obj->pid_, ap_other_def->eDomain);
      return OMX_FALSE;
    }

  /* INFO: */
  /* This is not specified in the spec, but a binary audio reader */
  /* could use OMX_AUDIO_CodingUnused as a means to signal "any" format */

  if (ap_other_def->format.audio.eEncoding != OMX_AUDIO_CodingUnused)
    {
      if (ap_other_def->format.audio.eEncoding != OMX_AUDIO_CodingMP3)
        {
          TIZ_LOG (TIZ_TRACE, "port [%d] check_tunnel_compat : "
                   "MP3 encoding not found, instead found encoding [%d]",
                   p_obj->pid_, ap_other_def->format.audio.eEncoding);
          return OMX_FALSE;
        }
    }

  TIZ_LOG (TIZ_TRACE, "port [%d] check_tunnel_compat [OK]", p_obj->pid_);

  return OMX_TRUE;
}

static OMX_ERRORTYPE
mp3port_apply_slaving_behaviour (void *ap_obj, void *ap_mos_port,
                                 const OMX_INDEXTYPE a_index,
                                 const OMX_PTR ap_struct,
                                 tiz_vector_t * ap_changed_idxs)
{
  tiz_mp3port_t *p_obj = ap_obj;
  tiz_port_t *p_base = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  /* OpenMAX IL 1.2 Section 3.5 : Slaving behaviour for nSamplingRate and
   * nChannels, both in OMX_AUDIO_PARAM_MP3TYPE */

  assert (p_obj);
  assert (ap_struct);
  assert (ap_changed_idxs);

  {
    OMX_U32 new_rate = p_obj->mp3type_.nSampleRate;
    OMX_U32 new_channels = p_obj->mp3type_.nChannels;

    switch (a_index)
      {
      case OMX_IndexParamAudioPcm:
        {
          const OMX_AUDIO_PARAM_PCMMODETYPE *p_pcmmode = ap_struct;
          new_rate = p_pcmmode->nSamplingRate;
          new_channels = p_pcmmode->nChannels;

          TIZ_LOG (TIZ_TRACE,
                   "OMX_IndexParamAudioPcm : new sampling rate[%d] "
                   "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioMp3:
        {
          const OMX_AUDIO_PARAM_MP3TYPE *p_mp3type = ap_struct;
          new_rate = p_mp3type->nSampleRate;
          new_channels = p_mp3type->nChannels;

          TIZ_LOG (TIZ_TRACE,
                   "OMX_IndexParamAudioMp3 : new sampling rate[%d] "
                   "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioAac:
        {
          const OMX_AUDIO_PARAM_AACPROFILETYPE *p_aactype = ap_struct;
          new_rate = p_aactype->nSampleRate;
          new_channels = p_aactype->nChannels;

          TIZ_LOG (TIZ_TRACE,
                   "OMX_IndexParamAudioAac : new sampling rate[%d] "
                   "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioVorbis:
        {
          const OMX_AUDIO_PARAM_VORBISTYPE *p_vortype = ap_struct;
          new_rate = p_vortype->nSampleRate;
          new_channels = p_vortype->nChannels;

          TIZ_LOG (TIZ_TRACE,
                   "OMX_IndexParamAudioVorbis : new sampling rate[%d] "
                   "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioWma:
        {
          const OMX_AUDIO_PARAM_WMATYPE *p_wmatype = ap_struct;
          new_rate = p_wmatype->nSamplingRate;
          new_channels = p_wmatype->nChannels;

          TIZ_LOG (TIZ_TRACE,
                   "OMX_IndexParamAudioWma : new sampling rate[%d] "
                   "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioRa:
        {
          const OMX_AUDIO_PARAM_RATYPE *p_ratype = ap_struct;
          new_rate = p_ratype->nSamplingRate;
          new_channels = p_ratype->nChannels;

          TIZ_LOG (TIZ_TRACE,
                   "OMX_IndexParamAudioRa : new sampling rate[%d] "
                   "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioSbc:
        {
          const OMX_AUDIO_PARAM_SBCTYPE *p_sbctype = ap_struct;
          new_rate = p_sbctype->nSampleRate;
          new_channels = p_sbctype->nChannels;

          TIZ_LOG (TIZ_TRACE,
                   "OMX_IndexParamAudioSbc : new sampling rate[%d] "
                   "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioAdpcm:
        {
          const OMX_AUDIO_PARAM_ADPCMTYPE *p_adpcmtype = ap_struct;
          new_rate = p_adpcmtype->nSampleRate;
          new_channels = p_adpcmtype->nChannels;

          TIZ_LOG (TIZ_TRACE,
                   "OMX_IndexParamAudioAdpcm : new sampling rate[%d] "
                   "new num channels[%d]", new_rate, new_channels);
        }
        break;

      default:
        {
        }
      };

    if ((p_obj->mp3type_.nSampleRate != new_rate)
        || (p_obj->mp3type_.nChannels != new_channels))
      {
        OMX_INDEXTYPE id = OMX_IndexParamAudioMp3;

        p_obj->mp3type_.nSampleRate = new_rate;
        p_obj->mp3type_.nChannels = new_channels;
        tiz_vector_push_back (ap_changed_idxs, &id);

        TIZ_LOG (TIZ_TRACE, " original pid [%d] this pid [%d] : [%s] -> "
                 "changed [OMX_IndexParamAudioMp3]...",
                 tiz_port_index (ap_mos_port),
                 p_base->portdef_.nPortIndex, tiz_idx_to_str (a_index));
      }

  }
  return rc;
}

/*
 * tizmp3port_class
 */

static void *
mp3port_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (tizmp3port_class, ap_obj, app);
}

/*
 * initialization
 */

const void *tizmp3port, *tizmp3port_class;

OMX_ERRORTYPE
tiz_mp3port_init (void)
{
  if (!tizmp3port_class)
    {
      tiz_check_omx_err_ret_oom (tiz_port_init ());
      tiz_check_null_ret_oom
        (tizmp3port_class = factory_new (tizport_class,
                                         "tizmp3port_class",
                                         tizport_class,
                                         sizeof (tiz_mp3port_class_t),
                                         ctor, mp3port_class_ctor, 0));
    }

  if (!tizmp3port)
    {
      tiz_check_omx_err_ret_oom (tiz_audioport_init ());
      tiz_check_null_ret_oom
        (tizmp3port =
         factory_new
         (tizaudioport_class,
          "tizmp3port",
          tizaudioport,
          sizeof (tiz_mp3port_t),
          ctor, mp3port_ctor,
          dtor, mp3port_dtor,
          tiz_api_GetParameter, mp3port_GetParameter,
          tiz_api_SetParameter, mp3port_SetParameter,
          tiz_port_set_portdef_format, mp3port_set_portdef_format,
          tiz_port_check_tunnel_compat, mp3port_check_tunnel_compat,
          tiz_port_apply_slaving_behaviour, mp3port_apply_slaving_behaviour, 0));
    }
  return OMX_ErrorNone;
}
