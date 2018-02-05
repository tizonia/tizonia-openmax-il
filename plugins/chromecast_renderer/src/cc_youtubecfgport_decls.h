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
 * @file   cc_youtubecfgport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  A specialised config port class for the Youtube renderer component
 *
 *
 */

#ifndef CC_YOUTUBECFGPORT_DECLS_H
#define CC_YOUTUBECFGPORT_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_TizoniaExt.h>
#include <OMX_Types.h>

#include "cc_cfgport_decls.h"

typedef struct cc_youtube_cfgport cc_youtube_cfgport_t;
struct cc_youtube_cfgport
{
  /* Object */
  const cc_cfgport_t _;
  OMX_TIZONIA_AUDIO_PARAM_YOUTUBESESSIONTYPE session_;
  OMX_TIZONIA_AUDIO_PARAM_YOUTUBEPLAYLISTTYPE playlist_;
};

typedef struct cc_youtube_cfgport_class cc_youtube_cfgport_class_t;
struct cc_youtube_cfgport_class
{
  /* Class */
  const cc_cfgport_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* CC_YOUTUBECFGPORT_DECLS_H */
