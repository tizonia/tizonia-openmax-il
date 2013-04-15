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
 * @file   tizimageport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - imageport class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizimageport.h"
#include "tizimageport_decls.h"

#include "tizosal.h"
#include "tizutils.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.imageport"
#endif

/*
 * tizimageport class
 */

static void *
imageport_ctor (void *ap_obj, va_list * app)
{
  struct tizimageport *p_obj = super_ctor (tizimageport, ap_obj, app);
  OMX_IMAGE_PORTDEFINITIONTYPE *p_portdef = NULL;
  OMX_IMAGE_CODINGTYPE *p_encodings = NULL;
  OMX_COLOR_FORMATTYPE *p_formats = NULL;
  OMX_U32 i = 0;

  tiz_port_register_index (p_obj, OMX_IndexParamImagePortFormat);

  tiz_vector_init (&(p_obj->p_image_encodings_),
                   sizeof (OMX_IMAGE_CODINGTYPE));
  tiz_vector_init (&(p_obj->p_color_formats_), sizeof (OMX_COLOR_FORMATTYPE));

  /* Finalize the base's port definition structure */
  if (NULL != (p_portdef = va_arg (*app, OMX_IMAGE_PORTDEFINITIONTYPE *)))
    {
      tiz_port_t *p_base = ap_obj;
      p_base->portdef_.format.image.pNativeRender = p_portdef->pNativeRender;
      p_base->portdef_.format.image.nFrameWidth = p_portdef->nFrameWidth;
      p_base->portdef_.format.image.nFrameHeight = p_portdef->nFrameHeight;
      p_base->portdef_.format.image.nStride = p_portdef->nStride;
      p_base->portdef_.format.image.nSliceHeight = p_portdef->nSliceHeight;
      p_base->portdef_.format.image.bFlagErrorConcealment =
        p_portdef->bFlagErrorConcealment;
      p_base->portdef_.format.image.eCompressionFormat =
        p_portdef->eCompressionFormat;
      p_base->portdef_.format.image.eColorFormat = p_portdef->eColorFormat;
      p_base->portdef_.format.image.pNativeWindow = p_portdef->pNativeWindow;
    }

  /* Initialize the OMX_IMAGE_PARAM_PORTFORMATTYPE structure */
  p_obj->port_format_.nSize = sizeof (OMX_IMAGE_PARAM_PORTFORMATTYPE);
  p_obj->port_format_.nVersion.nVersion = OMX_VERSION;
  p_obj->port_format_.nIndex = 0;

  if (NULL != (p_encodings = va_arg (*app, OMX_IMAGE_CODINGTYPE *)))
    {
      while (OMX_IMAGE_CodingMax != p_encodings[i])
        {
          TIZ_LOG (TIZ_TRACE, "p_encodings[%u] = [%d]...",
                   i, p_encodings[i]);
          tiz_vector_push_back (p_obj->p_image_encodings_, &p_encodings[i++]);
        }
    }

  i = 0;
  if (NULL != (p_formats = va_arg (*app, OMX_COLOR_FORMATTYPE *)))
    {
      while (OMX_COLOR_FormatMax != p_formats[i])
        {
          TIZ_LOG (TIZ_TRACE, "p_formats[%u] = [%d]...", i, p_formats[i]);
          tiz_vector_push_back (p_obj->p_color_formats_, &p_formats[i++]);
        }
    }

  p_obj->port_format_.eCompressionFormat = p_encodings
    ? p_encodings[0] : OMX_IMAGE_CodingUnused;
  p_obj->port_format_.eColorFormat = p_formats
    ? p_formats[0] : OMX_COLOR_FormatUnused;

  return p_obj;
}

