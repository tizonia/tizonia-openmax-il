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
 * @file   tizkernel.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - kernel class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include "OMX_Types.h"
#include "OMX_TizoniaExt.h"

#include "tizfsm.h"
#include "tizport.h"
#include "tizkernel.h"
#include "tizkernel_decls.h"
#include "tizutils.h"


#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.kernel"
#endif

/* Forward declarations */
static OMX_BOOL all_populated (const void *ap_obj);
static OMX_BOOL all_depopulated (const void *ap_obj);
static OMX_BOOL all_buffers_returned (void *ap_obj);
static OMX_ERRORTYPE dispatch_sc (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_etb (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_ftb (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_cb (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_pe (void *ap_obj, OMX_PTR ap_msg);

typedef struct tiz_kernel_msg_sendcommand tiz_kernel_msg_sendcommand_t;
static OMX_ERRORTYPE dispatch_state_set (void *ap_obj, OMX_HANDLETYPE p_hdl,
                                         tiz_kernel_msg_sendcommand_t *
                                         ap_msg_sc);
static OMX_ERRORTYPE dispatch_port_disable (void *ap_obj,
                                            OMX_HANDLETYPE p_hdl,
                                            tiz_kernel_msg_sendcommand_t *
                                            ap_msg_sc);
static OMX_ERRORTYPE dispatch_port_enable (void *ap_obj, OMX_HANDLETYPE p_hdl,
                                           tiz_kernel_msg_sendcommand_t *
                                           ap_msg_sc);
static OMX_ERRORTYPE dispatch_port_flush (void *ap_obj, OMX_HANDLETYPE p_hdl,
                                          tiz_kernel_msg_sendcommand_t *
                                          ap_msg_sc);
static OMX_ERRORTYPE dispatch_mark_buffer (void *ap_obj, OMX_HANDLETYPE p_hdl,
                                           tiz_kernel_msg_sendcommand_t *
                                           ap_msg_sc);
static OMX_ERRORTYPE flush_marks (void *ap_obj, OMX_PTR ap_port);


typedef enum tiz_kernel_msg_class tiz_kernel_msg_class_t;
enum tiz_kernel_msg_class
{
  ETIZKernelMsgSendCommand = 0,
  ETIZKernelMsgEmptyThisBuffer,
  ETIZKernelMsgFillThisBuffer,
  ETIZKernelMsgCallback,
  ETIZKernelMsgPluggableEvent,
  ETIZKernelMsgMax
};

typedef OMX_ERRORTYPE (*tiz_kernel_msg_dispatch_f) (void *ap_obj,
                                                   OMX_PTR ap_msg);

static const tiz_kernel_msg_dispatch_f tiz_kernel_msg_to_fnt_tbl[] = {
  dispatch_sc,
  dispatch_etb,
  dispatch_ftb,
  dispatch_cb,
  dispatch_pe,
};

typedef OMX_ERRORTYPE (*tiz_kernel_msg_dispatch_sc_f)
  (void *ap_obj, OMX_HANDLETYPE p_hdl,
   tiz_kernel_msg_sendcommand_t * ap_msg_sc);

static const tiz_kernel_msg_dispatch_sc_f
  tiz_kernel_msg_dispatch_sc_to_fnt_tbl[] = {
  dispatch_state_set,
  dispatch_port_flush,
  dispatch_port_disable,
  dispatch_port_enable,
  dispatch_mark_buffer,
};

typedef struct tiz_kernel_msg_sendcommand tiz_kernel_msg_sendcommand_t;
struct tiz_kernel_msg_sendcommand
{
  OMX_COMMANDTYPE cmd;
  OMX_U32 param1;
  OMX_PTR p_cmd_data;
};

typedef struct tiz_kernel_msg_emptyfillbuffer tiz_kernel_msg_emptyfillbuffer_t;
struct tiz_kernel_msg_emptyfillbuffer
{
  OMX_BUFFERHEADERTYPE *p_hdr;
};

typedef struct tiz_kernel_msg_callback tiz_kernel_msg_callback_t;
struct tiz_kernel_msg_callback
{
  OMX_BUFFERHEADERTYPE *p_hdr;
  OMX_U32 pid;
  OMX_DIRTYPE dir;
};

typedef struct tiz_kernel_msg_plg_event tiz_kernel_msg_plg_event_t;
struct tiz_kernel_msg_plg_event
{
  tizevent_t *p_event;
};

typedef struct tiz_kernel_msg tiz_kernel_msg_t;
struct tiz_kernel_msg
{
  OMX_HANDLETYPE p_hdl;
  tiz_kernel_msg_class_t class;
  union
  {
    tiz_kernel_msg_sendcommand_t sc;
    tiz_kernel_msg_emptyfillbuffer_t ef;
    tiz_kernel_msg_callback_t cb;
    tiz_kernel_msg_plg_event_t pe;
  };
};

typedef struct tiz_kernel_msg_str tiz_kernel_msg_str_t;
struct tiz_kernel_msg_str
{
  tiz_kernel_msg_class_t msg;
  OMX_STRING str;
};

static const tiz_kernel_msg_str_t tiz_kernel_msg_to_str_tbl[] = {
  {ETIZKernelMsgSendCommand, "ETIZKernelMsgSendCommand"},
  {ETIZKernelMsgEmptyThisBuffer, "ETIZKernelMsgEmptyThisBuffer"},
  {ETIZKernelMsgFillThisBuffer, "ETIZKernelMsgFillThisBuffer"},
  {ETIZKernelMsgCallback, "ETIZKernelMsgCallback"},
  {ETIZKernelMsgPluggableEvent, "ETIZKernelMsgPluggableEvent"},
  {ETIZKernelMsgMax, "ETIZKernelMsgMax"},
};

static const OMX_STRING
tiz_kernel_msg_to_str (tiz_kernel_msg_class_t a_msg)
{
  const OMX_S32 count =
    sizeof (tiz_kernel_msg_to_str_tbl) / sizeof (tiz_kernel_msg_str_t);
  OMX_S32 i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_kernel_msg_to_str_tbl[i].msg == a_msg)
        {
          return tiz_kernel_msg_to_str_tbl[i].str;
        }
    }

  return "Unknown kernel message";
}

static OMX_BOOL
remove_buffer_from_servant_queue (OMX_PTR ap_elem, OMX_S32 a_data1,
                                  OMX_PTR ap_data2)
{
  OMX_BOOL rc = OMX_FALSE;
  tiz_kernel_msg_t *p_msg = ap_elem;
  const OMX_BUFFERHEADERTYPE *p_hdr = ap_data2;

  assert (NULL != ap_elem);
  assert (NULL != ap_data2);

  if (p_msg->class == a_data1)
    {
      tiz_kernel_msg_callback_t *p_msg_c = &(p_msg->cb);
      if (p_hdr == p_msg_c->p_hdr)
        {
          /* Found, return TRUE so this item will be removed from the servant
           * queue */
          TIZ_LOGN (TIZ_TRACE, p_msg->p_hdl, "Found HEADER [%p]", p_hdr);
          rc = OMX_TRUE;
        }
    }
  else
    {
      /* Not interested */
      TIZ_LOG (TIZ_TRACE, "Not interested in msg - class [%s]",
               tiz_kernel_msg_to_str (p_msg->class));
    }

  return rc;
}

static void
rm_callback_hdlr (void *ap_obj, OMX_HANDLETYPE ap_hdl, tizevent_t * ap_event)
{
  if (NULL != ap_event)
    {
      tiz_mem_free (ap_event->p_data);
      tiz_mem_free (ap_event);
    }
}

static void
deliver_pluggable_event (OMX_U32 rid, OMX_HANDLETYPE ap_hdl)
{
  tizevent_t *p_event = (tizevent_t *) tiz_mem_calloc (1, sizeof (tizevent_t));
  OMX_U32 *p_rid = (OMX_U32 *) tiz_mem_calloc (1, sizeof (OMX_U32));
  *p_rid = rid;

  p_event->p_hdl = ap_hdl;
  p_event->p_servant = tiz_get_krn (ap_hdl);
  p_event->p_data = p_rid;
  p_event->pf_hdlr = &rm_callback_hdlr;

  tiz_receive_pluggable_event (ap_hdl, p_event);
}

static void
wait_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  TIZ_LOG (TIZ_TRACE, "wait_complete : rid [%u]", rid);
  deliver_pluggable_event (rid, ap_data);
}

static void
preemption_req (OMX_U32 rid, OMX_PTR ap_data)
{
  TIZ_LOG (TIZ_TRACE, "preemption_req : rid [%u]", rid);
  deliver_pluggable_event (rid, ap_data);
}

static void
preemption_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  TIZ_LOG (TIZ_TRACE, "preemption_complete : rid [%u]", rid);
  deliver_pluggable_event (rid, ap_data);
}

static OMX_ERRORTYPE
init_rm (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  OMX_U8 comp_name[OMX_MAX_STRINGNAME_SIZE];
  OMX_VERSIONTYPE comp_ver, spec_ver;
  OMX_UUIDTYPE uuid;
  OMX_PRIORITYMGMTTYPE primgmt;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tizrm_error_t rmrc = TIZRM_SUCCESS;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  rc = tiz_api_GetComponentVersion (p_obj->p_cport_, ap_hdl,
                                   (OMX_STRING) (&comp_name),
                                   &comp_ver, &spec_ver, &uuid);
  if (OMX_ErrorNone != rc)
    {
      TIZ_LOGN (TIZ_ERROR, ap_hdl, "[%s] : while retrieving the component's "
                "name...", tiz_err_to_str (rc));
      return rc;
    }

  primgmt.nSize = sizeof (OMX_PRIORITYMGMTTYPE);
  primgmt.nVersion.nVersion = OMX_VERSION;
  if (OMX_ErrorNone != (rc = tiz_api_GetConfig (p_obj->p_cport_, ap_hdl,
                                               OMX_IndexConfigPriorityMgmt,
                                               &primgmt)))
    {
      TIZ_LOGN (TIZ_ERROR, ap_hdl, "[%s] : while retrieving "
                "OMX_IndexConfigPriorityMgmt...", tiz_err_to_str (rc));
      return rc;
    }

  if (TIZRM_SUCCESS
      != (rmrc = tizrm_proxy_init (&p_obj->rm_, (OMX_STRING) (&comp_name),
                                   (const OMX_UUIDTYPE *) &uuid,
                                   &primgmt, &p_obj->rm_cbacks_, ap_hdl)))
    {
      TIZ_LOGN (TIZ_ERROR, ap_hdl, "[OMX_ErrorInsufficientResources] : "
                "RM error [%d]...", rmrc);
      return OMX_ErrorInsufficientResources;
    }

  TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s] [%p] : RM init'ed", comp_name, ap_hdl);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
