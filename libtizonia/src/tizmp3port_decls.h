/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizmp3port_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  mp3port class declarations
 *
 *
 */

#ifndef TIZMP3PORT_DECLS_H
#define TIZMP3PORT_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tizaudioport_decls.h"

  typedef struct tiz_mp3port tiz_mp3port_t;
  struct tiz_mp3port
  {
    /* Object */
    const tiz_audioport_t _;
    OMX_AUDIO_PARAM_MP3TYPE mp3type_;
  };

  typedef struct tiz_mp3port_class tiz_mp3port_class_t;
  struct tiz_mp3port_class
  {
    /* Class */
    const tiz_audioport_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* TIZMP3PORT_DECLS_H */
