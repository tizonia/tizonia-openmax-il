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
 * @file   tizidle.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Idle OMX IL state implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizscheduler.h"
#include "tizfsm.h"
#include "tizkernel.h"
#include "tizport.h"
#include "tizidle.h"
#include "tizstate_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.idle"
#endif


static void *
idle_ctor (void *ap_obj, va_list * app)
{
  struct tizidle *p_obj = super_ctor (tizidle, ap_obj, app);
  return p_obj;
}

static void *
idle_dtor (void *ap_obj)
{
  return super_dtor (tizidle, ap_obj);
}

static OMX_ERRORTYPE
idle_SetParameter (const void *ap_obj,
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
          tiz_kernel_find_managing_port (p_krn, a_index, a_struct, &p_port)))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
               "Cannot retrieve managing port (%s)...",
               tiz_err_to_str (ret_val));
      return ret_val;
    }

  assert (p_port);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "SetParameter : ENABLED [%d]",
                 TIZ_PORT_IS_ENABLED (p_port) ? 1 : 0);

  if (TIZ_PORT_IS_CONFIG_PORT (p_port)
      || (!TIZ_PORT_IS_CONFIG_PORT (p_port) && TIZ_PORT_IS_ENABLED (p_port)))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "Incorrect state op "
                     "(SetParameter received in Idle state)...");
      return OMX_ErrorIncorrectStateOperation;
    }

  return tiz_api_SetParameter (p_krn, ap_hdl, a_index, a_struct);

}

static OMX_ERRORTYPE
idle_GetState (const void *ap_obj,
               OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  assert (NULL != ap_state);
  *ap_state = OMX_StateIdle;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
idle_UseBuffer (const void *ap_obj,
                OMX_HANDLETYPE ap_hdl,
                OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                OMX_U32 a_port_index,
                OMX_PTR ap_app_private, OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idle_EmptyThisBuffer (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  OMX_U32 pid = 0;
  const void *p_krn = NULL;
  const void *p_port = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_hdr);

  pid = ap_hdr->nInputPortIndex;
  p_krn = tiz_get_krn (ap_hdl);
  p_port = tiz_kernel_get_port (p_krn, pid);

  /* TODO: Review whether this check is needed here or not */

  if (TIZ_PORT_IS_ENABLED (p_port))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorIncorrectStateOperation] : "
                     "(ETB received in Idle state on an enabled port)...");
      return OMX_ErrorIncorrectStateOperation;
    }

  return tiz_api_EmptyThisBuffer (p_krn, ap_hdl, ap_hdr);
}

static OMX_ERRORTYPE
idle_FillThisBuffer (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_hdr)
{

  /* TODO: Review whether this check is needed here or not */

/*   const struct tizidle *p_obj = ap_obj; */
/*   const OMX_U32 pid = ap_hdr->nOutputPortIndex; */
  const void *p_krn = tiz_get_krn (ap_hdl);
/*   const void *p_port = tiz_kernel_get_port (p_krn, pid); */

/*   if (TIZ_PORT_IS_ENABLED(p_port)) */
/*     { */
/*       TIZ_LOG(TIZ_TRACE, "Incorrect state op " */
/*                 "(FTB received in Idle state on an enabled port)..."); */
/*       return OMX_ErrorIncorrectStateOperation; */
/*     } */

  /* Delegate to the kernel... */
  return tiz_api_FillThisBuffer (p_krn, ap_hdl, ap_hdr);
}

/*
 * from tizstate
 */

static OMX_ERRORTYPE
idle_state_set (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  tiz_fsm_state_id_t new_state = EStateMax;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  struct tizkernel *p_krn = NULL;
  tiz_kernel_tunneled_ports_status_t status = ETIZKernelTunneledPortsMax;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (a_cmd == OMX_CommandStateSet);

  TIZ_LOG_CNAME (TIZ_DEBUG, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "Requested transition to state [%s]...",
                 tiz_fsm_state_to_str (a_param1));

  p_krn = tiz_get_krn (ap_hdl);
  status = tiz_kernel_get_tunneled_ports_status (p_krn, OMX_FALSE);

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "kernel's tunneled port status [%d] ", status);

  /* Allowed transitions are OMX_StateLoaded, OMX_StateExecuting */
  /* and OMX_StatePause. */
  switch (a_param1)
    {
    case OMX_StateLoaded:
      {
        new_state = ESubStateIdleToLoaded;
      }
      break;

    case OMX_StateExecuting:
      {
        if (ETIZKernelNoTunneledPorts == status)
          {
            /* Transition directly to OMX_StateExecuting */
            new_state = a_param1;
          }
        else
          {
            /* Transition to the transitional state */
            new_state = ESubStateIdleToExecuting;
          }
      }
      break;

    case OMX_StatePause:
      {
        new_state = a_param1;
      }
      break;

    case OMX_StateIdle:
      {
        return OMX_ErrorSameState;
      }

    default:
      {
        TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                       "[OMX_ErrorIncorrectStateTransition] : "
                       "(OMX_StateLoaded -> [%s]...)",
                       tiz_state_to_str (a_param1));
        return OMX_ErrorIncorrectStateTransition;
      }

    };

  if (ESubStateIdleToLoaded == new_state
      || ESubStateIdleToExecuting == new_state)
    {
      if (OMX_ErrorNone !=
          (rc = tiz_fsm_set_state
           (tiz_get_fsm (ap_hdl), new_state, EStateMax)))
        {
          return rc;
        }
    }

  if (ESubStateIdleToExecuting == new_state)
    {
      if (ETIZKernelTunneledPortsAcceptNone == status)
      {
        TIZ_LOG_CNAME (TIZ_DEBUG, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                       "wait until all the tunneled non-supplier neighbours have "
                       "reported that they are ready to exchange buffers ...");
        return rc;
      }
    }

  return tizstate_super_state_set (tizidle, ap_obj, ap_hdl, a_cmd,
                                   a_param1, ap_cmd_data);
}

/*
 * initialization
 */

const void *tizidle;

void
init_tizidle (void)
{
  if (!tizidle)
    {
      init_tizstate ();
      tizidle =
        factory_new
        (tizstate_class, "tizidle",
         tizstate, sizeof (struct tizidle),
         ctor, idle_ctor,
         dtor, idle_dtor,
         tiz_api_SetParameter, idle_SetParameter,
         tiz_api_GetState, idle_GetState,
         tiz_api_UseBuffer, idle_UseBuffer,
         tiz_api_EmptyThisBuffer, idle_EmptyThisBuffer,
         tiz_api_FillThisBuffer, idle_FillThisBuffer,
         tizstate_state_set, idle_state_set, 0);
    }
}