deinit_rm (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  tizrm_error_t rmrc = TIZRM_SUCCESS;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  if (TIZRM_SUCCESS != (rmrc = tizrm_proxy_destroy (&p_obj->rm_)))
    {
      /* TODO: Translate into a proper error code, especially OOM error  */
      TIZ_LOGN (TIZ_TRACE, ap_hdl, "RM proxy deinitialization failed...");
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

static inline tiz_vector_t *
get_ingress_lst (const struct tizkernel *ap_obj, OMX_U32 a_pid)
{
  tiz_vector_t * p_list = NULL;
  assert (NULL != ap_obj);
  /* Grab the port's ingress list */
  p_list = tiz_vector_at (ap_obj->p_ingress_, a_pid);
  assert (p_list && *(tiz_vector_t **) p_list);
  return (*(tiz_vector_t **) p_list);
}

static inline tiz_vector_t *
get_egress_lst (const struct tizkernel *ap_obj, OMX_U32 a_pid)
{
  tiz_vector_t * p_list = NULL;
  assert (NULL != ap_obj);
  /* Grab the port's egress list */
  p_list = tiz_vector_at (ap_obj->p_egress_, a_pid);
  assert (p_list && *(tiz_vector_t **) p_list);
  return (*(tiz_vector_t **) p_list);
}

static inline tiz_vector_t *
get_port (const struct tizkernel *ap_obj, OMX_U32 a_pid)
{
  OMX_PTR *pp_port = NULL;
  assert (NULL != ap_obj);
  /* Find the port.. */
  pp_port = tiz_vector_at (ap_obj->p_ports_, a_pid);
  assert (pp_port && *pp_port);
  return *pp_port;
}

static inline OMX_BUFFERHEADERTYPE *
get_header (const tiz_vector_t *ap_list, OMX_U32 a_index)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  assert (NULL != ap_list);
  /* Retrieve the header... */
  pp_hdr = tiz_vector_at (ap_list, a_index);
  assert (pp_hdr && *pp_hdr);
  return *pp_hdr;
}

static OMX_ERRORTYPE
complete_port_disable (void *ap_obj, OMX_PTR ap_port, OMX_U32 a_pid,
                       OMX_ERRORTYPE a_error)
{
  struct tizkernel *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;

  assert (NULL != ap_obj);
  assert (NULL != ap_port);

  /* Set disabled flag */
  TIZ_PORT_SET_DISABLED (ap_port);

  /* Decrement the completion counter */
  assert (p_obj->cmd_completion_count_ > 0);
  p_obj->cmd_completion_count_--;

  if (p_obj->cmd_completion_count_ > 0)
    {
      /* Complete the OMX_CommandPortDisable command here */
      (void) tiz_servant_issue_cmd_event (p_obj, OMX_CommandPortDisable, a_pid,
                                         a_error);
    }

  /* If the completion count is zero, let the FSM complete, as it will know
   * whether this a cancelation or not. */
  if (p_obj->cmd_completion_count_ == 0)
    {
      tiz_fsm_complete_command (tiz_get_fsm (p_parent->p_hdl_), p_obj,
                               OMX_CommandPortDisable, a_pid);
    }

  /* Flush buffer marks and complete commands as required */
  return flush_marks (p_obj, ap_port);
}

static OMX_ERRORTYPE
complete_port_enable (void *ap_obj, OMX_PTR ap_port, OMX_U32 a_pid,
                      OMX_ERRORTYPE a_error)
{
  struct tizkernel *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;

  assert (NULL != ap_obj);
  assert (NULL != ap_port);

  /* Set enabled flag */
  TIZ_PORT_SET_ENABLED (ap_port);

  /* Decrement the completion counter */
  assert (p_obj->cmd_completion_count_ > 0);
  p_obj->cmd_completion_count_--;

  if (p_obj->cmd_completion_count_ > 0)
    {
      /* Complete the OMX_CommandPortEnable command here */
      (void) tiz_servant_issue_cmd_event (p_obj, OMX_CommandPortEnable, a_pid,
                                         a_error);
    }

  /* If the completion count is zero, let the FSM complete, as it will know
   * whether this a cancelation or not. */
  if (p_obj->cmd_completion_count_ == 0)
    {
      tiz_fsm_complete_command (tiz_get_fsm (p_parent->p_hdl_), p_obj,
                               OMX_CommandPortEnable, a_pid);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
complete_port_flush (void *ap_obj, OMX_PTR ap_port, OMX_U32 a_pid,
                     OMX_ERRORTYPE a_error)
{
  struct tizkernel *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;

  assert (NULL != ap_obj);
  assert (NULL != ap_port);

  TIZ_PORT_CLEAR_FLUSH_IN_PROGRESS (ap_port);

  /* Complete the OMX_CommandFlush command */
  (void) tiz_servant_issue_cmd_event (p_obj, OMX_CommandFlush, a_pid, a_error);

  /* Decrement the completion counter */
  assert (p_obj->cmd_completion_count_ > 0);
  if (--p_obj->cmd_completion_count_ == 0)
    {
      tiz_fsm_complete_command (tiz_get_fsm (p_parent->p_hdl_), p_obj,
                               OMX_CommandFlush, a_pid);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
complete_mark_buffer (void *ap_obj, OMX_PTR ap_port, OMX_U32 a_pid,
                      OMX_ERRORTYPE a_error)
{
  struct tizkernel *p_obj = ap_obj;

  assert (NULL != ap_obj);
  assert (NULL != ap_port);

  /* Complete the OMX_CommandMarkBuffer command */
  (void) tiz_servant_issue_cmd_event (p_obj, OMX_CommandMarkBuffer, a_pid,
                                     a_error);

  /* Decrement the completion counter */
/*   assert (p_obj->cmd_completion_count_ > 0); */
/*   if (--p_obj->cmd_completion_count_ == 0) */
/*     { */
/*       tiz_fsm_complete_command (tiz_get_fsm (p_parent->p_hdl), p_obj, */
/*                                OMX_CommandMarkBuffer); */
/*     } */

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
complete_ongoing_transitions (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const tiz_fsm_state_id_t cur_state =
    tiz_fsm_get_substate (tiz_get_fsm (ap_hdl));

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  if ((ESubStateIdleToLoaded == cur_state) && all_depopulated (ap_obj))
    {
      TIZ_LOGN (TIZ_TRACE, ap_hdl, "AllPortsDepopulated : [TRUE]");
      /* TODO : Review this */
      /* If all ports are depopulated, kick off removal of buffer
       * callbacks from servants kernel and proc queues  */
      rc = tiz_fsm_complete_transition
        (tiz_get_fsm (ap_hdl), ap_obj, OMX_StateLoaded);
    }
  else if ((ESubStateLoadedToIdle == cur_state) && all_populated (ap_obj))
    {
      TIZ_LOGN (TIZ_TRACE, ap_hdl, "AllPortsPopulated : [TRUE]");
      rc = tiz_fsm_complete_transition
        (tiz_get_fsm (ap_hdl), ap_obj, OMX_StateIdle);
    }
  return rc;
}

static OMX_ERRORTYPE
acquire_rm_resources (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  tizrm_error_t rmrc = TIZRM_SUCCESS;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  /* Request permission to use the RM-based resources */
  if (TIZRM_SUCCESS
      != (rmrc = tizrm_proxy_acquire (&p_obj->rm_, TIZRM_RESOURCE_DUMMY, 1)))
    {
      switch (rmrc)
        {
        case TIZRM_PREEMPTION_IN_PROGRESS:
          {
            rc = OMX_ErrorResourcesPreempted;
          }
          break;

        case TIZRM_NOT_ENOUGH_RESOURCE_AVAILABLE:
        default:
          {
            rc = OMX_ErrorInsufficientResources;
          }
        };

      TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s] : While acquiring resource - "
                "RM error [%d]...", tiz_err_to_str (rc), rmrc);

    }

  return rc;
}

static OMX_ERRORTYPE
release_rm_resources (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  tizrm_error_t rmrc = TIZRM_SUCCESS;

  if (TIZRM_SUCCESS
      != (rmrc = tizrm_proxy_release (&p_obj->rm_, TIZRM_RESOURCE_DUMMY, 1)))
    {
      TIZ_LOGN (TIZ_TRACE,ap_hdl, "Resource release failed - RM error [%d]...",
                rmrc);
    }

  return OMX_ErrorNone;
}

static OMX_S32
move_to_ingress (void *ap_obj, OMX_U32 a_pid)
{

  struct tizkernel *p_obj = ap_obj;
  tiz_vector_t *p_elist = NULL;
  tiz_vector_t *p_ilist = NULL;
  const OMX_S32 nports = tiz_vector_length (p_obj->p_ports_);
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (a_pid < nports);

  p_elist = tiz_vector_at (p_obj->p_egress_, a_pid);
  p_ilist = tiz_vector_at (p_obj->p_ingress_, a_pid);
  assert (p_elist && *(tiz_vector_t **) p_elist);
  p_elist = *(tiz_vector_t **) p_elist;
  assert (p_ilist && *(tiz_vector_t **) p_elist);
  p_ilist = *(tiz_vector_t **) p_ilist;
  rc = tiz_vector_append (p_ilist, p_elist);
  tiz_vector_clear (p_elist);

  if (OMX_ErrorNone != rc)
    {
      return -1;
    }

  return tiz_vector_length (p_ilist);
}

static OMX_S32
move_to_egress (void *ap_obj, OMX_U32 a_pid)
{

  struct tizkernel *p_obj = ap_obj;
  const OMX_S32 nports = tiz_vector_length (p_obj->p_ports_);
  tiz_vector_t *p_elist = NULL;
  tiz_vector_t *p_ilist = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (a_pid < nports);

  p_elist = tiz_vector_at (p_obj->p_egress_, a_pid);
  p_ilist = tiz_vector_at (p_obj->p_ingress_, a_pid);
  assert (p_elist && *(tiz_vector_t **) p_elist);
  p_elist = *(tiz_vector_t **) p_elist;
  assert (p_ilist && *(tiz_vector_t **) p_ilist);
  p_ilist = *(tiz_vector_t **) p_ilist;
  rc = tiz_vector_append (p_elist, p_ilist);
  tiz_vector_clear (p_ilist);

  if (OMX_ErrorNone != rc)
    {
      return -1;
    }

  return tiz_vector_length (p_elist);
}

static OMX_S32
add_to_buflst (void *ap_obj, tiz_vector_t * ap_dst2darr,
               const OMX_BUFFERHEADERTYPE * ap_hdr, const void *ap_port)
{
  const struct tizkernel *p_obj = ap_obj;
  tiz_vector_t *p_list = NULL;
  const OMX_U32 pid = tiz_port_index (ap_port);
  OMX_HANDLETYPE hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);
  (void) hdl;

  assert (ap_dst2darr);
  assert (ap_hdr);
  assert (tiz_vector_length (ap_dst2darr) >= pid);

  p_list = tiz_vector_at (ap_dst2darr, pid);
  assert (p_list && *(tiz_vector_t **) p_list);
  p_list = *(tiz_vector_t **) p_list;

  TIZ_LOGN (TIZ_TRACE, hdl, "HEADER [%p] BUFFER [%p] PID [%d] "
            "list size [%d] buf count [%d]", ap_hdr, ap_hdr->pBuffer, pid,
            tiz_vector_length (p_list), tiz_port_buffer_count (ap_port));

  assert (tiz_vector_length (p_list) < tiz_port_buffer_count (ap_port));

  if (OMX_ErrorNone != tiz_vector_push_back (p_list, &ap_hdr))
    {
      return -1;
    }
  else
    {
      assert (tiz_vector_length (p_list) <= tiz_port_buffer_count (ap_port));

      return tiz_vector_length (p_list);
    }
}

static OMX_S32
clear_hdr_lst (tiz_vector_t * ap_hdr_lst, OMX_U32 a_pid)
{
  tiz_vector_t *p_list = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_S32 i, hdr_count = 0;

  assert (NULL != ap_hdr_lst);
  assert (tiz_vector_length (ap_hdr_lst) >= a_pid);

  p_list = tiz_vector_at (ap_hdr_lst, a_pid);
  assert (p_list && *(tiz_vector_t **) p_list);
  p_list = *(tiz_vector_t **) p_list;

  hdr_count = tiz_vector_length (p_list);
  for (i = 0; i < hdr_count; ++i)
    {
      p_hdr = get_header (p_list, i);
      tiz_clear_header (p_hdr);
    }

  return hdr_count;
}

static OMX_ERRORTYPE
append_buflsts (tiz_vector_t * ap_dst2darr,
                const tiz_vector_t * ap_srclst, OMX_U32 a_pid)
{
  tiz_vector_t *p_list = NULL;
  assert (NULL != ap_dst2darr);
  assert (NULL != ap_srclst);
  assert (tiz_vector_length (ap_dst2darr) >= a_pid);

  p_list = tiz_vector_at (ap_dst2darr, a_pid);
  assert (p_list && *(tiz_vector_t **) p_list);
  p_list = *(tiz_vector_t **) p_list;
  assert (tiz_vector_length (p_list) == 0);

  return tiz_vector_append (p_list, ap_srclst);
}

static OMX_ERRORTYPE
check_pid (const struct tizkernel *ap_obj, OMX_U32 a_pid)
{
  assert (NULL != ap_obj);

  if (a_pid >= tiz_vector_length (ap_obj->p_ports_))
    {
      TIZ_LOGN (TIZ_ERROR, tiz_servant_super_get_hdl (tizkernel, ap_obj),
                "[OMX_ErrorBadPortIndex] : port [%d]...", a_pid);
      return OMX_ErrorBadPortIndex;
    }

  return OMX_ErrorNone;
}

static inline tiz_kernel_msg_t *
init_kernel_message (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                     tiz_kernel_msg_class_t a_msg_class)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  tiz_kernel_msg_t *p_msg = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (a_msg_class < ETIZKernelMsgMax);

  if (NULL == (p_msg = tiz_servant_init_msg (p_obj, sizeof (tiz_kernel_msg_t))))
    {
      TIZ_LOGN (TIZ_TRACE, ap_hdl, "[OMX_ErrorInsufficientResources] : "
                "Could not allocate message [%s]",
                tiz_kernel_msg_to_str (a_msg_class));
    }
  else
    {
      p_msg->p_hdl = ap_hdl;
      p_msg->class = a_msg_class;
    }

  return p_msg;
}

static OMX_ERRORTYPE
enqueue_callback_msg (const void *ap_obj,
                      OMX_BUFFERHEADERTYPE * ap_hdr,
                      OMX_U32 a_pid, OMX_DIRTYPE a_dir)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  tiz_kernel_msg_t *p_msg = NULL;
  tiz_kernel_msg_callback_t *p_msg_cb = NULL;
  OMX_HANDLETYPE p_hdl = NULL;

  assert (NULL != ap_obj);

  p_hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);

  TIZ_LOGN (TIZ_TRACE, p_hdl, "Enqueue msg callback : HEADER [%p] BUFFER [%p] "
            "PID [%d] DIR [%s]", ap_hdr, ap_hdr ? ap_hdr->pBuffer : NULL,
            a_pid, tiz_dir_to_str (a_dir));

  if (NULL == (p_msg = init_kernel_message (p_obj, p_hdl,
                                            ETIZKernelMsgCallback)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_cb = &(p_msg->cb);
  p_msg_cb->p_hdr = ap_hdr;
  p_msg_cb->pid = a_pid;
  p_msg_cb->dir = a_dir;

  return tiz_servant_enqueue (ap_obj, p_msg, 1);
}

static inline OMX_U32
cmd_to_priority (OMX_COMMANDTYPE a_cmd)
{
  OMX_U32 prio = 0;

  switch (a_cmd)
    {
    case OMX_CommandStateSet:
    case OMX_CommandFlush:
    case OMX_CommandPortDisable:
    case OMX_CommandPortEnable:
    case OMX_CommandMarkBuffer:
      {
        prio = 0;
      }
      break;

    default:
      assert (0);
      break;
    };

  return prio;
}

static OMX_ERRORTYPE
propagate_ingress (void *ap_obj, OMX_U32 a_pid)
{
  struct tizkernel *p_obj = ap_obj;
  struct tizservant *base = ap_obj;
  void *p_prc = NULL;
  tiz_vector_t *p_list = NULL;
  OMX_PTR p_port = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_S32 i = 0;
  OMX_S32 j = 0;
  OMX_S32 nbufs = 0;
  OMX_U32 pid = 0;
  OMX_DIRTYPE pdir = OMX_DirMax;
  OMX_S32 nports = 0;

  assert (NULL != ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);
  p_prc = tiz_get_prc (base->p_hdl_);

  do
    {
      /* Grab the port */
      pid = ((OMX_ALL != a_pid) ? a_pid : i);
      p_port = get_port (p_obj, pid);

      /* Get port direction */
      pdir = tiz_port_dir (p_port);

      /* Grab the port's ingress list */
      p_list = get_ingress_lst (p_obj, pid);
      TIZ_LOGN (TIZ_TRACE, tiz_servant_get_hdl (p_obj),
                "port [%d]'s ingress list length [%d]...",
                pid, tiz_vector_length (p_list));

      nbufs = tiz_vector_length (p_list);
      for (j = 0; j < nbufs; ++j)
        {
          /* Retrieve the header... */
          p_hdr = get_header (p_list, j);

          TIZ_LOGN (TIZ_TRACE, tiz_servant_get_hdl (p_obj),
                    "Dispatching HEADER [%p] BUFFER [%p]",
                    p_hdr, p_hdr->pBuffer);

          tiz_clear_header (p_hdr);

          /* ... delegate to the processor... */
          if (OMX_DirInput == pdir)
            {
              tiz_api_EmptyThisBuffer (p_prc, base->p_hdl_, p_hdr);
            }
          else
            {
              tiz_api_FillThisBuffer (p_prc, base->p_hdl_, p_hdr);
            }

          /* ... and keep the header in the list. */
        }

      ++i;
    }
  while (OMX_ALL == pid && i < nports);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
transfer_mark (void *ap_obj, const OMX_MARKTYPE * ap_mark)
{
  const struct tizkernel *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_S32 nports = 0;
  OMX_PTR p_port = NULL;
  OMX_S32 i = 0;

  assert (NULL != ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  for (i = 0; i < nports && OMX_ErrorNone == rc; ++i)
    {
      p_port = get_port (p_obj, i);

      if (OMX_DirOutput == tiz_port_dir (p_port))
        {
          rc = tiz_port_store_mark (p_port, ap_mark, OMX_FALSE);
        }
    }

  return rc;
}

static OMX_ERRORTYPE
process_marks (void *ap_obj, OMX_BUFFERHEADERTYPE * ap_hdr, OMX_U32 a_pid,
               OMX_COMPONENTTYPE * ap_hdl)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_S32 nports = 0;
  OMX_PTR p_port = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdr);
  assert (NULL != ap_hdl);

  nports = tiz_vector_length (p_obj->p_ports_);

  assert (a_pid < nports);

  p_port = get_port (p_obj, a_pid);

  /* Look for buffer marks to be signalled or propagated */
  if (ap_hdr->hMarkTargetComponent)
    {
      /* See if this component is the buffer mark target component... */
      if (ap_hdr->hMarkTargetComponent == ap_hdl)
        {
          /* Return the mark to the IL Client */
          tiz_servant_issue_event (ap_obj, OMX_EventMark, 0, 0,
                                  ap_hdr->pMarkData);

          /* Remove the mark from the header as it has been delivered */
          /* to the client... */
          ap_hdr->hMarkTargetComponent = 0;
          ap_hdr->pMarkData = 0;

        }
      else
        {
          /* Buffer mark propagation logic */
          /* If port is output, do nothing */
          /* If port is input, transfer its mark to all the output ports in the
           * component */
          const OMX_DIRTYPE dir = tiz_port_dir (p_port);
          if (dir == OMX_DirInput)
            {
              const OMX_MARKTYPE mark = {
                ap_hdr->hMarkTargetComponent,
                ap_hdr->pMarkData
              };
              rc = transfer_mark (ap_obj, &mark);

              /* Remove the mark from the processed header... */
              ap_hdr->hMarkTargetComponent = 0;
              ap_hdr->pMarkData = 0;
            }
        }
    }

  else
    {
      /* No mark found. If port is input, nothing to do. */
      /* If port if output, mark the buffer, if any marks available... */
      const OMX_DIRTYPE dir = tiz_port_dir (p_port);
      if (dir == OMX_DirOutput)
        {
          /* NOTE: tiz_port_mark_buffer returns OMX_ErrorNone if the port marked
           * the buffer with one of its own marks */
          if (OMX_ErrorNone == (rc = tiz_port_mark_buffer (p_port, ap_hdr)))
            {
              /* Successfully complete here the OMX_CommandMarkBuffer command */
              complete_mark_buffer (p_obj, p_port, a_pid, OMX_ErrorNone);
            }
          else
            {
              /* These two return codes are not actual errors. */
              if (OMX_ErrorNoMore == rc || OMX_ErrorNotReady == rc)
                {
                  rc = OMX_ErrorNone;
                }
            }
        }
    }

  return rc;
}

static OMX_ERRORTYPE
flush_marks (void *ap_obj, OMX_PTR ap_port)
{
  struct tizkernel *p_obj = ap_obj;
  struct tizport *p_port = ap_port;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE hdr;

  assert (NULL != ap_obj);
  assert (NULL != p_port);

  /* Use a dummy header to flush all marks in the port */
  do
    {
      hdr.hMarkTargetComponent = NULL;
      hdr.pMarkData = NULL;
      /* tiz_port_mark_buffer returns OMX_ErrorNone if the port owned the
       * mark. If the mark is not owned, it returns OMX_ErrorNotReady. If no
       * marks found, it returns OMX_ErrorNoMore */
      if (OMX_ErrorNone == (rc = tiz_port_mark_buffer (p_port, &hdr)))
        {
          /* Need to complete the mark buffer command with an error */
          complete_mark_buffer (p_obj, p_port, tiz_port_index (p_port),
                                OMX_ErrorPortUnpopulated);
        }
    }
  while (OMX_ErrorNoMore != rc);

  return rc;
}

static OMX_ERRORTYPE
flush_egress (void *ap_obj, OMX_U32 a_pid, OMX_BOOL a_clear)
{
  struct tizkernel *p_obj = ap_obj;
  struct tizservant *p_base = ap_obj;
  tiz_vector_t *p_list = NULL;
  OMX_PTR p_port = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_S32 i = 0;
  OMX_U32 pid = 0;
  OMX_DIRTYPE pdir = OMX_DirMax;
  OMX_HANDLETYPE hdl = NULL;
  OMX_HANDLETYPE thdl = NULL;
  OMX_S32 nports = 0;

  assert (NULL != ap_obj);

  nports = tiz_vector_length (p_obj->p_ports_);

  hdl = p_base->p_hdl_;
  assert (NULL != hdl);

  do
    {
      /* Grab the port */
      pid = ((OMX_ALL != a_pid) ? a_pid : i);
      p_port = get_port (p_obj, pid);

      /* Get port direction and tunnel info */
      pdir = tiz_port_dir (p_port);
      thdl = tiz_port_get_tunnel_comp (p_port);

      /* Grab the port's egress list */
      p_list = get_egress_lst (p_obj, pid);

      TIZ_LOGN (TIZ_TRACE, hdl, "pid [%d] loop index=[%d] egress length [%d] "
                "- thdl [%p]...", pid, i, tiz_vector_length (p_list), thdl);

      while (tiz_vector_length (p_list) > 0)
        {
          /* Retrieve the header... */
          p_hdr = get_header (p_list, 0);

          TIZ_LOGN (TIZ_TRACE, hdl, "HEADER [%p] BUFFER [%p]", p_hdr,
                    p_hdr->pBuffer);

          /* ... issue the callback... */
          {

            /* If it's an input port and allocator, ask the port to
             * allocate the actual buffer, in case pre-announcements have
             * been disabled on this port. This function call has no effect
             * if pre-announcements are enabled on the port. */
            if (OMX_DirInput == pdir && TIZ_PORT_IS_ALLOCATOR (p_port))
              {
                tiz_port_populate_header (p_port, hdl, p_hdr);
              }

            /* Propagate buffer marks... */
            process_marks (p_obj, p_hdr, pid, hdl);

            if (a_clear)
              {
                tiz_clear_header (p_hdr);
              }
            else
              {
                /* Automatically report EOS event on output ports, but only
                 * once...  */
                if (p_hdr->nFlags & OMX_BUFFERFLAG_EOS
                    && OMX_DirOutput == pdir && p_obj->eos_ == OMX_FALSE)
                  {
                    TIZ_LOGN (TIZ_NOTICE, hdl, "OMX_BUFFERFLAG_EOS on "
                              "port [%d]...", pid);

                    /* ... flag EOS ... */
                    p_obj->eos_ = OMX_TRUE;
                    tiz_servant_issue_event ((OMX_PTR) ap_obj,
                                            OMX_EventBufferFlag,
                                            pid, p_hdr->nFlags, NULL);
                  }
              }

            /* get rid of the buffer */
            tiz_servant_issue_buf_callback ((OMX_PTR) ap_obj, p_hdr,
                                           pid, pdir, thdl);
            /* ... and delete it from the list. */
            tiz_vector_erase (p_list, 0, 1);

          }

        }
      ++i;
    }
  while (OMX_ALL == a_pid && i < nports);

  return OMX_ErrorNone;
}

static void
init_ports_and_lists (void *ap_obj)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_PORT_PARAM_TYPE null_param = {
    sizeof (OMX_PORT_PARAM_TYPE),
    {.nVersion = OMX_VERSION}
    ,
    0,
    0
  };

  tiz_vector_init (&(p_obj->p_ports_), sizeof (OMX_PTR));
  tiz_vector_init (&(p_obj->p_ingress_), sizeof (tiz_vector_t *));
  tiz_vector_init (&(p_obj->p_egress_), sizeof (tiz_vector_t *));

  p_obj->p_cport_ = NULL;
  p_obj->p_proc_ = NULL;
  p_obj->eos_ = OMX_FALSE;
  p_obj->rm_ = 0;
  p_obj->rm_cbacks_.pf_waitend = &wait_complete;
  p_obj->rm_cbacks_.pf_preempt = &preemption_req;
  p_obj->rm_cbacks_.pf_preempt_end = &preemption_complete;
  p_obj->audio_init_ = null_param;
  p_obj->image_init_ = null_param;
  p_obj->video_init_ = null_param;
  p_obj->other_init_ = null_param;
  p_obj->cmd_completion_count_ = 0;
  p_obj->accept_use_buffer_notified_ = OMX_FALSE;
  p_obj->accept_buffer_exchange_notified_ = OMX_FALSE;
  p_obj->may_transition_exe2idle_notified_ = OMX_FALSE;
}

static void
deinit_ports_and_lists (void *ap_obj)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_PTR *pp_port = NULL;
  tiz_vector_t *p_list = NULL;

  /* delete the config port */
  factory_delete (p_obj->p_cport_);
  p_obj->p_cport_ = NULL;

  /* delete all regular (non-config) ports */
  while (tiz_vector_length (p_obj->p_ports_) > 0)
    {
      pp_port = tiz_vector_back (p_obj->p_ports_);
      assert (pp_port);
      factory_delete (*pp_port);
      tiz_vector_pop_back (p_obj->p_ports_);
    }
  tiz_vector_destroy (p_obj->p_ports_);
  p_obj->p_ports_ = NULL;

  /* delete the ingress and egress lists */
  while (tiz_vector_length (p_obj->p_ingress_) > 0)
    {
      p_list = *(tiz_vector_t **) tiz_vector_back (p_obj->p_ingress_);
      tiz_vector_clear (p_list);
      tiz_vector_destroy (p_list);
      tiz_vector_pop_back (p_obj->p_ingress_);
    }
  tiz_vector_destroy (p_obj->p_ingress_);
  p_obj->p_ingress_ = NULL;

  while (tiz_vector_length (p_obj->p_egress_) > 0)
    {
      p_list = *(tiz_vector_t **) tiz_vector_back (p_obj->p_egress_);
      tiz_vector_clear (p_list);
      tiz_vector_destroy (p_list);
      tiz_vector_pop_back (p_obj->p_egress_);
    }
  tiz_vector_destroy (p_obj->p_egress_);
  p_obj->p_egress_ = NULL;
}

