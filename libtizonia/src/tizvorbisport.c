/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   tizvorbisport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  vorbisport class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizplatform.h>

#include "tizutils.h"
#include "tizvorbisport.h"
#include "tizvorbisport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.vorbisport"
#endif

/*
 * tizvorbisport class
 */

static void *
vorbisport_ctor (void * ap_obj, va_list * app)
{
  tiz_vorbisport_t * p_obj
    = super_ctor (typeOf (ap_obj, "tizvorbisport"), ap_obj, app);
  tiz_port_t * p_base = ap_obj;
  OMX_AUDIO_PARAM_VORBISTYPE * p_vorbismode = NULL;
  tiz_port_register_index (p_obj, OMX_IndexParamAudioVorbis);

  /* Initialize the OMX_AUDIO_PARAM_VORBISTYPE structure */
  if ((p_vorbismode = va_arg (*app, OMX_AUDIO_PARAM_VORBISTYPE *)))
    {
      p_obj->vorbistype_ = *p_vorbismode;
    }

  p_base->portdef_.eDomain = OMX_PortDomainAudio;
  /* NOTE: MIME type is gone in 1.2 */
  p_base->portdef_.format.audio.pNativeRender = 0;
  p_base->portdef_.format.audio.bFlagErrorConcealment = OMX_FALSE;
  p_base->portdef_.format.audio.eEncoding = OMX_AUDIO_CodingVORBIS;

  return p_obj;
}

