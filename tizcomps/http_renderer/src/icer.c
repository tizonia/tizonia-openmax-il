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
 * @file   icer.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Icecast-like Http Sink component
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
#include "tizbinaryport.h"
#include "icercfgport.h"

#include "icerprc.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer"
#endif

#define ARATELIA_HTTP_RENDERER_DEFAULT_ROLE "ice_renderer.http"
#define ARATELIA_HTTP_RENDERER_COMPONENT_NAME "OMX.Aratelia.ice_renderer.http"
#define ARATELIA_HTTP_RENDERER_PORT_MIN_BUF_COUNT 2
#define ARATELIA_HTTP_RENDERER_PORT_MIN_BUF_SIZE (8*1024)
#define ARATELIA_HTTP_RENDERER_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_HTTP_RENDERER_PORT_ALIGNMENT 0
#define ARATELIA_HTTP_RENDERER_PORT_SUPPLIERPREF OMX_BufferSupplyInput

static OMX_VERSIONTYPE http_renderer_version = { {1, 0, 0, 0} };

static OMX_PTR
instantiate_binary_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_PTR p_binaryport = NULL;
  tiz_port_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirInput,
    ARATELIA_HTTP_RENDERER_PORT_MIN_BUF_COUNT,
    ARATELIA_HTTP_RENDERER_PORT_MIN_BUF_SIZE,
    ARATELIA_HTTP_RENDERER_PORT_NONCONTIGUOUS,
    ARATELIA_HTTP_RENDERER_PORT_ALIGNMENT,
    ARATELIA_HTTP_RENDERER_PORT_SUPPLIERPREF,
    {NULL, NULL, NULL},
    -1                          /* use -1 for now */
  };

  init_tizbinaryport ();
  p_binaryport = factory_new (tizbinaryport, &port_opts);
  assert (p_binaryport);

  return p_binaryport;
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_PTR p_cport = NULL;

  /* Instantiate the config port */
  init_icercfgport ();
  p_cport = factory_new (icercfgport, NULL,     /* this port does not take options */
                         ARATELIA_HTTP_RENDERER_COMPONENT_NAME,
                         http_renderer_version);
  assert (p_cport);

  return p_cport;
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  OMX_PTR p_proc = NULL;

  /* Instantiate the processor */
  init_icerprc ();
  p_proc = factory_new (icerprc, ap_hdl);
  assert (p_proc);

  return p_proc;
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t *rf_list[] = { &role_factory };

  assert (ap_hdl);

  TIZ_LOG (TIZ_TRACE, "OMX_ComponentInit: Inititializing [%s]",
           ARATELIA_HTTP_RENDERER_COMPONENT_NAME);

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_HTTP_RENDERER_DEFAULT_ROLE);
  role_factory.pf_cport = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_binary_port;
  role_factory.nports = 1;
  role_factory.pf_proc = instantiate_processor;

  tiz_init_component (ap_hdl, ARATELIA_HTTP_RENDERER_COMPONENT_NAME);

  tiz_register_roles (ap_hdl, rf_list, 1);

  return OMX_ErrorNone;
}
