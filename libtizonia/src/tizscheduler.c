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
 * @file   tizscheduler.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Scheduler
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizscheduler.h"
#include "tizosal.h"
#include "tizutils.h"
#include "tizfsm.h"
#include "tizkernel.h"
#include "tizproc.h"
#include "tizport.h"
#include "OMX_TizoniaExt.h"

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Types.h"

#include <assert.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.scheduler"
#endif

#define SCHED_OMX_DEFAULT_ROLE "default"

#define TIZ_COMP_INIT_MSG(hdl,msg,msgtype)                              \
  do {                                                                  \
    tiz_ret_on_err ( (msg = init_scheduler_message (hdl,                \
                                                    (msgtype))));       \
  } while (0)

#define TIZ_COMP_INIT_MSG_OOM(hdl,msg,msgtype)                          \
  do {                                                                  \
    tiz_ret_val_on_err ( (msg = init_scheduler_message (hdl,            \
                                                        (msgtype))),    \
                         OMX_ErrorInsufficientResources);               \
  } while (0)


typedef enum tiz_sched_state tiz_sched_state_t;
enum tiz_sched_state
  {
    ETIZSchedStateStopped = 0,
    ETIZSchedStateStarting,
    ETIZSchedStateStarted,
    /* TODO : Check these two states at run time */
    ETIZSchedStateCompInited,
    ETIZSchedStateRolesRegistered,
  };

typedef struct tiz_srv_group tiz_srv_group_t;
struct tiz_srv_group
{
  void *p_fsm;
  void *p_ker;
  void *p_prc;
  tiz_role_factory_t **p_role_list;
  OMX_U32 nroles;
  OMX_COMPONENTTYPE *p_hdl;
};

typedef struct tiz_scheduler tiz_scheduler_t;
struct tiz_scheduler
{
  /* TODO: Reconsider the implementation of the buffer for the component's
     name */
  char cname[OMX_MAX_STRINGNAME_SIZE + 4096];
  tiz_thread_t thread;
  OMX_S32 thread_id;
  tiz_mutex_t mutex;
  tiz_sem_t sem;
  tiz_queue_t *p_queue;
  tiz_soa_t *p_soa;
  OMX_S32 error;
  tiz_srv_group_t child;
  tiz_sched_state_t state;
};

typedef enum tiz_sched_msg_class tiz_sched_msg_class_t;
enum tiz_sched_msg_class
  {
    ETIZSchedMsgComponentInit = 0,
    ETIZSchedMsgComponentDeInit,
    ETIZSchedMsgGetComponentVersion,
    ETIZSchedMsgSendCommand,
    ETIZSchedMsgGetParameter,
    ETIZSchedMsgSetParameter,
    ETIZSchedMsgGetConfig,
    ETIZSchedMsgSetConfig,
    ETIZSchedMsgGetExtensionIndex,
    ETIZSchedMsgGetState,
    ETIZSchedMsgComponentTunnelRequest,
    ETIZSchedMsgUseBuffer,
    ETIZSchedMsgAllocateBuffer,
    ETIZSchedMsgFreeBuffer,
    ETIZSchedMsgEmptyThisBuffer,
    ETIZSchedMsgFillThisBuffer,
    ETIZSchedMsgSetCallbacks,
    ETIZSchedMsgUseEGLImage,
    ETIZSchedMsgComponentRoleEnum,
    ETIZSchedMsgPluggableEvent,
    ETIZSchedMsgRegisterRoles,
    ETIZSchedMsgRegisterPortHooks,
    ETIZSchedMsgEvIo,
    ETIZSchedMsgEvTimer,
    ETIZSchedMsgEvStat,
    ETIZSchedMsgMax,
  };

typedef struct tiz_sched_msg_getcomponentversion
tiz_sched_msg_getcomponentversion_t;
struct tiz_sched_msg_getcomponentversion
{
  OMX_STRING p_comp_name;
  OMX_VERSIONTYPE *p_comp_version;
  OMX_VERSIONTYPE *p_spec_version;
  OMX_UUIDTYPE *p_comp_uuid;
};

typedef struct tiz_sched_msg_componentroleenum
tiz_sched_msg_componentroleenum_t;
struct tiz_sched_msg_componentroleenum
{
  OMX_U8 *p_role;
  OMX_U32 index;
};

typedef struct tiz_sched_msg_setget_paramconfig
tiz_sched_msg_setget_paramconfig_t;
struct tiz_sched_msg_setget_paramconfig
{
  OMX_INDEXTYPE index;
  OMX_PTR p_struct;
};

typedef struct tiz_sched_msg_getextindex tiz_sched_msg_getextindex_t;
struct tiz_sched_msg_getextindex
{
  OMX_STRING p_ext_name;
  OMX_INDEXTYPE *p_index;
};

typedef struct tiz_sched_msg_sendcommand tiz_sched_msg_sendcommand_t;
struct tiz_sched_msg_sendcommand
{
  OMX_COMMANDTYPE cmd;
  OMX_U32 param1;
  OMX_PTR p_cmd_data;
};

typedef struct tiz_sched_msg_setcallbacks tiz_sched_msg_setcallbacks_t;
struct tiz_sched_msg_setcallbacks
{
  OMX_CALLBACKTYPE *p_cbacks;
  OMX_PTR p_appdata;
};

typedef struct tiz_sched_msg_getstate tiz_sched_msg_getstate_t;
struct tiz_sched_msg_getstate
{
  OMX_STATETYPE *p_state;
};

typedef struct tiz_sched_msg_usebuffer tiz_sched_msg_usebuffer_t;
struct tiz_sched_msg_usebuffer
{
  OMX_BUFFERHEADERTYPE **pp_hdr;
  OMX_U32 pid;
  OMX_PTR p_app_priv;
  OMX_U32 size;
  OMX_U8 *p_buf;
};

typedef struct tiz_sched_msg_allocbuffer tiz_sched_msg_allocbuffer_t;
struct tiz_sched_msg_allocbuffer
{
  OMX_BUFFERHEADERTYPE **pp_hdr;
  OMX_U32 pid;
  OMX_PTR p_app_priv;
  OMX_U32 size;
};

typedef struct tiz_sched_msg_freebuffer tiz_sched_msg_freebuffer_t;
struct tiz_sched_msg_freebuffer
{
  OMX_U32 pid;
  OMX_BUFFERHEADERTYPE *p_hdr;
};

typedef struct tiz_sched_msg_emptyfillbuffer tiz_sched_msg_emptyfillbuffer_t;
struct tiz_sched_msg_emptyfillbuffer
{
  OMX_BUFFERHEADERTYPE *p_hdr;
};

typedef struct tiz_sched_msg_tunnelrequest tiz_sched_msg_tunnelrequest_t;
struct tiz_sched_msg_tunnelrequest
{
  OMX_U32 pid;
  OMX_HANDLETYPE p_thdl;
  OMX_U32 tpid;
  OMX_TUNNELSETUPTYPE *p_tsetup;
};

typedef struct tiz_sched_msg_plg_event tiz_sched_msg_plg_event_t;
struct tiz_sched_msg_plg_event
{
  tiz_event_pluggable_t *p_event;
};

typedef struct tiz_sched_msg_regroles tiz_sched_msg_regroles_t;
struct tiz_sched_msg_regroles
{
  OMX_U32 nroles;
  const tiz_role_factory_t **p_role_list;
};

typedef struct tiz_sched_msg_regphooks tiz_sched_msg_regphooks_t;
struct tiz_sched_msg_regphooks
{
  const tiz_alloc_hooks_t *p_hooks;
  tiz_alloc_hooks_t *p_old_hooks;
};

typedef struct tiz_sched_msg_ev_io tiz_sched_msg_ev_io_t;
struct tiz_sched_msg_ev_io
{
  tiz_event_io_t * p_ev_io;
  int fd;
  int events;
};

typedef struct tiz_sched_msg_ev_timer tiz_sched_msg_ev_timer_t;
struct tiz_sched_msg_ev_timer
{
  tiz_event_timer_t * p_ev_timer;
  void *p_arg;
};

typedef struct tiz_sched_msg_ev_stat tiz_sched_msg_ev_stat_t;
struct tiz_sched_msg_ev_stat
{
  tiz_event_stat_t * p_ev_stat;
  int events;
};

typedef struct tiz_sched_msg tiz_sched_msg_t;
struct tiz_sched_msg
{
  OMX_HANDLETYPE p_hdl;
  OMX_BOOL will_block;
  OMX_BOOL may_block;
  tiz_sched_msg_class_t class;
  union
  {
    tiz_sched_msg_getcomponentversion_t gcv;
    tiz_sched_msg_componentroleenum_t cre;
    tiz_sched_msg_setget_paramconfig_t sgpc;
    tiz_sched_msg_getextindex_t gei;
    tiz_sched_msg_sendcommand_t scmd;
    tiz_sched_msg_setcallbacks_t scbs;
    tiz_sched_msg_getstate_t gs;
    tiz_sched_msg_usebuffer_t ub;
    tiz_sched_msg_allocbuffer_t ab;
    tiz_sched_msg_freebuffer_t fb;
    tiz_sched_msg_emptyfillbuffer_t efb;
    tiz_sched_msg_tunnelrequest_t tr;
    tiz_sched_msg_plg_event_t pe;
    tiz_sched_msg_regroles_t rr;
    tiz_sched_msg_regphooks_t rph;
    tiz_sched_msg_ev_io_t eio;
    tiz_sched_msg_ev_timer_t etmr;
    tiz_sched_msg_ev_stat_t estat;
  };
};

