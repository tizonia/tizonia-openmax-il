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
 * @file   tizivrport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  i/v renderer port  class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizplatform.h>

#include "tizutils.h"
#include "tizivrport.h"
#include "tizivrport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.ivrport"
#endif

/*
 * tizivrport class
 */

static void *
ivrport_ctor (void * ap_obj, va_list * app)
{
  tiz_ivrport_t * p_obj
    = super_ctor (typeOf (ap_obj, "tizivrport"), ap_obj, app);
  tiz_port_t * p_base = ap_obj;

  tiz_port_register_index (p_obj, OMX_IndexConfigCommonRotate);
  tiz_port_register_index (p_obj, OMX_IndexConfigCommonMirror);
  tiz_port_register_index (p_obj, OMX_IndexConfigCommonScale);
  tiz_port_register_index (p_obj, OMX_IndexConfigCommonInputCrop);
  tiz_port_register_index (p_obj, OMX_IndexConfigTimeRenderingDelay);

  /* Initialize the OMX_CONFIG_ROTATIONTYPE structure */
  p_obj->crotate_.nSize = sizeof (OMX_CONFIG_ROTATIONTYPE);
  p_obj->crotate_.nVersion.nVersion = OMX_VERSION;
  p_obj->crotate_.nPortIndex = p_base->portdef_.nPortIndex;
  p_obj->crotate_.nRotation = 0;

  /* Initialize the OMX_CONFIG_MIRRORTYPE structure */
  p_obj->cmirror_.nSize = sizeof (OMX_CONFIG_MIRRORTYPE);
  p_obj->cmirror_.nVersion.nVersion = OMX_VERSION;
  p_obj->cmirror_.nPortIndex = p_base->portdef_.nPortIndex;
  p_obj->cmirror_.eMirror = OMX_MirrorNone;

  /* Initialize the OMX_CONFIG_SCALEFACTORTYPE structure */
  p_obj->cscale_.nSize = sizeof (OMX_CONFIG_SCALEFACTORTYPE);
  p_obj->cscale_.nVersion.nVersion = OMX_VERSION;
  p_obj->cscale_.nPortIndex = p_base->portdef_.nPortIndex;
  p_obj->cscale_.xWidth = 1;
  p_obj->cscale_.xHeight = 1;

  /* Initialize the OMX_CONFIG_RECTTYPE structure */
  p_obj->cincrop_.nSize = sizeof (OMX_CONFIG_RECTTYPE);
  p_obj->cincrop_.nVersion.nVersion = OMX_VERSION;
  p_obj->cincrop_.nPortIndex = p_base->portdef_.nPortIndex;
  p_obj->cincrop_.nLeft = 0;
  p_obj->cincrop_.nTop = 0;
  p_obj->cincrop_.nWidth = 0;
  p_obj->cincrop_.nHeight = 0;

  /* Initialize the OMX_TIME_CONFIG_RENDERINGDELAYTYPE structure */
  p_obj->crendelay_.nSize = sizeof (OMX_TIME_CONFIG_RENDERINGDELAYTYPE);
  p_obj->crendelay_.nVersion.nVersion = OMX_VERSION;
  p_obj->crendelay_.nPortIndex = p_base->portdef_.nPortIndex;
  p_obj->crendelay_.nRenderingDelay = 0;

  return p_obj;
}

