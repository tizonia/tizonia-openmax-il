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
 * @file   fr.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Tizonia OpenMAX IL - File Reader component
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "frprc.h"
#include "tizosal.h"
#include "tizscheduler.h"
#include "tizmp3port.h"
#include "tizpcmport.h"
#include "tizconfigport.h"

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Types.h"

#include <assert.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.file_reader"
#endif

#define ARATELIA_FILE_READER_DEFAULT_ROLE   "file_reader.binary"
#define ARATELIA_FILE_READER_COMPONENT_NAME "OMX.Aratelia.file_reader.binary"
/* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_FILE_DECODER_INPUT_PORT_INDEX        0
#define ARATELIA_FILE_DECODER_OUTPUT_PORT_INDEX       1
#define ARATELIA_FILE_READER_PORT_MIN_BUF_COUNT       2
#define ARATELIA_FILE_READER_PORT_MIN_INPUT_BUF_SIZE  8192
#define ARATELIA_FILE_READER_PORT_MIN_OUTPUT_BUF_SIZE 8192
#define ARATELIA_FILE_READER_PORT_NONCONTIGUOUS       OMX_FALSE
#define ARATELIA_FILE_READER_PORT_ALIGNMENT           0
#define ARATELIA_FILE_READER_PORT_SUPPLIERPREF        OMX_BufferSupplyInput

static OMX_VERSIONTYPE file_reader_version = { {1, 0, 0, 0} };

static OMX_PTR
instantiate_input_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_MP3TYPE mp3type;
  OMX_AUDIO_CODINGTYPE encodings[] = {
    OMX_AUDIO_CodingMP3,
    OMX_AUDIO_CodingMax
  };
  tiz_port_options_t mp3_port_opts = {
    OMX_PortDomainAudio,
    OMX_DirInput,
    ARATELIA_FILE_READER_PORT_MIN_BUF_COUNT,
    ARATELIA_FILE_READER_PORT_MIN_INPUT_BUF_SIZE,
    ARATELIA_FILE_READER_PORT_NONCONTIGUOUS,
    ARATELIA_FILE_READER_PORT_ALIGNMENT,
    ARATELIA_FILE_READER_PORT_SUPPLIERPREF,
    {ARATELIA_FILE_DECODER_INPUT_PORT_INDEX, NULL, NULL, NULL},
    1                           /* slave port's index  */
  };

  mp3type.nSize             = sizeof (OMX_AUDIO_PARAM_MP3TYPE);
  mp3type.nVersion.nVersion = OMX_VERSION;
  mp3type.nPortIndex        = 0;
  mp3type.nChannels         = 2;
  mp3type.nBitRate          = 0;
  mp3type.nSampleRate       = 0;
  mp3type.nAudioBandWidth   = 0;
  mp3type.eChannelMode      = OMX_AUDIO_ChannelModeStereo;
  mp3type.eFormat           = OMX_AUDIO_MP3StreamFormatMP1Layer3;

  tiz_check_omx_err_ret_null (init_tizmp3port ());
  return factory_new (tizmp3port, ap_hdl, &mp3_port_opts, &encodings, &mp3type);
}

static OMX_PTR
instantiate_output_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  OMX_AUDIO_CONFIG_MUTETYPE mute;
  OMX_AUDIO_CODINGTYPE encodings[] = {
    OMX_AUDIO_CodingPCM,
    OMX_AUDIO_CodingMax
  };
  tiz_port_options_t pcm_port_opts = {
    OMX_PortDomainAudio,
    OMX_DirOutput,
    ARATELIA_FILE_READER_PORT_MIN_BUF_COUNT,
    ARATELIA_FILE_READER_PORT_MIN_OUTPUT_BUF_SIZE,
    ARATELIA_FILE_READER_PORT_NONCONTIGUOUS,
    ARATELIA_FILE_READER_PORT_ALIGNMENT,
    ARATELIA_FILE_READER_PORT_SUPPLIERPREF,
    {ARATELIA_FILE_DECODER_OUTPUT_PORT_INDEX, NULL, NULL, NULL},
    0                           /* Master port */
  };

  /* Instantiate the pcm port */
  pcmmode.nSize              = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmmode.nVersion.nVersion  = OMX_VERSION;
  pcmmode.nPortIndex         = 1;
  pcmmode.nChannels          = 2;
  pcmmode.eNumData           = OMX_NumericalDataSigned;
  pcmmode.eEndian            = OMX_EndianBig;
  pcmmode.bInterleaved       = OMX_TRUE;
  pcmmode.nBitPerSample      = 16;
  pcmmode.nSamplingRate      = 48000;
  pcmmode.ePCMMode           = OMX_AUDIO_PCMModeLinear;
  pcmmode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  pcmmode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

  volume.nSize             = sizeof (OMX_AUDIO_CONFIG_VOLUMETYPE);
  volume.nVersion.nVersion = OMX_VERSION;
  volume.nPortIndex        = 1;
  volume.bLinear           = OMX_FALSE;
  volume.sVolume.nValue    = 50;
  volume.sVolume.nMin      = 0;
  volume.sVolume.nMax      = 100;

  mute.nSize             = sizeof (OMX_AUDIO_CONFIG_MUTETYPE);
  mute.nVersion.nVersion = OMX_VERSION;
  mute.nPortIndex        = 1;
  mute.bMute             = OMX_FALSE;
  
  tiz_check_omx_err_ret_null (tiz_pcmport_init ());
  return factory_new (tizpcmport, ap_hdl, &pcm_port_opts, &encodings,
                      &pcmmode, &volume, &mute);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  tiz_check_omx_err_ret_null (tiz_configport_init ());
  return factory_new (tizconfigport, ap_hdl, NULL,   /* this port does not take options */
                      ARATELIA_FILE_READER_COMPONENT_NAME,
                      file_reader_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  tiz_check_omx_err_ret_null (fr_prc_init ());
  return factory_new (frprc, ap_hdl);
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t *rf_list[] = { &role_factory };

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_FILE_READER_DEFAULT_ROLE);
  role_factory.pf_cport   = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_input_port;
  role_factory.pf_port[1] = instantiate_output_port;
  role_factory.nports     = 2;
  role_factory.pf_proc    = instantiate_processor;

  TIZ_LOG (TIZ_TRACE, "OMX_ComponentInit: "
           "Inititializing [%s]", ARATELIA_FILE_READER_COMPONENT_NAME);

  tiz_check_omx_err (tiz_comp_init (ap_hdl, ARATELIA_FILE_READER_COMPONENT_NAME));
  tiz_check_omx_err (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