/* Forward declarations */
static OMX_ERRORTYPE do_init (tiz_scheduler_t *, tiz_sched_state_t *,
                              tiz_sched_msg_t *);
static OMX_ERRORTYPE do_deinit (tiz_scheduler_t *, tiz_sched_state_t *,
                                tiz_sched_msg_t *);
static OMX_ERRORTYPE do_getcv (tiz_scheduler_t *, tiz_sched_state_t *,
                               tiz_sched_msg_t *);
static OMX_ERRORTYPE do_scmd (tiz_scheduler_t *, tiz_sched_state_t *,
                              tiz_sched_msg_t *);
static OMX_ERRORTYPE do_gparam (tiz_scheduler_t *, tiz_sched_state_t *,
                                tiz_sched_msg_t *);
static OMX_ERRORTYPE do_sparam (tiz_scheduler_t *, tiz_sched_state_t *,
                                tiz_sched_msg_t *);
static OMX_ERRORTYPE do_gconfig (tiz_scheduler_t *, tiz_sched_state_t *,
                                 tiz_sched_msg_t *);
static OMX_ERRORTYPE do_sconfig (tiz_scheduler_t *, tiz_sched_state_t *,
                                 tiz_sched_msg_t *);
static OMX_ERRORTYPE do_gei (tiz_scheduler_t *, tiz_sched_state_t *,
                             tiz_sched_msg_t *);
static OMX_ERRORTYPE do_gs (tiz_scheduler_t *, tiz_sched_state_t *,
                            tiz_sched_msg_t *);
static OMX_ERRORTYPE do_tr (tiz_scheduler_t *, tiz_sched_state_t *,
                            tiz_sched_msg_t *);
static OMX_ERRORTYPE do_ub (tiz_scheduler_t *, tiz_sched_state_t *,
                            tiz_sched_msg_t *);
static OMX_ERRORTYPE do_ab (tiz_scheduler_t *, tiz_sched_state_t *,
                            tiz_sched_msg_t *);
static OMX_ERRORTYPE do_fb (tiz_scheduler_t *, tiz_sched_state_t *,
                            tiz_sched_msg_t *);
static OMX_ERRORTYPE do_etb (tiz_scheduler_t *, tiz_sched_state_t *,
                             tiz_sched_msg_t *);
static OMX_ERRORTYPE do_ftb (tiz_scheduler_t *, tiz_sched_state_t *,
                             tiz_sched_msg_t *);
static OMX_ERRORTYPE do_scbs (tiz_scheduler_t *, tiz_sched_state_t *,
                              tiz_sched_msg_t *);
static OMX_ERRORTYPE do_cre (tiz_scheduler_t *, tiz_sched_state_t *,
                             tiz_sched_msg_t *);
static OMX_ERRORTYPE do_plgevt (tiz_scheduler_t *, tiz_sched_state_t *,
                                tiz_sched_msg_t *);
static OMX_ERRORTYPE do_rr (tiz_scheduler_t *, tiz_sched_state_t *,
                            tiz_sched_msg_t *);
static OMX_ERRORTYPE do_rph (tiz_scheduler_t *, tiz_sched_state_t *,
                             tiz_sched_msg_t *);
static OMX_ERRORTYPE do_eio (tiz_scheduler_t *, tiz_sched_state_t *,
                             tiz_sched_msg_t *);
static OMX_ERRORTYPE do_etmr (tiz_scheduler_t *, tiz_sched_state_t *,
                              tiz_sched_msg_t *);
static OMX_ERRORTYPE do_estat (tiz_scheduler_t *, tiz_sched_state_t *,
                               tiz_sched_msg_t *);

static OMX_ERRORTYPE init_servants (tiz_scheduler_t *, tiz_sched_msg_t *);
static OMX_ERRORTYPE deinit_servants (tiz_scheduler_t *, tiz_sched_msg_t *);
static OMX_ERRORTYPE init_and_register_role (tiz_scheduler_t *,
                                             const OMX_U32);
static tiz_scheduler_t *instantiate_scheduler (OMX_HANDLETYPE, const char *);
static OMX_ERRORTYPE start_scheduler (tiz_scheduler_t *);
static void delete_scheduler (tiz_scheduler_t *);


typedef OMX_ERRORTYPE (*tiz_sched_msg_dispatch_f) (tiz_scheduler_t * ap_sched,
                                                   tiz_sched_state_t * ap_state,
                                                   tiz_sched_msg_t * ap_msg);
static const
tiz_sched_msg_dispatch_f tiz_sched_msg_to_fnt_tbl[] = {
  do_init,
  do_deinit,
  do_getcv,
  do_scmd,
  do_gparam,
  do_sparam,
  do_gconfig,
  do_sconfig,
  do_gei,
  do_gs,
  do_tr,
  do_ub,
  do_ab,
  do_fb,
  do_etb,
  do_ftb,
  do_scbs,
  NULL,                         /* ETIZSchedMsgUseEGLImage */
  do_cre,
  do_plgevt,
  do_rr,
  do_rph,
  do_eio,
  do_etmr,
  do_estat,
};

static OMX_BOOL
dispatch_msg (tiz_scheduler_t * ap_sched,
              tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg);

typedef struct tiz_sched_msg_str tiz_sched_msg_str_t;
struct tiz_sched_msg_str
{
  tiz_sched_msg_class_t msg;
  OMX_STRING str;
};

static tiz_sched_msg_str_t tiz_sched_msg_to_str_tbl[] = {
  {ETIZSchedMsgComponentInit, "ETIZSchedMsgComponentInit"},
  {ETIZSchedMsgComponentDeInit, "ETIZSchedMsgComponentDeInit"},
  {ETIZSchedMsgGetComponentVersion, "ETIZSchedMsgGetComponentVersion"},
  {ETIZSchedMsgSendCommand, "ETIZSchedMsgSendCommand"},
  {ETIZSchedMsgGetParameter, "ETIZSchedMsgGetParameter"},
  {ETIZSchedMsgSetParameter, "ETIZSchedMsgSetParameter"},
  {ETIZSchedMsgGetConfig, "ETIZSchedMsgGetConfig"},
  {ETIZSchedMsgSetConfig, "ETIZSchedMsgSetConfig"},
  {ETIZSchedMsgGetExtensionIndex, "ETIZSchedMsgGetExtensionIndex"},
  {ETIZSchedMsgGetState, "ETIZSchedMsgGetState"},
  {ETIZSchedMsgComponentTunnelRequest, "ETIZSchedMsgComponentTunnelRequest"},
  {ETIZSchedMsgUseBuffer, "ETIZSchedMsgUseBuffer"},
  {ETIZSchedMsgAllocateBuffer, "ETIZSchedMsgAllocateBuffer"},
  {ETIZSchedMsgFreeBuffer, "ETIZSchedMsgFreeBuffer"},
  {ETIZSchedMsgEmptyThisBuffer, "ETIZSchedMsgEmptyThisBuffer"},
  {ETIZSchedMsgFillThisBuffer, "ETIZSchedMsgFillThisBuffer"},
  {ETIZSchedMsgSetCallbacks, "ETIZSchedMsgSetCallbacks"},
  {ETIZSchedMsgUseEGLImage, "ETIZSchedMsgUseEGLImage"},
  {ETIZSchedMsgComponentRoleEnum, "ETIZSchedMsgComponentRoleEnum"},
  {ETIZSchedMsgPluggableEvent, "ETIZSchedMsgPluggableEvent"},
  {ETIZSchedMsgRegisterRoles, "ETIZSchedMsgRegisterRoles"},
  {ETIZSchedMsgRegisterPortHooks, "ETIZSchedMsgRegisterPortHooks"},
  {ETIZSchedMsgEvIo,"{ETIZSchedMsgEvIo,"},
  {ETIZSchedMsgEvTimer,"ETIZSchedMsgEvTimer"},
  {ETIZSchedMsgEvStat,"ETIZSchedMsgEvStat"},
  {ETIZSchedMsgMax, "ETIZSchedMsgMax"},
};

static const OMX_STRING
tiz_sched_msg_to_str (const tiz_sched_msg_class_t a_msg)
{
  const OMX_S32 count =
    sizeof (tiz_sched_msg_to_str_tbl) / sizeof (tiz_sched_msg_str_t);
  OMX_S32 i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_sched_msg_to_str_tbl[i].msg == a_msg)
        {
          return tiz_sched_msg_to_str_tbl[i].str;
        }
    }

  return "Unknown scheduler message";
}

