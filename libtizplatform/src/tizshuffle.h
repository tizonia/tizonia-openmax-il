/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizshuffle.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - Integer list shuffling
 *
 *
 */

#ifndef TIZSHUFFLE_H
#define TIZSHUFFLE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup shuffle Simple implementation of the Knuth-Fisher-Yates shuffle
 * algorithm.
 * @ingroup libtizplatform
 */

#include <OMX_Core.h>
#include <OMX_Types.h>

/**
 * Shuffle list handle
 * @ingroup shuffle
 */
typedef struct tiz_shuffle_lst tiz_shuffle_lst_t;
typedef /*@null@ */ tiz_shuffle_lst_t *tiz_shuffle_lst_ptr_t;

/**
 * Create a new shuffle_lstd list of integers.
 *
 * @ingroup shuffle_lst
 *
 * @return OMX_ErrorNone if success, OMX_ErrorUndefined otherwise.
 */
OMX_ERRORTYPE tiz_shuffle_lst_init (tiz_shuffle_lst_ptr_t *app_shuffle_lst,
                                    const size_t a_list_size);

/**
 * Return the next integer in the list.
 *
 * @ingroup shuffle_lst
 *
 * @return 0 if success, -1 otherwise.
 */
OMX_S32 tiz_shuffle_lst_next (tiz_shuffle_lst_t *ap_shuffle_lst);

/**
* Return the prev integer in the list.
*
* @ingroup shuffle_lst
*
* @return 0 if success, -1 otherwise.
*/
OMX_S32 tiz_shuffle_lst_prev (tiz_shuffle_lst_t *ap_shuffle_lst);

/**
* Return the prev integer in the list.
*
* @ingroup shuffle_lst
*
* @return 0 if success, -1 otherwise.
*/
OMX_S32 tiz_shuffle_lst_jump (tiz_shuffle_lst_t *ap_shuffle_lst,
                              const OMX_S32 a_jump);

/**
 * Terminate the calling shuffle_lst.
 *
 * @ingroup shuffle_lst
 */
void tiz_shuffle_lst_destroy (tiz_shuffle_lst_t *p_shuffle_lst);

#ifdef __cplusplus
}
#endif

#endif /* TIZSHUFFLE_H */
