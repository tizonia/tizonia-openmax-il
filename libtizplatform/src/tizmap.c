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
 * @file   tizmap.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Associative array implementation based on Sam Rushing's AVL tree
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include "tizplatform.h"
#include "avl/avl.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.map"
#endif

/**
 * @defgroup map Associative array
 *
 * Based on the avl tree implementation by Sam Rushing
 * <rushing@nightmare.com>
 *
 * @ingroup libtizplatform
 */

struct tiz_map
{
  avl_tree * p_tree;
  OMX_S32 size;
  tiz_map_cmp_f pf_cmp;
  tiz_map_free_f pf_free;
  tiz_map_for_each_f pf_for_each;
  tiz_soa_t * p_soa;
};

typedef struct tiz_map_item tiz_map_item_t;
struct tiz_map_item
{
  void * p_key;
  void * p_value;
  tiz_map_t * p_map;
};

static /*@null@ */ void *
map_calloc (/*@null@ */ tiz_soa_t * p_soa, size_t a_size)
{
  return p_soa ? tiz_soa_calloc (p_soa, a_size) : tiz_mem_calloc (1, a_size);
}

static inline void
map_free (tiz_soa_t * p_soa, void * ap_addr)
{
  p_soa ? tiz_soa_free (p_soa, ap_addr) : tiz_mem_free (ap_addr);
}

static int
map_compare (void * compare_arg, void * a, void * b)
{
  tiz_map_t * p_map = (tiz_map_t *) compare_arg;
  tiz_map_item_t * p_item1 = (tiz_map_item_t *) a;
  tiz_map_item_t * p_item2 = (tiz_map_item_t *) b;
  return p_map->pf_cmp (p_item1->p_key, p_item2->p_key);
}

static int
map_free_key (void * key)
{
  tiz_map_item_t * p_item = (tiz_map_item_t *) key;
  assert (p_item);
  assert (p_item->p_map);
  assert (p_item->p_map->pf_free);
  p_item->p_map->pf_free (p_item->p_key, p_item->p_value);
  return 0;
}

static int
map_iter_function (void * key, void * iter_arg)
{
  tiz_map_item_t * p_item = (tiz_map_item_t *) key;
  assert (p_item);
  assert (p_item->p_map);
  assert (p_item->p_map->pf_for_each);
  return p_item->p_map->pf_for_each (p_item->p_key, p_item->p_value, iter_arg);
}

static void
map_erase_item (tiz_map_t * ap_map, tiz_map_item_t * ap_item)
{
  assert (ap_map);
  assert (ap_map->p_tree);
  assert (ap_item);

  if (0 == avl_remove_by_key (ap_map->p_tree, (void *) ap_item, map_free_key))
    {
      map_free (ap_map->p_soa, ap_item);
      ap_map->size--;
    }
}

/**
 * Initializes a new empty map.
 *
 * @ingroup map
 *
 * @param a_pf_cmp A comparison function for map keys. This function should
 * return negative, 0, or positive values when a comparison result that is
 * less than, equal or greater than).
 *
 * @param a_pf_free A function to free the key-value pair of a map item.
 *
 * @param ap_soa The Tizonia's small object allocator to allocate from. Or
 * NULL if the Tizonia's default allocation/deallocation routines should be
 * used instead.
 *
 * @return OMX_ErrorNone if success, OMX_ErrorInsufficientResources otherwise
 */
OMX_ERRORTYPE
tiz_map_init (tiz_map_t ** app_map, tiz_map_cmp_f a_pf_cmp,
              tiz_map_free_f a_pf_free, tiz_soa_t * ap_soa)
{
  tiz_map_t * p_map = NULL;

  assert (app_map);
  assert (a_pf_cmp);

  if (!(p_map = (tiz_map_t *) map_calloc (ap_soa, sizeof (tiz_map_t))))
    {
      return OMX_ErrorInsufficientResources;
    }

  if (!(p_map->p_tree = avl_new_avl_tree (map_compare, p_map)))
    {
      map_free (ap_soa, p_map);
      p_map = NULL;
      return OMX_ErrorInsufficientResources;
    }

  p_map->size = 0;
  p_map->pf_cmp = a_pf_cmp;
  p_map->pf_free = a_pf_free;
  p_map->p_soa = ap_soa;

  *app_map = p_map;

  return OMX_ErrorNone;
}

void
tiz_map_destroy (tiz_map_t * p_map)
{
  if (p_map)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Destroying map [%p]", p_map);

      assert (p_map->p_tree);
      assert (p_map->size == 0);

      avl_free_avl_tree (p_map->p_tree, map_free_key);
      map_free (p_map->p_soa, p_map);
    }
}

