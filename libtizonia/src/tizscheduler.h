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

/**
 * @defgroup libtizonia 'libtizonia' : Tizonia's OpenMAX IL component framework
 *
 * This library implements the base OpenMAX IL component infrastructure, which
 * includes support for the standard OpenMAX IL state machine, port management,
 * and buffer processing.
 *
 * @ingroup Tizonia
 */

#include <OMX_Core.h>
#include <OMX_Types.h>
#include <OMX_Component.h>

#include <tizplatform.h>

/**
 * Maximum number of OpenMAX IL ports that may be registered with a Tizonia
 * component.
 * @ingroup libtizonia
 */
#define TIZ_COMP_MAX_PORTS 32

/**
 * Maximum number of roles that may be registered with a Tizonia component.
 * @ingroup libtizonia
 */
#define TIZ_COMP_MAX_ROLES 64

/**
 * The maximum number of types that may be registered with a Tizonia component.
 * @ingroup libtizonia
 */
#define TIZ_COMP_MAX_TYPES 10

/**
 * @brief 'Configuration port' factory function prototype.
 *
 * This function instantiates a 'configuration port' object for a specific
 * component role. The 'configuration' port is a pseudo-port, it does not have
 * an associated OpenMAX IL port index. It is used in libtizonia to encapsulate
 * the logic for the component's OpenMAX IL structures that are not associated
 * to an actual OpenMAX IL port, i.e. OpenMAX IL parameter structures without
 * an 'nIndex' member (e.g. OMX_PARAM_CONTENTURITYPE).
 *
 * @note Tizonia components require exactly one 'configuration port' to be
 * functional.
 *
 * @ingroup libtizonia
 * @param ap_hdl The component's OpenMAX IL handle.
 * @return A pointer to a newly instantiated 'configuration port' object.
 */

typedef OMX_PTR (*tiz_role_config_port_init_f)(OMX_HANDLETYPE ap_hdl);

/**
 * @brief 'port' factory function prototype.
 *
 * This function instantiates an OpenMAX IL 'port' object for a specific
 * component role. Tizonia components require one or more 'port' objects to
 * be functional.
 *
 * @ingroup libtizonia
 * @param ap_hdl The component's OpenMAX IL handle.
 * @return A pointer to a newly instantiated 'port' object.
 */
typedef OMX_PTR (*tiz_role_port_init_f)(OMX_HANDLETYPE ap_hdl);

/**
 * @brief 'processor' factory function prototype.
 *
 * This function instantiates an OpenMAX IL 'processor' object for a specific
 * component role. Tizonia components require exactly one 'processor' object
 * to be functional.
 *
 * @ingroup libtizonia
 * @param ap_hdl The component's OpenMAX IL handle.
 * @return A pointer to a newly instantiated 'processor' object.
 */
typedef OMX_PTR (*tiz_role_proc_init_f)(OMX_HANDLETYPE ap_hdl);

/**
 * @brief OpenMAX IL role registration factory structure (typedef).
 * @ingroup libtizonia
 */
typedef struct tiz_role_factory tiz_role_factory_t;

/**
 * @brief OpenMAX IL role registration factory structure.
 *
 * This structure is used to hold the various factory functions and elements
 * necessary to register a new role within the libtizonia component
 * infrastrucure.
 * @ingroup libtizonia
 */
struct tiz_role_factory
{
  tiz_role_config_port_init_f pf_cport; /**< 'configuration port'
                                           factory function */
  tiz_role_proc_init_f pf_proc;         /**< 'processor' factory function */
  OMX_U32 nports;                       /**< number of ports in this role */
  tiz_role_port_init_f pf_port[TIZ_COMP_MAX_PORTS]; /* list of regular 'port'
                                                       factory functions */
  OMX_U8 role[OMX_MAX_STRINGNAME_SIZE];             /**< the role name */
};

/**
 * @brief 'Pluggable' event structure (typedef).
 * @ingroup libtizonia
 */
typedef struct tiz_event_pluggable tiz_event_pluggable_t;

typedef void (*tiz_event_pluggable_hdlr_f)(OMX_PTR ap_servant,
                                           tiz_event_pluggable_t *ap_event);
