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
#include "tizutils.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm"
#endif

/* Forward declarations */
static OMX_ERRORTYPE dispatch_sc (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_tc (void *ap_obj, OMX_PTR ap_msg);

typedef enum tizfsm_msg_class tizfsm_msg_class_t;
enum tizfsm_msg_class
{
  ETIZFsmMsgSendCommand = 0,
  ETIZFsmMsgTransComplete,
  ETIZFsmMsgMax
};

typedef struct tizfsm_msg_sendcommand tizfsm_msg_sendcommand_t;
struct tizfsm_msg_sendcommand
{
  OMX_COMMANDTYPE cmd;
  OMX_U32 param1;
  OMX_PTR p_cmd_data;
};

typedef struct tizfsm_msg_transcomplete tizfsm_msg_transcomplete_t;
struct tizfsm_msg_transcomplete
{
  OMX_PTR p_servant;
  OMX_STATETYPE state;
};

typedef struct tizfsm_msg tizfsm_msg_t;
struct tizfsm_msg
{
  OMX_HANDLETYPE p_hdl;
  tizfsm_msg_class_t class;
  union
  {
    tizfsm_msg_sendcommand_t sc;
    tizfsm_msg_transcomplete_t tc;
  };
};

typedef OMX_ERRORTYPE (*tizfsm_msg_dispatch_f) (void *ap_obj, OMX_PTR ap_msg);
static const tizfsm_msg_dispatch_f tizfsm_msg_to_fnt_tbl[] = {
  dispatch_sc,
  dispatch_tc
};

static OMX_ERRORTYPE
dispatch_sc (void *ap_obj, OMX_PTR ap_msg)
{
  struct tizfsm *p_obj = ap_obj;
  tizfsm_msg_t *p_msg = ap_msg;
  tizfsm_msg_sendcommand_t *p_msg_sc = NULL;

  assert (NULL != p_msg);

  p_msg_sc = &(p_msg->sc);
  assert (NULL != p_msg_sc);
  assert (p_msg_sc->cmd <= OMX_CommandMarkBuffer);

  return tizapi_SendCommand (p_obj->p_current_state_,
                             p_msg->p_hdl,
                             p_msg_sc->cmd,
                             p_msg_sc->param1,
                             p_msg_sc->p_cmd_data);
}

static OMX_ERRORTYPE
dispatch_tc (void *ap_obj, OMX_PTR ap_msg)
{
  struct tizfsm *p_obj = ap_obj;
  tizfsm_msg_t *p_msg = ap_msg;
  tizfsm_msg_transcomplete_t *p_msg_tc = NULL;

  assert (NULL != p_obj);
  assert (NULL != p_msg);

  p_msg_tc = &(p_msg->tc);
  assert (p_msg_tc->state <= OMX_StateWaitForResources);

  return tizstate_trans_complete (p_obj->p_current_state_,
                                  p_msg_tc->p_servant,
                                  p_msg_tc->state);
}

typedef struct tizfsm_msg_str tizfsm_msg_str_t;
struct tizfsm_msg_str
{
  tizfsm_msg_class_t msg;
  OMX_STRING str;
};

static const tizfsm_msg_str_t tizfsm_msg_to_str_tbl[] = {
  {ETIZFsmMsgSendCommand, "ETIZFsmMsgSendCommand"},
  {ETIZFsmMsgTransComplete, "ETIZFsmMsgTransComplete"},
  {ETIZFsmMsgMax, "ETIZFsmMsgMax"},
};

static const OMX_STRING
tizfsm_msg_to_str (tizfsm_msg_class_t a_msg)
{
  const OMX_S32 count =
    sizeof (tizfsm_msg_to_str_tbl) / sizeof (tizfsm_msg_str_t);
  OMX_S32 i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tizfsm_msg_to_str_tbl[i].msg == a_msg)
        {
          return tizfsm_msg_to_str_tbl[i].str;
        }
    }

  return "Unknown fsm message";
}

