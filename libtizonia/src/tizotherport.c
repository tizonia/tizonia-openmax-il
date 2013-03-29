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
 * @file   tizotherport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - otherport class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizotherport.h"
#include "tizotherport_decls.h"

#include "tizosal.h"
#include "tizutils.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.otherport"
#endif

/*
 * tizotherport class
 */

static void *
otherport_ctor (void *ap_obj, va_list * app)
{
  struct tizotherport *p_obj = super_ctor (tizotherport, ap_obj, app);
  OMX_OTHER_FORMATTYPE *p_formats = NULL;
  OMX_U32 i = 0;

  tiz_port_register_index (p_obj, OMX_IndexParamOtherPortFormat);

  tiz_vector_init (&(p_obj->p_formats_), sizeof (OMX_OTHER_FORMATTYPE));

  /* Initialize the OMX_OTHER_PARAM_PORTFORMATTYPE structure */
  p_obj->port_format_.nSize = sizeof (OMX_OTHER_PARAM_PORTFORMATTYPE);
  p_obj->port_format_.nVersion.nVersion = OMX_VERSION;
  p_obj->port_format_.nIndex = 0;

  if (NULL != (p_formats = va_arg (*app, OMX_OTHER_FORMATTYPE *)))
    {
      while (OMX_OTHER_FormatMax != p_formats[i])
        {
          TIZ_LOG (TIZ_TRACE, "p_formats[%u] = [%d]...", i, p_formats[i]);
          tiz_vector_push_back (p_obj->p_formats_, &p_formats[i++]);
        }
    }

  p_obj->port_format_.eFormat = p_formats ? p_formats[0] : OMX_OTHER_FormatMax;

  return p_obj;
}

static void *
otherport_dtor (void *ap_obj)
{
  struct tizotherport *p_obj = ap_obj;
  assert (p_obj);

  tiz_vector_clear (p_obj->p_formats_);
  tiz_vector_destroy (p_obj->p_formats_);

  return super_dtor (tizotherport, ap_obj);
}

/*
 * from tizapi
 */

static OMX_ERRORTYPE
otherport_GetParameter (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tizotherport *p_obj = ap_obj;

  TIZ_LOG (TIZ_TRACE, "GetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamOtherPortFormat:
      {
        OMX_OTHER_PARAM_PORTFORMATTYPE *p_pft = ap_struct;
        OMX_OTHER_FORMATTYPE *p_format = NULL;

        if (p_pft->nIndex >= tiz_vector_length (p_obj->p_formats_))
          {
            return OMX_ErrorNoMore;
          }

        p_format = tiz_vector_at (p_obj->p_formats_, p_pft->nIndex);
        assert (p_format && *p_format);
        p_pft->eFormat = *p_format;
        TIZ_LOG (TIZ_TRACE, "Format [0x%08x]...", *p_format);
      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_GetParameter (tizotherport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
otherport_SetParameter (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  struct tizotherport *p_obj = (struct tizotherport *) ap_obj;

  TIZ_LOG (TIZ_TRACE, "SetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamOtherPortFormat:
      {

        const OMX_OTHER_PARAM_PORTFORMATTYPE *p_other_format
          = (OMX_OTHER_PARAM_PORTFORMATTYPE *) ap_struct;
        OMX_OTHER_FORMATTYPE format = p_other_format->eFormat;

        if (format >= OMX_OTHER_FormatMax)
          {
            TIZ_LOG (TIZ_TRACE, "OMX_ErrorBadParameter "
                     "(Bad format [0x%08x]...)", format);
            return OMX_ErrorBadParameter;
          }

        if (!tiz_vector_find (p_obj->p_formats_, &format))
          {
            TIZ_LOG (TIZ_TRACE, "OMX_ErrorUnsupportedSetting "
                     "(Format not supported [0x%08x]...)", format);
            return OMX_ErrorUnsupportedSetting;
          }

        p_obj->port_format_.eFormat = format;

        TIZ_LOG (TIZ_TRACE, "Set new other format " "[0x%08x]...", format);

      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_SetParameter (tizotherport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

/*
 * tizotherport_class
 */

static void *
otherport_class_ctor (void *ap_obj, va_list * app)
{
  struct tizotherport_class *p_obj =
    super_ctor (tizotherport_class, ap_obj, app);
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

const void *tizotherport, *tizotherport_class;

void
init_tizotherport (void)
{

  if (!tizotherport_class)
    {
      init_tizport ();
      tizotherport_class = factory_new (tiz_port_class,
                                        "tizotherport_class",
                                        tiz_port_class,
                                        sizeof (struct tizotherport_class),
                                        ctor, otherport_class_ctor, 0);

    }

  if (!tizotherport)
    {
      init_tizport ();
      tizotherport =
        factory_new
        (tizotherport_class,
         "tizotherport",
         tizport,
         sizeof (struct tizotherport),
         ctor, otherport_ctor,
         dtor, otherport_dtor,
         tiz_api_GetParameter, otherport_GetParameter,
         tiz_api_SetParameter, otherport_SetParameter, 0);
    }

}
