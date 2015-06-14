/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizkernel_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - kernel class declarations
 *
 *
 */

#ifndef TIZKERNEL_DECLS_H
#define TIZKERNEL_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>

#include <tizrmproxy_c.h>

#include "tizservant_decls.h"

typedef struct tiz_krn_msg_sendcommand tiz_krn_msg_sendcommand_t;

typedef OMX_ERRORTYPE (*tiz_krn_msg_dispatch_sc_f)(
    void *ap_obj, OMX_HANDLETYPE ap_hdl, tiz_krn_msg_sendcommand_t *ap_msg_sc);

typedef enum tiz_krn_msg_class tiz_krn_msg_class_t;
enum tiz_krn_msg_class
{
  ETIZKrnMsgSendCommand = 0,
  ETIZKrnMsgEmptyThisBuffer,
  ETIZKrnMsgFillThisBuffer,
  ETIZKrnMsgCallback,
  ETIZKrnMsgPluggableEvent,
  ETIZKrnMsgMax
};

typedef OMX_ERRORTYPE (*tiz_krn_msg_dispatch_f)(void *ap_obj, OMX_PTR ap_msg);

struct tiz_krn_msg_sendcommand
{
  OMX_COMMANDTYPE cmd;
  OMX_U32 param1;
  OMX_PTR p_cmd_data;
};

typedef struct tiz_krn_msg_emptyfillbuffer tiz_krn_msg_emptyfillbuffer_t;
struct tiz_krn_msg_emptyfillbuffer
{
  OMX_BUFFERHEADERTYPE *p_hdr;
};

typedef struct tiz_krn_msg_callback tiz_krn_msg_callback_t;
struct tiz_krn_msg_callback
{
  OMX_BUFFERHEADERTYPE *p_hdr;
  OMX_U32 pid;
  OMX_DIRTYPE dir;
};

typedef struct tiz_krn_msg_plg_event tiz_krn_msg_plg_event_t;
struct tiz_krn_msg_plg_event
{
  tiz_event_pluggable_t *p_event;
};

typedef struct tiz_krn_msg tiz_krn_msg_t;
struct tiz_krn_msg
{
  OMX_HANDLETYPE p_hdl;
  tiz_krn_msg_class_t class;
  union
  {
    tiz_krn_msg_sendcommand_t sc;
    tiz_krn_msg_emptyfillbuffer_t ef;
    tiz_krn_msg_callback_t cb;
    tiz_krn_msg_plg_event_t pe;
  };
};

typedef struct tiz_krn_msg_str tiz_krn_msg_str_t;
struct tiz_krn_msg_str
{
  tiz_krn_msg_class_t msg;
  OMX_STRING str;
};

typedef struct tiz_krn tiz_krn_t;
struct tiz_krn
{
  /* Object */
  const tiz_srv_t _;
  tiz_vector_t *p_ports_;
  tiz_vector_t *p_ingress_;
  tiz_vector_t *p_egress_;
  OMX_PTR p_cport_;
  OMX_PTR p_proc_;
  bool eos_;
  tiz_rm_t rm_;
  tiz_rm_proxy_callbacks_t rm_cbacks_;
  OMX_PORT_PARAM_TYPE audio_init_;
  OMX_PORT_PARAM_TYPE image_init_;
  OMX_PORT_PARAM_TYPE video_init_;
  OMX_PORT_PARAM_TYPE other_init_;
  OMX_S32 cmd_completion_count_;
  bool accept_use_buffer_notified_;
  bool accept_buffer_exchange_notified_;
  bool may_transition_exe2idle_notified_;
};

OMX_ERRORTYPE
tiz_krn_super_register_port (const void *a_class, const void *ap_obj,
                             OMX_PTR ap_port, const bool ais_config);
tiz_krn_population_status_t tiz_krn_super_get_population_status (
    const void *a_class, const void *ap_obj, const OMX_U32 a_pid,
    OMX_BOOL *ap_may_be_fully_unpopulated);
OMX_ERRORTYPE
tiz_krn_super_select (const void *a_class, const void *ap_obj,
                      const OMX_U32 a_nports, tiz_pd_set_t *p_set);
OMX_ERRORTYPE
tiz_krn_super_claim_buffer (const void *a_class, const void *ap_obj,
                            const OMX_U32 a_port_id, const OMX_U32 a_pos,
                            OMX_BUFFERHEADERTYPE **p_hdr);
OMX_ERRORTYPE
tiz_krn_super_release_buffer (const void *a_class, const void *ap_obj,
                              const OMX_U32 a_port_id,
                              OMX_BUFFERHEADERTYPE *ap_hdr);
void tiz_krn_super_deregister_all_ports (const void *a_class, void *ap_obj);
void tiz_krn_super_reset_tunneled_ports_status (
    const void *a_class, void *ap_obj, const OMX_U32 a_port_status_flag);

typedef struct tiz_krn_class tiz_krn_class_t;
struct tiz_krn_class
{
  /* Class */
  const tiz_srv_class_t _;

  OMX_ERRORTYPE (*register_port)(const void *ap_obj, OMX_PTR ap_port,
                                 const bool ais_config);
  void *(*get_port)(const void *ap_obj, const OMX_U32 a_pid);
  OMX_ERRORTYPE (*find_managing_port)(const void *ap_obj,
                                      const OMX_INDEXTYPE a_index,
                                      const OMX_PTR ap_struct,
                                      OMX_PTR *app_port);
  tiz_krn_population_status_t (*get_population_status)
  (const void *ap_obj, const OMX_U32 a_pid,
   OMX_BOOL *ap_may_be_fully_unpopulated);
  OMX_ERRORTYPE (*select)(const void *p_obj, const OMX_U32 a_nports,
                          /*@out@*/ tiz_pd_set_t *ap_set);
  OMX_ERRORTYPE (*claim_buffer)(const void *ap_obj, const OMX_U32 a_port_id,
                                const OMX_U32 a_pos,
                                OMX_BUFFERHEADERTYPE **p_hdr);
  OMX_ERRORTYPE (*release_buffer)(const void *ap_obj, const OMX_U32 a_port_id,
                                  OMX_BUFFERHEADERTYPE *p_hdr);
  void (*deregister_all_ports)(void *ap_obj);
  void (*reset_tunneled_ports_status)(void *ap_obj,
                                      const OMX_U32 a_port_status_flag);
  bool (*get_restriction_status)(const void *ap_obj,
                                 const tiz_krn_restriction_t a_restriction);
  void (*clear_metadata)(void *ap_obj);
  OMX_ERRORTYPE (*store_metadata)(void *ap_obj, const OMX_CONFIG_METADATAITEMTYPE *ap_meta_item);
  OMX_ERRORTYPE (*SetParameter_internal) (const void *ap_obj,
                                          OMX_HANDLETYPE ap_hdl,
                                          OMX_INDEXTYPE a_index, OMX_PTR ap_struct);
  OMX_ERRORTYPE (*SetConfig_internal) (const void *ap_obj,
                                       OMX_HANDLETYPE ap_hdl,
                                       OMX_INDEXTYPE a_index, OMX_PTR ap_struct);
};

#ifdef __cplusplus
}
#endif

#endif /* TIZKERNEL_DECLS_H */
