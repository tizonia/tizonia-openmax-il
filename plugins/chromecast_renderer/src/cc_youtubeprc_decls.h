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
 * @file   cc_youtubeprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Youtube client - processor declarations
 *
 *
 */

#ifndef YOUTUBEPRC_DECLS_H
#define YOUTUBEPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>

#include <tizplatform.h>
#include <tizyoutube_c.h>
#include <tizchromecast_c.h>

typedef struct cc_youtube_prc cc_youtube_prc_t;
struct cc_youtube_prc
{
  /* Object */
  const tiz_prc_t _;
  OMX_TIZONIA_AUDIO_PARAM_YOUTUBESESSIONTYPE yt_session_;
  OMX_TIZONIA_AUDIO_PARAM_YOUTUBEPLAYLISTTYPE playlist_;
  OMX_TIZONIA_PLAYLISTSKIPTYPE playlist_skip_;
  OMX_TIZONIA_PARAM_CHROMECASTSESSIONTYPE cc_session_;
  OMX_PARAM_CONTENTURITYPE * p_uri_param_;
  OMX_BUFFERHEADERTYPE * p_inhdr_;
  tiz_youtube_t * p_yt_;
  tiz_chromecast_t * p_cc_;
  bool eos_;
  bool port_disabled_;
  bool uri_changed_;
  OMX_U32 bytes_before_eos_;
  bool remove_current_url_;
};

typedef struct cc_youtube_prc_class cc_youtube_prc_class_t;
struct cc_youtube_prc_class
{
  /* Class */
  const tiz_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* YOUTUBEPRC_DECLS_H */
