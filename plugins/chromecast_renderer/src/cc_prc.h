/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   cc_prc.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL Chromecast renderer - base processor class
 *
 *
 */

#ifndef CC_PRC_H
#define CC_PRC_H

#ifdef __cplusplus
extern "C" {
#endif

#define CC_INPUT_PORT_INDEX 0
#define CC_OUTPUT_PORT_INDEX 1

void *
cc_prc_class_init (void * ap_tos, void * ap_hdl);
void *
cc_prc_init (void * ap_tos, void * ap_hdl);

const char *
cc_prc_get_next_url (const void * p_obj);

const char *
cc_prc_get_prev_url (const void * p_obj);

const char *
cc_prc_get_current_stream_album_art_url (const void * p_obj);

OMX_ERRORTYPE
cc_prc_store_stream_metadata (const void * p_obj);

OMX_ERRORTYPE
cc_prc_store_stream_metadata_item (const void * p_obj,
                                 const char * ap_header_name,
                                 const char * ap_header_info);

OMX_ERRORTYPE
cc_prc_store_display_title (const void * p_obj, const char * ap_artist,
                            const char * ap_title);

#ifdef __cplusplus
}
#endif

#endif /* CC_PRC_H */
