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
 * along with Tizonia.  If not, see <chromecast://www.gnu.org/licenses/>.
 */

/**
 * @file   cc_cfgport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief A specialised config port class for the Google music renderer component
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

#include "cc_cfgport.h"
#include "cc_cfgport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer.cfgport"
#endif

/*
 * cc_cfgport class
 */

static void *
cc_cfgport_ctor (void * ap_obj, va_list * app)
{
  cc_cfgport_t * p_obj
    = super_ctor (typeOf (ap_obj, "cc_cfgport"), ap_obj, app);

  assert (p_obj);

  tiz_port_register_index (p_obj, OMX_TizoniaIndexParamChromecastSession);

  /* Initialize the OMX_TIZONIA_PARAM_CHROMECASTSESSIONTYPE structure */
  TIZ_INIT_OMX_STRUCT (p_obj->cc_session_);
  snprintf ((char *) p_obj->cc_session_.cNameOrIpAddr,
            sizeof (p_obj->cc_session_.cNameOrIpAddr), "127.0.0.1");

  return p_obj;
}

static void *
cc_cfgport_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "cc_cfgport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
cc_cfgport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const cc_cfgport_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamChromecastSession == a_index)
    {
      memcpy (ap_struct, &(p_obj->cc_session_),
              sizeof (OMX_TIZONIA_PARAM_CHROMECASTSESSIONTYPE));
    }
  else
    {
      /* Delegate to the base port */
      rc = super_GetParameter (typeOf (ap_obj, "cc_cfgport"), ap_obj, ap_hdl,
                               a_index, ap_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
cc_cfgport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  cc_cfgport_t * p_obj = (cc_cfgport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamChromecastSession == a_index)
    {
      memcpy (&(p_obj->cc_session_), ap_struct,
              sizeof (OMX_TIZONIA_PARAM_CHROMECASTSESSIONTYPE));
      p_obj->cc_session_.cNameOrIpAddr[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      TIZ_TRACE (ap_hdl, "Chromecast name of ip [%s]...",
                 p_obj->cc_session_.cNameOrIpAddr);
    }
  else
    {
      /* Delegate to the base port */
      rc = super_SetParameter (typeOf (ap_obj, "cc_cfgport"), ap_obj, ap_hdl,
                               a_index, ap_struct);
    }

  return rc;
}

/*
 * cc_cfgport_class
 */

static void *
cc_cfgport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "cc_cfgport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
cc_cfgport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizuricfgport = tiz_get_type (ap_hdl, "tizuricfgport");
  void * cc_cfgport_class
    = factory_new (classOf (tizuricfgport), "cc_cfgport_class",
                   classOf (tizuricfgport), sizeof (cc_cfgport_class_t), ap_tos,
                   ap_hdl, ctor, cc_cfgport_class_ctor, 0);
  return cc_cfgport_class;
}

void *
cc_cfgport_init (void * ap_tos, void * ap_hdl)
{
  void * tizuricfgport = tiz_get_type (ap_hdl, "tizuricfgport");
  void * cc_cfgport_class = tiz_get_type (ap_hdl, "cc_cfgport_class");
  TIZ_LOG_CLASS (cc_cfgport_class);
  void * cc_cfgport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_cfgport_class, "cc_cfgport", tizuricfgport, sizeof (cc_cfgport_t),
     /* TIZ_CLASS_COMMENT: class constructor */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_cfgport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, cc_cfgport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, cc_cfgport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, cc_cfgport_SetParameter,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return cc_cfgport;
}
