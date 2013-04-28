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
 * @file   tizproc.c
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
#include "tizproc.h"
#include "tizproc_decls.h"
#include "tizutils.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.proc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE dispatch_sc (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_br (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_eio (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_etmr (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_estat (void *ap_obj, OMX_PTR ap_msg);

typedef struct tiz_prc_msg_sendcommand tiz_prc_msg_sendcommand_t;
static OMX_ERRORTYPE dispatch_state_set (const void *ap_obj,
                                         OMX_HANDLETYPE p_hdl,
                                         tiz_prc_msg_sendcommand_t *
                                         ap_msg_sc);
static OMX_ERRORTYPE dispatch_port_disable (const void *ap_obj,
                                            OMX_HANDLETYPE p_hdl,
                                            tiz_prc_msg_sendcommand_t *
                                            ap_msg_sc);
static OMX_ERRORTYPE dispatch_port_enable (const void *ap_obj,
                                           OMX_HANDLETYPE p_hdl,
                                           tiz_prc_msg_sendcommand_t *
                                           ap_msg_sc);
static OMX_ERRORTYPE dispatch_port_flush (const void *ap_obj,
                                          OMX_HANDLETYPE p_hdl,
                                          tiz_prc_msg_sendcommand_t *
                                          ap_msg_sc);

typedef enum tiz_prc_msg_class tiz_prc_msg_class_t;
enum tiz_prc_msg_class
  {
    ETIZPrcMsgSendCommand = 0,
    ETIZPrcMsgBuffersReady,
    ETIZPrcMsgEvIo,
    ETIZPrcMsgEvTimer,
    ETIZPrcMsgEvStat,
    ETIZPrcMsgMax,
  };

typedef struct tiz_prc_msg_sendcommand tiz_prc_msg_sendcommand_t;
struct tiz_prc_msg_sendcommand
{
  OMX_COMMANDTYPE cmd;
  OMX_U32 param1;
  OMX_PTR p_cmd_data;
};

typedef struct tiz_prc_msg_buffersready tiz_prc_msg_buffersready_t;
struct tiz_prc_msg_buffersready
{
  OMX_BUFFERHEADERTYPE *p_buffer;
  OMX_U32 pid;
};

typedef struct tiz_prc_msg_ev_io tiz_prc_msg_ev_io_t;
struct tiz_prc_msg_ev_io
{
  tiz_event_io_t * p_ev_io;
  int fd;
  int events;
};

typedef struct tiz_prc_msg_ev_timer tiz_prc_msg_ev_timer_t;
struct tiz_prc_msg_ev_timer
{
  tiz_event_timer_t * p_ev_timer;
};

typedef struct tiz_prc_msg_ev_stat tiz_prc_msg_ev_stat_t;
struct tiz_prc_msg_ev_stat
{
  tiz_event_stat_t * p_ev_stat;
  int events;
};

typedef struct tiz_prc_msg tiz_prc_msg_t;
struct tiz_prc_msg
{
  OMX_HANDLETYPE p_hdl;
  tiz_prc_msg_class_t class;
  union
  {
    tiz_prc_msg_sendcommand_t sc;
    tiz_prc_msg_buffersready_t br;
    tiz_prc_msg_ev_io_t eio;
    tiz_prc_msg_ev_timer_t etmr;
    tiz_prc_msg_ev_stat_t estat;
  };
};

typedef OMX_ERRORTYPE (*tiz_prc_msg_dispatch_f) (void *ap_obj,
                                                 OMX_PTR ap_msg);

static const tiz_prc_msg_dispatch_f tiz_prc_msg_to_fnt_tbl[] = {
  dispatch_sc,
  dispatch_br,
  dispatch_eio,
  dispatch_etmr,
  dispatch_estat,
};

typedef OMX_ERRORTYPE (*tiz_prc_msg_dispatch_sc_f)
(const void *ap_obj, OMX_HANDLETYPE p_hdl,
 tiz_prc_msg_sendcommand_t * ap_msg_sc);

