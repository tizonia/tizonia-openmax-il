/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   tizbuffer.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - Utility buffer
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "tizmem.h"
#include "tizlog.h"
#include "tizmacros.h"
#include "tizbuffer.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.buffer"
#endif

struct tiz_buffer
{
  unsigned char *p_store;
  int alloc_len;
  int filled_len;
  int offset;
};

static inline void *alloc_data_store (tiz_buffer_t *ap_buf, const size_t nbytes)
{
  assert (ap_buf);
  assert (NULL == ap_buf->p_store);

  if (nbytes > 0)
    {
      ap_buf->p_store = tiz_mem_calloc (1, nbytes);
      if (ap_buf->p_store)
        {
          ap_buf->alloc_len = nbytes;
          ap_buf->filled_len = 0;
          ap_buf->offset = 0;
        }
    }
  return ap_buf->p_store;
}

static inline void dealloc_data_store (
    /*@special@ */ tiz_buffer_t *ap_buf)
/*@releases ap_buf->p_store@ */
/*@ensures isnull ap_buf->p_store@ */
{
  if (ap_buf)
    {
      tiz_mem_free (ap_buf->p_store);
      ap_buf->p_store = NULL;
      ap_buf->alloc_len = 0;
      ap_buf->filled_len = 0;
      ap_buf->offset = 0;
    }
}

OMX_ERRORTYPE
tiz_buffer_init (/*@null@ */ tiz_buffer_ptr_t *app_buf, const size_t a_nbytes)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  tiz_buffer_t *p_buf = NULL;
  void *p_store = NULL;

  assert (app_buf);

  if (NULL == (p_buf = tiz_mem_calloc (1, sizeof(tiz_buffer_t))))
    {
      goto end;
    }

  if (NULL == (p_store = alloc_data_store (p_buf, a_nbytes)))
    {
      goto end;
    }

  /* All allocations succeeded */
  rc = OMX_ErrorNone;

end:

  if (OMX_ErrorNone != rc && p_buf)
    {
      dealloc_data_store (p_buf);
      p_store = NULL;
      tiz_mem_free (p_buf);
      p_buf = NULL;
    }

  *app_buf = p_buf;

  return rc;
}

void tiz_buffer_destroy (tiz_buffer_t *ap_buf)
{
  if (ap_buf)
    {
      dealloc_data_store (ap_buf);
      tiz_mem_free (ap_buf);
    }
}

int tiz_buffer_push (tiz_buffer_t *ap_buf, const void *ap_data,
                     const size_t a_nbytes)
{
  OMX_U32 nbytes_to_copy = 0;

  assert (ap_buf);
  assert (ap_buf->alloc_len >= (ap_buf->offset + ap_buf->filled_len));

  TIZ_LOG (TIZ_PRIORITY_TRACE, "filled_len [%d] offset [%d]",
           ap_buf->filled_len, ap_buf->offset);

  if (ap_data && a_nbytes > 0)
    {
      size_t avail = 0;

      if (ap_buf->offset > 0)
        {
          memmove (ap_buf->p_store, (ap_buf->p_store + ap_buf->offset),
                   ap_buf->filled_len);
          ap_buf->offset = 0;
        }

      avail = ap_buf->alloc_len - (ap_buf->offset + ap_buf->filled_len);

      if (a_nbytes > avail)
        {
          /* need to re-alloc */
          OMX_U8 *p_new_store = NULL;
          size_t need = ap_buf->alloc_len + (a_nbytes - avail);
          p_new_store = tiz_mem_realloc (ap_buf->p_store, need);
          if (p_new_store)
            {
              ap_buf->p_store = p_new_store;
              ap_buf->alloc_len = need;
              TIZ_LOG (TIZ_PRIORITY_TRACE,
                       "Realloc'd data store : "
                       "new size [%d]",
                       ap_buf->alloc_len);
              avail = a_nbytes;
            }
        }
      nbytes_to_copy = MIN (avail, a_nbytes);
      memcpy (ap_buf->p_store + ap_buf->filled_len, ap_data, nbytes_to_copy);
      ap_buf->filled_len += nbytes_to_copy;
      TIZ_LOG (TIZ_PRIORITY_TRACE, "bytes currently stored [%d]",
               ap_buf->filled_len);
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "filled_len [%d] offset [%d]",
           ap_buf->filled_len, ap_buf->offset);

  return nbytes_to_copy;
}

int tiz_buffer_available (const tiz_buffer_t *ap_buf)
{
  assert (ap_buf);
  assert (ap_buf->alloc_len >= (ap_buf->offset + ap_buf->filled_len));
  return ap_buf->filled_len;
}

void *tiz_buffer_get (const tiz_buffer_t *ap_buf)
{
  assert (ap_buf);
  assert (ap_buf->alloc_len >= (ap_buf->offset + ap_buf->filled_len));
  return (ap_buf->p_store + ap_buf->offset);
}

int tiz_buffer_advance (tiz_buffer_t *ap_buf, const int nbytes)
{
  int min_nbytes = 0;
  assert (ap_buf);
  if (nbytes > 0)
    {
      min_nbytes = MIN (nbytes, tiz_buffer_available (ap_buf));
      ap_buf->offset += min_nbytes;
      ap_buf->filled_len -= min_nbytes;
    }
  TIZ_LOG (TIZ_PRIORITY_TRACE,
           "nbytes = [%d] advanced [%d] alloc_len = [%d] "
           "offset = [%d] available = [%d]",
           nbytes, min_nbytes, ap_buf->alloc_len, ap_buf->offset,
           ap_buf->filled_len);
  return min_nbytes;
}

void tiz_buffer_clear (tiz_buffer_t *ap_buf)
{
  if (ap_buf)
    {
      ap_buf->offset = 0;
      ap_buf->filled_len = 0;
    }
}

/* DEPRECATED. For backward compatibility only. Will be removed in v0.3.0. */
int tiz_buffer_store_data (tiz_buffer_t *ap_buf, const void *ap_data,
                           const size_t a_nbytes)
{
  return tiz_buffer_push (ap_buf, ap_data, a_nbytes);
}

/* DEPRECATED. For backward compatibility only. Will be removed in v0.3.0. */
int tiz_buffer_bytes_available (const tiz_buffer_t *ap_buf)
{
  return tiz_buffer_available (ap_buf);
}

/* DEPRECATED. For backward compatibility only. Will be removed in v0.3.0. */
void *tiz_buffer_get_data (const tiz_buffer_t *ap_buf)
{
  return tiz_buffer_get (ap_buf);
}
