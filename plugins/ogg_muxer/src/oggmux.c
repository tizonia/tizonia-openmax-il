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
 * @file   oggmux.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Ogg Muxer component
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
#include <tizoggport.h>
#include <tizmuxerport.h>
#include <tizconfigport.h>
#include <tizscheduler.h>

#include "oggmuxsnkprc.h"
#include "oggmuxfltprc.h"
#include "oggmux.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.ogg_muxer"
#endif

/**
 *@defgroup tizoggmux 'tizoggmux' : OpenMAX IL Ogg Muxer
 *
 * - Component name : "OMX.Aratelia.container_muxer.ogg"
 * - Implements role: "container_muxer.sink.ogg"
 * - Implements role: "container_muxer.filter.ogg"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE ogg_muxer_version = {{1, 0, 0, 0}};

static OMX_PTR
instantiate_filter_ogg_output_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_CODINGTYPE encodings[]
    = {OMX_AUDIO_CodingUnused, OMX_AUDIO_CodingMax};
  tiz_port_options_t port_opts = {
    OMX_PortDomainOther,
    OMX_DirOutput,
    ARATELIA_OGG_MUXER_OGG_PORT_MIN_BUF_COUNT,
    ARATELIA_OGG_MUXER_OGG_PORT_MIN_BUF_SIZE,
    ARATELIA_OGG_MUXER_OGG_PORT_NONCONTIGUOUS,
    ARATELIA_OGG_MUXER_OGG_PORT_ALIGNMENT,
    ARATELIA_OGG_MUXER_OGG_PORT_SUPPLIERPREF,
    {ARATELIA_OGG_MUXER_FILTER_PORT_2_INDEX, NULL, NULL, NULL},
    -1 /* use -1 for now */
  };

  return factory_new (tiz_get_type (ap_hdl, "tizoggport"), &port_opts,
                      &encodings);
}

static OMX_PTR
instantiate_audio_input_port (OMX_HANDLETYPE ap_hdl, const OMX_U32 port_id)
{
  OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE opustype;
  OMX_AUDIO_CODINGTYPE encodings[]
    = {(OMX_AUDIO_CODINGTYPE) OMX_AUDIO_CodingOPUS, OMX_AUDIO_CodingMax};
  tiz_port_options_t opus_port_opts = {
    OMX_PortDomainAudio,
    OMX_DirInput,
    ARATELIA_OGG_MUXER_AUDIO_PORT_MIN_BUF_COUNT,
    ARATELIA_OGG_MUXER_AUDIO_PORT_MIN_BUF_SIZE,
    ARATELIA_OGG_MUXER_AUDIO_PORT_NONCONTIGUOUS,
    ARATELIA_OGG_MUXER_AUDIO_PORT_ALIGNMENT,
    ARATELIA_OGG_MUXER_AUDIO_PORT_SUPPLIERPREF,
    {port_id, NULL, NULL, NULL},
    1 /* slave port's index  */
  };

  /* Instantiate the muxer port as an OPUS audio port */
  /* For now, this is the only audio encoding type supported in this muxer
     component */

  opustype.nSize = sizeof (OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE);
  opustype.nVersion.nVersion = OMX_VERSION;
  opustype.nPortIndex = port_id;
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

  return factory_new (tiz_get_type (ap_hdl, "tizmuxerport"), &opus_port_opts,
                      &encodings, &opustype);
}

static OMX_PTR
instantiate_sink_audio_input_port (OMX_HANDLETYPE ap_hdl)
{
  return instantiate_audio_input_port (ap_hdl,
                                       ARATELIA_OGG_MUXER_SINK_PORT_0_INDEX);
}

static OMX_PTR
instantiate_filter_audio_input_port (OMX_HANDLETYPE ap_hdl)
{
  return instantiate_audio_input_port (ap_hdl,
                                       ARATELIA_OGG_MUXER_FILTER_PORT_0_INDEX);
}

