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
 * along with Tizonia.  If not, see <chromecast://www.gnu.org/licenses/>.
 */

/**
 * @file   chromecastrnd.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Chromecast renderer component
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
#include <OMX_TizoniaExt.h>
#include <OMX_Types.h>

#include <tizplatform.h>

#include <tizport.h>
#include <tizscheduler.h>

#include "chromecastrnd.h"
#include "cc_prc.h"
#include "cc_cfgport.h"
#include "cc_httpprc.h"
#include "cc_gmusicprc.h"
#include "cc_gmusiccfgport.h"
#include "cc_scloudprc.h"
#include "cc_scloudcfgport.h"
#include "cc_tuneinprc.h"
#include "cc_tuneincfgport.h"
#include "cc_youtubeprc.h"
#include "cc_youtubecfgport.h"
#include "cc_plexprc.h"
#include "cc_plexcfgport.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer"
#endif

/**
 *@defgroup libtizchromecastrnd 'libtizchromecastrnd' : OpenMAX IL Chromecast
 *audio renderer
 *
 * - Component name : "OMX.Aratelia.audio_renderer.chromecast"
 * - Implements role: "audio_renderer.chromecast"
 * - Implements role: "audio_renderer.chromecast.gmusic"
 * - Implements role: "audio_renderer.chromecast.scloud"
 * - Implements role: "audio_renderer.chromecast.tunein" (DEPRECATED)
 * - Implements role: "audio_renderer.chromecast.youtube"
 * - Implements role: "audio_renderer.chromecast.plex"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE chromecast_renderer_version = {{1, 0, 0, 0}};

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
    ARATELIA_CHROMECAST_RENDERER_PORT_MIN_BUF_COUNT,
    ARATELIA_CHROMECAST_RENDERER_PORT_MIN_BUF_SIZE,
    ARATELIA_CHROMECAST_RENDERER_PORT_NONCONTIGUOUS,
    ARATELIA_CHROMECAST_RENDERER_PORT_ALIGNMENT,
    ARATELIA_CHROMECAST_RENDERER_PORT_SUPPLIERPREF,
    {ARATELIA_CHROMECAST_RENDERER_PORT_INDEX, NULL, NULL, NULL},
    -1 /* use -1 for now */
  };

  /* Instantiate the pcm port */
  pcmmode.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmmode.nVersion.nVersion = OMX_VERSION;
  pcmmode.nPortIndex = ARATELIA_CHROMECAST_RENDERER_PORT_INDEX;
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
  volume.nPortIndex = ARATELIA_CHROMECAST_RENDERER_PORT_INDEX;
  volume.bLinear = OMX_FALSE;
  volume.sVolume.nValue = ARATELIA_CHROMECAST_RENDERER_DEFAULT_VOLUME_VALUE;
  volume.sVolume.nMin = ARATELIA_CHROMECAST_RENDERER_MIN_VOLUME_VALUE;
  volume.sVolume.nMax = ARATELIA_CHROMECAST_RENDERER_MAX_VOLUME_VALUE;

  mute.nSize = sizeof (OMX_AUDIO_CONFIG_MUTETYPE);
  mute.nVersion.nVersion = OMX_VERSION;
  mute.nPortIndex = ARATELIA_CHROMECAST_RENDERER_PORT_INDEX;
  mute.bMute = OMX_FALSE;

  return factory_new (tiz_get_type (ap_hdl, "tizpcmport"), &port_opts,
                      &encodings, &pcmmode, &volume, &mute);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_cfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME,
                      chromecast_renderer_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_httpprc"));
}

static OMX_PTR
instantiate_gmusic_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_gmusiccfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME,
                      chromecast_renderer_version);
}

static OMX_PTR
instantiate_gmusic_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_gmusicprc"));
}

static OMX_PTR
instantiate_scloud_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_scloudcfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME,
                      chromecast_renderer_version);
}

static OMX_PTR
instantiate_scloud_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_scloudprc"));
}

static OMX_PTR
instantiate_tunein_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_tuneincfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME,
                      chromecast_renderer_version);
}

static OMX_PTR
instantiate_tunein_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_tuneinprc"));
}

static OMX_PTR
instantiate_youtube_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_youtubecfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME,
                      chromecast_renderer_version);
}

static OMX_PTR
instantiate_youtube_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_youtubeprc"));
}

static OMX_PTR
instantiate_plex_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_plexcfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME,
                      chromecast_renderer_version);
}

