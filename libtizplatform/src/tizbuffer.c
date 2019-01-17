/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
#include <limits.h>

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
  unsigned char * p_store;
  int alloc_len;
  int filled_len;
  int offset;
  int seek_mode;
};

static long
abs_of (const long v)
{
  const int mask = v >> (sizeof (long) * CHAR_BIT - 1);
  return (v + mask) ^ mask;
}

static inline void *
alloc_data_store (tiz_buffer_t * ap_buf, const size_t nbytes)
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
          ap_buf->seek_mode = TIZ_BUFFER_NON_SEEKABLE;
        }
    }
  return ap_buf->p_store;
}

static inline void
dealloc_data_store (
  /*@special@ */ tiz_buffer_t * ap_buf)
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
      ap_buf->seek_mode = TIZ_BUFFER_NON_SEEKABLE;
    }
}

OMX_ERRORTYPE
tiz_buffer_init (/*@null@ */ tiz_buffer_ptr_t * app_buf, const size_t a_nbytes)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  tiz_buffer_t * p_buf = NULL;
  void * p_store = NULL;

  assert (app_buf);

  if (!(p_buf = tiz_mem_calloc (1, sizeof (tiz_buffer_t))))
    {
      goto end;
    }

  if (!(p_store = alloc_data_store (p_buf, a_nbytes)))
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

void
tiz_buffer_destroy (tiz_buffer_t * ap_buf)
{
  if (ap_buf)
    {
      dealloc_data_store (ap_buf);
      tiz_mem_free (ap_buf);
    }
}

int
tiz_buffer_seek_mode (tiz_buffer_t * ap_buf, const int a_seek_mode)
{
  int old_val = -1;
  if (a_seek_mode == TIZ_BUFFER_SEEKABLE
      || a_seek_mode == TIZ_BUFFER_NON_SEEKABLE)
    {
      assert (ap_buf);
      old_val = ap_buf->seek_mode;
      ap_buf->seek_mode = a_seek_mode;
    }
  return old_val;
}

int
tiz_buffer_push (tiz_buffer_t * ap_buf, const void * ap_data,
                 const size_t a_nbytes)
{
  OMX_U32 nbytes_to_copy = 0;

  assert (ap_buf);
  assert (ap_buf->alloc_len >= (ap_buf->offset + ap_buf->filled_len));

  if (ap_data && a_nbytes > 0)
    {
      size_t avail = 0;

      if (ap_buf->seek_mode == TIZ_BUFFER_NON_SEEKABLE
          && ap_buf->offset > 0)
        {
          memmove (ap_buf->p_store, (ap_buf->p_store + ap_buf->offset),
                   ap_buf->filled_len);
          ap_buf->offset = 0;
        }

      avail = ap_buf->alloc_len - (ap_buf->offset + ap_buf->filled_len);

      if (a_nbytes > avail)
        {
          /* need to re-alloc */
          OMX_U8 * p_new_store = NULL;
          size_t need = ap_buf->alloc_len * 2;
          p_new_store = tiz_mem_realloc (ap_buf->p_store, need);
          if (p_new_store)
            {
              ap_buf->p_store = p_new_store;
              ap_buf->alloc_len = need;
              avail = ap_buf->alloc_len - (ap_buf->offset + ap_buf->filled_len);
            }
        }
      nbytes_to_copy = MIN (avail, a_nbytes);
      memcpy (ap_buf->p_store + ap_buf->offset + ap_buf->filled_len, ap_data, nbytes_to_copy);
      ap_buf->filled_len += nbytes_to_copy;
    }
  return nbytes_to_copy;
}

int
tiz_buffer_available (const tiz_buffer_t * ap_buf)
{
  assert (ap_buf);
  assert (ap_buf->alloc_len >= (ap_buf->offset + ap_buf->filled_len));
  return ap_buf->filled_len;
}

int
tiz_buffer_offset (const tiz_buffer_t * ap_buf)
{
  assert (ap_buf);
  assert (ap_buf->alloc_len >= (ap_buf->offset + ap_buf->filled_len));
  return ap_buf->offset;
}

void *
tiz_buffer_get (const tiz_buffer_t * ap_buf)
{
  assert (ap_buf);
  assert (ap_buf->alloc_len >= (ap_buf->offset + ap_buf->filled_len));
  return (ap_buf->p_store + ap_buf->offset);
}

int
tiz_buffer_advance (tiz_buffer_t * ap_buf, const int nbytes)
{
  int min_nbytes = 0;
  assert (ap_buf);
  if (nbytes > 0)
    {
      min_nbytes = MIN (nbytes, tiz_buffer_available (ap_buf));
      ap_buf->offset += min_nbytes;
      ap_buf->filled_len -= min_nbytes;
    }
  return min_nbytes;
}

int
tiz_buffer_seek (tiz_buffer_t * ap_buf, const long offset, const int whence)
{
  int rc = -1;
  assert (ap_buf);
  assert (ap_buf->alloc_len >= (ap_buf->offset + ap_buf->filled_len));

  int total = ap_buf->offset + ap_buf->filled_len;
  if (whence == TIZ_BUFFER_SEEK_SET)
    {
      ap_buf->offset = MIN (offset, total);
      rc = 0;
    }
  else if (whence == TIZ_BUFFER_SEEK_CUR)
    {
      unsigned int r = abs_of (offset);
      if (offset < 0)
        {
          ap_buf->offset -= MIN (r, ap_buf->offset);
        }
      else
        {
          ap_buf->offset += MIN (r, ap_buf->filled_len);
        }
      rc = 0;
    }
  else if (whence == TIZ_BUFFER_SEEK_END && offset < 0)
    {
      unsigned int r = abs_of (offset);
      ap_buf->offset = total - MIN (r, total);
      rc = 0;
    }
  if (0 == rc)
    {
      ap_buf->filled_len = total - ap_buf->offset;
    }
  assert (total == ap_buf->offset + ap_buf->filled_len);
  assert (ap_buf->alloc_len >= (ap_buf->offset + ap_buf->filled_len));

  return rc;
}

void
tiz_buffer_clear (tiz_buffer_t * ap_buf)
{
  if (ap_buf)
    {
      ap_buf->offset = 0;
      ap_buf->filled_len = 0;
    }
}
