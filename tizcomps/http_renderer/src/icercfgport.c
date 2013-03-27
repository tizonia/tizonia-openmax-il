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


/*
 * icercfgport class
 */

static void *
icer_cfgport_ctor (void *ap_obj, va_list * app)
{
  struct icercfgport *p_obj = super_ctor (icercfgport, ap_obj, app);
  tizport_register_index (p_obj, OMX_TizoniaIndexParamHttpServer);
  p_obj->http_conf_.nSize = sizeof (OMX_TIZONIA_PARAM_HTTPSERVERTYPE);
  p_obj->http_conf_.nVersion.nVersion = OMX_VERSION;
  p_obj->http_conf_.nListeningPort = 8010;
  p_obj->http_conf_.nMaxClients = 5;
  return p_obj;
}

static void *
icer_cfgport_dtor (void *ap_obj)
{
  return super_dtor (icercfgport, ap_obj);
}

/*
 * from tizapi
 */

static OMX_ERRORTYPE
icer_cfgport_GetParameter (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct icercfgport *p_obj = ap_obj;

  if (OMX_TizoniaIndexParamHttpServer == a_index)
    {
      OMX_TIZONIA_PARAM_HTTPSERVERTYPE *p_http_conf
        = (OMX_TIZONIA_PARAM_HTTPSERVERTYPE *) ap_struct;

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
  struct icercfgport *p_obj = (struct icercfgport *) ap_obj;

  TIZ_LOG (TIZ_LOG_TRACE, "SetParameter [%s]...", tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamHttpServer == a_index)
    {
      OMX_TIZONIA_PARAM_HTTPSERVERTYPE *p_http_conf
        = (OMX_TIZONIA_PARAM_HTTPSERVERTYPE *) ap_struct;
      p_obj->http_conf_ = *p_http_conf;

      TIZ_LOG (TIZ_LOG_TRACE, "nListeningPort [%d]...",
               p_obj->http_conf_.nListeningPort);
      TIZ_LOG (TIZ_LOG_TRACE, "nMaxClients [%d]...",
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
 * icercfgport_class
 */

static void *
icer_cfgport_class_ctor (void *ap_obj, va_list * app)
{
  struct icercfgport_class *p_obj =
    super_ctor (icercfgport_class, ap_obj, app);
  typedef void (*voidf) ();
  voidf selector;
  va_list ap;
  va_copy (ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      /* voidf method = va_arg (ap, voidf); */
      /*          if (selector == (voidf) tiz_servant_tick) */
      /*             { */
      /*                *(voidf*) & p_obj->tick = method; */
      /*             } */
      /*          else if (selector == (voidf) tiz_servant_enqueue) */
      /*             { */
      /*                *(voidf*) & p_obj->enqueue = method; */
      /*             } */

    }

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

const void *icercfgport, *icercfgport_class;

void
init_icercfgport (void)
{

  if (!icercfgport_class)
    {
      init_tizconfigport ();
      icercfgport_class = factory_new (tizconfigport_class,
                                       "icercfgport_class",
                                       tizconfigport_class,
                                       sizeof (struct icercfgport_class),
                                       ctor, icer_cfgport_class_ctor, 0);

    }

  if (!icercfgport)
    {
      init_tizconfigport ();
      icercfgport =
        factory_new
        (icercfgport_class,
         "icercfgport",
         tizconfigport,
         sizeof (struct icercfgport),
         ctor, icer_cfgport_ctor,
         dtor, icer_cfgport_dtor,
         tizapi_GetParameter, icer_cfgport_GetParameter,
         tizapi_SetParameter, icer_cfgport_SetParameter, 0);
    }

}