static OMX_PTR
instantiate_video_input_port (OMX_HANDLETYPE ap_hdl, const OMX_U32 port_id)
{
  OMX_VIDEO_PORTDEFINITIONTYPE portdef;
  OMX_VIDEO_CODINGTYPE encodings[]
    = {OMX_VIDEO_CodingUnused, OMX_VIDEO_CodingMax};
  OMX_COLOR_FORMATTYPE formats[]
    = {OMX_COLOR_FormatUnused, OMX_COLOR_FormatMax};
  tiz_port_options_t port_opts = {
    OMX_PortDomainVideo,
    OMX_DirInput,
    ARATELIA_OGG_MUXER_VIDEO_PORT_MIN_BUF_COUNT,
    ARATELIA_OGG_MUXER_VIDEO_PORT_MIN_BUF_SIZE,
    ARATELIA_OGG_MUXER_VIDEO_PORT_NONCONTIGUOUS,
    ARATELIA_OGG_MUXER_VIDEO_PORT_ALIGNMENT,
    ARATELIA_OGG_MUXER_VIDEO_PORT_SUPPLIERPREF,
    {port_id, NULL, NULL, NULL},
    -1 /* use -1 for now */
  };

  /* The muxer port expects to receive raw the same structures as in a normal
     raw video port when instantiated as a video domain port */

  /* NOTE: No defaults are defined in the standard for the video
   * output port of the video_reader.muxer component. So for the
   * sake of completeness, simply provide some default values
   * here. */

  portdef.pNativeRender = NULL;
  portdef.nFrameWidth = 176;
  portdef.nFrameHeight = 144;
  portdef.nStride = 0;
  portdef.nSliceHeight = 0;
  portdef.nBitrate = 0;
  portdef.xFramerate = 15;
  portdef.bFlagErrorConcealment = OMX_FALSE;
  portdef.eCompressionFormat = OMX_VIDEO_CodingUnused;
  portdef.eColorFormat = OMX_COLOR_FormatYUV420Planar;
  portdef.pNativeWindow = NULL;

  return factory_new (tiz_get_type (ap_hdl, "tizmuxerport"), &port_opts,
                      &portdef, &encodings, &formats);
}

static OMX_PTR
instantiate_sink_video_input_port (OMX_HANDLETYPE ap_hdl)
{
  return instantiate_video_input_port (ap_hdl,
                                       ARATELIA_OGG_MUXER_SINK_PORT_1_INDEX);
}

static OMX_PTR
instantiate_filter_video_input_port (OMX_HANDLETYPE ap_hdl)
{
  return instantiate_video_input_port (ap_hdl,
                                       ARATELIA_OGG_MUXER_FILTER_PORT_1_INDEX);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizconfigport"),
                      NULL, /* this port does not take options */
                      ARATELIA_OGG_MUXER_COMPONENT_NAME, ogg_muxer_version);
}

static OMX_PTR
instantiate_sink_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "oggmuxsnkprc"));
}

static OMX_PTR
instantiate_filter_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "oggmuxfltprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t sink_role_factory;
  tiz_role_factory_t filter_role_factory;
  const tiz_role_factory_t * rf_list[]
    = {&sink_role_factory, &filter_role_factory};

  tiz_type_factory_t oggmuxsnkprc_type;
  tiz_type_factory_t oggmuxfltprc_type;
  const tiz_type_factory_t * tf_list[]
    = {&oggmuxsnkprc_type, &oggmuxfltprc_type};

  strcpy ((OMX_STRING) sink_role_factory.role, ARATELIA_OGG_MUXER_SINK_ROLE);
  sink_role_factory.pf_cport = instantiate_config_port;
  sink_role_factory.pf_port[0] = instantiate_sink_audio_input_port;
  sink_role_factory.pf_port[1] = instantiate_sink_video_input_port;
  sink_role_factory.nports = 2;
  sink_role_factory.pf_proc = instantiate_sink_processor;

  strcpy ((OMX_STRING) filter_role_factory.role,
          ARATELIA_OGG_MUXER_FILTER_ROLE);
  filter_role_factory.pf_cport = instantiate_config_port;
  filter_role_factory.pf_port[0] = instantiate_filter_audio_input_port;
  filter_role_factory.pf_port[1] = instantiate_filter_video_input_port;
  filter_role_factory.pf_port[2] = instantiate_filter_ogg_output_port;
  filter_role_factory.nports = 3;
  filter_role_factory.pf_proc = instantiate_filter_processor;

  strcpy ((OMX_STRING) oggmuxsnkprc_type.class_name, "oggmuxsnkprc_class");
  oggmuxsnkprc_type.pf_class_init = oggmuxsnk_prc_class_init;
  strcpy ((OMX_STRING) oggmuxsnkprc_type.object_name, "oggmuxsnkprc");
  oggmuxsnkprc_type.pf_object_init = oggmuxsnk_prc_init;

  strcpy ((OMX_STRING) oggmuxfltprc_type.class_name, "oggmuxfltprc_class");
  oggmuxfltprc_type.pf_class_init = oggmuxflt_prc_class_init;
  strcpy ((OMX_STRING) oggmuxfltprc_type.object_name, "oggmuxfltprc");
  oggmuxfltprc_type.pf_object_init = oggmuxflt_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx (tiz_comp_init (ap_hdl, ARATELIA_OGG_MUXER_COMPONENT_NAME));

  /* Register the various classes */
  tiz_check_omx (tiz_comp_register_types (ap_hdl, tf_list, 2));

  /* Register the component roles */
  tiz_check_omx (tiz_comp_register_roles (ap_hdl, rf_list, 2));

  return OMX_ErrorNone;
}
