/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   webmdmuxfltport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief A specialised port class for the WebM demuxer filter role - implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <limits.h>

#include <tizplatform.h>

#include "webmdmux.h"
#include "webmdmuxfltport.h"
#include "webmdmuxfltport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.webm_demuxer.filter.port"
#endif

/*
 * webmdmuxfltport class
 */

static void *webmdmuxflt_port_ctor (void *ap_obj, va_list *app)
{
  webmdmuxflt_port_t *p_obj
      = super_ctor (typeOf (ap_obj, "webmdmuxfltport"), ap_obj, app);
  assert (p_obj);

  tiz_port_register_index (p_obj, OMX_IndexParamAudioMp3);
  tiz_port_register_index (p_obj, OMX_IndexParamAudioAac);
  tiz_port_register_index (p_obj, OMX_TizoniaIndexParamAudioOpus);

  p_obj->mp3type_.nSize             = sizeof (OMX_AUDIO_PARAM_MP3TYPE);
  p_obj->mp3type_.nVersion.nVersion = OMX_VERSION;
  p_obj->mp3type_.nPortIndex        = ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX;
  p_obj->mp3type_.nChannels         = 2;
  p_obj->mp3type_.nBitRate          = 0;
  p_obj->mp3type_.nSampleRate       = 44100;
  p_obj->mp3type_.nAudioBandWidth   = 0;
  p_obj->mp3type_.eChannelMode      = OMX_AUDIO_ChannelModeStereo;
  p_obj->mp3type_.eFormat           = OMX_AUDIO_MP3StreamFormatMP1Layer3;

  p_obj->aactype_.nSize = sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE);
  p_obj->aactype_.nVersion.nVersion = OMX_VERSION;
  p_obj->aactype_.nPortIndex = ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX;
  p_obj->aactype_.nChannels = 2;
  p_obj->aactype_.nSampleRate = 48000;
  p_obj->aactype_.nBitRate = 0;
  p_obj->aactype_.nAudioBandWidth = 0;
  p_obj->aactype_.nFrameLength = 0;
  p_obj->aactype_.nAACtools = OMX_AUDIO_AACToolAll;
  p_obj->aactype_.nAACERtools = OMX_AUDIO_AACERAll;
  p_obj->aactype_.eAACProfile = OMX_AUDIO_AACObjectLC;
  p_obj->aactype_.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP2ADTS;
  p_obj->aactype_.eChannelMode = OMX_AUDIO_ChannelModeStereo;

  p_obj->opustype_.nSize                   = sizeof (OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE);
  p_obj->opustype_.nVersion.nVersion       = OMX_VERSION;
  p_obj->opustype_.nPortIndex              = ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX;
  p_obj->opustype_.nChannels               = 2;
  p_obj->opustype_.nBitRate                = 256;
  p_obj->opustype_.nSampleRate             = 48000;
  p_obj->opustype_.nFrameDuration          = 2;
  p_obj->opustype_.nEncoderComplexity      = 0;
  p_obj->opustype_.bPacketLossResilience   = OMX_FALSE;
  p_obj->opustype_.bForwardErrorCorrection = OMX_FALSE;
  p_obj->opustype_.bDtx                    = OMX_FALSE;
  p_obj->opustype_.eChannelMode            = OMX_AUDIO_ChannelModeStereo;
  p_obj->opustype_.eFormat                 = OMX_AUDIO_OPUSStreamFormatVBR;

  return p_obj;
}

