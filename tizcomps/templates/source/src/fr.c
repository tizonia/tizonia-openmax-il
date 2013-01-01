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
 * @file   fr.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - File Reader component
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
#include "tizconfigport.h"
#include "frprc.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.file_reader"
#endif

#define ARATELIA_FILE_READER_DEFAULT_ROLE "file_reader.binary"
#define ARATELIA_FILE_READER_COMPONENT_NAME "OMX.Aratelia.file_reader.binary"
#define ARATELIA_FILE_READER_PORT_MIN_BUF_COUNT 2
#define ARATELIA_FILE_READER_PORT_MIN_BUF_SIZE 2048
#define ARATELIA_FILE_READER_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_FILE_READER_PORT_ALIGNMENT 0
#define ARATELIA_FILE_READER_PORT_SUPPLIERPREF OMX_BufferSupplyInput

static OMX_VERSIONTYPE file_reader_version = { { 1, 0, 0, 0 } };

static OMX_PTR
instantiate_binary_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_PTR p_binaryport = NULL;
  tizport_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirOutput,
    ARATELIA_FILE_READER_PORT_MIN_BUF_COUNT,
    ARATELIA_FILE_READER_PORT_MIN_BUF_SIZE,
    ARATELIA_FILE_READER_PORT_NONCONTIGUOUS,
    ARATELIA_FILE_READER_PORT_ALIGNMENT,
    ARATELIA_FILE_READER_PORT_SUPPLIERPREF,
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
  init_frcfgport ();
  p_cport = factory_new (frcfgport, NULL,       /* this port does not take options */
                         ARATELIA_FILE_READER_COMPONENT_NAME,
                         file_reader_version);
  assert (p_cport);

  return p_cport;
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  OMX_PTR p_proc = NULL;

  /* Instantiate the processor */
  init_frprc ();
  p_proc = factory_new (frprc, ap_hdl);
  assert (p_proc);

  return p_proc;
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  tiz_role_factory_t *rf_list [] = {&role_factory};

  assert (ap_hdl);

  TIZ_LOG (TIZ_LOG_TRACE, "OMX_ComponentInit: Inititializing [%s]",
             ARATELIA_FILE_READER_COMPONENT_NAME);

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_FILE_READER_DEFAULT_ROLE);
  role_factory.pf_cport   = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_binary_port;
  role_factory.nports     = 1;
  role_factory.pf_proc    = instantiate_processor;

  tiz_init_component (ap_hdl, ARATELIA_FILE_READER_COMPONENT_NAME);

  tiz_register_roles (ap_hdl, rf_list, 1);

  return OMX_ErrorNone;
}