static OMX_BOOL tiz_sched_blocking_apis_tbl[] = {
  OMX_TRUE,                     /* ETIZSchedMsgComponentInit */
  OMX_TRUE,                     /* ETIZSchedMsgComponentDeInit */
  OMX_TRUE,                     /* ETIZSchedMsgGetComponentVersion */
  OMX_TRUE,                     /* ETIZSchedMsgSendCommand */
  OMX_TRUE,                     /* ETIZSchedMsgGetParameter */
  OMX_TRUE,                     /* ETIZSchedMsgSetParameter */
  OMX_TRUE,                     /* ETIZSchedMsgGetConfig */
  OMX_FALSE,                    /* ETIZSchedMsgSetConfig */
  OMX_TRUE,                     /* ETIZSchedMsgGetExtensionIndex */
  OMX_TRUE,                     /* ETIZSchedMsgGetState */
  OMX_TRUE,                     /* ETIZSchedMsgComponentTunnelRequest */
  OMX_TRUE,                     /* ETIZSchedMsgUseBuffer */
  OMX_TRUE,                     /* ETIZSchedMsgAllocateBuffer */
  OMX_TRUE,                     /* ETIZSchedMsgFreeBuffer */
#ifdef EFB_FTB_SHOULD_BLOCK
  OMX_TRUE,                     /* ETIZSchedMsgEmptyThisBuffer */
  OMX_TRUE,                     /* ETIZSchedMsgFillThisBuffer */
#else
  OMX_FALSE,                    /* ETIZSchedMsgEmptyThisBuffer */
  OMX_FALSE,                    /* ETIZSchedMsgEmptyThisBuffer */
#endif
  OMX_TRUE,                     /* ETIZSchedMsgSetCallbacks */
  OMX_BOOL_MAX,                 /* ETIZSchedMsgUseEGLImage */
  OMX_TRUE,                     /* ETIZSchedMsgComponentRoleEnum */
  OMX_FALSE,                    /* ETIZSchedMsgPluggableEvent */
  OMX_TRUE,                     /* ETIZSchedMsgRegisterRoles */
  OMX_TRUE,                     /* ETIZSchedMsgRegisterPortHooks */
  OMX_FALSE,                    /* ETIZSchedMsgEvIo */
  OMX_FALSE,                    /* ETIZSchedMsgEvTimer */
  OMX_FALSE,                    /* ETIZSchedMsgEvStat */
  OMX_BOOL_MAX,                 /* ETIZSchedMsgMax */
};

static inline tiz_scheduler_t *
get_sched (const OMX_HANDLETYPE ap_hdl)
{
  assert (NULL != ap_hdl);
  return ((OMX_COMPONENTTYPE *) ap_hdl)->pComponentPrivate;
}

static void
delete_roles (tiz_scheduler_t * ap_sched)
{
  OMX_U32 i = 0;

  assert (NULL != ap_sched);

  for (i = 0; i < ap_sched->child.nroles; ++i)
    {
      tiz_mem_free (ap_sched->child.p_role_list[i]);
    }

  tiz_mem_free (ap_sched->child.p_role_list);

  ap_sched->child.p_role_list = NULL;
  ap_sched->child.nroles = 0;
}

static OMX_ERRORTYPE
store_roles (tiz_scheduler_t * ap_sched,
             const tiz_sched_msg_regroles_t * ap_msg_regroles)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_role_factory_t *p_rf = NULL;
  OMX_U32 i = 0;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg_regroles);

  if (NULL == (ap_sched->child.p_role_list
               = tiz_mem_calloc (ap_msg_regroles->nroles,
                                 sizeof (tiz_role_factory_t *))))
    {
      TIZ_LOGN (TIZ_TRACE, ap_sched->child.p_hdl,
                "[OMX_ErrorInsufficientResources] : list of roles - "
                "Failed when making local copy ..");
      rc = OMX_ErrorInsufficientResources;
    }

  for (i = 0; i < ap_msg_regroles->nroles && rc == OMX_ErrorNone; ++i)
    {
      p_rf = tiz_mem_calloc (1, sizeof (tiz_role_factory_t));
      if (p_rf)
        {
          memcpy (p_rf, ap_msg_regroles->p_role_list[i],
                  sizeof (tiz_role_factory_t));
          ap_sched->child.p_role_list[i] = p_rf;
          ap_sched->child.nroles++;
        }
      else
        {
          rc = OMX_ErrorInsufficientResources;
          TIZ_LOGN (TIZ_TRACE, ap_sched->child.p_hdl,
                    "[OMX_ErrorInsufficientResources] : list of roles - "
                    "Failed when making local copy ..");
        }
    }

  if (OMX_ErrorNone != rc)
    {
      /* Clean-up */
      delete_roles (ap_sched);
    }

  return rc;
}

static inline OMX_ERRORTYPE
send_msg_blocking (tiz_scheduler_t * ap_sched, tiz_sched_msg_t * ap_msg)
{
  assert (NULL != ap_msg);
  assert (NULL != ap_sched);
  ap_msg->will_block = OMX_TRUE;
  tiz_check_omx_err_ret_oom (tiz_queue_send (ap_sched->p_queue, ap_msg));
  tiz_check_omx_err_ret_oom (tiz_sem_wait (&(ap_sched->sem)));
  return ap_sched->error;
}

static inline OMX_ERRORTYPE
send_msg_non_blocking (tiz_scheduler_t * ap_sched, tiz_sched_msg_t * ap_msg)
{
  assert (NULL != ap_msg);
  assert (NULL != ap_sched);
  ap_msg->will_block = OMX_FALSE;
  tiz_check_omx_err_ret_oom (tiz_queue_send (ap_sched->p_queue, ap_msg));
  return ap_sched->error;
}

static inline OMX_ERRORTYPE
send_msg (tiz_scheduler_t * ap_sched, tiz_sched_msg_t * ap_msg)
{
  const OMX_S32 tid = tiz_thread_id ();
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);

  if (tid == ap_sched->thread_id)
    {
      TIZ_LOGN (TIZ_WARN, ap_sched->child.p_hdl,
                "WARNING: (API %s called from IL callback context...)",
                tiz_sched_msg_to_str (ap_msg->class));
      ap_msg->will_block = OMX_FALSE;
      (void) dispatch_msg (ap_sched, &(ap_sched->state), ap_msg);
      rc = ap_sched->error;
    }
  else
    {
      if (OMX_FALSE == ap_msg->will_block)
        {
          rc = send_msg_non_blocking (ap_sched, ap_msg);
        }
      else
        {
          tiz_check_omx_err_ret_oom (tiz_mutex_lock (&(ap_sched->mutex)));
          rc = send_msg_blocking (ap_sched, ap_msg);
          tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&(ap_sched->mutex)));
        }
    }

  return rc;
}

static OMX_ERRORTYPE
do_init (tiz_scheduler_t * ap_sched,
         tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  assert (ap_state && ETIZSchedStateStarting == *ap_state);
  *ap_state = ETIZSchedStateStarted;
  return init_servants (ap_sched, ap_msg);
}

static OMX_ERRORTYPE
do_deinit (tiz_scheduler_t * ap_sched,
           tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  *ap_state = ETIZSchedStateStopped;
  return deinit_servants (ap_sched, ap_msg);
}

static OMX_ERRORTYPE
do_getcv (tiz_scheduler_t * ap_sched,
          tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_getcomponentversion_t *p_msg_getcv = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_getcv = &(ap_msg->gcv);
  assert (NULL != p_msg_getcv);

  return tiz_api_GetComponentVersion (ap_sched->child.p_ker,
                                      ap_msg->p_hdl,
                                      p_msg_getcv->p_comp_name,
                                      p_msg_getcv->p_comp_version,
                                      p_msg_getcv->p_spec_version,
                                      p_msg_getcv->p_comp_uuid);
}

static OMX_ERRORTYPE
do_scmd (tiz_scheduler_t * ap_sched,
         tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_sendcommand_t *p_msg_sc = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_sc = &(ap_msg->scmd);
  assert (NULL != p_msg_sc);

  return tiz_api_SendCommand (ap_sched->child.p_fsm,
                              ap_msg->p_hdl, p_msg_sc->cmd,
                              p_msg_sc->param1, p_msg_sc->p_cmd_data);
}

