/**
 * Copyright (C) 2017 Julien Isorce
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

#ifndef VP8DINPORT_DECLS_H
#define VP8DINPORT_DECLS_H

#include <tizvp8port_decls.h>

typedef struct vp8d_inport vp8d_inport_t;
struct vp8d_inport
{
   /* Object */
   const tiz_vp8port_t _;
};

typedef struct vp8d_inport_class vp8d_inport_class_t;
struct vp8d_inport_class
{
   /* Class */
   const tiz_vp8port_class_t _;
   /* NOTE: Class methods might be added in the future */
};

#endif /* VP8DINPORT_DECLS_H */
