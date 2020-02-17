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
 * @file   tizshufflelst.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - Integer list shuffling
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <time.h>
#include <stdlib.h>

#include "tizplatform.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.shufflelst"
#endif

struct tiz_shuffle_lst
{
  OMX_S32 * p_lst;
  size_t length;
  OMX_S32 current_index;
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
shuffle_lst (OMX_S32 * ap_array, OMX_S32 n)
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
init_lst (tiz_shuffle_lst_t * ap_shuffle_lst)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  assert (ap_shuffle_lst);
  ap_shuffle_lst->p_lst
    = tiz_mem_alloc (ap_shuffle_lst->length * sizeof (OMX_S32));
  if (ap_shuffle_lst->p_lst)
    {
      OMX_S32 i = 0;
      rc = OMX_ErrorNone;
      for (i = 0; i < ap_shuffle_lst->length; ++i)
        {
          ap_shuffle_lst->p_lst[i] = i;
        }
    }
  return rc;
}

static void
destroy_lst (tiz_shuffle_lst_t * ap_shuffle_lst)
{
  if (ap_shuffle_lst)
    {
      tiz_mem_free (ap_shuffle_lst->p_lst);
    }
  tiz_mem_free (ap_shuffle_lst);
}

OMX_ERRORTYPE
tiz_shuffle_lst_init (tiz_shuffle_lst_ptr_t * app_shuffle_lst,
                      const size_t a_list_size)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  tiz_shuffle_lst_t * p_shuffle_lst = NULL;

  assert (app_shuffle_lst);
  assert (a_list_size > 0);

  p_shuffle_lst = tiz_mem_calloc (1, sizeof (tiz_shuffle_lst_t));
  if (p_shuffle_lst)
    {
      p_shuffle_lst->length = a_list_size;
      if (OMX_ErrorNone == (rc = init_lst (p_shuffle_lst)))
        {
          srand (time (NULL));
          shuffle_lst (p_shuffle_lst->p_lst, p_shuffle_lst->length);
        }
    }

  if (OMX_ErrorNone != rc)
    {
      destroy_lst (p_shuffle_lst);
      p_shuffle_lst = NULL;
    }

  *app_shuffle_lst = p_shuffle_lst;
  return rc;
}

OMX_S32
tiz_shuffle_lst_next (tiz_shuffle_lst_t * ap_shuffle_lst)
{
  return tiz_shuffle_lst_jump (ap_shuffle_lst, 1);
}

OMX_S32
tiz_shuffle_lst_prev (tiz_shuffle_lst_t * ap_shuffle_lst)
{
  return tiz_shuffle_lst_jump (ap_shuffle_lst, -1);
}

OMX_S32
tiz_shuffle_lst_jump (tiz_shuffle_lst_t * ap_shuffle_lst, const OMX_S32 a_jump)
{
  OMX_S32 new_index = 0;
  assert (ap_shuffle_lst);
  assert (ap_shuffle_lst->p_lst);
  new_index = ap_shuffle_lst->current_index + a_jump;
  if (new_index >= ap_shuffle_lst->length)
    {
      new_index %= ap_shuffle_lst->length;
    }
  else if (new_index < 0)
    {
      new_index = ap_shuffle_lst->length - labs (new_index);
    }
  ap_shuffle_lst->current_index = new_index;
  assert (new_index >= 0 && new_index < ap_shuffle_lst->length);
  return ap_shuffle_lst->p_lst[new_index];
}

void
tiz_shuffle_lst_destroy (tiz_shuffle_lst_t * ap_shuffle_lst)
{
  destroy_lst (ap_shuffle_lst);
}
