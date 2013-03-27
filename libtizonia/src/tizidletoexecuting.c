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
  struct tizidletoexecuting *p_obj =
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
  const struct tizstate *p_base = (const struct tizstate *) ap_obj;

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (tiz_servant_get_hdl (ap_servant)),
                 TIZ_CBUF (tiz_servant_get_hdl (ap_servant)),
                 "Trans complete to state [%s]...",
                 tiz_fsm_state_to_str (a_new_state));

  assert (NULL != ap_obj);
  assert (NULL != ap_servant);
  assert (OMX_StateExecuting == a_new_state);

  if (2 == p_base->servants_count_ + 1)
    {
      /* Reset the OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE flag in all ports where this
       * has been set */
      tiz_kernel_reset_tunneled_ports_status (tiz_get_krn
                                             (tiz_servant_get_hdl (ap_servant)),
                                             OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE);
    }

  return tizstate_super_trans_complete (tizidletoexecuting, ap_obj, ap_servant,
                                        a_new_state);
}

static OMX_ERRORTYPE
idletoexecuting_tunneled_ports_status_update (void *ap_obj)
{
  struct tizstate *p_base = (struct tizstate *) ap_obj;

  assert (NULL != ap_obj);

  {
    OMX_HANDLETYPE p_hdl = tiz_servant_get_hdl (p_base->p_fsm_);
    struct tizkernel *p_krn = tiz_get_krn (p_hdl);
    tiz_kernel_tunneled_ports_status_t status =
      tiz_kernel_get_tunneled_ports_status (p_krn, OMX_FALSE);

    TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (p_hdl), TIZ_CBUF (p_hdl),
                   "kernel's tunneled port status [%d] ", status);

    if (ETIZKernelNoTunneledPorts == status
        || ETIZKernelTunneledPortsAcceptBufferExchange == status
        || ETIZKernelTunneledPortsAcceptBoth == status)
      {
        /* OK, at this point all the tunneled non-supplier neighboring ports
         * are ready to receive ETB/FTB calls.  NOTE: This will call the
         * 'tizstate_state_set' function (we are passing 'tizidle' as the 1st
         * parameter  */
        return tizstate_super_state_set (tizidle, ap_obj, p_hdl,
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
init_tizidletoexecuting (void)
{
  if (!tizidletoexecuting)
    {
      init_tizidle ();
      tizidletoexecuting =
        factory_new
        (tizstate_class, "tizidletoexecuting",
         tizidle, sizeof (struct tizidletoexecuting),
         ctor, idletoexecuting_ctor,
         dtor, idletoexecuting_dtor,
         tizapi_SetParameter, idletoexecuting_SetParameter,
         tizapi_GetState, idletoexecuting_GetState,
         tizapi_UseBuffer, idletoexecuting_UseBuffer,
         tizapi_AllocateBuffer, idletoexecuting_AllocateBuffer,
         tizapi_FreeBuffer, idletoexecuting_FreeBuffer,
         tizapi_EmptyThisBuffer, idletoexecuting_EmptyThisBuffer,
         tizapi_FillThisBuffer, idletoexecuting_FillThisBuffer,
         tizstate_trans_complete, idletoexecuting_trans_complete,
         tizstate_tunneled_ports_status_update,
         idletoexecuting_tunneled_ports_status_update, 0);
    }
}