static void *
vorbisport_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tizvorbisport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
vorbisport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_vorbisport_t * p_obj = ap_obj;

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));

  switch (a_index)
    {
      case OMX_IndexParamAudioVorbis:
        {
          OMX_AUDIO_PARAM_VORBISTYPE * p_vorbismode
            = (OMX_AUDIO_PARAM_VORBISTYPE *) ap_struct;
          *p_vorbismode = p_obj->vorbistype_;
          break;
        }

      default:
        {
          /* Try the parent's indexes */
          return super_GetParameter (typeOf (ap_obj, "tizvorbisport"), ap_obj,
                                     ap_hdl, a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vorbisport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_vorbisport_t * p_obj = (tiz_vorbisport_t *) ap_obj;

  TIZ_TRACE (ap_hdl, "PORT [%d] SetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));
  assert (p_obj);

  switch (a_index)
    {

      case OMX_IndexParamAudioVorbis:
        {
          const OMX_AUDIO_PARAM_VORBISTYPE * p_vorbistype
            = (OMX_AUDIO_PARAM_VORBISTYPE *) ap_struct;

          switch (p_vorbistype->nSampleRate)
            {
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
                  TIZ_TRACE (ap_hdl,
                             "[%s] : OMX_ErrorBadParameter : "
                             "Sample rate not supported [%d]. "
                             "Returning...",
                             tiz_idx_to_str (a_index),
                             p_vorbistype->nSampleRate);
                  return OMX_ErrorBadParameter;
                }
            };

          /* Do now allow changes to sampling rate or num of channels if this is
         * a slave output port */
          {
            const tiz_port_t * p_base = ap_obj;

            if ((OMX_DirOutput == p_base->portdef_.eDir)
                && (p_base->opts_.mos_port != -1)
                && (p_base->opts_.mos_port != p_base->portdef_.nPortIndex)
                && (p_obj->vorbistype_.nChannels != p_vorbistype->nChannels
                    || p_obj->vorbistype_.nSampleRate
                         != p_vorbistype->nSampleRate))
              {
                TIZ_ERROR (ap_hdl,
                           "[OMX_ErrorBadParameter] : PORT [%d] "
                           "SetParameter [OMX_IndexParamAudioVorbis]... "
                           "Slave port, cannot update sample rate "
                           "or number of channels",
                           tiz_port_dir (p_obj));
                return OMX_ErrorBadParameter;
              }
          }

          /* Apply the new default values */
          p_obj->vorbistype_.nChannels = p_vorbistype->nChannels;
          p_obj->vorbistype_.nBitRate = p_vorbistype->nBitRate;
          p_obj->vorbistype_.nMinBitRate = p_vorbistype->nMinBitRate;
          p_obj->vorbistype_.nMaxBitRate = p_vorbistype->nMaxBitRate;
          p_obj->vorbistype_.nSampleRate = p_vorbistype->nSampleRate;
          p_obj->vorbistype_.nAudioBandWidth = p_vorbistype->nAudioBandWidth;
          p_obj->vorbistype_.nQuality = p_vorbistype->nQuality;
          p_obj->vorbistype_.bManaged = p_vorbistype->bManaged;
          p_obj->vorbistype_.bDownmix = p_vorbistype->bDownmix;
        }
        break;

      default:
        {
          /* Try the parent's indexes */
          return super_SetParameter (typeOf (ap_obj, "tizvorbisport"), ap_obj,
                                     ap_hdl, a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static bool
vorbisport_check_tunnel_compat (const void * ap_obj,
                                OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                                OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t * p_obj = (tiz_port_t *) ap_obj;

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
      if (ap_other_def->format.audio.eEncoding != OMX_AUDIO_CodingVORBIS)
        {
          TIZ_ERROR (handleOf (ap_obj),
                     "port [%d] check_tunnel_compat : "
                     "VORBIS encoding not found, instead found encoding [%d]",
                     p_obj->pid_, ap_other_def->format.audio.eEncoding);
          return false;
        }
    }

  TIZ_TRACE (handleOf (ap_obj), "port [%d] check_tunnel_compat [OK]",
             p_obj->pid_);

  return true;
}

static OMX_ERRORTYPE
vorbisport_apply_slaving_behaviour (void * ap_obj, void * ap_mos_port,
                                    const OMX_INDEXTYPE a_index,
                                    const OMX_PTR ap_struct,
                                    tiz_vector_t * ap_changed_idxs)
{
  tiz_vorbisport_t * p_obj = ap_obj;
  tiz_port_t * p_base = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  /* OpenMAX IL 1.2 Section 3.5 : Slaving behaviour for nSamplingRate and
   * nChannels, both in OMX_AUDIO_PARAM_VORBISTYPE */

  assert (p_obj);
  assert (ap_struct);
  assert (ap_changed_idxs);

  {
    OMX_U32 new_rate = p_obj->vorbistype_.nSampleRate;
    OMX_U32 new_channels = p_obj->vorbistype_.nChannels;

    switch (a_index)
      {
        case OMX_IndexParamAudioPcm:
          {
            const OMX_AUDIO_PARAM_PCMMODETYPE * p_pcmmode = ap_struct;
            new_rate = p_pcmmode->nSamplingRate;
            new_channels = p_pcmmode->nChannels;

            TIZ_TRACE (handleOf (ap_obj),
                       "OMX_IndexParamAudioPcm : new sampling rate[%d] "
                       "new num channels[%d]",
                       new_rate, new_channels);
          }
          break;

        case OMX_IndexParamAudioMp3:
          {
            const OMX_AUDIO_PARAM_MP3TYPE * p_mp3type = ap_struct;
            new_rate = p_mp3type->nSampleRate;
            new_channels = p_mp3type->nChannels;

            TIZ_TRACE (handleOf (ap_obj),
                       "OMX_IndexParamAudioMp3 : new sampling rate[%d] "
                       "new num channels[%d]",
                       new_rate, new_channels);
          }
          break;

        case OMX_IndexParamAudioAac:
          {
            const OMX_AUDIO_PARAM_AACPROFILETYPE * p_aactype = ap_struct;
            new_rate = p_aactype->nSampleRate;
            new_channels = p_aactype->nChannels;

            TIZ_TRACE (handleOf (ap_obj),
                       "OMX_IndexParamAudioAac : new sampling rate[%d] "
                       "new num channels[%d]",
                       new_rate, new_channels);
          }
          break;

        case OMX_IndexParamAudioVorbis:
          {
            const OMX_AUDIO_PARAM_VORBISTYPE * p_vortype = ap_struct;
            new_rate = p_vortype->nSampleRate;
            new_channels = p_vortype->nChannels;

            TIZ_TRACE (handleOf (ap_obj),
                       "OMX_IndexParamAudioVorbis : new sampling rate[%d] "
                       "new num channels[%d]",
                       new_rate, new_channels);
          }
          break;

        case OMX_IndexParamAudioWma:
          {
            const OMX_AUDIO_PARAM_WMATYPE * p_wmatype = ap_struct;
            new_rate = p_wmatype->nSamplingRate;
            new_channels = p_wmatype->nChannels;

            TIZ_TRACE (handleOf (ap_obj),
                       "OMX_IndexParamAudioWma : new sampling rate[%d] "
                       "new num channels[%d]",
                       new_rate, new_channels);
          }
          break;

        case OMX_IndexParamAudioRa:
          {
            const OMX_AUDIO_PARAM_RATYPE * p_ratype = ap_struct;
            new_rate = p_ratype->nSamplingRate;
            new_channels = p_ratype->nChannels;

            TIZ_TRACE (handleOf (ap_obj),
                       "OMX_IndexParamAudioRa : new sampling rate[%d] "
                       "new num channels[%d]",
                       new_rate, new_channels);
          }
          break;

        case OMX_IndexParamAudioSbc:
          {
            const OMX_AUDIO_PARAM_SBCTYPE * p_sbctype = ap_struct;
            new_rate = p_sbctype->nSampleRate;
            new_channels = p_sbctype->nChannels;

            TIZ_TRACE (handleOf (ap_obj),
                       "OMX_IndexParamAudioSbc : new sampling rate[%d] "
                       "new num channels[%d]",
                       new_rate, new_channels);
          }
          break;

        case OMX_IndexParamAudioAdpcm:
          {
            const OMX_AUDIO_PARAM_ADPCMTYPE * p_adpcmtype = ap_struct;
            new_rate = p_adpcmtype->nSampleRate;
            new_channels = p_adpcmtype->nChannels;

            TIZ_TRACE (handleOf (ap_obj),
                       "OMX_IndexParamAudioAdpcm : new sampling rate[%d] "
                       "new num channels[%d]",
                       new_rate, new_channels);
          }
          break;

        default:
          {
            if (OMX_TizoniaIndexParamAudioOpus == a_index)
              {
                const OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE * p_opustype = ap_struct;
                new_rate = p_opustype->nSampleRate;
                new_channels = p_opustype->nChannels;

                TIZ_TRACE (handleOf (ap_obj),
                           "OMX_IndexParamAudioOpus : new sampling rate[%d] "
                           "new num channels[%d]",
                           new_rate, new_channels);
              }

            else if (OMX_TizoniaIndexParamAudioFlac == a_index)
              {
                const OMX_TIZONIA_AUDIO_PARAM_FLACTYPE * p_flactype = ap_struct;
                new_rate = p_flactype->nSampleRate;
                new_channels = p_flactype->nChannels;

                TIZ_TRACE (
                  handleOf (ap_obj),
                  "OMX_TizoniaIndexParamAudioFlac : new sampling rate[%d] "
                  "new num channels[%d]",
                  new_rate, new_channels);
              }

            else if (OMX_TizoniaIndexParamAudioMp2 == a_index)
              {
                const OMX_TIZONIA_AUDIO_PARAM_MP2TYPE * p_mp2type = ap_struct;
                new_rate = p_mp2type->nSampleRate;
                new_channels = p_mp2type->nChannels;

                TIZ_TRACE (
                  handleOf (ap_obj),
                  "OMX_TizoniaIndexParamAudioMp2 : new sampling rate[%d] "
                  "new num channels[%d]",
                  new_rate, new_channels);
              }
          }
      };

    if ((p_obj->vorbistype_.nSampleRate != new_rate)
        || (p_obj->vorbistype_.nChannels != new_channels))
      {
        OMX_INDEXTYPE id = OMX_IndexParamAudioVorbis;

        p_obj->vorbistype_.nSampleRate = new_rate;
        p_obj->vorbistype_.nChannels = new_channels;
        tiz_vector_push_back (ap_changed_idxs, &id);

        TIZ_TRACE (handleOf (ap_obj),
                   " original pid [%d] this pid [%d] : [%s] -> "
                   "changed [OMX_IndexParamAudioVorbis]...",
                   tiz_port_index (ap_mos_port), p_base->portdef_.nPortIndex,
                   tiz_idx_to_str (a_index));
      }
  }
  return rc;
}

/*
 * tizvorbisport_class
 */

static void *
vorbisport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizvorbisport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_vorbisport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizaudioport = tiz_get_type (ap_hdl, "tizaudioport");
  void * tizvorbisport_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizaudioport), "tizvorbisport_class", classOf (tizaudioport),
     sizeof (tiz_vorbisport_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, vorbisport_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizvorbisport_class;
}

void *
tiz_vorbisport_init (void * ap_tos, void * ap_hdl)
{
  void * tizaudioport = tiz_get_type (ap_hdl, "tizaudioport");
  void * tizvorbisport_class = tiz_get_type (ap_hdl, "tizvorbisport_class");
  TIZ_LOG_CLASS (tizvorbisport_class);
  void * tizvorbisport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizvorbisport_class, "tizvorbisport", tizaudioport,
     sizeof (tiz_vorbisport_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, vorbisport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, vorbisport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, vorbisport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, vorbisport_SetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_check_tunnel_compat, vorbisport_check_tunnel_compat,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_apply_slaving_behaviour, vorbisport_apply_slaving_behaviour,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizvorbisport;
}
