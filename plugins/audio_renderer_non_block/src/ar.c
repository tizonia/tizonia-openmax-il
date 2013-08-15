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
 * @file   audio_renderer.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - PCM Audio Renderer based on ALSA
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
#include "tizutils.h"
#include "tizscheduler.h"
#include "tizpcmport.h"
#include "tizconfigport.h"
#include "arprc.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.audio_renderer"
#endif

#define ARATELIA_AUDIO_RENDERER_DEFAULT_ROLE "audio_renderer.pcm"
#define ARATELIA_AUDIO_RENDERER_COMPONENT_NAME "OMX.Aratelia.audio_renderer_nb.pcm"
#define ARATELIA_AUDIO_RENDERER_PORT_INDEX 0 /* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_AUDIO_RENDERER_PORT_MIN_BUF_COUNT 2
#define ARATELIA_AUDIO_RENDERER_PORT_MIN_BUF_SIZE 1024
#define ARATELIA_AUDIO_RENDERER_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_AUDIO_RENDERER_PORT_ALIGNMENT 0
#define ARATELIA_AUDIO_RENDERER_PORT_SUPPLIERPREF OMX_BufferSupplyInput

static OMX_VERSIONTYPE audio_renderer_version = { {1, 0, 0, 0} };

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
    ARATELIA_AUDIO_RENDERER_PORT_MIN_BUF_COUNT,
    ARATELIA_AUDIO_RENDERER_PORT_MIN_BUF_SIZE,
    ARATELIA_AUDIO_RENDERER_PORT_NONCONTIGUOUS,
    ARATELIA_AUDIO_RENDERER_PORT_ALIGNMENT,
    ARATELIA_AUDIO_RENDERER_PORT_SUPPLIERPREF,
    {ARATELIA_AUDIO_RENDERER_PORT_INDEX, NULL, NULL, NULL},
    -1                          /* use -1 for now */
  };

  /* Instantiate the pcm port */
  pcmmode.nSize              = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmmode.nVersion.nVersion  = OMX_VERSION;
  pcmmode.nPortIndex         = ARATELIA_AUDIO_RENDERER_PORT_INDEX;
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
  volume.nPortIndex        = ARATELIA_AUDIO_RENDERER_PORT_INDEX;
  volume.bLinear           = OMX_FALSE;
  volume.sVolume.nValue    = 50;
  volume.sVolume.nMin      = 0;
  volume.sVolume.nMax      = 100;
  

  mute.nSize             = sizeof (OMX_AUDIO_CONFIG_MUTETYPE);
  mute.nVersion.nVersion = OMX_VERSION;
  mute.nPortIndex        = ARATELIA_AUDIO_RENDERER_PORT_INDEX;
  mute.bMute             = OMX_FALSE;

  tiz_check_omx_err_ret_null (tiz_pcmport_init ());
  return factory_new (tizpcmport, ap_hdl, &port_opts, &encodings,
                           &pcmmode, &volume, &mute);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  /* Instantiate the config port */
  tiz_check_omx_err_ret_null (tiz_configport_init ());
  return factory_new (tizconfigport, ap_hdl, NULL,   /* this port does not take options */
                      ARATELIA_AUDIO_RENDERER_COMPONENT_NAME,
                      audio_renderer_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  tiz_check_omx_err_ret_null (ar_prc_init ());
  return factory_new (arprc, ap_hdl);
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t *rf_list[] = { &role_factory };

  TIZ_LOG (TIZ_TRACE, "OMX_ComponentInit: Inititializing [%s]",
           ARATELIA_AUDIO_RENDERER_COMPONENT_NAME);

  strcpy ((OMX_STRING) role_factory.role,
          ARATELIA_AUDIO_RENDERER_DEFAULT_ROLE);
  role_factory.pf_cport   = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_pcm_port;
  role_factory.nports     = 1;
  role_factory.pf_proc    = instantiate_processor;

  tiz_check_omx_err (tiz_comp_init (ap_hdl, ARATELIA_AUDIO_RENDERER_COMPONENT_NAME));
  tiz_check_omx_err (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
