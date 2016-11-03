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
 * @file   tizstate.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - FSM's state base class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizplatform.h>

#include "tizscheduler.h"
#include "tizkernel.h"
#include "tizport.h"
#include "tizport-macros.h"
#include "tizutils.h"

#include "tizstate.h"
#include "tizstate_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.state"
#endif

static void *
state_ctor (void * ap_obj, va_list * app)
{
  tiz_state_t * p_obj = super_ctor (typeOf (ap_obj, "tizstate"), ap_obj, app);
  p_obj->p_fsm_ = va_arg (*app, void *);
  p_obj->servants_count_ = 0;
  return p_obj;
}

static void *
state_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tizstate"), ap_obj);
}

/*
 * from tiz_api class
 */

static OMX_ERRORTYPE
state_SendCommand (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                   OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  switch (a_cmd)
    {
      case OMX_CommandStateSet:
        {
          TIZ_TRACE (ap_hdl, "SendCommand [%s] [%s]...", tiz_cmd_to_str (a_cmd),
                     tiz_fsm_state_to_str (a_param1));
          rc
            = tiz_state_state_set (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
        }
        break;

      case OMX_CommandFlush:
        {
          TIZ_TRACE (ap_hdl, "SendCommand [%s] PORT [%d]...",
                     tiz_cmd_to_str (a_cmd), a_param1);
          rc = tiz_state_flush (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
        }
        break;

      case OMX_CommandPortDisable:
        {
          TIZ_TRACE (ap_hdl, "SendCommand [%s] PORT [%d]...",
                     tiz_cmd_to_str (a_cmd), a_param1);
          rc = tiz_state_disable (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
        }
        break;

      case OMX_CommandPortEnable:
        {
          TIZ_TRACE (ap_hdl, "SendCommand [%s] PORT [%d]...",
                     tiz_cmd_to_str (a_cmd), a_param1);
          rc = tiz_state_enable (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
        }
        break;

      case OMX_CommandMarkBuffer:
        {
          rc = tiz_state_mark (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
        }
        break;

      default:
        {
          assert (0);
        }
    };

  return rc;
}

static OMX_ERRORTYPE
state_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                    OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_SetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                 OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  /* NOTE: Default implementation is a no op */
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
state_GetState (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                OMX_STATETYPE * ap_state)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_ComponentTunnelRequest (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                              OMX_U32 a_port, OMX_HANDLETYPE ap_tunn_comp,
                              OMX_U32 a_tunn_port,
                              OMX_TUNNELSETUPTYPE * ap_tunn_setup)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_UseBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                 OMX_BUFFERHEADERTYPE ** app_buf_hdr, OMX_U32 a_port_index,
                 OMX_PTR ap_app_private, OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_AllocateBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_BUFFERHEADERTYPE ** pap_buf, OMX_U32 a_port_index,
                      OMX_PTR ap_app_private, OMX_U32 a_size_bytes)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_FreeBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                  OMX_U32 a_port_index, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_EmptyThisBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                       OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_FillThisBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_SetCallbacks (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                    OMX_CALLBACKTYPE * ap_callbacks, OMX_PTR ap_app_data)
{
  return OMX_ErrorNotImplemented;
}

/*
 * from tiz_state
 */

static OMX_ERRORTYPE
state_state_set (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                 OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  /* NOTE: This is a default implementation, to be overriden as/when needed */

  /* First notify the kernel servant */
  tiz_check_omx (tiz_api_SendCommand (tiz_get_krn (ap_hdl), ap_hdl, a_cmd,
                                      a_param1, ap_cmd_data));
  /* Now notify the processor servant */
  tiz_check_omx (tiz_api_SendCommand (tiz_get_prc (ap_hdl), ap_hdl, a_cmd,
                                      a_param1, ap_cmd_data));
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_state_state_set (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                     OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                     OMX_PTR ap_cmd_data)
{
  const tiz_state_class_t * class = classOf (ap_obj);
  assert (class->state_set);
  return class->state_set (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

OMX_ERRORTYPE
tiz_state_super_state_set (const void * ap_class, const void * ap_obj,
                           OMX_HANDLETYPE ap_hdl, OMX_COMMANDTYPE a_cmd,
                           OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_class_t * superclass = super (ap_class);
  assert (ap_obj && superclass->state_set);
  return superclass->state_set (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_flush (const void * ap_obj, OMX_HANDLETYPE ap_hdl, OMX_COMMANDTYPE a_cmd,
             OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  /* Notify the kernel servant, which will in turn notify the processor
   * servant, if needed */
  return tiz_api_SendCommand (tiz_get_krn (ap_hdl), ap_hdl, a_cmd, a_param1,
                              ap_cmd_data);
}

OMX_ERRORTYPE
tiz_state_flush (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                 OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_class_t * class = classOf (ap_obj);
  assert (class->flush);
  return class->flush (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_disable (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
               OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  /* First notify the kernel servant */
  tiz_check_omx (tiz_api_SendCommand (tiz_get_krn (ap_hdl), ap_hdl, a_cmd,
                                      a_param1, ap_cmd_data));
  /* Now notify the processor servant */
  tiz_check_omx (tiz_api_SendCommand (tiz_get_prc (ap_hdl), ap_hdl, a_cmd,
                                      a_param1, ap_cmd_data));
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_state_disable (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                   OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_class_t * class = classOf (ap_obj);
  assert (class->disable);
  return class->disable (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_enable (const void * ap_obj, OMX_HANDLETYPE ap_hdl, OMX_COMMANDTYPE a_cmd,
              OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  /* Notify the kernel servant, which will in turn notify the processor
   * servant, if needed */
  return tiz_api_SendCommand (tiz_get_krn (ap_hdl), ap_hdl, a_cmd, a_param1,
                              ap_cmd_data);
}

OMX_ERRORTYPE
tiz_state_enable (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                  OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_class_t * class = classOf (ap_obj);
  assert (class->enable);
  return class->enable (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_mark (const void * ap_obj, OMX_HANDLETYPE ap_hdl, OMX_COMMANDTYPE a_cmd,
            OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  /* This is the default implementation for states in which this command is not
   * allowed */
  void * p_krn = tiz_get_krn (ap_hdl);

  /* The port must be disabled at this point */
  assert (TIZ_PORT_IS_DISABLED (tiz_krn_get_port (p_krn, a_param1)));

  /* Notify the kernel servant */
  /* No need to notify the processor servant */
  return tiz_api_SendCommand (p_krn, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

OMX_ERRORTYPE
tiz_state_mark (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_class_t * class = classOf (ap_obj);
  assert (class->mark);
  return class->mark (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_trans_complete (const void * ap_obj, OMX_PTR ap_servant,
                      OMX_STATETYPE a_new_state)
{
  tiz_state_t * p_obj = (tiz_state_t *) ap_obj;
  assert (p_obj);

  p_obj->servants_count_++;

  TIZ_TRACE (handleOf (ap_obj),
             "Transition to [%s] is NOW complete at [%s]..."
             "Servant count is now [%d]...",
             tiz_fsm_state_to_str ((tiz_fsm_state_id_t) a_new_state),
             nameOf (ap_servant), p_obj->servants_count_);

  /* Check that the two servats are complete */
  if (2 == p_obj->servants_count_)
    {

      if (OMX_StateExecuting == a_new_state)
        {
          /* Reset the OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE flag in all ports where this
           * has been set */
          tiz_krn_reset_tunneled_ports_status (
            tiz_get_krn (handleOf (ap_obj)),
            OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE);
        }
      tiz_fsm_set_state (p_obj->p_fsm_, (tiz_fsm_state_id_t) a_new_state,
                         EStateMax);
    }

  p_obj->servants_count_ = p_obj->servants_count_ % 2;
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_state_trans_complete (const void * ap_obj, OMX_PTR ap_servant,
                          OMX_STATETYPE a_new_state)
{
  const tiz_state_class_t * class = classOf (ap_obj);
  assert (class->trans_complete);
  return class->trans_complete (ap_obj, ap_servant, a_new_state);
}

OMX_ERRORTYPE
tiz_state_super_trans_complete (const void * a_class, const void * ap_obj,
                                OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  const tiz_state_class_t * superclass = super (a_class);
  assert (ap_obj && superclass->trans_complete);
  return superclass->trans_complete (ap_obj, ap_servant, a_new_state);
}

static OMX_ERRORTYPE
state_tunneled_ports_status_update (void * ap_obj)
{
  tiz_state_t * p_obj = ap_obj;
  assert (p_obj);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_state_tunneled_ports_status_update (void * ap_obj)
{
  const tiz_state_class_t * class = classOf (ap_obj);
  assert (class->tunneled_ports_status_update);
  return class->tunneled_ports_status_update (ap_obj);
}

/*
 * tizstate_class
 */

static void *
state_class_ctor (void * ap_obj, va_list * app)
{
  tiz_state_class_t * p_obj
    = super_ctor (typeOf (ap_obj, "tizstate_class"), ap_obj, app);
  typedef void (*voidf) ();
  voidf selector = NULL;
  va_list ap;
  va_copy (ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) tiz_state_state_set)
        {
          *(voidf *) &p_obj->state_set = method;
        }
      else if (selector == (voidf) tiz_state_flush)
        {
          *(voidf *) &p_obj->flush = method;
        }
      else if (selector == (voidf) tiz_state_disable)
        {
          *(voidf *) &p_obj->disable = method;
        }
      else if (selector == (voidf) tiz_state_enable)
        {
          *(voidf *) &p_obj->enable = method;
        }
      else if (selector == (voidf) tiz_state_mark)
        {
          *(voidf *) &p_obj->mark = method;
        }
      else if (selector == (voidf) tiz_state_trans_complete)
        {
          *(voidf *) &p_obj->trans_complete = method;
        }
      else if (selector == (voidf) tiz_state_tunneled_ports_status_update)
        {
          *(voidf *) &p_obj->tunneled_ports_status_update = method;
        }
    }

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

void *
tiz_state_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizapi = tiz_get_type (ap_hdl, "tizapi");
  void * tizstate_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizapi), "tizstate_class", classOf (tizapi),
     sizeof (tiz_state_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, state_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizstate_class;
}

void *
tiz_state_init (void * ap_tos, void * ap_hdl)
{
  void * tizapi = tiz_get_type (ap_hdl, "tizapi");
  void * tizstate_class = tiz_get_type (ap_hdl, "tizstate_class");
  TIZ_LOG_CLASS (tizstate_class);
  void * tizstate = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizstate_class, "tizstate", tizapi, sizeof (tiz_state_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, state_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, state_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SendCommand, state_SendCommand,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, state_SetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetConfig, state_SetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetState, state_GetState,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_ComponentTunnelRequest, state_ComponentTunnelRequest,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_UseBuffer, state_UseBuffer,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_AllocateBuffer, state_AllocateBuffer,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_FreeBuffer, state_FreeBuffer,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_EmptyThisBuffer, state_EmptyThisBuffer,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_FillThisBuffer, state_FillThisBuffer,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetCallbacks, state_SetCallbacks,
     /* TIZ_CLASS_COMMENT: */
     tiz_state_state_set, state_state_set,
     /* TIZ_CLASS_COMMENT: */
     tiz_state_flush, state_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_state_disable, state_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_state_enable, state_enable,
     /* TIZ_CLASS_COMMENT: */
     tiz_state_mark, state_mark,
     /* TIZ_CLASS_COMMENT: */
     tiz_state_trans_complete, state_trans_complete,
     /* TIZ_CLASS_COMMENT: */
     tiz_state_tunneled_ports_status_update,
     /* TIZ_CLASS_COMMENT: */
     state_tunneled_ports_status_update,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizstate;
}
