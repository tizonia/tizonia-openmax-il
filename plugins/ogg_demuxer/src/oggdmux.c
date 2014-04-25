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
 * @file   oggdmux.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Ogg demuxer component
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

#include <tizplatform.h>

#include <tizport.h>
#include <tizscheduler.h>

#include "oggdmuxprc.h"
#include "oggdmux.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.ogg_demuxer"
#endif

static OMX_VERSIONTYPE ogg_demuxer_version = { {1, 0, 0, 0} };

static OMX_PTR
instantiate_audio_output_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  OMX_AUDIO_CONFIG_MUTETYPE mute;
  OMX_AUDIO_CODINGTYPE encodings[] = {
    OMX_AUDIO_CodingUnused,
    OMX_AUDIO_CodingMax
  };
  tiz_port_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirOutput,
    ARATELIA_OGG_DEMUXER_PORT_MIN_BUF_COUNT,
    ARATELIA_OGG_DEMUXER_PORT_MIN_AUDIO_OUTPUT_BUF_SIZE,
    ARATELIA_OGG_DEMUXER_PORT_NONCONTIGUOUS,
    ARATELIA_OGG_DEMUXER_PORT_ALIGNMENT,
    ARATELIA_OGG_DEMUXER_PORT_SUPPLIERPREF,
    {ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX, NULL, NULL, NULL},
    -1                          /* use -1 for now */
  };

  /* The demuxer port expects to receive PCM mode, volume and mute structures
     when instantiated as an audio domain port */

  /* Initialize the pcm info */
  pcmmode.nSize              = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmmode.nVersion.nVersion  = OMX_VERSION;
  pcmmode.nPortIndex         = ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX;
  pcmmode.nChannels          = 2;
  pcmmode.eNumData           = OMX_NumericalDataSigned;
  pcmmode.eEndian            = OMX_EndianLittle;
  pcmmode.bInterleaved       = OMX_TRUE;
  pcmmode.nBitPerSample      = 16;
  pcmmode.nSamplingRate      = 48000;
  pcmmode.ePCMMode           = OMX_AUDIO_PCMModeLinear;
  pcmmode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  pcmmode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

  /* Initialize the pcm struct */
  volume.nSize             = sizeof (OMX_AUDIO_CONFIG_VOLUMETYPE);
  volume.nVersion.nVersion = OMX_VERSION;
  volume.nPortIndex        = ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX;
  volume.bLinear           = OMX_FALSE;
  volume.sVolume.nValue    = 50;
  volume.sVolume.nMin      = 0;
  volume.sVolume.nMax      = 100;

  /* Initialize the mute struct */
  mute.nSize             = sizeof (OMX_AUDIO_CONFIG_MUTETYPE);
  mute.nVersion.nVersion = OMX_VERSION;
  mute.nPortIndex        = ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX;
  mute.bMute             = OMX_FALSE;

  return factory_new (tiz_get_type (ap_hdl, "tizdemuxerport"),
                      &port_opts, &encodings,
                      &pcmmode, &volume, &mute);
}

static OMX_PTR
instantiate_video_output_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_VIDEO_PORTDEFINITIONTYPE portdef;
  OMX_VIDEO_CODINGTYPE encodings[] = {
    OMX_VIDEO_CodingUnused,
    OMX_VIDEO_CodingMax
  };
  OMX_COLOR_FORMATTYPE formats[] = {
    OMX_COLOR_FormatUnused,
    OMX_COLOR_FormatMax
  };
  tiz_port_options_t port_opts = {
    OMX_PortDomainVideo,
    OMX_DirOutput,
    ARATELIA_OGG_DEMUXER_PORT_MIN_BUF_COUNT,
    ARATELIA_OGG_DEMUXER_PORT_MIN_VIDEO_OUTPUT_BUF_SIZE,
    ARATELIA_OGG_DEMUXER_PORT_NONCONTIGUOUS,
    ARATELIA_OGG_DEMUXER_PORT_ALIGNMENT,
    ARATELIA_OGG_DEMUXER_PORT_SUPPLIERPREF,
    {ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX, NULL, NULL, NULL},
    -1                          /* use -1 for now */
  };

  /* The demuxer port expects to receive raw the same structures as in a normal
     raw video port when instantiated as a video domain port */

  /* NOTE: No defaults are defined in the standard for the video
   * output port of the video_reader.demuxer component. So for the
   * sake of completeness, simply provide some default values
   * here. */

  portdef.pNativeRender         = NULL;
  portdef.nFrameWidth           = 176;
  portdef.nFrameHeight          = 144;
  portdef.nStride               = 0;
  portdef.nSliceHeight          = 0;
  portdef.nBitrate              = 0;
  portdef.xFramerate            = 15;
  portdef.bFlagErrorConcealment = OMX_FALSE;
  portdef.eCompressionFormat    = OMX_VIDEO_CodingUnused;
  portdef.eColorFormat          = OMX_COLOR_FormatYUV420Planar;
  portdef.pNativeWindow         = NULL;

  return factory_new (tiz_get_type (ap_hdl, "tizdemuxerport"),
                      &port_opts, &portdef,
                      &encodings, &formats);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizdemuxercfgport"),
                      NULL,   /* this port does not take options */
                      ARATELIA_OGG_DEMUXER_COMPONENT_NAME,
                      ogg_demuxer_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "oggdmuxprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t *rf_list[] = { &role_factory };
  tiz_type_factory_t oggdmuxprc_type;
  const tiz_type_factory_t *tf_list[] = { &oggdmuxprc_type };

  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_ComponentInit: "
           "Inititializing [%s]", ARATELIA_OGG_DEMUXER_COMPONENT_NAME);

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_OGG_DEMUXER_DEFAULT_ROLE);
  role_factory.pf_cport   = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_audio_output_port;
  role_factory.pf_port[1] = instantiate_video_output_port;
  /* TODO : Add clock output port */
  role_factory.nports     = 2;
  role_factory.pf_proc    = instantiate_processor;

  strcpy ((OMX_STRING) oggdmuxprc_type.class_name, "oggdmuxprc_class");
  oggdmuxprc_type.pf_class_init = oggdmux_prc_class_init;
  strcpy ((OMX_STRING) oggdmuxprc_type.object_name, "oggdmuxprc");
  oggdmuxprc_type.pf_object_init = oggdmux_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx_err (tiz_comp_init (ap_hdl, ARATELIA_OGG_DEMUXER_COMPONENT_NAME));

  /* Register the "oggdmuxprc" class */
  tiz_check_omx_err (tiz_comp_register_types (ap_hdl, tf_list, 1));

  /* Register the various roles */
  tiz_check_omx_err (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
