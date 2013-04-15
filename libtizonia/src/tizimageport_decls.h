/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
 * @file   tizimageport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - imageport class declarations
 *
 *
 */

#ifndef TIZIMAGEPORT_DECLS_H
#define TIZIMAGEPORT_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Component.h"

#include "tizport_decls.h"
#include "tizosal.h"

  typedef struct tiz_imageport tiz_imageport_t;
  struct tiz_imageport
  {
    /* Object */
    const tiz_port_t _;
    OMX_IMAGE_PARAM_PORTFORMATTYPE port_format_;
    tiz_vector_t *p_image_encodings_;
    tiz_vector_t *p_color_formats_;
  };

  typedef struct tiz_imageport_class tiz_imageport_class_t;
  struct tiz_imageport_class
  {
    /* Class */
    const tiz_port_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* TIZIMAGEPORT_DECLS_H */
