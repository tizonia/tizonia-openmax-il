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
 * @file   tizurltransfer.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief URL file transfer
 *
 *
 */

#ifndef TIZURLTRANSFER_H
#define TIZURLTRANSFER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tizurltransfer A URL file transfer API (based on libcurl) to be
 * used in Tizonia processor objects.
 *
 * A URL file transfer API (based on libcurl) to be used in Tizonia processor
 * objects that need to access files over HTTP or FILE protocols.
 *
 * @ingroup libtizplatform
 */

#include <OMX_Component.h>

typedef struct tiz_urltrans tiz_urltrans_t;
typedef /*@null@ */ tiz_urltrans_t * tiz_urltrans_ptr_t;

/**
 * This callback is invoked when the URL file transfer object has filled a
 * buffer.
 *
 * @param ap_hdr The OpenMAX IL buffer header.
 *
 * @param ap_arg The client data structure.
 */
typedef void (*tiz_urltrans_buffer_filled_f) (OMX_BUFFERHEADERTYPE * ap_hdr,
                                              OMX_PTR ap_arg);
/**
 * This callback is invoked when the URL file transfer object needs another
 * buffer to put data into.
 *
 * @param ap_arg The client data structure.
 *
 * @return An OpenMAX IL buffer header, or NULL if none is available.
 */
typedef OMX_BUFFERHEADERTYPE * (*tiz_urltrans_buffer_emptied_f) (
  OMX_PTR ap_arg);

/**
 * This callback is invoked when an HTTP header becomes available.
 *
 * @param ap_arg The client data structure.
 *
 * @param ap_ptr A pointer to the data received.
 *
 * @param a_nbytes Number of bytes received.
 *
 */