static void *webmdmuxflt_port_dtor (void *ap_obj)
{
  webmdmuxflt_port_t *p_obj = ap_obj;
  assert (p_obj);
  return super_dtor (typeOf (ap_obj, "webmdmuxfltport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE webmdmuxflt_port_GetParameter (const void *ap_obj,
                                                 OMX_HANDLETYPE ap_hdl,
                                                 OMX_INDEXTYPE a_index,
                                                 OMX_PTR ap_struct)
{
  const webmdmuxflt_port_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...",
            tiz_port_index (ap_obj), tiz_idx_to_str (a_index));


  switch (a_index)
    {
    case OMX_IndexParamAudioMp3:
      {
        OMX_AUDIO_PARAM_MP3TYPE *p_mp3mode
          = (OMX_AUDIO_PARAM_MP3TYPE *) ap_struct;
        *p_mp3mode = p_obj->mp3type_;
      }
      break;

    case OMX_IndexParamAudioAac:
      {
        OMX_AUDIO_PARAM_AACPROFILETYPE *p_aacmode
          = (OMX_AUDIO_PARAM_AACPROFILETYPE *) ap_struct;
        *p_aacmode = p_obj->aactype_;
      }
      break;

    default:
      {
        if (OMX_TizoniaIndexParamAudioOpus == a_index)
          {
            OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE *p_opusmode
              = (OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE *) ap_struct;
            *p_opusmode = p_obj->opustype_;
          }
        else
          {
            /* Try the parent's indexes */
            rc = super_GetParameter (typeOf (ap_obj, "webmdmuxfltport"),
                                     ap_obj, ap_hdl, a_index, ap_struct);
          }
      }
      break;
    };

  return rc;
}

static OMX_ERRORTYPE webmdmuxflt_port_SetParameter (const void *ap_obj,
                                                OMX_HANDLETYPE ap_hdl,
                                                OMX_INDEXTYPE a_index,
                                                OMX_PTR ap_struct)
{
  webmdmuxflt_port_t *p_obj = (webmdmuxflt_port_t *)ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

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
              TIZ_TRACE (ap_hdl,
                         "[%s] : OMX_ErrorBadParameter : "
                         "Sample rate not supported [%d]. "
                         "Returning...", tiz_idx_to_str (a_index),
                         p_mp3type->nSampleRate);
              rc = OMX_ErrorBadParameter;
            }
          };

        if (OMX_ErrorNone == rc)
          {
            /* Apply the new default values */
            p_obj->mp3type_.nChannels = p_mp3type->nChannels;
            p_obj->mp3type_.nBitRate = p_mp3type->nBitRate;
            p_obj->mp3type_.nSampleRate = p_mp3type->nSampleRate;
            p_obj->mp3type_.nAudioBandWidth = p_mp3type->nAudioBandWidth;
            p_obj->mp3type_.eChannelMode = p_mp3type->eChannelMode;
            p_obj->mp3type_.eFormat = p_mp3type->eFormat;
          }
      }
      break;

    case OMX_IndexParamAudioAac:
      {
        const OMX_AUDIO_PARAM_AACPROFILETYPE *p_aactype
          = (OMX_AUDIO_PARAM_AACPROFILETYPE *) ap_struct;

        switch (p_aactype->nSampleRate)
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
              TIZ_TRACE (ap_hdl, "[%s] : OMX_ErrorBadParameter : "
                         "Sample rate not supported [%d]. "
                         "Returning...", tiz_idx_to_str (a_index),
                         p_aactype->nSampleRate);
              rc = OMX_ErrorBadParameter;
            }
          };

        if (OMX_ErrorNone == rc)
          {
            /* Apply the new default values */
            p_obj->aactype_.nChannels        = p_aactype->nChannels;
            p_obj->aactype_.nSampleRate      = p_aactype->nSampleRate;
            p_obj->aactype_.nBitRate         = p_aactype->nBitRate;
            p_obj->aactype_.nAudioBandWidth  = p_aactype->nAudioBandWidth;
            p_obj->aactype_.nFrameLength     = p_aactype->nFrameLength;
            p_obj->aactype_.nAACtools        = p_aactype->nAACtools;
            p_obj->aactype_.nAACERtools      = p_aactype->nAACERtools;
            p_obj->aactype_.eAACProfile      = p_aactype->eAACProfile;
            p_obj->aactype_.eAACStreamFormat = p_aactype->eAACStreamFormat;
            p_obj->aactype_.eChannelMode     = p_aactype->eChannelMode;
          }
      }
      break;

    default:
      {

        if (OMX_TizoniaIndexParamAudioOpus == a_index)
          {
            const OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE *p_opustype
              = (OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE *) ap_struct;

            switch (p_opustype->nSampleRate)
              {
              case 8000:
              case 16000:
              case 24000:
              case 22050:
              case 32000:
              case 44100:
              case 48000:
                {
                  break;
                }
              default:
                {
                  TIZ_ERROR (ap_hdl, "[%s] : OMX_ErrorBadParameter : "
                             "Sample rate not supported [%d]. "
                             "Returning...", tiz_idx_to_str (a_index),
                             p_opustype->nSampleRate);
                  rc = OMX_ErrorBadParameter;
                }
              };

            if (OMX_ErrorNone == rc)
              {
                /* Apply the new default values */
                p_obj->opustype_.nChannels               = p_opustype->nChannels;
                p_obj->opustype_.nBitRate                = p_opustype->nBitRate;
                p_obj->opustype_.nSampleRate             = p_opustype->nSampleRate;
                p_obj->opustype_.nFrameDuration          = p_opustype->nFrameDuration;
                p_obj->opustype_.nEncoderComplexity      = p_opustype->nEncoderComplexity;
                p_obj->opustype_.bPacketLossResilience   = p_opustype->bPacketLossResilience;
                p_obj->opustype_.bForwardErrorCorrection = p_opustype->bForwardErrorCorrection;
                p_obj->opustype_.bDtx                    = p_opustype->bDtx;
                p_obj->opustype_.eChannelMode            = p_opustype->eChannelMode;
                p_obj->opustype_.eFormat                 = p_opustype->eFormat;
              }
          }
        else
          {
            /* Try the parent's indexes */
            rc = super_SetParameter (typeOf (ap_obj, "webmdmuxfltport"),
                                     ap_obj, ap_hdl, a_index, ap_struct);
          }
      }
    };
  return rc;
}

