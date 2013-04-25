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
 * @file   tizfsm.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - OMX IL FSM implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizfsm.h"
#include "tizfsm_decls.h"
#include "tizstate.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizport.h"
#include "tizport-macros.h"
#include "tizutils.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm"
#endif

/* Forward declarations */
static OMX_ERRORTYPE dispatch_sc (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_tc (void *ap_obj, OMX_PTR ap_msg);

typedef enum tiz_fsm_msg_class tiz_fsm_msg_class_t;
enum tiz_fsm_msg_class
{
  ETIZFsmMsgSendCommand = 0,
  ETIZFsmMsgTransComplete,
  ETIZFsmMsgMax
};

typedef struct tiz_fsm_msg_sendcommand tiz_fsm_msg_sendcommand_t;
struct tiz_fsm_msg_sendcommand
{
  OMX_COMMANDTYPE cmd;
  OMX_U32 param1;
  OMX_PTR p_cmd_data;
};

typedef struct tiz_fsm_msg_transcomplete tiz_fsm_msg_transcomplete_t;
struct tiz_fsm_msg_transcomplete
{
  OMX_PTR p_servant;
  OMX_STATETYPE state;
};

typedef struct tiz_fsm_msg tiz_fsm_msg_t;
struct tiz_fsm_msg
{
  OMX_HANDLETYPE p_hdl;
  tiz_fsm_msg_class_t class;
  union
  {
    tiz_fsm_msg_sendcommand_t sc;
    tiz_fsm_msg_transcomplete_t tc;
  };
};

typedef struct tiz_fsm_msg_str tiz_fsm_msg_str_t;
struct tiz_fsm_msg_str
{
  tiz_fsm_msg_class_t msg;
  OMX_STRING str;
};

static const tiz_fsm_msg_str_t tiz_fsm_msg_to_str_tbl[] = {
  {ETIZFsmMsgSendCommand, "ETIZFsmMsgSendCommand"},
  {ETIZFsmMsgTransComplete, "ETIZFsmMsgTransComplete"},
  {ETIZFsmMsgMax, "ETIZFsmMsgMax"},
};

static const OMX_STRING
tiz_fsm_msg_to_str (tiz_fsm_msg_class_t a_msg)
{
  const OMX_S32 count =
    sizeof (tiz_fsm_msg_to_str_tbl) / sizeof (tiz_fsm_msg_str_t);
  OMX_S32 i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_fsm_msg_to_str_tbl[i].msg == a_msg)
        {
          return tiz_fsm_msg_to_str_tbl[i].str;
        }
    }

  return "Unknown fsm message";
}


typedef OMX_ERRORTYPE (*tiz_fsm_msg_dispatch_f) (void *ap_obj, OMX_PTR ap_msg);
static const tiz_fsm_msg_dispatch_f tiz_fsm_msg_to_fnt_tbl[] = {
  dispatch_sc,
  dispatch_tc
};

static OMX_ERRORTYPE
dispatch_sc (void *ap_obj, OMX_PTR ap_msg)
{
  tiz_fsm_t *p_obj = ap_obj;
  tiz_fsm_msg_t *p_msg = ap_msg;
  tiz_fsm_msg_sendcommand_t *p_msg_sc = NULL;

  assert (NULL != p_msg);

  p_msg_sc = &(p_msg->sc);
  assert (NULL != p_msg_sc);
  assert (p_msg_sc->cmd <= OMX_CommandMarkBuffer);

  return tiz_api_SendCommand (p_obj->p_current_state_,
                             p_msg->p_hdl,
                             p_msg_sc->cmd,
                             p_msg_sc->param1, p_msg_sc->p_cmd_data);
}

static OMX_ERRORTYPE
dispatch_tc (void *ap_obj, OMX_PTR ap_msg)
{
  tiz_fsm_t *p_obj = ap_obj;
  tiz_fsm_msg_t *p_msg = ap_msg;
  tiz_fsm_msg_transcomplete_t *p_msg_tc = NULL;

  assert (NULL != p_obj);
  assert (NULL != p_msg);

  p_msg_tc = &(p_msg->tc);
  assert (p_msg_tc->state <= OMX_StateWaitForResources);

  return tiz_state_trans_complete (p_obj->p_current_state_,
                                  p_msg_tc->p_servant, p_msg_tc->state);
}

static inline tiz_fsm_msg_t *
init_fsm_message (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                  tiz_fsm_msg_class_t a_msg_class)
{
  tiz_fsm_t *p_obj = (tiz_fsm_t *) ap_obj;
  tiz_fsm_msg_t *p_msg = NULL;

  assert (NULL != p_obj);
  assert (NULL != ap_hdl);
  assert (a_msg_class < ETIZFsmMsgMax);

  if (NULL == (p_msg = tiz_servant_init_msg (p_obj, sizeof (tiz_fsm_msg_t))))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : "
                     "(Could not allocate message [%s]...)",
                     tiz_fsm_msg_to_str (a_msg_class));
    }
  else
    {
      p_msg->p_hdl = ap_hdl;
      p_msg->class = a_msg_class;
    }

  return p_msg;
}

