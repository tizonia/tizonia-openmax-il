/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   frcfgport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Binary Reader config port implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "frcfgport.h"
#include "frcfgport_decls.h"
#include "tizosal.h"

#include <assert.h>
#include <string.h>
#include <limits.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.file_reader.frcfgport"
#endif


static char *
find_default_uri ()
{
  char *p_rv = NULL;
  const char *p_uri = NULL;

  p_uri = tiz_rcfile_get_value ("plugins-data",
                                "OMX.Aratelia.file_reader.binary.default_audio_uri");

  assert (NULL != p_uri
          &&
          "OMX.Aratelia.file_reader.binary.default_audio_uri not present in tizrc...");

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Default URI [%s]...", p_uri);

  p_rv = strndup (p_uri, PATH_MAX);

  return p_rv;
}


/*
 * frcfgport class
 */

static void *
fr_cfgport_ctor (void *ap_obj, va_list * app)
{
  fr_cfgport_t *p_obj = super_ctor (typeOf (ap_obj, "frcfgport"), ap_obj, app);
  p_obj->p_uri_ = find_default_uri ();
  tiz_port_register_index (p_obj, OMX_IndexParamContentURI);
  return p_obj;
}

static void *
fr_cfgport_dtor (void *ap_obj)
{
  fr_cfgport_t *p_obj = ap_obj;
  tiz_mem_free (p_obj->p_uri_);
  return super_dtor (typeOf (ap_obj, "frcfgport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
fr_cfgport_GetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const fr_cfgport_t *p_obj = ap_obj;

  switch (a_index)
    {
    case OMX_IndexParamContentURI:
      {
        OMX_PARAM_CONTENTURITYPE *p_uri
          = (OMX_PARAM_CONTENTURITYPE *) ap_struct;
        OMX_U32 uri_buf_size =
          p_uri->nSize - sizeof (OMX_U32) - sizeof (OMX_VERSIONTYPE);
        OMX_U32 uri_len = strlen (p_obj->p_uri_);
        TIZ_LOG (TIZ_PRIORITY_TRACE, "uri_buf_size [%d]...", uri_buf_size);
        TIZ_LOG (TIZ_PRIORITY_TRACE, "p_obj->p_uri_ [%08x]...", p_obj->p_uri_);

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
        return super_GetParameter (typeOf (ap_obj, "frcfgport"),
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
fr_cfgport_SetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  fr_cfgport_t *p_obj = (fr_cfgport_t *) ap_obj;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "SetParameter [%s]...", tiz_idx_to_str (a_index));

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

        TIZ_LOG (TIZ_PRIORITY_TRACE, "Set URI [%s]...", p_obj->p_uri_);
      }
      break;

    default:
      {
        /* Delegate to the base port */
        return super_SetParameter (typeOf (ap_obj, "frcfgport"),
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

/*
 * fr_cfgport_class
 */

static void *
fr_cfgport_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "frcfgport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
fr_cfgport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * frcfgport_class = factory_new (classOf (tizconfigport),
                                        "frcfgport_class",
                                        classOf (tizconfigport),
                                        sizeof (fr_cfgport_class_t),
                                        ap_tos, ap_hdl,
                                        ctor, fr_cfgport_class_ctor, 0);
  return frcfgport_class;
}

void *
fr_cfgport_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * frcfgport_class = tiz_get_type (ap_hdl, "frcfgport_class");
  TIZ_LOG_CLASS (frcfgport_class);
  void * frcfgport =
    factory_new
    (frcfgport_class,
     "frcfgport",
     tizconfigport,
     sizeof (fr_cfgport_t),
     ap_tos, ap_hdl,
     ctor, fr_cfgport_ctor,
     dtor, fr_cfgport_dtor,
     tiz_api_GetParameter, fr_cfgport_GetParameter,
     tiz_api_SetParameter, fr_cfgport_SetParameter, 0);

  return frcfgport;
}
