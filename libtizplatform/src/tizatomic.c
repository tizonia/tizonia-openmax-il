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
 * @file   tizatomic.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - Atomic operations
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <time.h>
#include <stdlib.h>

#include <atomic_ops.h>

#include "tizplatform.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.atomic"
#endif

struct tiz_atomic_var
{
  AO_TS_t lock;
};

static OMX_S32
rand_number (const OMX_S32 n)
{
  OMX_S32 limit = RAND_MAX - RAND_MAX % n;
  OMX_S32 rnd;

  do
    {
      rnd = rand ();
    }
  while (rnd >= limit);
  return rnd % n;
}

static void
atomic_var (OMX_S32 * ap_array, OMX_S32 n)
{
  OMX_S32 i = 0;
  OMX_S32 j = 0;
  OMX_S32 tmp = 0;

  assert (ap_array);

  for (i = n - 1; i > 0; --i)
    {
      j = rand_number (i + 1);
      tmp = ap_array[j];
      ap_array[j] = ap_array[i];
      ap_array[i] = tmp;
    }
}

static OMX_ERRORTYPE
init_lst (tiz_atomic_var_t * ap_atomic_var)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  assert (ap_atomic_var);
  ap_atomic_var->p_lst
    = tiz_mem_alloc (ap_atomic_var->length * sizeof (OMX_S32));
  if (ap_atomic_var->p_lst)
    {
      OMX_S32 i = 0;
      rc = OMX_ErrorNone;
      for (i = 0; i < ap_atomic_var->length; ++i)
        {
          ap_atomic_var->p_lst[i] = i;
        }
    }
  return rc;
}

static void
destroy_lst (tiz_atomic_var_t * ap_atomic_var)
{
  if (ap_atomic_var)
    {
      tiz_mem_free (ap_atomic_var->p_lst);
    }
  tiz_mem_free (ap_atomic_var);
}

OMX_ERRORTYPE
tiz_atomic_var_init (tiz_atomic_var_ptr_t * app_atomic_var,
                     const size_t a_list_size)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  tiz_atomic_var_t * p_atomic_var = NULL;

  assert (app_atomic_var);
  assert (a_list_size > 0);

  p_atomic_var = tiz_mem_calloc (1, sizeof (tiz_atomic_var_t));
  if (p_atomic_var)
    {
      p_atomic_var->length = a_list_size;
      if (OMX_ErrorNone == (rc = init_lst (p_atomic_var)))
        {
          srand (time (NULL));
          atomic_var (p_atomic_var->p_lst, p_atomic_var->length);
        }
    }

  if (OMX_ErrorNone != rc)
    {
      destroy_lst (p_atomic_var);
      p_atomic_var = NULL;
    }

  *app_atomic_var = p_atomic_var;
  return rc;
}

void
tiz_atomic_var_destroy (tiz_atomic_var_t * ap_atomic_var)
{
  destroy_lst (ap_atomic_var);
}
