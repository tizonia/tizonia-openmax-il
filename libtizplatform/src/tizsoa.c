/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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
 * @file   tizsoa.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - Small object allocation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizplatform.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.soa"
#endif

#define SOA_MAX_SLICE_SIZE 256
#define SOA_SLICE_ALIGN 8
#define SOA_CHUNK_SZ 4096

static const int32_t chunk_class_tbl[] = {
  0, 0, 0, 0, 0,                                 /* 32 bytes */
  1, 1, 1, 1,                                    /* 64 bytes */
  2, 2, 2, 2,                                    /* 96 bytes */
  3, 3, 3, 3,                                    /* 128 bytes */
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 /* 256 bytes */
};

static const size_t slice_sz_tbl[TIZ_SOA_NUM_CHUNK_CLASSES]
  = {32, 64, 96, 128, 256};

typedef struct chunk chunk_t;
struct chunk
{
  chunk_t * p_next;
  tiz_soa_t * p_soa;
  int32_t n_allocated_slices;
  int32_t class;
  uint8_t data[SOA_CHUNK_SZ];
};

typedef struct slice slice_t;
struct slice
{
  size_t size;
  chunk_t * p_chunk;
  slice_t * p_next_free;
};
#define SLICE_PREAMBLE_SZ (sizeof (size_t) + sizeof (chunk_t *))

static inline uint8_t *
get_usr_ptr (slice_t * p_slice)
{
  return ((uint8_t *) p_slice + SLICE_PREAMBLE_SZ);
}

static inline slice_t *
get_slice_ptr (void * p_usr)
{
  return ((slice_t *) ((uint8_t *) p_usr - SLICE_PREAMBLE_SZ));
}

struct tiz_soa
{
  slice_t * p_slice_store[TIZ_SOA_NUM_CHUNK_CLASSES];
  slice_t * p_pending_chain;
  chunk_t * p_chunk_lst;
  int32_t n_chunks;
  int32_t n_allocated_objects;
};

/*@null@*/ static slice_t *
alloc_chunk (tiz_soa_t * p_soa, int32_t chunk_class)
{
  slice_t * p_slice = NULL;
  int32_t num_slices = 0;
  size_t slice_sz = 0;
  chunk_t * p_new_chunk = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "chunk_class [%d] ", chunk_class);

  assert (p_soa != NULL);
  assert (chunk_class < TIZ_SOA_NUM_CHUNK_CLASSES);

  slice_sz = slice_sz_tbl[chunk_class];

  if ((p_new_chunk = tiz_mem_calloc (1, sizeof (chunk_t))))
    {
      p_new_chunk->p_soa = p_soa;
      p_new_chunk->p_next = p_soa->p_chunk_lst;
      p_new_chunk->n_allocated_slices = 0;
      p_new_chunk->class = chunk_class;
      p_soa->p_chunk_lst = p_new_chunk;
      p_soa->n_chunks += 1;
      num_slices = SOA_CHUNK_SZ / slice_sz - 2;
      p_soa->p_slice_store[chunk_class] = p_slice
        = (slice_t *) (p_new_chunk->data + slice_sz);
      do
        {
          slice_t * p_next = (slice_t *) ((uint8_t *) p_slice + slice_sz);
          p_slice->p_chunk = p_new_chunk;
          p_slice->size = 0;
          p_slice->p_next_free = p_next;
          p_slice = p_next;
        }
      while (--num_slices != 0);

      p_slice->p_chunk = p_new_chunk;
      p_slice->size = 0;
      p_slice->p_next_free = NULL;
      p_slice = (slice_t *) p_new_chunk->data;
      p_slice->p_chunk = p_new_chunk;
    }

  return p_slice;
}

OMX_ERRORTYPE
tiz_soa_init (/*@null@ */ tiz_soa_ptr_t * app_soa)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_soa_t * p_soa = NULL;

  assert (app_soa);

  if (NULL == (p_soa = tiz_mem_calloc (1, sizeof (tiz_soa_t))))
    {
      rc = OMX_ErrorInsufficientResources;
    }

  *app_soa = p_soa;

  return rc;
}