static const tiz_prc_msg_dispatch_sc_f tiz_prc_msg_dispatch_sc_to_fnt_tbl[] = {
  dispatch_state_set,
  dispatch_port_flush,
  dispatch_port_disable,
  dispatch_port_enable
};

static OMX_ERRORTYPE
dispatch_sc (void *ap_obj, OMX_PTR ap_msg)
{
  tiz_prc_t *p_obj = ap_obj;
  tiz_prc_msg_t *p_msg = ap_msg;
  tiz_prc_msg_sendcommand_t *p_msg_sc = NULL;

  assert (NULL != p_msg);

  p_msg_sc = &(p_msg->sc);
  assert (NULL != p_msg_sc);
  /* NOTE: Mark buffer command is not hdld in this class */
  assert (p_msg_sc->cmd < OMX_CommandMarkBuffer);

  return tiz_prc_msg_dispatch_sc_to_fnt_tbl[p_msg_sc->cmd] (p_obj,
                                                            p_msg->p_hdl,
                                                            p_msg_sc);
}

static OMX_ERRORTYPE
dispatch_br (void *ap_obj, OMX_PTR ap_msg)
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

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_msg->p_hdl),
                 TIZ_CBUF (p_msg->p_hdl),
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
  if (EStatePause != now
      && ESubStateExecutingToIdle != now
      && ESubStateExecutingToIdle != now
      && ESubStatePauseToIdle != now
      && !TIZ_PORT_IS_DISABLED (p_port)
      && !TIZ_PORT_IS_BEING_DISABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_msg->p_hdl),
                     TIZ_CBUF (p_msg->p_hdl),
                     "p_msg_br->p_buffer [%p] ", p_msg_br->p_buffer);
      rc = tiz_prc_buffers_ready (p_obj);
    }

  return rc;
}

static OMX_ERRORTYPE
dispatch_eio (void *ap_obj, OMX_PTR ap_msg)
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

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_msg->p_hdl),
                 TIZ_CBUF (p_msg->p_hdl),
                 "p_msg->p_hdl [%p] "
                 "p_msg_br->pid = [%d] p_port [%p]",
                 p_msg->p_hdl, p_msg_br->pid, p_port);

  assert (p_port);

  /* Do not notify this buffer in OMX_StatePause or if the port is disabled or
   * being disabled */
  if (EStatePause != now && !TIZ_PORT_IS_DISABLED (p_port)
      && !TIZ_PORT_IS_BEING_DISABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_msg->p_hdl),
                     TIZ_CBUF (p_msg->p_hdl),
                     "p_msg_br->p_buffer [%p] ", p_msg_br->p_buffer);
      rc = tiz_prc_buffers_ready (p_obj);
    }

  return rc;
}

static OMX_ERRORTYPE
dispatch_etmr (void *ap_obj, OMX_PTR ap_msg)
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

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_msg->p_hdl),
                 TIZ_CBUF (p_msg->p_hdl),
                 "p_msg->p_hdl [%p] "
                 "p_msg_br->pid = [%d] p_port [%p]",
                 p_msg->p_hdl, p_msg_br->pid, p_port);

  assert (p_port);

  /* Do not notify this buffer in OMX_StatePause or if the port is disabled or
   * being disabled */
  if (EStatePause != now && !TIZ_PORT_IS_DISABLED (p_port)
      && !TIZ_PORT_IS_BEING_DISABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_msg->p_hdl),
                     TIZ_CBUF (p_msg->p_hdl),
                     "p_msg_br->p_buffer [%p] ", p_msg_br->p_buffer);
      rc = tiz_prc_buffers_ready (p_obj);
    }

  return rc;
}

