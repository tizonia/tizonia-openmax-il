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

OMX_ERRORTYPE
cc_prc_buffers_ready (const void * ap_prc);

OMX_ERRORTYPE
cc_prc_pause (const void * ap_prc);

OMX_ERRORTYPE
cc_prc_resume (const void * ap_prc);

/* OMX_BUFFERHEADERTYPE * */
/* cc_prc_get_header (void * ap_obj, const OMX_U32 a_pid); */

#ifdef __cplusplus
}
#endif

#endif /* CC_PRC_H */