static inline tizfsm_msg_t *
init_fsm_message (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                  tizfsm_msg_class_t a_msg_class)
{
  struct tizfsm *p_obj = (struct tizfsm *) ap_obj;
  tizfsm_msg_t *p_msg = NULL;

  assert (NULL != p_obj);
  assert (NULL != ap_hdl);
  assert (a_msg_class < ETIZFsmMsgMax);

  if (NULL == (p_msg = tizservant_init_msg (p_obj, sizeof (tizfsm_msg_t))))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                     "[OMX_ErrorInsufficientResources] : "
                     "Could not allocate message [%s]",
                     tizfsm_msg_to_str (a_msg_class));
    }
  else
    {
      p_msg->p_hdl = ap_hdl;
      p_msg->class = a_msg_class;
    }

  return p_msg;
}

static inline OMX_U32
msg_to_priority (tizfsm_msg_class_t a_msg_class)
{
  OMX_U32 prio = 0;

  switch (a_msg_class)
    {
    case ETIZFsmMsgSendCommand:
    case ETIZFsmMsgTransComplete:
      {
        prio = 0;
      }
      break;

    default:
      TIZ_LOG (TIZ_LOG_TRACE, "Unknown msg class [%d]", a_msg_class);
      assert (0);
      break;
    };

  return prio;
}