static bool
webmdmuxflt_port_check_tunnel_compat (const void *ap_obj,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t *p_obj = (tiz_port_t *) ap_obj;

  assert (ap_this_def);
  assert (ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_ERROR (handleOf (ap_obj),
               "port [%d] check_tunnel_compat : "
               "Audio domain not found, instead found domain [%d]",
               p_obj->pid_, ap_other_def->eDomain);
      return false;
    }

  /* INFO: */
  /* This is not specified in the spec, but a binary audio reader */
  /* could use OMX_AUDIO_CodingUnused as a means to signal "any" format */

  if (ap_other_def->format.audio.eEncoding != OMX_AUDIO_CodingUnused)
    {
      if (ap_other_def->format.audio.eEncoding != OMX_AUDIO_CodingMP3
          && ap_other_def->format.audio.eEncoding != OMX_AUDIO_CodingAAC)
        {
          TIZ_ERROR (handleOf (ap_obj),
                    "port [%d] check_tunnel_compat : "
                    "Unknown encoding found [%d]",
                    p_obj->pid_, ap_other_def->format.audio.eEncoding);
          return false;
        }
    }

  TIZ_TRACE (handleOf (ap_obj),
            "port [%d] check_tunnel_compat [OK]", p_obj->pid_);

  return true;
}