OMX_ERRORTYPE
tiz_soa_reserve_chunk (tiz_soa_t * p_soa, int32_t chunk_class)
{
  assert (p_soa != NULL);
  assert (chunk_class < TIZ_SOA_NUM_CHUNK_CLASSES);

  return alloc_chunk (p_soa, chunk_class) == NULL
           ? OMX_ErrorInsufficientResources
           : OMX_ErrorNone;
}

void
tiz_soa_destroy (tiz_soa_t * p_soa)
{
  if (p_soa)
    {
      chunk_t * p_chunk = NULL;
      chunk_t * p_next = NULL;

      p_chunk = p_soa->p_chunk_lst;

      while (p_chunk != NULL)
        {
          p_next = p_chunk->p_next;
          tiz_mem_free (p_chunk);
          p_chunk = p_next;
        }

      tiz_mem_free (p_soa);
    }
}

/*@null@*/ void *
tiz_soa_calloc (tiz_soa_t * p_soa, size_t size)
{
  size_t alloc_sz = ((size + SOA_SLICE_ALIGN - 1) & ~(SOA_SLICE_ALIGN - 1))
                    + SLICE_PREAMBLE_SZ;
  uint8_t * p_usr = NULL;

  assert (p_soa);
  assert (alloc_sz > 0);
  assert (alloc_sz <= SOA_MAX_SLICE_SIZE);

  {
    int32_t chunk_class = chunk_class_tbl[alloc_sz / SOA_SLICE_ALIGN];
    slice_t * p_slice = NULL;

    p_slice = p_soa->p_slice_store[chunk_class];

    if (NULL == p_slice)
      {
        p_slice = alloc_chunk (p_soa, chunk_class);
      }
    else
      {
        p_soa->p_slice_store[chunk_class] = p_slice->p_next_free;
      }

    if (p_slice)
      {
        p_slice->p_chunk->n_allocated_slices += 1;
        p_soa->n_allocated_objects += 1;
        p_slice->size = alloc_sz;
        p_usr = get_usr_ptr (p_slice);
        (void) tiz_mem_set (p_usr, 0, size);
      }
  }

  return p_usr;
}

void
tiz_soa_free (tiz_soa_t * p_soa, void * p_addr)
{
  assert (p_soa != NULL);

  if (p_addr)
    {
      slice_t * p_slice = get_slice_ptr (p_addr);

      assert (p_slice != NULL);
      assert (p_slice->size <= SOA_MAX_SLICE_SIZE);
      {
        chunk_t * p_chunk = p_slice->p_chunk;
        int32_t chunk_class = chunk_class_tbl[p_slice->size / SOA_SLICE_ALIGN];

        assert (p_chunk != NULL);

        p_slice->p_chunk->n_allocated_slices -= 1;
        p_soa->n_allocated_objects -= 1;
        p_slice->p_next_free = p_soa->p_slice_store[chunk_class];
        p_soa->p_slice_store[chunk_class] = p_slice;
      }
    }
}

void
tiz_soa_info (tiz_soa_t * p_soa, tiz_soa_info_t * p_info)
{
  int32_t i = 0;
  chunk_t * p_chunk = NULL;

  assert (p_soa != NULL);
  assert (p_info != NULL);

  (void) tiz_mem_set (p_info, 0, sizeof (tiz_soa_info_t));

  p_info->chunks = p_soa->n_chunks;
  p_chunk = p_soa->p_chunk_lst;

  for (i = p_soa->n_chunks; i > 0; --i)
    {
      p_info->slices[p_chunk->class] = p_chunk->n_allocated_slices;
      p_chunk = p_chunk->p_next;
    }

  assert (p_chunk == NULL);

  p_info->chunks = p_soa->n_chunks;
  p_info->objects = p_soa->n_allocated_objects;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "objects [%d] chunks [%d]", p_info->objects,
           p_info->chunks);
}
