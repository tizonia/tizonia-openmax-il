/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   opusfiled.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Opus Decoder (libopusfile-based) component
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

#include "opusfiledprc.h"
#include "opusfiled.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.opusfile_decoder"
#endif

/**
 *@defgroup libtizopusfiledec 'libtizopusfiledec' : OpenMAX IL OPUS decoder
 *
 * - Component name : "OMX.Aratelia.audio_decoder.opusfile.opus"
 * - Implements role: "audio_decoder.opus"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE opus_decoder_version = {{1, 0, 0, 0}};

static OMX_PTR
instantiate_opus_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE opustype;
  OMX_AUDIO_CODINGTYPE encodings[]
    = {(OMX_AUDIO_CODINGTYPE) OMX_AUDIO_CodingOPUS, OMX_AUDIO_CodingMax};
  tiz_port_options_t opus_port_opts = {
    OMX_PortDomainAudio,
    OMX_DirInput,
    ARATELIA_OPUS_DECODER_PORT_MIN_BUF_COUNT,
    ARATELIA_OPUS_DECODER_PORT_MIN_INPUT_BUF_SIZE,
    ARATELIA_OPUS_DECODER_PORT_NONCONTIGUOUS,
    ARATELIA_OPUS_DECODER_PORT_ALIGNMENT,
    ARATELIA_OPUS_DECODER_PORT_SUPPLIERPREF,
    {ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX, NULL, NULL, NULL},
    1 /* slave port's index  */
  };

  opustype.nSize = sizeof (OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE);
  opustype.nVersion.nVersion = OMX_VERSION;
  opustype.nPortIndex = ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX;
  opustype.nChannels = 2;
  opustype.nBitRate = 256;
  opustype.nSampleRate = 48000;
  opustype.nFrameDuration = 2;
  opustype.nEncoderComplexity = 0;
  opustype.bPacketLossResilience = OMX_FALSE;
  opustype.bForwardErrorCorrection = OMX_FALSE;
  opustype.bDtx = OMX_FALSE;
  opustype.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  opustype.eFormat = OMX_AUDIO_OPUSStreamFormatVBR;

  return factory_new (tiz_get_type (ap_hdl, "tizopusport"), &opus_port_opts,
                      &encodings, &opustype);
}

static OMX_PTR
instantiate_pcm_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  OMX_AUDIO_CONFIG_MUTETYPE mute;
  OMX_AUDIO_CODINGTYPE encodings[] = {OMX_AUDIO_CodingPCM, OMX_AUDIO_CodingMax};
  tiz_port_options_t pcm_port_opts = {
    OMX_PortDomainAudio,
    OMX_DirOutput,
    ARATELIA_OPUS_DECODER_PORT_MIN_BUF_COUNT,
    ARATELIA_OPUS_DECODER_PORT_MIN_OUTPUT_BUF_SIZE,
    ARATELIA_OPUS_DECODER_PORT_NONCONTIGUOUS,
    ARATELIA_OPUS_DECODER_PORT_ALIGNMENT,
    ARATELIA_OPUS_DECODER_PORT_SUPPLIERPREF,
    {ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX, NULL, NULL, NULL},
    0 /* Master port */
  };

  /* Instantiate the pcm port */
  pcmmode.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmmode.nVersion.nVersion = OMX_VERSION;
  pcmmode.nPortIndex = ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX;
  pcmmode.nChannels = 2;
  pcmmode.eNumData = OMX_NumericalDataSigned;
  pcmmode.eEndian = OMX_EndianLittle;
  pcmmode.bInterleaved = OMX_TRUE;
  pcmmode.nBitPerSample
    = 32; /* This component will output 32-bit float samples */
  pcmmode.nSamplingRate = 48000;
  pcmmode.ePCMMode = OMX_AUDIO_PCMModeLinear;
  pcmmode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  pcmmode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

  volume.nSize = sizeof (OMX_AUDIO_CONFIG_VOLUMETYPE);
  volume.nVersion.nVersion = OMX_VERSION;
  volume.nPortIndex = ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX;
  volume.bLinear = OMX_FALSE;
  volume.sVolume.nValue = 50;
  volume.sVolume.nMin = 0;
  volume.sVolume.nMax = 100;

  mute.nSize = sizeof (OMX_AUDIO_CONFIG_MUTETYPE);
  mute.nVersion.nVersion = OMX_VERSION;
  mute.nPortIndex = ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX;
  mute.bMute = OMX_FALSE;

  return factory_new (tiz_get_type (ap_hdl, "tizpcmport"), &pcm_port_opts,
                      &encodings, &pcmmode, &volume, &mute);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizconfigport"),
                      NULL, /* this port does not take options */
                      ARATELIA_OPUS_DECODER_COMPONENT_NAME,
                      opus_decoder_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "opusfiledprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t * rf_list[] = {&role_factory};
  tiz_type_factory_t opusfiledprc_type;
  const tiz_type_factory_t * tf_list[] = {&opusfiledprc_type};

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_OPUS_DECODER_DEFAULT_ROLE);
  role_factory.pf_cport = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_opus_port;
  role_factory.pf_port[1] = instantiate_pcm_port;
  role_factory.nports = 2;
  role_factory.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING) opusfiledprc_type.class_name, "opusfiledprc_class");
  opusfiledprc_type.pf_class_init = opusfiled_prc_class_init;
  strcpy ((OMX_STRING) opusfiledprc_type.object_name, "opusfiledprc");
  opusfiledprc_type.pf_object_init = opusfiled_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx (tiz_comp_init (ap_hdl, ARATELIA_OPUS_DECODER_COMPONENT_NAME));

  /* Register the "opusfiledprc" class */
  tiz_check_omx (tiz_comp_register_types (ap_hdl, tf_list, 1));

  /* Register the component role(s) */
  tiz_check_omx (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