OMX_ERRORTYPE
tiz_map_insert (tiz_map_t * ap_map, OMX_PTR ap_key, OMX_PTR ap_value,
                OMX_U32 * ap_index)
{
  tiz_map_item_t * p_item = NULL;

  assert (ap_map);
  assert (ap_key);
  assert (ap_map->p_tree);
  assert (ap_index);

  if (!tiz_map_empty (ap_map) && tiz_map_find (ap_map, ap_key))
    {
      return OMX_ErrorBadParameter;
    }

  if (!(p_item = (tiz_map_item_t *) map_calloc (ap_map->p_soa,
                                                sizeof (tiz_map_item_t))))
    {
      return OMX_ErrorInsufficientResources;
    }

  p_item->p_key = ap_key;
  p_item->p_value = ap_value;
  p_item->p_map = ap_map;

  if (-1 == avl_insert_by_key (ap_map->p_tree, p_item, ap_index))
    {
      map_free (ap_map->p_soa, p_item);
      p_item = NULL;
      return OMX_ErrorInsufficientResources;
    }

  ap_map->size++;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Inserted in map. size [%d]", ap_map->size);

  return OMX_ErrorNone;
}

OMX_PTR
tiz_map_find (const tiz_map_t * ap_map, OMX_PTR ap_key)
{
  tiz_map_item_t item;
  tiz_map_item_t * p_item_found = NULL;
  void * pp_itemf = NULL;

  assert (ap_map);
  assert (ap_map->p_tree);
  assert (ap_key);

  pp_itemf = &p_item_found;
  item.p_key = (char *) ap_key;
  item.p_value = NULL;
  item.p_map = (tiz_map_t *) ap_map;

  if (0 == avl_get_item_by_key (ap_map->p_tree, &item, pp_itemf))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Found value");
      return p_item_found->p_value;
    }

  return NULL;
}

OMX_PTR
tiz_map_value_at (const tiz_map_t * ap_map, OMX_S32 a_pos)
{
  tiz_map_item_t * p_item_found = NULL;
  void * pp_itemf = NULL;

  assert (ap_map);
  assert (a_pos < ap_map->size);
  assert (a_pos >= 0);

  pp_itemf = &p_item_found;
  if (0 == avl_get_item_by_index (ap_map->p_tree, a_pos, pp_itemf))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Found value");
      return p_item_found->p_value;
    }

  return NULL;
}

OMX_PTR
tiz_map_key_at (const tiz_map_t * ap_map, OMX_S32 a_pos)
{
  tiz_map_item_t * p_item_found = NULL;
  void * pp_itemf = NULL;

  assert (ap_map);
  assert (a_pos < ap_map->size);
  assert (a_pos >= 0);

  pp_itemf = &p_item_found;
  if (0 == avl_get_item_by_index (ap_map->p_tree, a_pos, pp_itemf))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Found value");
      return p_item_found->p_key;
    }

  return NULL;
}

OMX_ERRORTYPE
tiz_map_for_each (tiz_map_t * ap_map, tiz_map_for_each_f a_pf_for_each,
                  OMX_PTR ap_arg)
{
  int result = 0;

  assert (ap_map);
  assert (ap_map->p_tree);
  assert (a_pf_for_each);

  ap_map->pf_for_each = a_pf_for_each;

  result = avl_iterate_inorder (ap_map->p_tree, map_iter_function, ap_arg);

  return (result == 0 ? OMX_ErrorNone : OMX_ErrorUndefined);
}

void
tiz_map_erase (tiz_map_t * ap_map, OMX_PTR ap_key)
{
  tiz_map_item_t item;
  tiz_map_item_t * p_item_found = NULL;
  void * pp_itemf = NULL;

  assert (ap_map);
  assert (ap_map->p_tree);
  assert (ap_key);

  pp_itemf = &p_item_found;
  item.p_key = (char *) ap_key;
  item.p_value = NULL;
  item.p_map = ap_map;

  if (0 == avl_get_item_by_key (ap_map->p_tree, &item, pp_itemf))
    {
      map_erase_item (ap_map, p_item_found);
    }
}

void
tiz_map_erase_at (tiz_map_t * ap_map, OMX_S32 a_pos)
{
  tiz_map_item_t * p_item_found = NULL;
  void * pp_itemf = NULL;

  assert (ap_map);
  assert (a_pos < ap_map->size);
  assert (a_pos >= 0);

  pp_itemf = &p_item_found;
  if (0 == avl_get_item_by_index (ap_map->p_tree, a_pos, pp_itemf))
    {
      map_erase_item (ap_map, p_item_found);
    }
}

OMX_ERRORTYPE
tiz_map_clear (tiz_map_t * ap_map)
{
  assert (ap_map);
  assert (ap_map->p_tree);

  if (ap_map->size > 0)
    {
      avl_free_avl_tree (ap_map->p_tree, map_free_key);
      ap_map->size = 0;

      if (!(ap_map->p_tree = avl_new_avl_tree (map_compare, ap_map)))
        {
          return OMX_ErrorInsufficientResources;
        }
    }

  return OMX_ErrorNone;
}

bool
tiz_map_empty (const tiz_map_t * ap_map)
{
  assert (ap_map);
  return (ap_map->size == 0 ? true : false);
}

OMX_S32
tiz_map_size (const tiz_map_t * ap_map)
{
  assert (ap_map);
  return ap_map->size;
}