static OMX_ERRORTYPE
dispatch_estat (void *ap_obj, OMX_PTR ap_msg)
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

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_msg->p_hdl),
                 TIZ_CBUF (p_msg->p_hdl),
                 "p_msg->p_hdl [%p] "
                 "p_msg_br->pid = [%d] p_port [%p]",
                 p_msg->p_hdl, p_msg_br->pid, p_port);

  assert (p_port);

  /* Do not notify this buffer in OMX_StatePause or if the port is disabled or
   * being disabled */
  if (EStatePause != now && !TIZ_PORT_IS_DISABLED (p_port)
      && !TIZ_PORT_IS_BEING_DISABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_msg->p_hdl),
                     TIZ_CBUF (p_msg->p_hdl),
                     "p_msg_br->p_buffer [%p] ", p_msg_br->p_buffer);
      rc = tiz_prc_buffers_ready (p_obj);
    }

  return rc;
}

static OMX_ERRORTYPE
dispatch_state_set (const void *ap_obj, OMX_HANDLETYPE p_hdl,
                    tiz_prc_msg_sendcommand_t * ap_msg_sc)
{
  const tiz_prc_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_STATETYPE now = OMX_StateMax;
  OMX_BOOL done = OMX_FALSE;

  TIZ_LOG (TIZ_TRACE, "Requested transition to state [%s]",
           tiz_fsm_state_to_str (ap_msg_sc->param1));

  /* Obtain the current state */
  tiz_api_GetState (tiz_get_fsm (p_hdl), p_hdl, &now);

  switch (ap_msg_sc->param1)
    {
    case OMX_StateLoaded:
      {
        if (OMX_StateIdle == now)
          {
            rc = tiz_srv_deallocate_resources (p_obj);
            done = OMX_TRUE;
          }
        else if (OMX_StateWaitForResources == now)
          {
            done = OMX_TRUE;
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
            rc = tiz_srv_allocate_resources (p_obj, OMX_ALL);
            done = OMX_TRUE;
          }
        else if (OMX_StateExecuting == now || OMX_StatePause == now)
          {
            rc = tiz_srv_stop_and_return (p_obj);
            done = OMX_TRUE;

          }
        else if (OMX_StateIdle == now)
          {
            /* TODO : review when this situation would occur  */
            TIZ_LOG_CNAME (TIZ_WARN, TIZ_CNAME (p_hdl), TIZ_CBUF (p_hdl),
                           "Ignoring transition [%s] -> [%s]",
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
            rc = tiz_srv_prepare_to_transfer (p_obj, OMX_ALL);
            done = OMX_TRUE;
          }
        else if (OMX_StatePause == now)
          {
            done = OMX_TRUE;
          }
        else if (OMX_StateExecuting == now)
          {
            rc = tiz_srv_transfer_and_process (p_obj, OMX_ALL);
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
        done = OMX_TRUE;
        break;
      }

    default:
      {
        assert (0);
        break;
      }

    };

  if (OMX_ErrorNone == rc && OMX_TRUE == done)
    {
      rc = tiz_fsm_complete_transition
        (tiz_get_fsm (p_hdl), p_obj, ap_msg_sc->param1);
    }

  TIZ_LOG (TIZ_TRACE, "rc [%s]", tiz_err_to_str (rc));

  return rc;
}

static OMX_ERRORTYPE
dispatch_port_flush (const void *ap_obj, OMX_HANDLETYPE p_hdl,
                     tiz_prc_msg_sendcommand_t * ap_msg_sc)
{
  assert (ap_msg_sc);
  return tiz_prc_port_flush (ap_obj, ap_msg_sc->param1);
}

static OMX_ERRORTYPE
dispatch_port_disable (const void *ap_obj, OMX_HANDLETYPE p_hdl,
                       tiz_prc_msg_sendcommand_t * ap_msg_sc)
{
  assert (ap_msg_sc);
  return tiz_prc_port_disable (ap_obj, ap_msg_sc->param1);
}

static OMX_ERRORTYPE
dispatch_port_enable (const void *ap_obj, OMX_HANDLETYPE p_hdl,
                      tiz_prc_msg_sendcommand_t * ap_msg_sc)
{
  assert (ap_msg_sc);
  return tiz_prc_port_enable (ap_obj, ap_msg_sc->param1);
}

typedef struct tiz_prc_msg_str tiz_prc_msg_str_t;
struct tiz_prc_msg_str
{
  tiz_prc_msg_class_t msg;
  OMX_STRING str;
};

tiz_prc_msg_str_t tiz_prc_msg_to_str_tbl[] = {
  {ETIZPrcMsgSendCommand, "ETIZPrcMsgSendCommand"},
  {ETIZPrcMsgBuffersReady, "ETIZPrcMsgBuffersReady"},
  {ETIZPrcMsgMax, "ETIZPrcMsgMax"},
};

static const OMX_STRING
tiz_prc_msg_to_str (tiz_prc_msg_class_t a_msg)
{
  const OMX_S32 count =
    sizeof (tiz_prc_msg_to_str_tbl) / sizeof (tiz_prc_msg_str_t);
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

static OMX_BOOL
remove_buffer_from_servant_queue (OMX_PTR ap_elem, OMX_S32 a_data1,
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
          TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_msg->p_hdl),
                         TIZ_CBUF (p_msg->p_hdl),
                         "tiz_prc_msg_buffersready_t : Found HEADER [%p]",
                         p_hdr);
          rc = OMX_TRUE;
        }
    }
  else
    {
      /* Not interested */
      TIZ_LOG (TIZ_TRACE,
               "Not interested : class  [%s]",
               tiz_prc_msg_to_str (p_msg->class));
    }

  return rc;
}


