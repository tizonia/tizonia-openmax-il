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
 * @file   tizbuffer.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Utility buffer
 *
 *
 */

#ifndef TIZBUFFER_H
#define TIZBUFFER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

  typedef struct tiz_buffer tiz_buffer_t;
  typedef /*@null@ */ tiz_buffer_t* tiz_buffer_ptr_t;

  OMX_ERRORTYPE tiz_buffer_init (/*@null@ */ tiz_buffer_ptr_t *app_buf,
                                 const size_t a_nbytes);
  void tiz_buffer_destroy (tiz_buffer_t *ap_buf);
  int tiz_buffer_store_data (tiz_buffer_t *ap_buf, const void *ap_data,
                             const size_t  a_nbytes);
  int tiz_buffer_bytes_available (const tiz_buffer_t *ap_buf);
  void * tiz_buffer_get_data (const tiz_buffer_t *ap_buf);
  int tiz_buffer_advance (tiz_buffer_t *ap_buf, const int nbytes);
  void tiz_buffer_clear (tiz_buffer_t *ap_buf);

#ifdef __cplusplus
}
#endif

#endif                          /* TIZBUFFER_H */
