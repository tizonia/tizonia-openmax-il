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
 * @file   httpsrcport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  A specialised port class for the HTTP source component - class decls
 *
 *
 */

#ifndef HTTPSRCPORT_DECLS_H
#define HTTPSRCPORT_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Audio.h>
#include <OMX_TizoniaExt.h>
#include <OMX_Types.h>

#include <tizaudioport_decls.h>

typedef struct httpsrc_port httpsrc_port_t;
struct httpsrc_port
{
  /* Object */
  const tiz_audioport_t _;
  OMX_AUDIO_PARAM_MP3TYPE mp3type_;
  OMX_AUDIO_PARAM_AACPROFILETYPE aactype_;
  OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE opustype_;
};

typedef struct httpsrc_port_class httpsrc_port_class_t;
struct httpsrc_port_class
{
  /* Class */
  const tiz_audioport_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* HTTPSRCPORT_DECLS_H */
