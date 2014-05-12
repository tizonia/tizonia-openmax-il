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
 * @file   tizprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizscheduler.h"
#include "tizport.h"
#include "tizport-macros.h"
#include "tizkernel.h"
#include "tizutils.h"

#include "tizprc.h"
#include "tizprc_decls.h"
#include "tizprc_internal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.prc"
#endif

static const OMX_STRING tiz_prc_msg_to_str (tiz_prc_msg_class_t a_msg)
{
  const OMX_S32 count = sizeof(tiz_prc_msg_to_str_tbl)
                        / sizeof(tiz_prc_msg_str_t);
  OMX_S32 i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_prc_msg_to_str_tbl[i].msg == a_msg)
        {
          return tiz_prc_msg_to_str_tbl[i].str;
        }
    }

  return "Unknown proc message";
}

static OMX_ERRORTYPE dispatch_idle_to_loaded (tiz_prc_t *ap_prc,
                                              bool * ap_done)
{
  assert (NULL != ap_done);
  *ap_done = true;
  return tiz_srv_deallocate_resources (ap_prc);
}

static OMX_ERRORTYPE dispatch_loaded_to_idle (tiz_prc_t *ap_prc,
                                              bool * ap_done)
{
  assert (NULL != ap_done);
  *ap_done = true;
  return tiz_srv_allocate_resources (ap_prc, OMX_ALL);
}

static OMX_ERRORTYPE dispatch_exe_or_pause_to_idle (tiz_prc_t *ap_prc,
                                              bool * ap_done)
{
  assert (NULL != ap_done);
  *ap_done = true;
  return tiz_srv_stop_and_return (ap_prc);
}

static OMX_ERRORTYPE dispatch_idle_to_exe (tiz_prc_t *ap_prc,
                                           bool * ap_done)
{
  assert (NULL != ap_done);
  *ap_done = true;
  return tiz_srv_prepare_to_transfer (ap_prc, OMX_ALL);
}

static OMX_ERRORTYPE prc_DeferredResume (const void *ap_obj);
static OMX_ERRORTYPE dispatch_pause_to_exe (tiz_prc_t *ap_prc,
                                           bool * ap_done)
{
  assert (NULL != ap_done);
  *ap_done = true;
  return prc_DeferredResume (ap_prc);
}

static OMX_ERRORTYPE dispatch_exe_to_exe (tiz_prc_t *ap_prc,
                                          bool * ap_done)
{
  assert (NULL != ap_done);
  *ap_done = true;
  return tiz_srv_transfer_and_process (ap_prc, OMX_ALL);
}

static OMX_ERRORTYPE dispatch_exe_or_idle_to_pause (tiz_prc_t *ap_prc,
                                                    bool *     ap_done)
{
  assert (NULL != ap_done);
  *ap_done = true;
  return tiz_prc_pause (ap_prc);
}