/**
 * @brief 'Pluggable' event structure.
 *
 * A 'pluggable' event is a user-defined event that gets queued up in the
 * component's event queue. Once in the component's event loop, the event is
 * then delivered (in the context of the component's thread) to the
 * corresponding servant object ('fsm', 'kernel', or 'processor') for
 * processing.
 *
 * The main use for this type of event is to allow the component's 'processor'
 * the processing of events coming from an external thread from within the
 * component's event loop and thread. Any data received in the external event
 * tipically needs to be dup'ed before enqueueing the pluggable event (to avoid
 * data races).
 *
 * @ingroup libtizonia
 */
struct tiz_event_pluggable
{
  OMX_PTR p_servant; /**< The servant object that will be processing
                        the external event. */
  OMX_PTR p_data;    /* Tipically, a copy of the data received in
                        the external event. */
  tiz_event_pluggable_hdlr_f pf_hdlr; /**< The event handler */
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

typedef OMX_BOOL (*tiz_eglimage_hook_f)(const OMX_HANDLETYPE ap_hdl,
                                        OMX_U32 pid,
                                        OMX_PTR ap_eglimage,
                                        void *ap_args);

typedef struct tiz_eglimage_hook tiz_eglimage_hook_t;
struct tiz_eglimage_hook
{
  OMX_U32 pid;
  /*@null@*/ tiz_eglimage_hook_f pf_egl_validator;
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

/**
 * @brief Instantiate the OpenMAX IL component infrastructure.
 *
 * When this function returns, the OpenMAX IL component handle is fully
 * initialised. However, the component is still not functional until at least
 * one role is registered (see tiz_comp_register_roles, and
 * tiz_comp_register_types).
 *
 * @ingroup libtizonia
 * @note This function must be called during the execution of the component's
 * entry point, usually OMX_ComponentInit.
 *
 * @param ap_hdl The OpenMAX IL handle.
 * @param ap_cname The component name.
 * @return OMX_ErrorNone on success, other OMX_ERRORTYPE on error.
 */
OMX_ERRORTYPE tiz_comp_init (const OMX_HANDLETYPE ap_hdl, const char *ap_cname);

/**
 * @brief Registration of component roles.
 *
 * At least one factory must be registered for the component to be fully
 * functional. At a minimum, this requires the specialisation of the
 * 'processor' class (see tiz_comp_register_types).
 *
 * @ingroup libtizonia
 * @note This function must be called during the execution of the component's
 * entry point, usually OMX_ComponentInit.
 *
 * @param ap_hdl The OpenMAX IL handle.
 * @param ap_role_list A list of role factories.
 * @param a_nroles The number of factories in the list.
 * @return OMX_ErrorNone on success, other OMX_ERRORTYPE on error.
 */
OMX_ERRORTYPE tiz_comp_register_roles (const OMX_HANDLETYPE ap_hdl,
                                       const tiz_role_factory_t *ap_role_list[],
                                       const OMX_U32 a_nroles);

/**
 * @brief Registration of component types (a.k.a. classes).
 *
 * Components need to register at least one additional class, a specialised
 * 'processor' class. Ports may also need to be specialised.
 *
 * @ingroup libtizonia
 * @note This function must be called during the execution of the component's
 * entry point, usually OMX_ComponentInit.
 *
 * @param ap_hdl The OpenMAX IL handle.
 * @param ap_type_list A list of type/class factories.
 * @param a_ntypes The number of factories in the list.
 * @return OMX_ErrorNone on success, other OMX_ERRORTYPE on error.
 */
OMX_ERRORTYPE tiz_comp_register_types (const OMX_HANDLETYPE ap_hdl,
                                       const tiz_type_factory_t *ap_type_list[],
                                       const OMX_U32 a_ntypes);

/**
 * Registration of port buffer allocation hooks.
 *
 * @ingroup libtizonia
 *
 * @param ap_hdl The OpenMAX IL handle.
 * @param ap_new_hooks The new port buffer allocation hooks.
 * @param ap_old_hooks The old allocation hooks (the ones replaced).
 * @return OMX_ErrorNone on success, other OMX_ERRORTYPE on error.
 */
OMX_ERRORTYPE tiz_comp_register_alloc_hooks (
    const OMX_HANDLETYPE ap_hdl, const tiz_alloc_hooks_t *ap_new_hooks,
    tiz_alloc_hooks_t *ap_old_hooks);

/**
 * Registration of the EGL image validation hook.
 *
 * @ingroup libtizonia
 *
 * @param ap_hdl The OpenMAX IL handle.
 * @param ap_hook EGL image validation hook info.
 * @return OMX_ErrorNone on success, other OMX_ERRORTYPE on error.
 */
OMX_ERRORTYPE tiz_comp_register_eglimage_hook (
    const OMX_HANDLETYPE ap_hdl, const tiz_eglimage_hook_t *ap_hook);

/**
 * Queueing of 'pluggable' events.
 *
 * A 'pluggable' event is submitted to the component's event queue using this
 * function. The component's event loop will deliver the event to its handler
 * for processing within the component's thread context.
 *
 * @ingroup libtizonia
 *
 * @param ap_hdl The OpenMAX IL handle.
 * @param ap_event The pluggable event.
 * @return OMX_ErrorNone on success, other OMX_ERRORTYPE on error.
 */
OMX_ERRORTYPE tiz_comp_event_pluggable (const OMX_HANDLETYPE ap_hdl,
                                        tiz_event_pluggable_t *ap_event);
/**
 * Queueing of 'io' events.
 *
 * An 'io' event is submitted to the component's event queue using this
 * function. The component's event loop will deliver the 'io' event to the
 * 'processor' object for processing within the component's thread context.
 *
 * @ingroup libtizonia
 *
 * @param ap_hdl The OpenMAX IL handle.
 * @param ap_ev_io
 * @param ap_arg
 * @param a_id
 * @param a_fd
 * @param a_events
 * @return OMX_ErrorNone on success, other OMX_ERRORTYPE on error.
 */
void tiz_comp_event_io (const OMX_HANDLETYPE ap_hdl, tiz_event_io_t *ap_ev_io,
                        void *ap_arg, const uint32_t a_id, const int a_fd,
                        const int a_events);

/**
 * Queueing of 'timer' events.
 *
 * An 'timer' event is submitted to the component's event queue using this
 * function. The component's event loop will deliver the 'timer' event to the
 * 'processor' object for processing within the component's thread context.
 *
 * @ingroup libtizonia
 *
 * @param ap_hdl The OpenMAX IL handle.
 * @param ap_ev_timer
 * @param ap_arg
 * @param a_id
 * @return OMX_ErrorNone on success, other OMX_ERRORTYPE on error.
 */
void tiz_comp_event_timer (const OMX_HANDLETYPE ap_hdl,
                           tiz_event_timer_t *ap_ev_timer, void *ap_arg,
                           const uint32_t a_id);

/**
 * Queueing of 'stat' events.
 *
 * An 'stat' event is submitted to the component's event queue using this
 * function. The component's event loop will deliver the 'stat' event to the
 * 'processor' object for processing within the component's thread context.
 *
 * @ingroup libtizonia
 *
 * @param ap_hdl The OpenMAX IL handle.
 * @param ap_ev_stat
 * @param ap_arg
 * @param a_id
 * @param a_events
 * @return OMX_ErrorNone on success, other OMX_ERRORTYPE on error.
 */
void tiz_comp_event_stat (const OMX_HANDLETYPE ap_hdl,
                          tiz_event_stat_t *ap_ev_stat, void *ap_arg,
                          const uint32_t a_id, const int a_events);

/* Utility functions */

/**
 * Retrieve the component's 'fsm' servant object.
 * @ingroup libtizonia
 * @param ap_hdl The OpenMAX IL handle.
 * @return The fsm servant.
 */
void *tiz_get_fsm (const OMX_HANDLETYPE ap_hdl);

/**
 * Retrieve the component's 'kernel' servant object.
 * @ingroup libtizonia
 * @param ap_hdl The OpenMAX IL handle.
 * @return The kernel servant.
 */
void *tiz_get_krn (const OMX_HANDLETYPE ap_hdl);

/**
 * Retrieve the component's 'processor' servant object.
 * @ingroup libtizonia
 * @param ap_hdl The OpenMAX IL handle.
 * @return The processor servant.
 */
void *tiz_get_prc (const OMX_HANDLETYPE ap_hdl);

/**
 * Retrieve the component's servant 'scheduler' object.
 * @ingroup libtizonia
 * @param ap_hdl The OpenMAX IL handle.
 * @return The scheduler object.
 */
void *tiz_get_sched (const OMX_HANDLETYPE ap_hdl);

/**
 * Retrieve a component's registered type / class.
 * @ingroup libtizonia
 * @param ap_hdl The OpenMAX IL handle.
 * @return A registered type.
 */
void *tiz_get_type (const OMX_HANDLETYPE ap_hdl, const char *ap_type_name);

#ifdef __cplusplus
}
#endif

#endif /* TIZSCHEDULER_H */
