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
 * @file   tizopusport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  opusport class implementation
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizopusport.h"
#include "tizopusport_decls.h"

#include "tizutils.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.opusport"
#endif

/*
 * tizopusport class
 */

static void *
opusport_ctor (void *ap_obj, va_list * app)
{
  tiz_opusport_t *p_obj = super_ctor (tizopusport, ap_obj, app);
  tiz_port_t *p_base = ap_obj;
  OMX_AUDIO_PARAM_OPUSTYPE *p_opusmode = NULL;
  tiz_port_register_index (p_obj, OMX_IndexParamAudioOpus);

  /* Initialize the OMX_AUDIO_PARAM_OPUSTYPE structure */
  if ((p_opusmode = va_arg (*app, OMX_AUDIO_PARAM_OPUSTYPE *)))
    {
      p_obj->opustype_ = *p_opusmode;
    }

  p_base->portdef_.eDomain                            = OMX_PortDomainAudio;
  p_base->portdef_.format.audio.pNativeRender         = 0;
  /* TODO: MIME type */
  p_base->portdef_.format.audio.bFlagErrorConcealment = OMX_FALSE;
  p_base->portdef_.format.audio.eEncoding             = (OMX_AUDIO_CODINGTYPE)
    OMX_AUDIO_CodingOPUS;

  return p_obj;
}

