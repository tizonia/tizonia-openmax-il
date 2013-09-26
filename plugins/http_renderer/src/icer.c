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
 * @brief  Tizonia OpenMAX IL - Icecast-like HTTP renderer
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizosal.h"
#include "tizscheduler.h"
#include "icermp3port.h"
#include "icercfgport.h"
#include "icerprc.h"

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Types.h"

#include <assert.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer"
#endif

#define ARATELIA_HTTP_RENDERER_DEFAULT_ROLE       "ice_renderer.http"
#define ARATELIA_HTTP_RENDERER_COMPONENT_NAME     "OMX.Aratelia.ice_renderer.http"
#define ARATELIA_HTTP_RENDERER_PORT_INDEX         0 /* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_HTTP_RENDERER_PORT_MIN_BUF_COUNT 2
#define ARATELIA_HTTP_RENDERER_PORT_MIN_BUF_SIZE  (8*1024)
#define ARATELIA_HTTP_RENDERER_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_HTTP_RENDERER_PORT_ALIGNMENT     0
#define ARATELIA_HTTP_RENDERER_PORT_SUPPLIERPREF  OMX_BufferSupplyInput

static OMX_VERSIONTYPE http_renderer_version = { {1, 0, 0, 0} };

static OMX_PTR
instantiate_mp3_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_MP3TYPE mp3type;
  OMX_AUDIO_CODINGTYPE encodings[] = {
    OMX_AUDIO_CodingMP3,
    OMX_AUDIO_CodingMax
  };
  tiz_port_options_t mp3_port_opts = {
    OMX_PortDomainAudio,
    OMX_DirInput,
    ARATELIA_HTTP_RENDERER_PORT_MIN_BUF_COUNT,
    ARATELIA_HTTP_RENDERER_PORT_MIN_BUF_SIZE,
    ARATELIA_HTTP_RENDERER_PORT_NONCONTIGUOUS,
    ARATELIA_HTTP_RENDERER_PORT_ALIGNMENT,
    ARATELIA_HTTP_RENDERER_PORT_SUPPLIERPREF,
    {ARATELIA_HTTP_RENDERER_PORT_INDEX, NULL, NULL, NULL},
    0                           /* Master port */
  };

  mp3type.nSize             = sizeof (OMX_AUDIO_PARAM_MP3TYPE);
  mp3type.nVersion.nVersion = OMX_VERSION;
  mp3type.nPortIndex        = ARATELIA_HTTP_RENDERER_PORT_INDEX;
  mp3type.nChannels         = 2;
  mp3type.nBitRate          = 128000;
  mp3type.nSampleRate       = 44100;
  mp3type.nAudioBandWidth   = 0;
  mp3type.eChannelMode      = OMX_AUDIO_ChannelModeStereo;
  mp3type.eFormat           = OMX_AUDIO_MP3StreamFormatMP1Layer3;

  tiz_check_omx_err_ret_null (icer_mp3port_init ());
  return factory_new (icermp3port, ap_hdl, &mp3_port_opts, &encodings, &mp3type);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  tiz_check_omx_err_ret_null (icer_cfgport_init ());
  return factory_new (icercfgport, ap_hdl, NULL,     /* this port does not take options */
                      ARATELIA_HTTP_RENDERER_COMPONENT_NAME,
                      http_renderer_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  tiz_check_omx_err_ret_null (icer_prc_init ());
  return factory_new (icerprc, ap_hdl);
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t *rf_list[] = { &role_factory };

  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_ComponentInit: Inititializing [%s]",
           ARATELIA_HTTP_RENDERER_COMPONENT_NAME);

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_HTTP_RENDERER_DEFAULT_ROLE);
  role_factory.pf_cport   = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_mp3_port;
  role_factory.nports     = 1;
  role_factory.pf_proc    = instantiate_processor;

  tiz_check_omx_err (tiz_comp_init (ap_hdl, ARATELIA_HTTP_RENDERER_COMPONENT_NAME));
  tiz_check_omx_err (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