static inline OMX_U32
msg_to_priority (tiz_fsm_msg_class_t a_msg_class)
{
  OMX_U32 prio = 0;

  switch (a_msg_class)
    {
    case ETIZFsmMsgSendCommand:
      {
        prio = 1;
      }
      break;
    case ETIZFsmMsgTransComplete:
      {
        prio = 0;
      }
      break;

    default:
      TIZ_LOG (TIZ_ERROR, "Unknown msg class [%d]", a_msg_class);
      assert (0);
      break;
    };

  return prio;
}

static OMX_ERRORTYPE
validate_stateset (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                   OMX_STATETYPE a_state)
{
  const tiz_fsm_t *p_obj = ap_obj;
  const void *p_krn = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  p_krn = tiz_get_krn (ap_hdl);
  assert (NULL != p_krn);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "Requested transition to [%s] - cur_state_id_ [%s] "
                 "in_progress_cmd_ [%s]",
                 tiz_state_to_str (a_state),
                 tiz_fsm_state_to_str (p_obj->cur_state_id_),
                 tiz_cmd_to_str (p_obj->in_progress_cmd_));

  if (OMX_CommandMax == p_obj->in_progress_cmd_)
    {
      return OMX_ErrorNone;
    }

  if (OMX_CommandMax != p_obj->in_progress_cmd_
      && OMX_CommandStateSet != p_obj->in_progress_cmd_)
    {
      /* There is a command of a different type going on. Reject this
       * operation.  */
      return OMX_ErrorIncorrectStateOperation;
    }

  /* OK, at this point, we now there is a state transition in progress... By
   * default, we do not allow the state transition command, but there are
   * some exceptions. */

  /* If we are is a transitional state, check if we can allow its
   * cancellation. */

  if (p_obj->cur_state_id_ > EStateWaitForResources
      && p_obj->cur_state_id_ < EStateMax)
    {
      /* We are is a transitional state, which may or may not be
       * cancelled. Check that. */

      if (ESubStateLoadedToIdle == p_obj->cur_state_id_
          && a_state == OMX_StateLoaded)
        {
          /* Check the population status of the kernel. We will reject the
           * command if the kernel cannot be fully unpopulated by the IL
           * client. In other words, there is at least a buffer header
           * allocated that needs to be freed by a tunneled component. If
           * that is the case, we won't allow the cancellation of the ongoing
           * transition. */
          OMX_BOOL may_be_fully_unpopulated = OMX_FALSE;
          const tiz_kernel_population_status_t kps
            = tiz_kernel_get_population_status (p_krn, OMX_ALL,
                                               &may_be_fully_unpopulated);
          if (ETIZKernelFullyUnpopulated == kps
              || (ETIZKernelUnpopulated == kps
                  && OMX_TRUE == may_be_fully_unpopulated))
            {
              /* This is OK */
              return OMX_ErrorNone;
            }
        }
    }

  return OMX_ErrorIncorrectStateOperation;
}

static OMX_ERRORTYPE
validate_portdisable (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_U32 a_pid)
{
  const tiz_fsm_t *p_obj = ap_obj;
  const void *p_krn = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  p_krn = tiz_get_krn (ap_hdl);
  assert (NULL != p_krn);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "[OMX_CommandPortDisable] pid [%d] cur_state_id_ [%s]",
                 a_pid, tiz_fsm_state_to_str (p_obj->cur_state_id_));

  /* If no other command is currently being processed, the go on with this
   * one */
  if (OMX_CommandMax == p_obj->in_progress_cmd_)
    {
      return OMX_ErrorNone;
    }

  /* If OMX_ALL, then reject the command. We'll accept a "cancellation"
   * command only if it applies to a single port. */
  if (a_pid == OMX_ALL)
    {
      return OMX_ErrorIncorrectStateOperation;
    }

  if (OMX_CommandStateSet == p_obj->in_progress_cmd_)
    {
      /* OK, at this point, we now there is a state transition in progress... By
       * default, we do not allow the port disable command, but there are
       * exceptions. */

      if (p_obj->cur_state_id_ > EStateWaitForResources
          && p_obj->cur_state_id_ < EStateMax)
        {
          /* There is an on-going state transition */
          if (ESubStateLoadedToIdle == p_obj->cur_state_id_)
            {
              OMX_BOOL may_be_fully_unpopulated = OMX_FALSE;
              const tiz_kernel_population_status_t kps
                = tiz_kernel_get_population_status (p_krn, a_pid,
                                                   &may_be_fully_unpopulated);
              if (ETIZKernelFullyUnpopulated == kps
                  || (ETIZKernelUnpopulated == kps
                      && OMX_TRUE == may_be_fully_unpopulated))
                {
                  /* This is OK */
                  return OMX_ErrorNone;
                }
            }
        }
    }
  else if (OMX_CommandPortEnable == p_obj->in_progress_cmd_)
    {
      OMX_BOOL may_be_fully_unpopulated = OMX_FALSE;
      const tiz_kernel_population_status_t kps
        = tiz_kernel_get_population_status (p_krn, a_pid,
                                           &may_be_fully_unpopulated);
      if (ETIZKernelFullyUnpopulated == kps
          || (ETIZKernelUnpopulated == kps
              && OMX_TRUE == may_be_fully_unpopulated))
        {
          /* This is OK */
          return OMX_ErrorNone;
        }
    }

  return OMX_ErrorIncorrectStateOperation;
}