static OMX_ERRORTYPE
do_gparam (tiz_scheduler_t * ap_sched,
           tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_setget_paramconfig_t *p_msg_gparam = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_gparam = &(ap_msg->sgpc);
  assert (NULL != p_msg_gparam);

  if (OMX_IndexParamStandardComponentRole == p_msg_gparam->index)
    {
      /* IL 1.2 does not mandate support read-access for this index. Only write
       * access is mandated. */
      rc = OMX_ErrorUnsupportedIndex;
    }
  else
    {
      rc = tiz_api_GetParameter (ap_sched->child.p_fsm,
                                 ap_msg->p_hdl,
                                 p_msg_gparam->index, p_msg_gparam->p_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
do_set_component_role (tiz_scheduler_t * ap_sched,
                       const OMX_PARAM_COMPONENTROLETYPE *ap_role)
{
  tiz_fsm_state_id_t now = EStateMax;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_sched);
  now = tiz_fsm_get_substate (ap_sched->child.p_fsm);

  /* Only allow role (re)set if in OMX_StateLoded state */
  if (EStateLoaded != now)
    {
      rc = OMX_ErrorIncorrectStateOperation;
    }
  else 
    {
      OMX_U32 role_pos = 0, nroles = ap_sched->child.nroles;
      OMX_STRING p_str = NULL;

      for (role_pos = 0; role_pos < nroles; ++role_pos)
        {
          if (0 == strncmp ((char *) ap_role->cRole,
                            (const char *) ap_sched->
                            child.p_role_list[role_pos]->role,
                            OMX_MAX_STRINGNAME_SIZE))
            {
              TIZ_LOGN (TIZ_TRACE, ap_sched->child.p_hdl,
                        "Found role [%s]...", p_str);
              break;
            }
        }

      if (role_pos >= nroles)
        {
          /* Check for "default" role */
          if (0 == strncmp ((char *) ap_role->cRole,
                            SCHED_OMX_DEFAULT_ROLE,
                            OMX_MAX_STRINGNAME_SIZE))
            {
              TIZ_LOGN (TIZ_TRACE, ap_sched->child.p_hdl,
                        "Found default role...");
              role_pos = 0;
            }
        }

      if (role_pos < nroles)
        {
          /* Deregister the current role */

          /* First, delete the processor */
          factory_delete (ap_sched->child.p_prc);
          ap_sched->child.p_prc = NULL;

          tiz_krn_deregister_all_ports (ap_sched->child.p_ker);

          /* Populate defaults according to the new role */
          rc = init_and_register_role (ap_sched, role_pos);
        }
      else
        {
          rc = OMX_ErrorBadParameter;
        }
    }

  return rc;
}

static OMX_ERRORTYPE
do_sparam (tiz_scheduler_t * ap_sched,
           tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_setget_paramconfig_t *p_msg_gparam = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_gparam = &(ap_msg->sgpc);
  assert (NULL != p_msg_gparam);

  if (OMX_IndexParamStandardComponentRole == p_msg_gparam->index)
    {
      rc = do_set_component_role (ap_sched, p_msg_gparam->p_struct);
    }
  else
    {
      rc = tiz_api_SetParameter (ap_sched->child.p_fsm, ap_msg->p_hdl,
                                 p_msg_gparam->index, p_msg_gparam->p_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
do_gconfig (tiz_scheduler_t * ap_sched,
            tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_setget_paramconfig_t *p_msg_gconfig = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_gconfig = &(ap_msg->sgpc);
  assert (NULL != p_msg_gconfig);

  return tiz_api_GetConfig (ap_sched->child.p_fsm, ap_msg->p_hdl,
                            p_msg_gconfig->index, p_msg_gconfig->p_struct);
}

static OMX_ERRORTYPE
do_sconfig (tiz_scheduler_t * ap_sched,
            tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_setget_paramconfig_t *p_msg_sconfig = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_sconfig = &(ap_msg->sgpc);
  assert (NULL != p_msg_sconfig);

  rc = tiz_api_SetConfig (ap_sched->child.p_fsm, ap_msg->p_hdl,
                          p_msg_sconfig->index, p_msg_sconfig->p_struct);

  /* Now have to delete the config struct, if api has been called
     non-blocking */
  if (OMX_FALSE == tiz_sched_blocking_apis_tbl[ETIZSchedMsgSetConfig])
    {
      tiz_mem_free (p_msg_sconfig->p_struct);
      p_msg_sconfig->p_struct = NULL;
    }

  return rc;
}

static OMX_ERRORTYPE
do_gei (tiz_scheduler_t * ap_sched,
        tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_getextindex_t *p_msg_gei = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_gei = &(ap_msg->gei);
  assert (NULL != p_msg_gei);

  /* Delegate to the kernel directly, no need to do checks in the fsm */
  return tiz_api_GetExtensionIndex (ap_sched->child.p_ker, ap_msg->p_hdl,
                                    p_msg_gei->p_ext_name, p_msg_gei->p_index);
}

static OMX_ERRORTYPE
do_gs (tiz_scheduler_t * ap_sched,
       tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_getstate_t *p_msg_gs = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_gs = &(ap_msg->gs);
  assert (NULL != p_msg_gs);

  return tiz_api_GetState (ap_sched->child.p_fsm,
                           ap_msg->p_hdl, p_msg_gs->p_state);
}

static OMX_ERRORTYPE
do_tr (tiz_scheduler_t * ap_sched,
       tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_tunnelrequest_t *p_msg_tr = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_tr = &(ap_msg->tr);
  assert (NULL != p_msg_tr);

  return tiz_api_ComponentTunnelRequest (ap_sched->child.p_fsm,
                                         ap_msg->p_hdl,
                                         p_msg_tr->pid,
                                         p_msg_tr->p_thdl,
                                         p_msg_tr->tpid, p_msg_tr->p_tsetup);
}

static OMX_ERRORTYPE
do_ub (tiz_scheduler_t * ap_sched,
       tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_usebuffer_t *p_msg_ub = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_ub = &(ap_msg->ub);
  assert (NULL != p_msg_ub);

  return tiz_api_UseBuffer (ap_sched->child.p_fsm,
                            ap_msg->p_hdl,
                            p_msg_ub->pp_hdr,
                            p_msg_ub->pid,
                            p_msg_ub->p_app_priv,
                            p_msg_ub->size, p_msg_ub->p_buf);
}

static OMX_ERRORTYPE
do_ab (tiz_scheduler_t * ap_sched,
       tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_allocbuffer_t *p_msg_ab = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_ab = &(ap_msg->ab);
  assert (NULL != p_msg_ab);

  return tiz_api_AllocateBuffer (ap_sched->child.p_fsm,
                                 ap_msg->p_hdl,
                                 p_msg_ab->pp_hdr,
                                 p_msg_ab->pid,
                                 p_msg_ab->p_app_priv, p_msg_ab->size);
}

static OMX_ERRORTYPE
do_fb (tiz_scheduler_t * ap_sched,
       tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_freebuffer_t *p_msg_fb = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_fb = &(ap_msg->fb);
  assert (NULL != p_msg_fb);

  return tiz_api_FreeBuffer (ap_sched->child.p_fsm,
                             ap_msg->p_hdl, p_msg_fb->pid, p_msg_fb->p_hdr);
}

static OMX_ERRORTYPE
do_etb (tiz_scheduler_t * ap_sched,
        tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_emptyfillbuffer_t *p_msg_efb = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_efb = &(ap_msg->efb);
  assert (NULL != p_msg_efb);

  return tiz_api_EmptyThisBuffer (ap_sched->child.p_fsm,
                                  ap_msg->p_hdl, p_msg_efb->p_hdr);
}

static OMX_ERRORTYPE
do_ftb (tiz_scheduler_t * ap_sched,
        tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_emptyfillbuffer_t *p_msg_efb = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_efb = &(ap_msg->efb);
  assert (NULL != p_msg_efb);

  return tiz_api_FillThisBuffer (ap_sched->child.p_fsm,
                                 ap_msg->p_hdl, p_msg_efb->p_hdr);
}

static OMX_ERRORTYPE
do_scbs (tiz_scheduler_t * ap_sched,
         tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_sched_msg_setcallbacks_t *p_msg_scbs = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);

  p_msg_scbs = &(ap_msg->scbs);
  assert (NULL != p_msg_scbs);

  /* Use the FSM to validate this API call */
  if (OMX_ErrorNone != (rc = tiz_api_SetCallbacks (ap_sched->child.p_fsm,
                                                   ap_msg->p_hdl,
                                                   p_msg_scbs->p_cbacks,
                                                   p_msg_scbs->p_appdata)))
    {
      return rc;
    }

  /* do the actual storing of the callbacks in the servants, who will be using
     them */
  tiz_srv_set_callbacks (ap_sched->child.p_fsm, p_msg_scbs->p_appdata,
                         p_msg_scbs->p_cbacks);
  tiz_srv_set_callbacks (ap_sched->child.p_ker, p_msg_scbs->p_appdata,
                         p_msg_scbs->p_cbacks);
  tiz_srv_set_callbacks (ap_sched->child.p_prc, p_msg_scbs->p_appdata,
                         p_msg_scbs->p_cbacks);

  return rc;
}

static OMX_ERRORTYPE
do_cre (tiz_scheduler_t * ap_sched,
        tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_componentroleenum_t *p_msg_getcre = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);
  p_msg_getcre = &(ap_msg->cre);
  assert (NULL != p_msg_getcre);

  if (p_msg_getcre->index < ap_sched->child.nroles)
    {
      strncpy ((char *) p_msg_getcre->p_role,
               (const char *) ap_sched->child.p_role_list[p_msg_getcre->
                                                          index]->role,
               OMX_MAX_STRINGNAME_SIZE);
      p_msg_getcre->p_role[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
    }
  else
    {
      rc = OMX_ErrorNoMore;
    }

  return rc;
}

static OMX_ERRORTYPE
do_plgevt (tiz_scheduler_t * ap_sched,
           tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_plg_event_t *p_msg_pe = NULL;
  tiz_event_pluggable_t *p_event = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);

  p_msg_pe = &(ap_msg->pe);
  assert (NULL != p_msg_pe);
  assert (NULL != p_msg_pe->p_event);

  p_event = p_msg_pe->p_event;
  return tiz_srv_receive_pluggable_event (p_event->p_servant,
                                          p_event->p_hdl, p_event);
}

static OMX_ERRORTYPE
do_rr (tiz_scheduler_t * ap_sched,
       tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_regroles_t *p_msg_rr = NULL;
  const tiz_role_factory_t *p_rf = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_U32 i = 0;
  OMX_U32 j = 0;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);

  p_msg_rr = &(ap_msg->rr);
  assert (NULL != p_msg_rr);
  assert (NULL != p_msg_rr->p_role_list);
  assert (p_msg_rr->nroles > 0);

  /* TODO: Validate this inputs only in debug mode */
  for (i = 0; i < p_msg_rr->nroles && rc == OMX_ErrorNone; ++i)
    {
      p_rf = p_msg_rr->p_role_list[i];
      if (NULL == p_rf->pf_cport || NULL == p_rf->pf_proc)
        {
          assert (0);
        }

      assert (p_rf->nports > 0);
      assert (p_rf->nports <= TIZ_COMP_MAX_PORTS);

      for (j = 0; j < p_rf->nports && rc == OMX_ErrorNone; ++j)
        {
          if (!p_rf->pf_port[j])
            {
              assert (0);
            }
        }
    }

  /* Store a local copy of the role list in the child struct */
  rc = store_roles (ap_sched, p_msg_rr);

  if (OMX_ErrorNone == rc)
    {
      /* Now instantiate the entities of role #0, the default role */
      rc = init_and_register_role (ap_sched, 0);
    }

  return rc;
}

static OMX_ERRORTYPE
do_rph (tiz_scheduler_t * ap_sched,
        tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_regphooks_t *p_msg_rph = NULL;
  const tiz_alloc_hooks_t *p_hooks = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);

  p_msg_rph = &(ap_msg->rph);
  assert (NULL != p_msg_rph);
  p_hooks = p_msg_rph->p_hooks;
  assert (NULL != p_hooks);

  {
    const tiz_fsm_state_id_t now = tiz_fsm_get_substate (ap_sched->child.p_fsm);

    /* Only allow role registration if in OMX_StateLoded state. Disallowed for
     * other states, even if the port is disabled.
     */
    if (EStateLoaded != now)
      {
        rc = OMX_ErrorIncorrectStateOperation;
      }
    else
      {
        OMX_U32 i = 0;
        void *p_port = NULL;
        OMX_U32 pid = 0;

        do
          {
            pid = ((OMX_ALL != p_hooks->pid) ? p_hooks->pid : i++);

            if (NULL != (p_port
                         = tiz_krn_get_port (ap_sched->child.p_ker, pid)))
              {
                tiz_port_set_alloc_hooks (p_port, p_hooks,
                                          p_hooks->pid == OMX_ALL ?
                                          NULL : p_msg_rph->p_old_hooks);
              }
          }
        while (p_port != NULL && (OMX_ALL == p_hooks->pid));

        if (NULL == p_port && OMX_ALL != p_hooks->pid)
          {
            /* Bad port index received */
            rc = OMX_ErrorBadPortIndex;
          }
      }
  }

  return rc;
}

static OMX_ERRORTYPE
do_eio (tiz_scheduler_t * ap_sched,
        tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_ev_io_t *p_msg_eio = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);

  p_msg_eio = &(ap_msg->eio);
  assert (NULL != p_msg_eio);

  return tiz_prc_io_ready (ap_sched->child.p_prc,
                           p_msg_eio->p_ev_io,
                           p_msg_eio->fd,
                           p_msg_eio->events);
}

