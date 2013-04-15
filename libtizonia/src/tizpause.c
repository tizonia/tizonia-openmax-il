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
#include "tizport-macros.h"
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
  tiz_pause_t *p_obj = super_ctor (tizpause, ap_obj, app);
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
          tiz_kernel_find_managing_port (p_krn, a_index, a_struct, &p_port)))
    {
      TIZ_LOG (TIZ_TRACE, "Cannot retrieve managing port (%s)...",
               tiz_err_to_str (ret_val));
      return ret_val;
    }

  assert (p_port);

  if (TIZ_PORT_IS_CONFIG_PORT (p_port)
      || (!TIZ_PORT_IS_CONFIG_PORT (p_port) && TIZ_PORT_IS_ENABLED (p_port)))
    {
      TIZ_LOG (TIZ_TRACE, "Incorrect state op "
               "(SetParameter received in Pause state)...");
      return OMX_ErrorIncorrectStateOperation;
    }

  return tiz_api_SetParameter (p_krn, ap_hdl, a_index, a_struct);

}

static OMX_ERRORTYPE
pause_GetState (const void *ap_obj,
                OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  *ap_state = OMX_StatePause;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
pause_UseBuffer (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                 OMX_U32 a_port_index,
                 OMX_PTR ap_app_private, OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
pause_EmptyThisBuffer (const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_hdr)
{
/*   const tiz_pause_t *p_obj = ap_obj; */
/*   const OMX_U32 pid = ap_hdr->nInputPortIndex; */
  const void *p_krn = tiz_get_krn (ap_hdl);
/*   const void *p_port = tiz_kernel_get_port (p_krn, pid); */

  /*   if (TIZ_PORT_IS_ENABLED(p_port)) */
  /*     { */
  /*       TIZ_LOG(TIZ_TRACE, "Incorrect state op " */
  /*                 "(ETB received in Pause state on an enabled port)..."); */
  /*       return OMX_ErrorIncorrectStateOperation; */
  /*     } */

  /* Delegate to the kernel... */
  return tiz_api_EmptyThisBuffer (p_krn, ap_hdl, ap_hdr);
}

static OMX_ERRORTYPE
pause_FillThisBuffer (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_hdr)
{
/*   const tiz_pause_t *p_obj = ap_obj; */
/*   const OMX_U32 pid = ap_hdr->nOutputPortIndex; */
  const void *p_krn = tiz_get_krn (ap_hdl);
/*   const void *p_port = tiz_kernel_get_port (p_krn, pid); */

  /*   if (TIZ_PORT_IS_ENABLED(p_port)) */
  /*     { */
  /*       TIZ_LOG(TIZ_TRACE, "Incorrect state op " */
  /*                 "(FTB received in Pause state on an enabled port)..."); */
  /*       return OMX_ErrorIncorrectStateOperation; */
  /*     } */


  /* Delegate to the kernel... */
  return tiz_api_FillThisBuffer (p_krn, ap_hdl, ap_hdr);
}

/*
 * from tiz_state
 */

static OMX_ERRORTYPE
pause_state_set (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_pause_t *p_obj = ap_obj;
  tiz_fsm_state_id_t new_state = EStateMax;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;

  assert (p_obj);
  assert (a_cmd == OMX_CommandStateSet);

  TIZ_LOG (TIZ_TRACE, "Requested transition to state [%s]...",
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
        TIZ_LOG (TIZ_TRACE, "OMX_ErrorIncorrectStateTransition...");
        return OMX_ErrorIncorrectStateTransition;
      }

    };

  /* Move FSM to the transitional state */
  if (ESubStatePauseToIdle == new_state)
    {
      if (OMX_ErrorNone !=
          (ret_val = tiz_fsm_set_state
           (tiz_get_fsm (ap_hdl), new_state, EStateMax)))
        {
          return ret_val;
        }
    }

  {
    void *p_prc = tiz_get_prc (ap_hdl);
    void *p_krn = tiz_get_krn (ap_hdl);

    /* First notify the kernel servant */
    if (OMX_ErrorNone != (ret_val = tiz_api_SendCommand (p_krn, ap_hdl,
                                                        a_cmd, a_param1,
                                                        ap_cmd_data)))
      {
        return ret_val;
      }

    /* Now notify the processor servant */
    if (OMX_ErrorNone != (ret_val = tiz_api_SendCommand (p_prc, ap_hdl,
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
                  OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  void *p_krn = tiz_get_krn (ap_hdl);
  /* Notify the kernel servant */
  /* No need to notify the processor servant */
  return tiz_api_SendCommand (p_krn, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
pause_trans_complete (const void *ap_obj,
                      OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (tiz_servant_get_hdl (ap_servant)),
                 TIZ_CBUF (tiz_servant_get_hdl (ap_servant)),
                 "Trans complete to state [%s]...",
                 tiz_fsm_state_to_str (a_new_state));
  assert (OMX_StatePause == a_new_state || OMX_StateIdle == a_new_state
          || OMX_StateExecuting == a_new_state);
  return tiz_state_super_trans_complete (tizpause, ap_obj, ap_servant,
                                        a_new_state);
}

/*
 * initialization
 */

const void *tizpause;

void
tiz_pause_init (void)
{
  if (!tizpause)
    {
      tiz_state_init ();
      tizpause =
        factory_new
        (tizstate_class, "tizpause",
         tizstate, sizeof (tiz_pause_t),
         ctor, pause_ctor,
         dtor, pause_dtor,
         tiz_api_SetParameter, pause_SetParameter,
         tiz_api_GetState, pause_GetState,
         tiz_api_UseBuffer, pause_UseBuffer,
         tiz_api_EmptyThisBuffer, pause_EmptyThisBuffer,
         tiz_api_FillThisBuffer, pause_FillThisBuffer,
         tiz_state_state_set, pause_state_set,
         tiz_state_mark, pause_state_mark,
         tiz_state_trans_complete, pause_trans_complete, 0);
    }
}