static OMX_ERRORTYPE
validate_sendcommand (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                      OMX_PTR ap_cmd_data)
{
  tiz_fsm_t *p_obj = (tiz_fsm_t *) ap_obj;
  const void *p_krn = tiz_get_krn (ap_hdl);
  const void *p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  if (OMX_CommandFlush == a_cmd
      || OMX_CommandPortDisable == a_cmd
      || OMX_CommandPortEnable == a_cmd || OMX_CommandMarkBuffer == a_cmd)
    {
      /* Validate that the port index is correct */
      /* in those commands where a port index is needed */

      if (a_param1 != OMX_ALL)
        {
          p_port = tiz_kernel_get_port (p_krn, a_param1);
          if (NULL == p_port)
            {
              return OMX_ErrorBadPortIndex;
            }
        }
    }

  switch (a_cmd)
    {
    case OMX_CommandStateSet:
      {
        /* Verify that this state transition may be processed at this specific
         * point in time */
        rc = validate_stateset (p_obj, ap_hdl, a_param1);
      }
      break;

    case OMX_CommandPortDisable:
      {
        /* Verify that this port transition may be processed at this specific
         * point in time */
        rc = validate_portdisable (p_obj, ap_hdl, a_param1);
      }
      break;

    case OMX_CommandFlush:
      {
        if (OMX_CommandMax != p_obj->in_progress_cmd_)
          {
            rc = OMX_ErrorIncorrectStateOperation;
          }
      }
      break;

    case OMX_CommandPortEnable:
      {
        if (OMX_CommandMax != p_obj->in_progress_cmd_)
          {
            rc = OMX_ErrorIncorrectStateOperation;
          }
      }
      break;

    case OMX_CommandMarkBuffer:
      {
        const OMX_MARKTYPE *p_mark = (OMX_MARKTYPE *) ap_cmd_data;
        if (!p_mark || !p_mark->hMarkTargetComponent)
          {
            return OMX_ErrorBadParameter;
          }

        /* Not sure whether OMX_ALL may be used with OMX_CommandMarkBuffer. For
         * for now explicitly disallow it */
        assert (OMX_ALL != a_param1);

        /* OMX_CommandMarkBuffer shall be allowed in Exe, Paused or port
         * Disabled. Reject otherwise */
        if (!(TIZ_PORT_IS_DISABLED (p_port)
              || EStateExecuting == p_obj->cur_state_id_
              || EStatePause == p_obj->cur_state_id_))
          {
            return OMX_ErrorIncorrectStateOperation;
          }

      }
      break;

    default:
      {
      }
    };

  if (OMX_ErrorNone == rc)
    {
      /* At this point, we are accepting a command for processing. If we have
       * an unfinished command, then the new command is interpreted as a
       * cancellation. */
      if (p_obj->in_progress_cmd_ != OMX_CommandMax)
        {
          TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                         "Unfinished command [%s] - cur_state_id_ [%s] "
                         "New command [%s]",
                         tiz_cmd_to_str (p_obj->in_progress_cmd_),
                         tiz_fsm_state_to_str (p_obj->cur_state_id_),
                         tiz_cmd_to_str (a_cmd));
          assert (OMX_CommandMax == p_obj->cancellation_cmd_);
          p_obj->cancellation_cmd_ = a_cmd;
        }
      else
        {
          /* Record the command that we are going to process */
          /* TODO: For now do not record OMX_CommandMarkBuffer cmds */
          if (OMX_CommandMarkBuffer != a_cmd)
            {
              p_obj->in_progress_cmd_ = a_cmd;
              p_obj->in_progress_param1_ = a_param1;

            }
        }
    }

  return rc;
}

/*
 * tiz_fsm
 */

static void *
fsm_ctor (void *ap_obj, va_list * app)
{
  OMX_U32 i;
  tiz_fsm_t *p_obj = super_ctor (tizfsm, ap_obj, app);

  for (i = 0; i < EStateMax; ++i)
    {
      p_obj->p_states_[i] = NULL;
    }

  /* Add the standard states... */
  p_obj->p_states_[EStateLoaded] = factory_new (tizloaded, p_obj);
  p_obj->p_states_[EStateIdle] = factory_new (tizidle, p_obj);
  p_obj->p_states_[EStateExecuting] = factory_new (tizexecuting, p_obj);
  p_obj->p_states_[EStatePause] = factory_new (tizpause, p_obj);
  p_obj->p_states_[EStateWaitForResources] =
    factory_new (tizwaitforresources, p_obj);
  p_obj->p_states_[ESubStateLoadedToIdle] =
    factory_new (tizloadedtoidle, p_obj);
  p_obj->p_states_[ESubStateIdleToLoaded] =
    factory_new (tizidletoloaded, p_obj);
  p_obj->p_states_[ESubStateIdleToExecuting] =
    factory_new (tizidletoexecuting, p_obj);
  p_obj->p_states_[ESubStateExecutingToIdle] =
    factory_new (tizexecutingtoidle, p_obj);
  p_obj->p_states_[ESubStatePauseToIdle] =
    factory_new (tizpausetoidle, p_obj);

  p_obj->cur_state_id_ = EStateLoaded;
  p_obj->canceled_substate_id_ = EStateMax;
  p_obj->p_current_state_ = p_obj->p_states_[p_obj->cur_state_id_];
  p_obj->in_progress_cmd_ = OMX_CommandMax;
  p_obj->in_progress_param1_ = 0;
  p_obj->cancellation_cmd_ = OMX_CommandMax;

  return p_obj;
}

