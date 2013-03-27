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
 * @file   tizidletoloaded.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - IdleToLoaded OMX IL substate implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizidletoloaded.h"
#include "tizstate_decls.h"
#include "tizutils.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.idletoloaded"
#endif


static void *
idletoloaded_ctor (void *ap_obj, va_list * app)
{
  struct tizidletoloaded *p_obj = super_ctor (tizidletoloaded, ap_obj, app);
  return p_obj;
}

static void *
idletoloaded_dtor (void *ap_obj)
{
  return super_dtor (tizidletoloaded, ap_obj);
}

/*
 * from tizapi class
 */

static OMX_ERRORTYPE
idletoloaded_SetParameter (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoloaded_GetState (const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  *ap_state = OMX_StateIdle;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
idletoloaded_UseBuffer (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                        OMX_U32 a_port_index,
                        OMX_PTR ap_app_private,
                        OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoloaded_AllocateBuffer (const void *ap_obj,
                             OMX_HANDLETYPE ap_hdl,
                             OMX_BUFFERHEADERTYPE ** pap_buf,
                             OMX_U32 a_port_index,
                             OMX_PTR ap_app_private, OMX_U32 a_size_bytes)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoloaded_FreeBuffer (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_U32 a_port_index, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoloaded_EmptyThisBuffer (const void *ap_obj,
                              OMX_HANDLETYPE ap_hdl,
                              OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoloaded_FillThisBuffer (const void *ap_obj,
                             OMX_HANDLETYPE ap_hdl,
                             OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

/*
 * from tizstate class
 */

static OMX_ERRORTYPE
idletoloaded_trans_complete (const void *ap_obj,
                             OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (tiz_servant_get_hdl (ap_servant)),
                 TIZ_CBUF (tiz_servant_get_hdl (ap_servant)),
                 "Trans complete to state [%s]...",
                 tiz_fsm_state_to_str (a_new_state));
  assert (OMX_StateLoaded == a_new_state);
  return tizstate_super_trans_complete (tizidletoloaded, ap_obj, ap_servant,
                                        a_new_state);
}

/*
 * initialization
 */

const void *tizidletoloaded;

void
init_tizidletoloaded (void)
{
  if (!tizidletoloaded)
    {
      init_tizidle ();
      tizidletoloaded =
        factory_new
        (tizstate_class, "tizidletoloaded",
         tizidle, sizeof (struct tizidletoloaded),
         ctor, idletoloaded_ctor,
         dtor, idletoloaded_dtor,
         tizapi_SetParameter, idletoloaded_SetParameter,
         tizapi_GetState, idletoloaded_GetState,
         tizapi_UseBuffer, idletoloaded_UseBuffer,
         tizapi_AllocateBuffer, idletoloaded_AllocateBuffer,
         tizapi_FreeBuffer, idletoloaded_FreeBuffer,
         tizapi_EmptyThisBuffer, idletoloaded_EmptyThisBuffer,
         tizapi_FillThisBuffer, idletoloaded_FillThisBuffer,
         tizstate_trans_complete, idletoloaded_trans_complete, 0);
    }
}
