/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   tizbuffer.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Dynamically re-sizeable buffer of contiguous binary data.
 *
 *
 */

#ifndef TIZBUFFER_H
#define TIZBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* @defgroup tizbuffer Dynamically re-sizeable buffer of contiguous binary data.
*
* Dynamically re-sizeable buffer of contiguous binary data.
*
* @ingroup libtizplatform
*/

#include <OMX_Core.h>
#include <OMX_Types.h>

/**
 * Dynamic buffer object opaque handle.
 * @ingroup tizbuffer
 */
typedef struct tiz_buffer tiz_buffer_t;
typedef /*@null@ */ tiz_buffer_t *tiz_buffer_ptr_t;

/**
 * Create a new dynamic buffer object.
 *
 * @ingroup tizbuffer
 * @param app_buf A dynamic buffer handle to be initialised.
 * @param a_nbytes Initial size of the data store.
 * @return OMX_ErrorNone if success, OMX_ErrorUndefined otherwise.
 */
OMX_ERRORTYPE tiz_buffer_init (/*@null@ */ tiz_buffer_ptr_t *app_buf,
                               const size_t a_nbytes);
/**
 * Destroy a dynamic buffer object.
 *
 * @ingroup tizbuffer
 * @param ap_buf A dynamic buffer handle to be destroyed.
 */
void tiz_buffer_destroy (tiz_buffer_t *ap_buf);

/**
 * Copy data at the back the buffer.
 *
 * @ingroup tizbuffer
 * @param ap_buf The dynamic buffer handle.
 * @param ap_data The data to be stored.
 * @param a_nbytes The number of bytes to store.
 * @return The number of bytes actually stored.
 */
int tiz_buffer_push (tiz_buffer_t *ap_buf, const void *ap_data,
                     const size_t a_nbytes);

/**
 * Retrieve the number of bytes available in the buffer.
 *
 * @ingroup tizbuffer
 * @param ap_buf The dynamic buffer handle.
 * @return The total number of bytes currently available.
 */
int tiz_buffer_available (const tiz_buffer_t *ap_buf);

/**
 * @brief Retrieve the current position in the buffer where data can be read
 * from.
 *
 * If the buffer is empty, i.e. tiz_buffer_available returns zero, the
 * pointer returned is the position of the start of the buffer.
 *
 * @ingroup tizbuffer
 * @param ap_buf The dynamic buffer handle.
 * @return The pointer to the current position in the buffer.
 */
void *tiz_buffer_get (const tiz_buffer_t *ap_buf);

/**
 * @brief Advance the current position in the buffer.
 *
 * @ingroup tizbuffer
 * @param ap_buf The dynamic buffer handle.
 * @param nbytes The number of bytes to increment the position marker by.
 * @return The number of bytes actually advanced.
 */
int tiz_buffer_advance (tiz_buffer_t *ap_buf, const int nbytes);

/**
 * @brief Reset the position marker.
 *
 * After this operation, tiz_buffer_available returns zero.
 *
 * @ingroup tizbuffer
 * @param ap_buf The dynamic buffer handle.
 */
void tiz_buffer_clear (tiz_buffer_t *ap_buf);

/* DEPRECATED APIS */

/* @deprecated From v0.2.0. Use tiz_buffer_push instead. Will be removed in
 * v0.3.0 */
int tiz_buffer_store_data (tiz_buffer_t *ap_buf, const void *ap_data,
                           const size_t a_nbytes);

/* @deprecated From v0.2.0. Use tiz_buffer_available instead. Will be removed in
*v0.3.0 */
int tiz_buffer_bytes_available (const tiz_buffer_t *ap_buf);

/* @deprecated From v0.2.0. Use tiz_buffer_get instead. Will be removed in
*v0.3.0 */
void *tiz_buffer_get_data (const tiz_buffer_t *ap_buf);

#ifdef __cplusplus
}
#endif

#endif /* TIZBUFFER_H */