static OMX_BOOL
all_populated (const void *ap_obj)
{
  const struct tizkernel *p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_PTR p_port = NULL;
  OMX_U32 i = 0;
  OMX_HANDLETYPE p_hdl = NULL;
  (void) p_hdl;

  assert (NULL != ap_obj);

  nports = tiz_vector_length (p_obj->p_ports_);
  p_hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);

  for (i = 0; i < nports; ++i)
    {
      p_port = get_port (p_obj, i);

      TIZ_LOGN (TIZ_TRACE, p_hdl, "PORT [%d] is [%s] and [%s]", i,
                TIZ_PORT_IS_ENABLED (p_port) ? "ENABLED" : "NOT ENABLED",
                TIZ_PORT_IS_POPULATED (p_port) ? "POPULATED" :
                "NOT POPULATED");

      if (TIZ_PORT_IS_ENABLED (p_port) && !(TIZ_PORT_IS_POPULATED (p_port)))
        {
          TIZ_LOGN (TIZ_TRACE, p_hdl, "ALL ENABLED ports are populated = "
                    "[OMX_FALSE]");
          return OMX_FALSE;
        }

    }

  TIZ_LOGN (TIZ_TRACE, p_hdl, "ALL ENABLED ports are populated = [OMX_TRUE]");

  return OMX_TRUE;
}

static OMX_BOOL
all_depopulated (const void *ap_obj)
{
  const struct tizkernel *p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_PTR p_port = NULL;
  OMX_U32 i;
  OMX_HANDLETYPE p_hdl = NULL;
  (void) p_hdl;

  assert (NULL != ap_obj);

  nports = tiz_vector_length (p_obj->p_ports_);
  p_hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);

  for (i = 0; i < nports; ++i)
    {
      p_port = get_port (p_obj, i);

      if (tiz_port_buffer_count (p_port))
        {
          TIZ_LOGN (TIZ_TRACE, p_hdl, "ALL DEPOPULATED = [OMX_FALSE]");
          return OMX_FALSE;
        }
    }

  TIZ_LOGN (TIZ_TRACE, p_hdl, "ALL DEPOPULATED = [OMX_TRUE]");

  return OMX_TRUE;
}

static OMX_BOOL
all_buffers_returned (void *ap_obj)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_PTR p_port = NULL;
  tiz_vector_t *p_list = NULL;
  OMX_U32 i;
  OMX_S32 nbuf = 0, nbufin = 0;
  OMX_HANDLETYPE p_hdl = NULL;
  (void) p_hdl;

  assert (NULL != ap_obj);

  nports = tiz_vector_length (p_obj->p_ports_);
  p_hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);

  for (i = 0; i < nports; ++i)
    {
      p_port = get_port (p_obj, i);

      nbuf = tiz_port_buffer_count (p_port);

      if (TIZ_PORT_IS_DISABLED (p_port) || !nbuf)
        {
          continue;
        }

      if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
        {
          p_list = get_ingress_lst (p_obj, i);

          if ((nbufin = tiz_vector_length (p_list)) != nbuf)
            {
              int j = 0;
              OMX_BUFFERHEADERTYPE *p_hdr = NULL;

              TIZ_LOGN (TIZ_TRACE, p_hdl, "Port [%d] : awaiting buffers (only "
                        " [%d] out of [%d] have arrived)", i, nbufin, nbuf);

              for (j = 0; j < nbufin; ++j)
                {
                  p_hdr = get_header (p_list, j);
                  
                  TIZ_LOGN (TIZ_TRACE, p_hdl, "HEADER [%p] BUFFER [%p]", p_hdr,
                            p_hdr->pBuffer);
                }

              return OMX_FALSE;
            }
        }
      else
        {
          const OMX_S32 claimed_count = TIZ_PORT_GET_CLAIMED_COUNT (p_port);
          if (claimed_count > 0)
            {
              TIZ_LOGN (TIZ_TRACE, p_hdl, "Port [%d] : still need to "
                        "return [%d] buffers", i, claimed_count);
              return OMX_FALSE;
            }
        }

    }

  TIZ_LOGN (TIZ_TRACE, p_hdl, "ALL BUFFERS returned = [TRUE]...");

  p_obj->eos_ = OMX_FALSE;

  return OMX_TRUE;
}

static OMX_ERRORTYPE
dispatch_state_set (void *ap_obj, OMX_HANDLETYPE ap_hdl,
                    tiz_kernel_msg_sendcommand_t * ap_msg_sc)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_STATETYPE now = OMX_StateMax;
  OMX_BOOL done = OMX_FALSE;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_msg_sc);

  /* Obtain the current state */
  tiz_api_GetState (tiz_get_fsm (ap_hdl), ap_hdl, &now);

  TIZ_LOGN (TIZ_DEBUG, ap_hdl, "Requested transition [%s] -> [%s]",
            tiz_fsm_state_to_str (now),
            tiz_fsm_state_to_str (ap_msg_sc->param1));

  switch (ap_msg_sc->param1)
    {
    case OMX_StateLoaded:
      {
        if (OMX_StateIdle == now)
          {
            rc = tiz_servant_deallocate_resources (ap_obj);

            release_rm_resources (p_obj, ap_hdl);

            /* Uninitialize the Resource Manager hdl */
            deinit_rm (p_obj, ap_hdl);

            done = (OMX_ErrorNone == rc &&
                    all_depopulated (p_obj)) ? OMX_TRUE : OMX_FALSE;

            /* TODO: If all ports are depopulated, kick off removal of buffer
             * callbacks from kernel and proc servants queues  */

          }
        else if (OMX_StateWaitForResources == now)
          {
            done = OMX_TRUE;
          }
        else if (OMX_StateLoaded == now)
          {
            /* TODO : Need to review whe this situation would occur  */
            return OMX_ErrorSameState;
          }
        else
          {
            assert (0);
          }
        break;
      }

    case OMX_StateWaitForResources:
      {
        done = OMX_TRUE;
        break;
      }

    case OMX_StateIdle:
      {
        if (OMX_StateLoaded == now)
          {
            /* Before allocating any resources, we need to initialize the
             * Resource Manager hdl */
            init_rm (p_obj, ap_hdl);

            rc = acquire_rm_resources (p_obj, ap_hdl);

            if (OMX_ErrorNone == rc)
              {
                rc = tiz_servant_allocate_resources (ap_obj, OMX_ALL);
              }

            done = (OMX_ErrorNone == rc &&
                    all_populated (p_obj)) ? OMX_TRUE : OMX_FALSE;

          }
        else if (OMX_StateExecuting == now || OMX_StatePause == now)
          {
            rc = tiz_servant_stop_and_return (ap_obj);
            done = (OMX_ErrorNone == rc && all_buffers_returned
                    ((struct tizkernel *) p_obj)) ? OMX_TRUE : OMX_FALSE;

          }
        else if (OMX_StateIdle == now)
          {
            /* TODO : review when this situation would occur  */
            TIZ_LOGN (TIZ_WARN, ap_hdl, "Ignoring transition [%s] -> [%s]",
                      tiz_fsm_state_to_str (now),
                      tiz_fsm_state_to_str (ap_msg_sc->param1));
          }
        else
          {
            assert (0);
          }
        break;
      }

    case OMX_StateExecuting:
      {
        if (OMX_StateIdle == now)
          {
            rc = tiz_servant_prepare_to_transfer (ap_obj, OMX_ALL);

            done = OMX_TRUE;
          }
        else if (OMX_StatePause == now)
          {
            /* Enqueue a dummy callback msg to be processed ...   */
            /* ...in case there are headers present in the egress lists... */
            rc = enqueue_callback_msg (p_obj, NULL, 0, OMX_DirMax);
            done = OMX_TRUE;
          }
        else if (OMX_StateExecuting == now)
          {
            rc = tiz_servant_transfer_and_process (ap_obj, OMX_ALL);
            done = OMX_FALSE;
          }
        else
          {
            assert (0);
          }
        break;
      }

    case OMX_StatePause:
      {
        /* TODO: Consider the removal of buffer indications from the processor
         * queue */
        done = OMX_TRUE;
        break;
      }

    default:
      {
        TIZ_LOG (TIZ_TRACE, "Unknown state [%s] [%d]",
                 tiz_fsm_state_to_str (ap_msg_sc->param1), ap_msg_sc->param1);
        assert (0);
        break;
      }
    };

  if (OMX_ErrorNone == rc && OMX_TRUE == done)
    {
      rc = tiz_fsm_complete_transition
        (tiz_get_fsm (ap_hdl), ap_obj, ap_msg_sc->param1);
    }

  return rc;
}

static OMX_ERRORTYPE
dispatch_port_disable (void *ap_obj, OMX_HANDLETYPE p_hdl,
                       tiz_kernel_msg_sendcommand_t * ap_msg_sc)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_U32 pid = 0;
  OMX_S32 nports = 0;
  OMX_S32 i = 0;
  OMX_S32 nbufs = 0;

  assert (NULL != p_obj);
  assert (NULL != p_hdl);

  nports = tiz_vector_length (p_obj->p_ports_);

  assert (NULL != ap_msg_sc);
  pid = ap_msg_sc->param1;

  /* Verify the port index.. */
  if ((OMX_ALL != pid) && (check_pid (p_obj, pid) != OMX_ErrorNone))
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Record here the number of times we need to notify the IL client */
  p_obj->cmd_completion_count_ = (OMX_ALL == ap_msg_sc->param1) ? nports : 1;

  do
    {
      pid = ((OMX_ALL != ap_msg_sc->param1) ? ap_msg_sc->param1 : i);
      p_port = get_port (p_obj, pid);

      /* If port is already disabled, simply notify the command completion */
      if (TIZ_PORT_IS_DISABLED (p_port))
        {
          complete_port_disable (p_obj, p_port, pid, OMX_ErrorNone);
          ++i;
          continue;
        }

      if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
        {
          /* Move buffers from egress to ingress */
          nbufs = move_to_ingress (p_obj, pid);
          if (nbufs < 0)
            {
              TIZ_LOGN (TIZ_ERROR, p_hdl, "[OMX_ErrorInsufficientResources] : "
                        "on port [%d] while moving buffers to ingress list",
                        pid);
              return OMX_ErrorInsufficientResources;
            }

          if (tiz_port_buffer_count (p_port) != nbufs)
            {
              /* Some of the buffers aren't back yet */
              TIZ_PORT_SET_GOING_TO_DISABLED (p_port);
            }
          else
            {
              tiz_vector_t *p_hdr_lst = tiz_port_get_hdrs_list (p_port);
              tiz_vector_t *p_hdr_lst_copy;
              tiz_vector_init (&(p_hdr_lst_copy),
                               sizeof (OMX_BUFFERHEADERTYPE *));
              tiz_vector_append (p_hdr_lst_copy, p_hdr_lst);

              /* Depopulate the tunnel... */
              if (OMX_ErrorNone == (rc = tiz_port_depopulate (p_port)))
                {
                  const OMX_S32 nhdrs = tiz_vector_length (p_hdr_lst_copy);
                  OMX_S32 i = 0;
                  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;

                  for (i = 0; i < nhdrs; ++i)
                    {
                      pp_hdr = tiz_vector_at (p_hdr_lst_copy, i);
                      assert (pp_hdr && *pp_hdr);

                      TIZ_LOGN (TIZ_TRACE, p_hdl, "port [%d] depopulated - "
                                "removing leftovers - nhdrs [%d] "
                                "HEADER [%p] BUFFER [%p]...",
                                pid, nhdrs, *pp_hdr, (*pp_hdr)->pBuffer);

                      tiz_servant_remove_from_queue
                        (ap_obj, &remove_buffer_from_servant_queue,
                         ETIZKernelMsgCallback, *pp_hdr);

                      {
                        const void *p_prc = tiz_get_prc (p_hdl);
                        /* NOTE : 2nd and 3rd parameters are dummy ones, the
                         * processor servant implementation of
                         * 'remove_from_queue' will replace them with its
                         * correct values */
                        tiz_servant_remove_from_queue (p_prc, NULL, 0, *pp_hdr);
                      }

                    }
                }

              tiz_vector_clear (p_hdr_lst_copy);
              tiz_vector_destroy (p_hdr_lst_copy);

              if (OMX_ErrorNone != rc)
                {
                  TIZ_LOGN (TIZ_ERROR, p_hdl, "[%s] depopulating port [%d]",
                            tiz_err_to_str (rc), pid);
                  return rc;
                }

              complete_port_disable (p_obj, p_port, pid, OMX_ErrorNone);
            }

        }
      else
        {
          if (tiz_port_buffer_count (p_port) > 0)
            {
              TIZ_PORT_SET_GOING_TO_DISABLED (p_port);

              /* Move headers from ingress to egress, ... */
              /* ....and clear their contents before doing that... */
              const OMX_S32 count = clear_hdr_lst (p_obj->p_ingress_, pid);

              if (count)
                {
                  if (0 > move_to_egress (p_obj, pid))
                    {
                      TIZ_LOGN (TIZ_ERROR, p_hdl,
                                "[OMX_ErrorInsufficientResources] : "
                                "on port [%d]...", pid);
                      rc = OMX_ErrorInsufficientResources;
                    }
                }

              if (OMX_ErrorNone == rc)
                {
                  /* ... and finally flush them so that they leave the
                   * component ...  */
                  rc = flush_egress (p_obj, pid, OMX_FALSE);
                }

              if (TIZ_PORT_GET_CLAIMED_COUNT (p_port) > 0)
                {
                  /* We need to wait until the processor relinquishes all the
                   * buffers it is currently holding. */

                  TIZ_LOGN (TIZ_TRACE, p_hdl, "port [%d] going to disabled - "
                            "claimed [%d]...", pid,
                            TIZ_PORT_GET_CLAIMED_COUNT (p_port));

                  /* Notify the processor servant... */
                  {
                    struct tizproc *p_prc = tiz_get_prc (p_hdl);
                    rc = tiz_api_SendCommand (p_prc, p_hdl,
                                             ap_msg_sc->cmd,
                                             pid, ap_msg_sc->p_cmd_data);
                  }
                }

            }
          else
            {
              TIZ_LOGN (TIZ_TRACE, p_hdl, "port [%d] is disabled...", pid);
              complete_port_disable (p_obj, p_port, pid, OMX_ErrorNone);
            }

        }

      ++i;

    }
  while (OMX_ALL == ap_msg_sc->param1 && i < nports);

  return complete_ongoing_transitions (p_obj, p_hdl);
}