static void *
fsm_dtor (void *ap_obj)
{
  OMX_U32 i;
  tiz_fsm_t *p_obj = ap_obj;

  p_obj->cur_state_id_ = EStateLoaded;
  p_obj->p_current_state_ = NULL;

  for (i = 0; i < EStateMax; ++i)
    {
      factory_delete (p_obj->p_states_[i]);
      p_obj->p_states_[i] = NULL;
    }

  return super_dtor (tizfsm, ap_obj);
}

static OMX_ERRORTYPE
fsm_SendCommand (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_fsm_t *p_obj = ap_obj;
  tiz_fsm_msg_t *p_msg = NULL;
  tiz_fsm_msg_sendcommand_t *p_msg_sc = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "SendCommand [%s] param1 [%d]",
                 tiz_cmd_to_str (a_cmd), a_param1);

  if (OMX_ErrorNone !=
      (rc = validate_sendcommand (p_obj, ap_hdl, a_cmd,
                                  a_param1, ap_cmd_data)))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] : (Invalid command - Command [%s] a_param1 [%d])",
                     tiz_err_to_str (rc),
                     tiz_cmd_to_str (a_cmd), a_param1);
      return rc;
    }

  if (NULL == (p_msg = init_fsm_message (p_obj, ap_hdl, ETIZFsmMsgSendCommand)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_sc = &(p_msg->sc);
  p_msg_sc->cmd = a_cmd;
  p_msg_sc->param1 = a_param1;
  p_msg_sc->p_cmd_data = ap_cmd_data;

  return tiz_servant_enqueue (p_obj, p_msg,
                             msg_to_priority (ETIZFsmMsgSendCommand));

}

static OMX_ERRORTYPE
fsm_GetParameter (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_fsm_t *p_obj = ap_obj;
  const void *p_krn = tiz_get_krn (ap_hdl);
  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "GetParameter : [%s] [%s]",
                 tiz_fsm_state_to_str (p_obj->cur_state_id_),
                 tiz_idx_to_str (a_index));
  return tiz_api_GetParameter (p_krn, ap_hdl, a_index, ap_struct);
}

static OMX_ERRORTYPE
fsm_SetParameter (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                  OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_fsm_t *p_obj = ap_obj;
  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "SetParameter : [%s] [%s]",
                 tiz_fsm_state_to_str (p_obj->cur_state_id_),
                 tiz_idx_to_str (a_index));
  return tiz_api_SetParameter (p_obj->p_current_state_,
                              ap_hdl, a_index, ap_struct);
}

static OMX_ERRORTYPE
fsm_GetConfig (const void *ap_obj,
               OMX_HANDLETYPE ap_hdl, OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const void *p_krn = tiz_get_krn (ap_hdl);
  return tiz_api_GetConfig (p_krn, ap_hdl, a_index, ap_struct);
}

static OMX_ERRORTYPE
fsm_SetConfig (const void *ap_obj,
               OMX_HANDLETYPE ap_hdl, OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const void *p_krn = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  p_krn = tiz_get_krn (ap_hdl);

  /* Let's get this processed at the kernel */
  return tiz_api_SetConfig (p_krn, ap_hdl, a_index, ap_struct);
}

static OMX_ERRORTYPE
fsm_GetState (const void *ap_obj,
              OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  const tiz_fsm_t *p_obj = ap_obj;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_state);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "GetState [%s]",
                 tiz_fsm_state_to_str (p_obj->cur_state_id_));

  assert (p_obj->cur_state_id_ != EStateMax);

  return tiz_api_GetState (p_obj->p_current_state_, ap_hdl, ap_state);
}

static OMX_ERRORTYPE
fsm_ComponentTunnelRequest (const void *ap_obj,
                            OMX_HANDLETYPE ap_hdl,
                            OMX_U32 a_pid,
                            OMX_HANDLETYPE ap_thdl,
                            OMX_U32 a_tpid, OMX_TUNNELSETUPTYPE * ap_tsetup)
{
  const tiz_fsm_t *p_obj = ap_obj;
  const void *p_krn = NULL;
  const void *p_port = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  p_krn = tiz_get_krn (ap_hdl);
  p_port = tiz_kernel_get_port (p_krn, a_pid);

  if (NULL == p_port)
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorBadParameter] : "
                     "(Bad port index found [%d])...", a_pid);
      return OMX_ErrorBadParameter;
    }

  if ((EStateLoaded != p_obj->cur_state_id_) && TIZ_PORT_IS_ENABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorIncorrectStateOperation] : "
                     "(In state %s, port [%d] enabled)...",
                     tiz_fsm_state_to_str (p_obj->cur_state_id_), a_pid);
      return OMX_ErrorIncorrectStateOperation;
    }

  return tiz_api_ComponentTunnelRequest (p_krn,
                                        ap_hdl,
                                        a_pid, ap_thdl, a_tpid, ap_tsetup);
}

