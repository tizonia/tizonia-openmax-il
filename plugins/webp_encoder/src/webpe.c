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

#include "webpeprc.h"

#include "tizscheduler.h"
#include "tizport.h"
#include "tizimageport.h"
#include "tizvideoport.h"
#include "tizconfigport.h"

#include "tizosal.h"

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Types.h"

#include <assert.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.webp_encoder"
#endif

#define ARATELIA_WEBP_ENCODER_DEFAULT_ROLE "image_encoder.webp"
#define ARATELIA_WEBP_ENCODER_COMPONENT_NAME "OMX.Aratelia.image_encoder.webp"

/* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_WEBP_ENCODER_INPUT_PORT_INDEX  0
#define ARATELIA_WEBP_ENCODER_OUTPUT_PORT_INDEX 1

#define ARATELIA_WEBP_ENCODER_PORT_MIN_BUF_COUNT       2
#define ARATELIA_WEBP_ENCODER_PORT_MIN_INPUT_BUF_SIZE  8192
#define ARATELIA_WEBP_ENCODER_PORT_MIN_OUTPUT_BUF_SIZE 8192
#define ARATELIA_WEBP_ENCODER_PORT_NONCONTIGUOUS       OMX_FALSE
#define ARATELIA_WEBP_ENCODER_PORT_ALIGNMENT           0
#define ARATELIA_WEBP_ENCODER_PORT_SUPPLIERPREF        OMX_BufferSupplyInput

static OMX_VERSIONTYPE webp_encoder_version = { {1, 0, 0, 0} };

static OMX_PTR
instantiate_input_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_VIDEO_PORTDEFINITIONTYPE portdef;
  OMX_VIDEO_CODINGTYPE encodings[] = {
    OMX_VIDEO_CodingUnused,
    OMX_VIDEO_CodingMax
  };
  OMX_COLOR_FORMATTYPE formats[] = {
    OMX_COLOR_FormatYUV420Planar,
    OMX_COLOR_FormatMax
  };
  tiz_port_options_t video_port_opts = {
    OMX_PortDomainVideo,
    OMX_DirInput,
    ARATELIA_WEBP_ENCODER_PORT_MIN_BUF_COUNT,
    ARATELIA_WEBP_ENCODER_PORT_MIN_OUTPUT_BUF_SIZE,
    ARATELIA_WEBP_ENCODER_PORT_NONCONTIGUOUS,
    ARATELIA_WEBP_ENCODER_PORT_ALIGNMENT,
    ARATELIA_WEBP_ENCODER_PORT_SUPPLIERPREF,
    {ARATELIA_WEBP_ENCODER_INPUT_PORT_INDEX, NULL, NULL, NULL},
    -1                          /* use -1 until this becomes a real
                                 * component */
  };

  /* This figures are based on the defaults defined in the standard for the WebP
   * decoder component */
  portdef.pNativeRender         = NULL;
  portdef.nFrameWidth           = 640;
  portdef.nFrameHeight          = 480;
  portdef.nStride               = 0;
  portdef.nSliceHeight          = 0;
  portdef.nBitrate              = 0;
  portdef.xFramerate            = 0;
  portdef.bFlagErrorConcealment = OMX_FALSE;
  portdef.eCompressionFormat    = OMX_VIDEO_CodingUnused;
  portdef.eColorFormat          = OMX_COLOR_FormatYUV420Planar;
  portdef.pNativeWindow         = NULL;

  return factory_new (tiz_get_type (ap_hdl, "tizvideoport"), ap_hdl,
                      &video_port_opts, &portdef,
                      &encodings, &formats);
}

static OMX_PTR
instantiate_output_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_IMAGE_PORTDEFINITIONTYPE portdef;
  OMX_IMAGE_CODINGTYPE encodings[] = {
    OMX_IMAGE_CodingWEBP,
    OMX_IMAGE_CodingMax
  };
  OMX_COLOR_FORMATTYPE formats[] = {
    OMX_COLOR_FormatUnused,
    OMX_COLOR_FormatMax
  };
  tiz_port_options_t image_port_opts = {
    OMX_PortDomainImage,
    OMX_DirOutput,
    ARATELIA_WEBP_ENCODER_PORT_MIN_BUF_COUNT,
    ARATELIA_WEBP_ENCODER_PORT_MIN_INPUT_BUF_SIZE,
    ARATELIA_WEBP_ENCODER_PORT_NONCONTIGUOUS,
    ARATELIA_WEBP_ENCODER_PORT_ALIGNMENT,
    ARATELIA_WEBP_ENCODER_PORT_SUPPLIERPREF,
    {ARATELIA_WEBP_ENCODER_OUTPUT_PORT_INDEX, NULL, NULL, NULL},
    -1                          /* use -1 until this becomes a real
                                 * component */
  };

  portdef.pNativeRender         = NULL;
  portdef.nFrameWidth           = 640;
  portdef.nFrameHeight          = 480;
  portdef.nStride               = 0;
  portdef.nSliceHeight          = 0;
  portdef.bFlagErrorConcealment = OMX_FALSE;
  portdef.eCompressionFormat    = OMX_IMAGE_CodingWEBP;
  portdef.eColorFormat          = OMX_COLOR_FormatUnused;
  portdef.pNativeWindow         = NULL;

  return factory_new (tiz_get_type (ap_hdl, "tizimageport"), ap_hdl,
                      &image_port_opts, &portdef,
                      &encodings, &formats);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizconfigport"), ap_hdl,
                      NULL,   /* this port does not take options */
                      ARATELIA_WEBP_ENCODER_COMPONENT_NAME,
                      webp_encoder_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "webpeprc"), ap_hdl);
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t *rf_list[] = { &role_factory };
  tiz_type_factory_t webpeprc_type;
  const tiz_type_factory_t *tf_list[] = { &webpeprc_type};

  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_ComponentInit: "
           "Inititializing [%s]", ARATELIA_WEBP_ENCODER_COMPONENT_NAME);

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_WEBP_ENCODER_DEFAULT_ROLE);
  role_factory.pf_cport   = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_input_port;
  role_factory.pf_port[1] = instantiate_output_port;
  role_factory.nports     = 2;
  role_factory.pf_proc    = instantiate_processor;

  strcpy ((OMX_STRING) webpeprc_type.class_name, "webpeprc_class");
  webpeprc_type.pf_class_init = webpe_prc_class_init;
  strcpy ((OMX_STRING) webpeprc_type.object_name, "webpeprc");
  webpeprc_type.pf_object_init = webpe_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx_err (tiz_comp_init (ap_hdl, ARATELIA_WEBP_ENCODER_COMPONENT_NAME));

  /* Register the "webpeprc" class */
  tiz_check_omx_err (tiz_comp_register_types (ap_hdl, tf_list, 1));

  /* Register the component role */
  tiz_check_omx_err (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