static void *
ivrport_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tizivrport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
ivrport_GetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_ivrport_t * p_obj = ap_obj;

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] GetConfig [%s]",
             tiz_port_index (ap_obj), tiz_idx_to_str (a_index));
  assert (ap_obj);

  switch (a_index)
    {
      case OMX_IndexConfigCommonRotate:
        {
          OMX_CONFIG_ROTATIONTYPE * p_crotate
            = (OMX_CONFIG_ROTATIONTYPE *) ap_struct;
          *p_crotate = p_obj->crotate_;
        }
        break;

      case OMX_IndexConfigCommonMirror:
        {
          OMX_CONFIG_MIRRORTYPE * p_cmirror
            = (OMX_CONFIG_MIRRORTYPE *) ap_struct;
          *p_cmirror = p_obj->cmirror_;
        }
        break;

      case OMX_IndexConfigCommonScale:
        {
          OMX_CONFIG_SCALEFACTORTYPE * p_cscale
            = (OMX_CONFIG_SCALEFACTORTYPE *) ap_struct;
          *p_cscale = p_obj->cscale_;
        }
        break;

      case OMX_IndexConfigCommonInputCrop:
        {
          OMX_CONFIG_RECTTYPE * p_cincrop = (OMX_CONFIG_RECTTYPE *) ap_struct;
          *p_cincrop = p_obj->cincrop_;
        }
        break;

      case OMX_IndexConfigTimeRenderingDelay:
        {
          OMX_TIME_CONFIG_RENDERINGDELAYTYPE * p_crendelay
            = (OMX_TIME_CONFIG_RENDERINGDELAYTYPE *) ap_struct;
          *p_crendelay = p_obj->crendelay_;
        }
        break;

      default:
        {
          /* Try the parent's indexes */
          return super_GetConfig (typeOf (ap_obj, "tizivrport"), ap_obj, ap_hdl,
                                  a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ivrport_SetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_ivrport_t * p_obj = (tiz_ivrport_t *) ap_obj;

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] SetConfig [%s]",
             tiz_port_index (ap_obj), tiz_idx_to_str (a_index));
  assert (ap_obj);

  switch (a_index)
    {
      case OMX_IndexConfigCommonRotate:
        {
          OMX_CONFIG_ROTATIONTYPE * p_crotate
            = (OMX_CONFIG_ROTATIONTYPE *) ap_struct;
          p_obj->crotate_.nRotation = p_crotate->nRotation;
        }
        break;

      case OMX_IndexConfigCommonMirror:
        {
          OMX_CONFIG_MIRRORTYPE * p_cmirror
            = (OMX_CONFIG_MIRRORTYPE *) ap_struct;
          p_obj->cmirror_.eMirror = p_cmirror->eMirror;
        }
        break;

      case OMX_IndexConfigCommonScale:
        {
          OMX_CONFIG_SCALEFACTORTYPE * p_cscale
            = (OMX_CONFIG_SCALEFACTORTYPE *) ap_struct;
          p_obj->cscale_.xWidth = p_cscale->xWidth;
          p_obj->cscale_.xHeight = p_cscale->xHeight;
        }
        break;

      case OMX_IndexConfigCommonInputCrop:
        {
          OMX_CONFIG_RECTTYPE * p_cincrop = (OMX_CONFIG_RECTTYPE *) ap_struct;
          p_obj->cincrop_.nLeft = p_cincrop->nLeft;
          p_obj->cincrop_.nTop = p_cincrop->nTop;
          p_obj->cincrop_.nWidth = p_cincrop->nWidth;
          p_obj->cincrop_.nHeight = p_cincrop->nHeight;
        }
        break;

      case OMX_IndexConfigTimeRenderingDelay:
        {
          /* This is a read-only index. Simply ignore it. */
          TIZ_NOTICE (ap_hdl, "Ignoring read-only index [%s] ",
                      tiz_idx_to_str (a_index));
        }
        break;

      default:
        {
          /* Try the parent's indexes */
          return super_SetConfig (typeOf (ap_obj, "tizivrport"), ap_obj, ap_hdl,
                                  a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static bool
ivrport_check_tunnel_compat (const void * ap_obj,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t * p_obj = (tiz_port_t *) ap_obj;

  assert (ap_this_def);
  assert (ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_ERROR (
        handleOf (ap_obj),
        "PORT [%d] : Video domain not found, instead found domain [%d]",
        p_obj->pid_, ap_other_def->eDomain);
      return false;
    }

  /* TODO : Review these compatibility checks */

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] check_tunnel_compat [OK]",
             p_obj->pid_);

  return true;
}

/*
 * tizivrport_class
 */

static void *
ivrport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizivrport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_ivrport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizvideoport = tiz_get_type (ap_hdl, "tizvideoport");
  void * tizivrport_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizvideoport), "tizivrport_class", classOf (tizvideoport),
     sizeof (tiz_ivrport_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, ivrport_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizivrport_class;
}

void *
tiz_ivrport_init (void * ap_tos, void * ap_hdl)
{
  void * tizvideoport = tiz_get_type (ap_hdl, "tizvideoport");
  void * tizivrport_class = tiz_get_type (ap_hdl, "tizivrport_class");
  TIZ_LOG_CLASS (tizivrport_class);
  void * tizivrport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizivrport_class, "tizivrport", tizvideoport, sizeof (tiz_ivrport_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, ivrport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, ivrport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetConfig, ivrport_GetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetConfig, ivrport_SetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_check_tunnel_compat, ivrport_check_tunnel_compat,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizivrport;
}