static OMX_ERRORTYPE dispatch_true (tiz_prc_t *ap_prc,
                                    bool * ap_done)
{
  assert (NULL != ap_done);
  *ap_done = true;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE dispatch_false (tiz_prc_t *ap_prc,
                                    bool * ap_done)
{
  return OMX_ErrorIncorrectStateTransition;
}

static inline tiz_prc_msg_t *init_prc_message (const void *ap_obj,
                                               OMX_HANDLETYPE ap_hdl,
                                               tiz_prc_msg_class_t a_msg_class)
{
  tiz_prc_t *p_obj = (tiz_prc_t *)ap_obj;
  tiz_prc_msg_t *p_msg = NULL;

  assert (NULL != p_obj);
  assert (NULL != ap_hdl);
  assert (a_msg_class < ETIZPrcMsgMax);

  if (NULL == (p_msg = tiz_srv_init_msg (p_obj, sizeof(tiz_prc_msg_t))))
    {
      TIZ_ERROR (ap_hdl,
                 "[OMX_ErrorInsufficientResources] : "
                 "Could not allocate message [%s]",
                 tiz_prc_msg_to_str (a_msg_class));
    }
  else
    {
      p_msg->p_hdl = ap_hdl;
      p_msg->class = a_msg_class;
    }

  return p_msg;
}

static OMX_ERRORTYPE enqueue_buffersready_msg (const void *ap_obj,
                                               OMX_HANDLETYPE ap_hdl,
                                               OMX_BUFFERHEADERTYPE *ap_hdr,
                                               OMX_U32 a_pid)
{
  const tiz_prc_t *p_obj = ap_obj;
  tiz_prc_msg_t *p_msg = NULL;
  tiz_prc_msg_buffersready_t *p_msg_br = NULL;

  TIZ_TRACE (ap_hdl, "BuffersReady : HEADER [%p]", ap_hdr);

  if (NULL
      == (p_msg = init_prc_message (p_obj, ap_hdl, ETIZPrcMsgBuffersReady)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_br = &(p_msg->br);
  p_msg_br->p_buffer = ap_hdr;
  p_msg_br->pid = a_pid;

  /* Enqueueing with the lowest priority */
  return tiz_srv_enqueue (ap_obj, p_msg, 1);
}

static inline OMX_U32 cmd_to_priority (OMX_COMMANDTYPE a_cmd)
{
  OMX_U32 prio = 0;

  switch (a_cmd)
    {
      case OMX_CommandStateSet:
      case OMX_CommandFlush:
      case OMX_CommandPortDisable:
      case OMX_CommandPortEnable:
        {
          prio = 0;
        }
        break;

      case OMX_CommandMarkBuffer:
      /* NOTE: Mark buffer command is not hdld in this class */
      default:
        assert (0);
        break;
    };

  return prio;
}

static OMX_ERRORTYPE prc_DeferredResume (const void *ap_obj)
{
  tiz_prc_t *p_obj = (tiz_prc_t *)ap_obj;
  tiz_prc_msg_t *p_msg = NULL;
  tiz_prc_msg_deferredresume_t *p_msg_dr = NULL;

  assert (NULL != ap_obj);

  TIZ_TRACE (handleOf(p_obj), "DeferredResume");

  if (NULL
      == (p_msg = init_prc_message (p_obj, handleOf(p_obj), ETIZPrcMsgDeferredResume)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_dr = &(p_msg->dr);
  p_msg_dr->target_state = OMX_StateExecuting;
  return tiz_srv_enqueue (ap_obj, p_msg, 1);
}

static OMX_ERRORTYPE dispatch_sc (void *ap_obj, OMX_PTR ap_msg)
{
  tiz_prc_t *p_obj = ap_obj;
  tiz_prc_msg_t *p_msg = ap_msg;
  tiz_prc_msg_sendcommand_t *p_msg_sc = NULL;

  assert (NULL != p_msg);

  p_msg_sc = &(p_msg->sc);
  assert (NULL != p_msg_sc);
  /* NOTE: Mark buffer command is not hdld in this class */
  assert (p_msg_sc->cmd < OMX_CommandMarkBuffer);

  return tiz_prc_msg_dispatch_sc_to_fnt_tbl[p_msg_sc->cmd](p_obj, p_msg->p_hdl,
                                                           p_msg_sc);
}

static OMX_ERRORTYPE dispatch_br (void *ap_obj, OMX_PTR ap_msg)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_prc_t *p_obj = ap_obj;
  tiz_prc_msg_t *p_msg = ap_msg;
  tiz_prc_msg_buffersready_t *p_msg_br = NULL;
  const void *p_krn = NULL;
  const void *p_port = NULL;
  tiz_fsm_state_id_t now = EStateMax;

  assert (NULL != p_obj);
  assert (NULL != p_msg);
  assert (NULL != p_msg->p_hdl);

  p_msg_br = &(p_msg->br);
  assert (NULL != p_msg_br);
  assert (NULL != p_msg_br->p_buffer);

  p_krn = tiz_get_krn (p_msg->p_hdl);
  p_port = tiz_krn_get_port (p_krn, p_msg_br->pid);
  now = tiz_fsm_get_substate (tiz_get_fsm (p_msg->p_hdl));

  TIZ_TRACE (p_msg->p_hdl,
             "p_msg->p_hdl [%p] "
             "p_msg_br->pid = [%d] p_port [%p]",
             p_msg->p_hdl, p_msg_br->pid, p_port);

  assert (p_port);

  /* Do not notify this buffer in the following situations:
   *
   * - Component in OMX_StatePause or
   *
   * - Component in ExeToIdle or PauseToIdle
   *
   * - the port is disabled or being disabled
   *
   * */
  if (EStatePause != now && ESubStateExecutingToIdle != now
      && ESubStatePauseToIdle != now && !TIZ_PORT_IS_DISABLED (p_port)
      && !TIZ_PORT_IS_BEING_DISABLED (p_port))
    {
      TIZ_TRACE (p_msg->p_hdl, "p_msg_br->p_buffer [%p] ", p_msg_br->p_buffer);
      rc = tiz_prc_buffers_ready (p_obj);
    }

  return rc;
}

static OMX_ERRORTYPE dispatch_config (void *ap_obj, OMX_PTR ap_msg)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_prc_t *p_obj = ap_obj;
  tiz_prc_msg_t *p_msg = ap_msg;
  tiz_prc_msg_configchange_t *p_msg_cc = NULL;
  const void *p_krn = NULL;
  const void *p_port = NULL;
  tiz_fsm_state_id_t now = EStateMax;

  assert (NULL != p_obj);
  assert (NULL != p_msg);

  p_msg_cc = &(p_msg->cc);
  assert (NULL != p_msg_cc);

  p_krn = tiz_get_krn (p_msg->p_hdl);
  p_port = tiz_krn_get_port (p_krn, p_msg_cc->pid);
  now = tiz_fsm_get_substate (tiz_get_fsm (p_msg->p_hdl));

  TIZ_TRACE (p_msg->p_hdl, "p_msg_cc->pid = [%d] p_port [%p]", p_msg->p_hdl,
             p_msg_cc->pid, p_port);

  assert (NULL != p_port);

  /* Do not notify this config change in the following situations:
   *
   * - Component in OMX_StatePause or
   *
   * - Component in ExeToIdle or PauseToIdle
   *
   * - the port is disabled or being disabled
   *
   * */
  if (EStatePause != now && ESubStateExecutingToIdle != now
      && ESubStatePauseToIdle != now && !TIZ_PORT_IS_DISABLED (p_port)
      && !TIZ_PORT_IS_BEING_DISABLED (p_port))
    {
      TIZ_TRACE (p_msg->p_hdl, "index [%s] ", tiz_idx_to_str (p_msg_cc->index));
      rc = tiz_prc_config_change (p_obj, p_msg_cc->pid, p_msg_cc->index);
    }

  return rc;
}

static OMX_ERRORTYPE dispatch_dr (void *ap_obj, OMX_PTR ap_msg)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_prc_t *p_obj = ap_obj;
  tiz_prc_msg_t *p_msg = ap_msg;
  tiz_prc_msg_deferredresume_t *p_msg_dr = NULL;
  tiz_fsm_state_id_t now = EStateMax;

  assert (NULL != p_obj);
  assert (NULL != p_msg);

  p_msg_dr = &(p_msg->dr);
  assert (NULL != p_msg_dr);

  now = tiz_fsm_get_substate (tiz_get_fsm (p_msg->p_hdl));

  TIZ_TRACE (p_msg->p_hdl, "[%s] - [%s]", tiz_prc_msg_to_str (p_msg->class),
             tiz_fsm_state_to_str (now));

  /* Only notify resume in the following situation:
   *
   * - Component in OMX_StateExecuting
   *
   * */
  if (EStateExecuting == now)
    {
      /* Now we can ask the processor to resume */
      rc = tiz_prc_resume (p_obj);
    }
  else if (EStatePause == now)
    {
      /* Enqueue another deferred resume command  */
      rc = prc_DeferredResume (p_obj);
    }
  else
    {
      /* the FSM has transitioned to some other state, so we can ignore the
         resume request. Simply log the fact...
      */
      TIZ_DEBUG (p_msg->p_hdl,
                 "Ignoring deferred resume "
                 "command in [%s]",
                 tiz_fsm_state_to_str (now));
    }

  return rc;
}

static inline tiz_prc_msg_dispatch_state_set_f
lookup_state_set_transition (const void *ap_obj,
                             const OMX_STATETYPE a_current_state,
                             const OMX_STATETYPE a_next_state)
{
  assert (a_current_state >= OMX_StateLoaded);
  assert (a_current_state <= OMX_StateWaitForResources);
  assert (a_next_state >= OMX_StateLoaded);
  assert (a_next_state <= OMX_StateWaitForResources);

  TIZ_TRACE (handleOf (ap_obj), "Requested transition [%s] -> [%s]",
             tiz_state_to_str (a_current_state), tiz_state_to_str (a_next_state));

  return tiz_prc_state_set_dispatch_tbl[a_current_state][a_next_state];
}

static OMX_ERRORTYPE dispatch_state_set (const void *ap_obj,
                                         OMX_HANDLETYPE ap_hdl,
                                         tiz_prc_msg_sendcommand_t *ap_msg_sc)
{
  tiz_prc_t *p_prc = (tiz_prc_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_STATETYPE now = OMX_StateMax;
  OMX_STATETYPE next = OMX_StateMax;
  bool done = false;
  tiz_prc_msg_dispatch_state_set_f p_action_f = NULL;

  assert (NULL != ap_msg_sc);

  /* Obtain the current state */
  tiz_check_omx_err (tiz_api_GetState (tiz_get_fsm (ap_hdl), ap_hdl, &now));

  /* ...and this is the next state */
  next =  ap_msg_sc->param1;

  /* find the action */
  p_action_f = lookup_state_set_transition (p_prc, now, next);
  assert (NULL != p_action_f);
  rc = p_action_f (p_prc, &done);

  if (OMX_ErrorIncorrectStateTransition == rc)
    {
      TIZ_ERROR (ap_hdl, "unhandled transition : [%s] -> [%s]",
                 tiz_state_to_str (now), tiz_state_to_str (next));
      assert (false && ("Unhandled transition in processor servant"));
    }

  if (OMX_ErrorNone == rc && done)
    {
      rc = tiz_fsm_complete_transition (tiz_get_fsm (ap_hdl), p_prc,
                                        ap_msg_sc->param1);
    }

  /* TODO: Move this logic to its own function in the servant class. Will also
     be used by the kernel and fsm servants.  */
  if (OMX_ErrorFormatNotDetected == rc)
    {
      TIZ_ERROR (ap_hdl, "[%s] : Signalling processor error via event handler.",
                 tiz_err_to_str (rc));
      tiz_srv_issue_event (p_prc, OMX_EventError, rc, 0, NULL);
      rc = OMX_ErrorNone;
    }

  TIZ_TRACE (ap_hdl, "[%s]", tiz_err_to_str (rc));

  return rc;
}

static OMX_ERRORTYPE dispatch_port_flush (const void *ap_obj,
                                          OMX_HANDLETYPE p_hdl,
                                          tiz_prc_msg_sendcommand_t *ap_msg_sc)
{
  assert (NULL != ap_msg_sc);
  return tiz_prc_port_flush (ap_obj, ap_msg_sc->param1);
}

static OMX_ERRORTYPE dispatch_port_disable (
    const void *ap_obj, OMX_HANDLETYPE p_hdl,
    tiz_prc_msg_sendcommand_t *ap_msg_sc)
{
  assert (NULL != ap_msg_sc);
  return tiz_prc_port_disable (ap_obj, ap_msg_sc->param1);
}

static OMX_ERRORTYPE dispatch_port_enable (const void *ap_obj,
                                           OMX_HANDLETYPE p_hdl,
                                           tiz_prc_msg_sendcommand_t *ap_msg_sc)
{
  assert (NULL != ap_msg_sc);
  return tiz_prc_port_enable (ap_obj, ap_msg_sc->param1);
}

static OMX_BOOL remove_buffer_from_servant_queue (OMX_PTR ap_elem,
                                                  OMX_S32 a_data1,
                                                  OMX_PTR ap_data2)
{
  OMX_BOOL rc = OMX_FALSE;
  tiz_prc_msg_t *p_msg = ap_elem;
  const OMX_BUFFERHEADERTYPE *p_hdr = ap_data2;

  assert (NULL != p_msg);
  assert (NULL != p_hdr);

  if (p_msg->class == a_data1)
    {
      tiz_prc_msg_buffersready_t *p_msg_br = &(p_msg->br);
      assert (NULL != p_msg_br);

      if (p_hdr == p_msg_br->p_buffer)
        {
          /* Found, return TRUE so this item will be removed from the servant
           * queue */
          TIZ_TRACE (p_msg->p_hdl,
                     "tiz_prc_msg_buffersready_t : Found HEADER [%p]", p_hdr);
          rc = OMX_TRUE;
        }
    }
  else
    {
      /* Not interested */
      TIZ_TRACE (p_msg->p_hdl, "Not interested : class  [%s]",
                 tiz_prc_msg_to_str (p_msg->class));
    }

  return rc;
}

/*
 * tiz_prc
 */

static void *prc_ctor (void *ap_obj, va_list *app)
{
  return super_ctor (typeOf (ap_obj, "tizprc"), ap_obj, app);
}

static void *prc_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tizprc"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE prc_EmptyThisBuffer (const void *ap_obj,
                                          OMX_HANDLETYPE ap_hdl,
                                          OMX_BUFFERHEADERTYPE *ap_buf)
{
  TIZ_TRACE (ap_hdl, "pid [%d]", ap_buf->nInputPortIndex);
  return enqueue_buffersready_msg (ap_obj, ap_hdl, ap_buf,
                                   ap_buf->nInputPortIndex);
}

static OMX_ERRORTYPE prc_FillThisBuffer (const void *ap_obj,
                                         OMX_HANDLETYPE ap_hdl,
                                         OMX_BUFFERHEADERTYPE *ap_buf)
{
  TIZ_TRACE (ap_hdl, "pid [%d]", ap_buf->nOutputPortIndex);
  return enqueue_buffersready_msg (ap_obj, ap_hdl, ap_buf,
                                   ap_buf->nOutputPortIndex);
}

static OMX_ERRORTYPE prc_SendCommand (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                                      OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                                      OMX_PTR ap_cmd_data)
{
  const tiz_prc_t *p_obj = ap_obj;
  tiz_prc_msg_t *p_msg = NULL;
  tiz_prc_msg_sendcommand_t *p_msg_sc = NULL;

  TIZ_TRACE (ap_hdl, "SendCommand [%s]", tiz_cmd_to_str (a_cmd));

  if (NULL == (p_msg = init_prc_message (p_obj, ap_hdl, ETIZPrcMsgSendCommand)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_sc = &(p_msg->sc);
  p_msg_sc->cmd = a_cmd;
  p_msg_sc->param1 = a_param1;
  p_msg_sc->p_cmd_data = ap_cmd_data;

  return tiz_srv_enqueue (ap_obj, p_msg, cmd_to_priority (a_cmd));
}

static OMX_ERRORTYPE prc_SetConfig (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                                    OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_prc_t *p_obj = (tiz_prc_t *)ap_obj;
  tiz_prc_msg_t *p_msg = NULL;
  tiz_prc_msg_configchange_t *p_msg_cc = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_struct);

  TIZ_TRACE (ap_hdl, "SetConfig [%s]", tiz_idx_to_str (a_index));

  if (NULL == (p_msg = init_prc_message (p_obj, ap_hdl, ETIZPrcMsgConfig)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_cc = &(p_msg->cc);
  /* TODO: This is not the best way to do this */
  p_msg_cc->pid = *((OMX_U32 *)ap_struct + sizeof(OMX_U32) / sizeof(OMX_U32)
                    + sizeof(OMX_VERSIONTYPE) / sizeof(OMX_U32));
  p_msg_cc->index = a_index;

  return tiz_srv_enqueue (ap_obj, p_msg, 1);
}

/*
 * from tiz_srv api
 */

static void prc_remove_from_queue (const void *ap_obj, tiz_pq_func_f apf_func,
                                   OMX_S32 a_data1, OMX_PTR ap_data2)
{
  tiz_srv_t *p_obj = (tiz_srv_t *)ap_obj;
  /* Actual implementation is in the parent class */
  /* Replace dummy parameters apf_func and a_data1 */
  tiz_srv_super_remove_from_queue (typeOf (ap_obj, "tizprc"), p_obj,
                                   &remove_buffer_from_servant_queue,
                                   ETIZPrcMsgBuffersReady, ap_data2);
}

static OMX_ERRORTYPE prc_dispatch_msg (const void *ap_obj, OMX_PTR ap_msg)
{
  const tiz_prc_t *p_obj = ap_obj;
  tiz_prc_msg_t *p_msg = ap_msg;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_obj);
  assert (NULL != p_msg);

  TIZ_TRACE (handleOf (ap_obj), "Processing [%s]...",
             tiz_prc_msg_to_str (p_msg->class));

  assert (p_msg->class < ETIZPrcMsgMax);

  rc = tiz_prc_msg_to_fnt_tbl[p_msg->class]((OMX_PTR)ap_obj, p_msg);

  TIZ_TRACE (handleOf (ap_obj), "rc [%s]...", tiz_err_to_str (rc));

  return rc;
}

static OMX_ERRORTYPE prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  assert (ap_obj);
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE prc_deallocate_resources (void *ap_obj)
{
  assert (ap_obj);
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  assert (ap_obj);
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  assert (ap_obj);
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE prc_stop_and_return (void *ap_obj)
{
  assert (ap_obj);
  return OMX_ErrorNotImplemented;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE prc_buffers_ready (const void *ap_obj)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_prc_buffers_ready (const void *ap_obj)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->buffers_ready);
  return class->buffers_ready (ap_obj);
}

OMX_ERRORTYPE
tiz_prc_super_buffers_ready (const void *a_class, const void *ap_obj)
{
  const tiz_prc_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->buffers_ready);
  return superclass->buffers_ready (ap_obj);
}

static OMX_ERRORTYPE prc_pause (const void *ap_obj)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_pause (const void *ap_obj)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->pause);
  return class->pause (ap_obj);
}

static OMX_ERRORTYPE prc_resume (const void *ap_obj)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_resume (const void *ap_obj)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->resume);
  return class->resume (ap_obj);
}

