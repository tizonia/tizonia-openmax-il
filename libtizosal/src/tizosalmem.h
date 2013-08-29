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
 * @file   tizosalmem.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL OSAL - Memory management
 *
 *
 */

#ifndef TIZOSALMEM_H
#define TIZOSALMEM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>
#include "OMX_Types.h"

  /*@only@*/ /*@null@*/ /*@out@*/
  OMX_PTR tiz_mem_alloc (size_t size);

  void tiz_mem_free ( /*@only@*/ /*@out@*/ /*@null@*/ OMX_PTR addr);

  /*@only@*/ /*@null@*/ /*@out@*/
  OMX_PTR tiz_mem_realloc (     /*@only@*/ /*@out@*/ /*@null@*/
                            OMX_PTR ptr, size_t size);

  /*@only@*/ /*@null@*/ /*@out@*/
  OMX_PTR tiz_mem_calloc (size_t nmemb, size_t size);

  OMX_PTR tiz_mem_set (OMX_PTR ap_dest, OMX_S32 a_orig, size_t a_num_bytes);

#ifdef __cplusplus
}
#endif

#endif                          /* TIZOSALMEM_H */
