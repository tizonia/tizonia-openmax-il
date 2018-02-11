/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   tizshufflelst.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - Integer list shuffling
 *
 *
 */

#ifndef TIZSHUFFLELST_H
#define TIZSHUFFLELST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tizshufflelst Simple implementation of the Knuth-Fisher-Yates shuffle algorithm.
 *
 * Simple implementation of the Knuth-Fisher-Yates shuffle algorithm.
 *
 * @ingroup libtizplatform
 */

#include <OMX_Core.h>
#include <OMX_Types.h>

/**
 * Shuffle list opaque handle.
 * @ingroup tizshufflelst
 */
typedef struct tiz_shuffle_lst tiz_shuffle_lst_t;
typedef /*@null@ */ tiz_shuffle_lst_t * tiz_shuffle_lst_ptr_t;

/**
 * Create a new shuffled list of integers.
 *
 * @ingroup tizshufflelst
 *
 * @return OMX_ErrorNone if success, OMX_ErrorUndefined otherwise.
 */
OMX_ERRORTYPE
tiz_shuffle_lst_init (tiz_shuffle_lst_ptr_t * app_shuffle_lst,
                      const size_t a_list_size);

/**
 * Return the next integer in the list.
 *
 * @ingroup tizshufflelst
 *
 * @return 0 if success, -1 otherwise.
 */
OMX_S32
tiz_shuffle_lst_next (tiz_shuffle_lst_t * ap_shuffle_lst);

/**
* Return the prev integer in the list.
*
* @ingroup tizshufflelst
*
* @return 0 if success, -1 otherwise.
*/
OMX_S32
tiz_shuffle_lst_prev (tiz_shuffle_lst_t * ap_shuffle_lst);

/**
* Return the prev integer in the list.
*
* @ingroup tizshufflelst
*
* @return 0 if success, -1 otherwise.
*/
OMX_S32
tiz_shuffle_lst_jump (tiz_shuffle_lst_t * ap_shuffle_lst, const OMX_S32 a_jump);

/**
 * Destroy the shuffled list object.
 *
 * @ingroup tizshufflelst
 */
void
tiz_shuffle_lst_destroy (tiz_shuffle_lst_t * p_shuffle_lst);

#ifdef __cplusplus
}
#endif

#endif /* TIZSHUFFLELST_H */
