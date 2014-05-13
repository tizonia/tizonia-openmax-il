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

#include <tizplatform.h>

#include "tizutils.h"
#include "tizimageport.h"
#include "tizimageport_decls.h"

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
  tiz_imageport_t *p_obj = super_ctor (typeOf (ap_obj, "tizimageport"), ap_obj, app);
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
          tiz_vector_push_back (p_obj->p_image_encodings_, &p_encodings[i++]);
        }
    }

  i = 0;
  if (NULL != (p_formats = va_arg (*app, OMX_COLOR_FORMATTYPE *)))
    {
      while (OMX_COLOR_FormatMax != p_formats[i])
        {
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
  tiz_imageport_t *p_obj = ap_obj;
  assert (p_obj);

  tiz_vector_clear (p_obj->p_image_encodings_);
  tiz_vector_destroy (p_obj->p_image_encodings_);

  tiz_vector_clear (p_obj->p_color_formats_);
  tiz_vector_destroy (p_obj->p_color_formats_);

  return super_dtor (typeOf (ap_obj, "tizimageport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
imageport_GetParameter (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_imageport_t *p_obj = ap_obj;

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...",
            tiz_port_index (ap_obj), tiz_idx_to_str (a_index));
  assert (NULL != p_obj);

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

            TIZ_TRACE (ap_hdl, "ColorFormat [%x]...", *p_format);
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

            TIZ_TRACE (ap_hdl, "CompressionFormat [%x] ", *p_encoding);
          }

      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_GetParameter (typeOf (ap_obj, "tizimageport"),
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
  tiz_imageport_t *p_obj = (tiz_imageport_t *) ap_obj;

  TIZ_TRACE (ap_hdl, "PORT [%d] SetParameter [%s]...",
            tiz_port_index (ap_obj), tiz_idx_to_str (a_index));
  assert (NULL != p_obj);

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
                TIZ_ERROR (ap_hdl, "[OMX_ErrorBadParameter] : "
                         "(Bad compression format [0x%08x]...)", encoding);
                return OMX_ErrorBadParameter;
              }

            if (!tiz_vector_find (p_obj->p_color_formats_, &format))
              {
                TIZ_ERROR (ap_hdl,
                          "[OMX_ErrorUnsupportedSetting] : "
                          "(Color format not supported [0x%08x]...)", format);
                return OMX_ErrorUnsupportedSetting;
              }

            p_obj->port_format_.eColorFormat = format;
            TIZ_TRACE (ap_hdl, "Set new color format "
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
                TIZ_TRACE (ap_hdl, "Set new image encoding "
                          "[0x%08x]...", encoding);
              }
          }

      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_SetParameter (typeOf (ap_obj, "tizimageport"),
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
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizimageport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_imageport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizimageport_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizport), "tizimageport_class", classOf (tizport), sizeof (tiz_imageport_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, imageport_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizimageport_class;
}

void *
tiz_imageport_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizimageport_class = tiz_get_type (ap_hdl, "tizimageport_class");
  TIZ_LOG_CLASS (tizimageport_class);
  void * tizimageport =
    factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizimageport_class, "tizimageport", tizport, sizeof (tiz_imageport_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, imageport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, imageport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, imageport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, imageport_SetParameter,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizimageport;
}
