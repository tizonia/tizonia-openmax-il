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
 * @file   httpsrcprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - HTTP streaming client processor declarations
 *
 *
 */

#ifndef HTTPSRCPRC_DECLS_H
#define HTTPSRCPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#include <curl/curl.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>

  typedef enum httpsrc_curl_state_id httpsrc_curl_state_id_t;
  enum httpsrc_curl_state_id
  {
    ECurlStateStopped,
    ECurlStateConnecting,
    ECurlStateTransfering,
    ECurlStatePaused
  };

  typedef struct httpsrc_prc httpsrc_prc_t;
  struct httpsrc_prc
  {
    /* Object */
    const tiz_prc_t _;
    OMX_BUFFERHEADERTYPE *p_outhdr_;
    OMX_PARAM_CONTENTURITYPE *p_uri_param_;
    bool eos_;
    bool port_disabled_;
    OMX_S32 audio_coding_type_;
    OMX_U32 num_channels_;
    OMX_U32 samplerate_;
    bool auto_detect_on_;
    tiz_event_io_t *p_ev_io_;
    int sockfd_;
    bool awaiting_io_ev_;
    tiz_event_timer_t *p_ev_curl_timer_;
    bool awaiting_curl_timer_ev_;
    double curl_timeout_;
    tiz_event_timer_t *p_ev_reconnect_timer_;
    bool awaiting_reconnect_timer_ev_;
    double reconnect_timeout_;
    int bitrate_;
    int cache_bytes_;
    tiz_buffer_t *p_store_;
    CURL *p_curl_;              /* curl easy */
    CURLM *p_curl_multi_;        /* curl multi */
    struct curl_slist *p_http_ok_aliases_;
    struct curl_slist *p_http_headers_;
    httpsrc_curl_state_id_t curl_state_;
    unsigned int curl_version_;
    char curl_err[CURL_ERROR_SIZE];
  };

  typedef struct httpsrc_prc_class httpsrc_prc_class_t;
  struct httpsrc_prc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* HTTPSRCPRC_DECLS_H */
