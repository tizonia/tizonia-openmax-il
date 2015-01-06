/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizdemuxercfgport.c
 * @author Juan A. Rubio <juan.rubio@aaratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Demuxer config port implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <tizplatform.h>

#include "tizdemuxercfgport.h"
#include "tizdemuxercfgport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.demuxercfgport"
#endif


/*
 * tizdemuxercfgport class
 */

static void *
demuxer_cfgport_ctor (void *ap_obj, va_list * app)
{
  tiz_demuxercfgport_t *p_obj
    = super_ctor (typeOf (ap_obj, "tizdemuxercfgport"), ap_obj, app);

  /* In addition to the indexes registered by the parent class, register here
     the demuxer-specific ones */
  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexConfigTimePosition)); /* r/w */
  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexConfigTimeSeekMode)); /* r/w */

  return p_obj;
}

static void *
demuxer_cfgport_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tizdemuxercfgport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
demuxer_cfgport_GetConfig (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                           OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_demuxercfgport_t *p_obj = (tiz_demuxercfgport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "GetConfig [%s]...", tiz_idx_to_str (a_index));
  assert (NULL != p_obj);

  switch (a_index)
    {
    case OMX_IndexConfigTimePosition:
    case OMX_IndexConfigTimeSeekMode:
      {
        /* Only the processor knows about current position or seek mode. So
           lets get the processor to fill this info for us. */
        void *p_prc = tiz_get_prc (ap_hdl);
        assert (NULL != p_prc);
        if (OMX_ErrorNone != (rc = tiz_api_GetConfig (p_prc, ap_hdl,
                                                      a_index, ap_struct)))
          {
            TIZ_ERROR (ap_hdl, "[%s] : Error retrieving [%s] "
                      "from the processor", tiz_err_to_str (rc),
                      tiz_idx_to_str (a_index));
          }
      }
      break;

    default:
      {
        /* Delegate to the base port */
        rc = super_GetConfig (typeOf (ap_obj, "tizdemuxercfgport"),
                              ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return rc;
}

static OMX_ERRORTYPE
demuxer_cfgport_SetConfig (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                           OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_demuxercfgport_t *p_obj = (tiz_demuxercfgport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "SetConfig [%s]...", tiz_idx_to_str (a_index));
  assert (NULL != p_obj);

  switch (a_index)
    {
    case OMX_IndexConfigTimePosition:
    case OMX_IndexConfigTimeSeekMode:
      {
        /* Only the processor knows about current position or seek mode. So
           lets get the processor update this info for us. */
        void *p_prc = tiz_get_prc (ap_hdl);
        assert (NULL != p_prc);
        if (OMX_ErrorNone != (rc = tiz_api_SetConfig (p_prc, ap_hdl,
                                                      a_index, ap_struct)))
          {
            TIZ_ERROR (ap_hdl, "[%s] : Error retrieving [%s] "
                      "from the processor", tiz_err_to_str (rc),
                      tiz_idx_to_str (a_index));
          }
      }
      break;

    default:
      {
        /* Delegate to the base port */
        rc = super_SetConfig (typeOf (ap_obj, "tizdemuxercfgport"),
                              ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return rc;
}

/*
 * tizdemuxercfgport_class
 */

static void *
demuxercfgport_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizdemuxercfgport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_demuxercfgport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizuricfgport = tiz_get_type (ap_hdl, "tizuricfgport");
  void * tizdemuxercfgport_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizuricfgport), "tizdemuxercfgport_class", classOf (tizuricfgport), sizeof (tiz_demuxercfgport_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, demuxercfgport_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
                                                0);
  return tizdemuxercfgport_class;
}

void *
tiz_demuxercfgport_init (void * ap_tos, void * ap_hdl)
{
  void * tizuricfgport = tiz_get_type (ap_hdl, "tizuricfgport");
  void * tizdemuxercfgport_class = tiz_get_type (ap_hdl, "tizdemuxercfgport_class");
  TIZ_LOG_CLASS (tizdemuxercfgport_class);
  void * tizdemuxercfgport =
    factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizdemuxercfgport_class, "tizdemuxercfgport", tizuricfgport, sizeof (tiz_demuxercfgport_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, demuxer_cfgport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, demuxer_cfgport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetConfig, demuxer_cfgport_GetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetConfig, demuxer_cfgport_SetConfig,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizdemuxercfgport;
}
