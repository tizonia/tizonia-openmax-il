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
 * @file   httpsrctrans.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief HTTP source component - http transfer functions
 *
 *
 */

#ifndef HTTPSRCTRANS_H
#define HTTPSRCTRANS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Component.h>

typedef struct httpsrc_trans httpsrc_trans_t;
typedef /*@null@ */ httpsrc_trans_t *httpsrc_trans_ptr_t;

/**
 * This callback is invoked when the OMX buffer is full and is ready to be
 * pushed out of the component.
 *
 * @param ap_hdr The OpenMAX IL buffer header.
 *
 * @param ap_arg The client data structure.
 */
typedef void (*httpsrc_trans_buffer_filled_f)(OMX_BUFFERHEADERTYPE *ap_hdr,
                                              OMX_PTR ap_arg);
/**
 * This callback is invoked when the HTTP transfer helper needs another buffer
 * to put data into.
 *
 * @param ap_arg The client data structure.
 *
 * @return An OpenMAX IL buffer header, or NULL if none is available.
 */
typedef OMX_BUFFERHEADERTYPE *(*httpsrc_trans_buffer_wanted_f)(OMX_PTR ap_arg);

/**
 * This callback is invoked when HTTP header becomes available.
 *
 * @param ap_arg The client data structure.
 *
 * @param ap_ptr A pointer to the data received.
 *
 * @param a_nbytes Number of bytes received.
 *
 */
typedef void (*httpsrc_trans_header_available_f)(OMX_PTR ap_arg,
                                                 const void *ap_ptr,
                                                 const size_t a_nbytes);

/**
 * This callback is invoked when HTTP data becomes available.
 *
 * @param ap_arg The client data structure.
 *
 * @param ap_ptr A pointer to the data received.
 *
 * @param a_nbytes Number of bytes received.
 *
 * @return true if the HTTP transfer needs to be paused, false otherwise.
 *
 */
typedef bool (*httpsrc_trans_data_available_f)(OMX_PTR ap_arg,
                                               const void *ap_ptr,
                                               const size_t a_nbytes);

/**
 * This callback is invoked when the HTTP transfer is interrupted.
 *
 * @param ap_arg The client data structure.
 *
 * @return true to request automatic reconnection to the same URI. False otherwise.
 *
 */
typedef bool (*httpsrc_trans_connection_lost_f)(OMX_PTR ap_arg);

/**
 * Initialize a new HTTP transfer object.
 *
 * @param app_trans A reference to HTTP transfer object that will be created.
 *
 * @param ap_parent The parent processor object.
 *
 * @param ap_uri_param The HTTP uri to transfer from.
 *
 * @param apf_buf_filled The callback to be invoked when an OMX buffer is full.
 *
 * @param apf_buf_wanted The callback to be invoked when an empty OMX buffer is
 * required.
 *
 * @param apf_header_avail The callback invoked when an HTTP header becomes
 * available.
 *
 * @param apf_data_avail The callback invoked when an HTTP data becomes
 * available.
 *
 * @param apf_connection_lost The callback invoked when the HTTP connection gets
 * interrupted.
 *
 * @return OMX_ErrorNone if success, OMX_ErrorInsufficientResources otherwise.
 */
OMX_ERRORTYPE httpsrc_trans_init (
    httpsrc_trans_ptr_t *app_trans, void *ap_parent,
    OMX_PARAM_CONTENTURITYPE *ap_uri_param,
    httpsrc_trans_buffer_filled_f apf_buf_filled,
    httpsrc_trans_buffer_wanted_f apf_buf_wanted,
    httpsrc_trans_header_available_f apf_header_avail,
    httpsrc_trans_data_available_f apf_data_avail,
    httpsrc_trans_connection_lost_f apf_connection_lost);

void httpsrc_trans_destroy (httpsrc_trans_t *ap_trans);

void httpsrc_trans_set_uri (httpsrc_trans_t *ap_trans,
                            OMX_PARAM_CONTENTURITYPE *ap_uri_param);

void httpsrc_trans_set_internal_buffer_size (httpsrc_trans_t *ap_trans,
                                             const int a_nbytes);

OMX_ERRORTYPE httpsrc_trans_start (httpsrc_trans_t *ap_trans);

OMX_ERRORTYPE httpsrc_trans_pause (httpsrc_trans_t *ap_trans);

OMX_ERRORTYPE httpsrc_trans_unpause (httpsrc_trans_t *ap_trans);

void httpsrc_trans_cancel (httpsrc_trans_t *ap_trans);

void httpsrc_trans_flush_buffer (httpsrc_trans_t *ap_trans);

OMX_ERRORTYPE httpsrc_trans_on_buffers_ready (httpsrc_trans_t *ap_trans);

OMX_ERRORTYPE httpsrc_trans_on_io_ready (httpsrc_trans_t *ap_trans,
                                         tiz_event_io_t *ap_ev_io, int a_fd,
                                         int a_events);

OMX_ERRORTYPE httpsrc_trans_on_timer_ready (httpsrc_trans_t *ap_trans,
                                            tiz_event_timer_t *ap_ev_timer);

#ifdef __cplusplus
}
#endif

#endif /* HTTPSRCTRANS_H */
