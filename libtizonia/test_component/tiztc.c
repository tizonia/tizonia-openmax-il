/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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

#include "tiztcproc.h"
#include "tizscheduler.h"
#include "tizport.h"
#include "tizpcmport.h"
#include "tizconfigport.h"

#include "tizplatform.h"

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Types.h"

#include <assert.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.test_comp"
#endif

#define TC_DEFAULT_ROLE1 "tizonia_test_component.role1"
#define TC_DEFAULT_ROLE2 "tizonia_test_component.role2"
#define TC_COMPONENT_NAME "OMX.Aratelia.tizonia.test_component"
#define TC_PORT_MIN_BUF_COUNT 1
#define TC_PORT_MIN_BUF_SIZE 1024
#define TC_PORT_NONCONTIGUOUS OMX_FALSE
#define TC_PORT_ALIGNMENT 0
#define TC_PORT_SUPPLIERPREF OMX_BufferSupplyInput

static OMX_VERSIONTYPE tc_comp_version = { {1, 0, 0, 0} };

static OMX_U8 *
pcm_port_alloc_hook (OMX_U32 * ap_size,
                     OMX_PTR * app_port_priv, void *ap_args)
{
  OMX_U8 *p = NULL;
  assert (ap_size);
  p = tiz_mem_alloc (*ap_size * sizeof (OMX_U8));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Test Component Alloc Hook :size[%u] p=[%p]",
           *ap_size, p);
  return p;
}

static void
pcm_port_free_hook (OMX_PTR ap_buf, OMX_PTR ap_port_priv, void *ap_args)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Test Component Free Hook : ap_buf[%p]", ap_buf);
  assert (ap_buf);
  tiz_mem_free (ap_buf);
}

static OMX_BOOL
egl_image_validation_hook (const OMX_HANDLETYPE ap_hdl,
                           OMX_U32 pid, OMX_PTR ap_eglimage,
                           void *ap_args)
{
  assert (ap_hdl);
  assert (ap_eglimage);
  assert (!ap_args);
  TIZ_LOG (TIZ_PRIORITY_TRACE,
           "Test Component EGLImage Hook : ap_eglimage=[%p]", ap_eglimage);
  return OMX_TRUE;
}

static OMX_PTR
instantiate_pcm_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  OMX_AUDIO_CONFIG_MUTETYPE mute;
  OMX_AUDIO_CODINGTYPE encodings[] = {
    OMX_AUDIO_CodingPCM,
    OMX_AUDIO_CodingMax
  };
  tiz_port_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirInput,
    TC_PORT_MIN_BUF_COUNT,
    TC_PORT_MIN_BUF_SIZE,
    TC_PORT_NONCONTIGUOUS,
    TC_PORT_ALIGNMENT,
    TC_PORT_SUPPLIERPREF,
    {0, pcm_port_alloc_hook, pcm_port_free_hook, NULL},
    -1
  };

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Inititializing the test component's pcm port");

  /* Instantiate the pcm port */
  pcmmode.nSize              = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmmode.nVersion.nVersion  = OMX_VERSION;
  pcmmode.nPortIndex         = 0;
  pcmmode.nChannels          = 2;
  pcmmode.eNumData           = OMX_NumericalDataSigned;
  pcmmode.eEndian            = OMX_EndianLittle;
  pcmmode.bInterleaved       = OMX_TRUE;
  pcmmode.nBitPerSample      = 16;
  pcmmode.nSamplingRate      = 48000;
  pcmmode.ePCMMode           = OMX_AUDIO_PCMModeLinear;
  pcmmode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  pcmmode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

  volume.nSize             = sizeof (OMX_AUDIO_CONFIG_VOLUMETYPE);
  volume.nVersion.nVersion = OMX_VERSION;
  volume.nPortIndex        = 0;
  volume.bLinear           = OMX_FALSE;
  volume.sVolume.nValue    = 75;
  volume.sVolume.nMin      = 0;
  volume.sVolume.nMax      = 100;

  mute.nSize             = sizeof (OMX_AUDIO_CONFIG_MUTETYPE);
  mute.nVersion.nVersion = OMX_VERSION;
  mute.nPortIndex        = 0;
  mute.bMute             = OMX_FALSE;

  return factory_new (tiz_get_type (ap_hdl, "tizpcmport"), &port_opts,
                      &encodings, &pcmmode, &volume, &mute);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizconfigport"),
                      NULL,   /* this port does not take options */
                      TC_COMPONENT_NAME, tc_comp_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tiztcprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory1, role_factory2;
  const tiz_role_factory_t *rf_list[] = { &role_factory1, &role_factory2 };
  tiz_type_factory_t type_factory;
  const tiz_type_factory_t *tf_list[] = { &type_factory};
  const tiz_alloc_hooks_t new_hooks =
    { 0, pcm_port_alloc_hook, pcm_port_free_hook, NULL };
  tiz_alloc_hooks_t old_hooks = { 0, NULL, NULL, NULL };
  const tiz_eglimage_hook_t egl_validation_hook =
    { 0, egl_image_validation_hook, NULL };

  strcpy ((OMX_STRING) role_factory1.role, TC_DEFAULT_ROLE1);
  role_factory1.pf_cport = instantiate_config_port;
  role_factory1.pf_port[0] = instantiate_pcm_port;
  role_factory1.nports = 1;
  role_factory1.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING) role_factory2.role, TC_DEFAULT_ROLE2);
  role_factory2.pf_cport = instantiate_config_port;
  role_factory2.pf_port[0] = instantiate_pcm_port;
  role_factory2.nports = 1;
  role_factory2.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING) type_factory.class_name, "tiztcprc_class");
  type_factory.pf_class_init = tiz_tcprc_class_init;
  strcpy ((OMX_STRING) type_factory.object_name, "tiztcprc");
  type_factory.pf_object_init = tiz_tcprc_init;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_ComponentInit: "
           "Inititializing the test component");

  assert (ap_hdl);

  /* Initialize the component infrastructure */
  tiz_check_omx (tiz_comp_init (ap_hdl, TC_COMPONENT_NAME));

  /* Register the "tiztcprc" class */
  tiz_check_omx (tiz_comp_register_types (ap_hdl, tf_list, 1));

  /* Register two roles */
  tiz_check_omx (tiz_comp_register_roles (ap_hdl, rf_list, 2));

  /* Register alloc hooks */
  tiz_check_omx (tiz_comp_register_alloc_hooks
                     (ap_hdl, &new_hooks, &old_hooks));

  /* Register egl image validation hook */
  tiz_check_omx (tiz_comp_register_eglimage_hook
                     (ap_hdl, &egl_validation_hook));

  /* Verify that the old hooks have been returned */
  assert (old_hooks.pf_alloc);
  assert (old_hooks.pf_free);

  return OMX_ErrorNone;
}
