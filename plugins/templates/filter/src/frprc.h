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
 * @file   frprc.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Tizonia OpenMAX IL - File Reader processor class
 * 
 * 
 */

#ifndef FRPRC_H
#define FRPRC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Core.h"

/* factory_new(frprc, ...) */
  extern const void *frprc;

  OMX_ERRORTYPE fr_prc_init (void);

#ifdef __cplusplus
}
#endif

#endif                          /* FRPRC_H */
