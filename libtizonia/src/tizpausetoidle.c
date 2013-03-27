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
 * @file   tizpausetoidle.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - PauseToIdle OMX IL substate implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizpausetoidle.h"
#include "tizstate_decls.h"
#include "tizutils.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.pausetoidle"
#endif

static void *
pausetoidle_ctor (void *ap_obj, va_list * app)
{
  struct tizpausetoidle *p_obj = super_ctor (tizpausetoidle, ap_obj, app);
  return p_obj;
}

static void *
pausetoidle_dtor (void *ap_obj)
{
  return super_dtor (tizpausetoidle, ap_obj);
}

static OMX_ERRORTYPE
pausetoidle_GetState (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  *ap_state = OMX_StatePause;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
pausetoidle_UseBuffer (const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl,
                       OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                       OMX_U32 a_port_index,
                       OMX_PTR ap_app_private,
                       OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
pausetoidle_EmptyThisBuffer (const void *ap_obj,
                             OMX_HANDLETYPE ap_hdl,
                             OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
pausetoidle_FillThisBuffer (const void *ap_obj,
                            OMX_HANDLETYPE ap_hdl,
                            OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

/*
 * from tizstate class
 */

static OMX_ERRORTYPE
pausetoidle_trans_complete (const void *ap_obj,
                            OMX_PTR ap_servant, OMX_STATETYPE a_new_state)
{
  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (tiz_servant_get_hdl (ap_servant)),
                 TIZ_CBUF (tiz_servant_get_hdl (ap_servant)),
                 "Trans complete to state [%s]...",
                 tiz_fsm_state_to_str (a_new_state));
  assert (OMX_StateIdle == a_new_state);
  return tizstate_super_trans_complete (tizpausetoidle, ap_obj, ap_servant,
                                        a_new_state);
}

/*
 * initialization
 */

const void *tizpausetoidle;

void
init_tizpausetoidle (void)
{
  if (!tizpausetoidle)
    {
      init_tizpause ();
      tizpausetoidle =
        factory_new
        (tizstate_class, "tizpausetoidle",
         tizpause, sizeof (struct tizpausetoidle),
         ctor, pausetoidle_ctor,
         dtor, pausetoidle_dtor,
         tizapi_GetState, pausetoidle_GetState,
         tizapi_UseBuffer, pausetoidle_UseBuffer,
         tizapi_EmptyThisBuffer, pausetoidle_EmptyThisBuffer,
         tizapi_FillThisBuffer, pausetoidle_FillThisBuffer,
         tizstate_trans_complete, pausetoidle_trans_complete, 0);
    }
}
