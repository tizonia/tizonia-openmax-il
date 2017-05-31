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
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   tizavcport_decls.h
 * @author Gurkirpal Singh
 * 
 * @brief  avcport class declarations
 * 
 * 
 */

#ifndef TIZAVCPORT_DECLS_H
#define TIZAVCPORT_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "OMX_Component.h"
#include "tizvideoport_decls.h"

typedef struct tiz_avcport tiz_avcport_t;
struct tiz_avcport
{
  /* Object */
  const tiz_videoport_t _;
  OMX_VIDEO_PARAM_AVCTYPE avctype_;
  OMX_VIDEO_PARAM_PROFILELEVELTYPE pltype_;
  tiz_vector_t * p_levels_;
  OMX_VIDEO_PARAM_BITRATETYPE pbrtype_;
  OMX_VIDEO_CONFIG_BITRATETYPE cbrtype_;
  OMX_CONFIG_FRAMERATETYPE frtype_;
};

typedef struct tiz_avcport_class tiz_avcport_class_t;
struct tiz_avcport_class
{
  /* Class */
  const tiz_videoport_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* TIZAVCPORT_DECLS_H */
