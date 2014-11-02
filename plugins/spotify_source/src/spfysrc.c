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
 * @file   spfysrc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Spotify client component
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

#include "spfysrcprc.h"
#include "spfysrc.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.spotify_source"
#endif

static OMX_VERSIONTYPE spotify_source_version = { {1, 0, 0, 0} };

static OMX_PTR
instantiate_binary_port (OMX_HANDLETYPE ap_hdl)
{
  tiz_port_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirOutput,
    ARATELIA_SPOTIFY_SOURCE_PORT_MIN_BUF_COUNT,
    ARATELIA_SPOTIFY_SOURCE_PORT_MIN_BUF_SIZE,
    ARATELIA_SPOTIFY_SOURCE_PORT_NONCONTIGUOUS,
    ARATELIA_SPOTIFY_SOURCE_PORT_ALIGNMENT,
    ARATELIA_SPOTIFY_SOURCE_PORT_SUPPLIERPREF,
    {ARATELIA_SPOTIFY_SOURCE_PORT_INDEX, NULL, NULL, NULL},
    -1                          /* slave port's index, use -1 for now */
  };

  return factory_new (tiz_get_type (ap_hdl, "tizbinaryport"), &port_opts);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizconfigport"),
                      NULL,       /* this port does not take options */
                      ARATELIA_SPOTIFY_SOURCE_COMPONENT_NAME,
                      spotify_source_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "spfysrcprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t *rf_list[] = { &role_factory };
  tiz_type_factory_t spfysrcprc_type;
  const tiz_type_factory_t *tf_list[] = { &spfysrcprc_type };

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_SPOTIFY_SOURCE_DEFAULT_ROLE);
  role_factory.pf_cport   = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_binary_port;
  role_factory.nports     = 1;
  role_factory.pf_proc    = instantiate_processor;

  strcpy ((OMX_STRING) spfysrcprc_type.class_name, "spfysrcprc_class");
  spfysrcprc_type.pf_class_init = spfysrc_prc_class_init;
  strcpy ((OMX_STRING) spfysrcprc_type.object_name, "spfysrcprc");
  spfysrcprc_type.pf_object_init = spfysrc_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx_err (tiz_comp_init (ap_hdl, ARATELIA_SPOTIFY_SOURCE_COMPONENT_NAME));

  /* Register the "spfysrcprc" class */
  tiz_check_omx_err (tiz_comp_register_types (ap_hdl, tf_list, 1));

  /* Register the component role(s) */
  tiz_check_omx_err (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
