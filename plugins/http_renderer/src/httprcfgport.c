/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   httprcfgport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Http renderer config port implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <limits.h>

#include <tizplatform.h>

#include "httpr.h"
#include "httprcfgport.h"
#include "httprcfgport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.cfgport"
#endif

/*
 * httprcfgport class
 */

static void *
httpr_cfgport_ctor (void * ap_obj, va_list * app)
{
  httpr_cfgport_t * p_obj
    = super_ctor (typeOf (ap_obj, "httprcfgport"), ap_obj, app);

  assert (p_obj);

  tiz_port_register_index (p_obj, OMX_TizoniaIndexParamHttpServer);
  p_obj->http_conf_.nSize = sizeof (OMX_TIZONIA_HTTPSERVERTYPE);
  p_obj->http_conf_.nVersion.nVersion = OMX_VERSION;
  p_obj->http_conf_.nListeningPort
    = ARATELIA_HTTP_RENDERER_DEFAULT_HTTP_SERVER_PORT;
  p_obj->http_conf_.nMaxClients = 5;

  return p_obj;
}

static void *
httpr_cfgport_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "httprcfgport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
httpr_cfgport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                            OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const httpr_cfgport_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  assert (p_obj);

  if (OMX_TizoniaIndexParamHttpServer == a_index)
    {
      OMX_TIZONIA_HTTPSERVERTYPE * p_http_conf
        = (OMX_TIZONIA_HTTPSERVERTYPE *) ap_struct;

      *p_http_conf = p_obj->http_conf_;
    }
  else
    {
      /* Delegate to the base port */
      rc = super_GetParameter (typeOf (ap_obj, "httprcfgport"), ap_obj, ap_hdl,
                               a_index, ap_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
httpr_cfgport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                            OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  httpr_cfgport_t * p_obj = (httpr_cfgport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  assert (p_obj);

  if (OMX_TizoniaIndexParamHttpServer == a_index)
    {
      OMX_TIZONIA_HTTPSERVERTYPE * p_http_conf
        = (OMX_TIZONIA_HTTPSERVERTYPE *) ap_struct;
      p_obj->http_conf_ = *p_http_conf;

      TIZ_TRACE (ap_hdl, "nListeningPort [%d]...",
                 p_obj->http_conf_.nListeningPort);
      TIZ_TRACE (ap_hdl, "nMaxClients [%d]...", p_obj->http_conf_.nMaxClients);
    }
  else
    {
      /* Delegate to the base port */
      rc = super_SetParameter (typeOf (ap_obj, "httprcfgport"), ap_obj, ap_hdl,
                               a_index, ap_struct);
    }

  return rc;
}

/*
 * httpr_cfgport_class
 */

static void *
httpr_cfgport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "httprcfgport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
httpr_cfgport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * httprcfgport_class
    = factory_new (classOf (tizconfigport), "httprcfgport_class",
                   classOf (tizconfigport), sizeof (httpr_cfgport_class_t),
                   ap_tos, ap_hdl, ctor, httpr_cfgport_class_ctor, 0);
  return httprcfgport_class;
}

void *
httpr_cfgport_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * httprcfgport_class = tiz_get_type (ap_hdl, "httprcfgport_class");
  TIZ_LOG_CLASS (httprcfgport_class);
  void * httprcfgport = factory_new (
    httprcfgport_class, "httprcfgport", tizconfigport, sizeof (httpr_cfgport_t),
    ap_tos, ap_hdl, ctor, httpr_cfgport_ctor, dtor, httpr_cfgport_dtor,
    tiz_api_GetParameter, httpr_cfgport_GetParameter, tiz_api_SetParameter,
    httpr_cfgport_SetParameter, 0);

  return httprcfgport;
}