static OMX_ERRORTYPE
do_etmr (tiz_scheduler_t * ap_sched,
         tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_ev_timer_t *p_msg_etmr = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);

  p_msg_etmr = &(ap_msg->etmr);
  assert (NULL != p_msg_etmr);

  return tiz_prc_timer_ready (ap_sched->child.p_prc,
                              p_msg_etmr->p_ev_timer,
                              p_msg_etmr->p_arg);
}

static OMX_ERRORTYPE
do_estat (tiz_scheduler_t * ap_sched,
          tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  tiz_sched_msg_ev_stat_t *p_msg_estat = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (ap_state && ETIZSchedStateStarted == *ap_state);

  p_msg_estat = &(ap_msg->estat);
  assert (NULL != p_msg_estat);

  return tiz_prc_stat_ready (ap_sched->child.p_prc,
                             p_msg_estat->p_ev_stat,
                             p_msg_estat->events);
}

static inline tiz_sched_msg_t *
init_scheduler_message (OMX_HANDLETYPE ap_hdl,
                        tiz_sched_msg_class_t a_msg_class)
{
  tiz_sched_msg_t *p_msg = NULL;

  assert (NULL != ap_hdl);
  assert (a_msg_class < ETIZSchedMsgMax);

  if (NULL == (p_msg = (tiz_sched_msg_t *)
               tiz_mem_calloc (1, sizeof (tiz_sched_msg_t))))
    {
      TIZ_LOGN (TIZ_TRACE, ap_hdl, "[OMX_ErrorInsufficientResources] : "
                "Creating message [%s]", tiz_sched_msg_to_str (a_msg_class));
    }
  else
    {
      p_msg->p_hdl = ap_hdl;
      p_msg->class = a_msg_class;
      p_msg->will_block = tiz_sched_blocking_apis_tbl [a_msg_class];
      assert (OMX_BOOL_MAX != p_msg->will_block);
    }

  return p_msg;
}

static OMX_ERRORTYPE
configure_port_preannouncements (tiz_scheduler_t * ap_sched,
                                 OMX_HANDLETYPE ap_hdl, OMX_PTR p_port)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const char *p_preannounce_disabled = NULL;
  char fqd_key[OMX_MAX_STRINGNAME_SIZE];
  char port_num[OMX_MAX_STRINGNAME_SIZE];
  OMX_U32 pid = tiz_port_index (p_port);

  /* OMX.component.name.key */
  sprintf (port_num, "%u", (unsigned int) pid);
  strncpy (fqd_key, ap_sched->cname, OMX_MAX_STRINGNAME_SIZE - 1);
  /* Make sure fqd_key is null-terminated */
  fqd_key[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
  strncat (fqd_key, ".preannouncements_disabled.port",
           OMX_MAX_STRINGNAME_SIZE - strlen (fqd_key) - 1);
  strncat (fqd_key, port_num, OMX_MAX_STRINGNAME_SIZE - strlen (fqd_key) - 1);

  p_preannounce_disabled = tiz_rcfile_get_value ("plugins-data", fqd_key);

  if (NULL == p_preannounce_disabled
      || (0 != strncmp (p_preannounce_disabled, "true", 4)))
    {
      TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s:port-%d] Preannouncements are "
                "[ENABLED]...", ap_sched->cname, pid);
    }
  else
    {
      OMX_TIZONIA_PARAM_BUFFER_PREANNOUNCEMENTSMODETYPE pamode;

      TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s:port-%d] Preannouncements are "
                "[DISABLED]...", ap_sched->cname, pid);

      pamode.nSize             = 
        sizeof (OMX_TIZONIA_PARAM_BUFFER_PREANNOUNCEMENTSMODETYPE);
      pamode.nVersion.nVersion = OMX_VERSION;
      pamode.nPortIndex        = pid;
      pamode.bEnabled          = OMX_FALSE;
      
      rc = tiz_api_SetParameter
        (p_port, ap_hdl,
         OMX_TizoniaIndexParamBufferPreAnnouncementsMode, &pamode);
    }

  return rc;
}

static OMX_ERRORTYPE
sched_ComponentDeInit (OMX_HANDLETYPE ap_hdl)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_sched_msg_t *p_msg = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : (null handle.)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgComponentDeInit);

  rc = send_msg (p_sched, p_msg);
  delete_scheduler (p_sched);
  return rc;
}

