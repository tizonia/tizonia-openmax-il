/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
 * @file   tizscheduler.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Scheduler class
 *
 *
 */


#ifndef TIZSCHEDULER_H
#define TIZSCHEDULER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Core.h"
#include "OMX_Types.h"
#include "OMX_Component.h"
#include "tizosal.h"

  typedef enum tizsched_state tizsched_state_t;

  enum tizsched_state
  {
    ETIZSchedStateStopped = 0,
    ETIZSchedStateStarting,
    ETIZSchedStateStarted,
    /* TODO : Check these two states at run time */
    ETIZSchedStateCompInited,
    ETIZSchedStateRolesRegistered,
  };

  typedef OMX_PTR (*tiz_cport_factory_f) (OMX_HANDLETYPE ap_hdl);
  typedef OMX_PTR (*tiz_port_factory_f) (OMX_HANDLETYPE ap_hdl);
  typedef OMX_PTR (*tiz_proc_factory_f) (OMX_HANDLETYPE ap_hdl);

#define TIZ_MAX_PORTS 32
#define TIZ_MAX_ROLES 64

  typedef struct tiz_role_factory tiz_role_factory_t;

  struct tiz_role_factory
  {
    tiz_cport_factory_f pf_cport;
    tiz_proc_factory_f pf_proc;
    OMX_U32 nports;
    tiz_port_factory_f pf_port[TIZ_MAX_PORTS];
    OMX_U8 role[OMX_MAX_STRINGNAME_SIZE];
  };

  typedef struct tiz_servant tiz_servant_t;

  typedef tiz_servant_t *tiz_servant_list_t;

  struct tiz_servant
  {
    void *p_fsm;
    void *p_ker;
    void *p_prc;
    tiz_role_factory_t **p_role_list;
    OMX_U32 nroles;
    OMX_COMPONENTTYPE *p_hdl;
  };

  typedef enum peer_type peer_type_t;
  enum peer_type
  {
    ETIZSchedPeerTypeIlClient,
    ETIZSchedPeerTypeIlCore,
    ETIZSchedPeerTypeIlComponent,
    ETIZSchedPeerTypeUnknown
  };

  typedef struct peer_info peer_info_t;
  struct peer_info
  {
    peer_type_t type;
    OMX_S32 tid;
    tiz_mutex_t mutex;
    tiz_sem_t sem;
    OMX_HANDLETYPE hdl;         /* TODO: This should be a list of hdls */
    peer_info_t *p_next;
  };

  typedef struct tiz_scheduler tiz_scheduler_t;
  struct tiz_scheduler
  {
    /* TODO: 4096 - this value needs to be set at project configuration time */
    char cname[OMX_MAX_STRINGNAME_SIZE + 4096];
    tiz_thread_t thread;
    OMX_S32 thread_id;
    tiz_mutex_t mutex;
    tiz_sem_t sem;
    tiz_sem_t schedsem;
    tiz_sem_t cbacksem;
    tiz_queue_t *p_queue;
    tiz_soa_t *p_soa;
    OMX_S32 error;
    tiz_servant_t child;
    tizsched_state_t state;
    tiz_servant_list_t *p_servants;
    peer_info_t *p_peers;
    OMX_S32 npeers;
  };

  typedef struct tizevent tizevent_t;

  typedef void (*tiz_pluggable_event_hdlr_f) (OMX_PTR ap_servant,
                                              OMX_HANDLETYPE ap_hdl,
                                              tizevent_t * ap_event);
  struct tizevent
  {
    OMX_HANDLETYPE p_hdl;
    OMX_PTR p_servant;
    OMX_PTR p_data;
    tiz_pluggable_event_hdlr_f pf_hdlr;
  };

  typedef OMX_U8 *(*tiz_port_alloc_hook_f) (OMX_U32 * ap_size,
                                            OMX_PTR * app_port_priv,
                                            void *ap_args);

  typedef void (*tiz_port_free_hook_f) (OMX_PTR ap_buf,
                                        OMX_PTR ap_port_priv, void *ap_args);

  typedef struct tiz_port_alloc_hooks tiz_port_alloc_hooks_t;
  struct tiz_port_alloc_hooks
  {
    tiz_port_alloc_hook_f pf_alloc;
    tiz_port_free_hook_f pf_free;
    void *p_args;
  };

/**
 * This is first function that must be called by a plugin in order to
 * initialize the base component infrastructure. When this function returns,
 * the component has a fsm and kernel objects ready to function.
 *
 * @param ap_hdl The component handle to be initialized
 *
 * @return An OpenMAX IL error
 */
  OMX_ERRORTYPE tiz_init_component (OMX_HANDLETYPE ap_hdl,
                                    const char *ap_cname);

  OMX_ERRORTYPE tiz_register_roles (OMX_HANDLETYPE ap_hdl,
                                    const tiz_role_factory_t * ap_role_list[],
                                    const OMX_U32 a_nroles);

  OMX_ERRORTYPE tiz_receive_pluggable_event (OMX_HANDLETYPE ap_hdl,
                                             tizevent_t * ap_event);

  OMX_ERRORTYPE tiz_register_port_alloc_hooks (OMX_HANDLETYPE ap_hdl,
                                               const OMX_U32 a_pid,
                                               const tiz_port_alloc_hooks_t *
                                               ap_new_hooks,
                                               tiz_port_alloc_hooks_t *
                                               ap_old_hooks);

  void tiz_receive_event_io (OMX_HANDLETYPE ap_hdl, tiz_event_io_t * ap_ev_io,
                             int a_fd, int a_events);

  void tiz_receive_event_timer (OMX_HANDLETYPE ap_hdl,
                                tiz_event_timer_t * ap_ev_timer,
                                void *ap_arg);

  void tiz_receive_event_stat (OMX_HANDLETYPE ap_hdl,
                               tiz_event_stat_t * ap_ev_stat, int a_events);

  /* Utility functions */

  void *tiz_get_fsm (const OMX_HANDLETYPE ap_hdl);

  void *tiz_get_krn (const OMX_HANDLETYPE ap_hdl);

  void *tiz_get_prc (const OMX_HANDLETYPE ap_hdl);

  void *tiz_get_sched (const OMX_HANDLETYPE ap_hdl);


#ifdef __cplusplus
}
#endif

#endif                          /* TIZSCHEDULER_H */
