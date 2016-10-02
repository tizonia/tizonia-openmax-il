/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   tizloaded.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Loaded OMX IL state implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizloaded.h"
#include "tizstate.h"
#include "tizstate_decls.h"
#include "tizfsm.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizport.h"
#include "tizport-macros.h"

#include "tizplatform.h"

#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.loaded"
#endif

static void *
loaded_ctor (void *ap_obj, va_list * app)
{
  tiz_loaded_t *p_obj = super_ctor (typeOf (ap_obj, "tizloaded"), ap_obj, app);
  return p_obj;
}

static void *
loaded_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tizloaded"), ap_obj);
}

static OMX_ERRORTYPE
loaded_SetParameter (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl,
                     OMX_INDEXTYPE a_index, OMX_PTR a_struct)
{
  const void *p_krn = tiz_get_krn (ap_hdl);
  return tiz_api_SetParameter (p_krn, ap_hdl, a_index, a_struct);
}

static OMX_ERRORTYPE
loaded_GetState (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  *ap_state = OMX_StateLoaded;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
loaded_UseBuffer (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                  OMX_U32 a_port_index,
                  OMX_PTR ap_app_private,
                  OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
loaded_EmptyThisBuffer (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const OMX_U32 pid = ap_hdr->nInputPortIndex;
  const void *p_krn = tiz_get_krn (ap_hdl);
  const void *p_port = tiz_krn_get_port (p_krn, pid);

  if (TIZ_PORT_IS_ENABLED (p_port))
    {
      TIZ_ERROR (ap_hdl,
                     "[OMX_ErrorIncorrectStateOperation] : "
                     "(ETB received in Loaded state on an enabled port)...");
      return OMX_ErrorIncorrectStateOperation;
    }


  /* Delegate to the kernel... */
  return tiz_api_EmptyThisBuffer (p_krn, ap_hdl, ap_hdr);
}

static OMX_ERRORTYPE
loaded_FillThisBuffer (const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const OMX_U32 pid = ap_hdr->nOutputPortIndex;
  const void *p_krn = tiz_get_krn (ap_hdl);
  const void *p_port = tiz_krn_get_port (p_krn, pid);

  if (TIZ_PORT_IS_ENABLED (p_port))
    {
      TIZ_ERROR (ap_hdl, "[OMX_ErrorIncorrectStateOperation] : "
                "(FTB received in Loaded state on an enabled port)...");
      return OMX_ErrorIncorrectStateOperation;
    }


  /* Delegate to the kernel... */
  return tiz_api_FillThisBuffer (p_krn, ap_hdl, ap_hdr);
}


/*
 * from tiz_state
 */

static OMX_ERRORTYPE
loaded_state_set (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                  OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  tiz_fsm_state_id_t new_state = EStateMax;

  assert (ap_obj);
  assert (ap_hdl);
  assert (a_cmd == OMX_CommandStateSet);

  TIZ_DEBUG (ap_hdl, "Requested transition "
            "[OMX_StateLoaded -> %s]...", tiz_fsm_state_to_str (a_param1));

  /* Allowed transitions are OMX_StateIdle and OMX_StateWaitForResources. */
  switch (a_param1)
    {
    case OMX_StateIdle:
      {
        new_state = ESubStateLoadedToIdle;
        break;
      }

    case OMX_StateWaitForResources:
      {
        new_state = (tiz_fsm_state_id_t)OMX_StateWaitForResources;
        break;
      }

    case OMX_StateLoaded:
      {
        return OMX_ErrorSameState;
      }

    default:
      {
        TIZ_ERROR (ap_hdl, "[OMX_ErrorIncorrectStateTransition]...: "
                  "(OMX_StateLoaded -> [%s]...)", tiz_state_to_str (a_param1));
        return OMX_ErrorIncorrectStateTransition;
      }

    };

  if (ESubStateLoadedToIdle == new_state)
    {
      OMX_ERRORTYPE rc = OMX_ErrorNone;

      if (OMX_ErrorNone !=
          (rc = tiz_fsm_set_state
           (tiz_get_fsm (ap_hdl), new_state, EStateMax)))
        {
          return rc;
        }

      {
        if (!TIZ_KRN_MAY_INIT_ALLOC_PHASE (tiz_get_krn (ap_hdl)))
          {
            TIZ_DEBUG (ap_hdl,
                      "wait until all the tunneled non-supplier "
                      "neighbours have reported that they can accept "
                      "OMX_UseBuffer calls...");
            return rc;
          }
      }

    }

  /* IL resource allocation takes place now */
  return tiz_state_super_state_set (typeOf (ap_obj, "tizloaded"), ap_obj, ap_hdl, a_cmd,
                                   a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
loaded_trans_complete (const void *ap_obj,
                       OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  TIZ_DEBUG (handleOf (ap_servant), "Trans complete to state [%s]...",
             tiz_fsm_state_to_str ((tiz_fsm_state_id_t)a_new_state));
  assert (OMX_StateWaitForResources == a_new_state
          || OMX_StateIdle == a_new_state);
  return tiz_state_super_trans_complete (typeOf (ap_obj, "tizloaded"), ap_obj, ap_servant,
                                        a_new_state);
}

/*
 * loaded_class
 */

static void *
loaded_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizloaded_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_loaded_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizstate = tiz_get_type (ap_hdl, "tizstate");
  void * tizloaded_class = factory_new (classOf (tizstate),
                                        "tizloaded_class",
                                        classOf (tizstate),
                                        sizeof (tiz_loaded_class_t),
                                        ap_tos, ap_hdl,
                                        ctor, loaded_class_ctor, 0);
  return tizloaded_class;
}

void *
tiz_loaded_init (void * ap_tos, void * ap_hdl)
{
  void * tizstate = tiz_get_type (ap_hdl, "tizstate");
  void * tizloaded_class = tiz_get_type (ap_hdl, "tizloaded_class");
  TIZ_LOG_CLASS (tizloaded_class);
  void * tizloaded =
    factory_new
    (tizloaded_class,
     "tizloaded",
     tizstate,
     sizeof (tiz_loaded_t),
     ap_tos, ap_hdl,
     ctor, loaded_ctor,
     dtor, loaded_dtor,
     tiz_api_SetParameter, loaded_SetParameter,
     tiz_api_GetState, loaded_GetState,
     tiz_api_UseBuffer, loaded_UseBuffer,
     tiz_api_EmptyThisBuffer, loaded_EmptyThisBuffer,
     tiz_api_FillThisBuffer, loaded_FillThisBuffer,
     tiz_state_state_set, loaded_state_set,
     tiz_state_trans_complete, loaded_trans_complete, 0);

  return tizloaded;
}