static OMX_ERRORTYPE
fsm_UseBuffer (const void *ap_obj,
               OMX_HANDLETYPE ap_hdl,
               OMX_BUFFERHEADERTYPE ** app_hdr,
               OMX_U32 a_pid,
               OMX_PTR ap_apppriv, OMX_U32 a_size, OMX_U8 * ap_buf)
{
  const tiz_fsm_t *p_obj = ap_obj;
  const void *p_krn = NULL;
  const void *p_port = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  p_krn = tiz_get_krn (ap_hdl);
  p_port = tiz_kernel_get_port (p_krn, a_pid);

  if (NULL == p_port)
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorBadParameter] : (Unknown port index [%d]) ...",
                     a_pid);
      return OMX_ErrorBadParameter;
    }

  if ((ESubStateLoadedToIdle != p_obj->cur_state_id_)
      && TIZ_PORT_IS_ENABLED (p_port) && !TIZ_PORT_IS_BEING_ENABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorIncorrectStateOperation] : "
                     "(API not allowed in the current state - port [%d] "
                     "state [%d])...",
                     a_pid, tiz_fsm_state_to_str (p_obj->cur_state_id_));
      return OMX_ErrorIncorrectStateOperation;
    }

  return tiz_api_UseBuffer (p_krn,
                           ap_hdl,
                           app_hdr, a_pid, ap_apppriv, a_size, ap_buf);
}

static OMX_ERRORTYPE
fsm_AllocateBuffer (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_BUFFERHEADERTYPE ** app_hdr,
                    OMX_U32 a_pid, OMX_PTR ap_apppriv, OMX_U32 a_size)
{
  const tiz_fsm_t *p_obj = ap_obj;
  const void *p_krn = NULL;
  const void *p_port = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  p_krn = tiz_get_krn (ap_hdl);
  p_port = tiz_kernel_get_port (p_krn, a_pid);

  if (NULL == p_port)
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorBadParameter] : (Unknown port index [%d]) ...",
                     a_pid);
      return OMX_ErrorBadParameter;
    }

  if ((ESubStateLoadedToIdle != p_obj->cur_state_id_)
      && TIZ_PORT_IS_ENABLED (p_port) && !TIZ_PORT_IS_BEING_ENABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorIncorrectStateOperation] : "
                     "(API not allowed in the current state - port [%d] "
                     "state [%d])...",
                     a_pid, tiz_fsm_state_to_str (p_obj->cur_state_id_));
      return OMX_ErrorIncorrectStateOperation;
    }

  return tiz_api_AllocateBuffer (p_krn,
                                ap_hdl, app_hdr, a_pid, ap_apppriv, a_size);

}

static OMX_ERRORTYPE
fsm_FreeBuffer (const void *ap_obj,
                OMX_HANDLETYPE ap_hdl,
                OMX_U32 a_pid, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const void *p_krn = NULL;
  const void *p_port = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  p_krn = tiz_get_krn (ap_hdl);
  p_port = tiz_kernel_get_port (p_krn, a_pid);

  if (NULL == p_port)
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorBadParameter] : (Unknown port index [%d]) ...",
                     a_pid);
      return OMX_ErrorBadPortIndex;
    }

  return tiz_api_FreeBuffer (p_krn, ap_hdl, a_pid, ap_hdr);
}

static OMX_ERRORTYPE
fsm_EmptyThisBuffer (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const tiz_fsm_t *p_obj = ap_obj;
  OMX_U32 pid = 0;
  const void *p_krn = NULL;
  const void *p_port = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_hdr);

  pid = ap_hdr->nInputPortIndex;
  p_krn = tiz_get_krn (ap_hdl);
  p_port = tiz_kernel_get_port (p_krn, pid);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "EmptyThisBuffer HEADER = [%p] "
                 "pid = [%d] nAllocLen = [%d] nFilledLen = [%d] "
                 "nOutputPortIndex  = [%d] nInputPortIndex = [%d]",
                 ap_hdr, pid, ap_hdr->nAllocLen, ap_hdr->nFilledLen,
                 ap_hdr->nOutputPortIndex, ap_hdr->nInputPortIndex);

  if (!p_port || (OMX_DirInput != tiz_port_dir (p_port)))
    {
      OMX_ERRORTYPE rc = (p_port ? OMX_ErrorBadParameter
                          : OMX_ErrorBadPortIndex);

      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] : (Bad parameter found...%s)",
                     tiz_err_to_str (rc),
                     p_port ? "port dir" : "port index");
      return rc;
    }

  if (TIZ_PORT_IS_DISABLED (p_port) && !TIZ_PORT_IS_BEING_ENABLED (p_port)
      && !TIZ_PORT_IS_BEING_DISABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorIncorrectStateOperation] : "
                     "(port %d is disabled...)", pid);
      return OMX_ErrorIncorrectStateOperation;
    }

  /* Delegate to the current state... */
  return tiz_api_EmptyThisBuffer (p_obj->p_current_state_, ap_hdl, ap_hdr);
}

