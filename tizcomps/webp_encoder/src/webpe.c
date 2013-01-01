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
 * @file   webpe.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Tizonia OpenMAX IL - WebP Encoder component
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <assert.h>
#include <string.h>

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Types.h"

#include "tizosal.h"
#include "tizscheduler.h"
#include "tizimageport.h"
#include "tizvideoport.h"
#include "tizconfigport.h"
#include "webpeprc.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.webp_encoder"
#endif

#define ARATELIA_WEBP_ENCODER_DEFAULT_ROLE "image_encoder.webp"
#define ARATELIA_WEBP_ENCODER_COMPONENT_NAME "OMX.Aratelia.image_encoder.webp"
#define ARATELIA_WEBP_ENCODER_PORT_MIN_BUF_COUNT 2
#define ARATELIA_WEBP_ENCODER_PORT_MIN_INPUT_BUF_SIZE 8192
#define ARATELIA_WEBP_ENCODER_PORT_MIN_OUTPUT_BUF_SIZE 8192
#define ARATELIA_WEBP_ENCODER_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_WEBP_ENCODER_PORT_ALIGNMENT 0
#define ARATELIA_WEBP_ENCODER_PORT_SUPPLIERPREF OMX_BufferSupplyInput

static OMX_VERSIONTYPE webp_encoder_version = { {1, 0, 0, 0 } };

static OMX_PTR
instantiate_input_port(OMX_HANDLETYPE ap_hdl)
{
  OMX_PTR p_videoport = NULL;
  OMX_VIDEO_PORTDEFINITIONTYPE portdef;
  OMX_VIDEO_CODINGTYPE encodings[] = {
    OMX_VIDEO_CodingUnused,
    OMX_VIDEO_CodingMax
  };
  OMX_COLOR_FORMATTYPE formats[] = {
    OMX_COLOR_FormatYUV420Planar,
    OMX_COLOR_FormatMax
  };
  tizport_options_t video_port_opts = {
    OMX_PortDomainVideo,
    OMX_DirInput,
    ARATELIA_WEBP_ENCODER_PORT_MIN_BUF_COUNT,
    ARATELIA_WEBP_ENCODER_PORT_MIN_OUTPUT_BUF_SIZE,
    ARATELIA_WEBP_ENCODER_PORT_NONCONTIGUOUS,
    ARATELIA_WEBP_ENCODER_PORT_ALIGNMENT,
    ARATELIA_WEBP_ENCODER_PORT_SUPPLIERPREF,
    {NULL, NULL, NULL},
    -1                          /* use -1 until this becomes a real
                                   component */
  };

  /* This figures are based on the defaults defined in the standard for the WebP
     decoder component */
  portdef.pNativeRender = NULL;
  portdef.nFrameWidth = 640;
  portdef.nFrameHeight = 480;
  portdef.nStride = 0;
  portdef.nSliceHeight = 0;
  portdef.nBitrate = 0;
  portdef.xFramerate = 0;
  portdef.bFlagErrorConcealment = OMX_FALSE;
  portdef.eCompressionFormat = OMX_VIDEO_CodingUnused;
  portdef.eColorFormat = OMX_COLOR_FormatYUV420Planar;
  portdef.pNativeWindow = NULL;

  init_tizvideoport ();
  p_videoport = factory_new (tizvideoport, &video_port_opts, &portdef,
                             &encodings, &formats);
  assert (p_videoport);

  return p_videoport;
}

static OMX_PTR
instantiate_output_port(OMX_HANDLETYPE ap_hdl)
{
  OMX_PTR p_imageport = NULL;
  OMX_IMAGE_PORTDEFINITIONTYPE portdef;
  OMX_IMAGE_CODINGTYPE encodings[] = {
    OMX_IMAGE_CodingWEBP,
    OMX_IMAGE_CodingMax
  };
  OMX_COLOR_FORMATTYPE formats[] = {
    OMX_COLOR_FormatUnused,
    OMX_COLOR_FormatMax
  };
  tizport_options_t image_port_opts = {
    OMX_PortDomainImage,
    OMX_DirOutput,
    ARATELIA_WEBP_ENCODER_PORT_MIN_BUF_COUNT,
    ARATELIA_WEBP_ENCODER_PORT_MIN_INPUT_BUF_SIZE,
    ARATELIA_WEBP_ENCODER_PORT_NONCONTIGUOUS,
    ARATELIA_WEBP_ENCODER_PORT_ALIGNMENT,
    ARATELIA_WEBP_ENCODER_PORT_SUPPLIERPREF,
    {NULL, NULL, NULL},
    -1                          /* use -1 until this becomes a real
                                   component */
  };

  portdef.pNativeRender = NULL;
  portdef.nFrameWidth = 640;
  portdef.nFrameHeight = 480;
  portdef.nStride = 0;
  portdef.nSliceHeight = 0;
  portdef.bFlagErrorConcealment = OMX_FALSE;
  portdef.eCompressionFormat = OMX_IMAGE_CodingWEBP;
  portdef.eColorFormat = OMX_COLOR_FormatUnused;
  portdef.pNativeWindow = NULL;

  init_tizimageport ();
  p_imageport = factory_new (tizimageport, &image_port_opts, &portdef,
                             &encodings, &formats);
  assert (p_imageport);

  return p_imageport;
}

static OMX_PTR
instantiate_config_port(OMX_HANDLETYPE ap_hdl)
{
  OMX_PTR p_cport = NULL;

  init_tizconfigport ();
  p_cport = factory_new (tizconfigport, NULL,   /* this port does not take options */
                         ARATELIA_WEBP_ENCODER_COMPONENT_NAME,
                         webp_encoder_version);
  assert (p_cport);

  return p_cport;
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  OMX_PTR p_proc = NULL;

  /* Instantiate the processor */
  init_webpeprc ();
  p_proc = factory_new (webpeprc, ap_hdl);
  assert (p_proc);

  return p_proc;
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t *rf_list [] = {&role_factory};

  assert (ap_hdl);

  TIZ_LOG (TIZ_LOG_TRACE, "OMX_ComponentInit: "
             "Inititializing [%s]", ARATELIA_WEBP_ENCODER_COMPONENT_NAME);

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_WEBP_ENCODER_DEFAULT_ROLE);
  role_factory.pf_cport   = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_input_port;
  role_factory.pf_port[1] = instantiate_output_port;
  role_factory.nports     = 2;
  role_factory.pf_proc    = instantiate_processor;

  tiz_init_component (ap_hdl, ARATELIA_WEBP_ENCODER_COMPONENT_NAME);

  tiz_register_roles (ap_hdl, rf_list, 1);

  return OMX_ErrorNone;
}