static OMX_ERRORTYPE
sched_GetComponentVersion (OMX_HANDLETYPE ap_hdl,
                           OMX_STRING ap_comp_name,
                           OMX_VERSIONTYPE * ap_comp_version,
                           OMX_VERSIONTYPE * ap_spec_version,
                           OMX_UUIDTYPE * ap_comp_uuid)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_getcomponentversion_t *p_msg_gcv = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl
      || NULL == ap_comp_name
      || NULL == ap_comp_version
      || NULL == ap_spec_version || NULL == ap_comp_uuid)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Null pointer argument received)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgGetComponentVersion);

  p_msg_gcv                 = &(p_msg->gcv);
  assert (NULL != p_msg_gcv);
  p_msg_gcv->p_comp_name    = ap_comp_name;
  p_msg_gcv->p_comp_version = ap_comp_version;
  p_msg_gcv->p_spec_version = ap_spec_version;
  p_msg_gcv->p_comp_uuid    = ap_comp_uuid;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_SendCommand (OMX_HANDLETYPE ap_hdl,
                   OMX_COMMANDTYPE a_cmd,
                   OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_sendcommand_t *p_msg_scmd = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || (OMX_CommandStateSet == a_cmd &&
                         (a_param1 < OMX_StateLoaded ||
                          a_param1 > OMX_StateWaitForResources)))
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Bad parameter found)");
      return OMX_ErrorBadParameter;
    }

  TIZ_LOGN (TIZ_ERROR, ap_hdl, "SendCommand [%s]", tiz_cmd_to_str (a_cmd));

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgSendCommand);

  p_msg_scmd             = &(p_msg->scmd);
  assert (NULL != p_msg_scmd);
  p_msg_scmd->cmd        = a_cmd;
  p_msg_scmd->param1     = a_param1;
  p_msg_scmd->p_cmd_data = ap_cmd_data;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_GetParameter (OMX_HANDLETYPE ap_hdl,
                    OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_setget_paramconfig_t *p_msg_gparam = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || NULL == ap_struct)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Null pointer argument received)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgGetParameter);

  p_msg_gparam           = &(p_msg->sgpc);
  assert (NULL != p_msg_gparam);
  p_msg_gparam->index    = a_index;
  p_msg_gparam->p_struct = ap_struct;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_SetParameter (OMX_HANDLETYPE ap_hdl,
                    OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_setget_paramconfig_t *p_msg_sparam = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || NULL == ap_struct)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Null pointer argument received)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgSetParameter);

  p_msg_sparam           = &(p_msg->sgpc);
  assert (NULL != p_msg_sparam);
  p_msg_sparam->index    = a_index;
  p_msg_sparam->p_struct = ap_struct;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_GetConfig (OMX_HANDLETYPE ap_hdl,
                 OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_setget_paramconfig_t *p_msg_gconf = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || NULL == ap_struct)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Null pointer argument received)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgGetConfig);

  p_msg_gconf           = &(p_msg->sgpc);
  assert (NULL != p_msg_gconf);
  p_msg_gconf->index    = a_index;
  p_msg_gconf->p_struct = ap_struct;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_SetConfig (OMX_HANDLETYPE ap_hdl,
                 OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_setget_paramconfig_t *p_msg_sconf = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || NULL == ap_struct)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Null pointer argument received)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgSetConfig);

  p_msg_sconf = &(p_msg->sgpc);
  assert (NULL != p_msg_sconf);

  /* Copy the config struct, if api is to be called non-blocking */
  if (OMX_FALSE == tiz_sched_blocking_apis_tbl[ETIZSchedMsgSetConfig])
    {
      if (NULL == (p_msg_sconf->p_struct
                   = tiz_mem_calloc (1, (*(OMX_U32 *) ap_struct))))
        {
          tiz_mem_free (p_msg);
          TIZ_LOGN (TIZ_ERROR, ap_hdl, "[OMX_ErrorInsufficientResources] : "
                    "(While allocating memory for config struct)");
          return OMX_ErrorInsufficientResources;
        }
      p_msg_sconf->p_struct
        = memcpy (p_msg_sconf->p_struct, ap_struct, (*(OMX_U32 *) ap_struct));
    }
  else
    {
      p_msg_sconf->p_struct = ap_struct;
    }

  p_msg_sconf->index = a_index;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_GetExtensionIndex (OMX_HANDLETYPE ap_hdl,
                         OMX_STRING ap_param_name,
                         OMX_INDEXTYPE * ap_index_type)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_getextindex_t *p_msg_gei = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_index_type || NULL == ap_param_name)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Null pointer argument received)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgGetExtensionIndex);

  p_msg_gei             = &(p_msg->gei);
  assert (NULL != p_msg_gei);
  p_msg_gei->p_ext_name = ap_param_name;
  p_msg_gei->p_index    = ap_index_type;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_GetState (OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_getstate_t *p_msg_gs = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || NULL == ap_state)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Null pointer argument received)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgGetState);

  p_msg_gs          = &(p_msg->gs);
  assert (NULL != p_msg_gs);
  p_msg_gs->p_state = ap_state;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_ComponentTunnelRequest (OMX_HANDLETYPE ap_hdl,
                              OMX_U32 a_pid,
                              OMX_HANDLETYPE ap_thdl,
                              OMX_U32 a_tpid,
                              OMX_TUNNELSETUPTYPE * ap_tsetup)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_tunnelrequest_t *p_msg_tr = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || (ap_thdl && !ap_tsetup))
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Bad parameter found "
               "p_hdl [%p] p_tsetup [%p])", ap_hdl, ap_tsetup);
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgComponentTunnelRequest);

  p_msg_tr           = &(p_msg->tr);
  assert (NULL != p_msg_tr);
  p_msg_tr->pid      = a_pid;
  p_msg_tr->p_thdl   = ap_thdl;
  p_msg_tr->tpid     = a_tpid;
  p_msg_tr->p_tsetup = ap_tsetup;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_UseBuffer (OMX_HANDLETYPE ap_hdl,
                 OMX_BUFFERHEADERTYPE ** app_hdr,
                 OMX_U32 a_pid,
                 OMX_PTR ap_apppriv, OMX_U32 a_size, OMX_U8 * ap_buf)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_usebuffer_t *p_msg_ub = NULL;
  tiz_scheduler_t *p_sched = NULL;

  /* From 1.2, ap_buf may be NULL. The provisional spec does not say what
   * a_size would be when ap_buf is NULL. For now assume a_size may also be
   * zero. */
  if (NULL == ap_hdl || NULL == app_hdr)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Bad parameter found "
               "p_hdl [%p] pp_hdr [%p] size [%d] p_buf [%p])",
               ap_hdl, app_hdr, a_size, ap_buf);
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgUseBuffer);

  p_msg_ub             = &(p_msg->ub);
  assert (NULL != p_msg_ub);
  p_msg_ub->pp_hdr     = app_hdr;
  p_msg_ub->pid        = a_pid;
  p_msg_ub->p_app_priv = ap_apppriv;
  p_msg_ub->size       = a_size;
  p_msg_ub->p_buf      = ap_buf;
  

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_AllocateBuffer (OMX_HANDLETYPE ap_hdl,
                      OMX_BUFFERHEADERTYPE ** app_hdr,
                      OMX_U32 a_pid, OMX_PTR ap_apppriv, OMX_U32 a_size)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_allocbuffer_t *p_msg_ab = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || 0 == a_size)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Bad parameter found)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgAllocateBuffer);

  p_msg_ab = &(p_msg->ab);
  assert (NULL != p_msg_ab);
  p_msg_ab->pp_hdr = app_hdr;
  p_msg_ab->pid = a_pid;
  p_msg_ab->p_app_priv = ap_apppriv;
  p_msg_ab->size = a_size;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_FreeBuffer (OMX_HANDLETYPE ap_hdl,
                  OMX_U32 a_pid, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_freebuffer_t *p_msg_fb = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || NULL == ap_hdr)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Null pointer argument received)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgFreeBuffer);

  p_msg_fb        = &(p_msg->fb);
  assert (NULL != p_msg_fb);
  p_msg_fb->pid   = a_pid;
  p_msg_fb->p_hdr = ap_hdr;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_EmptyThisBuffer (OMX_HANDLETYPE ap_hdl,
                       OMX_BUFFERHEADERTYPE * ap_hdr)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_emptyfillbuffer_t *p_msg_efb = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || NULL == ap_hdr)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Null pointer argument received)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgEmptyThisBuffer);

  p_msg_efb = &(p_msg->efb);
  assert (NULL != p_msg_efb);
  p_msg_efb->p_hdr = ap_hdr;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_FillThisBuffer (OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_emptyfillbuffer_t *p_msg_efb = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || NULL == ap_hdr)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Null pointer argument received)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgFillThisBuffer);

  p_msg_efb = &(p_msg->efb);
  assert (NULL != p_msg_efb);
  p_msg_efb->p_hdr = ap_hdr;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_SetCallbacks (OMX_HANDLETYPE ap_hdl,
                    OMX_CALLBACKTYPE * ap_cbacks, OMX_PTR ap_appdata)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_setcallbacks_t *p_msg_scbs = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || NULL == ap_cbacks)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Bad parameter found)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgSetCallbacks);

  p_msg_scbs = &(p_msg->scbs);
  assert (NULL != p_msg_scbs);
  p_msg_scbs->p_cbacks = ap_cbacks;
  p_msg_scbs->p_appdata = ap_appdata;

  return send_msg (p_sched, p_msg);
}

static OMX_ERRORTYPE
sched_UseEGLImage (OMX_HANDLETYPE ap_hdl,
                   OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                   OMX_U32 a_port_index, OMX_PTR ap_app_private,
                   void *eglImage)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
sched_ComponentRoleEnum (OMX_HANDLETYPE ap_hdl,
                         OMX_U8 * ap_role, OMX_U32 a_index)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_componentroleenum_t *p_msg_cre = NULL;
  tiz_scheduler_t *p_sched = NULL;

  if (NULL == ap_hdl || NULL == ap_role)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : "
               "(Null pointer argument received)");
      return OMX_ErrorBadParameter;
    }

  p_sched = get_sched (ap_hdl);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgComponentRoleEnum);

  p_msg_cre = &(p_msg->cre);
  assert (NULL != p_msg_cre);
  p_msg_cre->p_role = ap_role;
  p_msg_cre->index = a_index;

  return send_msg (p_sched, p_msg);
}

static OMX_BOOL
dispatch_msg (tiz_scheduler_t * ap_sched,
              tiz_sched_state_t * ap_state, tiz_sched_msg_t * ap_msg)
{
  OMX_BOOL signal_client = OMX_FALSE;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);
  assert (NULL != ap_state);
  assert (ap_msg->class < ETIZSchedMsgMax);

  TIZ_LOGN (TIZ_TRACE, ap_sched->child.p_hdl, "msg [%p] class [%s]",
            ap_msg, tiz_sched_msg_to_str (ap_msg->class));

  signal_client = ap_msg->will_block;

  rc = tiz_sched_msg_to_fnt_tbl[ap_msg->class] (ap_sched, ap_state, ap_msg);

  /* Return error to client */
  ap_sched->error = rc;

  tiz_mem_free (ap_msg);

  return signal_client;
}