static OMX_ERRORTYPE
dispatch_port_enable (void *ap_obj, OMX_HANDLETYPE p_hdl,
                      tiz_kernel_msg_sendcommand_t * ap_msg_pe)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_U32 pid = 0;
  OMX_S32 nports = 0;
  OMX_S32 i = 0;
  tiz_fsm_state_id_t now = EStateMax;

  assert (NULL != p_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  assert (NULL != p_hdl);
  now = tiz_fsm_get_substate (tiz_get_fsm (p_hdl));

  assert (NULL != ap_msg_pe);
  pid = ap_msg_pe->param1;

  TIZ_LOGN (TIZ_TRACE, tiz_servant_super_get_hdl (tizkernel, p_obj),
            "Requested port enable for PORT [%d]", pid);

  /* Verify the port index.. */
  if ((OMX_ALL != pid) && (check_pid (p_obj, pid) != OMX_ErrorNone))
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Record here the number of times we need to notify the IL client */
  p_obj->cmd_completion_count_ = (OMX_ALL == ap_msg_pe->param1) ? nports : 1;

  do
    {
      pid = ((OMX_ALL != ap_msg_pe->param1) ? ap_msg_pe->param1 : i);
      p_port = get_port (p_obj, pid);

      if (TIZ_PORT_IS_ENABLED (p_port))
        {
          /* If port is already enabled, must notify the command completion */
          complete_port_enable (p_obj, p_port, pid, OMX_ErrorNone);
          ++i;
          continue;
        }

      if (OMX_ErrorNone == rc)
        {
          if (EStateWaitForResources == now || EStateLoaded == now)
            {
              /* Complete OMX_CommandPortEnable on this port now */
              complete_port_enable (p_obj, p_port, pid, OMX_ErrorNone);
            }
          else
            {
              TIZ_PORT_SET_GOING_TO_ENABLED (p_port);
              if (OMX_ErrorNone
                  == (rc = tiz_servant_allocate_resources (ap_obj, pid)))
                {
                  if (ESubStateLoadedToIdle == now)
                    {
                      if (all_populated (p_obj))
                        {
                          /* Complete transition to OMX_StateIdle */
                          rc = tiz_fsm_complete_transition
                            (tiz_get_fsm (p_hdl), ap_obj, OMX_StateIdle);
                        }
                    }
                  else if (EStateExecuting == now)
                    {
                      rc = tiz_servant_transfer_and_process (ap_obj, pid);
                    }
                }
            }
        }

      ++i;
    }
  while (OMX_ALL == ap_msg_pe->param1 && i < nports && OMX_ErrorNone == rc);

  return rc;
}

static OMX_ERRORTYPE
dispatch_port_flush (void *ap_obj, OMX_HANDLETYPE ap_hdl,
                     tiz_kernel_msg_sendcommand_t * ap_msg_pf)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_U32 pid = 0;
  OMX_S32 nports = 0;
  OMX_S32 i = 0;
  OMX_S32 nbufs = 0;
  tiz_fsm_state_id_t now = EStateMax;

  assert (NULL != ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  assert (NULL != ap_hdl);
  now = tiz_fsm_get_substate (tiz_get_fsm (ap_hdl));

  assert (NULL != ap_msg_pf);
  pid = ap_msg_pf->param1;

  TIZ_LOGN (TIZ_TRACE, ap_hdl, "Requested port flush on PORT [%d]", pid);

  /* Verify the port index */
  if ((OMX_ALL != pid) && (check_pid (p_obj, pid) != OMX_ErrorNone))
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Record here the number of times we need to notify the IL client */
  p_obj->cmd_completion_count_ = (OMX_ALL == ap_msg_pf->param1) ? nports : 1;

  /* My Flush matrix */
  /*  |---------------+---------------+---------+--------------------------| */
  /*  | Tunneled/     | Supplier/     | Input/  | Outcome                  | */
  /*  | Non-Tunneled? | Non-Supplier? | Output? |                          | */
  /*  |---------------+---------------+---------+--------------------------| */
  /*  | NT            | S             | I       | Return                   | */
  /*  | NT            | S             | O       | Return + zero nFilledLen | */
  /*  |---------------+---------------+---------+--------------------------| */
  /*  | T             | S             | I       | Return + zero nFilledLen | */
  /*  | T             | S             | O       | Hold + zero nFilledLen   | */
  /*  |---------------+---------------+---------+--------------------------| */
  /*  | NT            | NS            | I       | Return                   | */
  /*  | NT            | NS            | O       | Return + zero nFilledLen | */
  /*  |---------------+---------------+---------+--------------------------| */
  /*  | T             | NS            | I       | Return                   | */
  /*  | T             | NS            | O       | Return + zero nFilledLen | */
  /*  |---------------+---------------+---------+--------------------------| */

  do
    {
      pid = ((OMX_ALL != ap_msg_pf->param1) ? ap_msg_pf->param1 : i);
      p_port = get_port (p_obj, pid);

      if (tiz_port_buffer_count (p_port) && TIZ_PORT_IS_ENABLED (p_port)
          && (now == EStateExecuting || now == EStatePause))
        {

          if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
            {
              if (OMX_DirInput == tiz_port_dir (p_port))
                {
                  /* INPUT PORT: Move input headers from ingress to egress,
                   * ... */
                  /* ....and clear their contents before doing that... */
                  const OMX_S32 count = clear_hdr_lst (p_obj->p_ingress_, pid);

                  if (count)
                    {
                      nbufs = move_to_egress (p_obj, pid);
                      if (nbufs < 0)
                        {
                          TIZ_LOGN (TIZ_ERROR, ap_hdl,
                                    "[OMX_ErrorInsufficientResources] : "
                                    "on port [%d]...", pid);
                          rc = OMX_ErrorInsufficientResources;
                        }
                    }

                  if (OMX_ErrorNone == rc)
                    {
                      /* ... and finally flush them so that they go
                       * upstream...  */
                      rc = flush_egress (p_obj, pid, OMX_FALSE);
                    }

                }

              else
                {
                  /* OUTPUT PORT: Move output headers from egress to
                   * ingress... */
                  /* ....and clear their contents before doing that */
                  const OMX_S32 count = clear_hdr_lst (p_obj->p_egress_, pid);
                  if (count)
                    {
                      nbufs = move_to_ingress (p_obj, pid);
                      if (nbufs < 0)
                        {
                          TIZ_LOGN (TIZ_ERROR, ap_hdl,
                                    "[OMX_ErrorInsufficientResources] : "
                                    "on port [%d]...", pid);
                          rc = OMX_ErrorInsufficientResources;
                        }
                    }
                  /* Buffers are kept */
                }

            }

          else
            {
              /* Move (input or output) headers from ingress to egress... */
              /* ....but clear only output headers ... */
              if (OMX_DirInput == tiz_port_dir (p_port))
                {
                  clear_hdr_lst (p_obj->p_egress_, pid);
                }

              nbufs = move_to_egress (p_obj, pid);
              if (nbufs < 0)
                {
                  TIZ_LOGN (TIZ_ERROR, ap_hdl, "[OMX_ErrorInsufficientResources] : "
                            "on port [%d]...", pid);
                  rc = OMX_ErrorInsufficientResources;
                }

              if (OMX_ErrorNone == rc)
                {
                  /* ... and finally flush them ...  */
                  rc = flush_egress (p_obj, pid, OMX_FALSE);
                }

            }
        }

      if (OMX_ErrorNone != rc)
        {
          /* Complete the command with an error event */
          TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s] : Flush command failed on port [%d]...",
                    tiz_err_to_str (rc), pid);
          complete_port_flush (p_obj, p_port, pid, rc);
        }
      else
        {
          /* Check if the processor holds a buffer. Will need to wait until any
           * buffers held by the processor are relinquished.  */

          TIZ_LOGN (TIZ_TRACE, ap_hdl, "port [%d] claimed_count = [%d]...",
                    pid, TIZ_PORT_GET_CLAIMED_COUNT (p_port));

          if (TIZ_PORT_GET_CLAIMED_COUNT (p_port) == 0)
            {
              /* There are no buffers with the processor, then we can
               * sucessfully complete the OMX_CommandFlush command here. */
              complete_port_flush (p_obj, p_port, pid, OMX_ErrorNone);
            }
          else
            {
              /* We need to wait until the processor relinquishes all the
               * buffers it is currently holding. */
              TIZ_PORT_SET_FLUSH_IN_PROGRESS (p_port);

              /* Notify the processor servant... */
              {
                struct tizproc *p_prc = tiz_get_prc (ap_hdl);
                rc = tiz_api_SendCommand (p_prc, ap_hdl,
                                         ap_msg_pf->cmd,
                                         pid, ap_msg_pf->p_cmd_data);
              }
            }
        }

      ++i;
    }
  while (OMX_ALL == ap_msg_pf->param1 && i < nports);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
dispatch_mark_buffer (void *ap_obj, OMX_HANDLETYPE p_hdl,
                      tiz_kernel_msg_sendcommand_t * ap_msg_sc)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_PTR p_port = NULL;
  const OMX_U32 pid = ap_msg_sc->param1;
  const OMX_MARKTYPE *p_mark = ap_msg_sc->p_cmd_data;

  /* TODO : Check whether pid can be OMX_ALL. For now assume it can't */

  /* Record here the number of times we need to notify the IL client */
  /*   assert (p_obj->cmd_completion_count_ == 0); */
  /*   p_obj->cmd_completion_count_ = 1; */

  p_port = get_port (p_obj, pid);

  /* Simply enqueue the mark in the port... */
  return tiz_port_store_mark (p_port, p_mark, OMX_TRUE); /* The port owns this
                                                         * mark */

}

static OMX_ERRORTYPE
dispatch_cb (void *ap_obj, OMX_PTR ap_msg)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_kernel_msg_t *p_msg = ap_msg;
  tiz_kernel_msg_callback_t *p_msg_cb = NULL;
  tiz_fsm_state_id_t now = OMX_StateMax;
  tiz_vector_t *p_egress_lst = NULL;
  OMX_PTR p_port = NULL;
  OMX_S32 claimed_count = 0;
  OMX_HANDLETYPE p_hdl = NULL;
  OMX_U32 pid = 0;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  assert (NULL != p_obj);
  assert (NULL != p_msg);

  p_hdl = p_msg->p_hdl;
  assert (NULL != p_hdl);

  p_msg_cb = &(p_msg->cb);
  pid = p_msg_cb->pid;
  p_hdr = p_msg_cb->p_hdr;

  now = tiz_fsm_get_substate (tiz_get_fsm (p_hdl));

  TIZ_LOGN (TIZ_TRACE, p_hdl, "HEADER [%p] STATE [%s] ", p_hdr,
            tiz_fsm_state_to_str (now));

  /* Find the port.. */
  p_port = get_port (p_obj, pid);

  /* Grab the port's egress list */
  p_egress_lst = get_egress_lst (p_obj, pid);

  /* Buffers are not allowed to leave the component in OMX_StatePause, unless
   * the port is being explicitly flushed by the IL Client. If the port is not
   * being flushed and the component is paused, a dummy callback msg will be
   * added to the queue once the component transitions from OMX_StatePause to
   * OMX_StateExecuting. */
  if (EStatePause == now && !TIZ_PORT_IS_BEING_FLUSHED (p_port))
    {
      TIZ_LOGN (TIZ_TRACE, p_hdl, "OMX_StatePause -> Deferring HEADER [%p]",
                p_hdr);

      /* TODO: Double check whether this hack is needed anymore */
      if (NULL == p_hdr && OMX_DirMax == p_msg_cb->dir)
        {
          TIZ_LOGN (TIZ_TRACE, p_hdl, "Enqueueing another dummy callback...");
          rc = enqueue_callback_msg (p_obj, NULL, 0, OMX_DirMax);
        }
      else
        {
          /* ...add the header to the egress list... */
          if (OMX_ErrorNone
              != (rc = tiz_vector_push_back (p_egress_lst, &p_hdr)))
            {
              TIZ_LOGN (TIZ_ERROR, p_hdl, "[%s] : Could not add HEADER [%p] "
                        "to port [%d] egress list", tiz_err_to_str (rc), p_hdr,
                        pid);
            }
          else
            {
              /* Now decrement by one the port's claimed buffers count */
              claimed_count = TIZ_PORT_DEC_CLAIMED_COUNT (p_port);
            }
        }

      return rc;
    }

  if (NULL == p_hdr && OMX_DirMax == p_msg_cb->dir)
    {
      /* If this is a dummy callback, simply flush the lists and return */
      return flush_egress (p_obj, OMX_ALL, OMX_FALSE);
    }

  /* ...add the header to the egress list... */
  if (OMX_ErrorNone != (rc = tiz_vector_push_back (p_egress_lst, &p_hdr)))
    {
      TIZ_LOGN (TIZ_ERROR, p_hdl, "[%s] : Could not add header [%p] to "
                "port [%d] egress list", tiz_err_to_str (rc), p_hdr, pid);
    }

  if (OMX_ErrorNone == rc)
    {
      /* Now decrement by one the port's claimed buffers count */
      claimed_count = TIZ_PORT_DEC_CLAIMED_COUNT (p_port);

      if ((ESubStateExecutingToIdle == now || ESubStatePauseToIdle == now)
          && TIZ_PORT_IS_ENABLED_TUNNELED_AND_SUPPLIER (p_port))
        {
          int nbufs = 0;
          /* If we are moving to Idle, move the buffers to ingress so they
             don't leave the component in the next step */
          nbufs = move_to_ingress (p_obj, pid);
          TIZ_LOGN (TIZ_ERROR, p_hdl, "nbufs [%d]", nbufs);
        }

      /* Here, we always flush the egress lists for ALL ports */
      if (OMX_ErrorNone != (rc = flush_egress (p_obj, OMX_ALL, OMX_FALSE)))
        {
          TIZ_LOGN (TIZ_ERROR, p_hdl, "[%s] : Could not flush the egress lists",
                    tiz_err_to_str (rc));
        }
    }

  /* Check in case we can complete at this point an ongoing flush or disable
   * command or state transition to OMX_StateIdle. */
  if (0 == claimed_count)
    {
      if (TIZ_PORT_IS_BEING_FLUSHED (p_port))
        {
          /* Notify flush complete */
          complete_port_flush (p_obj, p_port, pid, rc);
        }

      if (all_buffers_returned (p_obj))
        {
          tiz_kernel_tunneled_ports_status_t status =
            tiz_kernel_get_tunneled_ports_status (ap_obj, OMX_TRUE);

          if ((ESubStateExecutingToIdle == now || ESubStatePauseToIdle == now)
              && (ETIZKernelNoTunneledPorts == status
                  || ETIZKernelTunneledPortsMayInitiateExeToIdle == status))
            {
              /* complete state transition to OMX_StateIdle */
              rc = tiz_fsm_complete_transition
                (tiz_get_fsm (p_hdl), p_obj, OMX_StateIdle);
            }
        }

    }

  return rc;
}

