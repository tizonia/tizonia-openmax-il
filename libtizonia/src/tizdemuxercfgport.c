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

#include "tizdemuxercfgport.h"
#include "tizdemuxercfgport_decls.h"

#include "tizosal.h"

#include <assert.h>
#include <string.h>
#include <limits.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.demuxercfgport"
#endif


static char *
find_default_uri (tiz_demuxercfgport_t *ap_obj)
{
  const char       *p_uri  = NULL;
  char              fqd_key[OMX_MAX_STRINGNAME_SIZE];
  tiz_configport_t *p_base = (tiz_configport_t *) ap_obj; /* The base class
                                                             contains the
                                                             component name in
                                                             a member
                                                             variable */

  assert (NULL != ap_obj);

  /* Looking for OMX.component.name.default_uri */
  strncpy (fqd_key, p_base->comp_name_, OMX_MAX_STRINGNAME_SIZE - 1);
  /* Make sure fqd_key is null-terminated */
  fqd_key[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
  strncat (fqd_key, ".default_uri",
           OMX_MAX_STRINGNAME_SIZE - strlen (fqd_key) - 1);

  p_uri = tiz_rcfile_get_value ("plugins-data", fqd_key);
  assert (NULL != p_uri
          &&
          ".default_uri not found in configuration file...");
  TIZ_TRACE (tiz_api_get_hdl (ap_obj), "Default URI [%s]...", p_uri);
  return strndup (p_uri, PATH_MAX);
}

/*
 * tizdemuxercfgport class
 */

static void *
demuxer_cfgport_ctor (void *ap_obj, va_list * app)
{
  tiz_demuxercfgport_t *p_obj = super_ctor (typeOf (ap_obj, "tizdemuxercfgport"), ap_obj, app);
  if (NULL == (p_obj->p_uri_ = find_default_uri (p_obj)))
    {
      return NULL;
    }

  /* In addition to the indexes registered by the parent class, register here
     the demuxer-specific ones */
  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexConfigTimePosition)); /* r/w */
  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexConfigTimeSeekMode)); /* r/w */
  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexParamContentURI)); /* r/w */

  return p_obj;
}

static void *
demuxer_cfgport_dtor (void *ap_obj)
{
  tiz_demuxercfgport_t *p_obj = ap_obj;
  tiz_mem_free (p_obj->p_uri_);
  return super_dtor (typeOf (ap_obj, "tizdemuxercfgport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
demuxer_cfgport_GetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_demuxercfgport_t *p_obj = ap_obj;

  TIZ_TRACE (ap_hdl, "GetParameter [%s]...", tiz_idx_to_str (a_index));
  assert (NULL != p_obj);

  switch (a_index)
    {
    case OMX_IndexParamContentURI:
      {
        OMX_PARAM_CONTENTURITYPE *p_uri
          = (OMX_PARAM_CONTENTURITYPE *) ap_struct;
        OMX_U32 uri_buf_size =
          p_uri->nSize - sizeof (OMX_U32) - sizeof (OMX_VERSIONTYPE);
        OMX_U32 uri_len = strlen (p_obj->p_uri_);
        TIZ_TRACE (ap_hdl, "uri_buf_size [%d]...", uri_buf_size);
        TIZ_TRACE (ap_hdl, "p_obj->p_uri_ [%08x]...", p_obj->p_uri_);

        if (uri_buf_size < (uri_len + 1))
          {
            return OMX_ErrorBadParameter;
          }

        p_uri->nVersion.nVersion = OMX_VERSION;
        if (p_uri->contentURI)
          {
            strncpy ((char *) p_uri->contentURI, p_obj->p_uri_, uri_len + 1);
            p_uri->contentURI[uri_len] = '\0';
          }
      }
      break;

    default:
      {
        /* Delegate to the base port */
        return super_GetParameter (typeOf (ap_obj, "tizdemuxercfgport"),
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
demuxer_cfgport_SetParameter (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_demuxercfgport_t *p_obj = (tiz_demuxercfgport_t *) ap_obj;

  TIZ_TRACE (ap_hdl, "SetParameter [%s]...", tiz_idx_to_str (a_index));
  assert (NULL != p_obj);

  switch (a_index)
    {

    case OMX_IndexParamContentURI:
      {
        OMX_PARAM_CONTENTURITYPE *p_uri
          = (OMX_PARAM_CONTENTURITYPE *) ap_struct;
        OMX_U32 uri_size =
          p_uri->nSize - sizeof (OMX_U32) - sizeof (OMX_VERSIONTYPE);

        if (uri_size > PATH_MAX)
        {
          uri_size = PATH_MAX;
        }

        tiz_mem_free (p_obj->p_uri_);
        p_obj->p_uri_ = tiz_mem_calloc (1, uri_size);
        if (p_obj->p_uri_)
          {
            strncpy (p_obj->p_uri_, (char *) p_uri->contentURI, uri_size);
            p_uri->contentURI[uri_size - 1] = '\000';
          }

        TIZ_TRACE (ap_hdl, "Set URI [%s]...", p_obj->p_uri_);
      }
      break;

    default:
      {
        /* Delegate to the base port */
        return super_SetParameter (typeOf (ap_obj, "tizdemuxercfgport"),
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

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
            return rc;
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
            return rc;
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
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * tizdemuxercfgport_class = factory_new (classOf (tizconfigport),
                                                "tizdemuxercfgport_class",
                                                classOf (tizconfigport),
                                                sizeof (tiz_demuxercfgport_class_t),
                                                ap_tos, ap_hdl,
                                                ctor, demuxercfgport_class_ctor, 0);
  return tizdemuxercfgport_class;
}

void *
tiz_demuxercfgport_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * tizdemuxercfgport_class = tiz_get_type (ap_hdl, "tizdemuxercfgport_class");
  TIZ_LOG_CLASS (tizdemuxercfgport_class);
  void * tizdemuxercfgport =
    factory_new
    (tizdemuxercfgport_class,
     "tizdemuxercfgport",
     tizconfigport,
     sizeof (tiz_demuxercfgport_t),
     ap_tos, ap_hdl,
     ctor, demuxer_cfgport_ctor,
     dtor, demuxer_cfgport_dtor,
     tiz_api_GetParameter, demuxer_cfgport_GetParameter,
     tiz_api_SetParameter, demuxer_cfgport_SetParameter,
     tiz_api_GetConfig, demuxer_cfgport_GetConfig,
     tiz_api_SetConfig, demuxer_cfgport_SetConfig, 0);

  return tizdemuxercfgport;
}
