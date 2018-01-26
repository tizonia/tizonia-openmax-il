/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   cc_gmusicprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Google Music client - processor declarations
 *
 *
 */

#ifndef CC_GMUSICPRC_DECLS_H
#define CC_GMUSICPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>
#include <tizgmusic_c.h>
#include <tizcastclient_c.h>

#include <tizplatform.h>

typedef struct cc_gmusic_prc cc_gmusic_prc_t;
struct cc_gmusic_prc
{
  /* Object */
  const tiz_prc_t _;
  OMX_TIZONIA_AUDIO_PARAM_GMUSICSESSIONTYPE gm_session_;
  OMX_TIZONIA_AUDIO_PARAM_GMUSICPLAYLISTTYPE playlist_;
  OMX_TIZONIA_PLAYLISTSKIPTYPE playlist_skip_;
  OMX_TIZONIA_PARAM_CHROMECASTSESSIONTYPE cc_session_;
  OMX_PARAM_CONTENTURITYPE * p_uri_param_;
  OMX_BUFFERHEADERTYPE * p_inhdr_;
  tiz_gmusic_t * p_gm_;
  tiz_cast_t * p_cc_;
  tiz_cast_client_cast_status_t cc_cast_status_;
  tiz_cast_client_media_status_t cc_media_status_;
  char * p_cc_display_title_;
  OMX_UUIDTYPE cc_uuid_;
  bool eos_;
  bool port_disabled_;
  bool uri_changed_;
  OMX_U32 bytes_before_eos_;
};

typedef struct cc_gmusic_prc_class cc_gmusic_prc_class_t;
struct cc_gmusic_prc_class
{
  /* Class */
  const tiz_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* CC_GMUSICPRC_DECLS_H */