static OMX_ERRORTYPE
dispatch_efb (void *ap_obj, OMX_PTR ap_msg, tiz_kernel_msg_class_t a_msg_class)
{
  struct tizkernel *p_obj = ap_obj;
  tiz_kernel_msg_t *p_msg = ap_msg;
  tiz_kernel_msg_emptyfillbuffer_t *p_msg_ef = NULL;
  tiz_fsm_state_id_t now = EStateMax;
  OMX_S32 nbufs = 0;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const OMX_DIRTYPE dir = a_msg_class == ETIZKernelMsgEmptyThisBuffer ?
    OMX_DirInput : OMX_DirOutput;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_U32 pid = 0;
  OMX_HANDLETYPE *p_hdl = NULL;

  assert (NULL != p_obj);
  assert (NULL != p_msg);

  p_msg_ef = &(p_msg->ef);
  assert (NULL != p_msg_ef);

  p_hdr = p_msg_ef->p_hdr;
  assert (NULL != p_hdr);

  p_hdl = p_msg->p_hdl;
  assert (NULL != p_hdl);

  now = tiz_fsm_get_substate (tiz_get_fsm (p_hdl));

  pid = a_msg_class == ETIZKernelMsgEmptyThisBuffer ?
    p_hdr->nInputPortIndex : p_hdr->nOutputPortIndex;

  TIZ_LOGN (TIZ_TRACE, p_hdl, "HEADER [%p] BUFFER [%p] PID [%d]",
            p_hdr, p_hdr->pBuffer, pid);

  if (check_pid (p_obj, pid) != OMX_ErrorNone)
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Retrieve the port... */
  p_port = get_port (p_obj, pid);

  /* Add this buffer to the ingress hdr list */
  if (0 > (nbufs = add_to_buflst (p_obj, p_obj->p_ingress_, p_hdr, p_port)))
    {
      TIZ_LOGN (TIZ_ERROR, p_hdl, "[OMX_ErrorInsufficientResources] : "
                "on port [%d] while adding buffer to ingress list", pid);
      return OMX_ErrorInsufficientResources;
    }

  assert (nbufs != 0);

  TIZ_LOGN (TIZ_TRACE, p_hdl, "ingress list length [%d]", nbufs);

  if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
    {
      if (TIZ_PORT_IS_BEING_DISABLED (p_port))
        {
          if (tiz_port_buffer_count (p_port) == nbufs)
            {
              tiz_vector_t *p_hdr_lst = tiz_port_get_hdrs_list (p_port);
              tiz_vector_t *p_hdr_lst_copy;
              tiz_vector_init (&(p_hdr_lst_copy),
                               sizeof (OMX_BUFFERHEADERTYPE *));
              tiz_vector_append (p_hdr_lst_copy, p_hdr_lst);

              /* All buffers are back... now free headers on the other end */
              if (OMX_ErrorNone == (rc = tiz_port_depopulate (p_port)))
                {
                  const OMX_S32 nhdrs = tiz_vector_length (p_hdr_lst_copy);
                  OMX_S32 i = 0;
                  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;

                  for (i = 0; i < nhdrs; ++i)
                    {
                      pp_hdr = tiz_vector_at (p_hdr_lst_copy, i);
                      assert (pp_hdr && *pp_hdr);

                      TIZ_LOGN (TIZ_TRACE, p_hdl, "port [%d] depopulated - "
                                "removing leftovers - nhdrs [%d] "
                                "HEADER [%p]...", pid, nhdrs, *pp_hdr);

                      tiz_servant_remove_from_queue (p_obj,
                                                    &remove_buffer_from_servant_queue,
                                                    ETIZKernelMsgCallback,
                                                    *pp_hdr);

                      {
                        const void *p_prc = tiz_get_prc (p_hdl);
                        /* NOTE : 2nd and 3rd parameters are dummy ones, the
                         * processor servant implementation of
                         * 'remove_from_queue' will replace them with its
                         * correct values */
                        tiz_servant_remove_from_queue (p_prc, NULL, 0, *pp_hdr);
                      }

                    }
                }

              tiz_vector_clear (p_hdr_lst_copy);
              tiz_vector_destroy (p_hdr_lst_copy);

              if (OMX_ErrorNone != rc)
                {
                  TIZ_LOGN (TIZ_ERROR, p_hdl, "[%s] depopulating pid [%d]",
                            tiz_err_to_str (rc), pid);
                  return rc;
                }

              complete_port_disable (p_obj, p_port, pid, OMX_ErrorNone);
            }

          /* TODO: */
          /* Clear header fields... */

          return OMX_ErrorNone;

        }

      if (ESubStateExecutingToIdle == now || ESubStatePauseToIdle == now)
        {
          tiz_kernel_tunneled_ports_status_t status =
            tiz_kernel_get_tunneled_ports_status (ap_obj, OMX_TRUE);

          if (all_buffers_returned (p_obj)
              && (ETIZKernelNoTunneledPorts == status
                  || ETIZKernelTunneledPortsMayInitiateExeToIdle == status))
            {
              TIZ_LOGN (TIZ_DEBUG, p_hdl, "Back to idle - status [%d] "
                        "all buffers returned : [TRUE]", status);

              rc = tiz_fsm_complete_transition
                (tiz_get_fsm (p_hdl), p_obj, OMX_StateIdle);
            }
          return rc;
        }
    }

  if (EStatePause != now && TIZ_PORT_IS_ENABLED (p_port))
    {
      const void *p_prc = tiz_get_prc (p_hdl);

      /* Delegate to the processor servant... */
      if (OMX_DirInput == dir)
        {
          rc = tiz_api_EmptyThisBuffer (p_prc, p_hdl, p_hdr);
        }
      else
        {
          rc = tiz_api_FillThisBuffer (p_prc, p_hdl, p_hdr);
        }
    }

  return rc;
}

static OMX_ERRORTYPE
dispatch_etb (void *ap_obj, OMX_PTR ap_msg)
{
  return dispatch_efb (ap_obj, ap_msg, ETIZKernelMsgEmptyThisBuffer);
}

static OMX_ERRORTYPE
dispatch_ftb (void *ap_obj, OMX_PTR ap_msg)
{
  return dispatch_efb (ap_obj, ap_msg, ETIZKernelMsgFillThisBuffer);
}