static void
schedule_servants (tiz_scheduler_t * ap_sched,
                   const tiz_sched_state_t ap_state)
{
  OMX_PTR *p_ready = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_sched);
  assert (ETIZSchedStateStopped < ap_state);

  if (ETIZSchedStateStarted != ap_state
      || NULL == ap_sched->child.p_prc
      || NULL == ap_sched->child.p_ker || NULL == ap_sched->child.p_fsm)
    {
      TIZ_LOGN (TIZ_TRACE, ap_sched->child.p_hdl,
                "Not ready prc [%p] fsm [%p] ker [%p]",
                ap_sched->child.p_prc, ap_sched->child.p_fsm,
                ap_sched->child.p_ker);
      return;
    }

  /* Find the servant that is ready */
  /* Round-robin policy: fsm->ker->prc */
  TIZ_LOGN (TIZ_TRACE, ap_sched->child.p_hdl,
            "READY fsm [%s] ker [%s] prc [%s]",
            tiz_srv_is_ready (ap_sched->child.p_fsm) ? "YES" : "NO",
            tiz_srv_is_ready (ap_sched->child.p_ker) ? "YES" : "NO",
            tiz_srv_is_ready (ap_sched->child.p_prc) ? "YES" : "NO");
  do
    {
      p_ready = NULL;
      if (tiz_srv_is_ready (ap_sched->child.p_fsm))
        {
          p_ready = ap_sched->child.p_fsm;
          rc = tiz_srv_tick (p_ready);
        }

      if (OMX_ErrorNone == rc && tiz_srv_is_ready (ap_sched->child.p_ker))
        {
          p_ready = ap_sched->child.p_ker;
          rc = tiz_srv_tick (p_ready);
        }

      if (OMX_ErrorNone == rc && tiz_srv_is_ready (ap_sched->child.p_prc))
        {
          p_ready = ap_sched->child.p_prc;
          rc = tiz_srv_tick (p_ready);
        }

      if (tiz_queue_length (ap_sched->p_queue))
        {
          break;
        }

    }
  while (p_ready && (OMX_ErrorNone == rc));

  if (OMX_ErrorNone != rc)
    {
      /* INFO: For now, errors are sent via EventHandler by the servants */
      /* TODO: Review errors allowed via EventHandler */
      /* TODO: Review if tiz_srv_tick should return void */
    }
}

static void *
il_sched_thread_func (void *p_arg)
{
  tiz_scheduler_t *p_sched = (tiz_scheduler_t *) (p_arg);
  OMX_PTR p_data = NULL;
  OMX_BOOL signal_client = OMX_FALSE;

  assert (NULL != p_sched);

  p_sched->thread_id = tiz_thread_id ();
  tiz_check_omx_err_ret_null (tiz_sem_post (&(p_sched->sem)));

  for (;;)
    {
      tiz_check_omx_err_ret_null
        (tiz_queue_receive (p_sched->p_queue, &p_data));

      signal_client = dispatch_msg
        (p_sched, &(p_sched->state), (tiz_sched_msg_t *) p_data);

      if (signal_client)
        {
          tiz_sem_post (&(p_sched->sem));
        }

      if (ETIZSchedStateStopped == p_sched->state)
        {
          break;
        }

      schedule_servants (p_sched, p_sched->state);
    }

  TIZ_LOG (TIZ_TRACE, "Tizonia scheduler exiting...");

  return NULL;
}

static OMX_ERRORTYPE
start_scheduler (tiz_scheduler_t * ap_sched)
{
  assert (NULL != ap_sched);

  /* Create scheduler thread */
  tiz_check_omx_err_ret_oom (tiz_mutex_lock (&(ap_sched->mutex)));
  tiz_check_omx_err_ret_oom
    (tiz_thread_create (&(ap_sched->thread),
                        0, 0, il_sched_thread_func, ap_sched));

  tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&(ap_sched->mutex)));
  tiz_check_omx_err_ret_oom (tiz_sem_wait (&(ap_sched->sem)));

  return OMX_ErrorNone;
}

static void
delete_scheduler (tiz_scheduler_t * ap_sched)
{
  OMX_PTR p_result = NULL;
  assert (NULL != ap_sched);
  tiz_thread_join (&(ap_sched->thread), &p_result);
  delete_roles (ap_sched);
  tiz_mutex_destroy (&(ap_sched->mutex));
  tiz_sem_destroy (&(ap_sched->sem));
  tiz_queue_destroy (ap_sched->p_queue);
  ap_sched->p_queue = NULL;
  tiz_mem_free (ap_sched);
}

static tiz_scheduler_t *
instantiate_scheduler (OMX_HANDLETYPE ap_hdl, const char *ap_cname)
{
  tiz_scheduler_t *p_sched = NULL;
  int len = 0;

  assert (NULL != ap_hdl);

  tiz_ret_val_on_err
    ((p_sched = tiz_mem_calloc (1, sizeof (tiz_scheduler_t))), NULL);

  tiz_check_omx_err_ret_null (tiz_mutex_init (&(p_sched->mutex)));
  tiz_check_omx_err_ret_null (tiz_sem_init (&(p_sched->sem), 0));
  tiz_check_omx_err_ret_null (tiz_queue_init (&(p_sched->p_queue), 10));

  p_sched->child.p_fsm       = NULL;
  p_sched->child.p_ker       = NULL;
  p_sched->child.p_prc       = NULL;
  p_sched->child.p_role_list = NULL;
  p_sched->child.nroles      = 0;
  p_sched->child.p_hdl       = ap_hdl;
  p_sched->error             = OMX_ErrorNone;
  p_sched->state             = ETIZSchedStateStarting;

  len = strnlen (ap_cname, OMX_MAX_STRINGNAME_SIZE - 1);
  strncpy (p_sched->cname, ap_cname, len);
  p_sched->cname[len] = '\0';

  ((OMX_COMPONENTTYPE *) ap_hdl)->pComponentPrivate = p_sched;

  return p_sched;
}

static inline void
set_thread_name (tiz_scheduler_t * ap_sched)
{
  char *p_cname    = NULL;
  char *p_next_dot = NULL;
  int   thread_name_len  = 0;
  char  thread_name [16];        /* 16 is the max number of characters that
                                    tiz_thread_setname will use */
  assert (NULL != ap_sched);

  /* Let's skip the 'OMX.Company.' part */
  p_cname         = strstr (strstr (ap_sched->cname, ".") + 1, ".") + 1;
  p_next_dot      = strstr (p_cname, ".");
  thread_name_len = MIN (p_next_dot - p_cname, 16 - 1);
  
  strncpy (thread_name, p_cname, thread_name_len);
  thread_name [thread_name_len] = '\0';
  (void) tiz_thread_setname (&(ap_sched->thread), thread_name);
}

