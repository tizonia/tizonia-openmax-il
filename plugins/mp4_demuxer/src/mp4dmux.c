/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   mp4dmux.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - MP4 Demuxer component
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>
#include <assert.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include <tizport.h>
/* #include <tizmp4port.h> */
#include <tizdemuxerport.h>
#include <tizdemuxercfgport.h>
#include <tizscheduler.h>

#include "mp4dmuxsrcprc.h"
#include "mp4dmuxfltprc.h"
#include "mp4dmux.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.mp4_demuxer"
#endif

/**
 *@defgroup tizmp4demux 'tizmp4demux' : OpenMAX IL MP4 Demuxer
 *
 * - Component name : "OMX.Aratelia.container_demuxer.mp4"
 * - Implements role: "container_demuxer.source.mp4"
 * - Implements role: "container_demuxer.filter.mp4"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE mp4_demuxer_version = {{1, 0, 0, 0}};

static OMX_PTR
instantiate_filter_mp4_input_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_CODINGTYPE encodings[]
    = {OMX_AUDIO_CodingUnused, OMX_AUDIO_CodingMax};
  tiz_port_options_t port_opts = {
    OMX_PortDomainOther,
    OMX_DirInput,
    ARATELIA_MP4_DEMUXER_MP4_PORT_MIN_BUF_COUNT,
    ARATELIA_MP4_DEMUXER_MP4_PORT_MIN_BUF_SIZE,
    ARATELIA_MP4_DEMUXER_MP4_PORT_NONCONTIGUOUS,
    ARATELIA_MP4_DEMUXER_MP4_PORT_ALIGNMENT,
    ARATELIA_MP4_DEMUXER_MP4_PORT_SUPPLIERPREF,
    {ARATELIA_MP4_DEMUXER_FILTER_PORT_0_INDEX, NULL, NULL, NULL},
    -1 /* use -1 for now */
  };

  return factory_new (tiz_get_type (ap_hdl, "tizmp4port"), &port_opts,
                      &encodings);
}

static OMX_PTR
instantiate_audio_output_port (OMX_HANDLETYPE ap_hdl, const OMX_U32 port_id)
{
  OMX_AUDIO_CODINGTYPE encodings[]
    = {OMX_AUDIO_CodingUnused, OMX_AUDIO_CodingAutoDetect, OMX_AUDIO_CodingOPUS,
       OMX_AUDIO_CodingVORBIS, OMX_AUDIO_CodingMax};
  tiz_port_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirOutput,
    ARATELIA_MP4_DEMUXER_AUDIO_PORT_MIN_BUF_COUNT,
    ARATELIA_MP4_DEMUXER_AUDIO_PORT_MIN_BUF_SIZE,
    ARATELIA_MP4_DEMUXER_AUDIO_PORT_NONCONTIGUOUS,
    ARATELIA_MP4_DEMUXER_AUDIO_PORT_ALIGNMENT,
    ARATELIA_MP4_DEMUXER_AUDIO_PORT_SUPPLIERPREF,
    {port_id, NULL, NULL, NULL},
    -1 /* use -1 for now */
  };

  return factory_new (tiz_get_type (ap_hdl, "tizdemuxerport"), &port_opts,
                      &encodings);
}

static OMX_PTR
instantiate_source_audio_output_port (OMX_HANDLETYPE ap_hdl)
{
  return instantiate_audio_output_port (
    ap_hdl, ARATELIA_MP4_DEMUXER_SOURCE_PORT_0_INDEX);
}

static OMX_PTR
instantiate_filter_audio_output_port (OMX_HANDLETYPE ap_hdl)
{
  return instantiate_audio_output_port (
    ap_hdl, ARATELIA_MP4_DEMUXER_FILTER_PORT_1_INDEX);
}