static void *
imageport_dtor (void *ap_obj)
{
  struct tizimageport *p_obj = ap_obj;
  assert (p_obj);

  tiz_vector_clear (p_obj->p_image_encodings_);
  tiz_vector_destroy (p_obj->p_image_encodings_);

  tiz_vector_clear (p_obj->p_color_formats_);
  tiz_vector_destroy (p_obj->p_color_formats_);

  return super_dtor (tizimageport, ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
imageport_GetParameter (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tizimageport *p_obj = ap_obj;

  TIZ_LOG (TIZ_TRACE, "GetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamImagePortFormat:
      {
        OMX_IMAGE_PARAM_PORTFORMATTYPE *p_pft = ap_struct;
        OMX_IMAGE_CODINGTYPE *p_encoding = NULL;
        OMX_COLOR_FORMATTYPE *p_format = NULL;

        if (OMX_IMAGE_CodingUnused == p_obj->port_format_.eCompressionFormat)
          {
            if (p_pft->nIndex >= tiz_vector_length (p_obj->p_color_formats_))
              {
                return OMX_ErrorNoMore;
              }

            p_format = tiz_vector_at (p_obj->p_color_formats_, p_pft->nIndex);
            assert (p_format);
            p_pft->eCompressionFormat = OMX_IMAGE_CodingUnused;
            p_pft->eColorFormat = *p_format;

            TIZ_LOG (TIZ_TRACE, "ColorFormat [%x]...", *p_format);
          }
        else
          {
            if (p_pft->nIndex >= tiz_vector_length (p_obj->p_image_encodings_))
              {
                return OMX_ErrorNoMore;
              }

            p_encoding =
              tiz_vector_at (p_obj->p_image_encodings_, p_pft->nIndex);
            assert (p_encoding);
            p_pft->eCompressionFormat = *p_encoding;

            TIZ_LOG (TIZ_TRACE, "CompressionFormat [%x] ", *p_encoding);
          }

      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_GetParameter (tizimageport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
imageport_SetParameter (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  struct tizimageport *p_obj = (struct tizimageport *) ap_obj;

  TIZ_LOG (TIZ_TRACE, "SetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamImagePortFormat:
      {

        const OMX_IMAGE_PARAM_PORTFORMATTYPE *p_image_format
          = (OMX_IMAGE_PARAM_PORTFORMATTYPE *) ap_struct;
        OMX_IMAGE_CODINGTYPE encoding = p_image_format->eCompressionFormat;
        OMX_COLOR_FORMATTYPE format = p_image_format->eColorFormat;

        if (OMX_IMAGE_CodingUnused == p_obj->port_format_.eCompressionFormat)
          {
            if (OMX_COLOR_FormatUnused == p_image_format->eColorFormat)
              {
                /* Both Compression Format and Color can not be Unused at the
                 * same time. */
                return OMX_ErrorBadParameter;
              }

            if (encoding >= OMX_IMAGE_CodingMax)
              {
                TIZ_LOG (TIZ_TRACE, "OMX_ErrorBadParameter "
                         "(Bad compression format [0x%08x]...)", encoding);
                return OMX_ErrorBadParameter;
              }

            if (!tiz_vector_find (p_obj->p_color_formats_, &format))
              {
                TIZ_LOG (TIZ_TRACE, "OMX_ErrorUnsupportedSetting "
                         "(Color format not supported [0x%08x]...)", format);
                return OMX_ErrorUnsupportedSetting;
              }

            p_obj->port_format_.eColorFormat = format;
            TIZ_LOG (TIZ_TRACE, "Set new color format "
                     "[0x%08x]...", format);
          }
        else
          {
            if (OMX_COLOR_FormatUnused != p_image_format->eColorFormat)
              {
                return OMX_ErrorBadParameter;
              }

            if (!tiz_vector_find (p_obj->p_image_encodings_, &encoding))
              {
                return OMX_ErrorUnsupportedSetting;
              }
            else
              {
                p_obj->port_format_.eCompressionFormat = encoding;
                TIZ_LOG (TIZ_TRACE, "Set new image encoding "
                         "[0x%08x]...", encoding);
              }
          }

      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_SetParameter (tizimageport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

/*
 * tizimageport_class
 */

static void *
imageport_class_ctor (void *ap_obj, va_list * app)
{
  struct tizimageport_class *p_obj =
    super_ctor (tizimageport_class, ap_obj, app);
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

const void *tizimageport, *tizimageport_class;

void
init_tizimageport (void)
{

  if (!tizimageport_class)
    {
      tiz_port_init ();
      tizimageport_class = factory_new (tiz_port_class,
                                        "tizimageport_class",
                                        tiz_port_class,
                                        sizeof (struct tizimageport_class),
                                        ctor, imageport_class_ctor, 0);

    }

  if (!tizimageport)
    {
      tiz_port_init ();
      tizimageport =
        factory_new
        (tizimageport_class,
         "tizimageport",
         tizport,
         sizeof (struct tizimageport),
         ctor, imageport_ctor,
         dtor, imageport_dtor,
         tiz_api_GetParameter, imageport_GetParameter,
         tiz_api_SetParameter, imageport_SetParameter, 0);
    }

}
