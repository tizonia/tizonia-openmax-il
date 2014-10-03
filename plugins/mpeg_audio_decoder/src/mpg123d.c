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
 * @file   mpg123d.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - MPEG audio decoder component based on libmpg123
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include <tizport.h>
#include <tizscheduler.h>

#include "mpg123dprc.h"
#include "mpg123d.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.mpg123_decoder"
#endif

static OMX_VERSIONTYPE mp3_decoder_version = { { 1, 0, 0, 0 } };

static OMX_PTR instantiate_mp3_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_MP3TYPE mp3type;
  OMX_AUDIO_CODINGTYPE encodings[]
      = { OMX_AUDIO_CodingMP3, OMX_AUDIO_CodingMax };
  tiz_port_options_t mp3_port_opts
      = { OMX_PortDomainAudio,
          OMX_DirInput,
          ARATELIA_MPG123_DECODER_PORT_MIN_BUF_COUNT,
          ARATELIA_MPG123_DECODER_PORT_MIN_INPUT_BUF_SIZE,
          ARATELIA_MPG123_DECODER_PORT_NONCONTIGUOUS,
          ARATELIA_MPG123_DECODER_PORT_ALIGNMENT,
          ARATELIA_MPG123_DECODER_PORT_SUPPLIERPREF,
          { ARATELIA_MPG123_DECODER_INPUT_PORT_INDEX, NULL, NULL, NULL },
          1 /* slave port's index  */
      };

  mp3type.nSize = sizeof(OMX_AUDIO_PARAM_MP3TYPE);
  mp3type.nVersion.nVersion = OMX_VERSION;
  mp3type.nPortIndex = 0;
  mp3type.nChannels = 2;
  mp3type.nBitRate = 0;
  mp3type.nSampleRate = 0;
  mp3type.nAudioBandWidth = 0;
  mp3type.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  mp3type.eFormat = OMX_AUDIO_MP3StreamFormatMP1Layer3;

  return factory_new (tiz_get_type (ap_hdl, "tizmp3port"), &mp3_port_opts,
                      &encodings, &mp3type);
}

static OMX_PTR instantiate_mp2_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_TIZONIA_AUDIO_PARAM_MP2TYPE mp2type;
  OMX_AUDIO_CODINGTYPE encodings[]
      = { OMX_AUDIO_CodingMP2, OMX_AUDIO_CodingMax };
  tiz_port_options_t mp2_port_opts
      = { OMX_PortDomainAudio,
          OMX_DirInput,
          ARATELIA_MPG123_DECODER_PORT_MIN_BUF_COUNT,
          ARATELIA_MPG123_DECODER_PORT_MIN_INPUT_BUF_SIZE,
          ARATELIA_MPG123_DECODER_PORT_NONCONTIGUOUS,
          ARATELIA_MPG123_DECODER_PORT_ALIGNMENT,
          ARATELIA_MPG123_DECODER_PORT_SUPPLIERPREF,
          { ARATELIA_MPG123_DECODER_INPUT_PORT_INDEX, NULL, NULL, NULL },
          1 /* slave port's index  */
      };

  mp2type.nSize = sizeof(OMX_TIZONIA_AUDIO_PARAM_MP2TYPE);
  mp2type.nVersion.nVersion = OMX_VERSION;
  mp2type.nPortIndex = 0;
  mp2type.nChannels = 2;
  mp2type.nBitRate = 0;
  mp2type.nSampleRate = 0;
  mp2type.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  mp2type.eFormat = OMX_AUDIO_MP2StreamFormatMP2Layer2;

  return factory_new (tiz_get_type (ap_hdl, "tizmp2port"), &mp2_port_opts,
                      &encodings, &mp2type);
}