static OMX_ERRORTYPE prc_port_flush (const void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_port_flush (const void *ap_obj, OMX_U32 a_pid)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->port_flush);
  return class->port_flush (ap_obj, a_pid);
}

static OMX_ERRORTYPE prc_port_disable (const void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_port_disable (const void *ap_obj, OMX_U32 a_pid)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->port_disable);
  return class->port_disable (ap_obj, a_pid);
}

static OMX_ERRORTYPE prc_port_enable (const void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_port_enable (const void *ap_obj, OMX_U32 a_pid)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->port_enable);
  return class->port_enable (ap_obj, a_pid);
}

static OMX_ERRORTYPE prc_config_change (const void *ap_obj, OMX_U32 a_pid,
                                        OMX_INDEXTYPE a_config_idx)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_config_change (const void *ap_obj, OMX_U32 a_pid,
                       OMX_INDEXTYPE a_config_idx)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->config_change);
  return class->config_change (ap_obj, a_pid, a_config_idx);
}

static OMX_ERRORTYPE prc_io_ready (const void *ap_obj, tiz_event_io_t *ap_ev_io,
                                   int a_fd, int a_events)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_io_ready (void *ap_obj, tiz_event_io_t *ap_ev_io, int a_fd,
                  int a_events)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->io_ready);
  return class->io_ready (ap_obj, ap_ev_io, a_fd, a_events);
}

