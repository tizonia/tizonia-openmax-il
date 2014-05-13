/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   tizaudioport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - audioport class declarations
 *
 *
 */

#ifndef TIZAUDIOPORT_DECLS_H
#define TIZAUDIOPORT_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tizport_decls.h"

  typedef struct tiz_audioport tiz_audioport_t;
  struct tiz_audioport
  {
    /* Object */
    const tiz_port_t _;
    OMX_AUDIO_PARAM_PORTFORMATTYPE port_format_;
    tiz_vector_t *p_encodings_;
  };

  typedef struct tiz_audioport_class tiz_audioport_class_t;
  struct tiz_audioport_class
  {
    /* Class */
    const tiz_port_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* TIZAUDIOPORT_DECLS_H */
