/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   tizuricfgport.c
 * @author Juan A. Rubio <juan.rubio@aaratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - URI config port implementation
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

#include "tizuricfgport.h"
#include "tizuricfgport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.uricfgport"
#endif

static char *
retrieve_default_uri_from_config (tiz_uricfgport_t * ap_obj)
{
  const char * p_uri = NULL;
  char * p_rv = NULL;
  char fqd_key[OMX_MAX_STRINGNAME_SIZE];
  long pathname_max = -1;
  tiz_configport_t * p_base = (tiz_configport_t *) ap_obj; /* The base class
                                                            contains the
                                                            component name in
                                                            a member
                                                            variable */
  assert (ap_obj);

  /* Looking for OMX.component.name.default_uri */
  strncpy (fqd_key, p_base->comp_name_, OMX_MAX_STRINGNAME_SIZE - 1);
  /* Make sure fqd_key is null-terminated */
  fqd_key[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
  strncat (fqd_key, ".default_uri",
           OMX_MAX_STRINGNAME_SIZE - strlen (fqd_key) - 1);

  p_uri = tiz_rcfile_get_value (TIZ_RCFILE_PLUGINS_DATA_SECTION, fqd_key);
  /* assert (p_uri && ".default_uri not found in configuration file..."); */
  TIZ_TRACE (handleOf (ap_obj), "Default URI [%s]...", p_uri);
  if (p_uri && (pathname_max = tiz_pathname_max (p_uri)) > 0)
    {
      p_rv = strndup (p_uri, pathname_max); /* A terminating \0 is added by
                                               strndup*/
    }
  return p_rv;
}

/*
 * tizuricfgport class
 */

static void *
uri_cfgport_ctor (void * ap_obj, va_list * app)
{
  tiz_uricfgport_t * p_obj
    = super_ctor (typeOf (ap_obj, "tizuricfgport"), ap_obj, app);
  p_obj->p_uri_ = retrieve_default_uri_from_config (p_obj);

  /* In addition to the indexes registered by the parent class, register here
     this port's specific ones */
  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_IndexParamContentURI)); /* r/w */

  return p_obj;
}

static void *
uri_cfgport_dtor (void * ap_obj)
{
  tiz_uricfgport_t * p_obj = ap_obj;
  tiz_mem_free (p_obj->p_uri_);
  return super_dtor (typeOf (ap_obj, "tizuricfgport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
uri_cfgport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                          OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_uricfgport_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "GetParameter [%s]...", tiz_idx_to_str (a_index));
  assert (p_obj);

  switch (a_index)
    {
      case OMX_IndexParamContentURI:
        {
          if (p_obj->p_uri_)
            {
              OMX_PARAM_CONTENTURITYPE * p_uri
                = (OMX_PARAM_CONTENTURITYPE *) ap_struct;
              const OMX_U32 uri_len = strlen (p_obj->p_uri_);

              if (p_uri && uri_len > 0)
                {
                  OMX_U32 uri_buf_offset
                    = sizeof (OMX_U32) + sizeof (OMX_VERSIONTYPE);
                  OMX_U32 uri_buf_size = (p_uri->nSize >= uri_buf_offset
                                            ? p_uri->nSize - uri_buf_offset
                                            : 0);

                  TIZ_TRACE (ap_hdl, "uri_buf_size [%d]...", uri_buf_size);

                  if (uri_buf_size < (uri_len + 1))
                    {
                      rc = OMX_ErrorBadParameter;
                    }
                  else
                    {
                      char * p_dest = (char *) p_uri->contentURI;
                      assert (p_dest);
                      assert (uri_len > 0);
                      p_uri->nVersion.nVersion = OMX_VERSION;
                      strncpy (p_dest, p_obj->p_uri_, uri_len);
                      p_dest[uri_len] = '\0';
                    }
                }
            }
        }
        break;

      default:
        {
          /* Delegate to the base port */
          rc = super_GetParameter (typeOf (ap_obj, "tizuricfgport"), ap_obj,
                                   ap_hdl, a_index, ap_struct);
        }
    };

  return rc;
}

static OMX_ERRORTYPE
uri_cfgport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                          OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_uricfgport_t * p_obj = (tiz_uricfgport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "SetParameter [%s]...", tiz_idx_to_str (a_index));
  assert (p_obj);

  switch (a_index)
    {
      case OMX_IndexParamContentURI:
        {
          OMX_PARAM_CONTENTURITYPE * p_uri
            = (OMX_PARAM_CONTENTURITYPE *) ap_struct;
          OMX_U32 uri_size
            = p_uri->nSize - sizeof (OMX_U32) - sizeof (OMX_VERSIONTYPE);
          const long pathname_max
            = tiz_pathname_max ((const char *) p_uri->contentURI);

          if (pathname_max > 0 && uri_size > pathname_max)
            {
              uri_size = pathname_max;
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
          rc = super_SetParameter (typeOf (ap_obj, "tizuricfgport"), ap_obj,
                                   ap_hdl, a_index, ap_struct);
        }
    };

  return rc;
}

/*
 * tizuricfgport_class
 */

static void *
uricfgport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizuricfgport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_uricfgport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * tizuricfgport_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizconfigport), "tizuricfgport_class", classOf (tizconfigport),
     sizeof (tiz_uricfgport_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, uricfgport_class_ctor,
     /* TIZ_CLASS_COMMENT: */
     0);
  return tizuricfgport_class;
}

void *
tiz_uricfgport_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * tizuricfgport_class = tiz_get_type (ap_hdl, "tizuricfgport_class");
  TIZ_LOG_CLASS (tizuricfgport_class);
  void * tizuricfgport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizuricfgport_class, "tizuricfgport", tizconfigport,
     sizeof (tiz_uricfgport_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, uri_cfgport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, uri_cfgport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, uri_cfgport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, uri_cfgport_SetParameter,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizuricfgport;
}
