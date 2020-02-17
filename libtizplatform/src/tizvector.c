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
 * @file   tizvector.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Dynamic array implementation - A thin wrapper over Troy D. Hanson's
 * utarray (see http://troydhanson.github.com/uthash/utarray.html)
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizplatform.h"
#include "utarray/utarray.h"

#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.vector"
#endif

struct tiz_vector
{
  UT_array * p_uta;
  UT_icd * p_icd;
};

OMX_ERRORTYPE
tiz_vector_init (tiz_vector_t ** app_vector, size_t a_elem_size)
{
  tiz_vector_t * p_vec = NULL;

  assert (app_vector);
  assert (a_elem_size > 0);

  if (NULL
      == (p_vec = (tiz_vector_t *) tiz_mem_calloc (1, sizeof (tiz_vector_t))))
    {
      return OMX_ErrorInsufficientResources;
    }

  if (NULL == (p_vec->p_icd = (UT_icd *) tiz_mem_calloc (1, sizeof (UT_icd))))
    {
      tiz_mem_free (p_vec);
      return OMX_ErrorInsufficientResources;
    }

  p_vec->p_icd->sz = a_elem_size;
  utarray_new (p_vec->p_uta, p_vec->p_icd);
  *app_vector = p_vec;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Initializing vector [%p] with elem size [%d]",
           p_vec, a_elem_size);

  return OMX_ErrorNone;
}

void
tiz_vector_destroy (tiz_vector_t * p_vec)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Destroying vector [%p]", p_vec);
  if (p_vec)
    {
      utarray_free (p_vec->p_uta);
      tiz_mem_free (p_vec->p_icd);
      tiz_mem_free (p_vec);
    }
}

OMX_ERRORTYPE
tiz_vector_insert (tiz_vector_t * p_vec, OMX_PTR ap_data, OMX_S32 a_pos)
{
  assert (p_vec);
  assert (a_pos > 0);
  assert (ap_data);
  utarray_insert (p_vec->p_uta, ap_data, a_pos);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_vector_push_back (tiz_vector_t * p_vec, OMX_PTR ap_data)
{
  assert (p_vec);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "pushing back [%p] in vector [%p]", ap_data,
           p_vec);
  utarray_push_back (p_vec->p_uta, ap_data);
  return OMX_ErrorNone;
}

void
tiz_vector_pop_back (tiz_vector_t * p_vec)
{
  assert (p_vec);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "poping back in vector [%p]", p_vec);
  utarray_pop_back (p_vec->p_uta);

  return;
}

void
tiz_vector_erase (tiz_vector_t * p_vec, OMX_S32 a_pos, OMX_S32 a_len)
{
  assert (p_vec);
  assert (a_pos >= 0);
  assert (a_len >= 0);
  utarray_erase (p_vec->p_uta, a_pos, a_len);
}

OMX_PTR
tiz_vector_at (const tiz_vector_t * p_vec, OMX_S32 a_pos)
{
  assert (p_vec);
  assert (a_pos >= 0);

  return utarray_eltptr (p_vec->p_uta, a_pos);
}

OMX_PTR
tiz_vector_front (tiz_vector_t * p_vec)
{
  assert (p_vec);

  return utarray_front (p_vec->p_uta);
}

OMX_PTR
tiz_vector_back (tiz_vector_t * p_vec)
{
  assert (p_vec);
  return utarray_back (p_vec->p_uta);
}

OMX_S32
tiz_vector_length (const tiz_vector_t * p_vec)
{
  assert (p_vec);
  return utarray_len (p_vec->p_uta);
}

void
tiz_vector_clear (tiz_vector_t * p_vec)
{
  if (p_vec)
    {
      utarray_clear (p_vec->p_uta);
    }
  return;
}

OMX_PTR
tiz_vector_find (const tiz_vector_t * p_vec, const OMX_PTR ap_data)
{
  OMX_PTR p_next = NULL, p_cur = NULL;

  assert (p_vec);
  assert (ap_data);

  for (;;)
    {
      p_next = utarray_next (p_vec->p_uta, p_cur);

      if (NULL == p_next)
        {
          return NULL;
        }

      if (0 == memcmp (ap_data, p_next, p_vec->p_icd->sz))
        {
          return p_next;
        }

      p_cur = p_next;
    }
}

OMX_ERRORTYPE
tiz_vector_append (tiz_vector_t * p_dst, const tiz_vector_t * p_src)
{
  assert (p_dst);
  assert (p_src);
  utarray_concat (p_dst->p_uta, p_src->p_uta);
  return OMX_ErrorNone;
}