static OMX_ERRORTYPE
fsm_FillThisBuffer (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const tiz_fsm_t *p_obj = ap_obj;
  OMX_U32 pid = 0;
  const void *p_krn = NULL;
  const void *p_port = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_hdr);

  pid = ap_hdr->nOutputPortIndex;
  p_krn = tiz_get_krn (ap_hdl);
  p_port = tiz_kernel_get_port (p_krn, pid);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "FillThisBuffer HEADER = [%p] "
                 "pid = [%d] nAllocLen = [%d] nFilledLen = [%d] "
                 "nOutputPortIndex  = [%d] nInputPortIndex = [%d]",
                 ap_hdr, pid, ap_hdr->nAllocLen, ap_hdr->nFilledLen,
                 ap_hdr->nOutputPortIndex, ap_hdr->nInputPortIndex);

  if (!p_port || (OMX_DirOutput != tiz_port_dir (p_port)))
    {
      OMX_ERRORTYPE rc = (p_port ? OMX_ErrorBadParameter
                          : OMX_ErrorBadPortIndex);
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] : (Bad parameter found...%s)", tiz_err_to_str (rc),
                     p_port ? "port dir" : "port index");
      return rc;
    }

  if (TIZ_PORT_IS_DISABLED (p_port) && !TIZ_PORT_IS_BEING_ENABLED (p_port)
      && !TIZ_PORT_IS_BEING_DISABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorIncorrectStateOperation] : "
                     "(port %d is disabled...)", pid);
      return OMX_ErrorIncorrectStateOperation;
    }

  /* Delegate to the current state... */
  return tiz_api_FillThisBuffer (p_obj->p_current_state_, ap_hdl, ap_hdr);

}

static OMX_ERRORTYPE
fsm_SetCallbacks (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_CALLBACKTYPE * ap_cbacks, OMX_PTR app_appdata)
{
  const tiz_fsm_t *p_obj = ap_obj;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  /* only allowed in OMX_StateLoaded state */
  if (EStateLoaded != p_obj->cur_state_id_)
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorIncorrectStateOperation] : "
                     "(API not allowed in %s state...)",
                     tiz_fsm_state_to_str (p_obj->cur_state_id_));
      return OMX_ErrorIncorrectStateOperation;
    }

  /* We only do validation here, so no need to forward this API to the kernel
     of processor */

  return OMX_ErrorNone;
}

/*
 * from tiz_servant api
 */


static OMX_ERRORTYPE
fsm_dispatch_msg (const void *ap_obj, OMX_PTR ap_msg)
{
  tiz_fsm_t *p_obj = (tiz_fsm_t *) ap_obj;
  const tiz_servant_t *p_parent = ap_obj;
  tiz_fsm_msg_t *p_msg = ap_msg;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_obj);
  assert (NULL != p_msg);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "Processing [%s]...", tiz_fsm_msg_to_str (p_msg->class));

  assert (p_msg->class < ETIZFsmMsgMax);

  rc = tiz_fsm_msg_to_fnt_tbl[p_msg->class] ((OMX_PTR) ap_obj, p_msg);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "rc [%s] [%d]...", tiz_err_to_str (rc), rc);

  /* There is an error, check whether we have an ongoing command. If that's the
   * case, this error will cancel the in-progress command. */
  if (OMX_ErrorNone != rc)
    {
      if (OMX_CommandMax != p_obj->in_progress_cmd_)
        {
          tiz_servant_issue_cmd_event
            (p_obj, p_obj->in_progress_cmd_, p_obj->in_progress_param1_, rc);

          p_obj->in_progress_cmd_ = OMX_CommandMax;
          p_obj->in_progress_param1_ = 0;


          /* This is to make sure that the servant base class does not report
           * this error twice */
          rc = OMX_ErrorNone;
        }
    }

  return rc;
}

/*
 * from tiz_fsm api
 */

static OMX_ERRORTYPE
fsm_set_state (const void *ap_obj, tiz_fsm_state_id_t a_new_state,
               tiz_fsm_state_id_t a_canceled_substate)
{
  tiz_fsm_t *p_obj = (tiz_fsm_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = NULL;
  void *p_prc = NULL;
  void *p_krn = NULL;

  assert (NULL != ap_obj);

  p_hdl = tiz_servant_get_hdl (p_obj);
  assert (NULL != p_hdl);

  p_prc = tiz_get_prc (p_hdl);
  p_krn = tiz_get_krn (p_hdl);

  assert (a_new_state < EStateMax);

  if (a_new_state != p_obj->cur_state_id_)
    {
      TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_hdl),
                     TIZ_CBUF (p_hdl), "New state = [%s]..."
                     "cancelled substate = [%s]",
                     tiz_fsm_state_to_str (a_new_state),
                     tiz_fsm_state_to_str (a_canceled_substate));

      p_obj->cur_state_id_ = a_new_state;
      p_obj->p_current_state_ = p_obj->p_states_[a_new_state];

      if (EStateMax != a_canceled_substate)
        {
          p_obj->canceled_substate_id_ = a_canceled_substate;
        }

      if (EStateWaitForResources >= a_new_state)
        {
          tiz_servant_issue_trans_event
            (p_obj, a_new_state,
             p_obj->canceled_substate_id_ == EStateMax
             ? OMX_ErrorNone : OMX_ErrorCommandCanceled);

          TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_hdl),
                         TIZ_CBUF (p_hdl), "in_progress_cmd_ = [%s]...",
                         tiz_cmd_to_str (p_obj->in_progress_cmd_));
          fflush (stdout);
          assert (OMX_CommandStateSet == p_obj->in_progress_cmd_);

          p_obj->in_progress_cmd_ = OMX_CommandMax;
          p_obj->in_progress_param1_ = 0;

          /* TODO: Perhaps implement a transitional state instead of notifying
           * Exe twice */
          if (EStateExecuting == a_new_state)
            {
              /* First notify the kernel servant */
              if (OMX_ErrorNone
                  != (rc = tiz_api_SendCommand (p_krn, p_hdl,
                                               OMX_CommandStateSet,
                                               a_new_state, NULL)))
                {
                  return rc;
                }

              /* Now notify the processor servant */
              if (OMX_ErrorNone
                  != (rc = tiz_api_SendCommand (p_prc, p_hdl,
                                               OMX_CommandStateSet,
                                               a_new_state, NULL)))
                {
                  return rc;
                }

            }

        }

    }

  return OMX_ErrorNone;

}