static OMX_ERRORTYPE
dispatch_pe (void *ap_obj, OMX_PTR ap_msg)
{
  tiz_kernel_msg_t *p_msg = ap_msg;
  tiz_kernel_msg_plg_event_t *p_msg_pe = NULL;

  assert (NULL != ap_obj);
  assert (NULL != p_msg);

  p_msg_pe = &(p_msg->pe);
  assert (NULL != p_msg_pe);

  /* TODO : Should this return something? */
  p_msg_pe->p_event->pf_hdlr ((OMX_PTR) ap_obj,
                              p_msg_pe->p_event->p_hdl, p_msg_pe->p_event);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
dispatch_sc (void *ap_obj, OMX_PTR ap_msg)
{
  struct tizkernel *p_obj = ap_obj;
  tiz_kernel_msg_t *p_msg = ap_msg;
  tiz_kernel_msg_sendcommand_t *p_msg_sc = NULL;

  assert (NULL != p_obj);
  assert (NULL != p_msg);

  p_msg_sc = &(p_msg->sc);
  assert (NULL != p_msg_sc);
  assert (p_msg_sc->cmd <= OMX_CommandMarkBuffer);

  return tiz_kernel_msg_dispatch_sc_to_fnt_tbl[p_msg_sc->cmd] (p_obj,
                                                              p_msg->p_hdl,
                                                              p_msg_sc);
}

/*
 * tizkernel
 */

static void *
kernel_ctor (void *ap_obj, va_list * app)
{
  struct tizkernel *p_obj = super_ctor (tizkernel, ap_obj, app);

  init_ports_and_lists (p_obj);

  return p_obj;
}

static void *
kernel_dtor (void *ap_obj)
{
  deinit_ports_and_lists (ap_obj);
  return super_dtor (tizkernel, ap_obj);
}

/*
 * tizapi
 */

static OMX_ERRORTYPE
kernel_GetComponentVersion (const void *ap_obj,
                            OMX_HANDLETYPE ap_hdl,
                            OMX_STRING ap_comp_name,
                            OMX_VERSIONTYPE * ap_comp_version,
                            OMX_VERSIONTYPE * ap_spec_version,
                            OMX_UUIDTYPE * ap_comp_uuid)
{
  const struct tizkernel *p_obj = ap_obj;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  /* Delegate to the config port */
  return tiz_api_GetComponentVersion (p_obj->p_cport_,
                                     ap_hdl,
                                     ap_comp_name,
                                     ap_comp_version,
                                     ap_spec_version, ap_comp_uuid);
}

static OMX_ERRORTYPE
kernel_GetParameter (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl,
                     OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tizkernel *p_obj = ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  /* Find the port that holds the data */
  if (OMX_ErrorNone == (rc = tiz_kernel_find_managing_port (p_obj, a_index,
                                                           ap_struct,
                                                           &p_port)))
    {
      /* Delegate to that port */
      return tiz_api_GetParameter (p_port, ap_hdl, a_index, ap_struct);
    }

  if (OMX_ErrorUnsupportedIndex != rc)
    {
      TIZ_LOGN (TIZ_ERROR, ap_hdl, "[%s] : Could not retrieve the managing "
                "port for index [%s]", tiz_err_to_str (rc),
                tiz_idx_to_str (a_index));
      return rc;
    }

  {
    OMX_PORT_PARAM_TYPE *p_struct = (OMX_PORT_PARAM_TYPE *) ap_struct;

    switch (a_index)
      {

      case OMX_IndexParamAudioInit:
        {
          *p_struct = p_obj->audio_init_;
          break;
        }

      case OMX_IndexParamVideoInit:
        {
          *p_struct = p_obj->video_init_;
          break;
        }

      case OMX_IndexParamImageInit:
        {
          *p_struct = p_obj->image_init_;
          break;
        }

      case OMX_IndexParamOtherInit:
        {
          *p_struct = p_obj->other_init_;
          break;
        }

      default:
        {
          TIZ_LOGN (TIZ_TRACE, ap_hdl, "OMX_ErrorUnsupportedIndex [0x%08x]...",
                    a_index);
          return OMX_ErrorUnsupportedIndex;
        }
      };

  }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
kernel_SetParameter (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl,
                     OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tizkernel *p_obj = ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  /* Find the port that holds the data */
  if (OMX_ErrorNone == (rc = tiz_kernel_find_managing_port (p_obj, a_index,
                                                           ap_struct,
                                                           &p_port)))
    {
      OMX_U32 mos_pid = 0;      /* master's or slave's pid */

      /* Delegate to the port */
      rc = tiz_api_SetParameter (p_port, ap_hdl, a_index, ap_struct);

      if (OMX_ErrorNone == rc && !TIZ_PORT_IS_CONFIG_PORT (p_port))
        {
          if (OMX_TRUE == tiz_port_is_master_or_slave (p_port, &mos_pid))
            {
              OMX_PTR *pp_mos_port = NULL, p_mos_port = NULL;
              tiz_vector_t *p_changed_idxs = NULL;

              /* Retrieve the master or slave's port... */
              pp_mos_port = tiz_vector_at (p_obj->p_ports_, mos_pid);
              assert (pp_mos_port && *pp_mos_port);
              p_mos_port = *pp_mos_port;

              tiz_vector_init (&(p_changed_idxs), sizeof (OMX_INDEXTYPE));
              rc = tiz_port_apply_slaving_behaviour (p_mos_port, p_port,
                                                    a_index, ap_struct,
                                                    p_changed_idxs);

              if (OMX_ErrorNone == rc)
                {
                  const OMX_S32 nidxs = tiz_vector_length (p_changed_idxs);
                  int i = 0;

                  for (; i < nidxs; ++i)
                    {
                      OMX_INDEXTYPE *p_idx = tiz_vector_at (p_changed_idxs, i);
                      assert (p_idx != NULL);
                      /* Trigger here a port settings changed event */
                      tiz_servant_issue_event (p_obj,
                                              OMX_EventPortSettingsChanged,
                                              mos_pid, *p_idx, NULL);
                    }
                }

              tiz_vector_clear (p_changed_idxs);
              tiz_vector_destroy (p_changed_idxs);
            }
        }

      return rc;
    }

  if (OMX_ErrorUnsupportedIndex != rc)
    {
      TIZ_LOGN (TIZ_ERROR, ap_hdl, "[%s] : Could not retrieve the managing "
                "port for index [%s]", tiz_err_to_str (rc),
                tiz_idx_to_str (a_index));
      return rc;
    }

  switch (a_index)
    {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
      {
        /* OMX_PORT_PARAM_TYPE structures are read only */
        return OMX_ErrorUnsupportedIndex;
      }
    default:
      {
        TIZ_LOG (TIZ_TRACE,
                 "OMX_ErrorUnsupportedIndex [0x%08x]...", a_index);
        return OMX_ErrorUnsupportedIndex;
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
kernel_SendCommand (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                    OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                    OMX_PTR ap_cmd_data)
{
  tiz_kernel_msg_t *p_msg = NULL;
  tiz_kernel_msg_sendcommand_t *p_msg_sc = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  TIZ_LOG (TIZ_TRACE, ap_hdl, "SendCommand [%s]", tiz_cmd_to_str (a_cmd));

  if (NULL == (p_msg = init_kernel_message (ap_obj, ap_hdl,
                                            ETIZKernelMsgSendCommand)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_sc = &(p_msg->sc);
  p_msg_sc->cmd = a_cmd;
  p_msg_sc->param1 = a_param1;
  p_msg_sc->p_cmd_data = ap_cmd_data;

  return tiz_servant_enqueue (ap_obj, p_msg, cmd_to_priority (a_cmd));
}

static OMX_ERRORTYPE
kernel_GetConfig (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tizkernel *p_obj = ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  /* Find the port that holds the data */
  if (OMX_ErrorNone == (rc = tiz_kernel_find_managing_port (p_obj, a_index,
                                                           ap_struct,
                                                           &p_port)))
    {
      /* Delegate to that port */
      return tiz_api_GetConfig (p_port, ap_hdl, a_index, ap_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
kernel_SetConfig (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  /* Find the port that holds the data */
  if (OMX_ErrorNone == (rc = tiz_kernel_find_managing_port (p_obj, a_index,
                                                           ap_struct,
                                                           &p_port)))
    {
      /* Delegate to that port */
      rc = tiz_api_SetConfig (p_port, ap_hdl, a_index, ap_struct);
    }


  if (OMX_ErrorNone != rc)
    {
      TIZ_LOGN (TIZ_ERROR, ap_hdl, "[%s] : SetConfig (%s) ...",
                tiz_err_to_str (rc), tiz_idx_to_str (a_index));
      return rc;
    }

  if (OMX_IndexConfigTunneledPortStatus == a_index)
    {
      tiz_kernel_tunneled_ports_status_t status
        = tiz_kernel_get_tunneled_ports_status (p_obj, OMX_FALSE);

      if (OMX_FALSE == p_obj->accept_use_buffer_notified_
          && ETIZKernelTunneledPortsAcceptUseBuffer == status)
        {
          TIZ_LOGN (TIZ_TRACE, ap_hdl, "kernel's tunneled port status [%d] ",
                    status);
          p_obj->accept_use_buffer_notified_ = OMX_TRUE;
          tiz_fsm_tunneled_ports_status_update (tiz_get_fsm (ap_hdl));
        }
      else if ((OMX_FALSE == p_obj->accept_buffer_exchange_notified_)
               &&
               (ETIZKernelTunneledPortsAcceptBufferExchange == status
                || ETIZKernelTunneledPortsAcceptBoth == status))
        {
          TIZ_LOGN (TIZ_TRACE, ap_hdl, "kernel's tunneled port status [%d] ",
                    status);
          p_obj->accept_buffer_exchange_notified_ = OMX_TRUE;
          tiz_fsm_tunneled_ports_status_update (tiz_get_fsm (ap_hdl));
        }
      else if ((OMX_FALSE == p_obj->may_transition_exe2idle_notified_)
               &&
               (ETIZKernelTunneledPortsMayInitiateExeToIdle
                == (status =
                    tiz_kernel_get_tunneled_ports_status (p_obj, OMX_TRUE))))
        {
          TIZ_LOGN (TIZ_TRACE, ap_hdl, "kernel's tunneled port status [%d] ",
                    status);
          p_obj->may_transition_exe2idle_notified_ = OMX_TRUE;
          tiz_fsm_tunneled_ports_status_update (tiz_get_fsm (ap_hdl));
        }
    }

  return rc;
}

static OMX_ERRORTYPE
kernel_GetExtensionIndex (const void *ap_obj,
                          OMX_HANDLETYPE ap_hdl,
                          OMX_STRING ap_param_name,
                          OMX_INDEXTYPE * ap_index_type)
{
  const struct tizkernel *p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_ERRORTYPE rc = OMX_ErrorUnsupportedIndex;
  OMX_PTR p_port = NULL;
  OMX_S32 i = 0;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  nports = tiz_vector_length (p_obj->p_ports_);

  TIZ_LOGN (TIZ_TRACE, ap_hdl, "GetExtensionIndex [%s] nports [%d]...",
            ap_param_name, nports);

  /* Check every port to see if the extension is supported... */
  for (i = 0; i < nports && OMX_ErrorUnsupportedIndex == rc; ++i)
    {
      p_port = get_port (p_obj, i);
      rc = tiz_api_GetExtensionIndex (p_port, ap_hdl,
                                     ap_param_name, ap_index_type);
    }

  if (OMX_ErrorUnsupportedIndex == rc)
    {
      /* Now check the config port */
      rc = tiz_api_GetExtensionIndex (p_obj->p_cport_, ap_hdl,
                                     ap_param_name, ap_index_type);
    }

  return rc;
}

static OMX_ERRORTYPE
kernel_ComponentTunnelRequest (const void *ap_obj,
                               OMX_HANDLETYPE ap_hdl,
                               OMX_U32 a_pid,
                               OMX_HANDLETYPE ap_thdl,
                               OMX_U32 a_tpid, OMX_TUNNELSETUPTYPE * ap_tsetup)
{
  const struct tizkernel *p_obj = ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  if (check_pid (p_obj, a_pid) != OMX_ErrorNone)
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Retrieve the port... */
  p_port = get_port (p_obj, a_pid);

  /* Check tunnel being torn down... */
  if (NULL == ap_thdl)
    {
      /* Delegate to the port */
      rc = tiz_api_ComponentTunnelRequest (p_port, ap_hdl,
                                          a_pid, ap_thdl, a_tpid, ap_tsetup);

      return rc;
    }

  /* Check port being re-tunnelled... */
  if (TIZ_PORT_IS_TUNNELED (p_port))
    {
      /* TODO */
    }

  /* Delegate to the port... */
  if (OMX_ErrorNone
      != (rc = tiz_api_ComponentTunnelRequest (p_port, ap_hdl, a_pid,
                                              ap_thdl, a_tpid, ap_tsetup)))
    {
      TIZ_LOGN (TIZ_ERROR, ap_hdl, "[%s] : While delegating "
                "ComponentTunnelRequest to port [%d]",
                tiz_err_to_str (rc), a_pid);
      return rc;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
kernel_UseBuffer (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_BUFFERHEADERTYPE ** app_hdr,
                  OMX_U32 a_pid,
                  OMX_PTR ap_apppriv, OMX_U32 a_size, OMX_U8 * ap_buf)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  if (check_pid (p_obj, a_pid) != OMX_ErrorNone)
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Retrieve the port... */
  p_port = get_port (p_obj, a_pid);

  /* Check that in case of tunnelling, this port is not a buffer supplier... */
  if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
    {
      TIZ_LOGN (TIZ_ERROR, ap_hdl, "[OMX_ErrorBadPortIndex] : Bad port index"
                "(port is tunneled)...");
      return OMX_ErrorBadPortIndex;
    }

  /* Now delegate to the port... */
  {
    const OMX_BOOL was_being_enabled = TIZ_PORT_IS_BEING_ENABLED (p_port);
    if (OMX_ErrorNone != (rc = tiz_api_UseBuffer (p_port,
                                                 ap_hdl,
                                                 app_hdr,
                                                 a_pid,
                                                 ap_apppriv, a_size, ap_buf)))
      {
        TIZ_LOGN (TIZ_ERROR, ap_hdl, "[%s] : While delegating UseBuffer "
                  "to port [%d]", tiz_err_to_str (rc), a_pid);
        return rc;
      }

    if (was_being_enabled && TIZ_PORT_IS_POPULATED (p_port))
      {
        complete_port_enable (p_obj, p_port, a_pid, OMX_ErrorNone);
      }
  }

  if (OMX_ErrorNone == rc && all_populated (p_obj))
    {
      TIZ_LOGN (TIZ_TRACE, ap_hdl, "AllPortsPopulated : [TRUE]");
      rc = tiz_fsm_complete_transition
        (tiz_get_fsm (ap_hdl), ap_obj, OMX_StateIdle);
    }

  return rc;
}

static OMX_ERRORTYPE
kernel_AllocateBuffer (const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl,
                       OMX_BUFFERHEADERTYPE ** app_hdr,
                       OMX_U32 a_pid, OMX_PTR ap_apppriv, OMX_U32 a_size)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_fsm_state_id_t now = EStateMax;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  
  now = tiz_fsm_get_substate (tiz_get_fsm (ap_hdl));

  TIZ_LOGN (TIZ_TRACE, ap_hdl, "AllocateBuffer...");

  if (check_pid (p_obj, a_pid) != OMX_ErrorNone)
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Grab the port here... */
  p_port = get_port (p_obj, a_pid);

  /* Check that in case of tunnelling, this port is not a buffer supplier... */
  if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
    {
      TIZ_LOGN (TIZ_ERROR, ap_hdl, "[OMX_ErrorBadPortIndex] : port [%d] "
                "is supplier...", a_pid);
      return OMX_ErrorBadPortIndex;
    }

  /* Now delegate to the port... */
  {
    const OMX_BOOL was_being_enabled = TIZ_PORT_IS_BEING_ENABLED (p_port);
    if (OMX_ErrorNone != (rc = tiz_api_AllocateBuffer (p_port,
                                                      ap_hdl,
                                                      app_hdr,
                                                      a_pid,
                                                      ap_apppriv, a_size)))
      {
        TIZ_LOGN (TIZ_ERROR, ap_hdl, "[%s] : While delegating AllocateBuffer "
                  "to port [%d]", tiz_err_to_str (rc), a_pid);
        return rc;
      }

    if (was_being_enabled && TIZ_PORT_IS_POPULATED (p_port))
      {
        complete_port_enable (p_obj, p_port, a_pid, OMX_ErrorNone);
      }
  }

  if (OMX_ErrorNone == rc && all_populated (p_obj))
    {
      TIZ_LOGN (TIZ_TRACE, ap_hdl, "AllPortsPopulated : [TRUE]");
      if (ESubStateLoadedToIdle == now)
        {
          rc = tiz_fsm_complete_transition
            (tiz_get_fsm (ap_hdl), ap_obj, OMX_StateIdle);
        }
    }

  if (OMX_ErrorNone == rc)
    {
      assert (NULL != *app_hdr);
    }

  return rc;
}

static OMX_ERRORTYPE
kernel_FreeBuffer (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_U32 a_pid, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const struct tizkernel *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_BOOL issue_unpop = OMX_FALSE;
  tiz_fsm_state_id_t now = EStateMax;
  OMX_S32 buf_count;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  now = tiz_fsm_get_substate (tiz_get_fsm (ap_hdl));

  if (check_pid (p_obj, a_pid) != OMX_ErrorNone)
    {
      return OMX_ErrorBadPortIndex;
    }

  p_port = get_port (p_obj, a_pid);

  TIZ_LOGN (TIZ_TRACE, ap_hdl, "FreeBuffer : PORT [%d] STATE [%s]", a_pid,
            tiz_fsm_state_to_str (now));

  /* Check that in case of tunnelling, this is not buffer supplier... */
  if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
    {
      TIZ_LOGN (TIZ_ERROR, ap_hdl, "[OMX_ErrorBadPortIndex] : port [%d] "
                "is supplier...", a_pid);
      return OMX_ErrorBadPortIndex;
    }

  if (ESubStateIdleToLoaded != now
      && TIZ_PORT_IS_ENABLED (p_port) && TIZ_PORT_IS_POPULATED (p_port))
    {
      /* The port should be disabled. */
      /* The buffer deallocation will raise an OMX_ErrorPortUnpopulated  */
      /* error in the current state. */
      issue_unpop = OMX_TRUE;
    }

  {
    const OMX_BOOL was_being_disabled = TIZ_PORT_IS_BEING_DISABLED (p_port);
    /* Delegate to the port... */
    if (OMX_ErrorNone != (rc = tiz_api_FreeBuffer (p_port, ap_hdl,
                                                  a_pid, ap_hdr)))
      {
        TIZ_LOGN (TIZ_DEBUG, ap_hdl, "[%s] when delegating FreeBuffer to "
                  "the port", tiz_err_to_str (rc));
        return rc;
      }

    if (issue_unpop)
      {
        tiz_servant_issue_err_event (p_obj, OMX_ErrorPortUnpopulated);
      }

    buf_count = tiz_port_buffer_count (p_port);

    if (!buf_count && was_being_disabled)
      {
        complete_port_disable ((struct tizkernel *) p_obj, p_port,
                               a_pid, OMX_ErrorNone);
      }
  }

  return complete_ongoing_transitions (p_obj, ap_hdl);
}

static OMX_ERRORTYPE
kernel_EmptyThisBuffer (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  tiz_kernel_msg_t *p_msg = NULL;
  tiz_kernel_msg_emptyfillbuffer_t *p_etb = NULL;

  TIZ_LOGN (TIZ_TRACE, ap_hdl, "HEADER [%p] BUFFER [%p] PID [%d]",
            ap_hdr, ap_hdr->pBuffer, ap_hdr->nInputPortIndex);

  if (NULL == (p_msg = init_kernel_message (ap_obj, ap_hdl,
                                            ETIZKernelMsgEmptyThisBuffer)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_etb = &(p_msg->ef);
  p_etb->p_hdr = ap_hdr;

  return tiz_servant_enqueue (ap_obj, p_msg, 2);
}

static OMX_ERRORTYPE
kernel_FillThisBuffer (const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  tiz_kernel_msg_t *p_msg = NULL;
  tiz_kernel_msg_emptyfillbuffer_t *p_msg_ftb = NULL;

  TIZ_LOGN (TIZ_TRACE, ap_hdl, "HEADER [%p] BUFFER [%p] PID [%d]",
            ap_hdr, ap_hdr->pBuffer, ap_hdr->nOutputPortIndex)

    if (NULL == (p_msg = init_kernel_message (ap_obj, ap_hdl,
                                              ETIZKernelMsgFillThisBuffer)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_ftb = &(p_msg->ef);
  p_msg_ftb->p_hdr = ap_hdr;

  return tiz_servant_enqueue (ap_obj, p_msg, 2);
}

static OMX_ERRORTYPE
kernel_SetCallbacks (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl,
                     OMX_CALLBACKTYPE * ap_callbacks, OMX_PTR ap_app_data)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
kernel_UseEGLImage (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                    OMX_U32 a_port_index,
                    OMX_PTR ap_app_private, void *eglImage)
{
  return OMX_ErrorNotImplemented;
}

/*
 * from tizservant api
 */

static OMX_ERRORTYPE
kernel_dispatch_msg (const void *ap_obj, OMX_PTR ap_msg)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  tiz_kernel_msg_t *p_msg = ap_msg;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_HANDLETYPE hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);
  (void) hdl;

  assert (NULL != p_obj);
  assert (NULL != p_msg);

  TIZ_LOGN (TIZ_TRACE, hdl, "Processing [%s]...",
                 tiz_kernel_msg_to_str (p_msg->class));

  assert (p_msg->class < ETIZKernelMsgMax);

  rc = tiz_kernel_msg_to_fnt_tbl[p_msg->class] ((OMX_PTR) p_obj, p_msg);

  return rc;
}

static OMX_ERRORTYPE
kernel_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_U32 pid = 0;
  OMX_S32 i = 0;
  OMX_HANDLETYPE p_hdl = NULL;
  (void) p_hdl;

  assert (NULL != ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);
  p_hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);

  TIZ_LOGN (TIZ_TRACE, p_hdl, "port index [%d]...", a_pid);

  /* Verify the port index.. */
  if ((OMX_ALL != a_pid) && (check_pid (p_obj, a_pid) != OMX_ErrorNone))
    {
      return OMX_ErrorBadPortIndex;
    }

  do
    {
      pid = ((OMX_ALL != a_pid) ? a_pid : i);
      p_port = get_port (p_obj, pid);

      /* This function will do nothing if it doesn't have to, e.g. because the
       * port isn't tunneled, or is disabled, etc. */
      tiz_port_update_tunneled_status (p_port, p_hdl,
                                      OMX_PORTSTATUS_ACCEPTUSEBUFFER);

      TIZ_LOGN (TIZ_TRACE, p_hdl, "pid [%d] enabled [%s] tunneled [%s] "
                "supplier [%s] populated [%s]..", pid,
                TIZ_PORT_IS_ENABLED (p_port) ? "YES" : "NO",
                TIZ_PORT_IS_TUNNELED (p_port) ? "YES" : "NO",
                TIZ_PORT_IS_SUPPLIER (p_port) ? "YES" : "NO",
                TIZ_PORT_IS_POPULATED (p_port) ? "YES" : "NO");

      if (TIZ_PORT_IS_ENABLED_TUNNELED_SUPPLIER_AND_NOT_POPULATED (p_port))
        {
          const OMX_BOOL being_enabled = TIZ_PORT_IS_BEING_ENABLED (p_port);
          if (OMX_ErrorNone != (rc = tiz_port_populate (p_port, p_hdl)))
            {
              TIZ_LOGN (TIZ_ERROR, p_hdl, "[%s] : While populating port [%d] ",
                        tiz_err_to_str (rc), pid);
              return rc;
            }

          if (being_enabled && TIZ_PORT_IS_POPULATED_AND_ENABLED (p_port))
            {
              complete_port_enable (p_obj, p_port, pid, OMX_ErrorNone);
            }

        }

      ++i;
    }
  while (OMX_ALL == a_pid && i < nports);

  return rc;
}

static OMX_ERRORTYPE
kernel_deallocate_resources (void *ap_obj)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_S32 i = 0;

  assert (NULL != ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  for (i = 0; i < nports; ++i)
    {
      p_port = get_port (p_obj, i);

      if (TIZ_PORT_IS_ENABLED_TUNNELED_AND_SUPPLIER (p_port))
        {
          if (OMX_ErrorNone != (rc = tiz_port_depopulate (p_port)))
            {
              break;
            }
        }
    }

  TIZ_LOGN (TIZ_TRACE, tiz_servant_super_get_hdl (tizkernel, p_obj),
            "[%s] : ALL depopulated [%s]...", tiz_err_to_str (rc),
            all_depopulated (p_obj) ? "TRUE" : "FALSE");

  return rc;
}

static OMX_ERRORTYPE
kernel_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_U32 pid = 0;
  OMX_S32 i = 0;
  OMX_HANDLETYPE p_hdl = NULL;
  (void) p_hdl;

  assert (NULL != ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  p_hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);
  assert (NULL != p_hdl);

  TIZ_LOGN (TIZ_TRACE, p_hdl, "pid [%d]", a_pid);

  if ((OMX_ALL != a_pid) && (check_pid (p_obj, a_pid) != OMX_ErrorNone))
    {
      return OMX_ErrorBadPortIndex;
    }

  do
    {
      pid = ((OMX_ALL != a_pid) ? a_pid : i);
      p_port = get_port (p_obj, pid);

      if (TIZ_PORT_IS_ENABLED_TUNNELED_AND_SUPPLIER (p_port))
        {
          const OMX_DIRTYPE dir = tiz_port_dir (p_port);
          tiz_vector_t *p_dst2darr, *p_srclst = tiz_port_get_hdrs_list (p_port);
          assert (OMX_DirInput == dir || OMX_DirOutput == dir);

          /* Input port -> Add header to egress list... */
          /* Output port -> Add header to ingress list... */
          p_dst2darr = (OMX_DirInput == dir ?
                        p_obj->p_egress_ : p_obj->p_ingress_);

          if (OMX_ErrorNone !=
              (rc = append_buflsts (p_dst2darr, p_srclst, pid)))
            {
              TIZ_LOGN (TIZ_ERROR, p_hdl, "[%s] : on port [%d] while appending "
                        "buffer lists", tiz_err_to_str (rc), pid);
              return rc;
            }

        }

      ++i;
    }
  while (OMX_ALL == a_pid && i < nports);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
kernel_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_PTR p_port = NULL;
  OMX_U32 pid = 0;
  OMX_S32 i = 0;
  OMX_HANDLETYPE p_hdl = NULL;
  (void) p_hdl;

  assert (NULL != ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  p_hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);
  assert (NULL != p_hdl);

  TIZ_LOGN (TIZ_TRACE, p_hdl, "T&P pid [%d]", a_pid);

  if ((OMX_ALL != a_pid) && (check_pid (p_obj, a_pid) != OMX_ErrorNone))
    {
      return OMX_ErrorBadPortIndex;
    }

  do
    {
      pid = ((OMX_ALL != a_pid) ? a_pid : i);
      p_port = get_port (p_obj, pid);

      /* This function will do nothing if it doesn't have to, e.g. because the
       * port isn't tunneled, or is disabled, etc. */
      tiz_port_update_tunneled_status (p_port, p_hdl,
                                      OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE);

      flush_egress (p_obj, pid, OMX_TRUE);
      propagate_ingress (p_obj, pid);


      i++;
    }
  while (OMX_ALL == a_pid && i < nports);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
kernel_stop_and_return (void *ap_obj)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_S32 i = 0, nbufs = 0;
  OMX_HANDLETYPE p_hdl = NULL;
  (void) p_hdl;

  assert (NULL != ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  p_hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);
  assert (NULL != p_hdl);

  TIZ_LOGN (TIZ_TRACE, p_hdl, "stop and return...[%p]", ap_obj);

  for (i = 0; i < nports && OMX_ErrorNone == rc; ++i)
    {
      p_port = get_port (p_obj, i);

      if (TIZ_PORT_IS_DISABLED (p_port) || !tiz_port_buffer_count (p_port))
        {
          continue;
        }

      if (TIZ_PORT_IS_ENABLED_TUNNELED_AND_SUPPLIER (p_port))
        {
          /* Move buffers from egress to ingress */
          nbufs = move_to_ingress (p_obj, i);
          TIZ_LOGN (TIZ_TRACE, p_hdl, "Moved [%d] tunnel buffers to ingress",
                    nbufs);
          if (nbufs < 0)
            {
              TIZ_LOGN (TIZ_ERROR, p_hdl, "[OMX_ErrorInsufficientResources] - "
                        "nbufs [%d]", nbufs);
              rc = OMX_ErrorInsufficientResources;
            }

          /* This function will do nothing if the port is disabled, for
           * example */
          tiz_port_update_tunneled_status (p_port, p_hdl,
                                          OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN);

          continue;
        }

      /* Move buffers from ingress to egress */
      nbufs = move_to_egress (p_obj, i);
      TIZ_LOGN (TIZ_TRACE, p_hdl, "Moved [%d] non-tunnel buffers to egress",
                nbufs);
      if (nbufs < 0)
        {
          TIZ_LOGN (TIZ_ERROR, p_hdl, "[OMX_ErrorInsufficientResources] - "
                    "nbufs [%d]", nbufs);
          rc = OMX_ErrorInsufficientResources;
        }

      rc = flush_egress (p_obj, i, OMX_FALSE);

      /* Flush buffer marks and complete commands as required */
      flush_marks (p_obj, p_port);
    }

  return rc;
}

static OMX_ERRORTYPE
kernel_receive_pluggable_event (const void *ap_obj,
                                OMX_HANDLETYPE ap_hdl, tizevent_t * ap_event)
{
  tiz_kernel_msg_t *p_msg = NULL;
  tiz_kernel_msg_plg_event_t *p_plgevt = NULL;

  TIZ_LOGN (TIZ_TRACE, ap_hdl, "PluggableEvent : event [%p]", ap_event);

  if (NULL == (p_msg = init_kernel_message (ap_obj, ap_hdl,
                                            ETIZKernelMsgPluggableEvent)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_plgevt = &(p_msg->pe);
  p_plgevt->p_event = ap_event;

  return tiz_servant_enqueue (ap_obj, p_msg, 2);
}

/*
 * API from tizkernel
 */

static OMX_ERRORTYPE
kernel_register_port (void *ap_obj, OMX_PTR ap_port, OMX_BOOL ais_config)
{
  struct tizkernel *p_obj = ap_obj;
  OMX_HANDLETYPE hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);
  (void) hdl;

  assert (ap_obj);
  assert (ap_port);

  if (OMX_TRUE == ais_config)
    {
      assert (NULL == p_obj->p_cport_);
      p_obj->p_cport_ = ap_port;
      tiz_port_set_index (ap_port, TIZ_PORT_CONFIG_PORT_INDEX);
      TIZ_LOGN (TIZ_TRACE, hdl, "Registering config port [%p] "
               "with index [%d]", ap_port, TIZ_PORT_CONFIG_PORT_INDEX);
      return OMX_ErrorNone;
    }

  {
    /* Create the corresponding ingress and egress lists */
    tiz_vector_t *p_in_list;
    tiz_vector_t *p_out_list;
    OMX_U32 pid;
    tiz_vector_init (&(p_in_list), sizeof (OMX_BUFFERHEADERTYPE *));
    tiz_vector_init (&(p_out_list), sizeof (OMX_BUFFERHEADERTYPE *));
    tiz_vector_push_back (p_obj->p_ingress_, &p_in_list);
    tiz_vector_push_back (p_obj->p_egress_, &p_out_list);

    pid = tiz_vector_length (p_obj->p_ports_);
    tiz_port_set_index (ap_port, pid);

    TIZ_LOGN (TIZ_TRACE, hdl, "Registering port [%p] with index [%d]",
              ap_port, pid);

    switch (tiz_port_domain (ap_port))
      {
      case OMX_PortDomainAudio:
        {
          if (0 == p_obj->audio_init_.nPorts)
            {
              p_obj->audio_init_.nStartPortNumber = pid;
            }
          p_obj->audio_init_.nPorts++;
          break;
        }

      case OMX_PortDomainVideo:
        {
          if (0 == p_obj->video_init_.nPorts)
            {
              p_obj->video_init_.nStartPortNumber = pid;
            }
          p_obj->video_init_.nPorts++;
          break;
        }

      case OMX_PortDomainImage:
        {
          if (0 == p_obj->image_init_.nPorts)
            {
              p_obj->image_init_.nStartPortNumber = pid;
            }
          p_obj->image_init_.nPorts++;
          break;
        }

      case OMX_PortDomainOther:
        {
          if (0 == p_obj->other_init_.nPorts)
            {
              p_obj->other_init_.nStartPortNumber = pid;
            }
          p_obj->other_init_.nPorts++;
          break;
        }

      default:
        {
          assert (0);
        }

      };

    /* TODO Assert that this port is not repeated in the array */
    return tiz_vector_push_back (p_obj->p_ports_, &ap_port);
  }
}

OMX_ERRORTYPE
tiz_kernel_register_port (const void *ap_obj, OMX_PTR ap_port,
                         OMX_BOOL ais_config)
{
  const struct tiz_kernel_class *class = classOf (ap_obj);
  assert (class->register_port);
  return class->register_port (ap_obj, ap_port, ais_config);
}

OMX_ERRORTYPE
tiz_kernel_super_register_port (const void *a_class, const void *ap_obj,
                               OMX_PTR ap_port, OMX_BOOL ais_config)
{
  const struct tiz_kernel_class *superclass = super (a_class);
  assert (ap_obj && superclass->register_port);
  return superclass->register_port (ap_obj, ap_port, ais_config);
}

static void *
kernel_get_port (const void *ap_obj, OMX_U32 a_pid)
{
  const struct tizkernel *p_obj = ap_obj;
  OMX_S32 num_ports = 0;

  assert (NULL != ap_obj);
  num_ports = tiz_vector_length (p_obj->p_ports_);

  TIZ_LOG (TIZ_TRACE, "num_ports [%d] a_pid [%d]...", num_ports, a_pid);

  if (num_ports <= a_pid)
    {
      return NULL;
    }

  return get_port (p_obj, a_pid);
}

void *
tiz_kernel_get_port (const void *ap_obj, OMX_U32 a_pid)
{
  const struct tiz_kernel_class *class = classOf (ap_obj);
  assert (class->get_port);
  return class->get_port (ap_obj, a_pid);
}

OMX_ERRORTYPE
kernel_find_managing_port (const struct tizkernel * ap_krn,
                           OMX_INDEXTYPE a_index,
                           OMX_PTR ap_struct, OMX_PTR * app_port)
{
  OMX_ERRORTYPE rc = OMX_ErrorUnsupportedIndex;
  OMX_PTR *pp_port = NULL;
  OMX_S32 i, num_ports;
  OMX_U32 *p_port_index;
  OMX_HANDLETYPE p_hdl = NULL;
  (void) p_hdl;

  assert (NULL != ap_krn);
  assert (NULL != app_port);
  assert (NULL != ap_struct);

  p_hdl = tiz_servant_super_get_hdl (tizkernel, ap_krn);

  if (OMX_ErrorNone == tiz_port_find_index (ap_krn->p_cport_, a_index))
    {
      *app_port = ap_krn->p_cport_;
      TIZ_LOGN (TIZ_TRACE, p_hdl, "[%s] : Config port being searched. "
                "Returning...", tiz_idx_to_str (a_index));
      return OMX_ErrorNone;
    }
  else
    {
      num_ports = tiz_vector_length (ap_krn->p_ports_);
      for (i = 0; i < num_ports; ++i)
        {
          pp_port = tiz_vector_at (ap_krn->p_ports_, i);
          if (OMX_ErrorNone == tiz_port_find_index (*pp_port, a_index))
            {
              rc = OMX_ErrorNone;
              break;
            }
        }

      if (OMX_ErrorNone == rc)
        {
          /* Now we retrieve the port index from the struct */
          p_port_index = (OMX_U32 *) ap_struct +
            sizeof (OMX_U32) / sizeof (OMX_U32) +
            sizeof (OMX_VERSIONTYPE) / sizeof (OMX_U32);

          if (OMX_ErrorNone != (rc = check_pid (ap_krn, *p_port_index)))
            {
              return rc;
            }

          TIZ_LOGN (TIZ_TRACE, p_hdl, "[%s] : Found in port index [%d]...",
                    tiz_idx_to_str (a_index), *p_port_index);

          pp_port = tiz_vector_at (ap_krn->p_ports_, *p_port_index);
          *app_port = *pp_port;
          return rc;
        }
    }

  TIZ_LOGN (TIZ_TRACE, p_hdl, "[%s] : Could not find the managing port...",
            tiz_idx_to_str (a_index));

  return rc;
}

OMX_ERRORTYPE
tiz_kernel_find_managing_port (const void *ap_obj, OMX_INDEXTYPE a_index,
                              OMX_PTR ap_struct, OMX_PTR * app_port)
{
  const struct tiz_kernel_class *class = classOf (ap_obj);
  assert (class->find_managing_port);
  return class->find_managing_port (ap_obj, a_index, ap_struct, app_port);
}

static tiz_kernel_population_status_t
kernel_get_population_status (const void *ap_obj, OMX_U32 a_pid,
                              OMX_BOOL * ap_may_be_fully_unpopulated)
{
  const struct tizkernel *p_obj = ap_obj;
  tiz_kernel_population_status_t status = ETIZKernelFullyPopulated;

  assert (NULL != ap_obj);

  if (OMX_ALL == a_pid)
    {
      if (all_populated (p_obj) == OMX_TRUE)
        {
          status = ETIZKernelFullyPopulated;
        }
      else if (all_depopulated (p_obj) == OMX_TRUE)
        {
          status = ETIZKernelFullyUnpopulated;
        }
      else
        {
          OMX_S32 i, nports = tiz_vector_length (p_obj->p_ports_);
          OMX_PTR p_port = NULL;

          assert (NULL != ap_may_be_fully_unpopulated);

          status = ETIZKernelUnpopulated;
          *ap_may_be_fully_unpopulated = OMX_TRUE;

          /* Loop through all regular (non-config) ports */
          for (i = 0; i < nports; ++i)
            {
              p_port = get_port (p_obj, i);

              if (tiz_port_buffer_count (p_port) > 0
                  && !TIZ_PORT_IS_SUPPLIER (p_port)
                  && TIZ_PORT_IS_TUNNELED (p_port))
                {
                  /* There is a non-supplier, tunneled port that is being
                   * populated. This means we cannot be fully unpopulated
                   * without help from the tunneled component */
                  *ap_may_be_fully_unpopulated = OMX_FALSE;
                  break;
                }
            }

        }
    }
  else
    {
      OMX_S32 nports = tiz_vector_length (p_obj->p_ports_);
      OMX_PTR p_port = NULL;

      assert (a_pid < nports);

      p_port = get_port (p_obj, a_pid);

      if (TIZ_PORT_IS_POPULATED (p_port))
        {
          status = ETIZKernelFullyPopulated;
        }
      else if (tiz_port_buffer_count (p_port) == 0)
        {
          status = ETIZKernelFullyUnpopulated;
        }
      else
        {
          assert (NULL != ap_may_be_fully_unpopulated);

          status = ETIZKernelUnpopulated;
          *ap_may_be_fully_unpopulated = OMX_TRUE;

          if (!TIZ_PORT_IS_SUPPLIER (p_port))
            {
              /* A non-supplier port that is being populated */
              *ap_may_be_fully_unpopulated = OMX_FALSE;
            }
        }
    }

  return status;
}

tiz_kernel_population_status_t
tiz_kernel_get_population_status (const void *ap_obj, OMX_U32 a_pid,
                                 OMX_BOOL * ap_may_be_fully_unpopulated)
{
  const struct tiz_kernel_class *class = classOf (ap_obj);
  assert (class->get_population_status);
  return class->get_population_status (ap_obj, a_pid,
                                       ap_may_be_fully_unpopulated);
}

tiz_kernel_population_status_t
tiz_kernel_super_get_population_status (const void *a_class,
                                       const void *ap_obj, OMX_U32 a_pid,
                                       OMX_BOOL * ap_may_be_fully_unpopulated)
{
  const struct tiz_kernel_class *superclass = super (a_class);
  assert (ap_obj && superclass->get_population_status);
  return superclass->get_population_status (ap_obj, a_pid,
                                            ap_may_be_fully_unpopulated);
}

static OMX_ERRORTYPE
kernel_select (const void *ap_obj, OMX_U32 a_nports, tiz_pd_set_t * ap_set)
{
  const struct tizkernel *p_obj = ap_obj;
  OMX_S32 i = 0;
  OMX_S32 nports = 0;
  tiz_vector_t *p_list = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_set);

  nports = tiz_vector_length (p_obj->p_ports_);

  if (a_nports < nports)
    {
      nports = a_nports;
    }

  /* Loop through the first nports in the ingress list */
  for (i = 0; i < nports; ++i)
    {
      p_list = get_ingress_lst (p_obj, i);
      if (tiz_vector_length (p_list) > 0)
        {
          TIZ_PD_SET (i, ap_set);
        }
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_kernel_select (const void *ap_obj, OMX_U32 a_nports, tiz_pd_set_t * ap_set)
{
  const struct tiz_kernel_class *class = classOf (ap_obj);
  assert (class->select);
  return class->select (ap_obj, a_nports, ap_set);
}

OMX_ERRORTYPE
tiz_kernel_super_select (const void *a_class, const void *ap_obj,
                        OMX_U32 a_nports, tiz_pd_set_t * ap_set)
{
  const struct tiz_kernel_class *superclass = super (a_class);
  assert (ap_obj && superclass->select);
  return superclass->select (ap_obj, a_nports, ap_set);
}

static OMX_ERRORTYPE
kernel_claim_buffer (const void *ap_obj, OMX_U32 a_pid,
                     OMX_U32 a_pos, OMX_BUFFERHEADERTYPE ** app_hdr)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_vector_t *p_list = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_PTR p_port = NULL;
  OMX_HANDLETYPE hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);
  OMX_DIRTYPE pdir = OMX_DirMax;

  assert (NULL != ap_obj);
  assert (NULL != app_hdr);
  assert (check_pid (p_obj, a_pid) == OMX_ErrorNone);

  /* Buffers can't be claimed in OMX_StatePause state */
  assert (EStatePause != tiz_fsm_get_substate (tiz_get_fsm (hdl)));

  /* Find the port.. */
  p_port = get_port (p_obj, a_pid);

  TIZ_LOGN (TIZ_TRACE, hdl, "port's [%d] a_pos [%d] buf count [%d]...",
            a_pid, a_pos, tiz_port_buffer_count (p_port));

  /* Buffers can't be claimed on an disabled port */
  assert (!TIZ_PORT_IS_DISABLED (p_port));

  assert (a_pos < tiz_port_buffer_count (p_port));

  /* Grab the port's ingress list  */
  p_list = get_ingress_lst (p_obj, a_pid);
  assert (tiz_vector_length (p_list) <= tiz_port_buffer_count (p_port));

  /* Retrieve the header... */
  p_hdr = get_header (p_list, a_pos);
  *app_hdr = p_hdr;

  TIZ_LOGN (TIZ_TRACE, hdl, "port's [%d] HEADER [%p] BUFFER [%p] ingress "
            "list length [%d]...", a_pid, p_hdr, p_hdr->pBuffer,
            tiz_vector_length (p_list));

  pdir = tiz_port_dir (p_port);
  /* If it's an output port and allocator, ask the port to allocate the actual
   * buffer, in case pre-announcements have been disabled on this port. This
   * function call has no effect if pre-announcements are enabled on the
   * port. */
  if (OMX_DirOutput == pdir && TIZ_PORT_IS_ALLOCATOR (p_port))
    {
      tiz_port_populate_header (p_port, hdl, p_hdr);
    }

  /* ... and delete it from the list */
  tiz_vector_erase (p_list, a_pos, 1);

  /* Now increment by one the claimed buffers count on this port */
  TIZ_PORT_INC_CLAIMED_COUNT (p_port);

  /* ...and if its an input buffer, mark the header, if any marks
   * available... */
  if (OMX_DirInput == pdir)
    {
      /* NOTE: tiz_port_mark_buffer returns OMX_ErrorNone if the port marked the
       * buffer with one of its own marks */
      if (OMX_ErrorNone == (rc = tiz_port_mark_buffer (p_port, p_hdr)))
        {
          /* Successfully complete here the OMX_CommandMarkBuffer command */
          complete_mark_buffer (p_obj, p_port, a_pid, OMX_ErrorNone);
        }
      else
        {
          /* These two return codes are not actual errors. */
          if (OMX_ErrorNoMore == rc || OMX_ErrorNotReady == rc)
            {
              rc = OMX_ErrorNone;
            }
        }
    }

  return rc;
}

OMX_ERRORTYPE
tiz_kernel_claim_buffer (const void *ap_obj, OMX_U32 a_pid,
                        OMX_U32 a_pos, OMX_BUFFERHEADERTYPE ** app_hdr)
{
  const struct tiz_kernel_class *class = classOf (ap_obj);
  assert (class->claim_buffer);
  return class->claim_buffer (ap_obj, a_pid, a_pos, app_hdr);
}

OMX_ERRORTYPE
tiz_kernel_super_claim_buffer (const void *a_class, const void *ap_obj,
                              OMX_U32 a_pid, OMX_U32 a_pos,
                              OMX_BUFFERHEADERTYPE ** app_hdr)
{
  const struct tiz_kernel_class *superclass = super (a_class);
  assert (ap_obj && superclass->claim_buffer);
  return superclass->claim_buffer (ap_obj, a_pid, a_pos, app_hdr);
}

static OMX_ERRORTYPE
kernel_relinquish_buffer (const void *ap_obj, OMX_U32 a_pid,
                          OMX_BUFFERHEADERTYPE * ap_hdr)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;
  tiz_vector_t *p_list = NULL;
  OMX_PTR p_port = NULL;
  OMX_HANDLETYPE p_hdl = NULL;
  (void) p_hdl;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdr);
  assert (check_pid (p_obj, a_pid) == OMX_ErrorNone);

  p_hdl = tiz_servant_super_get_hdl (tizkernel, p_obj);

  /* Find the port.. */
  p_port = get_port (p_obj, a_pid);

  /* TODO : Verify that the header effectively belongs to the given port */
  /* assert (tiz_port_find_buffer(p_port, ap_hdr, is_owned)); */

  /* Grab the port's egress list */
  p_list = get_egress_lst (p_obj, a_pid);

  TIZ_LOGN (TIZ_TRACE, p_hdl, "HEADER [%p] pid [%d] egress length [%d]...",
            ap_hdr, a_pid, tiz_vector_length (p_list));

  assert (tiz_vector_length (p_list) < tiz_port_buffer_count (p_port));

  return enqueue_callback_msg (p_obj, ap_hdr, a_pid, tiz_port_dir (p_port));
}

OMX_ERRORTYPE
tiz_kernel_relinquish_buffer (const void *ap_obj, OMX_U32 a_pid,
                             OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const struct tiz_kernel_class *class = classOf (ap_obj);
  assert (class->relinquish_buffer);
  return class->relinquish_buffer (ap_obj, a_pid, ap_hdr);
}

OMX_ERRORTYPE
tiz_kernel_super_relinquish_buffer (const void *a_class, const void *ap_obj,
                                   OMX_U32 a_pid,
                                   OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const struct tiz_kernel_class *superclass = super (a_class);
  assert (ap_obj && superclass->relinquish_buffer);
  return superclass->relinquish_buffer (ap_obj, a_pid, ap_hdr);
}

static void
kernel_deregister_all_ports (void *ap_obj)
{
  struct tizkernel *p_obj = (struct tizkernel *) ap_obj;

  /* Reset kernel data structures */
  deinit_ports_and_lists (p_obj);
  init_ports_and_lists (ap_obj);
}

void
tiz_kernel_deregister_all_ports (void *ap_obj)
{
  const struct tiz_kernel_class *class = classOf (ap_obj);
  assert (class->deregister_all_ports);
  class->deregister_all_ports (ap_obj);
}

void
tiz_kernel_super_deregister_all_ports (const void *a_class, void *ap_obj)
{
  const struct tiz_kernel_class *superclass = super (a_class);
  assert (ap_obj && superclass->deregister_all_ports);
  superclass->deregister_all_ports (ap_obj);
}

static OMX_U32
kernel_get_tunneled_ports_status (const void *ap_obj,
                                  OMX_BOOL a_exe_to_idle_interest)
{
  const struct tizkernel *p_obj = ap_obj;
  tiz_kernel_tunneled_ports_status_t status_report =
    ETIZKernelTunneledPortsMax;
  OMX_U32 status = 0;

  assert (NULL != ap_obj);

  {
    OMX_S32 i, nports = tiz_vector_length (p_obj->p_ports_);
    OMX_PTR *pp_port = NULL;
    OMX_BOOL tunneled_found = OMX_FALSE;

    /* Loop through all regular (non-config) ports */
    status |= OMX_PORTSTATUS_ACCEPTUSEBUFFER;
    status |= OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE;
    status |= OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN;

    for (i = 0; i < nports; ++i)
      {
        pp_port = tiz_vector_at (p_obj->p_ports_, i);
        assert (NULL != pp_port);

        if (TIZ_PORT_IS_TUNNELED (*pp_port) && TIZ_PORT_IS_SUPPLIER (*pp_port))
          {

            if (!TIZ_PORT_MAY_CALL_USE_BUFFER (*pp_port))
              {
                status &= ~(OMX_PORTSTATUS_ACCEPTUSEBUFFER);
                TIZ_LOGN (TIZ_DEBUG, tiz_servant_super_get_hdl (tizkernel, p_obj),
                          "TIZ_PORT_MAY_CALL_USE_BUFFER = FALSE status [0x%08X]",
                          status);
              }
            if (!TIZ_PORT_MAY_EXCHANGE_BUFFERS (*pp_port))
              {
                status &= ~(OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE);
                TIZ_LOGN (TIZ_DEBUG, tiz_servant_super_get_hdl (tizkernel, p_obj),
                          "TIZ_PORT_MAY_EXCHANGE_BUFFERS = FALSE status [0x%08X]",
                          status);
              }
          }

        if (TIZ_PORT_IS_TUNNELED (*pp_port))
          {
            tunneled_found = OMX_TRUE;

            if (!TIZ_PORT_MAY_INITIATE_EXE_TO_IDLE (*pp_port))
              {
                status &= ~(OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN);
                TIZ_LOGN (TIZ_DEBUG, tiz_servant_super_get_hdl (tizkernel, p_obj),
                          "TIZ_PORT_MAY_INITIATE_EXE_TO_IDLE = FALSE status [0x%08X]",
                          status);
              }
            else
              {
                TIZ_LOGN (TIZ_DEBUG, tiz_servant_super_get_hdl (tizkernel, p_obj),
                          "TIZ_PORT_MAY_INITIATE_EXE_TO_IDLE = TRUE status [0x%08X]",
                          status);
              }
          }
        else
          {
            TIZ_LOGN (TIZ_DEBUG, tiz_servant_super_get_hdl (tizkernel, p_obj),
                      "NOT TUNNELED pid [%d]", tiz_port_index (*pp_port));
          }
      }

    if (OMX_FALSE == tunneled_found)
      {
        status_report = ETIZKernelNoTunneledPorts;
      }
    else
      {
        if (a_exe_to_idle_interest)
          {
            if (status & OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN)
              {
                /* Ignore other flags if a_exe_to_idle_interest is
                 * true */
                status_report = ETIZKernelTunneledPortsMayInitiateExeToIdle;
              }
            else
              {
                status_report = ETIZKernelTunneledPortsAcceptNone;
              }
          }
        else
          {
            /* OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN is not set, so now we
             * look at the other flags */
            if ((status & OMX_PORTSTATUS_ACCEPTUSEBUFFER)
                && (status & OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE))
              {
                status_report = ETIZKernelTunneledPortsAcceptBoth;
              }
            else if (status & OMX_PORTSTATUS_ACCEPTUSEBUFFER)
              {
                status_report = ETIZKernelTunneledPortsAcceptUseBuffer;
              }
            else if (status & OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE)
              {
                status_report = ETIZKernelTunneledPortsAcceptBufferExchange;
              }
            else
              {
                status_report = ETIZKernelTunneledPortsAcceptNone;
              }
          }
      }
  }

  TIZ_LOGN (TIZ_DEBUG, tiz_servant_super_get_hdl (tizkernel, p_obj),
            "status_report [%d]", status_report);

  assert (ETIZKernelTunneledPortsMax != status_report);

  return status_report;
}

tiz_kernel_tunneled_ports_status_t
tiz_kernel_get_tunneled_ports_status (const void *ap_obj,
                                     OMX_BOOL a_exe_to_idle_interest)
{
  const struct tiz_kernel_class *class = classOf (ap_obj);
  assert (class->get_tunneled_ports_status);
  return class->get_tunneled_ports_status (ap_obj, a_exe_to_idle_interest);
}

tiz_kernel_tunneled_ports_status_t
tiz_kernel_super_get_tunneled_ports_status (const void *a_class,
                                           const void *ap_obj,
                                           OMX_BOOL a_exe_to_idle_interest)
{
  const struct tiz_kernel_class *superclass = super (a_class);
  assert (ap_obj && superclass->get_tunneled_ports_status);
  return superclass->get_tunneled_ports_status (ap_obj,
                                                a_exe_to_idle_interest);
}

static void
kernel_reset_tunneled_ports_status (void *ap_obj, OMX_U32 a_port_status_flag)
{
  struct tizkernel *p_obj = ap_obj;

  assert (NULL != ap_obj);

  switch (a_port_status_flag)
    {
    case OMX_PORTSTATUS_ACCEPTUSEBUFFER:
      {
        p_obj->accept_use_buffer_notified_ = OMX_FALSE;
      }
      break;
    case OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE:
      {
        p_obj->accept_buffer_exchange_notified_ = OMX_FALSE;
      }
      break;
    case OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN:
      {
        p_obj->may_transition_exe2idle_notified_ = OMX_FALSE;
      }
      break;

    default:
      {
        assert (0);
      }
    };

  {
    OMX_S32 i = tiz_vector_length (p_obj->p_ports_) - 1;
    OMX_PTR p_port = NULL;

    do
      {
        p_port = get_port (p_obj, i);
        tiz_port_reset_tunneled_port_status_flag (p_port,
                                                 tiz_servant_get_hdl (p_obj),
                                                 a_port_status_flag);
      }
    while (i-- != 0);
  }

}

void
tiz_kernel_reset_tunneled_ports_status (void *ap_obj,
                                       OMX_U32 a_port_status_flag)
{
  const struct tiz_kernel_class *class = classOf (ap_obj);
  assert (class->reset_tunneled_ports_status);
  class->reset_tunneled_ports_status (ap_obj, a_port_status_flag);
}

void
tiz_kernel_super_reset_tunneled_ports_status (const void *a_class,
                                           void *ap_obj,
                                             OMX_U32 a_port_status_flag)
{
  const struct tiz_kernel_class *superclass = super (a_class);
  assert (ap_obj && superclass->reset_tunneled_ports_status);
  superclass->reset_tunneled_ports_status (ap_obj, a_port_status_flag);
}

/*
 * tiz_kernel_class
 */

static void *
kernel_class_ctor (void *ap_obj, va_list * app)
{
  struct tiz_kernel_class *p_obj = super_ctor (tiz_kernel_class, ap_obj, app);
  typedef void (*voidf) ();
  voidf selector;
  va_list ap;
  va_copy (ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) tiz_kernel_register_port)
        {
          *(voidf *) & p_obj->register_port = method;
        }
      else if (selector == (voidf) tiz_kernel_get_port)
        {
          *(voidf *) & p_obj->get_port = method;
        }
      else if (selector == (voidf) tiz_kernel_find_managing_port)
        {
          *(voidf *) & p_obj->find_managing_port = method;
        }
      else if (selector == (voidf) tiz_kernel_get_population_status)
        {
          *(voidf *) & p_obj->get_population_status = method;
        }
      else if (selector == (voidf) tiz_kernel_select)
        {
          *(voidf *) & p_obj->select = method;
        }
      else if (selector == (voidf) tiz_kernel_claim_buffer)
        {
          *(voidf *) & p_obj->claim_buffer = method;
        }
      else if (selector == (voidf) tiz_kernel_relinquish_buffer)
        {
          *(voidf *) & p_obj->relinquish_buffer = method;
        }
      else if (selector == (voidf) tiz_kernel_deregister_all_ports)
        {
          *(voidf *) & p_obj->deregister_all_ports = method;
        }
      else if (selector == (voidf) tiz_kernel_get_tunneled_ports_status)
        {
          *(voidf *) & p_obj->get_tunneled_ports_status = method;
        }
      else if (selector == (voidf) tiz_kernel_reset_tunneled_ports_status)
        {
          *(voidf *) & p_obj->reset_tunneled_ports_status = method;
        }

    }

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

const void *tizkernel, *tiz_kernel_class;

void
init_tizkernel (void)
{

  if (!tiz_kernel_class)
    {
      init_tizservant ();
      tiz_kernel_class = factory_new (tiz_servant_class,
                                     "tiz_kernel_class",
                                     tiz_servant_class,
                                     sizeof (struct tiz_kernel_class),
                                     ctor, kernel_class_ctor, 0);

    }

  if (!tizkernel)
    {
      init_tizservant ();
      tizkernel =
        factory_new
        (tiz_kernel_class,
         "tizkernel",
         tizservant,
         sizeof (struct tizkernel),
         ctor, kernel_ctor,
         dtor, kernel_dtor,
         tiz_api_GetComponentVersion, kernel_GetComponentVersion,
         tiz_api_GetParameter, kernel_GetParameter,
         tiz_api_SetParameter, kernel_SetParameter,
         tiz_api_GetConfig, kernel_GetConfig,
         tiz_api_SetConfig, kernel_SetConfig,
         tiz_api_GetExtensionIndex, kernel_GetExtensionIndex,
         tiz_api_SendCommand, kernel_SendCommand,
         tiz_api_ComponentTunnelRequest, kernel_ComponentTunnelRequest,
         tiz_api_UseBuffer, kernel_UseBuffer,
         tiz_api_AllocateBuffer, kernel_AllocateBuffer,
         tiz_api_FreeBuffer, kernel_FreeBuffer,
         tiz_api_EmptyThisBuffer, kernel_EmptyThisBuffer,
         tiz_api_FillThisBuffer, kernel_FillThisBuffer,
         tiz_api_SetCallbacks, kernel_SetCallbacks,
         tiz_api_UseEGLImage, kernel_UseEGLImage,
         tiz_servant_dispatch_msg, kernel_dispatch_msg,
         tiz_servant_allocate_resources, kernel_allocate_resources,
         tiz_servant_deallocate_resources, kernel_deallocate_resources,
         tiz_servant_prepare_to_transfer, kernel_prepare_to_transfer,
         tiz_servant_transfer_and_process, kernel_transfer_and_process,
         tiz_servant_stop_and_return, kernel_stop_and_return,
         tiz_servant_receive_pluggable_event,
         kernel_receive_pluggable_event, tiz_kernel_register_port,
         kernel_register_port, tiz_kernel_get_port, kernel_get_port,
         tiz_kernel_find_managing_port, kernel_find_managing_port,
         tiz_kernel_get_population_status, kernel_get_population_status,
         tiz_kernel_get_tunneled_ports_status, kernel_get_tunneled_ports_status,
         tiz_kernel_select, kernel_select,
         tiz_kernel_claim_buffer, kernel_claim_buffer,
         tiz_kernel_relinquish_buffer, kernel_relinquish_buffer,
         tiz_kernel_deregister_all_ports, kernel_deregister_all_ports,
         tiz_kernel_reset_tunneled_ports_status,
         kernel_reset_tunneled_ports_status, 0);
    }
}
