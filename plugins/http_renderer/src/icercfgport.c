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
 * @file   icercfgport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Http renderer config port implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <limits.h>

#include "icercfgport.h"
#include "icercfgport_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.cfgport"
#endif

#define ARATELIA_HTTP_RENDERER_DEFAULT_HTTP_SERVER_PORT 8010

/*
 * icercfgport class
 */

static void *
icer_cfgport_ctor (void *ap_obj, va_list * app)
{
  icer_cfgport_t *p_obj = super_ctor (icercfgport, ap_obj, app);

  tiz_port_register_index (p_obj, OMX_TizoniaIndexParamHttpServer);
  p_obj->http_conf_.nSize             = sizeof (OMX_TIZONIA_HTTPSERVERTYPE);
  p_obj->http_conf_.nVersion.nVersion = OMX_VERSION;
  p_obj->http_conf_.nListeningPort    = ARATELIA_HTTP_RENDERER_DEFAULT_HTTP_SERVER_PORT;
  p_obj->http_conf_.nMaxClients       = 5;

  return p_obj;
}

static void *
icer_cfgport_dtor (void *ap_obj)
{
  return super_dtor (icercfgport, ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
icer_cfgport_GetParameter (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const icer_cfgport_t *p_obj = ap_obj;

  if (OMX_TizoniaIndexParamHttpServer == a_index)
    {
      OMX_TIZONIA_HTTPSERVERTYPE *p_http_conf
        = (OMX_TIZONIA_HTTPSERVERTYPE *) ap_struct;

      *p_http_conf = p_obj->http_conf_;
    }
  else
    {
      /* Delegate to the base port */
      return super_GetParameter (icercfgport,
                                 ap_obj, ap_hdl, a_index, ap_struct);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_cfgport_SetParameter (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  icer_cfgport_t *p_obj = (icer_cfgport_t *) ap_obj;

  if (OMX_TizoniaIndexParamHttpServer == a_index)
    {
      OMX_TIZONIA_HTTPSERVERTYPE *p_http_conf
        = (OMX_TIZONIA_HTTPSERVERTYPE *) ap_struct;
      p_obj->http_conf_ = *p_http_conf;

      TIZ_TRACE (ap_hdl, "nListeningPort [%d]...",
                p_obj->http_conf_.nListeningPort);
      TIZ_TRACE (ap_hdl, "nMaxClients [%d]...",
                p_obj->http_conf_.nMaxClients);
    }
  else
    {
      /* Delegate to the base port */
      return super_SetParameter (icercfgport,
                                 ap_obj, ap_hdl, a_index, ap_struct);
    }

  return OMX_ErrorNone;
}

/*
 * initialization
 */

const void *icercfgport;

OMX_ERRORTYPE
icer_cfgport_init (void)
{
  if (!icercfgport)
    {
      tiz_check_omx_err_ret_oom (tiz_configport_init ());
      tiz_check_null_ret_oom
        (icercfgport =
         factory_new
         (tizconfigport_class,
          "icercfgport",
          tizconfigport,
          sizeof (icer_cfgport_t),
          ctor, icer_cfgport_ctor,
          dtor, icer_cfgport_dtor,
          tiz_api_GetParameter, icer_cfgport_GetParameter,
          tiz_api_SetParameter, icer_cfgport_SetParameter, 0));
    }
  return OMX_ErrorNone;
}
