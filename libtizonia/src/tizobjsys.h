/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   tizobjsys.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Object system lifetime management
 *
 *
 */

#ifndef TIZOBJSYS_H
#define TIZOBJSYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

#include <tizplatform.h>

typedef struct tiz_os tiz_os_t;

typedef void * (*tiz_os_type_init_f) (void * ap_tos, void * ap_hdl);

OMX_ERRORTYPE
tiz_os_init (tiz_os_t ** app_os, const OMX_HANDLETYPE ap_hdl,
             tiz_soa_t * ap_soa);

void
tiz_os_destroy (tiz_os_t * ap_os);

OMX_ERRORTYPE
tiz_os_register_base_types (tiz_os_t * ap_os);

OMX_ERRORTYPE
tiz_os_register_type (tiz_os_t * ap_os, const tiz_os_type_init_f a_type_init_f,
                      const OMX_STRING a_type_name);

void *
tiz_os_get_type (const tiz_os_t * ap_os, const char * a_type_name);

void *
tiz_os_calloc (const tiz_os_t * ap_os, size_t a_size);
void
tiz_os_free (const tiz_os_t * ap_os, void * ap_addr);

#ifdef __cplusplus
}
#endif

#endif /* TIZOBJSYS_H */
