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
 * @file   tizfilterprc.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - filter processor class
 *
 *
 */

#ifndef TIZFILTERPRC_H
#define TIZFILTERPRC_H

#ifdef __cplusplus
extern "C" {
#endif

#define TIZ_FILTER_INPUT_PORT_INDEX 0
#define TIZ_FILTER_OUTPUT_PORT_INDEX 1

void *tiz_filter_prc_class_init (void *ap_tos, void *ap_hdl);
void *tiz_filter_prc_init (void *ap_tos, void *ap_hdl);

OMX_BUFFERHEADERTYPE **tiz_filter_prc_get_header_ptr (void *ap_obj, const OMX_U32 a_pid);
OMX_BUFFERHEADERTYPE * tiz_filter_prc_get_header (void *ap_obj, const OMX_U32 a_pid);
bool tiz_filter_prc_headers_available (const void *ap_obj);
OMX_ERRORTYPE tiz_filter_prc_release_header (void *ap_obj, const OMX_U32 a_pid);
OMX_ERRORTYPE tiz_filter_prc_release_all_headers (void *ap_obj);
bool * tiz_filter_prc_get_port_disabled_ptr (void *ap_obj, const OMX_U32 a_pid);
bool tiz_filter_prc_is_eos (const void *ap_obj);
void tiz_filter_prc_update_eos_flag (void *ap_obj, const bool flag);
void tiz_filter_prc_update_port_disabled_flag (void *ap_obj, const OMX_U32 a_pid, const bool flag);

#ifdef __cplusplus
}
#endif

#endif /* TIZFILTERPRC_H */
