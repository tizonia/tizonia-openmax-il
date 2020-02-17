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
 * @file   spfysrc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Spotify client component
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
#include "spfysrccfgport.h"
#include "spfysrc.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.spotify_source"
#endif

/**
 *@defgroup libtizspotifysrc 'libtizspotifysrc' : OpenMAX IL Spotify client
 *
 * - Component name : "OMX.Aratelia.audio_source.spotify.pcm"
 * - Implements role: "audio_source.pcm.spotify"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE spotify_source_version = {{1, 0, 0, 0}};

static OMX_PTR
instantiate_pcm_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  OMX_AUDIO_CONFIG_MUTETYPE mute;
  OMX_AUDIO_CODINGTYPE encodings[]
    = {OMX_AUDIO_CodingUnused, OMX_AUDIO_CodingAutoDetect, OMX_AUDIO_CodingPCM,
       OMX_AUDIO_CodingMax};
  tiz_port_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirOutput,
    ARATELIA_SPOTIFY_SOURCE_PORT_MIN_BUF_COUNT,
    ARATELIA_SPOTIFY_SOURCE_PORT_MIN_BUF_SIZE,
    ARATELIA_SPOTIFY_SOURCE_PORT_NONCONTIGUOUS,
    ARATELIA_SPOTIFY_SOURCE_PORT_ALIGNMENT,
    ARATELIA_SPOTIFY_SOURCE_PORT_SUPPLIERPREF,
    {ARATELIA_SPOTIFY_SOURCE_PORT_INDEX, NULL, NULL, NULL},
    -1 /* use -1 for now */
  };

  /* Instantiate the pcm port */
  pcmmode.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmmode.nVersion.nVersion = OMX_VERSION;
  pcmmode.nPortIndex = ARATELIA_SPOTIFY_SOURCE_PORT_INDEX;
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
  volume.nPortIndex = ARATELIA_SPOTIFY_SOURCE_PORT_INDEX;
  volume.bLinear = OMX_FALSE;
  volume.sVolume.nValue = ARATELIA_SPOTIFY_SOURCE_DEFAULT_VOLUME_VALUE;
  volume.sVolume.nMin = ARATELIA_SPOTIFY_SOURCE_MIN_VOLUME_VALUE;
  volume.sVolume.nMax = ARATELIA_SPOTIFY_SOURCE_MAX_VOLUME_VALUE;

  mute.nSize = sizeof (OMX_AUDIO_CONFIG_MUTETYPE);
  mute.nVersion.nVersion = OMX_VERSION;
  mute.nPortIndex = ARATELIA_SPOTIFY_SOURCE_PORT_INDEX;
  mute.bMute = OMX_FALSE;

  return factory_new (tiz_get_type (ap_hdl, "tizpcmport"), &port_opts,
                      &encodings, &pcmmode, &volume, &mute);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "spfysrccfgport"),
                      NULL, /* this port does not take options */
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
  const tiz_role_factory_t * rf_list[] = {&role_factory};
  tiz_type_factory_t spfysrcprc_type;
  tiz_type_factory_t spfysrccfgport_type;
  const tiz_type_factory_t * tf_list[]
    = {&spfysrcprc_type, &spfysrccfgport_type};

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_SPOTIFY_SOURCE_DEFAULT_ROLE);
  role_factory.pf_cport = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_pcm_port;
  role_factory.nports = 1;
  role_factory.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING) spfysrcprc_type.class_name, "spfysrcprc_class");
  spfysrcprc_type.pf_class_init = spfysrc_prc_class_init;
  strcpy ((OMX_STRING) spfysrcprc_type.object_name, "spfysrcprc");
  spfysrcprc_type.pf_object_init = spfysrc_prc_init;

  strcpy ((OMX_STRING) spfysrccfgport_type.class_name, "spfysrccfgport_class");
  spfysrccfgport_type.pf_class_init = spfysrc_cfgport_class_init;
  strcpy ((OMX_STRING) spfysrccfgport_type.object_name, "spfysrccfgport");
  spfysrccfgport_type.pf_object_init = spfysrc_cfgport_init;

  /* Initialize the component infrastructure */
  tiz_check_omx (
    tiz_comp_init (ap_hdl, ARATELIA_SPOTIFY_SOURCE_COMPONENT_NAME));

  /* Register the "spfysrcprc" and "spfysrccfgport" classes */
  tiz_check_omx (tiz_comp_register_types (ap_hdl, tf_list, 2));

  /* Register this component's role */
  tiz_check_omx (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
