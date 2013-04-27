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
#include "tizport-macros.h"
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
  tiz_executing_t *p_obj = super_ctor (tizexecuting, ap_obj, app);
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
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;
  const void *p_krn = NULL;
  OMX_PTR p_port = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  
  p_krn = tiz_get_krn (ap_hdl);

  /* TODO: Optimization: find_managing_port is called twice, first time here,
   * then in the SetParameter implementation of the kernel object. */

  if (OMX_ErrorNone
      != (ret_val =
          tiz_krn_find_managing_port (p_krn, a_index, a_struct, &p_port)))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] : (Unable to retrieve managing port for index %d...)",
                     tiz_err_to_str (ret_val), tiz_idx_to_str (a_index));
      return ret_val;
    }

  assert (NULL != p_port);

  if (TIZ_PORT_IS_CONFIG_PORT (p_port)
      || (!TIZ_PORT_IS_CONFIG_PORT (p_port) && TIZ_PORT_IS_ENABLED (p_port)))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorIncorrectStateOperation] : "
                     "(SetParameter received in Executing state)...");
      return OMX_ErrorIncorrectStateOperation;
    }

  return tiz_api_SetParameter (p_krn, ap_hdl, a_index, a_struct);
}

static OMX_ERRORTYPE
executing_GetState (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  assert (NULL != ap_state);
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

  /* TODO: Review whether this check is needed here or not */

/*   const tiz_executing_t *p_obj = ap_obj; */
/*   const OMX_U32 pid = ap_hdr->nInputPortIndex; */
  const void *p_krn = tiz_get_krn (ap_hdl);
/*   const void *p_port = tiz_krn_get_port (p_krn, pid); */

/*   if (TIZ_PORT_IS_ENABLED(p_port)) */
/*     { */
/*       TIZ_LOG(TIZ_TRACE, "Incorrect state op " */
/*              "(ETB received in Executing state on an enabled port)..."); */
/*       return OMX_ErrorIncorrectStateOperation; */
/*     } */

  /* Delegate to the kernel... */
  return tiz_api_EmptyThisBuffer (p_krn, ap_hdl, ap_hdr);
}

static OMX_ERRORTYPE
executing_FillThisBuffer (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                          OMX_BUFFERHEADERTYPE * ap_hdr)
{

  /* TODO: Review whether this check is needed here or not */


/*   const tiz_executing_t *p_obj = ap_obj; */
/*   const OMX_U32 pid = ap_hdr->nOutputPortIndex; */
  const void *p_krn = tiz_get_krn (ap_hdl);
/*   const void *p_port = tiz_krn_get_port (p_krn, pid); */

/*   if (TIZ_PORT_IS_ENABLED(p_port)) */
/*     { */
/*       TIZ_LOG(TIZ_TRACE, "Incorrect state op " */
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
executing_state_set (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                     OMX_COMMANDTYPE a_cmd,
                     OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  tiz_fsm_state_id_t new_state = EStateMax;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (a_cmd == OMX_CommandStateSet);

  TIZ_LOG_CNAME (TIZ_DEBUG, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "Requested transition to state [%s]...",
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
        TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                       "[OMX_ErrorIncorrectStateTransition]");
        return OMX_ErrorIncorrectStateTransition;
      }

    };

  if (ESubStateExecutingToIdle == new_state)
    {
      if (OMX_ErrorNone !=
          (rc = tiz_fsm_set_state
           (tiz_get_fsm (ap_hdl), new_state, EStateMax)))
        {
          return rc;
        }

      {
        void *p_krn = tiz_get_krn (ap_hdl);
        tiz_krn_tunneled_ports_status_t status =
          tiz_krn_get_tunneled_ports_status (p_krn, OMX_TRUE);

        if (ETIZKernelTunneledPortsAcceptNone == status)
          {
            TIZ_LOG_CNAME (TIZ_DEBUG, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                           "wait until all the tunneled supplier neighbours have "
                           "reported that they have stopped the buffer exchange...");
            return rc;
          }
      }
    }

  return tiz_state_super_state_set (tizexecuting, ap_obj, ap_hdl, a_cmd,
                                   a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
executing_state_mark (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                      OMX_PTR ap_cmd_data)
{
  void *p_krn = tiz_get_krn (ap_hdl);
  /* Notify the kernel servant */
  /* No need to notify the processor servant */
  return tiz_api_SendCommand (p_krn, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
executing_trans_complete (const void *ap_obj,
                          OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  assert (NULL != ap_obj);
  assert (NULL != ap_servant);

  TIZ_LOG_CNAME (TIZ_DEBUG, TIZ_CNAME (tiz_servant_get_hdl(ap_servant)),
                 TIZ_CBUF (tiz_servant_get_hdl(ap_servant)),
                 "Trans complete to state [%s]...",
                 tiz_fsm_state_to_str (a_new_state));
  assert (OMX_StateExecuting == a_new_state || OMX_StatePause == a_new_state
          || OMX_StateIdle == a_new_state);
  return tiz_state_super_trans_complete (tizexecuting, ap_obj, ap_servant,
                                        a_new_state);
}

/*
 * initialization
 */

const void *tizexecuting;

void
tiz_executing_init (void)
{
  if (!tizexecuting)
    {
      tiz_state_init ();
      tizexecuting =
        factory_new
        (tizstate_class, "tizexecuting",
         tizstate, sizeof (tiz_executing_t),
         ctor, executing_ctor,
         dtor, executing_dtor,
         tiz_api_SetParameter, executing_SetParameter,
         tiz_api_GetState, executing_GetState,
         tiz_api_UseBuffer, executing_UseBuffer,
         tiz_api_EmptyThisBuffer, executing_EmptyThisBuffer,
         tiz_api_FillThisBuffer, executing_FillThisBuffer,
         tiz_state_state_set, executing_state_set,
         tiz_state_mark, executing_state_mark,
         tiz_state_trans_complete, executing_trans_complete, 0);
    }
}
