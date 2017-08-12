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
 * @file   vp8d.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - VP8 Decoder component
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>
#include <tizport.h>
#include <tizport_decls.h>
#include <tizvideoport.h>
#include <tizvideoport_decls.h>

#include "vp8dinport.h"
#include "vp8dprc.h"
#include "vp8d.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.vp8_decoder"
#endif

/**
 *@defgroup libtizvp8dec 'libtizvp8dec' : OpenMAX IL VP8 decoder
 *
 * - Component name : "OMX.Aratelia.video_decoder.vp8"
 * - Implements role: "video_decoder.vp8"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE vp8_decoder_version = {{1, 0, 0, 0}};

static OMX_BOOL
egl_image_validation_hook (const OMX_HANDLETYPE ap_hdl,
                           OMX_U32 pid, OMX_PTR ap_eglimage,
                           void *ap_args)
{
  const void * p_krn = NULL;
  const tiz_port_t * p_port = NULL;
  const tiz_videoport_t * p_videoport = NULL;

  TIZ_DEBUG (ap_hdl, "vp8 decoder EGLImage validation hook : ap_eglimage=[%p]",
             ap_eglimage);

  /* TODO: */
  // vp8d_prc_t * ap_prc = NULL;

  assert (ap_hdl);
  assert (ap_eglimage);
  assert (!ap_args);

  p_krn = tiz_get_krn (ap_hdl);
  p_port = tiz_krn_get_port (p_krn, pid);
  p_videoport = (tiz_videoport_t *) p_port;

  assert (p_videoport);

/*   { */
/*     const OMX_VIDEO_PORTDEFINITIONTYPE * p_video_portdef */
/*       = &(p_port->portdef_.format.video); */

/*     if (!p_video_portdef->pNativeWindow) */
/*       { */
/*         return OMX_FALSE; */
/*       } */
/*   } */

  /* This function must return true or false */
  return OMX_TRUE;
}

static OMX_PTR
instantiate_input_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_VIDEO_PORTDEFINITIONTYPE portdef;
  OMX_VIDEO_PARAM_VP8TYPE vp8type;
  OMX_VIDEO_CODINGTYPE encodings[] = {OMX_VIDEO_CodingVP8, OMX_VIDEO_CodingMax};
  OMX_COLOR_FORMATTYPE formats[] = {
    OMX_COLOR_FormatUnused, OMX_COLOR_FormatYUV420Planar, OMX_COLOR_FormatMax};
  tiz_port_options_t vp8_port_opts = {
    OMX_PortDomainVideo,
    OMX_DirInput,
    ARATELIA_VP8_DECODER_PORT_MIN_BUF_COUNT,
    ARATELIA_VP8_DECODER_PORT_MIN_INPUT_BUF_SIZE,
    ARATELIA_VP8_DECODER_PORT_NONCONTIGUOUS,
    ARATELIA_VP8_DECODER_PORT_ALIGNMENT,
    ARATELIA_VP8_DECODER_PORT_SUPPLIERPREF,
    {ARATELIA_VP8_DECODER_INPUT_PORT_INDEX, NULL, NULL, NULL},
    1 /* slave port */
  };
  OMX_VIDEO_VP8LEVELTYPE levels[]
    = {OMX_VIDEO_VP8Level_Version0, OMX_VIDEO_VP8Level_Version1,
       OMX_VIDEO_VP8Level_Version2, OMX_VIDEO_VP8Level_Version3,
       OMX_VIDEO_VP8LevelMax};

  /* This figures are based on the defaults defined in the standard for the VP8
   * decoder component */
  portdef.pNativeRender = NULL;
  portdef.nFrameWidth = 176;
  portdef.nFrameHeight = 144;
  portdef.nStride = 0;
  portdef.nSliceHeight = 0;
  portdef.nBitrate = 64000;
  portdef.xFramerate = 15 << 16;
  portdef.bFlagErrorConcealment = OMX_FALSE;
  portdef.eCompressionFormat = OMX_VIDEO_CodingVP8;
  portdef.eColorFormat = OMX_COLOR_FormatUnused;
  portdef.pNativeWindow = NULL;

  vp8type.nSize = sizeof (OMX_VIDEO_PARAM_VP8TYPE);
  vp8type.nVersion.nVersion = OMX_VERSION;
  vp8type.nPortIndex = ARATELIA_VP8_DECODER_INPUT_PORT_INDEX;
  vp8type.eProfile = OMX_VIDEO_VP8ProfileMain;
  vp8type.eLevel = OMX_VIDEO_VP8Level_Version0;
  vp8type.nDCTPartitions = 0; /* 1 DCP partitiion */
  vp8type.bErrorResilientMode = OMX_FALSE;

  return factory_new (tiz_get_type (ap_hdl, "vp8dinport"), &vp8_port_opts,
                      &portdef, &encodings, &formats, &vp8type, &levels,
                      NULL /* OMX_VIDEO_PARAM_BITRATETYPE */);
}

