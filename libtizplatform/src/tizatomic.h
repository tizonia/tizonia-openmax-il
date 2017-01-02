/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   tizatomic.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - Atomic operations
 *
 *
 */

#ifndef TIZATOMIC_H
#define TIZATOMIC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tizatomic Atomic operations
 *
 * Atomic operations. This module is work-in-progress.
 *
 * @ingroup libtizplatform
 */

#include <OMX_Core.h>
#include <OMX_Types.h>

/**
 * Atomic variable opaque handle.
 * @ingroup tizatomic
 */
typedef struct tiz_atomic_var tiz_atomic_var_t;
typedef /*@null@ */ tiz_atomic_var_t * tiz_atomic_var_ptr_t;

/**
 * Create a new atomic binary variable.
 *
 * @ingroup tizatomic
 * @param app_atomic_var An atomic variable opaque handle to be initialised.
 * @return OMX_ErrorNone if success, OMX_ErrorUndefined otherwise.
 */
OMX_ERRORTYPE
tiz_atomic_var_init (tiz_atomic_var_ptr_t * app_atomic_var);

/**
 * Atomic test and set.
 *
 * @ingroup tizatomic
 * @param app_atomic_var An atomic variable opaque handle.
 * @return OMX_ErrorNone if success, OMX_ErrorUndefined otherwise.
 */
OMX_ERRORTYPE
tiz_atomic_var_test_and_set (tiz_atomic_var_t * ap_atomic_var);

/**
 * Destroy the atomic binary variable.
 * @param app_atomic_var An atomic variable opaque handle.
 * @ingroup tizatomic
 */
void
tiz_atomic_var_destroy (tiz_atomic_var_t * ap_atomic_var);

#ifdef __cplusplus
}
#endif

#endif /* TIZATOMIC_H */
