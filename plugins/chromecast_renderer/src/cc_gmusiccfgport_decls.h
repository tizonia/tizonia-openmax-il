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
 * @file   cc_gmusiccfgport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  A specialised config port class for the Google Music renderer component
 *
 *
 */

#ifndef CC_GMUSICCFGPORT_DECLS_H
#define CC_GMUSICCFGPORT_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_TizoniaExt.h>
#include <OMX_Types.h>

#include "cc_cfgport_decls.h"

typedef struct cc_gmusic_cfgport cc_gmusic_cfgport_t;
struct cc_gmusic_cfgport
{
  /* Object */
  const cc_cfgport_t _;
  OMX_TIZONIA_AUDIO_PARAM_GMUSICSESSIONTYPE gm_session_;
  OMX_TIZONIA_AUDIO_PARAM_GMUSICPLAYLISTTYPE playlist_;
};

typedef struct cc_gmusic_cfgport_class cc_gmusic_cfgport_class_t;
struct cc_gmusic_cfgport_class
{
  /* Class */
  const cc_cfgport_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* CC_GMUSICCFGPORT_DECLS_H */