static OMX_ERRORTYPE
webmdmuxflt_port_apply_slaving_behaviour (void *ap_obj, void *ap_mos_port,
                                 const OMX_INDEXTYPE a_index,
                                 const OMX_PTR ap_struct,
                                 tiz_vector_t * ap_changed_idxs)
{
  webmdmuxflt_port_t *p_obj = ap_obj;
  tiz_port_t *p_base = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  /* OpenMAX IL 1.2 Section 3.5 : Slaving behaviour for nSamplingRate and
   * nChannels, both in OMX_AUDIO_PARAM_MP3TYPE and OMX_AUDIO_PARAM_AACPROFILETYPE */

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

          TIZ_TRACE (handleOf (ap_obj),
                     "OMX_IndexParamAudioPcm : new sampling rate[%d] "
                     "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioMp3:
        {
          const OMX_AUDIO_PARAM_MP3TYPE *p_mp3type = ap_struct;
          new_rate = p_mp3type->nSampleRate;
          new_channels = p_mp3type->nChannels;

          TIZ_TRACE (handleOf (ap_obj),
                     "OMX_IndexParamAudioMp3 : new sampling rate[%d] "
                     "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioAac:
        {
          const OMX_AUDIO_PARAM_AACPROFILETYPE *p_aactype = ap_struct;
          new_rate = p_aactype->nSampleRate;
          new_channels = p_aactype->nChannels;

          TIZ_TRACE (handleOf (ap_obj),
                     "OMX_IndexParamAudioAac : new sampling rate[%d] "
                     "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioVorbis:
        {
          const OMX_AUDIO_PARAM_VORBISTYPE *p_vortype = ap_struct;
          new_rate = p_vortype->nSampleRate;
          new_channels = p_vortype->nChannels;

          TIZ_TRACE (handleOf (ap_obj),
                     "OMX_IndexParamAudioVorbis : new sampling rate[%d] "
                     "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioWma:
        {
          const OMX_AUDIO_PARAM_WMATYPE *p_wmatype = ap_struct;
          new_rate = p_wmatype->nSamplingRate;
          new_channels = p_wmatype->nChannels;

          TIZ_TRACE (handleOf (ap_obj),
                     "OMX_IndexParamAudioWma : new sampling rate[%d] "
                     "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioRa:
        {
          const OMX_AUDIO_PARAM_RATYPE *p_ratype = ap_struct;
          new_rate = p_ratype->nSamplingRate;
          new_channels = p_ratype->nChannels;

          TIZ_TRACE (handleOf (ap_obj),
                     "OMX_IndexParamAudioRa : new sampling rate[%d] "
                     "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioSbc:
        {
          const OMX_AUDIO_PARAM_SBCTYPE *p_sbctype = ap_struct;
          new_rate = p_sbctype->nSampleRate;
          new_channels = p_sbctype->nChannels;

          TIZ_TRACE (handleOf (ap_obj),
                     "OMX_IndexParamAudioSbc : new sampling rate[%d] "
                     "new num channels[%d]", new_rate, new_channels);
        }
        break;

      case OMX_IndexParamAudioAdpcm:
        {
          const OMX_AUDIO_PARAM_ADPCMTYPE *p_adpcmtype = ap_struct;
          new_rate = p_adpcmtype->nSampleRate;
          new_channels = p_adpcmtype->nChannels;

          TIZ_TRACE (handleOf (ap_obj),
                     "OMX_IndexParamAudioAdpcm : new sampling rate[%d] "
                     "new num channels[%d]", new_rate, new_channels);
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

    {
      const tiz_audioport_t *p_audio_port = ap_obj;
      const OMX_AUDIO_CODINGTYPE encoding = p_audio_port->port_format_.eEncoding;

      if (encoding == OMX_AUDIO_CodingMP3)
        {
          if ((p_obj->mp3type_.nSampleRate != new_rate)
              || (p_obj->mp3type_.nChannels != new_channels))
            {
              OMX_INDEXTYPE id = OMX_IndexParamAudioMp3;

              p_obj->mp3type_.nSampleRate = new_rate;
              p_obj->mp3type_.nChannels = new_channels;
              tiz_vector_push_back (ap_changed_idxs, &id);

              TIZ_TRACE (handleOf (ap_obj),
                         " original pid [%d] this pid [%d] : [%s] -> "
                         "changed [OMX_IndexParamAudioMp3]...",
                         tiz_port_index (ap_mos_port),
                         p_base->portdef_.nPortIndex, tiz_idx_to_str (a_index));
            }
        }
      else if (encoding == OMX_AUDIO_CodingAAC)
        {
          if ((p_obj->aactype_.nSampleRate != new_rate)
              || (p_obj->aactype_.nChannels != new_channels))
            {
              OMX_INDEXTYPE id = OMX_IndexParamAudioAac;

              p_obj->aactype_.nSampleRate = new_rate;
              p_obj->aactype_.nChannels = new_channels;
              tiz_vector_push_back (ap_changed_idxs, &id);

              TIZ_TRACE (handleOf (ap_obj),
                         " original pid [%d] this pid [%d] : [%s] -> "
                         "changed [OMX_IndexParamAudioAac]...",
                         tiz_port_index (ap_mos_port),
                         p_base->portdef_.nPortIndex, tiz_idx_to_str (a_index));
            }
        }
    }
  }
  return rc;
}

/*
 * webmdmuxflt_port_class
 */

static void *webmdmuxflt_port_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "webmdmuxfltport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *webmdmuxflt_port_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizaudioport = tiz_get_type (ap_hdl, "tizaudioport");
  void *webmdmuxfltport_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizaudioport), "webmdmuxfltport_class", classOf (tizaudioport), sizeof(webmdmuxflt_port_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, webmdmuxflt_port_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return webmdmuxfltport_class;
}

void *webmdmuxflt_port_init (void *ap_tos, void *ap_hdl)
{
  void *tizaudioport = tiz_get_type (ap_hdl, "tizaudioport");
  void *webmdmuxfltport_class = tiz_get_type (ap_hdl, "webmdmuxfltport_class");
  TIZ_LOG_CLASS (webmdmuxfltport_class);
  void *webmdmuxfltport = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (webmdmuxfltport_class, "webmdmuxfltport", tizaudioport, sizeof(webmdmuxflt_port_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, webmdmuxflt_port_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, webmdmuxflt_port_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_api_GetParameter, webmdmuxflt_port_GetParameter,
       /* TIZ_CLASS_COMMENT: */
       tiz_api_SetParameter, webmdmuxflt_port_SetParameter,
       /* TIZ_CLASS_COMMENT: */
       tiz_port_check_tunnel_compat, webmdmuxflt_port_check_tunnel_compat,
       /* TIZ_CLASS_COMMENT: */
       tiz_port_apply_slaving_behaviour, webmdmuxflt_port_apply_slaving_behaviour,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);

  return webmdmuxfltport;
}