static OMX_ERRORTYPE prc_timer_ready (void *ap_obj,
                                      tiz_event_timer_t *ap_ev_timer,
                                      void *ap_arg)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_timer_ready (void *ap_obj, tiz_event_timer_t *ap_ev_timer, void *ap_arg)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->timer_ready);
  return class->timer_ready (ap_obj, ap_ev_timer, ap_arg);
}

static OMX_ERRORTYPE prc_stat_ready (void *ap_obj, tiz_event_stat_t *ap_ev_stat,
                                     int a_events)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_stat_ready (void *ap_obj, tiz_event_stat_t *ap_ev_stat, int a_events)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (NULL != class->stat_ready);
  return class->stat_ready (ap_obj, ap_ev_stat, a_events);
}

/*
 * tizprc_class
 */

static void *prc_class_ctor (void *ap_obj, va_list *app)
{
  tiz_prc_class_t *p_obj
      = super_ctor (typeOf (ap_obj, "tizprc_class"), ap_obj, app);
  typedef void (*voidf)();
  voidf selector = NULL;
  va_list ap;
  va_copy (ap, *app);

  /* NOTE: Start ignoring splint warnings in this section of code */
  /*@ignore@*/
  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf)tiz_prc_buffers_ready)
        {
          *(voidf *)&p_obj->buffers_ready = method;
        }
      else if (selector == (voidf)tiz_prc_pause)
        {
          *(voidf *)&p_obj->pause = method;
        }
      else if (selector == (voidf)tiz_prc_resume)
        {
          *(voidf *)&p_obj->resume = method;
        }
      else if (selector == (voidf)tiz_prc_port_flush)
        {
          *(voidf *)&p_obj->port_flush = method;
        }
      else if (selector == (voidf)tiz_prc_port_disable)
        {
          *(voidf *)&p_obj->port_disable = method;
        }
      else if (selector == (voidf)tiz_prc_port_enable)
        {
          *(voidf *)&p_obj->port_enable = method;
        }
      else if (selector == (voidf)tiz_prc_config_change)
        {
          *(voidf *)&p_obj->config_change = method;
        }
      else if (selector == (voidf)tiz_prc_io_ready)
        {
          *(voidf *)&p_obj->io_ready = method;
        }
      else if (selector == (voidf)tiz_prc_timer_ready)
        {
          *(voidf *)&p_obj->timer_ready = method;
        }
      else if (selector == (voidf)tiz_prc_stat_ready)
        {
          *(voidf *)&p_obj->stat_ready = method;
        }
    }
  /*@end@*/
  /* NOTE: Stop ignoring splint warnings in this section  */

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