static OMX_PTR
instantiate_video_output_port (OMX_HANDLETYPE ap_hdl, const OMX_U32 port_id)
{
  OMX_VIDEO_PORTDEFINITIONTYPE portdef;
  OMX_VIDEO_CODINGTYPE encodings[]
    = {OMX_VIDEO_CodingUnused, OMX_VIDEO_CodingAutoDetect, OMX_VIDEO_CodingVP8,
       OMX_VIDEO_CodingVP9, OMX_VIDEO_CodingMax};
  OMX_COLOR_FORMATTYPE formats[]
    = {OMX_COLOR_FormatUnused, OMX_COLOR_FormatMax};
  tiz_port_options_t port_opts = {
    OMX_PortDomainVideo,
    OMX_DirOutput,
    ARATELIA_MP4_DEMUXER_VIDEO_PORT_MIN_BUF_COUNT,
    ARATELIA_MP4_DEMUXER_VIDEO_PORT_MIN_BUF_SIZE,
    ARATELIA_MP4_DEMUXER_VIDEO_PORT_NONCONTIGUOUS,
    ARATELIA_MP4_DEMUXER_VIDEO_PORT_ALIGNMENT,
    ARATELIA_MP4_DEMUXER_VIDEO_PORT_SUPPLIERPREF,
    {port_id, NULL, NULL, NULL},
    -1 /* use -1 for now */
  };

  /* The demuxer port expects to receive raw the same structures as in a normal
     raw video port when instantiated as a video domain port */

  /* NOTE: No defaults are defined in the standard for the video
   * output port of the video_reader.demuxer component. So for the
   * sake of completeness, simply provide some default values
   * here. */

  portdef.pNativeRender = NULL;
  portdef.nFrameWidth = 176;
  portdef.nFrameHeight = 144;
  portdef.nStride = 0;
  portdef.nSliceHeight = 0;
  portdef.nBitrate = 0;
  portdef.xFramerate = 15 << 16;
  portdef.bFlagErrorConcealment = OMX_FALSE;
  portdef.eCompressionFormat = OMX_VIDEO_CodingUnused;
  portdef.eColorFormat = OMX_COLOR_FormatYUV420Planar;
  portdef.pNativeWindow = NULL;

  return factory_new (tiz_get_type (ap_hdl, "tizdemuxerport"), &port_opts,
                      &portdef, &encodings, &formats);
}

static OMX_PTR
instantiate_source_video_output_port (OMX_HANDLETYPE ap_hdl)
{
  return instantiate_video_output_port (
    ap_hdl, ARATELIA_MP4_DEMUXER_SOURCE_PORT_1_INDEX);
}

static OMX_PTR
instantiate_filter_video_output_port (OMX_HANDLETYPE ap_hdl)
{
  return instantiate_video_output_port (
    ap_hdl, ARATELIA_MP4_DEMUXER_FILTER_PORT_2_INDEX);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizdemuxercfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_MP4_DEMUXER_COMPONENT_NAME,
                      mp4_demuxer_version);
}

static OMX_PTR
instantiate_source_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "mp4dmuxsrcprc"));
}

static OMX_PTR
instantiate_filter_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "mp4dmuxfltprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t source_role_factory;
  tiz_role_factory_t filter_role_factory;
  const tiz_role_factory_t * rf_list[]
    = {&source_role_factory, &filter_role_factory};

  tiz_type_factory_t mp4dmuxsrcprc_type;
  tiz_type_factory_t mp4dmuxfltprc_type;
  const tiz_type_factory_t * tf_list[]
    = {&mp4dmuxsrcprc_type, &mp4dmuxfltprc_type};

  strcpy ((OMX_STRING) source_role_factory.role,
          ARATELIA_MP4_DEMUXER_SOURCE_ROLE);
  source_role_factory.pf_cport = instantiate_config_port;
  source_role_factory.pf_port[0] = instantiate_source_audio_output_port;
  source_role_factory.pf_port[1] = instantiate_source_video_output_port;
  source_role_factory.nports = 2;
  source_role_factory.pf_proc = instantiate_source_processor;

  strcpy ((OMX_STRING) filter_role_factory.role,
          ARATELIA_MP4_DEMUXER_FILTER_ROLE);
  filter_role_factory.pf_cport = instantiate_config_port;
  filter_role_factory.pf_port[0] = instantiate_filter_mp4_input_port;
  filter_role_factory.pf_port[1] = instantiate_filter_audio_output_port;
  filter_role_factory.pf_port[2] = instantiate_filter_video_output_port;
  filter_role_factory.nports = 3;
  filter_role_factory.pf_proc = instantiate_filter_processor;

  strcpy ((OMX_STRING) mp4dmuxsrcprc_type.class_name, "mp4dmuxsrcprc_class");
  mp4dmuxsrcprc_type.pf_class_init = mp4dmuxsrc_prc_class_init;
  strcpy ((OMX_STRING) mp4dmuxsrcprc_type.object_name, "mp4dmuxsrcprc");
  mp4dmuxsrcprc_type.pf_object_init = mp4dmuxsrc_prc_init;

  strcpy ((OMX_STRING) mp4dmuxfltprc_type.class_name, "mp4dmuxfltprc_class");
  mp4dmuxfltprc_type.pf_class_init = mp4dmuxflt_prc_class_init;
  strcpy ((OMX_STRING) mp4dmuxfltprc_type.object_name, "mp4dmuxfltprc");
  mp4dmuxfltprc_type.pf_object_init = mp4dmuxflt_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx (tiz_comp_init (ap_hdl, ARATELIA_MP4_DEMUXER_COMPONENT_NAME));

  /* Register the various classes */
  tiz_check_omx (tiz_comp_register_types (ap_hdl, tf_list, 2));

  /* Register the component roles */
  tiz_check_omx (tiz_comp_register_roles (ap_hdl, rf_list, 2));

  return OMX_ErrorNone;
}
