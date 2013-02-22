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
 * @file   tizvp8port.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  VP8 port class
 * 
 * 
 */

#ifndef TIZVP8PORT_H
#define TIZVP8PORT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Core.h"
#include "OMX_Types.h"

#include "tizvideoport.h"

/* factory_new(tizvp8port, ...); */
  extern const void *tizvp8port;
  extern const void *tizvp8port_class;

  void init_tizvp8port (void);

#ifdef __cplusplus
}
#endif

#endif                          /* TIZVP8PORT_H */