static OMX_ERRORTYPE
validate_sendcommand (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                      OMX_PTR ap_cmd_data)
{
  const struct tizfsm *p_obj = ap_obj;
  const void *p_krn = tiz_get_krn (ap_hdl);
  struct tizport *p_port = NULL;

  if (OMX_CommandFlush == a_cmd
      || OMX_CommandPortDisable == a_cmd
      || OMX_CommandPortEnable == a_cmd
      || OMX_CommandMarkBuffer == a_cmd)
      {
        /* Validate that the port index is correct */
        /* in those commands where a port index is needed */

        p_port = tizkernel_get_port (p_krn, a_param1);
        assert (NULL != p_port);
        if (a_param1 != OMX_ALL && !p_port)
          {
            return OMX_ErrorBadPortIndex;
          }
      }

  switch (a_cmd)
    {
    case OMX_CommandStateSet:
      {
        /* We need to verify that we can process this command at this specific
           point in time */
        TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                       "SendCommand [%s] cur_state_id_ [%s]",
                       tiz_state_to_str (a_param1),
                       tiz_fsm_state_to_str (p_obj->cur_state_id_));

        if (p_obj->cur_state_id_ > EStateWaitForResources
            && p_obj->cur_state_id_ < EStateMax)
          {
            /* There is an on-going state transition */
            if (ESubStateLoadedToIdle == p_obj->cur_state_id_
                && a_param1 == OMX_StateLoaded)
              {
                OMX_BOOL may_be_fully_unpopulated = OMX_FALSE;
                if (ETIZKernelUnpopulated
                    == tizkernel_get_population_status (p_krn, OMX_ALL,
                                                        &may_be_fully_unpopulated))
                  {
                    if (OMX_FALSE == may_be_fully_unpopulated)
                      {
                        /* This is NOT OK */
                        return OMX_ErrorIncorrectStateOperation;
                      }
                  }
              }
          }
      }
      break;

    case OMX_CommandPortDisable:
      {
        if (p_obj->cur_state_id_ > EStateWaitForResources
            && p_obj->cur_state_id_ < EStateMax)
          {
            /* There is an on-going state transition */
            if (ESubStateLoadedToIdle == p_obj->cur_state_id_)
              {
                OMX_BOOL may_be_fully_unpopulated = OMX_FALSE;
                if (ETIZKernelUnpopulated
                    == tizkernel_get_population_status (p_krn, a_param1,
                                                        &may_be_fully_unpopulated))
                  {
                    if (OMX_FALSE == may_be_fully_unpopulated)
                      {
                        /* This is NOT OK */
                        return OMX_ErrorIncorrectStateOperation;
                      }
                  }
              }
          }

      }
      break;

    case OMX_CommandFlush:
      break;

    case OMX_CommandPortEnable:
      break;

    case OMX_CommandMarkBuffer:
      {
        const OMX_MARKTYPE* p_mark = (OMX_MARKTYPE*) ap_cmd_data;
        if (!p_mark || !p_mark->hMarkTargetComponent)
          {
            return OMX_ErrorBadParameter;
          }

        /* OMX_CommandMarkBuffer shall be allowed in Exe, Paused or port
           Disabled. Reject otherwise */
        if (!(TIZPORT_IS_DISABLED (p_port)
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

  return OMX_ErrorNone;

}

/*
 * tizfsm
 */

static void *
fsm_ctor (void *ap_obj, va_list * app)
{
  OMX_U32 i;
  struct tizfsm *p_obj = super_ctor (tizfsm, ap_obj, app);

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
  p_obj->p_states_[ESubStateExecutingToIdle] =
    factory_new (tizexecutingtoidle, p_obj);
  p_obj->p_states_[ESubStatePauseToIdle] = factory_new (tizpausetoidle, p_obj);

  p_obj->cur_state_id_ = EStateLoaded;
  p_obj->p_current_state_ = p_obj->p_states_[p_obj->cur_state_id_];
  p_obj->canceled_substate_id_ = EStateMax;

  return p_obj;
}

static void *
fsm_dtor (void *ap_obj)
{
  OMX_U32 i;
  struct tizfsm *p_obj = ap_obj;

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
                     OMX_COMMANDTYPE a_cmd,
                     OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const struct tizfsm *p_obj = ap_obj;
  tizfsm_msg_t *p_msg = NULL;
  tizfsm_msg_sendcommand_t *p_msg_sc = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                   "SendCommand [%s] param1 [%d]",
                   tiz_cmd_to_str (a_cmd), a_param1);

  if (OMX_ErrorNone !=
      (rc = validate_sendcommand (p_obj, ap_hdl, a_cmd,
                                  a_param1, ap_cmd_data)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                     "[%s] : Invalid SendCommand message",
                     tiz_err_to_str (rc));
      return rc;
    }

  if (!(p_msg = init_fsm_message (p_obj, ap_hdl, ETIZFsmMsgSendCommand)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_sc             = &(p_msg->sc);
  p_msg_sc->cmd        = a_cmd;
  p_msg_sc->param1     = a_param1;
  p_msg_sc->p_cmd_data = ap_cmd_data;

  return tizservant_enqueue (p_obj, p_msg,
                             msg_to_priority (ETIZFsmMsgSendCommand));

}

static OMX_ERRORTYPE
fsm_GetParameter (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR a_struct)
{
  const struct tizfsm *p_obj = ap_obj;
  const void *p_krn = tiz_get_krn (ap_hdl);
  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                 "GetParameter : [%s] [%s]",
                 tiz_fsm_state_to_str (p_obj->cur_state_id_),
                 tiz_idx_to_str (a_index));
  return tizapi_GetParameter (p_krn, ap_hdl, a_index, a_struct);
}

static OMX_ERRORTYPE
fsm_SetParameter (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR a_struct)
{
  const struct tizfsm *p_obj = ap_obj;
  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                 "SetParameter : [%s] [%s]",
                 tiz_fsm_state_to_str (p_obj->cur_state_id_),
                 tiz_idx_to_str (a_index));
  return tizapi_SetParameter (p_obj->p_current_state_,
                              ap_hdl, a_index, a_struct);
}

static OMX_ERRORTYPE
fsm_GetConfig (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR a_struct)
{
  const void *p_krn = tiz_get_krn (ap_hdl);
  return tizapi_GetConfig (p_krn, ap_hdl, a_index, a_struct);
}

static OMX_ERRORTYPE
fsm_SetConfig (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR a_struct)
{
  const void *p_krn = tiz_get_krn (ap_hdl);
  return tizapi_SetConfig (p_krn, ap_hdl, a_index, a_struct);
}

static OMX_ERRORTYPE
fsm_GetState (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  const struct tizfsm *p_obj = ap_obj;

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                 "GetState [%s]", tiz_fsm_state_to_str(p_obj->cur_state_id_));

  assert (p_obj->cur_state_id_ != EStateMax);

  if (!ap_state)
    {
      return OMX_ErrorBadParameter;
    }

  return tizapi_GetState (p_obj->p_current_state_, ap_hdl, ap_state);
}

static OMX_ERRORTYPE
fsm_ComponentTunnelRequest (const void *ap_obj,
                                OMX_HANDLETYPE ap_hdl,
                                OMX_U32 a_pid,
                                OMX_HANDLETYPE ap_thdl,
                                OMX_U32 a_tpid,
                                OMX_TUNNELSETUPTYPE * ap_tsetup)
{
  const struct tizfsm *p_obj = ap_obj;
  const void *p_krn = tiz_get_krn (ap_hdl);
  const void *p_port = tizkernel_get_port (p_krn, a_pid);

  if (!p_port)
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                 "ComponentTunnelRequest : "
                 "Bad parameter found (port index [%d])...", a_pid);
      return OMX_ErrorBadParameter;
    }

  if ((EStateLoaded != p_obj->cur_state_id_) && TIZPORT_IS_ENABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                       "ComponentTunnelRequest : Incorrect state op "
                       "(In state %s and port [%d] enabled)...",
                       tiz_fsm_state_to_str (p_obj->cur_state_id_), a_pid);
      return OMX_ErrorIncorrectStateOperation;
    }

  return tizapi_ComponentTunnelRequest (p_krn,
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
  const struct tizfsm *p_obj = ap_obj;
  const void *p_krn = tiz_get_krn (ap_hdl);
  const void *p_port = tizkernel_get_port (p_krn, a_pid);

  if (NULL == p_port)
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                     "[OMX_ErrorBadParameter] : Wrong port id [%d]? ...",
                     a_pid);
      return OMX_ErrorBadParameter;
    }

  if ((ESubStateLoadedToIdle != p_obj->cur_state_id_)
      && TIZPORT_IS_ENABLED (p_port) && !TIZPORT_IS_BEING_ENABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                       "[OMX_ErrorIncorrectStateOperation]...");
      return OMX_ErrorIncorrectStateOperation;
    }

  return tizapi_UseBuffer (p_krn,
                           ap_hdl,
                           app_hdr, a_pid, ap_apppriv, a_size, ap_buf);
}

