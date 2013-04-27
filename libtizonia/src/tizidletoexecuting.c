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
 * @file   tizidletoexecuting.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Idletoexecuting OMX IL substate implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizidletoexecuting.h"
#include "tizstate_decls.h"
#include "tizutils.h"
#include "tizosal.h"
#include "tizkernel.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.idletoexecuting"
#endif


static void *
idletoexecuting_ctor (void *ap_obj, va_list * app)
{
  tiz_idletoexecuting_t *p_obj =
    super_ctor (tizidletoexecuting, ap_obj, app);
  return p_obj;
}

static void *
idletoexecuting_dtor (void *ap_obj)
{
  return super_dtor (tizidletoexecuting, ap_obj);
}

/*
 * from tizapi class
 */

static OMX_ERRORTYPE
idletoexecuting_SetParameter (const void *ap_obj,
                              OMX_HANDLETYPE ap_hdl,
                              OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoexecuting_GetState (const void *ap_obj,
                          OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  assert (NULL != ap_state);
  *ap_state = OMX_StateIdle;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
idletoexecuting_UseBuffer (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                           OMX_U32 a_port_index,
                           OMX_PTR ap_app_private,
                           OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoexecuting_AllocateBuffer (const void *ap_obj,
                                OMX_HANDLETYPE ap_hdl,
                                OMX_BUFFERHEADERTYPE ** pap_buf,
                                OMX_U32 a_port_index,
                                OMX_PTR ap_app_private, OMX_U32 a_size_bytes)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoexecuting_FreeBuffer (const void *ap_obj,
                            OMX_HANDLETYPE ap_hdl,
                            OMX_U32 a_port_index,
                            OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoexecuting_EmptyThisBuffer (const void *ap_obj,
                                 OMX_HANDLETYPE ap_hdl,
                                 OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoexecuting_FillThisBuffer (const void *ap_obj,
                                OMX_HANDLETYPE ap_hdl,
                                OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

/*
 * from tizstate class
 */

static OMX_ERRORTYPE
idletoexecuting_trans_complete (const void *ap_obj,
                                OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  const tiz_state_t *p_base = (const tiz_state_t *) ap_obj;

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (tiz_srv_get_hdl (ap_servant)),
                 TIZ_CBUF (tiz_srv_get_hdl (ap_servant)),
                 "Trans complete to state [%s]...",
                 tiz_fsm_state_to_str (a_new_state));

  assert (NULL != ap_obj);
  assert (NULL != ap_servant);
  assert (OMX_StateExecuting == a_new_state);

  if (2 == p_base->servants_count_ + 1)
    {
      /* Reset the OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE flag in all ports where this
       * has been set */
      tiz_krn_reset_tunneled_ports_status (tiz_get_krn
                                             (tiz_srv_get_hdl (ap_servant)),
                                             OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE);
    }

  return tiz_state_super_trans_complete (tizidletoexecuting, ap_obj, ap_servant,
                                        a_new_state);
}

static OMX_ERRORTYPE
idletoexecuting_tunneled_ports_status_update (void *ap_obj)
{
  tiz_state_t *p_base = (tiz_state_t *) ap_obj;

  assert (NULL != ap_obj);

  {
    OMX_HANDLETYPE p_hdl = tiz_srv_get_hdl (p_base->p_fsm_);
    void *p_krn = tiz_get_krn (p_hdl);
    tiz_krn_tunneled_ports_status_t status =
      tiz_krn_get_tunneled_ports_status (p_krn, OMX_FALSE);

    TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_hdl), TIZ_CBUF (p_hdl),
                   "kernel's tunneled port status [%d] ", status);

    if (ETIZKrnNoTunneledPorts == status
        || ETIZKrnTunneledPortsAcceptBufferExchange == status
        || ETIZKrnTunneledPortsAcceptBoth == status)
      {
        /* OK, at this point all the tunneled non-supplier neighboring ports
         * are ready to receive ETB/FTB calls.  NOTE: This will call the
         * 'tiz_state_state_set' function (we are passing 'tizidle' as the 1st
         * parameter  */
        return tiz_state_super_state_set (tizidle, ap_obj, p_hdl,
                                         OMX_CommandStateSet,
                                         OMX_StateExecuting, NULL);
      }
  }

  return OMX_ErrorNone;
}


/*
 * initialization
 */

const void *tizidletoexecuting;

void
tiz_idletoexecuting_init (void)
{
  if (!tizidletoexecuting)
    {
      tiz_idle_init ();
      tizidletoexecuting =
        factory_new
        (tizstate_class, "tizidletoexecuting",
         tizidle, sizeof (tiz_idletoexecuting_t),
         ctor, idletoexecuting_ctor,
         dtor, idletoexecuting_dtor,
         tiz_api_SetParameter, idletoexecuting_SetParameter,
         tiz_api_GetState, idletoexecuting_GetState,
         tiz_api_UseBuffer, idletoexecuting_UseBuffer,
         tiz_api_AllocateBuffer, idletoexecuting_AllocateBuffer,
         tiz_api_FreeBuffer, idletoexecuting_FreeBuffer,
         tiz_api_EmptyThisBuffer, idletoexecuting_EmptyThisBuffer,
         tiz_api_FillThisBuffer, idletoexecuting_FillThisBuffer,
         tiz_state_trans_complete, idletoexecuting_trans_complete,
         tiz_state_tunneled_ports_status_update,
         idletoexecuting_tunneled_ports_status_update, 0);
    }
}
