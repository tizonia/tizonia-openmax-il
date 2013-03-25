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
#include "tizutils.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.state"
#endif


static void *
state_ctor (void *ap_obj, va_list * app)
{
  struct tizstate *p_obj = super_ctor (tizstate, ap_obj, app);
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
 * from tizapi class
 */

static OMX_ERRORTYPE
state_SendCommand (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_COMMANDTYPE a_cmd,
                   OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const struct tizstate *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  switch (a_cmd)
    {
    case OMX_CommandStateSet:
      {
        TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                       "SendCommand [%s] [%s]...",
                       tiz_cmd_to_str (a_cmd),
                       tiz_fsm_state_to_str (a_param1));
        rc = tizstate_state_set (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
      }
      break;

    case OMX_CommandFlush:
      {
        TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                       "SendCommand [%s] PORT [%d]...",
                       tiz_cmd_to_str (a_cmd), a_param1);
        rc = tizstate_flush (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
      }
      break;

    case OMX_CommandPortDisable:
      {
        TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                       "SendCommand [%s] PORT [%d]...",
                       tiz_cmd_to_str (a_cmd), a_param1);
        rc = tizstate_disable (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
      }
      break;

    case OMX_CommandPortEnable:
      {
        TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                       "SendCommand [%s] PORT [%d]...",
                       tiz_cmd_to_str (a_cmd), a_param1);
        rc = tizstate_enable (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
      }
      break;

    case OMX_CommandMarkBuffer:
      {
        rc = tizstate_mark (p_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
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
 * from tizstate
 */

static OMX_ERRORTYPE
state_state_set (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  /* NOTE: This is a default implementation, to be overriden as/when needed */
  struct tizstate *p_obj = (struct tizstate *) ap_obj;
  assert (NULL != p_obj);

  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_hdl);

  {
    struct tizproc *p_prc = tiz_get_prc (ap_hdl);
    struct tizkernel *p_krn = tiz_get_krn (ap_hdl);

    /* First notify the kernel servant */
    if (OMX_ErrorNone != (rc = tizapi_SendCommand (p_krn, ap_hdl,
                                                   a_cmd, a_param1,
                                                   ap_cmd_data)))
      {
        return rc;
      }

    /* Now notify the processor servant */
    if (OMX_ErrorNone != (rc = tizapi_SendCommand (p_prc, ap_hdl,
                                                   a_cmd, a_param1,
                                                   ap_cmd_data)))
      {
        return rc;
      }
  }

  return rc;
}

OMX_ERRORTYPE
tizstate_state_set (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_COMMANDTYPE a_cmd,
                    OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const struct tizstate_class *class = classOf (ap_obj);
  assert (class->state_set);
  return class->state_set (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

OMX_ERRORTYPE
tizstate_super_state_set (const void *ap_class, const void *ap_obj,
                          OMX_HANDLETYPE ap_hdl,
                          OMX_COMMANDTYPE a_cmd,
                          OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const struct tizstate_class *superclass = super (ap_class);
  assert (ap_obj && superclass->state_set);
  return superclass->state_set (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_flush (const void *ap_obj,
             OMX_HANDLETYPE ap_hdl,
             OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  struct tizkernel *p_krn = tiz_get_krn (ap_hdl);

  /* Notify the kernel servant, which will in turn notify the processor
   * servant, if needed */
  return tizapi_SendCommand (p_krn, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

OMX_ERRORTYPE
tizstate_flush (const void *ap_obj,
                OMX_HANDLETYPE ap_hdl,
                OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const struct tizstate_class *class = classOf (ap_obj);
  assert (class->flush);
  return class->flush (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_disable (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
               OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  struct tizproc *p_prc = tiz_get_prc (ap_hdl);
  struct tizkernel *p_krn = tiz_get_krn (ap_hdl);

  /* First notify the kernel servant */
  if (OMX_ErrorNone != (rc = tizapi_SendCommand (p_krn, ap_hdl,
                                                 a_cmd, a_param1,
                                                 ap_cmd_data)))
    {
      return rc;
    }

  /* Notify now the processor servant */
  if (OMX_ErrorNone != (rc = tizapi_SendCommand (p_prc, ap_hdl,
                                                 a_cmd, a_param1,
                                                 ap_cmd_data)))
    {
      return rc;
    }

  return OMX_ErrorNone;

}

OMX_ERRORTYPE
tizstate_disable (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const struct tizstate_class *class = classOf (ap_obj);
  assert (class->disable);
  return class->disable (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_enable (const void *ap_obj,
              OMX_HANDLETYPE ap_hdl,
              OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  struct tizproc *p_prc = tiz_get_prc (ap_hdl);
  struct tizkernel *p_krn = tiz_get_krn (ap_hdl);

  /* First notify the kernel servant */
  if (OMX_ErrorNone != (rc = tizapi_SendCommand (p_krn, ap_hdl,
                                                 a_cmd, a_param1,
                                                 ap_cmd_data)))
    {
      return rc;
    }

  /* Now notify the processor servant */
  if (OMX_ErrorNone != (rc = tizapi_SendCommand (p_prc, ap_hdl,
                                                 a_cmd, a_param1,
                                                 ap_cmd_data)))
    {
      return rc;
    }

  return OMX_ErrorNone;

}

OMX_ERRORTYPE
tizstate_enable (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const struct tizstate_class *class = classOf (ap_obj);
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
  struct tizkernel *p_krn = tiz_get_krn (ap_hdl);
  struct tizport *p_port = tizkernel_get_port (p_krn, a_param1);

  /* The port must be disabled at this point */

  assert (TIZPORT_IS_DISABLED (p_port));

  /* Notify the kernel servant */
  /* No need to notify the processor servant */
  return tizapi_SendCommand (p_krn, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

OMX_ERRORTYPE
tizstate_mark (const void *ap_obj,
               OMX_HANDLETYPE ap_hdl,
               OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  const struct tizstate_class *class = classOf (ap_obj);
  assert (class->mark);
  return class->mark (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

static OMX_ERRORTYPE
state_trans_complete (const void *ap_obj,
                      OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  struct tizstate *p_obj = (struct tizstate *) ap_obj;
  assert (NULL != p_obj);

  p_obj->servants_count_++;

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (tizservant_get_hdl (ap_servant)),
                 TIZ_CBUF (tizservant_get_hdl (ap_servant)),
                 "Transition to [%s] is NOW complete at [%s]..."
                 "Servant count is now [%d]...",
                 tiz_fsm_state_to_str (a_new_state), nameOf (ap_servant),
                 p_obj->servants_count_);

  if (2 == p_obj->servants_count_)
    {

      tizfsm_set_state (p_obj->p_fsm_, a_new_state, EStateMax);
    }

  p_obj->servants_count_ = p_obj->servants_count_ % 2;
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizstate_trans_complete (const void *ap_obj,
                         OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  const struct tizstate_class *class = classOf (ap_obj);
  assert (class->trans_complete);
  return class->trans_complete (ap_obj, ap_servant, a_new_state);
}

OMX_ERRORTYPE
tizstate_super_trans_complete (const void *a_class, const void *ap_obj,
                               OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  const struct tizstate_class *superclass = super (a_class);
  assert (ap_obj && superclass->trans_complete);
  return superclass->trans_complete (ap_obj, ap_servant, a_new_state);
}

static OMX_ERRORTYPE
state_tunneled_ports_status_update (void *ap_obj)
{
  struct tizstate *p_obj = ap_obj;
  assert (NULL != p_obj);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizstate_tunneled_ports_status_update (void *ap_obj)
{
  const struct tizstate_class *class = classOf (ap_obj);
  assert (class->tunneled_ports_status_update);
  return class->tunneled_ports_status_update (ap_obj);
}

/*
 * tizstate_class
 */

static void *
state_class_ctor (void *ap_obj, va_list * app)
{
  struct tizstate_class *p_obj = super_ctor (tizstate_class, ap_obj, app);
  typedef void (*voidf) ();
  voidf selector;
  va_list ap;
  va_copy (ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) tizstate_state_set)
        {
          *(voidf *) & p_obj->state_set = method;
        }
      else if (selector == (voidf) tizstate_flush)
        {
          *(voidf *) & p_obj->flush = method;
        }
      else if (selector == (voidf) tizstate_disable)
        {
          *(voidf *) & p_obj->disable = method;
        }
      else if (selector == (voidf) tizstate_enable)
        {
          *(voidf *) & p_obj->enable = method;
        }
      else if (selector == (voidf) tizstate_mark)
        {
          *(voidf *) & p_obj->mark = method;
        }
      else if (selector == (voidf) tizstate_trans_complete)
        {
          *(voidf *) & p_obj->trans_complete = method;
        }
      else if (selector == (voidf) tizstate_tunneled_ports_status_update)
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
init_tizstate (void)
{
  if (!tizstate_class)
    {
      init_tizapi ();
      tizstate_class = factory_new (tizapi_class,
                                    "tizstate_class",
                                    tizapi_class,
                                    sizeof (struct tizstate_class),
                                    ctor, state_class_ctor, 0);

    }

  if (!tizstate)
    {
      init_tizapi ();
      tizstate =
        factory_new
        (tizstate_class, "tizstate",
         tizapi, sizeof (struct tizstate),
         ctor, state_ctor,
         dtor, state_dtor,
         tizapi_SendCommand, state_SendCommand,
         tizapi_SetParameter, state_SetParameter,
         tizapi_SetConfig, state_SetConfig,
         tizapi_GetState, state_GetState,
         tizapi_ComponentTunnelRequest, state_ComponentTunnelRequest,
         tizapi_UseBuffer, state_UseBuffer,
         tizapi_AllocateBuffer, state_AllocateBuffer,
         tizapi_FreeBuffer, state_FreeBuffer,
         tizapi_EmptyThisBuffer, state_EmptyThisBuffer,
         tizapi_FillThisBuffer, state_FillThisBuffer,
         tizapi_SetCallbacks, state_SetCallbacks,
         tizstate_state_set, state_state_set,
         tizstate_flush, state_flush,
         tizstate_disable, state_disable,
         tizstate_enable, state_enable,
         tizstate_mark, state_mark,
         tizstate_trans_complete, state_trans_complete,
         tizstate_tunneled_ports_status_update,
         state_tunneled_ports_status_update, 0);
    }

}

void
init_tizstates (void)
{
  if (!tizstate)
    {
      init_tizstate ();
    }

  if (!tizloaded)
    {
      init_tizloaded ();
    }

  if (!tizloadedtoidle)
    {
      init_tizloadedtoidle ();
    }

  if (!tizwaitforresources)
    {
      init_tizwaitforresources ();
    }

  if (!tizidle)
    {
      init_tizidle ();
    }

  if (!tizidletoloaded)
    {
      init_tizidletoloaded ();
    }

  if (!tizidletoexecuting)
    {
      init_tizidletoexecuting ();
    }

  if (!tizexecuting)
    {
      init_tizexecuting ();
    }

  if (!tizexecutingtoidle)
    {
      init_tizexecutingtoidle ();
    }

  if (!tizpause)
    {
      init_tizpause ();
    }

  if (!tizpausetoidle)
    {
      init_tizpausetoidle ();
    }

}
