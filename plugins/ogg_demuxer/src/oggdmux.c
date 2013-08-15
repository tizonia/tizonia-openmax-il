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
 * @file   oggdmux.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - File Reader component
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "oggdmuxprc.h"
#include "tizosal.h"
#include "tizscheduler.h"
#include "tizconfigport.h"
#include "tizbinaryport.h"

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Types.h"

#include <assert.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.ogg_demuxer"
#endif

#define ARATELIA_OGG_DEMUXER_DEFAULT_ROLE                   "container_demuxer.ogg"
#define ARATELIA_OGG_DEMUXER_COMPONENT_NAME                 "OMX.Aratelia.container_demuxer.ogg"
/* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX               0
#define ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX               1
#define ARATELIA_OGG_DEMUXER_PORT_MIN_BUF_COUNT             2
#define ARATELIA_OGG_DEMUXER_PORT_MIN_AUDIO_OUTPUT_BUF_SIZE 8192
#define ARATELIA_OGG_DEMUXER_PORT_MIN_VIDEO_OUTPUT_BUF_SIZE 8192
#define ARATELIA_OGG_DEMUXER_PORT_NONCONTIGUOUS             OMX_FALSE
#define ARATELIA_OGG_DEMUXER_PORT_ALIGNMENT                 0
#define ARATELIA_OGG_DEMUXER_PORT_SUPPLIERPREF              OMX_BufferSupplyInput

static OMX_VERSIONTYPE ogg_demuxer_version = { {1, 0, 0, 0} };

static OMX_PTR
instantiate_audio_output_port (OMX_HANDLETYPE ap_hdl)
{
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

  tiz_check_omx_err_ret_null (tiz_binaryport_init ());
  return factory_new (tizbinaryport, ap_hdl, &port_opts);
}

static OMX_PTR
instantiate_video_output_port (OMX_HANDLETYPE ap_hdl)
{
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

  tiz_check_omx_err_ret_null (tiz_binaryport_init ());
  return factory_new (tizbinaryport, ap_hdl, &port_opts);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  tiz_check_omx_err_ret_null (tiz_configport_init ());
  return factory_new (tizconfigport, ap_hdl, NULL,   /* this port does not take options */
                      ARATELIA_OGG_DEMUXER_COMPONENT_NAME,
                      ogg_demuxer_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  tiz_check_omx_err_ret_null (oggdmux_prc_init ());
  return factory_new (oggdmuxprc, ap_hdl);
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t *rf_list[] = { &role_factory };

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_OGG_DEMUXER_DEFAULT_ROLE);
  role_factory.pf_cport   = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_audio_output_port;
  role_factory.pf_port[1] = instantiate_video_output_port;
  /* TODO : Add clock output port */
  role_factory.nports     = 2;
  role_factory.pf_proc    = instantiate_processor;

  TIZ_LOG (TIZ_TRACE, "OMX_ComponentInit: "
           "Inititializing [%s]", ARATELIA_OGG_DEMUXER_COMPONENT_NAME);

  tiz_check_omx_err (tiz_comp_init (ap_hdl, ARATELIA_OGG_DEMUXER_COMPONENT_NAME));
  tiz_check_omx_err (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