/*
 * tiz_prc
 */

static void *
proc_ctor (void *ap_obj, va_list * app)
{
  tiz_prc_t *p_obj = super_ctor (tizprc, ap_obj, app);
  return p_obj;
}

static void *
proc_dtor (void *ap_obj)
{
  return super_dtor (tizprc, ap_obj);
}

static inline tiz_prc_msg_t *
init_proc_message (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                   tiz_prc_msg_class_t a_msg_class)
{
  tiz_prc_t *p_obj = (tiz_prc_t *) ap_obj;
  tiz_prc_msg_t *p_msg = NULL;

  assert (NULL != p_obj);
  assert (NULL != ap_hdl);
  assert (a_msg_class < ETIZPrcMsgMax);

  if (NULL == (p_msg = tiz_srv_init_msg (p_obj, sizeof (tiz_prc_msg_t))))
    {
      TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
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

static OMX_ERRORTYPE
enqueue_buffersready_msg (const void *ap_obj,
                          OMX_HANDLETYPE ap_hdl,
                          OMX_BUFFERHEADERTYPE * ap_hdr, OMX_U32 a_pid)
{
  const tiz_prc_t *p_obj = ap_obj;
  tiz_prc_msg_t *p_msg = NULL;
  tiz_prc_msg_buffersready_t *p_msg_br = NULL;

  TIZ_LOG (TIZ_TRACE, "BuffersReady : HEADER [%p]", ap_hdr);

  if (NULL == (p_msg = init_proc_message (p_obj, ap_hdl,
                                          ETIZPrcMsgBuffersReady)))
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

/*
 * from tiz_api
 */

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
      {
        prio = 0;
      }
      break;

    case OMX_CommandMarkBuffer:
      /* NOTE: Mark buffer command is not hdld in this class */
    default:
      TIZ_LOG (TIZ_TRACE, "Unknown/unhandled command class [%d]", a_cmd);
      assert (0);
      break;
    };

  return prio;
}

static OMX_ERRORTYPE
proc_SendCommand (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_COMMANDTYPE a_cmd,
                  OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_prc_t *p_obj = ap_obj;
  tiz_prc_msg_t *p_msg = NULL;
  tiz_prc_msg_sendcommand_t *p_msg_sc = NULL;

  TIZ_LOG (TIZ_TRACE, "SendCommand [%s]", tiz_cmd_to_str (a_cmd));

  if (NULL == (p_msg = init_proc_message (p_obj, ap_hdl,
                                          ETIZPrcMsgSendCommand)))
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

static OMX_ERRORTYPE
proc_EmptyThisBuffer (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_buf)
{
  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl),
                 TIZ_CBUF (ap_hdl), "pid [%d]", ap_buf->nInputPortIndex);

  return enqueue_buffersready_msg (ap_obj, ap_hdl, ap_buf,
                                   ap_buf->nInputPortIndex);
}