static OMX_ERRORTYPE
fsm_AllocateBuffer (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_BUFFERHEADERTYPE ** app_hdr,
                        OMX_U32 a_pid, OMX_PTR ap_apppriv, OMX_U32 a_size)
{
  const struct tizfsm *p_obj = ap_obj;
  const void *p_krn = tiz_get_krn (ap_hdl);
  const void *p_port = tizkernel_get_port (p_krn, a_pid);

  if (!p_port)
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                     "[OMX_ErrorBadParameter] : Wrong port id [%d]? ...",
                     a_pid);
      return OMX_ErrorBadParameter;
    }

  if ((ESubStateLoadedToIdle != p_obj->cur_state_id_)
      && TIZPORT_IS_ENABLED (p_port) && !TIZPORT_IS_BEING_ENABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                     "[OMX_ErrorIncorrectStateOperation]...");
      return OMX_ErrorIncorrectStateOperation;
    }

  return tizapi_AllocateBuffer (p_krn,
                                ap_hdl,
                                app_hdr, a_pid, ap_apppriv, a_size);

}

static OMX_ERRORTYPE
fsm_FreeBuffer (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_U32 a_pid, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const void *p_krn = tiz_get_krn (ap_hdl);
  const void *p_port = tizkernel_get_port (p_krn, a_pid);

  if (NULL == p_port)
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                     "[OMX_ErrorBadPortIndex] : Wrong port id [%d]? ...",
                     a_pid);
      return OMX_ErrorBadPortIndex;
    }

  return tizapi_FreeBuffer (p_krn, ap_hdl, a_pid, ap_hdr);
}

