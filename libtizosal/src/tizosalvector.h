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
 * @file   tizosalvector.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Dynamic array API
 *
 *
 */

#ifndef TIZOSALVECTOR_H
#define TIZOSALVECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

typedef struct tiz_vector tiz_vector_t;

/* debug function to print an item */
typedef void (*tiz_pv_print_item_f)(OMX_PTR ap_data);

/* debug function to dump an item */
typedef void (*tiz_pv_dump_item_f)(OMX_PTR ap_data);

OMX_ERRORTYPE tiz_vector_init (tiz_vector_t **app_vector, size_t a_elem_size);
void tiz_vector_destroy (tiz_vector_t *ap_vector);
OMX_ERRORTYPE tiz_vector_insert (tiz_vector_t *ap_vector, OMX_PTR ap_data,
                                 OMX_S32 a_pos);
OMX_ERRORTYPE tiz_vector_push_back (tiz_vector_t *ap_vector, OMX_PTR ap_data);
void tiz_vector_pop_back (tiz_vector_t *ap_vector);
void tiz_vector_erase (tiz_vector_t *ap_vector, OMX_S32 a_pos, OMX_S32 a_len);
OMX_PTR tiz_vector_at (const tiz_vector_t *ap_vector, OMX_S32 a_pos);
OMX_PTR tiz_vector_front (tiz_vector_t *ap_vector);
OMX_PTR tiz_vector_back (tiz_vector_t *ap_vector);
OMX_S32 tiz_vector_length (const tiz_vector_t *ap_vector);
void tiz_vector_clear (tiz_vector_t *ap_vector);
OMX_PTR tiz_vector_find (const tiz_vector_t *ap_vector, OMX_PTR ap_data);
OMX_ERRORTYPE tiz_vector_append (tiz_vector_t *app_dst,
                                 const tiz_vector_t *app_src);
OMX_ERRORTYPE tiz_vector_print (tiz_vector_t *ap_vector,
                                tiz_pv_print_item_f apf_print);
OMX_ERRORTYPE tiz_vector_dump (tiz_vector_t *ap_vector,
                               tiz_pv_dump_item_f apf_dump);

#ifdef __cplusplus
}
#endif

#endif /* TIZOSALVECTOR_H */
