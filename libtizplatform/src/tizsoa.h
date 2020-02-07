/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   tizsoa.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - Small object allocator
 *
 *
 */

#ifndef TIZSOA_H
#define TIZSOA_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>
#include <stdint.h>

#include <OMX_Types.h>
#include <OMX_Core.h>

#define TIZ_SOA_NUM_CHUNK_CLASSES 5

typedef struct tiz_soa tiz_soa_t;
typedef /*@null@ */ tiz_soa_t * tiz_soa_ptr_t;

OMX_ERRORTYPE
tiz_soa_init (/*@null@ */ tiz_soa_ptr_t * app_soa);

void
tiz_soa_destroy (tiz_soa_t * p_soa);

OMX_ERRORTYPE
tiz_soa_reserve_chunk (tiz_soa_t * p_soa, int32_t chunk_class);

/*@null@ */ void *
tiz_soa_calloc (tiz_soa_t * p_soa, size_t a_size);

void
tiz_soa_free (tiz_soa_t * p_soa, void * ap_addr);

typedef struct tiz_soa_info tiz_soa_info_t;
struct tiz_soa_info
{
  /* Total number of chunks allocated */
  int32_t chunks;
  /* Total number of slices currently in use across all chunks */
  int32_t objects;
  /* Number of slices currently in use in each chunk class */
  int32_t slices[TIZ_SOA_NUM_CHUNK_CLASSES];
};

void
tiz_soa_info (tiz_soa_t * p_soa, tiz_soa_info_t * p_info);

#ifdef __cplusplus
}
#endif

#endif /* TIZSOA_H */