OMX_ERRORTYPE
tiz_fsm_set_state (void *ap_obj, tiz_fsm_state_id_t a_new_state,
                  tiz_fsm_state_id_t a_canceled_substate)
{
  const tiz_fsm_class_t *class = classOf (ap_obj);
  assert (class->set_state);
  return class->set_state (ap_obj, a_new_state, a_canceled_substate);
}

static OMX_ERRORTYPE
fsm_complete_transition (void *ap_obj, const void *ap_servant,
                         OMX_STATETYPE a_new_state)
{
  tiz_fsm_t *p_obj = ap_obj;
  const tiz_servant_t *p_parent = ap_obj;
  const tiz_servant_t *p_servant = ap_servant;
  tiz_fsm_msg_t *p_msg = NULL;
  tiz_fsm_msg_transcomplete_t *p_msg_tc = NULL;
  OMX_HANDLETYPE p_hdl = NULL;

  assert (NULL != p_obj);
  assert (NULL != p_parent);
  assert (NULL != p_servant);

  p_hdl = p_parent->p_hdl_;
  assert (NULL != p_hdl);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_hdl), TIZ_CBUF (p_hdl),
                 "Servant [%s] notifies transition complete (to state [%s])",
                 nameOf (ap_servant), tiz_fsm_state_to_str (a_new_state));

  /* Validate that this is not an remnant from a cancelled transition */
  if ((ESubStateLoadedToIdle == p_obj->cur_state_id_
       && OMX_StateIdle != a_new_state)
      ||
      (ESubStateIdleToLoaded == p_obj->cur_state_id_
       && OMX_StateLoaded != a_new_state)
      ||
      (ESubStateExecutingToIdle == p_obj->cur_state_id_
       && OMX_StateIdle != a_new_state)
      ||
      (ESubStatePauseToIdle == p_obj->cur_state_id_
       && OMX_StateIdle != a_new_state))
    {
      /* Ignore this as this transition is not relevant anymore */
      TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_hdl), TIZ_CBUF (p_hdl),
                     "[%s] Ignoring Servant [%s] transition complete "
                     "notification (to state [%s])",
                     tiz_fsm_state_to_str (p_obj->cur_state_id_),
                     nameOf (ap_servant), tiz_fsm_state_to_str (a_new_state));
      return OMX_ErrorNone;
    }

  if (NULL ==
      (p_msg = init_fsm_message (p_obj, p_hdl, ETIZFsmMsgTransComplete)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_tc = &(p_msg->tc);
  p_msg_tc->p_servant = (void *) ap_servant;
  p_msg_tc->state = a_new_state;

  return tiz_servant_enqueue (p_obj, p_msg,
                             msg_to_priority (ETIZFsmMsgTransComplete));
}

OMX_ERRORTYPE
tiz_fsm_complete_transition (void *ap_obj, const void *ap_servant,
                            OMX_STATETYPE a_new_state)
{
  const tiz_fsm_class_t *class = classOf (ap_obj);
  assert (class->complete_transition);
  return class->complete_transition (ap_obj, ap_servant, a_new_state);
}

static OMX_ERRORTYPE
fsm_complete_command (void *ap_obj, const void *ap_servant,
                      OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1)
{
  tiz_fsm_t *p_obj = ap_obj;
  const tiz_servant_t *p_parent = ap_obj;
  const tiz_servant_t *p_servant = ap_servant;
  OMX_HANDLETYPE p_hdl = NULL;

  assert (NULL != p_obj);
  assert (NULL != p_parent);
  assert (NULL != p_servant);

  p_hdl = p_parent->p_hdl_;
  assert (NULL != p_hdl);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_hdl), TIZ_CBUF (p_hdl),
                 "Servant [%s] notifies cmd complete (cmd [%s]) "
                 "in_progress_cmd_ [%s] cancellation_cmd_ [%s]",
                 nameOf (ap_servant), tiz_cmd_to_str (a_cmd),
                 tiz_cmd_to_str (p_obj->in_progress_cmd_),
                 tiz_cmd_to_str (p_obj->cancellation_cmd_));

  assert (a_cmd == p_obj->in_progress_cmd_
          || a_cmd == p_obj->cancellation_cmd_);

  if (a_cmd == p_obj->cancellation_cmd_)
    {
      if (p_obj->in_progress_cmd_ != OMX_CommandMax
          && p_obj->in_progress_cmd_ != OMX_CommandStateSet)
        {
          tiz_servant_issue_cmd_event
            (p_obj, p_obj->in_progress_cmd_, a_param1,
             OMX_ErrorCommandCanceled);
          
          p_obj->in_progress_cmd_ = OMX_CommandMax;
          p_obj->in_progress_param1_ = 0;
        }
      else
        {
          tiz_servant_issue_cmd_event
            (p_obj, p_obj->cancellation_cmd_, a_param1, OMX_ErrorNone);
        }
      p_obj->cancellation_cmd_ = OMX_CommandMax;
    }
  else if (a_cmd == p_obj->in_progress_cmd_)
    {
      tiz_servant_issue_cmd_event
        (p_obj, p_obj->in_progress_cmd_, a_param1, OMX_ErrorNone);

      p_obj->in_progress_cmd_ = OMX_CommandMax;
      p_obj->in_progress_param1_ = 0;

    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_fsm_complete_command (void *ap_obj, const void *ap_servant,
                         OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1)
{
  const tiz_fsm_class_t *class = classOf (ap_obj);
  assert (class->complete_command);
  return class->complete_command (ap_obj, ap_servant, a_cmd, a_param1);
}

tiz_fsm_state_id_t
fsm_get_substate (const void *ap_obj)
{
  const tiz_fsm_t *p_obj = ap_obj;
  return p_obj->cur_state_id_;
}

tiz_fsm_state_id_t
tiz_fsm_get_substate (const void *ap_obj)
{
  const tiz_fsm_class_t *class = classOf (ap_obj);
  assert (class->get_substate);
  return class->get_substate (ap_obj);
}

OMX_ERRORTYPE 
fsm_tunneled_ports_status_update (void *ap_obj)
{
  tiz_fsm_t *p_obj = ap_obj;

  /* Delegate to the current state... */
  return tiz_state_tunneled_ports_status_update (p_obj->p_current_state_);
}

OMX_ERRORTYPE
tiz_fsm_tunneled_ports_status_update (void *ap_obj)
{
  const tiz_fsm_class_t *class = classOf (ap_obj);
  assert (class->tunneled_ports_status_update);
  return class->tunneled_ports_status_update (ap_obj);
}

/*
 * tiz_servant_class
 */

static void *
fsm_class_ctor (void *ap_obj, va_list * app)
{
  tiz_fsm_class_t *p_obj = super_ctor (tizfsm_class, ap_obj, app);
  typedef void (*voidf) ();
  voidf selector;
  va_list ap;
  va_copy (ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);

      if (selector == (voidf) tiz_fsm_set_state)
        {
          *(voidf *) & p_obj->set_state = method;
        }
      else if (selector == (voidf) tiz_fsm_complete_transition)
        {
          *(voidf *) & p_obj->complete_transition = method;
        }
      else if (selector == (voidf) tiz_fsm_complete_command)
        {
          *(voidf *) & p_obj->complete_command = method;
        }
      else if (selector == (voidf) tiz_fsm_get_substate)
        {
          *(voidf *) & p_obj->get_substate = method;
        }
      else if (selector == (voidf) tiz_fsm_tunneled_ports_status_update)
        {
          *(voidf *) & p_obj->tunneled_ports_status_update = method;
        }

    }

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

const void *tizfsm, *tizfsm_class;

void
tiz_fsm_init (void)
{

  if (!tizfsm_class)
    {
      tiz_servant_init ();
      tizfsm_class = factory_new (tizservant_class,
                                  "tizfsm_class",
                                  tizservant_class,
                                  sizeof (tiz_fsm_class_t),
                                  ctor, fsm_class_ctor, 0);

    }

  if (!tizstate)
    {
      tiz_state_init_states ();
    }

  if (!tizfsm)
    {
      tiz_servant_init ();
      tizfsm =
        factory_new
        (tizfsm_class,
         "tizfsm",
         tizservant,
         sizeof (tiz_fsm_t),
         ctor, fsm_ctor,
         dtor, fsm_dtor,
         tiz_api_SendCommand, fsm_SendCommand,
         tiz_api_SetParameter, fsm_SetParameter,
         tiz_api_GetParameter, fsm_GetParameter,
         tiz_api_SetConfig, fsm_SetConfig,
         tiz_api_GetConfig, fsm_GetConfig,
         tiz_api_GetState, fsm_GetState,
         tiz_api_ComponentTunnelRequest, fsm_ComponentTunnelRequest,
         tiz_api_UseBuffer, fsm_UseBuffer,
         tiz_api_AllocateBuffer, fsm_AllocateBuffer,
         tiz_api_FreeBuffer, fsm_FreeBuffer,
         tiz_api_EmptyThisBuffer, fsm_EmptyThisBuffer,
         tiz_api_FillThisBuffer, fsm_FillThisBuffer,
         tiz_api_SetCallbacks, fsm_SetCallbacks,
         tiz_servant_dispatch_msg, fsm_dispatch_msg,
         tiz_fsm_set_state, fsm_set_state,
         tiz_fsm_complete_transition, fsm_complete_transition,
         tiz_fsm_complete_command, fsm_complete_command,
         tiz_fsm_get_substate, fsm_get_substate,
         tiz_fsm_tunneled_ports_status_update, fsm_tunneled_ports_status_update,
         0);
    }
}
