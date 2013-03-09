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
 * @file   tizosalmap.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Associative array API
 *
 *
 */

#ifndef TIZOSALMAP_H
#define TIZOSALMAP_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

#include <stdbool.h>

#include <OMX_Core.h>
#include <OMX_Types.h>

#include <tizosalsoa.h>

  typedef struct tiz_map tiz_map_t;

  typedef OMX_S32 (*tiz_map_cmp_f) (OMX_PTR ap_key1, OMX_PTR ap_key2);
  typedef void (*tiz_map_free_f) (OMX_PTR ap_key, OMX_PTR ap_value);
  typedef OMX_S32 (*tiz_map_for_each_f) (OMX_PTR ap_key, OMX_PTR ap_value,
                                         OMX_PTR ap_arg);

  OMX_ERRORTYPE tiz_map_init (tiz_map_t ** app_map, tiz_map_cmp_f a_pf_cmp,
                              tiz_map_free_f a_pf_free, tiz_soa_t * ap_soa);

  void tiz_map_destroy (tiz_map_t * ap_map);

  OMX_ERRORTYPE tiz_map_insert (tiz_map_t * ap_map, OMX_PTR ap_key,
                                OMX_PTR ap_value, OMX_U32 * ap_index);

  OMX_PTR tiz_map_find (const tiz_map_t * ap_map, OMX_PTR ap_key);

  OMX_PTR tiz_map_at (const tiz_map_t * ap_map, OMX_S32 a_pos);

  OMX_ERRORTYPE tiz_map_for_each (tiz_map_t * ap_map,
                                  tiz_map_for_each_f a_pf_for_each,
                                  OMX_PTR ap_arg);

  void tiz_map_erase (tiz_map_t * ap_map, OMX_PTR ap_key);

  OMX_ERRORTYPE tiz_map_clear (tiz_map_t * ap_map);

  bool tiz_map_empty (const tiz_map_t * ap_map);

  OMX_S32 tiz_map_size (const tiz_map_t * ap_map);


#ifdef __cplusplus
}
#endif

#endif                          /* TIZOSALMAP_H */