static OMX_ERRORTYPE
fsm_EmptyThisBuffer (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const struct tizfsm *p_obj = ap_obj;
  const OMX_U32 pid = ap_hdr->nInputPortIndex;
  const void *p_krn = tiz_get_krn (ap_hdl);
  const void *p_port = tizkernel_get_port (p_krn, pid);


  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                   "EmptyThisBuffer HEADER = [%p] "
                   "pid = [%d] nAllocLen = [%d] nFilledLen = [%d] "
                   "nOutputPortIndex  = [%d] nInputPortIndex = [%d]",
                   ap_hdr, pid, ap_hdr->nAllocLen, ap_hdr->nFilledLen,
                   ap_hdr->nOutputPortIndex, ap_hdr->nInputPortIndex);

  if (!p_port || (OMX_DirInput != tizport_dir (p_port)))
    {
      OMX_ERRORTYPE rc = (p_port ? OMX_ErrorBadParameter
                          : OMX_ErrorBadPortIndex);

      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                     "[%s] : Bad parameter found...(%s)", tiz_err_to_str (rc),
                       p_port ? "port dir" : "port index");
      return rc;
    }

  if (TIZPORT_IS_DISABLED (p_port) && !TIZPORT_IS_BEING_ENABLED (p_port)
      && !TIZPORT_IS_BEING_DISABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                     "[OMX_ErrorIncorrectStateOperation] : "
                     "port is disabled...");
      return OMX_ErrorIncorrectStateOperation;
    }

  /* Delegate to the current state... */
  return tizapi_EmptyThisBuffer (p_obj->p_current_state_, ap_hdl, ap_hdr);
}

static OMX_ERRORTYPE
fsm_FillThisBuffer (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const struct tizfsm *p_obj = ap_obj;
  const OMX_U32 pid = ap_hdr->nOutputPortIndex;
  const void *p_krn = tiz_get_krn (ap_hdl);
  const void *p_port = tizkernel_get_port (p_krn, pid);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                   "FillThisBuffer HEADER = [%p] "
                   "pid = [%d] nAllocLen = [%d] nFilledLen = [%d] "
                   "nOutputPortIndex  = [%d] nInputPortIndex = [%d]",
                   ap_hdr, pid, ap_hdr->nAllocLen, ap_hdr->nFilledLen,
                   ap_hdr->nOutputPortIndex, ap_hdr->nInputPortIndex);

  if (!p_port || (OMX_DirOutput != tizport_dir (p_port)))
    {
      OMX_ERRORTYPE rc = (p_port ? OMX_ErrorBadParameter
                          : OMX_ErrorBadPortIndex);
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                     "[%s] : Bad parameter found...(%s)", tiz_err_to_str (rc),
                       p_port ? "port dir" : "port index");
      return rc;
    }

  if (TIZPORT_IS_DISABLED (p_port) && !TIZPORT_IS_BEING_ENABLED (p_port)
      && !TIZPORT_IS_BEING_DISABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                     "[OMX_ErrorIncorrectStateOperation] : "
                     "port is disabled...");
      return OMX_ErrorIncorrectStateOperation;
    }

  /* Delegate to the current state... */
  return tizapi_FillThisBuffer (p_obj->p_current_state_, ap_hdl, ap_hdr);

}

static OMX_ERRORTYPE
fsm_SetCallbacks (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_CALLBACKTYPE * ap_cbacks, OMX_PTR app_appdata)
{
  const struct tizfsm *p_obj = ap_obj;
  assert (p_obj);

  /* only allowed in OMX_StateLoaded state */
  if (EStateLoaded != p_obj->cur_state_id_)
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME(ap_hdl), TIZ_CBUF(ap_hdl),
                     "[OMX_ErrorIncorrectStateOperation] : Not allowed in %s",
                     tiz_fsm_state_to_str (p_obj->cur_state_id_));
      return OMX_ErrorIncorrectStateOperation;
    }

  return OMX_ErrorNone;
}

/*
 * from tizservant api
 */


