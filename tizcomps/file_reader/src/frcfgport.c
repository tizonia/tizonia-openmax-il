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

#include <assert.h>
#include <string.h>
#include <limits.h>

#include "frcfgport.h"
#include "frcfgport_decls.h"

#include "tizosal.h"

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

  TIZ_LOG (TIZ_TRACE, "Default URI [%s]...", p_uri);

  p_rv = strndup (p_uri, PATH_MAX);

  return p_rv;
}


/*
 * frcfgport class
 */

static void *
fr_cfgport_ctor (void *ap_obj, va_list * app)
{
  struct frcfgport *p_obj = super_ctor (frcfgport, ap_obj, app);
  p_obj->p_uri_ = find_default_uri ();
  tiz_port_register_index (p_obj, OMX_IndexParamContentURI);
  return p_obj;
}

static void *
fr_cfgport_dtor (void *ap_obj)
{
  struct frcfgport *p_obj = (struct frcfgport *) ap_obj;
  tiz_mem_free (p_obj->p_uri_);
  return super_dtor (frcfgport, ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
fr_cfgport_GetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct frcfgport *p_obj = ap_obj;

  switch (a_index)
    {
    case OMX_IndexParamContentURI:
      {
        OMX_PARAM_CONTENTURITYPE *p_uri
          = (OMX_PARAM_CONTENTURITYPE *) ap_struct;
        OMX_U32 uri_buf_size =
          p_uri->nSize - sizeof (OMX_U32) - sizeof (OMX_VERSIONTYPE);
        OMX_U32 uri_len = strlen (p_obj->p_uri_);
        TIZ_LOG (TIZ_TRACE, "uri_buf_size [%d]...", uri_buf_size);
        TIZ_LOG (TIZ_TRACE, "p_obj->p_uri_ [%08x]...", p_obj->p_uri_);

        if (uri_buf_size < (uri_len + 1))
          {
            return OMX_ErrorBadParameter;
          }

        p_uri->nVersion.nVersion = OMX_VERSION;
        strncpy ((char *) p_uri->contentURI, p_obj->p_uri_, uri_len + 1);
        if (p_uri->contentURI)
          {
            p_uri->contentURI[uri_len] = '\0';
          }

      }
      break;

    default:
      {
        /* Delegate to the base port */
        return super_GetParameter (frcfgport,
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
  struct frcfgport *p_obj = (struct frcfgport *) ap_obj;

  TIZ_LOG (TIZ_TRACE, "SetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {

    case OMX_IndexParamContentURI:
      {
        OMX_PARAM_CONTENTURITYPE *p_uri
          = (OMX_PARAM_CONTENTURITYPE *) ap_struct;
        OMX_U32 uri_size =
          p_uri->nSize - sizeof (OMX_U32) - sizeof (OMX_VERSIONTYPE);

        tiz_mem_free (p_obj->p_uri_);
        p_obj->p_uri_ = tiz_mem_calloc (1, uri_size);
        strncpy (p_obj->p_uri_, (char *) p_uri->contentURI, uri_size);
        if (p_obj->p_uri_)
          {
            p_uri->contentURI[uri_size - 1] = '\0';
          }

        TIZ_LOG (TIZ_TRACE, "Set URI [%s]...", p_obj->p_uri_);
      }
      break;

    default:
      {
        /* Delegate to the base port */
        return super_SetParameter (frcfgport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

/*
 * frcfgport_class
 */

static void *
fr_cfgport_class_ctor (void *ap_obj, va_list * app)
{
  struct frcfgport_class *p_obj = super_ctor (frcfgport_class, ap_obj, app);
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

const void *frcfgport, *frcfgport_class;

void
init_frcfgport (void)
{

  if (!frcfgport_class)
    {
      TIZ_LOG (TIZ_TRACE, "Initializing frcfgport_class...");
      init_tizconfigport ();
      frcfgport_class = factory_new (tizconfigport_class,
                                     "frcfgport_class",
                                     tizconfigport_class,
                                     sizeof (struct frcfgport_class),
                                     ctor, fr_cfgport_class_ctor, 0);

    }

  if (!frcfgport)
    {
      TIZ_LOG (TIZ_TRACE, "Initializing frcfgport...");
      init_tizconfigport ();
      frcfgport =
        factory_new
        (frcfgport_class,
         "frcfgport",
         tizconfigport,
         sizeof (struct frcfgport),
         ctor, fr_cfgport_ctor,
         dtor, fr_cfgport_dtor,
         tiz_api_GetParameter, fr_cfgport_GetParameter,
         tiz_api_SetParameter, fr_cfgport_SetParameter, 0);
    }

}
