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

#include "tizstate.h"
#include "tizstate_decls.h"

#include "tizscheduler.h"
#include "tizkernel.h"
#include "tizport.h"
#include "tizport-macros.h"
#include "tizutils.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.state"
#endif


static void *
state_ctor (void *ap_obj, va_list * app)
{
  tiz_state_t *p_obj = super_ctor (tizstate, ap_obj, app);
  p_obj->p_fsm_ = va_arg (*app, void *);
  p_obj->servants_count_ = 0;
  return p_obj;
}

static void *
state_dtor (void *ap_obj)
{
  return super_dtor (tizstate, ap_obj);
}

/*
 * from tiz_api class
 */

static OMX_ERRORTYPE
state_SendCommand (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_COMMANDTYPE a_cmd,
                   OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  switch (a_cmd)
    {
    case OMX_CommandStateSet:
      {
        TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                       "SendCommand [%s] [%s]...",
                       tiz_cmd_to_str (a_cmd),
                       tiz_fsm_state_to_str (a_param1));
        rc = tiz_state_state_set (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
      }
      break;

    case OMX_CommandFlush:
      {
        TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                       "SendCommand [%s] PORT [%d]...",
                       tiz_cmd_to_str (a_cmd), a_param1);
        rc = tiz_state_flush (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
      }
      break;

    case OMX_CommandPortDisable:
      {
        TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                       "SendCommand [%s] PORT [%d]...",
                       tiz_cmd_to_str (a_cmd), a_param1);
        rc = tiz_state_disable (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
      }
      break;

    case OMX_CommandPortEnable:
      {
        TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                       "SendCommand [%s] PORT [%d]...",
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
state_SetParameter (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                    OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_SetConfig (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                 OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  /* NOTE: Default implementation is a no op */
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
state_GetState (const void *ap_obj,
                OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_ComponentTunnelRequest (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                              OMX_U32 a_port, OMX_HANDLETYPE ap_tunn_comp,
                              OMX_U32 a_tunn_port,
                              OMX_TUNNELSETUPTYPE * ap_tunn_setup)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_UseBuffer (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                 OMX_U32 a_port_index,
                 OMX_PTR ap_app_private, OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_AllocateBuffer (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_BUFFERHEADERTYPE ** pap_buf,
                      OMX_U32 a_port_index,
                      OMX_PTR ap_app_private, OMX_U32 a_size_bytes)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_FreeBuffer (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_U32 a_port_index, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_EmptyThisBuffer (const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_FillThisBuffer (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
state_SetCallbacks (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_CALLBACKTYPE * ap_callbacks, OMX_PTR ap_app_data)
{
  return OMX_ErrorNotImplemented;
}


/*
 * from tiz_state
 */

static OMX_ERRORTYPE
state_state_set (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  /* NOTE: This is a default implementation, to be overriden as/when needed */
  tiz_state_t *p_obj = (tiz_state_t *) ap_obj;
  assert (NULL != p_obj);

  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_hdl);

  {
    void *p_prc = tiz_get_prc (ap_hdl);
    void *p_krn = tiz_get_krn (ap_hdl);

    /* First notify the kernel servant */
    if (OMX_ErrorNone != (rc = tiz_api_SendCommand (p_krn, ap_hdl,
                                                   a_cmd, a_param1,
                                                   ap_cmd_data)))
      {
        return rc;
      }

    /* Now notify the processor servant */
    if (OMX_ErrorNone != (rc = tiz_api_SendCommand (p_prc, ap_hdl,
                                                   a_cmd, a_param1,
                                                   ap_cmd_data)))
      {
        return rc;
      }
  }

  return rc;
}

OMX_ERRORTYPE
tiz_state_state_set (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_COMMANDTYPE a_cmd,
                    OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_class_t *class = classOf (ap_obj);
  assert (class->state_set);
  return class->state_set (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

OMX_ERRORTYPE
tiz_state_super_state_set (const void *ap_class, const void *ap_obj,
                          OMX_HANDLETYPE ap_hdl,
                          OMX_COMMANDTYPE a_cmd,
                          OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_class_t *superclass = super (ap_class);
  assert (ap_obj && superclass->state_set);
  return superclass->state_set (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_flush (const void *ap_obj,
             OMX_HANDLETYPE ap_hdl,
             OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  void *p_krn = tiz_get_krn (ap_hdl);

  /* Notify the kernel servant, which will in turn notify the processor
   * servant, if needed */
  return tiz_api_SendCommand (p_krn, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

OMX_ERRORTYPE
tiz_state_flush (const void *ap_obj,
                OMX_HANDLETYPE ap_hdl,
                OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_class_t *class = classOf (ap_obj);
  assert (class->flush);
  return class->flush (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_disable (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
               OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  void *p_prc = tiz_get_prc (ap_hdl);
  void *p_krn = tiz_get_krn (ap_hdl);

  /* First notify the kernel servant */
  if (OMX_ErrorNone != (rc = tiz_api_SendCommand (p_krn, ap_hdl,
                                                 a_cmd, a_param1,
                                                 ap_cmd_data)))
    {
      return rc;
    }

  /* Notify now the processor servant */
  if (OMX_ErrorNone != (rc = tiz_api_SendCommand (p_prc, ap_hdl,
                                                 a_cmd, a_param1,
                                                 ap_cmd_data)))
    {
      return rc;
    }

  return OMX_ErrorNone;

}

OMX_ERRORTYPE
tiz_state_disable (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_class_t *class = classOf (ap_obj);
  assert (class->disable);
  return class->disable (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_enable (const void *ap_obj,
              OMX_HANDLETYPE ap_hdl,
              OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  void *p_prc = tiz_get_prc (ap_hdl);
  void *p_krn = tiz_get_krn (ap_hdl);

  /* First notify the kernel servant */
  if (OMX_ErrorNone != (rc = tiz_api_SendCommand (p_krn, ap_hdl,
                                                 a_cmd, a_param1,
                                                 ap_cmd_data)))
    {
      return rc;
    }

  /* Now notify the processor servant */
  if (OMX_ErrorNone != (rc = tiz_api_SendCommand (p_prc, ap_hdl,
                                                 a_cmd, a_param1,
                                                 ap_cmd_data)))
    {
      return rc;
    }

  return OMX_ErrorNone;

}

OMX_ERRORTYPE
tiz_state_enable (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_class_t *class = classOf (ap_obj);
  assert (class->enable);
  return class->enable (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_mark (const void *ap_obj,
            OMX_HANDLETYPE ap_hdl,
            OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  /* This is the default implementation for states in which this command is not
   * allowed */
  assert (NULL != ap_hdl);
  void *p_krn = tiz_get_krn (ap_hdl);

  /* The port must be disabled at this point */
  assert (TIZ_PORT_IS_DISABLED (tiz_kernel_get_port (p_krn, a_param1)));

  /* Notify the kernel servant */
  /* No need to notify the processor servant */
  return tiz_api_SendCommand (p_krn, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

OMX_ERRORTYPE
tiz_state_mark (const void *ap_obj,
               OMX_HANDLETYPE ap_hdl,
               OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const tiz_state_class_t *class = classOf (ap_obj);
  assert (class->mark);
  return class->mark (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_trans_complete (const void *ap_obj,
                      OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  tiz_state_t *p_obj = (tiz_state_t *) ap_obj;
  assert (NULL != p_obj);

  p_obj->servants_count_++;

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (tiz_servant_get_hdl (ap_servant)),
                 TIZ_CBUF (tiz_servant_get_hdl (ap_servant)),
                 "Transition to [%s] is NOW complete at [%s]..."
                 "Servant count is now [%d]...",
                 tiz_fsm_state_to_str (a_new_state), nameOf (ap_servant),
                 p_obj->servants_count_);

  if (2 == p_obj->servants_count_)
    {

      tiz_fsm_set_state (p_obj->p_fsm_, a_new_state, EStateMax);
    }

  p_obj->servants_count_ = p_obj->servants_count_ % 2;
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_state_trans_complete (const void *ap_obj,
                         OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  const tiz_state_class_t *class = classOf (ap_obj);
  assert (class->trans_complete);
  return class->trans_complete (ap_obj, ap_servant, a_new_state);
}

OMX_ERRORTYPE
tiz_state_super_trans_complete (const void *a_class, const void *ap_obj,
                               OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  const tiz_state_class_t *superclass = super (a_class);
  assert (ap_obj && superclass->trans_complete);
  return superclass->trans_complete (ap_obj, ap_servant, a_new_state);
}

static OMX_ERRORTYPE
state_tunneled_ports_status_update (void *ap_obj)
{
  tiz_state_t *p_obj = ap_obj;
  assert (NULL != p_obj);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_state_tunneled_ports_status_update (void *ap_obj)
{
  const tiz_state_class_t *class = classOf (ap_obj);
  assert (class->tunneled_ports_status_update);
  return class->tunneled_ports_status_update (ap_obj);
}

/*
 * tizstate_class
 */

static void *
state_class_ctor (void *ap_obj, va_list * app)
{
  tiz_state_class_t *p_obj = super_ctor (tizstate_class, ap_obj, app);
  typedef void (*voidf) ();
  voidf selector;
  va_list ap;
  va_copy (ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) tiz_state_state_set)
        {
          *(voidf *) & p_obj->state_set = method;
        }
      else if (selector == (voidf) tiz_state_flush)
        {
          *(voidf *) & p_obj->flush = method;
        }
      else if (selector == (voidf) tiz_state_disable)
        {
          *(voidf *) & p_obj->disable = method;
        }
      else if (selector == (voidf) tiz_state_enable)
        {
          *(voidf *) & p_obj->enable = method;
        }
      else if (selector == (voidf) tiz_state_mark)
        {
          *(voidf *) & p_obj->mark = method;
        }
      else if (selector == (voidf) tiz_state_trans_complete)
        {
          *(voidf *) & p_obj->trans_complete = method;
        }
      else if (selector == (voidf) tiz_state_tunneled_ports_status_update)
        {
          *(voidf *) & p_obj->tunneled_ports_status_update = method;
        }

    }

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

const void *tizstate, *tizstate_class;


void
tiz_state_init (void)
{
  if (!tizstate_class)
    {
      tiz_api_init ();
      tizstate_class = factory_new (tizapi_class,
                                    "tizstate_class",
                                    tizapi_class,
                                    sizeof (tiz_state_class_t),
                                    ctor, state_class_ctor, 0);

    }

  if (!tizstate)
    {
      tiz_api_init ();
      tizstate =
        factory_new
        (tizstate_class, "tizstate",
         tizapi, sizeof (tiz_state_t),
         ctor, state_ctor,
         dtor, state_dtor,
         tiz_api_SendCommand, state_SendCommand,
         tiz_api_SetParameter, state_SetParameter,
         tiz_api_SetConfig, state_SetConfig,
         tiz_api_GetState, state_GetState,
         tiz_api_ComponentTunnelRequest, state_ComponentTunnelRequest,
         tiz_api_UseBuffer, state_UseBuffer,
         tiz_api_AllocateBuffer, state_AllocateBuffer,
         tiz_api_FreeBuffer, state_FreeBuffer,
         tiz_api_EmptyThisBuffer, state_EmptyThisBuffer,
         tiz_api_FillThisBuffer, state_FillThisBuffer,
         tiz_api_SetCallbacks, state_SetCallbacks,
         tiz_state_state_set, state_state_set,
         tiz_state_flush, state_flush,
         tiz_state_disable, state_disable,
         tiz_state_enable, state_enable,
         tiz_state_mark, state_mark,
         tiz_state_trans_complete, state_trans_complete,
         tiz_state_tunneled_ports_status_update,
         state_tunneled_ports_status_update, 0);
    }

}

void
tiz_state_init_states (void)
{
  if (!tizstate)
    {
      tiz_state_init ();
    }

  if (!tizloaded)
    {
      tiz_loaded_init ();
    }

  if (!tizloadedtoidle)
    {
      tiz_loadedtoidle_init ();
    }

  if (!tizwaitforresources)
    {
      tiz_waitforresources_init ();
    }

  if (!tizidle)
    {
      tiz_idle_init ();
    }

  if (!tizidletoloaded)
    {
      tiz_idletoloaded_init ();
    }

  if (!tizidletoexecuting)
    {
      tiz_idletoexecuting_init ();
    }

  if (!tizexecuting)
    {
      tiz_executing_init ();
    }

  if (!tizexecutingtoidle)
    {
      tiz_executingtoidle_init ();
    }

  if (!tizpause)
    {
      tiz_pause_init ();
    }

  if (!tizpausetoidle)
    {
      tiz_pausetoidle_init ();
    }

}