typedef void (*tiz_urltrans_header_available_f) (OMX_PTR ap_arg,
                                                 const void * ap_ptr,
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
typedef bool (*tiz_urltrans_data_available_f) (OMX_PTR ap_arg,
                                               const void * ap_ptr,
                                               const size_t a_nbytes);

/**
 * This callback is invoked when the file transfer is interrupted.
 *
 * @param ap_arg The client data structure.
 *
 * @return true to request automatic reconnection to the same URL. False
 * otherwise.
 *
 */
typedef bool (*tiz_urltrans_connection_lost_f) (OMX_PTR ap_arg);

/**
 * @brief Buffer callbacks registration structure (typedef).
 * @ingroup tizurltransfer
 */
typedef struct tiz_urltrans_buffer_cbacks tiz_urltrans_buffer_cbacks_t;

/**
 * @brief Buffer callbacks registration structure.
 *
 * This structure is used to hold the buffer emptied and filled callback
 * functions.
 * @ingroup tizurltransfer
 */
struct tiz_urltrans_buffer_cbacks
{
  tiz_urltrans_buffer_filled_f pf_buf_filled;
  tiz_urltrans_buffer_emptied_f pf_buf_emptied;
};

/**
*@brief Informational callbacks registration structure (typedef).
*@ingroup tizurltransfer
*/
typedef struct tiz_urltrans_info_cbacks tiz_urltrans_info_cbacks_t;

/**
 * @brief Informational callbacks registration structure.
 *
 * This structure is used to hold various informational callbacks functions.
 * @ingroup tizurltransfer
 */
struct tiz_urltrans_info_cbacks
{
  tiz_urltrans_header_available_f pf_header_avail;
  tiz_urltrans_data_available_f pf_data_avail;
  tiz_urltrans_connection_lost_f pf_connection_lost;
};

/**
 * @brief IO watcher init function (same as in tizservant.h).
 * @ingroup tizurltransfer
 */
typedef OMX_ERRORTYPE (*tiz_urltrans_event_io_init_f) (
  void * ap_obj, tiz_event_io_t ** app_ev_io, int a_fd,
  tiz_event_io_event_t a_event, bool only_once);

/**
*@brief IO watcher start function (same as in tizservant.h).
*@ingroup tizurltransfer
*/
typedef OMX_ERRORTYPE (*tiz_urltrans_event_io_start_f) (
  void * ap_obj, tiz_event_io_t * ap_ev_io);

/**
*@brief IO watcher stop function (same as in tizservant.h).
*@ingroup tizurltransfer
*/
typedef OMX_ERRORTYPE (*tiz_urltrans_event_io_stop_f) (
  void * ap_obj, tiz_event_io_t * ap_ev_io);
/**
*@brief IO watcher destroy function (same as in tizservant.h).
*@ingroup tizurltransfer
*/
typedef void (*tiz_urltrans_event_io_destroy_f) (void * ap_obj,
                                                 tiz_event_io_t * ap_ev_io);

/**
*@brief I/O event callbacks registration structure (typedef).
*@ingroup tizurltransfer
*/
typedef struct tiz_urltrans_event_io_cbacks tiz_urltrans_event_io_cbacks_t;

/**
 * @brief I/O event callbacks registration structure.
 *
 * This structure is used to hold the I/O event callback functions.
 * @ingroup tizurltransfer
 */
struct tiz_urltrans_event_io_cbacks
{
  tiz_urltrans_event_io_init_f pf_io_init;
  tiz_urltrans_event_io_destroy_f pf_io_destroy;
  tiz_urltrans_event_io_start_f pf_io_start;
  tiz_urltrans_event_io_stop_f pf_io_stop;
};

/**
*@brief Timer watcher init function (same as in tizservant.h).
*@ingroup tizurltransfer
*/
typedef OMX_ERRORTYPE (*tiz_urltrans_event_timer_init_f) (
  void * ap_obj, tiz_event_timer_t ** app_ev_timer);

/**
*@brief Timer watcher start function (same as in tizservant.h).
*@ingroup tizurltransfer
*/
typedef OMX_ERRORTYPE (*tiz_urltrans_event_timer_start_f) (
  void * ap_obj, tiz_event_timer_t * ap_ev_timer, const double a_after,
  const double a_repeat);

/**
*@brief Timer watcher stop function (same as in tizservant.h).
*@ingroup tizurltransfer
*/
typedef OMX_ERRORTYPE (*tiz_urltrans_event_timer_stop_f) (
  void * ap_obj, tiz_event_timer_t * ap_ev_timer);

/**
*@brief Timer watcher restart function (same as in tizservant.h).
*@ingroup tizurltransfer
*/
typedef OMX_ERRORTYPE (*tiz_urltrans_event_timer_restart_f) (
  void * ap_obj, tiz_event_timer_t * ap_ev_timer);

/**
*@brief Timer watcher destroy function (same as in tizservant.h).
*@ingroup tizurltransfer
*/
typedef void (*tiz_urltrans_event_timer_destroy_f) (
  void * ap_obj, tiz_event_timer_t * ap_ev_timer);

/**
*@brief Timer event callbacks registration structure (typedef).
*@ingroup tizurltransfer
*/
typedef struct tiz_urltrans_event_timer_cbacks
  tiz_urltrans_event_timer_cbacks_t;

/**
 * @brief Timer event callbacks registration structure.
 *
 * This structure is used to hold the timer event callback functions.
 * @ingroup tizurltransfer
 */
struct tiz_urltrans_event_timer_cbacks
{
  tiz_urltrans_event_timer_init_f pf_timer_init;
  tiz_urltrans_event_timer_destroy_f pf_timer_destroy;
  tiz_urltrans_event_timer_start_f pf_timer_start;
  tiz_urltrans_event_timer_stop_f pf_timer_stop;
  tiz_urltrans_event_timer_restart_f pf_timer_restart;
};

/**
 * Initialize a new URI file transfer object.
 *
 * @param app_trans A reference to URL file transfer object that will be
 * created.
 *
 * @param ap_parent The parent Tizonia processor object.
 *
 * @param ap_uri_param The url to transfer from.
 *
 * @param ap_comp_name The component name.
 *
 * @param a_store_bytes The amount of bytes to use as internal buffer while
 * retrieving data.
 *
 * @param a_reconnect_timeout The number of seconds until a new connection is
 * attempted followin a disconnection.
 *
 * @param a_buffer_cbacks Buffer callbacks registration structure.
 *
 * @param a_info_cbacks Informational callbacks registration structure.
 *
 * @param a_io_cbacks I/O event callbacks registration structure.
 *
 * @param a_timer_cbacks Timer event callbacks registration structure.
 *
 * @return OMX_ErrorNone if success, OMX_ErrorInsufficientResources otherwise.
 */
OMX_ERRORTYPE
tiz_urltrans_init (tiz_urltrans_ptr_t * app_trans, void * ap_parent,
                   OMX_PARAM_CONTENTURITYPE * ap_uri_param,
                   OMX_STRING ap_comp_name, const size_t a_store_bytes,
                   const double a_reconnect_timeout,
                   const tiz_urltrans_buffer_cbacks_t a_buffer_cbacks,
                   const tiz_urltrans_info_cbacks_t a_info_cbacks,
                   const tiz_urltrans_event_io_cbacks_t a_io_cbacks,
                   const tiz_urltrans_event_timer_cbacks_t a_timer_cbacks);

void
tiz_urltrans_destroy (tiz_urltrans_t * ap_trans);

void
tiz_urltrans_set_uri (tiz_urltrans_t * ap_trans,
                      OMX_PARAM_CONTENTURITYPE * ap_uri_param);

void
tiz_urltrans_set_connect_timeout (tiz_urltrans_t * ap_trans,
                                  const long a_connect_timeout);

void
tiz_urltrans_set_internal_buffer_size (tiz_urltrans_t * ap_trans,
                                       const int a_nbytes);

OMX_ERRORTYPE
tiz_urltrans_start (tiz_urltrans_t * ap_trans);

OMX_ERRORTYPE
tiz_urltrans_pause (tiz_urltrans_t * ap_trans);

OMX_ERRORTYPE
tiz_urltrans_unpause (tiz_urltrans_t * ap_trans);

void
tiz_urltrans_cancel (tiz_urltrans_t * ap_trans);

void
tiz_urltrans_flush_buffer (tiz_urltrans_t * ap_trans);

OMX_ERRORTYPE
tiz_urltrans_on_buffers_ready (tiz_urltrans_t * ap_trans);

OMX_ERRORTYPE
tiz_urltrans_on_io_ready (tiz_urltrans_t * ap_trans, tiz_event_io_t * ap_ev_io,
                          int a_fd, int a_events);

OMX_ERRORTYPE
tiz_urltrans_on_timer_ready (tiz_urltrans_t * ap_trans,
                             tiz_event_timer_t * ap_ev_timer);

OMX_U32
tiz_urltrans_bytes_available (tiz_urltrans_t * ap_trans);

bool
tiz_urltrans_handshake_error_found (tiz_urltrans_t * ap_trans);

#ifdef __cplusplus
}
#endif

#endif /* TIZURLTRANSFER_H */
