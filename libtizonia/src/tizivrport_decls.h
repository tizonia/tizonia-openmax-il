/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   tizivrport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  i/v renderer port class declarations
 *
 *
 */

#ifndef TIZIVRPORT_DECLS_H
#define TIZIVRPORT_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tizvideoport_decls.h"

typedef struct tiz_ivrport tiz_ivrport_t;
struct tiz_ivrport
{
  /* Object */
  const tiz_videoport_t _;
  OMX_CONFIG_ROTATIONTYPE crotate_;
  OMX_CONFIG_MIRRORTYPE cmirror_;
  OMX_CONFIG_SCALEFACTORTYPE cscale_;
  OMX_CONFIG_RECTTYPE cincrop_;
  OMX_TIME_CONFIG_RENDERINGDELAYTYPE crendelay_;
};

typedef struct tiz_ivrport_class tiz_ivrport_class_t;
struct tiz_ivrport_class
{
  /* Class */
  const tiz_videoport_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* TIZIVRPORT_DECLS_H */