static OMX_ERRORTYPE
proc_FillThisBuffer (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_buf)
{
  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl),
                 TIZ_CBUF (ap_hdl), "pid [%d]", ap_buf->nOutputPortIndex);

  return enqueue_buffersready_msg (ap_obj, ap_hdl, ap_buf,
                                   ap_buf->nOutputPortIndex);
}

/*
 * from tiz_srv api
 */

static OMX_ERRORTYPE
proc_remove_from_queue (const void *ap_obj, tiz_pq_func_f apf_func,
                        OMX_S32 a_data1, OMX_PTR ap_data2)
{
  tiz_srv_t *p_obj = (tiz_srv_t *) ap_obj;
  /* Actual implementation is in the parent class */
  /* Replace dummy parameters apf_func and a_data1 */
  return tiz_srv_super_remove_from_queue
    (tizprc, p_obj,
     &remove_buffer_from_servant_queue, ETIZPrcMsgBuffersReady, ap_data2);
}

static OMX_ERRORTYPE
proc_dispatch_msg (const void *ap_obj, OMX_PTR ap_msg)
{
  const tiz_prc_t *p_obj = ap_obj;
  const tiz_srv_t *p_parent = ap_obj;
  tiz_prc_msg_t *p_msg = ap_msg;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_obj);
  assert (NULL != p_msg);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "Processing [%s]...", tiz_prc_msg_to_str (p_msg->class));

  assert (p_msg->class < ETIZPrcMsgMax);

  rc = tiz_prc_msg_to_fnt_tbl[p_msg->class] ((OMX_PTR) ap_obj, p_msg);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "rc [%s]...", tiz_err_to_str (rc));

  return rc;
}

static OMX_ERRORTYPE
proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  assert (ap_obj);
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
proc_deallocate_resources (void *ap_obj)
{
  assert (ap_obj);
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  assert (ap_obj);
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  assert (ap_obj);
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
proc_stop_and_return (void *ap_obj)
{
  assert (ap_obj);
  return OMX_ErrorNotImplemented;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
proc_buffers_ready (const void *ap_obj)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_prc_buffers_ready (const void *ap_obj)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (class->buffers_ready);
  return class->buffers_ready (ap_obj);
}

OMX_ERRORTYPE
tiz_prc_super_buffers_ready (const void *a_class, const void *ap_obj)
{
  const tiz_prc_class_t *superclass = super (a_class);
  assert (ap_obj && superclass->buffers_ready);
  return superclass->buffers_ready (ap_obj);
}

static OMX_ERRORTYPE
proc_port_flush (const void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_port_flush (const void *ap_obj, OMX_U32 a_pid)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (class->port_flush);
  return class->port_flush (ap_obj, a_pid);
}

static OMX_ERRORTYPE
proc_port_disable (const void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_port_disable (const void *ap_obj, OMX_U32 a_pid)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (class->port_disable);
  return class->port_disable (ap_obj, a_pid);
}

static OMX_ERRORTYPE
proc_port_enable (const void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_port_enable (const void *ap_obj, OMX_U32 a_pid)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (class->port_enable);
  return class->port_enable (ap_obj, a_pid);
}

static OMX_ERRORTYPE
proc_io_ready (const void *ap_obj,
               tiz_event_io_t * ap_ev_io, int a_fd,
               int a_events)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_io_ready (void *ap_obj,
                  tiz_event_io_t * ap_ev_io, int a_fd,
                  int a_events)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (class->io_ready);
  return class->io_ready (ap_obj, ap_ev_io, a_fd, a_events);
}

