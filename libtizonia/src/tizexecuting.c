/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   tizexecuting.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Executing OMX IL state implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizexecuting.h"
#include "tizstate.h"
#include "tizstate_decls.h"
#include "tizfsm.h"
#include "tizport.h"
#include "tizport-macros.h"
#include "tizscheduler.h"
#include "tizkernel.h"

#include "tizplatform.h"

#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.executing"
#endif

static void *
executing_ctor (void * ap_obj, va_list * app)
{
  tiz_executing_t * p_obj
    = super_ctor (typeOf (ap_obj, "tizexecuting"), ap_obj, app);
  return p_obj;
}

static void *
executing_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tizexecuting"), ap_obj);
}

static OMX_ERRORTYPE
executing_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR a_struct)
{
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;
  const void * p_krn = NULL;
  OMX_PTR p_port = NULL;

  assert (ap_obj);

  p_krn = tiz_get_krn (ap_hdl);

  /* TODO: Optimization: find_managing_port is called twice, first time here,
   * then in the SetParameter implementation of the kernel object. */

  if (OMX_ErrorNone != (ret_val = tiz_krn_find_managing_port (
                          p_krn, a_index, a_struct, &p_port)))
    {
      TIZ_ERROR (ap_hdl,
                 "[%s] : "
                 "(Unable to retrieve managing port for index %d...)",
                 tiz_err_to_str (ret_val), tiz_idx_to_str (a_index));
      return ret_val;
    }

  assert (p_port);

  if (TIZ_PORT_IS_CONFIG_PORT (p_port)
      || (!TIZ_PORT_IS_CONFIG_PORT (p_port) && TIZ_PORT_IS_ENABLED (p_port)))
    {
      TIZ_ERROR (ap_hdl,
                 "[OMX_ErrorIncorrectStateOperation] : "
                 "(SetParameter received in Executing state)...");
      return OMX_ErrorIncorrectStateOperation;
    }

  return tiz_api_SetParameter (p_krn, ap_hdl, a_index, a_struct);
}

static OMX_ERRORTYPE
executing_GetState (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                    OMX_STATETYPE * ap_state)
{
  assert (ap_state);
  *ap_state = OMX_StateExecuting;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
executing_UseBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                     OMX_BUFFERHEADERTYPE ** app_buf_hdr, OMX_U32 a_port_index,
                     OMX_PTR ap_app_private, OMX_U32 a_size_bytes,
                     OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
executing_EmptyThisBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                           OMX_BUFFERHEADERTYPE * ap_hdr)
{

  /* TODO: Review whether this check is needed here or not */

  /*   const tiz_executing_t *p_obj = ap_obj; */
  /*   const OMX_U32 pid = ap_hdr->nInputPortIndex; */
  const void * p_krn = tiz_get_krn (ap_hdl);
  /*   const void *p_port = tiz_krn_get_port (p_krn, pid); */

  /*   if (TIZ_PORT_IS_ENABLED(p_port)) */
  /*     { */
  /*       TIZ_LOG(TIZ_PRIORITY_TRACE, "Incorrect state op " */
  /*              "(ETB received in Executing state on an enabled port)..."); */
  /*       return OMX_ErrorIncorrectStateOperation; */
  /*     } */

  /* Delegate to the kernel... */
  return tiz_api_EmptyThisBuffer (p_krn, ap_hdl, ap_hdr);
}

static OMX_ERRORTYPE
executing_FillThisBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                          OMX_BUFFERHEADERTYPE * ap_hdr)
{

  /* TODO: Review whether this check is needed here or not */

  /*   const tiz_executing_t *p_obj = ap_obj; */
  /*   const OMX_U32 pid = ap_hdr->nOutputPortIndex; */
  const void * p_krn = tiz_get_krn (ap_hdl);
  /*   const void *p_port = tiz_krn_get_port (p_krn, pid); */

  /*   if (TIZ_PORT_IS_ENABLED(p_port)) */
  /*     { */
  /*       TIZ_LOG(TIZ_PRIORITY_TRACE, "Incorrect state op " */
  /*               "(FTB received in Executing state on an enabled port)..."); */
  /*       return OMX_ErrorIncorrectStateOperation; */
  /*     } */

  /* Delegate to the kernel... */
  return tiz_api_FillThisBuffer (p_krn, ap_hdl, ap_hdr);
}