static void *
opusport_dtor (void *ap_obj)
{
  return super_dtor (tizopusport, ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
opusport_GetParameter (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_opusport_t *p_obj = ap_obj;

  TIZ_LOG (TIZ_TRACE, "GetParameter [%s]...", tiz_idx_to_str (a_index));

  if (OMX_IndexParamAudioOpus == a_index)
    {
      OMX_AUDIO_PARAM_OPUSTYPE *p_opusmode
        = (OMX_AUDIO_PARAM_OPUSTYPE *) ap_struct;
      *p_opusmode = p_obj->opustype_;
    }
  else
    {
      /* Try the parent's indexes */
      return super_GetParameter (tizopusport,
                                 ap_obj, ap_hdl, a_index, ap_struct);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
opusport_SetParameter (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_opusport_t *p_obj = (tiz_opusport_t *) ap_obj;

  if (OMX_IndexParamAudioOpus == a_index)
    {
      const OMX_AUDIO_PARAM_OPUSTYPE *p_opustype
        = (OMX_AUDIO_PARAM_OPUSTYPE *) ap_struct;

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
            TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl),
                           TIZ_CBUF (ap_hdl),
                           "[%s] : OMX_ErrorBadParameter : "
                           "Sample rate not supported [%d]. "
                           "Returning...", tiz_idx_to_str (a_index),
                           p_opustype->nSampleRate);
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
            && (p_obj->opustype_.nChannels != p_opustype->nChannels
                || p_obj->opustype_.nSampleRate != p_opustype->nSampleRate))
          {
            TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl),
                           TIZ_CBUF (ap_hdl),
                           "[OMX_ErrorBadParameter] : PORT [%d] "
                           "SetParameter [OMX_IndexParamAudioOpus]... "
                           "Slave port, cannot update sample rate "
                           "or number of channels", tiz_port_dir (p_obj));
            return OMX_ErrorBadParameter;
          }
      }

      /* Apply the new default values */
      if (p_obj->opustype_.nChannels               != p_opustype->nChannels               ||
          p_obj->opustype_.nBitRate                != p_opustype->nBitRate                ||
          p_obj->opustype_.nSampleRate             != p_opustype->nSampleRate             ||
          p_obj->opustype_.nFrameDuration          != p_opustype->nFrameDuration          ||
          p_obj->opustype_.nEncoderComplexity      != p_opustype->nEncoderComplexity      ||
          p_obj->opustype_.bPacketLossResilience   != p_opustype->bPacketLossResilience   ||
          p_obj->opustype_.bForwardErrorCorrection != p_opustype->bForwardErrorCorrection ||
          p_obj->opustype_.bDtx                    != p_opustype->bDtx                    ||
          p_obj->opustype_.eChannelMode            != p_opustype->eChannelMode            ||
          p_obj->opustype_.eFormat                 != p_opustype->eFormat)
          
        {
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
      return super_SetParameter (tizopusport,
                                 ap_obj, ap_hdl, a_index, ap_struct);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
  opusport_set_portdef_format
  (void *ap_obj, const OMX_PARAM_PORTDEFINITIONTYPE * ap_pdef)
{
  /* TODO */
  return OMX_ErrorNone;
}

static OMX_BOOL
opusport_check_tunnel_compat (const void *ap_obj,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t *p_obj = (tiz_port_t *) ap_obj;

  assert (ap_this_def);
  assert (ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_LOG (TIZ_TRACE, "port [%d] check_tunnel_compat : "
               "Audio domain not found, instead found domain [%d]",
               p_obj->pid_, ap_other_def->eDomain);
      return OMX_FALSE;
    }

  /* INFO: */
  /* This is not specified in the spec, but a binary audio reader */
  /* could use OMX_AUDIO_CodingUnused as a means to signal "any" format */

  if (ap_other_def->format.audio.eEncoding != OMX_AUDIO_CodingUnused)
    {
      if (ap_other_def->format.audio.eEncoding
          != (OMX_AUDIO_CODINGTYPE) OMX_AUDIO_CodingOPUS)
        {
          TIZ_LOG (TIZ_TRACE, "port [%d] check_tunnel_compat : "
                   "OPUS encoding not found, instead found encoding [%d]",
                   p_obj->pid_, ap_other_def->format.audio.eEncoding);
          return OMX_FALSE;
        }
    }

  TIZ_LOG (TIZ_TRACE, "port [%d] check_tunnel_compat [OK]", p_obj->pid_);

  return OMX_TRUE;
}

static OMX_ERRORTYPE
opusport_apply_slaving_behaviour (void *ap_obj, void *ap_mos_port,
                                 const OMX_INDEXTYPE a_index,
                                 const OMX_PTR ap_struct,
                                 tiz_vector_t * ap_changed_idxs)
{
  tiz_opusport_t *p_obj = ap_obj;
  tiz_port_t *p_base = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  /* OpenMAX IL 1.2 Section 3.5 : Slaving behaviour for nSamplingRate and
   * nChannels, both in OMX_AUDIO_PARAM_OPUSTYPE */

  assert (p_obj);
  assert (ap_struct);
  assert (ap_changed_idxs);

  {
    OMX_U32 new_rate = p_obj->opustype_.nSampleRate;
    OMX_U32 new_channels = p_obj->opustype_.nChannels;

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

    if ((p_obj->opustype_.nSampleRate != new_rate)
        || (p_obj->opustype_.nChannels != new_channels))
      {
        OMX_INDEXTYPE id = OMX_IndexParamAudioOpus;

        p_obj->opustype_.nSampleRate = new_rate;
        p_obj->opustype_.nChannels = new_channels;
        tiz_vector_push_back (ap_changed_idxs, &id);

        TIZ_LOG (TIZ_TRACE, " original pid [%d] this pid [%d] : [%s] -> "
                 "changed [OMX_IndexParamAudioOpus]...",
                 tiz_port_index (ap_mos_port),
                 p_base->portdef_.nPortIndex, tiz_idx_to_str (a_index));
      }

  }
  return rc;
}

/*
 * tizopusport_class
 */

static void *
opusport_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (tizopusport_class, ap_obj, app);
}

/*
 * initialization
 */

const void *tizopusport, *tizopusport_class;

void
tiz_opusport_init (void)
{
  if (!tizopusport_class)
    {
      tiz_port_init ();
      tizopusport_class = factory_new (tizport_class,
                                        "tizopusport_class",
                                        tizport_class,
                                        sizeof (tiz_opusport_class_t),
                                        ctor, opusport_class_ctor, 0);

    }

  if (!tizopusport)
    {
      tiz_audioport_init ();
      tizopusport =
        factory_new
        (tizaudioport_class,
         "tizopusport",
         tizaudioport,
         sizeof (tiz_opusport_t),
         ctor, opusport_ctor,
         dtor, opusport_dtor,
         tiz_api_GetParameter, opusport_GetParameter,
         tiz_api_SetParameter, opusport_SetParameter,
         tiz_port_set_portdef_format, opusport_set_portdef_format,
         tiz_port_check_tunnel_compat, opusport_check_tunnel_compat,
         tiz_port_apply_slaving_behaviour, opusport_apply_slaving_behaviour, 0);
    }
}
