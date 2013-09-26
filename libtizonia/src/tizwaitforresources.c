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
 * @file   tizwaitforresources.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - WaitForResources OMX IL state implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizfsm.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizwaitforresources.h"
#include "tizstate_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.waitforresources"
#endif


static void *
waitforresources_ctor (void *ap_obj, va_list * app)
{
  tiz_waitforresources_t *p_obj =
    super_ctor (tizwaitforresources, ap_obj, app);
  return p_obj;
}

static void *
waitforresources_dtor (void *ap_obj)
{
  return super_dtor (tizwaitforresources, ap_obj);
}

static OMX_ERRORTYPE
waitforresources_SetParameter (const void *ap_obj,
                               OMX_HANDLETYPE ap_hdl,
                               OMX_INDEXTYPE a_index, OMX_PTR a_struct)
{
  const void *p_krn = tiz_get_krn (ap_hdl);

  return tiz_api_SetParameter (p_krn, ap_hdl, a_index, a_struct);
}

static OMX_ERRORTYPE
waitforresources_GetState (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  *ap_state = OMX_StateWaitForResources;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
waitforresources_UseBuffer (const void *ap_obj,
                            OMX_HANDLETYPE ap_hdl,
                            OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                            OMX_U32 a_port_index,
                            OMX_PTR ap_app_private,
                            OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
waitforresources_EmptyThisBuffer (const void *ap_obj,
                                  OMX_HANDLETYPE ap_hdl,
                                  OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
waitforresources_FillThisBuffer (const void *ap_obj,
                                 OMX_HANDLETYPE ap_hdl,
                                 OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

/*
 * from tiz_state
 */

static OMX_ERRORTYPE
waitforresources_state_set (const void *ap_obj,
                            OMX_HANDLETYPE ap_hdl,
                            OMX_COMMANDTYPE a_cmd,
                            OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_waitforresources_t *p_obj = ap_obj;
  tiz_fsm_state_id_t new_state = EStateMax;
  OMX_ERRORTYPE omx_error = OMX_ErrorNone;

  assert (p_obj);
  assert (a_cmd == OMX_CommandStateSet);

  TIZ_LOGN (TIZ_PRIORITY_TRACE, ap_hdl, "Requested transition to state [%s]...",
            tiz_fsm_state_to_str (a_param1));

  /* Only allowed transitions is OMX_StateLoaded. */
  switch (a_param1)
    {
    case OMX_StateLoaded:
      {
        new_state = OMX_StateLoaded;
        break;
      }

    case OMX_StateWaitForResources:
      {
        return OMX_ErrorSameState;
      }

    default:
      {
        TIZ_LOGN (TIZ_PRIORITY_ERROR, ap_hdl, "[OMX_ErrorIncorrectStateTransition] : ...");
        return OMX_ErrorIncorrectStateTransition;
      }

    };

  /* TODO:  make state transition effective here? */
  (void) new_state;
  /*   if (OMX_ErrorNone != */
  /*       (omx_error = tiz_fsm_set_state */
  /*        (tiz_get_fsm (ap_hdl), new_state))) */
  /*     { */
  /*       return omx_error; */
  /*     } */

  {
    void *p_prc = tiz_get_prc (ap_hdl);
    void *p_krn = tiz_get_krn (ap_hdl);

    /* First notify the kernel servant */
    if (OMX_ErrorNone != (omx_error = tiz_api_SendCommand (p_krn, ap_hdl,
                                                          a_cmd, a_param1,
                                                          ap_cmd_data)))
      {
        return omx_error;
      }

    /* Now notify the processor servant */
    if (OMX_ErrorNone != (omx_error = tiz_api_SendCommand (p_prc, ap_hdl,
                                                          a_cmd, a_param1,
                                                          ap_cmd_data)))
      {
        return omx_error;
      }

  }

  return omx_error;

}

static OMX_ERRORTYPE
waitforresources_trans_complete (const void *ap_obj,
                                 OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  TIZ_LOGN (TIZ_PRIORITY_TRACE, tiz_api_get_hdl (ap_servant),
                 "Trans complete to state [%s]...",
                 tiz_fsm_state_to_str (a_new_state));
  assert (OMX_StateWaitForResources == a_new_state
          || OMX_StateLoaded == a_new_state);
  return tiz_state_super_trans_complete (tizwaitforresources, ap_obj,
                                        ap_servant, a_new_state);
}

/*
 * initialization
 */

const void *tizwaitforresources;

OMX_ERRORTYPE
tiz_waitforresources_init (void)
{
  if (!tizwaitforresources)
    {
      tiz_check_omx_err_ret_oom (tiz_state_init ());
      tiz_check_null_ret_oom
        (tizwaitforresources =
         factory_new
         (tizstate_class, "tizwaitforresources",
          tizstate, sizeof (tiz_waitforresources_t),
          ctor, waitforresources_ctor,
          dtor, waitforresources_dtor,
          tiz_api_SetParameter, waitforresources_SetParameter,
          tiz_api_GetState, waitforresources_GetState,
          tiz_api_UseBuffer, waitforresources_UseBuffer,
          tiz_api_EmptyThisBuffer, waitforresources_EmptyThisBuffer,
          tiz_api_FillThisBuffer, waitforresources_FillThisBuffer,
          tiz_state_state_set, waitforresources_state_set,
          tiz_state_trans_complete, waitforresources_trans_complete, 0));
    }
  return OMX_ErrorNone;
}
