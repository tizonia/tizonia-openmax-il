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
 * @file   tizosalmem.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL OSAL - Memory management
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.osal.mem"
#endif

/*@only@ */ /*@null@ */ /*@out@ */
OMX_PTR
tiz_mem_alloc (size_t a_size)
{
  return malloc (a_size);
}

void tiz_mem_free (/*@only@ */ /*@out@ */ /*@null@ */ OMX_PTR a_ptr)
{
  free (a_ptr);
}

/*@only@ */ /*@null@ */ /*@out@ */
OMX_PTR
tiz_mem_realloc (/*@only@ */ /*@out@ */ /*@null@ */ OMX_PTR a_ptr,
                 size_t a_size)
{
  return realloc (a_ptr, a_size);
}

/*@only@ */ /*@null@ */ /*@out@ */
OMX_PTR
tiz_mem_calloc (size_t a_num_elem, size_t a_elem_size)
{
  return calloc (a_num_elem, a_elem_size);
}

OMX_PTR
tiz_mem_set (OMX_PTR ap_dest, OMX_S32 a_orig, size_t a_num_bytes)
{
  return memset (ap_dest, (int)a_orig, a_num_bytes);
}
