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
 * @file   webmdmux.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - WebM Demuxer component
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

#include "webmdmuxsrcprc.h"
#include "webmdmuxfltprc.h"
#include "webmdmux.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.webm_demuxer"
#endif

/**
 *@defgroup tizwebmdemux 'tizwebmdemux' : OpenMAX IL WebM Demuxer
 *
 * - Component name : "OMX.Aratelia.container_demuxer.webm"
 * - Implements role: "container_demuxer.source.webm"
 * - Implements role: "container_demuxer.filter.webm"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE webm_demuxer_version = { {1, 0, 0, 0} };

static OMX_PTR
instantiate_source_binary_port (OMX_HANDLETYPE ap_hdl)
{
  tiz_port_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirOutput,
    ARATELIA_WEBM_DEMUXER_SOURCE_PORT_0_MIN_BUF_COUNT,
    ARATELIA_WEBM_DEMUXER_SOURCE_PORT_0_MIN_BUF_SIZE,
    ARATELIA_WEBM_DEMUXER_SOURCE_PORT_0_NONCONTIGUOUS,
    ARATELIA_WEBM_DEMUXER_SOURCE_PORT_0_ALIGNMENT,
    ARATELIA_WEBM_DEMUXER_SOURCE_PORT_0_SUPPLIERPREF,
    {ARATELIA_WEBM_DEMUXER_SOURCE_PORT_0_INDEX, NULL, NULL, NULL},
    -1                          /* slave port's index, use -1 for now */
  };

  return factory_new (tiz_get_type (ap_hdl, "tizbinaryport"), &port_opts);
}

static OMX_PTR
instantiate_source_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizconfigport"),
                      NULL,       /* this port does not take options */
                      ARATELIA_WEBM_DEMUXER_COMPONENT_NAME,
                      webm_demuxer_version);
}

static OMX_PTR
instantiate_source_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "webmdmuxsrcprc"));
}

static OMX_PTR
instantiate_filter_binary_port (OMX_HANDLETYPE ap_hdl)
{
  tiz_port_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirOutput,
    ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_MIN_BUF_COUNT,
    ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_MIN_BUF_SIZE,
    ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_NONCONTIGUOUS,
    ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_ALIGNMENT,
    ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_SUPPLIERPREF,
    {ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX, NULL, NULL, NULL},
    -1                          /* slave port's index, use -1 for now */
  };

  return factory_new (tiz_get_type (ap_hdl, "tizbinaryport"), &port_opts);
}

static OMX_PTR
instantiate_filter_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizconfigport"),
                      NULL,       /* this port does not take options */
                      ARATELIA_WEBM_DEMUXER_COMPONENT_NAME,
                      webm_demuxer_version);
}

static OMX_PTR
instantiate_filter_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "webmdmuxfltprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t source_role_factory;
  tiz_role_factory_t filter_role_factory;
  const tiz_role_factory_t *rf_list[] = { &source_role_factory, &filter_role_factory };

  tiz_type_factory_t webmdmuxsrcprc_type;
  tiz_type_factory_t webmdmuxfltprc_type;
  const tiz_type_factory_t *tf_list[] = { &webmdmuxsrcprc_type, &webmdmuxfltprc_type };

  strcpy ((OMX_STRING) source_role_factory.role, ARATELIA_WEBM_DEMUXER_SOURCE_ROLE);
  source_role_factory.pf_cport   = instantiate_source_config_port;
  source_role_factory.pf_port[0] = instantiate_source_binary_port;
  source_role_factory.nports     = 1;
  source_role_factory.pf_proc    = instantiate_source_processor;

  strcpy ((OMX_STRING) filter_role_factory.role, ARATELIA_WEBM_DEMUXER_FILTER_ROLE);
  filter_role_factory.pf_cport   = instantiate_filter_config_port;
  filter_role_factory.pf_port[0] = instantiate_filter_binary_port;
  filter_role_factory.nports     = 1;
  filter_role_factory.pf_proc    = instantiate_filter_processor;

  strcpy ((OMX_STRING) webmdmuxsrcprc_type.class_name, "webmdmuxsrcprc_class");
  webmdmuxsrcprc_type.pf_class_init = webmdmuxsrc_prc_class_init;
  strcpy ((OMX_STRING) webmdmuxsrcprc_type.object_name, "webmdmuxsrcprc");
  webmdmuxsrcprc_type.pf_object_init = webmdmuxsrc_prc_init;

  strcpy ((OMX_STRING) webmdmuxfltprc_type.class_name, "webmdmuxfltprc_class");
  webmdmuxfltprc_type.pf_class_init = webmdmuxflt_prc_class_init;
  strcpy ((OMX_STRING) webmdmuxfltprc_type.object_name, "webmdmuxfltprc");
  webmdmuxfltprc_type.pf_object_init = webmdmuxflt_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx_err (tiz_comp_init (ap_hdl, ARATELIA_WEBM_DEMUXER_COMPONENT_NAME));

  /* Register the various classes */
  tiz_check_omx_err (tiz_comp_register_types (ap_hdl, tf_list, 2));

  /* Register the component roles */
  tiz_check_omx_err (tiz_comp_register_roles (ap_hdl, rf_list, 2));

  return OMX_ErrorNone;
}
