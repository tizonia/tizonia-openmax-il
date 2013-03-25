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

#include "tizivrport.h"
#include "tizivrport_decls.h"

#include "tizutils.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.ivrport"
#endif

/*
 * tizivrport class
 */

static void *
ivrport_ctor (void *ap_obj, va_list * app)
{
  struct tizivrport *p_obj = super_ctor (tizivrport, ap_obj, app);
  struct tizport *p_base = ap_obj;

  tizport_register_index (p_obj, OMX_IndexConfigCommonRotate);
  tizport_register_index (p_obj, OMX_IndexConfigCommonMirror);
  tizport_register_index (p_obj, OMX_IndexConfigCommonScale);
  tizport_register_index (p_obj, OMX_IndexConfigCommonInputCrop);
  tizport_register_index (p_obj, OMX_IndexConfigTimeRenderingDelay);

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
ivrport_dtor (void *ap_obj)
{
  return super_dtor (tizivrport, ap_obj);
}

/*
 * from tizapi
 */

static OMX_ERRORTYPE
ivrport_GetConfig (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tizivrport *p_obj = ap_obj;

  TIZ_LOG (TIZ_LOG_TRACE, "GetConfig [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexConfigCommonRotate:
      {
        OMX_CONFIG_ROTATIONTYPE *p_crotate
          = (OMX_CONFIG_ROTATIONTYPE *) ap_struct;
        *p_crotate = p_obj->crotate_;
      }
      break;

    case OMX_IndexConfigCommonMirror:
      {
        OMX_CONFIG_MIRRORTYPE *p_cmirror = (OMX_CONFIG_MIRRORTYPE *) ap_struct;
        *p_cmirror = p_obj->cmirror_;
      }
      break;

    case OMX_IndexConfigCommonScale:
      {
        OMX_CONFIG_SCALEFACTORTYPE *p_cscale
          = (OMX_CONFIG_SCALEFACTORTYPE *) ap_struct;
        *p_cscale = p_obj->cscale_;
      }
      break;

    case OMX_IndexConfigCommonInputCrop:
      {
        OMX_CONFIG_RECTTYPE *p_cincrop = (OMX_CONFIG_RECTTYPE *) ap_struct;
        *p_cincrop = p_obj->cincrop_;
      }
      break;

    case OMX_IndexConfigTimeRenderingDelay:
      {
        OMX_TIME_CONFIG_RENDERINGDELAYTYPE *p_crendelay
          = (OMX_TIME_CONFIG_RENDERINGDELAYTYPE *) ap_struct;
        *p_crendelay = p_obj->crendelay_;
      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_GetConfig (tizivrport,
                                ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ivrport_SetConfig (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  struct tizivrport *p_obj = (struct tizivrport *) ap_obj;

  TIZ_LOG (TIZ_LOG_TRACE, "SetConfig [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexConfigCommonRotate:
      {
        OMX_CONFIG_ROTATIONTYPE *p_crotate
          = (OMX_CONFIG_ROTATIONTYPE *) ap_struct;
        p_obj->crotate_.nRotation = p_crotate->nRotation;
      }
      break;

    case OMX_IndexConfigCommonMirror:
      {
        OMX_CONFIG_MIRRORTYPE *p_cmirror = (OMX_CONFIG_MIRRORTYPE *) ap_struct;
        p_obj->cmirror_.eMirror = p_cmirror->eMirror;
      }
      break;

    case OMX_IndexConfigCommonScale:
      {
        OMX_CONFIG_SCALEFACTORTYPE *p_cscale
          = (OMX_CONFIG_SCALEFACTORTYPE *) ap_struct;
        p_obj->cscale_.xWidth = p_cscale->xWidth;
        p_obj->cscale_.xHeight = p_cscale->xHeight;
      }
      break;

    case OMX_IndexConfigCommonInputCrop:
      {
        OMX_CONFIG_RECTTYPE *p_cincrop = (OMX_CONFIG_RECTTYPE *) ap_struct;
        p_obj->cincrop_.nLeft = p_cincrop->nLeft;
        p_obj->cincrop_.nTop = p_cincrop->nTop;
        p_obj->cincrop_.nWidth = p_cincrop->nWidth;
        p_obj->cincrop_.nHeight = p_cincrop->nHeight;
      }
      break;

    case OMX_IndexConfigTimeRenderingDelay:
      {
        /* This is a read-only index. Simply ignore it. */
      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_SetConfig (tizivrport,
                                ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
  ivrport_set_portdef_format
  (void *ap_obj, const OMX_PARAM_PORTDEFINITIONTYPE * ap_pdef)
{
  struct tizport *p_base = ap_obj;
  /* TODO: Finalize this function */
  p_base->portdef_.format.video.nFrameWidth =
    ap_pdef->format.video.nFrameWidth;
  p_base->portdef_.format.video.nFrameHeight =
    ap_pdef->format.video.nFrameHeight;
  return OMX_ErrorNone;
}

static OMX_BOOL
ivrport_check_tunnel_compat (const void *ap_obj,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;

  assert (ap_this_def);
  assert (ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_LOG (TIZ_LOG_TRACE,
               "port [%d] check_tunnel_compat : "
               "Video domain not found, instead found domain [%d]",
               p_obj->pid_, ap_other_def->eDomain);
      return OMX_FALSE;
    }

  /* TODO : Review these compatibility checks */

  TIZ_LOG (TIZ_LOG_TRACE, "port [%d] check_tunnel_compat [OK]", p_obj->pid_);

  return OMX_TRUE;
}

/*
 * tizivrport_class
 */

static void *
ivrport_class_ctor (void *ap_obj, va_list * app)
{
  struct tizivrport_class *p_obj = super_ctor (tizivrport_class, ap_obj, app);
  typedef void (*voidf) ();
  voidf selector;
  va_list ap;
  va_copy (ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      /* voidf method = va_arg (ap, voidf); */
      /*          if (selector == (voidf) tizservant_tick) */
      /*             { */
      /*                *(voidf*) & p_obj->tick = method; */
      /*             } */
      /*          else if (selector == (voidf) tizservant_enqueue) */
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

const void *tizivrport, *tizivrport_class;

void
init_tizivrport (void)
{

  if (!tizivrport_class)
    {
      init_tizvideoport ();
      tizivrport_class = factory_new (tizvideoport_class,
                                      "tizivrport_class",
                                      tizvideoport_class,
                                      sizeof (struct tizivrport_class),
                                      ctor, ivrport_class_ctor, 0);

    }

  if (!tizivrport)
    {
      init_tizvideoport ();
      tizivrport =
        factory_new
        (tizivrport_class,
         "tizivrport",
         tizvideoport,
         sizeof (struct tizivrport),
         ctor, ivrport_ctor,
         dtor, ivrport_dtor,
         tizapi_GetConfig, ivrport_GetConfig,
         tizapi_SetConfig, ivrport_SetConfig,
         tizport_set_portdef_format, ivrport_set_portdef_format,
         tizport_check_tunnel_compat, ivrport_check_tunnel_compat, 0);
    }

}