/*
 * from tiz_state
 */

static OMX_ERRORTYPE
executing_state_set (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                     OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                     OMX_PTR ap_cmd_data)
{
  tiz_fsm_state_id_t new_state = EStateMax;

  assert (ap_obj);
  assert (ap_hdl);
  assert (a_cmd == OMX_CommandStateSet);

  TIZ_DEBUG (ap_hdl, "Requested transition to state [%s]...",
             tiz_fsm_state_to_str (a_param1));

  /* Allowed transitions are OMX_StateIdle and OMX_StateExecuting. */
  switch (a_param1)
    {
      case OMX_StateIdle:
        {
          new_state = ESubStateExecutingToIdle;
          break;
        }

      case OMX_StateExecuting:
        {
          return OMX_ErrorSameState;
        }

      case OMX_StatePause:
        {
          new_state = (tiz_fsm_state_id_t) OMX_StatePause;
          break;
        }

      default:
        {
          TIZ_ERROR (ap_hdl, "[OMX_ErrorIncorrectStateTransition]");
          return OMX_ErrorIncorrectStateTransition;
        }
    };

  if (ESubStateExecutingToIdle == new_state)
    {
      OMX_ERRORTYPE rc = OMX_ErrorNone;
      if (OMX_ErrorNone != (rc = tiz_fsm_set_state (tiz_get_fsm (ap_hdl),
                                                    new_state, EStateMax)))
        {
          return rc;
        }

      {
        if (!TIZ_KRN_MAY_INIT_EXE_TO_IDLE (tiz_get_krn (ap_hdl)))
          {
            TIZ_DEBUG (
              ap_hdl,
              "wait until all the tunneled supplier neighbours have "
              "reported that they have stopped the buffer exchange...");
            return rc;
          }
      }
    }

  return tiz_state_super_state_set (typeOf (ap_obj, "tizexecuting"), ap_obj,
                                    ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
executing_state_mark (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                      OMX_PTR ap_cmd_data)
{
  void * p_krn = tiz_get_krn (ap_hdl);
  /* Notify the kernel servant */
  /* No need to notify the processor servant */
  return tiz_api_SendCommand (p_krn, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
executing_trans_complete (const void * ap_obj, OMX_PTR ap_servant,
                          OMX_STATETYPE a_new_state)
{
  assert (ap_obj);
  assert (ap_servant);

  TIZ_DEBUG (handleOf (ap_servant), "Trans complete to state [%s]...",
             tiz_fsm_state_to_str ((tiz_fsm_state_id_t) a_new_state));
  assert (OMX_StateExecuting == a_new_state || OMX_StatePause == a_new_state
          || OMX_StateIdle == a_new_state);
  return tiz_state_super_trans_complete (typeOf (ap_obj, "tizexecuting"),
                                         ap_obj, ap_servant, a_new_state);
}

/*
 * executing_class
 */

static void *
executing_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizexecuting_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_executing_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizstate = tiz_get_type (ap_hdl, "tizstate");
  void * tizexecuting_class
    = factory_new (classOf (tizstate), "tizexecuting_class", classOf (tizstate),
                   sizeof (tiz_executing_class_t), ap_tos, ap_hdl, ctor,
                   executing_class_ctor, 0);
  return tizexecuting_class;
}

void *
tiz_executing_init (void * ap_tos, void * ap_hdl)
{
  void * tizstate = tiz_get_type (ap_hdl, "tizstate");
  void * tizexecuting_class = tiz_get_type (ap_hdl, "tizexecuting_class");
  TIZ_LOG_CLASS (tizexecuting_class);
  void * tizexecuting = factory_new (
    tizexecuting_class, "tizexecuting", tizstate, sizeof (tiz_executing_t),
    ap_tos, ap_hdl, ctor, executing_ctor, dtor, executing_dtor,
    tiz_api_SetParameter, executing_SetParameter, tiz_api_GetState,
    executing_GetState, tiz_api_UseBuffer, executing_UseBuffer,
    tiz_api_EmptyThisBuffer, executing_EmptyThisBuffer, tiz_api_FillThisBuffer,
    executing_FillThisBuffer, tiz_state_state_set, executing_state_set,
    tiz_state_mark, executing_state_mark, tiz_state_trans_complete,
    executing_trans_complete, 0);

  return tizexecuting;
}
