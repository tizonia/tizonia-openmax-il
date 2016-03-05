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
 * @file   vorbisd.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Vorbis decoder component
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <tizplatform.h>

#include <tizscheduler.h>
#include <tizport.h>

#include "vorbisd.h"
#include "vorbisdprc.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.vorbis_decoder"
#endif

/**
 *@defgroup libtizvorbisdec 'libtizvorbisdec' : OpenMAX IL Vorbis decoder
 *
 * - Component name : "OMX.Aratelia.audio_decoder.vorbis"
 * - Implements role: "audio_decoder.vorbis"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE vorbis_decoder_version = { { 1, 0, 0, 0 } };

static OMX_PTR instantiate_input_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_VORBISTYPE vorbistype;
  OMX_AUDIO_CODINGTYPE encodings[]
      = { OMX_AUDIO_CodingVORBIS, OMX_AUDIO_CodingMax };
  tiz_port_options_t vorbis_port_opts
      = { OMX_PortDomainAudio,
          OMX_DirInput,
          ARATELIA_VORBIS_DECODER_PORT_MIN_BUF_COUNT,
          ARATELIA_VORBIS_DECODER_PORT_MIN_INPUT_BUF_SIZE,
          ARATELIA_VORBIS_DECODER_PORT_NONCONTIGUOUS,
          ARATELIA_VORBIS_DECODER_PORT_ALIGNMENT,
          ARATELIA_VORBIS_DECODER_PORT_SUPPLIERPREF,
          { ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX, NULL, NULL, NULL },
          1 /* slave port's index  */
      };

  vorbistype.nSize = sizeof(OMX_AUDIO_PARAM_VORBISTYPE);
  vorbistype.nVersion.nVersion = OMX_VERSION;
  vorbistype.nPortIndex = 0;
  vorbistype.nChannels = 2;
  vorbistype.nBitRate = 0;
  vorbistype.nMinBitRate = 0;
  vorbistype.nMaxBitRate = 0;
  vorbistype.nSampleRate = 48000;
  vorbistype.nAudioBandWidth = 0;
  vorbistype.nQuality = 5;
  vorbistype.bManaged = OMX_FALSE;
  vorbistype.bDownmix = OMX_FALSE;
  return factory_new (tiz_get_type (ap_hdl, "tizvorbisport"), &vorbis_port_opts,
                      &encodings, &vorbistype);
}

static OMX_PTR instantiate_output_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  OMX_AUDIO_CONFIG_MUTETYPE mute;
  OMX_AUDIO_CODINGTYPE encodings[]
      = { OMX_AUDIO_CodingPCM, OMX_AUDIO_CodingMax };
  tiz_port_options_t pcm_port_opts
      = { OMX_PortDomainAudio,
          OMX_DirOutput,
          ARATELIA_VORBIS_DECODER_PORT_MIN_BUF_COUNT,
          ARATELIA_VORBIS_DECODER_PORT_MIN_OUTPUT_BUF_SIZE,
          ARATELIA_VORBIS_DECODER_PORT_NONCONTIGUOUS,
          ARATELIA_VORBIS_DECODER_PORT_ALIGNMENT,
          ARATELIA_VORBIS_DECODER_PORT_SUPPLIERPREF,
          { ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX, NULL, NULL, NULL },
          0 /* Master port */
      };

  /* Instantiate the pcm port */
  pcmmode.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmmode.nVersion.nVersion = OMX_VERSION;
  pcmmode.nPortIndex = 1;
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
  volume.nPortIndex = 1;
  volume.bLinear = OMX_FALSE;
  volume.sVolume.nValue = 50;
  volume.sVolume.nMin = 0;
  volume.sVolume.nMax = 100;

  mute.nSize = sizeof(OMX_AUDIO_CONFIG_MUTETYPE);
  mute.nVersion.nVersion = OMX_VERSION;
  mute.nPortIndex = 1;
  mute.bMute = OMX_FALSE;

  return factory_new (tiz_get_type (ap_hdl, "tizpcmport"), &pcm_port_opts,
                      &encodings, &pcmmode, &volume, &mute);
}

static OMX_PTR instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizconfigport"),
                      NULL, /* this port does not take options */
                      ARATELIA_VORBIS_DECODER_COMPONENT_NAME,
                      vorbis_decoder_version);
}

static OMX_PTR instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "vorbisdprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t *rf_list[] = { &role_factory };
  tiz_type_factory_t vorbisdprc_type;
  const tiz_type_factory_t *tf_list[] = { &vorbisdprc_type };

  strcpy ((OMX_STRING)role_factory.role, ARATELIA_VORBIS_DECODER_DEFAULT_ROLE);
  role_factory.pf_cport = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_input_port;
  role_factory.pf_port[1] = instantiate_output_port;
  role_factory.nports = 2;
  role_factory.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING)vorbisdprc_type.class_name, "vorbisdprc_class");
  vorbisdprc_type.pf_class_init = vorbisd_prc_class_init;
  strcpy ((OMX_STRING)vorbisdprc_type.object_name, "vorbisdprc");
  vorbisdprc_type.pf_object_init = vorbisd_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx_err (
      tiz_comp_init (ap_hdl, ARATELIA_VORBIS_DECODER_COMPONENT_NAME));

  /* Register the "vorbisdprc" class */
  tiz_check_omx_err (tiz_comp_register_types (ap_hdl, tf_list, 1));

  /* Register the various roles */
  tiz_check_omx_err (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
