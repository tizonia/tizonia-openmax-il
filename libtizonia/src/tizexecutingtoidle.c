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
 * @file   tizexecutingtoidle.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - ExecutingToIdle OMX IL substate implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizexecutingtoidle.h"
#include "tizstate_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.fsm.executingtoidle"
#endif


static void *
tiz_executingtoidle_ctor (void *ap_obj, va_list * app)
{
  struct tizexecutingtoidle *p_obj =
    super_ctor (tizexecutingtoidle, ap_obj, app);
  return p_obj;
}

static void *
tiz_executingtoidle_dtor (void *ap_obj)
{
  return super_dtor (tizexecutingtoidle, ap_obj);
}

static OMX_ERRORTYPE
tiz_executingtoidle_GetState (const void *ap_obj,
                              OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  *ap_state = OMX_StateExecuting;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tiz_executingtoidle_UseBuffer (const void *ap_obj,
                               OMX_HANDLETYPE ap_hdl,
                               OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                               OMX_U32 a_port_index,
                               OMX_PTR ap_app_private,
                               OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
tiz_executingtoidle_ComponentDeInit (const void *ap_obj,
                                     OMX_HANDLETYPE ap_hdl)
{
  return OMX_ErrorNotImplemented;
}

/*
 * initialization
 */

/*
 * from tizstate
 */

static OMX_ERRORTYPE
tiz_executingtoidle_trans_complete (const void *ap_obj,
                                    OMX_PTR ap_servant,
                                    OMX_STATETYPE a_new_state)
{
  assert (OMX_StateIdle == a_new_state);
  return tizstate_super_trans_complete (tizexecutingtoidle, ap_obj,
                                        ap_servant, a_new_state);
}


/*
 * initialization
 */

const void *tizexecutingtoidle;

void
init_tizexecutingtoidle (void)
{
  if (!tizexecutingtoidle)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Initializing tizexecutingtoidle...");

      init_tizexecuting ();
      tizexecutingtoidle =
        factory_new
        (tizstate_class, "tizexecutingtoidle",
         tizexecuting, sizeof (struct tizexecutingtoidle),
         ctor, tiz_executingtoidle_ctor,
         dtor, tiz_executingtoidle_dtor,
         tizapi_GetState, tiz_executingtoidle_GetState,
         tizapi_UseBuffer, tiz_executingtoidle_UseBuffer,
         tizapi_ComponentDeInit, tiz_executingtoidle_ComponentDeInit,
         tizstate_trans_complete, tiz_executingtoidle_trans_complete, 0);
    }
}
