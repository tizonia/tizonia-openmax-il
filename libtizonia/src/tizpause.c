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
 * @file   tizpause.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Pause OMX IL state implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizscheduler.h"
#include "tizkernel.h"
#include "tizport.h"
#include "tizpause.h"
#include "tizstate_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.pause"
#endif


static void *
pause_ctor (void *ap_obj, va_list * app)
{
  struct tizpause *p_obj = super_ctor (tizpause, ap_obj, app);
  return p_obj;
}

static void *
pause_dtor (void *ap_obj)
{
  return super_dtor (tizpause, ap_obj);
}

static OMX_ERRORTYPE
pause_SetParameter (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR a_struct)
{
  const void *p_krn = tiz_get_krn (ap_hdl);
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;

  /* TODO: Optimization: find_managing_port is called twice, first time here,
   * then in the SetParameter implementation of the kernel object. */
  if (OMX_ErrorNone
      != (ret_val =
          tizkernel_find_managing_port (p_krn, a_index, a_struct, &p_port)))
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Cannot retrieve managing port (%s)...",
                 tiz_err_to_str (ret_val));
      return ret_val;
    }

  assert (p_port);

  if (TIZPORT_IS_CONFIG_PORT (p_port)
      || (!TIZPORT_IS_CONFIG_PORT (p_port) && TIZPORT_IS_ENABLED (p_port)))
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Incorrect state op "
                 "(SetParameter received in Pause state)...");
      return OMX_ErrorIncorrectStateOperation;
    }

  return tizapi_SetParameter (p_krn, ap_hdl, a_index, a_struct);

}

static OMX_ERRORTYPE
pause_GetState (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  * ap_state = OMX_StatePause;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
pause_UseBuffer (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl,
                     OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                     OMX_U32 a_port_index,
                     OMX_PTR ap_app_private,
                     OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
pause_EmptyThisBuffer (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_BUFFERHEADERTYPE * ap_hdr)
{
/*   const struct tizpause *p_obj = ap_obj; */
/*   const OMX_U32 pid = ap_hdr->nInputPortIndex; */
  const void *p_krn = tiz_get_krn (ap_hdl);
/*   const void *p_port = tizkernel_get_port (p_krn, pid); */

  /*   if (TIZPORT_IS_ENABLED(p_port)) */
  /*     { */
  /*       TIZ_LOG(TIZ_LOG_TRACE, "Incorrect state op " */
  /*                 "(ETB received in Pause state on an enabled port)..."); */
  /*       return OMX_ErrorIncorrectStateOperation; */
  /*     } */

  /* Delegate to the kernel... */
  return tizapi_EmptyThisBuffer (p_krn, ap_hdl, ap_hdr);
}

static OMX_ERRORTYPE
pause_FillThisBuffer (const void *ap_obj,
                          OMX_HANDLETYPE ap_hdl,
                          OMX_BUFFERHEADERTYPE * ap_hdr)
{
/*   const struct tizpause *p_obj = ap_obj; */
/*   const OMX_U32 pid = ap_hdr->nOutputPortIndex; */
  const void *p_krn = tiz_get_krn (ap_hdl);
/*   const void *p_port = tizkernel_get_port (p_krn, pid); */

  /*   if (TIZPORT_IS_ENABLED(p_port)) */
  /*     { */
  /*       TIZ_LOG(TIZ_LOG_TRACE, "Incorrect state op " */
  /*                 "(FTB received in Pause state on an enabled port)..."); */
  /*       return OMX_ErrorIncorrectStateOperation; */
  /*     } */


  /* Delegate to the kernel... */
  return tizapi_FillThisBuffer (p_krn, ap_hdl, ap_hdr);
}

static OMX_ERRORTYPE
pause_ComponentDeInit (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  return OMX_ErrorNotImplemented;
}

/*
 * from tizstate
 */

static OMX_ERRORTYPE
pause_state_set (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl,
                     OMX_COMMANDTYPE a_cmd,
                     OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const struct tizpause *p_obj = ap_obj;
  tizfsm_state_id_t new_state = EStateMax;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;

  assert (p_obj);
  assert (a_cmd == OMX_CommandStateSet);

  TIZ_LOG (TIZ_LOG_TRACE, "Requested transition to state [%s]...",
             tiz_fsm_state_to_str (a_param1));

  /* Allowed transitions are OMX_StateIdle, and OMX_StateExecuting */
  switch (a_param1)
    {
    case OMX_StateIdle:
      {
        new_state = ESubStatePauseToIdle;
        break;
      }

    case OMX_StateExecuting:
      {
        new_state = a_param1;
        break;
      }

    case OMX_StatePause:
      {
        return OMX_ErrorSameState;
      }

    default:
      {
        TIZ_LOG (TIZ_LOG_TRACE, "OMX_ErrorIncorrectStateTransition...");
        return OMX_ErrorIncorrectStateTransition;
      }

    };

  /* Move FSM to the transitional state */
  if (ESubStatePauseToIdle == new_state)
    {
      if (OMX_ErrorNone !=
          (ret_val = tizfsm_set_state
           (tiz_get_fsm (ap_hdl), new_state, EStateMax)))
        {
          return ret_val;
        }
    }

  {
    struct tizproc *p_prc = tiz_get_prc (ap_hdl);
    struct tizkernel *p_krn = tiz_get_krn (ap_hdl);

    /* First notify the kernel servant */
    if (OMX_ErrorNone != (ret_val = tizapi_SendCommand (p_krn, ap_hdl,
                                                        a_cmd, a_param1,
                                                        ap_cmd_data)))
      {
        return ret_val;
      }

    /* Now notify the processor servant */
    if (OMX_ErrorNone != (ret_val = tizapi_SendCommand (p_prc, ap_hdl,
                                                        a_cmd, a_param1,
                                                        ap_cmd_data)))
      {
        return ret_val;
      }
  }

  return ret_val;

}

static OMX_ERRORTYPE
pause_state_mark (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                          OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                          OMX_PTR ap_cmd_data)
{
  struct tizkernel *p_krn = tiz_get_krn (ap_hdl);
  /* Notify the kernel servant */
  /* No need to notify the processor servant */
  return tizapi_SendCommand (p_krn, ap_hdl, a_cmd, a_param1,
                             ap_cmd_data);
}

static OMX_ERRORTYPE
pause_trans_complete (const void *ap_obj,
                          OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  TIZ_LOG (TIZ_LOG_TRACE, "Trans complete to state [%s]...",
             tiz_fsm_state_to_str (a_new_state));
  assert (OMX_StatePause == a_new_state || OMX_StateIdle == a_new_state
          || OMX_StateExecuting == a_new_state);
  return tizstate_super_trans_complete (tizpause, ap_obj, ap_servant,
                                        a_new_state);
}

/*
 * initialization
 */

const void *tizpause;

void
init_tizpause (void)
{
  if (!tizpause)
    {
      init_tizstate ();
      tizpause =
        factory_new
        (tizstate_class, "tizpause",
         tizstate, sizeof (struct tizpause),
         ctor, pause_ctor,
         dtor, pause_dtor,
         tizapi_SetParameter, pause_SetParameter,
         tizapi_GetState, pause_GetState,
         tizapi_UseBuffer, pause_UseBuffer,
         tizapi_EmptyThisBuffer, pause_EmptyThisBuffer,
         tizapi_FillThisBuffer, pause_FillThisBuffer,
         tizapi_ComponentDeInit, pause_ComponentDeInit,
         tizstate_state_set, pause_state_set,
         tizstate_mark, pause_state_mark,
         tizstate_trans_complete, pause_trans_complete, 0);
    }
}
