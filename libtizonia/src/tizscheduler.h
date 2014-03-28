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
 * @file   tizscheduler.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Servant scheduler
 *
 *
 */

#ifndef TIZSCHEDULER_H
#define TIZSCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "OMX_Core.h"
#include "OMX_Types.h"
#include "OMX_Component.h"
#include "tizosal.h"

#define TIZ_COMP_MAX_PORTS 32
#define TIZ_COMP_MAX_ROLES 64
#define TIZ_COMP_MAX_TYPES 5

typedef OMX_PTR (*tiz_role_config_port_init_f)(OMX_HANDLETYPE ap_hdl);
typedef OMX_PTR (*tiz_role_port_init_f)(OMX_HANDLETYPE ap_hdl);
typedef OMX_PTR (*tiz_role_proc_init_f)(OMX_HANDLETYPE ap_hdl);

typedef struct tiz_role_factory tiz_role_factory_t;
struct tiz_role_factory
{
  tiz_role_config_port_init_f pf_cport;
  tiz_role_proc_init_f pf_proc;
  OMX_U32 nports;
  tiz_role_port_init_f pf_port[TIZ_COMP_MAX_PORTS];
  OMX_U8 role[OMX_MAX_STRINGNAME_SIZE];
};

typedef struct tiz_event_pluggable tiz_event_pluggable_t;
typedef void (*tiz_event_pluggable_hdlr_f)(OMX_PTR ap_servant,
                                           OMX_HANDLETYPE ap_hdl,
                                           tiz_event_pluggable_t *ap_event);
struct tiz_event_pluggable
{
  OMX_HANDLETYPE p_hdl;
  OMX_PTR p_servant;
  OMX_PTR p_data;
  tiz_event_pluggable_hdlr_f pf_hdlr;
};

typedef OMX_U8 *(*tiz_alloc_hook_f)(OMX_U32 *ap_size, OMX_PTR *app_port_priv,
                                    void *ap_args);

typedef void (*tiz_free_hook_f)(OMX_PTR ap_buf, OMX_PTR /*@null@*/ ap_port_priv,
                                void *ap_args);

typedef struct tiz_alloc_hooks tiz_alloc_hooks_t;
struct tiz_alloc_hooks
{
  OMX_U32 pid;
  /*@null@*/ tiz_alloc_hook_f pf_alloc;
  /*@null@*/ tiz_free_hook_f pf_free;
  /*@null@*/ void *p_args;
};

typedef void *(*tiz_class_init_f)(void *, void *);
typedef void *(*tiz_object_init_f)(void *, void *);
typedef struct tiz_type_factory tiz_type_factory_t;
struct tiz_type_factory
{
  tiz_class_init_f pf_class_init;
  OMX_U8 class_name[OMX_MAX_STRINGNAME_SIZE];
  tiz_object_init_f pf_object_init;
  OMX_U8 object_name[OMX_MAX_STRINGNAME_SIZE];
};

/* Component creation */

OMX_ERRORTYPE tiz_comp_init (const OMX_HANDLETYPE ap_hdl, const char *ap_cname);
OMX_ERRORTYPE tiz_comp_register_roles (const OMX_HANDLETYPE ap_hdl,
                                       const tiz_role_factory_t *ap_role_list[],
                                       const OMX_U32 a_nroles);
OMX_ERRORTYPE tiz_comp_register_alloc_hooks (
    const OMX_HANDLETYPE ap_hdl, const tiz_alloc_hooks_t *ap_new_hooks,
    tiz_alloc_hooks_t *ap_old_hooks);
OMX_ERRORTYPE tiz_comp_register_types (const OMX_HANDLETYPE ap_hdl,
                                       const tiz_type_factory_t *ap_type_list[],
                                       const OMX_U32 a_ntypes);
OMX_ERRORTYPE tiz_comp_event_pluggable (const OMX_HANDLETYPE ap_hdl,
                                        tiz_event_pluggable_t *ap_event);
void tiz_comp_event_io (OMX_HANDLETYPE ap_hdl, tiz_event_io_t *ap_ev_io,
                        int a_fd, int a_events);
void tiz_comp_event_timer (OMX_HANDLETYPE ap_hdl,
                           tiz_event_timer_t *ap_ev_timer, void *ap_arg);
void tiz_comp_event_stat (OMX_HANDLETYPE ap_hdl, tiz_event_stat_t *ap_ev_stat,
                          int a_events);

/* Utility functions */

void *tiz_get_fsm (const OMX_HANDLETYPE ap_hdl);
void *tiz_get_krn (const OMX_HANDLETYPE ap_hdl);
void *tiz_get_prc (const OMX_HANDLETYPE ap_hdl);
void *tiz_get_sched (const OMX_HANDLETYPE ap_hdl);
void *tiz_get_type (const OMX_HANDLETYPE ap_hdl, const char *ap_type_name);

#ifdef __cplusplus
}
#endif

#endif /* TIZSCHEDULER_H */
