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

#include <assert.h>

#include "tizfsm.h"
#include "tizport.h"
#include "tizscheduler.h"
#include "tizkernel.h"
#include "tizexecuting.h"
#include "tizstate_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.executing"
#endif


static void *
executing_ctor (void *ap_obj, va_list * app)
{
  struct tizexecuting *p_obj = super_ctor (tizexecuting, ap_obj, app);
  return p_obj;
}

static void *
executing_dtor (void *ap_obj)
{
  return super_dtor (tizexecuting, ap_obj);
}

static OMX_ERRORTYPE
executing_SetParameter (const void *ap_obj,
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
               "(SetParameter received in Executing state)...");
      return OMX_ErrorIncorrectStateOperation;
    }

  return tizapi_SetParameter (p_krn, ap_hdl, a_index, a_struct);
}

static OMX_ERRORTYPE
executing_GetState (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "executing_GetState");
  *ap_state = OMX_StateExecuting;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
executing_UseBuffer (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl,
                     OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                     OMX_U32 a_port_index,
                     OMX_PTR ap_app_private,
                     OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
executing_EmptyThisBuffer (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_BUFFERHEADERTYPE * ap_hdr)
{
/*   const struct tizexecuting *p_obj = ap_obj; */
/*   const OMX_U32 pid = ap_hdr->nInputPortIndex; */
  const void *p_krn = tiz_get_krn (ap_hdl);
/*   const void *p_port = tizkernel_get_port (p_krn, pid); */

/*   if (TIZPORT_IS_ENABLED(p_port)) */
/*     { */
/*       TIZ_LOG(TIZ_LOG_TRACE, "Incorrect state op " */
/*              "(ETB received in Executing state on an enabled port)..."); */
/*       return OMX_ErrorIncorrectStateOperation; */
/*     } */

  /* Delegate to the kernel... */
  return tizapi_EmptyThisBuffer (p_krn, ap_hdl, ap_hdr);
}

static OMX_ERRORTYPE
executing_FillThisBuffer (const void *ap_obj,
                          OMX_HANDLETYPE ap_hdl,
                          OMX_BUFFERHEADERTYPE * ap_hdr)
{
/*   const struct tizexecuting *p_obj = ap_obj; */
/*   const OMX_U32 pid = ap_hdr->nOutputPortIndex; */
  const void *p_krn = tiz_get_krn (ap_hdl);
/*   const void *p_port = tizkernel_get_port (p_krn, pid); */

/*   if (TIZPORT_IS_ENABLED(p_port)) */
/*     { */
/*       TIZ_LOG(TIZ_LOG_TRACE, "Incorrect state op " */
/*               "(FTB received in Executing state on an enabled port)..."); */
/*       return OMX_ErrorIncorrectStateOperation; */
/*     } */

  /* Delegate to the kernel... */
  return tizapi_FillThisBuffer (p_krn, ap_hdl, ap_hdr);
}

static OMX_ERRORTYPE
executing_ComponentDeInit (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  return OMX_ErrorNotImplemented;
}

/*
 * from tizstate
 */

static OMX_ERRORTYPE
executing_state_set (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl,
                     OMX_COMMANDTYPE a_cmd,
                     OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const struct tizexecuting *p_obj = ap_obj;
  tizfsm_state_id_t new_state = EStateMax;
  OMX_ERRORTYPE omx_error = OMX_ErrorNone;

  assert (p_obj);
  assert (a_cmd == OMX_CommandStateSet);

  TIZ_LOG (TIZ_LOG_TRACE, "Requested transition to state [%s]...",
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
        new_state = OMX_StatePause;
        break;
      }

    default:
      {
        TIZ_LOG (TIZ_LOG_TRACE, "OMX_ErrorIncorrectStateTransition...");
        return OMX_ErrorIncorrectStateTransition;
      }

    };

  if (ESubStateExecutingToIdle == new_state)
    {
      if (OMX_ErrorNone !=
          (omx_error = tizfsm_set_state
           (tiz_get_fsm (ap_hdl), new_state, EStateMax)))
        {
          return omx_error;
        }
    }

  {
    /* IL transfer and processing of buffers take place now */
    struct tizproc *p_prc = tiz_get_prc (ap_hdl);
    struct tizkernel *p_krn = tiz_get_krn (ap_hdl);

    /* First notify the kernel servant */
    if (OMX_ErrorNone != (omx_error = tizapi_SendCommand (p_krn, ap_hdl,
                                                          a_cmd, a_param1,
                                                          ap_cmd_data)))
      {
        return omx_error;
      }

    /* Now notify the processor servant */
    if (OMX_ErrorNone != (omx_error = tizapi_SendCommand (p_prc, ap_hdl,
                                                          a_cmd, a_param1,
                                                          ap_cmd_data)))
      {
        return omx_error;
      }
  }

  return omx_error;

}

static OMX_ERRORTYPE
executing_state_mark (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                      OMX_PTR ap_cmd_data)
{
  struct tizkernel *p_krn = tiz_get_krn (ap_hdl);
  /* Notify the kernel servant */
  /* No need to notify the processor servant */
  return tizapi_SendCommand (p_krn, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
executing_trans_complete (const void *ap_obj,
                          OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  TIZ_LOG (TIZ_LOG_TRACE, "Trans complete to state [%s]...",
           tiz_fsm_state_to_str (a_new_state));
  assert (OMX_StateExecuting == a_new_state || OMX_StatePause == a_new_state
          || OMX_StateIdle == a_new_state);
  return tizstate_super_trans_complete (tizexecuting, ap_obj, ap_servant,
                                        a_new_state);
}

/*
 * initialization
 */

const void *tizexecuting;

void
init_tizexecuting (void)
{
  if (!tizexecuting)
    {
      init_tizstate ();
      tizexecuting =
        factory_new
        (tizstate_class, "tizexecuting",
         tizstate, sizeof (struct tizexecuting),
         ctor, executing_ctor,
         dtor, executing_dtor,
         tizapi_SetParameter, executing_SetParameter,
         tizapi_GetState, executing_GetState,
         tizapi_UseBuffer, executing_UseBuffer,
         tizapi_EmptyThisBuffer, executing_EmptyThisBuffer,
         tizapi_FillThisBuffer, executing_FillThisBuffer,
         tizapi_ComponentDeInit, executing_ComponentDeInit,
         tizstate_state_set, executing_state_set,
         tizstate_mark, executing_state_mark,
         tizstate_trans_complete, executing_trans_complete, 0);
    }
}