static OMX_ERRORTYPE
proc_timer_ready (void *ap_obj,
                  tiz_event_timer_t * ap_ev_timer, void *ap_arg)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_timer_ready (void *ap_obj,
                     tiz_event_timer_t * ap_ev_timer,
                     void *ap_arg)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (class->timer_ready);
  return class->timer_ready (ap_obj, ap_ev_timer, ap_arg);
}

static OMX_ERRORTYPE
proc_stat_ready (void *ap_obj,
                 tiz_event_stat_t * ap_ev_stat,
                 int a_events)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_prc_stat_ready (void *ap_obj,
                    tiz_event_stat_t * ap_ev_stat,
                    int a_events)
{
  const tiz_prc_class_t *class = classOf (ap_obj);
  assert (class->stat_ready);
  return class->stat_ready (ap_obj, ap_ev_stat, a_events);
}

/*
 * tiz_prc_class
 */

static void *
proc_class_ctor (void *ap_obj, va_list * app)
{
  tiz_prc_class_t *p_obj = super_ctor (tizprc_class, ap_obj, app);
  typedef void (*voidf) ();
  voidf selector;
  va_list ap;
  va_copy (ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) tiz_prc_buffers_ready)
        {
          *(voidf *) & p_obj->buffers_ready = method;
        }
      else if (selector == (voidf) tiz_prc_port_flush)
        {
          *(voidf *) & p_obj->port_flush = method;
        }
      else if (selector == (voidf) tiz_prc_port_disable)
        {
          *(voidf *) & p_obj->port_disable = method;
        }
      else if (selector == (voidf) tiz_prc_port_enable)
        {
          *(voidf *) & p_obj->port_enable = method;
        }
      else if (selector == (voidf) tiz_prc_io_ready)
        {
          *(voidf *) & p_obj->io_ready = method;
        }
      else if (selector == (voidf) tiz_prc_timer_ready)
        {
          *(voidf *) & p_obj->timer_ready = method;
        }
      else if (selector == (voidf) tiz_prc_stat_ready)
        {
          *(voidf *) & p_obj->stat_ready = method;
        }
    }

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

const void *tizprc, *tizprc_class;

void
tiz_prc_init (void)
{

  if (!tizprc_class)
    {
      tiz_srv_init ();
      tizprc_class = factory_new (tizsrv_class,
                                  "tizprc_class",
                                  tizsrv_class,
                                  sizeof (tiz_prc_class_t),
                                  ctor, proc_class_ctor, 0);

    }

  if (!tizprc)
    {
      tiz_srv_init ();
      tizprc =
        factory_new
        (tizprc_class,
         "tizprc",
         tizsrv,
         sizeof (tiz_prc_t),
         ctor, proc_ctor,
         dtor, proc_dtor,
         tiz_api_EmptyThisBuffer, proc_EmptyThisBuffer,
         tiz_api_FillThisBuffer, proc_FillThisBuffer,
         tiz_api_SendCommand, proc_SendCommand,
         tiz_srv_remove_from_queue, proc_remove_from_queue,
         tiz_srv_dispatch_msg, proc_dispatch_msg,
         tiz_srv_allocate_resources, proc_allocate_resources,
         tiz_srv_deallocate_resources, proc_deallocate_resources,
         tiz_srv_prepare_to_transfer, proc_prepare_to_transfer,
         tiz_srv_transfer_and_process, proc_transfer_and_process,
         tiz_srv_stop_and_return, proc_stop_and_return,
         tiz_prc_buffers_ready, proc_buffers_ready,
         tiz_prc_port_flush, proc_port_flush,
         tiz_prc_port_disable, proc_port_disable,
         tiz_prc_port_enable, proc_port_enable,
         tiz_prc_io_ready, proc_io_ready,
         tiz_prc_timer_ready, proc_timer_ready,
         tiz_prc_stat_ready, proc_stat_ready,
         0);
    }

}