static OMX_ERRORTYPE
fsm_dispatch_msg (const void *ap_obj, OMX_PTR ap_msg)
{
  const struct tizfsm *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;
  tizfsm_msg_t *p_msg = ap_msg;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_obj);
  assert (NULL != p_msg);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(p_parent->p_hdl_),
                   TIZ_CBUF(p_parent->p_hdl_),
                 "Processing [%s]...", tizfsm_msg_to_str (p_msg->class));

  assert (p_msg->class < ETIZFsmMsgMax);

  rc = tizfsm_msg_to_fnt_tbl[p_msg->class] ((OMX_PTR) ap_obj, p_msg);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(p_parent->p_hdl_),
                   TIZ_CBUF(p_parent->p_hdl_),
                 "rc [%s] [%d]...", tiz_err_to_str (rc), rc);

  return rc;

}

/*
 * from tizfsm api
 */

static OMX_ERRORTYPE
fsm_set_state (const void *ap_obj, tizfsm_state_id_t a_new_state,
                   tizfsm_state_id_t a_canceled_substate)
{
  struct tizfsm *p_obj = (struct tizfsm *) ap_obj;
  OMX_HANDLETYPE p_hdl = tizservant_get_hdl (p_obj);
  struct tizproc *p_prc = tiz_get_prc (p_hdl);
  struct tizkernel *p_krn = tiz_get_krn (p_hdl);
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (a_new_state < EStateMax);

  if (a_new_state != p_obj->cur_state_id_)
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(p_hdl),
                     TIZ_CBUF(p_hdl), "New state = [%s]..."
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
          tizservant_issue_trans_event
            (p_obj, a_new_state,
             p_obj->canceled_substate_id_ == EStateMax
             ? OMX_ErrorNone : OMX_ErrorCommandCanceled);

          /* TODO: Perhaps implement a transitional state instead of notifying
             Exe twice */
          if (EStateExecuting == a_new_state)
            {
              /* First notify the kernel servant */
              if (OMX_ErrorNone
                  != (rc = tizapi_SendCommand (p_krn, p_hdl,
                                               OMX_CommandStateSet,
                                               a_new_state, NULL)))
                {
                  return rc;
                }

              /* Now notify the processor servant */
              if (OMX_ErrorNone
                  != (rc = tizapi_SendCommand (p_prc, p_hdl,
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
tizfsm_set_state (void *ap_obj, tizfsm_state_id_t a_new_state,
                  tizfsm_state_id_t a_canceled_substate)
{
  const struct tizfsm_class *class = classOf (ap_obj);
  assert (class->set_state);
  return class->set_state (ap_obj, a_new_state, a_canceled_substate);
}

static OMX_ERRORTYPE
fsm_complete_transition (const void *ap_obj, const void * ap_servant,
                             OMX_STATETYPE a_new_state)
{
  const struct tizfsm *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;
  const struct tizservant *p_servant = ap_servant;
  tizfsm_msg_t *p_msg = NULL;
  tizfsm_msg_transcomplete_t *p_msg_tc = NULL;
  OMX_HANDLETYPE p_hdl = NULL;

  assert (NULL != p_obj);
  assert (NULL != p_parent);
  assert (NULL != p_servant);

  p_hdl = p_parent->p_hdl_;
  assert (NULL != p_hdl);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(p_hdl), TIZ_CBUF(p_hdl),
                 "Servant [%s] notifies transition complete (to state [%s])",
                 nameOf(ap_servant),
                 tiz_fsm_state_to_str (a_new_state));

  /* Validate that this is not an remnant from a cancelled transition */
  if ( (ESubStateLoadedToIdle == p_obj->cur_state_id_
        && OMX_StateIdle != a_new_state)
       ||
       (ESubStateIdleToLoaded == p_obj->cur_state_id_
        && OMX_StateLoaded != a_new_state)
       ||
       (ESubStateExecutingToIdle == p_obj->cur_state_id_
        && OMX_StateIdle != a_new_state)
       ||
       (ESubStatePauseToIdle == p_obj->cur_state_id_
        && OMX_StateIdle != a_new_state) )
    {
      /* Ignore this as this transition is not relevant anymore */
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(p_hdl), TIZ_CBUF(p_hdl),
                     "[%s] Ignoring Servant [%s] transition complete "
                     "notification (to state [%s])",
                     tiz_fsm_state_to_str (p_obj->cur_state_id_),
                     nameOf(ap_servant),
                     tiz_fsm_state_to_str (a_new_state));
      return OMX_ErrorNone;
    }

  if (NULL == (p_msg = init_fsm_message (p_obj, p_hdl, ETIZFsmMsgTransComplete)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_tc            = &(p_msg->tc);
  p_msg_tc->p_servant = (void *) ap_servant;
  p_msg_tc->state     = a_new_state;

  return tizservant_enqueue (p_obj, p_msg,
                             msg_to_priority (ETIZFsmMsgTransComplete));
}

OMX_ERRORTYPE
tizfsm_complete_transition (const void *ap_obj, const void * ap_servant,
                            OMX_STATETYPE a_new_state)
{
  const struct tizfsm_class *class = classOf (ap_obj);
  assert (class->complete_transition);
  return class->complete_transition (ap_obj, ap_servant, a_new_state);
}

tizfsm_state_id_t
fsm_get_substate (const void *ap_obj)
{
  const struct tizfsm *p_obj = ap_obj;
  return p_obj->cur_state_id_;
}

tizfsm_state_id_t
tizfsm_get_substate (const void *ap_obj)
{
  const struct tizfsm_class *class = classOf (ap_obj);
  assert (class->get_substate);
  return class->get_substate (ap_obj);
}

/*
 * tizservant_class
 */

static void *
fsm_class_ctor (void *ap_obj, va_list * app)
{
  struct tizfsm_class *p_obj = super_ctor (tizfsm_class, ap_obj, app);
  typedef void (*voidf) ();
  voidf selector;
  va_list ap;
  va_copy(ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) tizfsm_set_state)
        {
          *(voidf *) & p_obj->set_state = method;
        }
      else if (selector == (voidf) tizfsm_complete_transition)
        {
          *(voidf *) & p_obj->complete_transition = method;
        }
      else if (selector == (voidf) tizfsm_get_substate)
        {
          *(voidf *) & p_obj->get_substate = method;
        }

    }

  va_end(ap);
  return p_obj;
}

/*
 * initialization
 */

const void *tizfsm, *tizfsm_class;

void
init_tizfsm (void)
{

  if (!tizfsm_class)
    {
      init_tizservant ();
      tizfsm_class = factory_new (tizservant_class,
                                  "tizfsm_class",
                                  tizservant_class,
                                  sizeof (struct tizfsm_class),
                                  ctor, fsm_class_ctor, 0);

    }

  if (!tizstate)
    {
      init_tizstates ();
    }

  if (!tizfsm)
    {
      init_tizservant ();
      tizfsm =
        factory_new
        (tizfsm_class,
         "tizfsm",
         tizservant,
         sizeof (struct tizfsm),
         ctor, fsm_ctor,
         dtor, fsm_dtor,
         tizapi_SendCommand, fsm_SendCommand,
         tizapi_SetParameter, fsm_SetParameter,
         tizapi_GetParameter, fsm_GetParameter,
         tizapi_SetConfig, fsm_SetConfig,
         tizapi_GetConfig, fsm_GetConfig,
         tizapi_GetState, fsm_GetState,
         tizapi_ComponentTunnelRequest, fsm_ComponentTunnelRequest,
         tizapi_UseBuffer, fsm_UseBuffer,
         tizapi_AllocateBuffer, fsm_AllocateBuffer,
         tizapi_FreeBuffer, fsm_FreeBuffer,
         tizapi_EmptyThisBuffer, fsm_EmptyThisBuffer,
         tizapi_FillThisBuffer, fsm_FillThisBuffer,
         tizapi_SetCallbacks, fsm_SetCallbacks,
         tizservant_dispatch_msg, fsm_dispatch_msg,
         tizfsm_set_state, fsm_set_state,
         tizfsm_complete_transition, fsm_complete_transition,
         tizfsm_get_substate, fsm_get_substate, 0);
    }
}