static OMX_PTR instantiate_pcm_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  OMX_AUDIO_CONFIG_MUTETYPE mute;
  OMX_AUDIO_CODINGTYPE encodings[]
      = { OMX_AUDIO_CodingPCM, OMX_AUDIO_CodingMax };
  tiz_port_options_t pcm_port_opts
      = { OMX_PortDomainAudio,
          OMX_DirOutput,
          ARATELIA_MPG123_DECODER_PORT_MIN_BUF_COUNT,
          ARATELIA_MPG123_DECODER_PORT_MIN_OUTPUT_BUF_SIZE,
          ARATELIA_MPG123_DECODER_PORT_NONCONTIGUOUS,
          ARATELIA_MPG123_DECODER_PORT_ALIGNMENT,
          ARATELIA_MPG123_DECODER_PORT_SUPPLIERPREF,
          { ARATELIA_MPG123_DECODER_OUTPUT_PORT_INDEX, NULL, NULL, NULL },
          0 /* Master port */
      };

  /* Instantiate the pcm port */
  pcmmode.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmmode.nVersion.nVersion = OMX_VERSION;
  pcmmode.nPortIndex = ARATELIA_MPG123_DECODER_OUTPUT_PORT_INDEX;
  pcmmode.nChannels = 2;
  pcmmode.eNumData = OMX_NumericalDataSigned;
  pcmmode.eEndian = OMX_EndianBig;
  pcmmode.bInterleaved = OMX_TRUE;
  pcmmode.nBitPerSample = 16;
  pcmmode.nSamplingRate = 48000;
  pcmmode.ePCMMode = OMX_AUDIO_PCMModeLinear;
  pcmmode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  pcmmode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

  volume.nSize = sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE);
  volume.nVersion.nVersion = OMX_VERSION;
  volume.nPortIndex = ARATELIA_MPG123_DECODER_OUTPUT_PORT_INDEX;
  volume.bLinear = OMX_FALSE;
  volume.sVolume.nValue = 50;
  volume.sVolume.nMin = 0;
  volume.sVolume.nMax = 100;

  mute.nSize = sizeof(OMX_AUDIO_CONFIG_MUTETYPE);
  mute.nVersion.nVersion = OMX_VERSION;
  mute.nPortIndex = ARATELIA_MPG123_DECODER_OUTPUT_PORT_INDEX;
  mute.bMute = OMX_FALSE;

  return factory_new (tiz_get_type (ap_hdl, "tizpcmport"), &pcm_port_opts,
                      &encodings, &pcmmode, &volume, &mute);
}

static OMX_PTR instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizconfigport"),
                      NULL, /* this port does not take options */
                      ARATELIA_MPG123_DECODER_COMPONENT_NAME,
                      mp3_decoder_version);
}

static OMX_PTR instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "mpg123dprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t mp3_role;
  tiz_role_factory_t mp2_role;
  const tiz_role_factory_t *rf_list[] = { &mp3_role, &mp2_role };
  tiz_type_factory_t mpg123dprc_type;
  const tiz_type_factory_t *tf_list[] = { &mpg123dprc_type };

  strcpy ((OMX_STRING)mp3_role.role, ARATELIA_MPG123_DECODER_MP3_ROLE);
  mp3_role.pf_cport = instantiate_config_port;
  mp3_role.pf_port[0] = instantiate_mp3_port;
  mp3_role.pf_port[1] = instantiate_pcm_port;
  mp3_role.nports = 2;
  mp3_role.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING)mp2_role.role, ARATELIA_MPG123_DECODER_MP2_ROLE);
  mp2_role.pf_cport = instantiate_config_port;
  mp2_role.pf_port[0] = instantiate_mp2_port;
  mp2_role.pf_port[1] = instantiate_pcm_port;
  mp2_role.nports = 2;
  mp2_role.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING)mpg123dprc_type.class_name, "mpg123dprc_class");
  mpg123dprc_type.pf_class_init = mpg123d_prc_class_init;
  strcpy ((OMX_STRING)mpg123dprc_type.object_name, "mpg123dprc");
  mpg123dprc_type.pf_object_init = mpg123d_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx_err (
      tiz_comp_init (ap_hdl, ARATELIA_MPG123_DECODER_COMPONENT_NAME));

  /* Register the "mpg123dprc" class */
  tiz_check_omx_err (tiz_comp_register_types (ap_hdl, tf_list, 1));

  /* Register the component role(s) */
  tiz_check_omx_err (tiz_comp_register_roles (ap_hdl, rf_list, 2));

  return OMX_ErrorNone;
}
