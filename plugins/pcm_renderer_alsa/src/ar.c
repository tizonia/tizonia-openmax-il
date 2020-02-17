/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @brief  Tizonia - PCM Audio Renderer based on ALSA
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

#include "arprc.h"
#include "ar.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.audio_renderer"
#endif

/**
 *@defgroup libtizalsapcmrnd 'libtizalsapcmrnd' : OpenMAX IL PCM Audio Renderer
 *based on ALSA
 *
 * - Component name : "OMX.Aratelia.audio_renderer.alsa.pcm"
 * - Implements role: "audio_renderer.pcm"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE audio_renderer_version = {{1, 0, 0, 0}};

static OMX_PTR
instantiate_pcm_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  OMX_AUDIO_CONFIG_MUTETYPE mute;
  OMX_AUDIO_CODINGTYPE encodings[] = {OMX_AUDIO_CodingPCM, OMX_AUDIO_CodingMax};
  tiz_port_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirInput,
    ARATELIA_AUDIO_RENDERER_PORT_MIN_BUF_COUNT,
    ARATELIA_AUDIO_RENDERER_PORT_MIN_BUF_SIZE,
    ARATELIA_AUDIO_RENDERER_PORT_NONCONTIGUOUS,
    ARATELIA_AUDIO_RENDERER_PORT_ALIGNMENT,
    ARATELIA_AUDIO_RENDERER_PORT_SUPPLIERPREF,
    {ARATELIA_AUDIO_RENDERER_PORT_INDEX, NULL, NULL, NULL},
    -1 /* use -1 for now */
  };

  /* Instantiate the pcm port */
  pcmmode.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmmode.nVersion.nVersion = OMX_VERSION;
  pcmmode.nPortIndex = ARATELIA_AUDIO_RENDERER_PORT_INDEX;
  pcmmode.nChannels = 2;
  pcmmode.eNumData = OMX_NumericalDataSigned;
  pcmmode.eEndian = OMX_EndianLittle;
  pcmmode.bInterleaved = OMX_TRUE;
  pcmmode.nBitPerSample = 16;
  pcmmode.nSamplingRate = 48000;
  pcmmode.ePCMMode = OMX_AUDIO_PCMModeLinear;
  pcmmode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  pcmmode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

  volume.nSize = sizeof (OMX_AUDIO_CONFIG_VOLUMETYPE);
  volume.nVersion.nVersion = OMX_VERSION;
  volume.nPortIndex = ARATELIA_AUDIO_RENDERER_PORT_INDEX;
  volume.bLinear = OMX_FALSE;
  volume.sVolume.nValue = ARATELIA_AUDIO_RENDERER_DEFAULT_VOLUME_VALUE;
  volume.sVolume.nMin = ARATELIA_AUDIO_RENDERER_MIN_VOLUME_VALUE;
  volume.sVolume.nMax = ARATELIA_AUDIO_RENDERER_MAX_VOLUME_VALUE;

  mute.nSize = sizeof (OMX_AUDIO_CONFIG_MUTETYPE);
  mute.nVersion.nVersion = OMX_VERSION;
  mute.nPortIndex = ARATELIA_AUDIO_RENDERER_PORT_INDEX;
  mute.bMute = OMX_FALSE;

  return factory_new (tiz_get_type (ap_hdl, "tizpcmport"), &port_opts,
                      &encodings, &pcmmode, &volume, &mute);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  /* Instantiate the config port */
  return factory_new (tiz_get_type (ap_hdl, "tizconfigport"),
                      NULL, /* this port does not take options */
                      ARATELIA_AUDIO_RENDERER_COMPONENT_NAME,
                      audio_renderer_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "arprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t * rf_list[] = {&role_factory};
  tiz_type_factory_t type_factory;
  const tiz_type_factory_t * tf_list[] = {&type_factory};

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_AUDIO_RENDERER_DEFAULT_ROLE);
  role_factory.pf_cport = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_pcm_port;
  role_factory.nports = 1;
  role_factory.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING) type_factory.class_name, "arprc_class");
  type_factory.pf_class_init = ar_prc_class_init;
  strcpy ((OMX_STRING) type_factory.object_name, "arprc");
  type_factory.pf_object_init = ar_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx (
    tiz_comp_init (ap_hdl, ARATELIA_AUDIO_RENDERER_COMPONENT_NAME));

  /* Register the "arprc" processor class */
  tiz_check_omx (tiz_comp_register_types (ap_hdl, tf_list, 1));

  /* Register pcm renderer role */
  tiz_check_omx (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