static OMX_PTR
instantiate_plex_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "cc_plexprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t http_client_role;
  tiz_role_factory_t gmusic_client_role;
  tiz_role_factory_t scloud_client_role;
  tiz_role_factory_t tunein_client_role;
  tiz_role_factory_t youtube_client_role;
  tiz_role_factory_t plex_client_role;
  const tiz_role_factory_t * rf_list[]
    = {&http_client_role, &gmusic_client_role, &scloud_client_role,
       &tunein_client_role, &youtube_client_role, &plex_client_role};
  tiz_type_factory_t cc_prc_type;
  tiz_type_factory_t cc_cfgport_type;
  tiz_type_factory_t cc_httpprc_type;
  tiz_type_factory_t cc_gmusicprc_type;
  tiz_type_factory_t cc_gmusiccfgport_type;
  tiz_type_factory_t cc_scloudprc_type;
  tiz_type_factory_t cc_scloudcfgport_type;
  tiz_type_factory_t cc_tuneinprc_type;
  tiz_type_factory_t cc_tuneincfgport_type;
  tiz_type_factory_t cc_youtubeprc_type;
  tiz_type_factory_t cc_youtubecfgport_type;
  tiz_type_factory_t cc_plexprc_type;
  tiz_type_factory_t cc_plexcfgport_type;
  const tiz_type_factory_t * tf_list[]
    = {&cc_prc_type,           &cc_cfgport_type,       &cc_httpprc_type,
       &cc_gmusicprc_type,     &cc_gmusiccfgport_type, &cc_scloudprc_type,
       &cc_scloudcfgport_type, &cc_tuneinprc_type,     &cc_tuneincfgport_type,
       &cc_youtubeprc_type,    &cc_youtubecfgport_type, &cc_plexprc_type,    &cc_plexcfgport_type};

  strcpy ((OMX_STRING) http_client_role.role,
          ARATELIA_CHROMECAST_RENDERER_DEFAULT_ROLE);
  http_client_role.pf_cport = instantiate_config_port;
  http_client_role.pf_port[0] = instantiate_pcm_port;
  http_client_role.nports = 1;
  http_client_role.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING) gmusic_client_role.role,
          ARATELIA_GMUSIC_SOURCE_DEFAULT_ROLE);
  gmusic_client_role.pf_cport = instantiate_gmusic_config_port;
  gmusic_client_role.pf_port[0] = instantiate_pcm_port;
  gmusic_client_role.nports = 1;
  gmusic_client_role.pf_proc = instantiate_gmusic_processor;

  strcpy ((OMX_STRING) scloud_client_role.role,
          ARATELIA_SCLOUD_SOURCE_DEFAULT_ROLE);
  scloud_client_role.pf_cport = instantiate_scloud_config_port;
  scloud_client_role.pf_port[0] = instantiate_pcm_port;
  scloud_client_role.nports = 1;
  scloud_client_role.pf_proc = instantiate_scloud_processor;

  strcpy ((OMX_STRING) tunein_client_role.role,
          ARATELIA_TUNEIN_SOURCE_DEFAULT_ROLE);
  tunein_client_role.pf_cport = instantiate_tunein_config_port;
  tunein_client_role.pf_port[0] = instantiate_pcm_port;
  tunein_client_role.nports = 1;
  tunein_client_role.pf_proc = instantiate_tunein_processor;

  strcpy ((OMX_STRING) youtube_client_role.role,
          ARATELIA_YOUTUBE_SOURCE_DEFAULT_ROLE);
  youtube_client_role.pf_cport = instantiate_youtube_config_port;
  youtube_client_role.pf_port[0] = instantiate_pcm_port;
  youtube_client_role.nports = 1;
  youtube_client_role.pf_proc = instantiate_youtube_processor;

  strcpy ((OMX_STRING) plex_client_role.role,
          ARATELIA_PLEX_SOURCE_DEFAULT_ROLE);
  plex_client_role.pf_cport = instantiate_plex_config_port;
  plex_client_role.pf_port[0] = instantiate_pcm_port;
  plex_client_role.nports = 1;
  plex_client_role.pf_proc = instantiate_plex_processor;

  strcpy ((OMX_STRING) cc_prc_type.class_name, "cc_prc_class");
  cc_prc_type.pf_class_init = cc_prc_class_init;
  strcpy ((OMX_STRING) cc_prc_type.object_name, "cc_prc");
  cc_prc_type.pf_object_init = cc_prc_init;

  strcpy ((OMX_STRING) cc_cfgport_type.class_name, "cc_cfgport_class");
  cc_cfgport_type.pf_class_init = cc_cfgport_class_init;
  strcpy ((OMX_STRING) cc_cfgport_type.object_name, "cc_cfgport");
  cc_cfgport_type.pf_object_init = cc_cfgport_init;

  strcpy ((OMX_STRING) cc_httpprc_type.class_name, "cc_httpprc_class");
  cc_httpprc_type.pf_class_init = cc_http_prc_class_init;
  strcpy ((OMX_STRING) cc_httpprc_type.object_name, "cc_httpprc");
  cc_httpprc_type.pf_object_init = cc_http_prc_init;

  strcpy ((OMX_STRING) cc_gmusicprc_type.class_name, "cc_gmusicprc_class");
  cc_gmusicprc_type.pf_class_init = cc_gmusic_prc_class_init;
  strcpy ((OMX_STRING) cc_gmusicprc_type.object_name, "cc_gmusicprc");
  cc_gmusicprc_type.pf_object_init = cc_gmusic_prc_init;

  strcpy ((OMX_STRING) cc_gmusiccfgport_type.class_name,
          "cc_gmusiccfgport_class");
  cc_gmusiccfgport_type.pf_class_init = cc_gmusic_cfgport_class_init;
  strcpy ((OMX_STRING) cc_gmusiccfgport_type.object_name, "cc_gmusiccfgport");
  cc_gmusiccfgport_type.pf_object_init = cc_gmusic_cfgport_init;

  strcpy ((OMX_STRING) cc_scloudprc_type.class_name, "cc_scloudprc_class");
  cc_scloudprc_type.pf_class_init = cc_scloud_prc_class_init;
  strcpy ((OMX_STRING) cc_scloudprc_type.object_name, "cc_scloudprc");
  cc_scloudprc_type.pf_object_init = cc_scloud_prc_init;

  strcpy ((OMX_STRING) cc_scloudcfgport_type.class_name,
          "cc_scloudcfgport_class");
  cc_scloudcfgport_type.pf_class_init = cc_scloud_cfgport_class_init;
  strcpy ((OMX_STRING) cc_scloudcfgport_type.object_name, "cc_scloudcfgport");
  cc_scloudcfgport_type.pf_object_init = cc_scloud_cfgport_init;

  strcpy ((OMX_STRING) cc_tuneinprc_type.class_name, "cc_tuneinprc_class");
  cc_tuneinprc_type.pf_class_init = cc_tunein_prc_class_init;
  strcpy ((OMX_STRING) cc_tuneinprc_type.object_name, "cc_tuneinprc");
  cc_tuneinprc_type.pf_object_init = cc_tunein_prc_init;

  strcpy ((OMX_STRING) cc_tuneincfgport_type.class_name,
          "cc_tuneincfgport_class");
  cc_tuneincfgport_type.pf_class_init = cc_tunein_cfgport_class_init;
  strcpy ((OMX_STRING) cc_tuneincfgport_type.object_name, "cc_tuneincfgport");
  cc_tuneincfgport_type.pf_object_init = cc_tunein_cfgport_init;

  strcpy ((OMX_STRING) cc_youtubeprc_type.class_name, "cc_youtubeprc_class");
  cc_youtubeprc_type.pf_class_init = cc_youtube_prc_class_init;
  strcpy ((OMX_STRING) cc_youtubeprc_type.object_name, "cc_youtubeprc");
  cc_youtubeprc_type.pf_object_init = cc_youtube_prc_init;

  strcpy ((OMX_STRING) cc_youtubecfgport_type.class_name,
          "cc_youtubecfgport_class");
  cc_youtubecfgport_type.pf_class_init = cc_youtube_cfgport_class_init;
  strcpy ((OMX_STRING) cc_youtubecfgport_type.object_name, "cc_youtubecfgport");
  cc_youtubecfgport_type.pf_object_init = cc_youtube_cfgport_init;

  strcpy ((OMX_STRING) cc_plexprc_type.class_name, "cc_plexprc_class");
  cc_plexprc_type.pf_class_init = cc_plex_prc_class_init;
  strcpy ((OMX_STRING) cc_plexprc_type.object_name, "cc_plexprc");
  cc_plexprc_type.pf_object_init = cc_plex_prc_init;

  strcpy ((OMX_STRING) cc_plexcfgport_type.class_name,
          "cc_plexcfgport_class");
  cc_plexcfgport_type.pf_class_init = cc_plex_cfgport_class_init;
  strcpy ((OMX_STRING) cc_plexcfgport_type.object_name, "cc_plexcfgport");
  cc_plexcfgport_type.pf_object_init = cc_plex_cfgport_init;

  /* Initialize the component infrastructure */
  tiz_check_omx (
    tiz_comp_init (ap_hdl, ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME));

  /* Register the various classes */
  tiz_check_omx (tiz_comp_register_types (ap_hdl, tf_list, 13));

  /* Register the component roles */
  tiz_check_omx (tiz_comp_register_roles (ap_hdl, rf_list, 6));

  return OMX_ErrorNone;
}
