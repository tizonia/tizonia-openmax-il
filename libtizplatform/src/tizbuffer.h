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

/* The possibilities for the third argument to 'tiz_buffer_seek_mode'.
   These values should not be changed.  */
#define TIZ_BUFFER_NON_SEEKABLE \
  0 /** During push operations, data behind the current position marker gets
        automatically discarded. This is the default mode of operation. */
#define TIZ_BUFFER_SEEKABLE \
  1 /** Data pushed on to the buffer is only discarded explicitely when
        'tiz_clear_buffer' is used. */

/* The possibilities for the third argument to 'tiz_buffer_seek'.
   These values should not be changed.  */
#define TIZ_BUFFER_SEEK_SET 0 /** Seek from beginning of buffer.  */
#define TIZ_BUFFER_SEEK_CUR 1 /** Seek from current position.  */
#define TIZ_BUFFER_SEEK_END 2 /** Seek from end of data.  */

/**
 * Dynamic buffer object opaque handle.
 * @ingroup tizbuffer
 */
typedef struct tiz_buffer tiz_buffer_t;
typedef /*@null@ */ tiz_buffer_t * tiz_buffer_ptr_t;

/**
 * Create a new dynamic buffer object.
 *
 * @ingroup tizbuffer
 * @param app_buf A dynamic buffer handle to be initialised.
 * @param a_nbytes Initial size of the data store.
 * @return OMX_ErrorNone if success, OMX_ErrorUndefined otherwise.
 */
OMX_ERRORTYPE
tiz_buffer_init (/*@null@ */ tiz_buffer_ptr_t * app_buf, const size_t a_nbytes);

/**
 * Destroy a dynamic buffer object.
 *
 * @ingroup tizbuffer
 * @param ap_buf A dynamic buffer handle to be destroyed.
 */
void
tiz_buffer_destroy (tiz_buffer_t * ap_buf);

/**
 * Set a new overwrite mode.
 *
 * @ingroup tizbuffer
 * @param ap_buf The dynamic buffer handle.
 * @param a_seek_mode TIZ_BUFFER_NON_SEEKABLE (default) or
 * TIZ_BUFFER_SEEKABLE.
 * @return The old seek mode, or -1 on error.
 */
int
tiz_buffer_seek_mode (tiz_buffer_t * ap_buf, const int a_seek_mode);

/**
 * Copy data at the back the buffer.
 *
 * @ingroup tizbuffer
 * @param ap_buf The dynamic buffer handle.
 * @param ap_data The data to be stored.
 * @param a_nbytes The number of bytes to store.
 * @return The number of bytes actually stored.
 */
int
tiz_buffer_push (tiz_buffer_t * ap_buf, const void * ap_data,
                 const size_t a_nbytes);

/**
 * @brief Reset the position marker.
 *
 * After this operation, tiz_buffer_available returns zero.
 *
 * @ingroup tizbuffer
 * @param ap_buf The dynamic buffer handle.
 */
void
tiz_buffer_clear (tiz_buffer_t * ap_buf);

/**
 * Retrieve the number of bytes available in the buffer.
 *
 * @ingroup tizbuffer
 * @param ap_buf The dynamic buffer handle.
 * @return The total number of bytes currently available.
 */
int
tiz_buffer_available (const tiz_buffer_t * ap_buf);

/**
 * @brief Retrieve the current position marker.
 *
 * @ingroup tizbuffer
 * @param ap_buf The  buffer handle.
 * @return The offset in bytes that marks the current position in the buffer.
 */
int
tiz_buffer_offset (const tiz_buffer_t * ap_buf);

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
void *
tiz_buffer_get (const tiz_buffer_t * ap_buf);

/**
 * @brief Advance the current position in the buffer.
 *
 * @ingroup tizbuffer
 * @param ap_buf The dynamic buffer handle.
 * @param nbytes The number of bytes to increment the position marker by.
 * @return The number of bytes actually advanced.
 */
int
tiz_buffer_advance (tiz_buffer_t * ap_buf, const int nbytes);

/**
 * @brief Re-position the position marker.
 *
 * The new position, measured in bytes, is obtained by adding offset bytes to
 * the position specified by whence. If whence is set to TIZ_BUFFER_SEEK_SET,
 * TIZ_BUFFER_SEEK_CUR, or TIZ_BUFFER_SEEK_END, the offset is relative to the
 * start of the buffer, the current position indicator, or end-of-data marker,
 * respectively.
 *
 * @ingroup tizbuffer
 * @param ap_buf The dynamic buffer handle.
 * @param a_offset The new position is obtained by adding a_offset bytes to the
 * position specified by a_whence.
 * @param a_whence TIZ_BUFFER_SEEK_SET, TIZ_BUFFER_SEEK_CUR, or
 * TIZ_BUFFER_SEEK_END.
 * @return 0 on success, -1 on error (e.g. the whence argument was not
 * TIZ_BUFFER_SEEK_SET, TIZ_BUFFER_SEEK_END, or TIZ_BUFFER_SEEK_CUR.  Or the
 * resulting buffer offset would be negative).
 */
int
tiz_buffer_seek (tiz_buffer_t * ap_buf, const long a_offset,
                 const int a_whence);

#ifdef __cplusplus
}
#endif

#endif /* TIZBUFFER_H */
