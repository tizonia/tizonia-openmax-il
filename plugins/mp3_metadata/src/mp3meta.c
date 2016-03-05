/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   mp3meta.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Mp3 Metadata Eraser component
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

#include "mp3metaprc.h"
#include "mp3meta.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.mp3_metadata"
#endif

/**
 *@defgroup libtizmp3meta 'libtizmp3meta' : OpenMAX IL MP3 metadata eraser
 *
 * - Component name : "OMX.Aratelia.audio_metadata_eraser.mp3"
 * - Implements role: "audio_metadata_eraser.mp3"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE mp3_metadata_version = { { 1, 0, 0, 0 } };

static OMX_PTR instantiate_output_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_MP3TYPE mp3type;
  OMX_AUDIO_CODINGTYPE encodings[]
      = { OMX_AUDIO_CodingMP3, OMX_AUDIO_CodingMax };
  tiz_port_options_t mp3_port_opts
      = { OMX_PortDomainAudio,
          OMX_DirOutput,
          ARATELIA_MP3_METADATA_ERASER_PORT_MIN_BUF_COUNT,
          ARATELIA_MP3_METADATA_ERASER_PORT_MIN_BUF_SIZE,
          ARATELIA_MP3_METADATA_ERASER_PORT_NONCONTIGUOUS,
          ARATELIA_MP3_METADATA_ERASER_PORT_ALIGNMENT,
          ARATELIA_MP3_METADATA_ERASER_PORT_SUPPLIERPREF,
          { ARATELIA_MP3_METADATA_ERASER_PORT_INDEX, NULL, NULL, NULL },
          -1 /* use -1 for now */
      };

  mp3type.nSize = sizeof(OMX_AUDIO_PARAM_MP3TYPE);
  mp3type.nVersion.nVersion = OMX_VERSION;
  mp3type.nPortIndex = 0;
  mp3type.nChannels = 2;
  mp3type.nBitRate = 0;
  mp3type.nSampleRate = 0;
  mp3type.nAudioBandWidth = 0;
  mp3type.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  mp3type.eFormat = OMX_AUDIO_MP3StreamFormatMP1Layer3;

  return factory_new (tiz_get_type (ap_hdl, "tizmp3port"), &mp3_port_opts,
                      &encodings, &mp3type);
}

static OMX_PTR instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizuricfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_MP3_METADATA_ERASER_COMPONENT_NAME,
                      mp3_metadata_version);
}

static OMX_PTR instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "mp3metaprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t *rf_list[] = { &role_factory };
  tiz_type_factory_t mp3metaprc_type;
  const tiz_type_factory_t *tf_list[] = { &mp3metaprc_type };

  strcpy ((OMX_STRING)role_factory.role,
          ARATELIA_MP3_METADATA_ERASER_DEFAULT_ROLE);
  role_factory.pf_cport = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_output_port;
  role_factory.nports = 1;
  role_factory.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING)mp3metaprc_type.class_name, "mp3metaprc_class");
  mp3metaprc_type.pf_class_init = mp3meta_prc_class_init;
  strcpy ((OMX_STRING)mp3metaprc_type.object_name, "mp3metaprc");
  mp3metaprc_type.pf_object_init = mp3meta_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx_err (
      tiz_comp_init (ap_hdl, ARATELIA_MP3_METADATA_ERASER_COMPONENT_NAME));

  /* Register the "mp3metaprc" class */
  tiz_check_omx_err (tiz_comp_register_types (ap_hdl, tf_list, 1));

  /* Register the various roles */
  tiz_check_omx_err (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