void *tiz_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizsrv = tiz_get_type (ap_hdl, "tizsrv");
  void *tizprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizsrv), "tizprc_class", classOf (tizsrv), sizeof(tiz_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizprc_class;
}

void *tiz_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizsrv = tiz_get_type (ap_hdl, "tizsrv");
  void *tizprc_class = tiz_get_type (ap_hdl, "tizprc_class");
  TIZ_LOG_CLASS (tizprc_class);
  void *tizprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (tizprc_class, "tizprc", tizsrv, sizeof(tiz_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_api_EmptyThisBuffer, prc_EmptyThisBuffer,
       /* TIZ_CLASS_COMMENT: */
       tiz_api_FillThisBuffer, prc_FillThisBuffer,
       /* TIZ_CLASS_COMMENT: */
       tiz_api_SendCommand, prc_SendCommand,
       /* TIZ_CLASS_COMMENT: */
       tiz_api_SetConfig, prc_SetConfig,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_remove_from_queue, prc_remove_from_queue,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_dispatch_msg, prc_dispatch_msg,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, prc_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, prc_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, prc_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, prc_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, prc_stop_and_return,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, prc_buffers_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_pause, prc_pause,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_resume, prc_resume,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_flush, prc_port_flush,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_disable, prc_port_disable,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_enable, prc_port_enable,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_config_change, prc_config_change,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_io_ready, prc_io_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_timer_ready, prc_timer_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_stat_ready, prc_stat_ready,
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return tizprc;
}
