/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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

#include "tizidletoloaded.h"
#include "tizstate.h"
#include "tizstate_decls.h"
#include "tizutils.h"
#include "tizplatform.h"

#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.idletoloaded"
#endif

static void *
idletoloaded_ctor (void * ap_obj, va_list * app)
{
  tiz_idletoloaded_t * p_obj
    = super_ctor (typeOf (ap_obj, "tizidletoloaded"), ap_obj, app);
  return p_obj;
}

static void *
idletoloaded_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tizidletoloaded"), ap_obj);
}

/*
 * from tiz_api class
 */

static OMX_ERRORTYPE
idletoloaded_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                           OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoloaded_GetState (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                       OMX_STATETYPE * ap_state)
{
  *ap_state = OMX_StateIdle;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
idletoloaded_UseBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                        OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                        OMX_U32 a_port_index, OMX_PTR ap_app_private,
                        OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoloaded_AllocateBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                             OMX_BUFFERHEADERTYPE ** pap_buf,
                             OMX_U32 a_port_index, OMX_PTR ap_app_private,
                             OMX_U32 a_size_bytes)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoloaded_FreeBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_U32 a_port_index, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoloaded_EmptyThisBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                              OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
idletoloaded_FillThisBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                             OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

/*
 * from tiz_state class
 */

static OMX_ERRORTYPE
idletoloaded_trans_complete (const void * ap_obj, OMX_PTR ap_servant,
                             OMX_STATETYPE a_new_state)
{
  TIZ_TRACE (handleOf (ap_servant), "Trans complete to state [%s]...",
             tiz_fsm_state_to_str ((tiz_fsm_state_id_t) a_new_state));
  assert (OMX_StateLoaded == a_new_state);
  return tiz_state_super_trans_complete (typeOf (ap_obj, "tizidletoloaded"),
                                         ap_obj, ap_servant, a_new_state);
}

/*
 * idletoloaded_class
 */

static void *
idletoloaded_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizidletoloaded_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_idletoloaded_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizidle = tiz_get_type (ap_hdl, "tizidle");
  void * tizidletoloaded_class
    = factory_new (classOf (tizidle), "tizidletoloaded_class",
                   classOf (tizidle), sizeof (tiz_idletoloaded_class_t), ap_tos,
                   ap_hdl, ctor, idletoloaded_class_ctor, 0);
  return tizidletoloaded_class;
}

void *
tiz_idletoloaded_init (void * ap_tos, void * ap_hdl)
{
  void * tizidle = tiz_get_type (ap_hdl, "tizidle");
  void * tizidletoloaded_class = tiz_get_type (ap_hdl, "tizidletoloaded_class");
  TIZ_LOG_CLASS (tizidletoloaded_class);
  void * tizidletoloaded = factory_new (
    tizidletoloaded_class, "tizidletoloaded", tizidle,
    sizeof (tiz_idletoloaded_t), ap_tos, ap_hdl, ctor, idletoloaded_ctor, dtor,
    idletoloaded_dtor, tiz_api_SetParameter, idletoloaded_SetParameter,
    tiz_api_GetState, idletoloaded_GetState, tiz_api_UseBuffer,
    idletoloaded_UseBuffer, tiz_api_AllocateBuffer, idletoloaded_AllocateBuffer,
    tiz_api_FreeBuffer, idletoloaded_FreeBuffer, tiz_api_EmptyThisBuffer,
    idletoloaded_EmptyThisBuffer, tiz_api_FillThisBuffer,
    idletoloaded_FillThisBuffer, tiz_state_trans_complete,
    idletoloaded_trans_complete, 0);

  return tizidletoloaded;
}
