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
 * @file   tiztc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Test IL Component
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
#include "tizpcmport.h"
#include "tizconfigport.h"
#include "tiztcproc.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.test_comp"
#endif

#define _DEFAULT_ROLE1 "tizonia_test_component.role1"
#define _DEFAULT_ROLE2 "tizonia_test_component.role2"
#define _COMPONENT_NAME "OMX.Aratelia.tizonia.test_component"
#define _PORT_MIN_BUF_COUNT 1
#define _PORT_MIN_BUF_SIZE 1024
#define _PORT_NONCONTIGUOUS OMX_FALSE
#define _PORT_ALIGNMENT 0
#define _PORT_SUPPLIERPREF OMX_BufferSupplyInput

#define RETURN_ON_IL_ERR(_a)                                    \
  do {                                                          \
    OMX_ERRORTYPE _err = _a;                                    \
    if (OMX_ErrorNone != _err) {                                \
      TIZ_LOG(TIZ_TRACE, "[%s] : [%s]...",                    \
              _COMPONENT_NAME, tiz_err_to_str (_err));          \
      return _err;                                              \
    }                                                           \
  } while(0)

static OMX_VERSIONTYPE tc_comp_version = { {1, 0, 0, 0} };

static OMX_U8 *
pcm_port_alloc_hook (OMX_U32 * ap_size,
                     OMX_PTR * app_port_priv, void *ap_args)
{
  OMX_U8 *p = NULL;
  assert (ap_size);
  p = tiz_mem_alloc (*ap_size * sizeof (OMX_U8));
  TIZ_LOG (TIZ_TRACE, "Test Component Alloc Hook :size[%u] p=[%p]",
           *ap_size, p);
  return p;
}

static void
pcm_port_free_hook (OMX_PTR ap_buf, OMX_PTR ap_port_priv, void *ap_args)
{
  TIZ_LOG (TIZ_TRACE, "Test Component Free Hook : ap_buf[%p]", ap_buf);
  assert (ap_buf);
  tiz_mem_free (ap_buf);
}


static OMX_PTR
instantiate_pcm_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_PTR p_pcmport = NULL;
  tiz_port_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirInput,
    _PORT_MIN_BUF_COUNT,
    _PORT_MIN_BUF_SIZE,
    _PORT_NONCONTIGUOUS,
    _PORT_ALIGNMENT,
    _PORT_SUPPLIERPREF,
    {pcm_port_alloc_hook, pcm_port_free_hook, NULL},
    -1
  };

  TIZ_LOG (TIZ_TRACE, "Inititializing the test component's pcm port");

  /* Instantiate a pcm port */
  init_tizpcmport ();
  p_pcmport = factory_new (tizpcmport, &port_opts, NULL, NULL, NULL, NULL);
  assert (p_pcmport);

  return p_pcmport;
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_PTR p_cport = NULL;

  TIZ_LOG (TIZ_TRACE, "Inititializing the test component's config port");

  /* Instantiate the config port */
  init_tizconfigport ();
  p_cport = factory_new (tizconfigport, NULL,   /* this port does not take options */
                         _COMPONENT_NAME, tc_comp_version);
  assert (p_cport);

  return p_cport;
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  OMX_PTR p_proc = NULL;

  TIZ_LOG (TIZ_TRACE, "Inititializing the test component's processor");

  /* Instantiate the processor */
  init_tiztcproc ();
  p_proc = factory_new (tiztcproc, ap_hdl);
  assert (p_proc);

  return p_proc;
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory1, role_factory2;
  const tiz_role_factory_t *rf_list[] = { &role_factory1, &role_factory2 };
  const tiz_alloc_hooks_t new_hooks =
    { pcm_port_alloc_hook, pcm_port_free_hook, NULL };
  tiz_alloc_hooks_t old_hooks = { NULL, NULL, NULL };

  strcpy ((OMX_STRING) role_factory1.role, _DEFAULT_ROLE1);
  role_factory1.pf_cport = instantiate_config_port;
  role_factory1.pf_port[0] = instantiate_pcm_port;
  role_factory1.nports = 1;
  role_factory1.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING) role_factory2.role, _DEFAULT_ROLE2);
  role_factory2.pf_cport = instantiate_config_port;
  role_factory2.pf_port[0] = instantiate_pcm_port;
  role_factory2.nports = 1;
  role_factory2.pf_proc = instantiate_processor;

  TIZ_LOG (TIZ_TRACE, "OMX_ComponentInit: "
           "Inititializing the test component");

  assert (ap_hdl);

  /* Initialize the component infrastructure */
  RETURN_ON_IL_ERR (tiz_comp_init (ap_hdl, _COMPONENT_NAME));

  /* Register two roles */
  RETURN_ON_IL_ERR (tiz_comp_register_roles (ap_hdl, rf_list, 2));

  /* Register alloc hooks */
  RETURN_ON_IL_ERR (tiz_comp_register_alloc_hooks
                    (ap_hdl, 0, &new_hooks, &old_hooks));

  /* Verify that the old hooks have been returned */
  assert (old_hooks.pf_alloc);
  assert (old_hooks.pf_free);

  return OMX_ErrorNone;
}