static OMX_ERRORTYPE
init_servants (tiz_scheduler_t * ap_sched, tiz_sched_msg_t * ap_msg)
{
  OMX_COMPONENTTYPE *p_hdl = NULL;

  assert (NULL != ap_sched);
  assert (NULL != ap_msg);

  set_thread_name (ap_sched);

  p_hdl = ap_sched->child.p_hdl;

  /* Init the component hdl */
  p_hdl->nVersion.s.nVersionMajor = 1;
  p_hdl->nVersion.s.nVersionMinor = 0;
  p_hdl->nVersion.s.nRevision     = 0;
  p_hdl->nVersion.s.nStep         = 0;
  p_hdl->pComponentPrivate        = ap_sched;
  
  p_hdl->GetComponentVersion    = sched_GetComponentVersion;
  p_hdl->SendCommand            = sched_SendCommand;
  p_hdl->GetParameter           = sched_GetParameter;
  p_hdl->SetParameter           = sched_SetParameter;
  p_hdl->GetConfig              = sched_GetConfig;
  p_hdl->SetConfig              = sched_SetConfig;
  p_hdl->GetExtensionIndex      = sched_GetExtensionIndex;
  p_hdl->GetState               = sched_GetState;
  p_hdl->ComponentTunnelRequest = sched_ComponentTunnelRequest;
  p_hdl->UseBuffer              = sched_UseBuffer;
  p_hdl->AllocateBuffer         = sched_AllocateBuffer;
  p_hdl->FreeBuffer             = sched_FreeBuffer;
  p_hdl->EmptyThisBuffer        = sched_EmptyThisBuffer;
  p_hdl->FillThisBuffer         = sched_FillThisBuffer;
  p_hdl->SetCallbacks           = sched_SetCallbacks;
  p_hdl->ComponentDeInit        = sched_ComponentDeInit;
  p_hdl->UseEGLImage            = sched_UseEGLImage;
  p_hdl->ComponentRoleEnum      = sched_ComponentRoleEnum;

  /* Init the small object allocator */
  tiz_check_omx_err_ret_oom (tiz_soa_init (&(ap_sched->p_soa)));

  /* Init the FSM */
  tiz_fsm_init ();
  ap_sched->child.p_fsm = factory_new (tizfsm, p_hdl);

  /* Init the kernel */
  tiz_krn_init ();
  ap_sched->child.p_ker = factory_new (tizkrn, p_hdl);

  /* All servants will use the same small object allocator */
  tiz_check_omx_err_ret_oom
    (tiz_srv_set_allocator (ap_sched->child.p_fsm, ap_sched->p_soa));
  tiz_check_omx_err_ret_oom
    (tiz_srv_set_allocator (ap_sched->child.p_ker, ap_sched->p_soa));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
deinit_servants (tiz_scheduler_t * ap_sched, tiz_sched_msg_t * ap_msg)
{
  assert (NULL != ap_sched);
  assert (NULL != ap_msg);

  /* delete the processor servant */
  factory_delete (ap_sched->child.p_prc);
  ap_sched->child.p_prc = NULL;

  /* delete the kernel servant */
  factory_delete (ap_sched->child.p_ker);
  ap_sched->child.p_ker = NULL;

  /* delete the FSM servant */
  factory_delete (ap_sched->child.p_fsm);
  ap_sched->child.p_fsm = NULL;

  /* Destroy the object allocator used by the servants */
  tiz_soa_destroy (ap_sched->p_soa);
  ap_sched->p_soa = NULL;

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
init_and_register_role (tiz_scheduler_t * ap_sched, const OMX_U32 a_role_pos)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_role_factory_t *p_rf = NULL;
  OMX_PTR p_port = NULL;
  OMX_PTR p_proc = NULL;
  OMX_HANDLETYPE p_hdl = NULL;
  OMX_U32 j = 0;

  assert (NULL != ap_sched);

  p_hdl = ap_sched->child.p_hdl;
  p_rf = ap_sched->child.p_role_list[a_role_pos];
  p_port = p_rf->pf_cport (p_hdl);

  assert (NULL != p_port);

  rc = tiz_krn_register_port (ap_sched->child.p_ker, p_port, OMX_TRUE);       /* it is a config port */

  TIZ_LOGN (TIZ_TRACE, ap_sched->child.p_hdl,
            "Registering role #[%d] -> [%s] nports = [%d] rc = [%s]...",
            a_role_pos, p_rf->role, p_rf->nports, tiz_err_to_str (rc));

  for (j = 0; j < p_rf->nports && rc == OMX_ErrorNone; ++j)
    {
      /* Instantiate the port */
      p_port = p_rf->pf_port[j] (p_hdl);
      assert (NULL != p_port);
      rc = tiz_krn_register_port (ap_sched->child.p_ker, p_port, OMX_FALSE);  /* not a config port */

      if (OMX_ErrorNone == rc)
        {
          rc = configure_port_preannouncements (ap_sched, p_hdl, p_port);
        }
    }

  if (OMX_ErrorNone == rc)
    {
      p_proc = p_rf->pf_proc (p_hdl);
      assert (NULL != p_proc);
      assert (NULL == ap_sched->child.p_prc);
      ap_sched->child.p_prc = p_proc;

      /* All servants will use the same object allocator */
      tiz_srv_set_allocator (p_proc, ap_sched->p_soa);
    }

  return rc;
}

OMX_ERRORTYPE
tiz_comp_init (const OMX_HANDLETYPE ap_hdl, const char *ap_cname)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_scheduler_t *p_sched = NULL;

  TIZ_LOG (TIZ_TRACE, "[%s] Initializing base component "
           "infrastructure", ap_cname);

  if (NULL == ap_hdl)
    {
      TIZ_LOG (TIZ_ERROR, "[OMX_ErrorBadParameter] : (%s)", ap_cname);
      return OMX_ErrorBadParameter;
    }

  /* Instantiate the scheduler */
  if (NULL == (p_sched = instantiate_scheduler (ap_hdl, ap_cname)))
    {
      TIZ_LOG (TIZ_ERROR,
               "[OMX_ErrorInsufficientResources] : ([%s] - Error Initializing"
               " component - hdl [%p])...", ap_cname, ap_hdl);
      return OMX_ErrorInsufficientResources;
    }

  tiz_check_omx_err_ret_oom (start_scheduler (p_sched));

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgComponentInit);

  return send_msg (p_sched, p_msg);
}

OMX_ERRORTYPE
tiz_comp_register_roles (const OMX_HANDLETYPE ap_hdl,
                         const tiz_role_factory_t * ap_role_list[],
                         const OMX_U32 a_nroles)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_regroles_t *p_msg_rr = NULL;
  tiz_scheduler_t *p_sched = get_sched (ap_hdl);

  assert (NULL != ap_role_list);
  assert (0 < a_nroles);
  assert (a_nroles <= TIZ_COMP_MAX_ROLES);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgRegisterRoles);

  p_msg_rr = &(p_msg->rr);
  assert (NULL != p_msg_rr);
  p_msg_rr->p_role_list = ap_role_list;
  p_msg_rr->nroles = a_nroles;

  return send_msg (p_sched, p_msg);
}

OMX_ERRORTYPE
tiz_comp_event_pluggable (const OMX_HANDLETYPE ap_hdl,
                          tiz_event_pluggable_t * ap_event)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_plg_event_t *p_msg_pe = NULL;
  tiz_scheduler_t *p_sched = get_sched (ap_hdl);

  assert (NULL != ap_event);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgPluggableEvent);

  p_msg_pe = &(p_msg->pe);
  assert (NULL != p_msg_pe);
  p_msg_pe->p_event = ap_event;

  return send_msg (p_sched, p_msg);
}

OMX_ERRORTYPE
tiz_comp_register_alloc_hooks (const OMX_HANDLETYPE ap_hdl,
                               const tiz_alloc_hooks_t * ap_new_hooks,
                               tiz_alloc_hooks_t * ap_old_hooks)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_regphooks_t *p_msg_rph = NULL;
  tiz_scheduler_t *p_sched = get_sched (ap_hdl);

  assert (NULL != ap_new_hooks);

  TIZ_COMP_INIT_MSG_OOM (ap_hdl, p_msg, ETIZSchedMsgRegisterPortHooks);

  p_msg_rph = &(p_msg->rph);
  assert (NULL != p_msg_rph);
  p_msg_rph->p_hooks     = ap_new_hooks;
  p_msg_rph->p_old_hooks = ap_old_hooks;

  return send_msg (p_sched, p_msg);
}

void
tiz_comp_event_io (OMX_HANDLETYPE ap_hdl, tiz_event_io_t * ap_ev_io, int a_fd,
                   int a_events)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_ev_io_t *p_msg_eio = NULL;
  tiz_scheduler_t *p_sched = get_sched (ap_hdl);

  assert (NULL != ap_ev_io);

  TIZ_COMP_INIT_MSG (ap_hdl, p_msg, ETIZSchedMsgEvIo);

  p_msg_eio = &(p_msg->eio);
  assert (NULL != p_msg_eio);
  p_msg_eio->p_ev_io = ap_ev_io;
  p_msg_eio->fd = a_fd;
  p_msg_eio->events = a_events;

  /* TODO: Shouldn't mask this return code */
  (void) send_msg (p_sched, p_msg);
}

void
tiz_comp_event_timer (OMX_HANDLETYPE ap_hdl, tiz_event_timer_t * ap_ev_timer,
                      void *ap_arg)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_ev_timer_t *p_msg_etmr = NULL;
  tiz_scheduler_t *p_sched = get_sched (ap_hdl);

  assert (NULL != ap_ev_timer);

  TIZ_COMP_INIT_MSG (ap_hdl, p_msg, ETIZSchedMsgEvTimer);

  p_msg_etmr = &(p_msg->etmr);
  assert (NULL != p_msg_etmr);
  p_msg_etmr->p_ev_timer = ap_ev_timer;
  p_msg_etmr->p_arg = ap_arg;

  /* TODO: Shouldn't mask this return code */
  (void) send_msg (p_sched, p_msg);
}

void
tiz_comp_event_stat (OMX_HANDLETYPE ap_hdl, tiz_event_stat_t * ap_ev_stat,
                     int a_events)
{
  tiz_sched_msg_t *p_msg = NULL;
  tiz_sched_msg_ev_stat_t *p_msg_estat = NULL;
  tiz_scheduler_t *p_sched = get_sched (ap_hdl);

  assert (NULL != ap_ev_stat);

  TIZ_COMP_INIT_MSG (ap_hdl, p_msg, ETIZSchedMsgEvStat);

  p_msg_estat = &(p_msg->estat);
  assert (NULL != p_msg_estat);
  p_msg_estat->p_ev_stat = ap_ev_stat;

  /* TODO: Shouldn't mask this return code */
  (void) send_msg (p_sched, p_msg);
}

void *
tiz_get_sched (const OMX_HANDLETYPE ap_hdl)
{
  return ((OMX_COMPONENTTYPE *) ap_hdl)->pComponentPrivate;
}

void *
tiz_get_fsm (const OMX_HANDLETYPE ap_hdl)
{
  tiz_scheduler_t *p_sched = NULL;
  assert (NULL != ap_hdl);
  p_sched = ((OMX_COMPONENTTYPE *) ap_hdl)->pComponentPrivate;
  return p_sched->child.p_fsm;
}

void *
tiz_get_krn (const OMX_HANDLETYPE ap_hdl)
{
  tiz_scheduler_t *p_sched = NULL;
  assert (NULL != ap_hdl);
  p_sched = ((OMX_COMPONENTTYPE *) ap_hdl)->pComponentPrivate;
  return p_sched->child.p_ker;
}

void *
tiz_get_prc (const OMX_HANDLETYPE ap_hdl)
{
  tiz_scheduler_t *p_sched = NULL;
  assert (NULL != ap_hdl);
  p_sched = ((OMX_COMPONENTTYPE *) ap_hdl)->pComponentPrivate;
  return p_sched->child.p_prc;
}