static OMX_PTR
instantiate_output_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_VIDEO_PORTDEFINITIONTYPE portdef;
  OMX_VIDEO_CODINGTYPE encodings[]
    = {OMX_VIDEO_CodingUnused, OMX_VIDEO_CodingMax};
  OMX_COLOR_FORMATTYPE formats[]
    = {OMX_COLOR_FormatYUV420Planar, OMX_COLOR_FormatMax};
  tiz_port_options_t rawvideo_port_opts = {
    OMX_PortDomainVideo,
    OMX_DirOutput,
    ARATELIA_VP8_DECODER_PORT_MIN_BUF_COUNT,
    ARATELIA_VP8_DECODER_PORT_MIN_OUTPUT_BUF_SIZE,
    ARATELIA_VP8_DECODER_PORT_NONCONTIGUOUS,
    ARATELIA_VP8_DECODER_PORT_ALIGNMENT,
    ARATELIA_VP8_DECODER_PORT_SUPPLIERPREF,
    {ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX, NULL, NULL, NULL},
    0 /* Master port */
  };

  /* This figures are based on the defaults defined in the standard for the VP8
   * decoder component */
  portdef.pNativeRender = NULL;
  portdef.nFrameWidth = 176;
  portdef.nFrameHeight = 144;
  portdef.nStride = 0;
  portdef.nSliceHeight = 0;
  portdef.nBitrate = 64000;
  portdef.xFramerate = 15 << 16;
  portdef.bFlagErrorConcealment = OMX_FALSE;
  portdef.eCompressionFormat = OMX_VIDEO_CodingUnused;
  portdef.eColorFormat = OMX_COLOR_FormatYUV420Planar;
  portdef.pNativeWindow = NULL;

  return factory_new (tiz_get_type (ap_hdl, "tizvideoport"),
                      &rawvideo_port_opts, &portdef, &encodings, &formats);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizconfigport"),
                      NULL, /* this port does not take options */
                      ARATELIA_VP8_DECODER_COMPONENT_NAME, vp8_decoder_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "vp8dprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t * rf_list[] = {&role_factory};
  tiz_type_factory_t vp8d_inport_type;
  tiz_type_factory_t vp8dprc_type;
  const tiz_type_factory_t * tf_list[] = {&vp8d_inport_type,
                                          &vp8dprc_type};
  const tiz_eglimage_hook_t egl_validation_hook = {
    ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX,
    egl_image_validation_hook,
    NULL
  };

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_VP8_DECODER_DEFAULT_ROLE);
  role_factory.pf_cport = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_input_port;
  role_factory.pf_port[1] = instantiate_output_port;
  role_factory.nports = 2;
  role_factory.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING) vp8d_inport_type.class_name, "vp8dinport_class");
  vp8d_inport_type.pf_class_init = vp8d_inport_class_init;
  strcpy ((OMX_STRING) vp8d_inport_type.object_name, "vp8dinport");
  vp8d_inport_type.pf_object_init = vp8d_inport_init;

  strcpy ((OMX_STRING) vp8dprc_type.class_name, "vp8dprc_class");
  vp8dprc_type.pf_class_init = vp8d_prc_class_init;
  strcpy ((OMX_STRING) vp8dprc_type.object_name, "vp8dprc");
  vp8dprc_type.pf_object_init = vp8d_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx (tiz_comp_init (ap_hdl, ARATELIA_VP8_DECODER_COMPONENT_NAME));

  /* Register the "vp8dprc" class */
  tiz_check_omx (tiz_comp_register_types (ap_hdl, tf_list, 2));

  /* Register the component role */
  tiz_check_omx (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  /* Register the egl image validation hook for the default role */
  tiz_check_omx (tiz_comp_register_role_eglimage_hook (
    ap_hdl, (const OMX_U8 *) ARATELIA_VP8_DECODER_DEFAULT_ROLE,
    &egl_validation_hook));

  return OMX_ErrorNone;
}
