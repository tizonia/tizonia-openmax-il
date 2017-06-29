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
 * @file   tizvideoport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - videoport class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizplatform.h>

#include "tizutils.h"
#include "tizvideoport.h"
#include "tizvideoport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.videoport"
#endif

static OMX_ERRORTYPE
videoport_SetParameter_common (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                               OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_videoport_t * p_obj = (tiz_videoport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "SetParameter [%s]...", tiz_idx_to_str (a_index));
  assert (p_obj);

  switch (a_index)
    {
      case OMX_IndexParamVideoPortFormat:
        {
          const OMX_VIDEO_PARAM_PORTFORMATTYPE * p_video_format
            = (OMX_VIDEO_PARAM_PORTFORMATTYPE *) ap_struct;
          OMX_VIDEO_CODINGTYPE encoding = p_video_format->eCompressionFormat;
          OMX_COLOR_FORMATTYPE format = p_video_format->eColorFormat;

          if (OMX_VIDEO_CodingUnused == p_obj->port_format_.eCompressionFormat)
            {
              if (OMX_COLOR_FormatUnused == p_video_format->eColorFormat)
                {
                  TIZ_ERROR (ap_hdl,
                             "[OMX_ErrorBadParameter] : "
                             "(Both Compression Format and Color can not be "
                             "Unused at the same time)",
                             encoding);
                  return OMX_ErrorBadParameter;
                }

              if (encoding >= OMX_VIDEO_CodingMax)
                {
                  TIZ_ERROR (ap_hdl,
                             "[OMX_ErrorBadParameter] : "
                             "(Bad compression format [0x%08x]...)",
                             encoding);
                  return OMX_ErrorBadParameter;
                }

              if (!tiz_vector_find (p_obj->p_color_formats_, &format))
                {
                  TIZ_ERROR (ap_hdl,
                             "[OMX_ErrorUnsupportedSetting] : "
                             "(Color format not supported [0x%08x]...)",
                             format);
                  return OMX_ErrorUnsupportedSetting;
                }

              p_obj->port_format_.eColorFormat = format;
              p_obj->port_format_.xFramerate = p_video_format->xFramerate;

              TIZ_TRACE (ap_hdl,
                         "Set color format [0x%08x] "
                         "and framerate [0x%08x]...",
                         format, p_video_format->xFramerate);
            }
          else
            {
              if (OMX_COLOR_FormatUnused != p_video_format->eColorFormat)
                {
                  TIZ_ERROR (ap_hdl,
                             "[OMX_ErrorBadParameter] : "
                             "(Expected OMX_COLOR_FormatUnused)");
                  return OMX_ErrorBadParameter;
                }

              if (!tiz_vector_find (p_obj->p_video_encodings_, &encoding))
                {
                  TIZ_ERROR (ap_hdl,
                             "[OMX_ErrorUnsupportedSetting] : "
                             "(Can't find enconding [0x%08x])",
                             encoding);
                  return OMX_ErrorUnsupportedSetting;
                }
              else
                {
                  p_obj->port_format_.eCompressionFormat = encoding;
                  TIZ_TRACE (ap_hdl, "Set video encoding [0x%08x]...",
                             encoding);
                }
            }
        }
        break;

      default:
        {
          /* Try the parent's indexes */
          rc = super_SetParameter (typeOf (ap_obj, "tizvideoport"), ap_obj,
                                   ap_hdl, a_index, ap_struct);
        }
    };

  return rc;
}

static inline OMX_ERRORTYPE
update_video_coding_type (void * ap_obj, const OMX_VIDEO_CODINGTYPE a_encoding)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_videoport_t * p_obj = ap_obj;
  tiz_port_t * p_base = ap_obj;

  assert (ap_obj);

  if (a_encoding >= OMX_VIDEO_CodingMax)
    {
      TIZ_ERROR (handleOf (ap_obj),
                 "[OMX_ErrorBadParameter] : "
                 "(Bad encoding [0x%08x]...)",
                 a_encoding);
      rc = OMX_ErrorBadParameter;
      goto end;
    }

  if (!tiz_vector_find (p_obj->p_video_encodings_, (const OMX_PTR) (&a_encoding)))
    {
      TIZ_ERROR (handleOf (ap_obj),
                 "[OMX_ErrorUnsupportedSetting] : "
                 "(Encoding not supported [0x%08x]...)",
                 a_encoding);
      rc = OMX_ErrorUnsupportedSetting;
      goto end;
    }

  /* All well */

  /* Update this port's OMX_VIDEO_PARAM_PORTFORMATTYPE structure */
  p_obj->port_format_.eCompressionFormat = a_encoding;

  /* Now update the base class' OMX_PARAM_PORTDEFINITIONTYPE */
  p_base->portdef_.format.video.eCompressionFormat = a_encoding;

end:

  return rc;
}

static inline OMX_ERRORTYPE
update_color_format_type (void * ap_obj, const OMX_COLOR_FORMATTYPE a_color_format)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_videoport_t * p_obj = ap_obj;
  tiz_port_t * p_base = ap_obj;

  assert (ap_obj);

  if (a_color_format >= OMX_COLOR_FormatMax)
    {
      TIZ_ERROR (handleOf (ap_obj),
                 "[OMX_ErrorBadParameter] : "
                 "(Bad color format [0x%08x]...)",
                 a_color_format);
      rc = OMX_ErrorBadParameter;
      goto end;
    }

  if (!tiz_vector_find (p_obj->p_color_formats_, (const OMX_PTR) (&a_color_format)))
    {
      TIZ_ERROR (handleOf (ap_obj),
                 "[OMX_ErrorUnsupportedSetting] : "
                 "(Color format not supported [0x%08x]...)",
                 a_color_format);
      rc = OMX_ErrorUnsupportedSetting;
      goto end;
    }

  /* All well */

  /* Update this port's OMX_VIDEO_PARAM_PORTFORMATTYPE structure */
  p_obj->port_format_.eColorFormat = a_color_format;

  /* Now update the base class' OMX_PARAM_PORTDEFINITIONTYPE */
  p_base->portdef_.format.video.eColorFormat = a_color_format;

end:

  return rc;
}

/*
 * tizvideoport class
 */

static void *
videoport_ctor (void * ap_obj, va_list * app)
{
  tiz_videoport_t * p_obj
    = super_ctor (typeOf (ap_obj, "tizvideoport"), ap_obj, app);
  OMX_VIDEO_PORTDEFINITIONTYPE * p_portdef = NULL;
  OMX_VIDEO_CODINGTYPE * p_encodings = NULL;
  OMX_COLOR_FORMATTYPE * p_formats = NULL;
  OMX_U32 i = 0;

  tiz_port_register_index (p_obj, OMX_IndexParamVideoPortFormat);

  tiz_vector_init (&(p_obj->p_video_encodings_), sizeof (OMX_VIDEO_CODINGTYPE));
  tiz_vector_init (&(p_obj->p_color_formats_), sizeof (OMX_COLOR_FORMATTYPE));

  /* Finalize the base's port definition structure */
  if ((p_portdef = va_arg (*app, OMX_VIDEO_PORTDEFINITIONTYPE *)))
    {
      tiz_port_t * p_base = ap_obj;
      p_base->portdef_.format.video.pNativeRender = p_portdef->pNativeRender;
      p_base->portdef_.format.video.nFrameWidth = p_portdef->nFrameWidth;
      p_base->portdef_.format.video.nFrameHeight = p_portdef->nFrameHeight;
      p_base->portdef_.format.video.nStride = p_portdef->nStride;
      p_base->portdef_.format.video.nSliceHeight = p_portdef->nSliceHeight;
      p_base->portdef_.format.video.nBitrate = p_portdef->nBitrate;
      p_base->portdef_.format.video.xFramerate = p_portdef->xFramerate;
      p_base->portdef_.format.video.bFlagErrorConcealment
        = p_portdef->bFlagErrorConcealment;
      p_base->portdef_.format.video.eCompressionFormat
        = p_portdef->eCompressionFormat;
      p_base->portdef_.format.video.eColorFormat = p_portdef->eColorFormat;
      p_base->portdef_.format.video.pNativeWindow = p_portdef->pNativeWindow;
    }

  /* Initialize the OMX_VIDEO_PARAM_PORTFORMATTYPE structure */
  p_obj->port_format_.nSize = sizeof (OMX_VIDEO_PARAM_PORTFORMATTYPE);
  p_obj->port_format_.nVersion.nVersion = OMX_VERSION;
  p_obj->port_format_.nIndex = 0;

  if ((p_encodings = va_arg (*app, OMX_VIDEO_CODINGTYPE *)))
    {
      while (OMX_VIDEO_CodingMax != p_encodings[i])
        {
          TIZ_TRACE (handleOf (ap_obj), "p_encodings[%u] = [%d]...", i,
                     p_encodings[i]);
          tiz_vector_push_back (p_obj->p_video_encodings_, &p_encodings[i++]);
        }
    }
  p_obj->port_format_.eCompressionFormat
    = p_encodings ? p_encodings[0] : OMX_VIDEO_CodingUnused;

  i = 0;
  if ((p_formats = va_arg (*app, OMX_COLOR_FORMATTYPE *)))
    {
      while (OMX_COLOR_FormatMax != p_formats[i])
        {
          TIZ_TRACE (handleOf (ap_obj), "p_formats[%u] = [%d]...", i,
                     p_formats[i]);
          tiz_vector_push_back (p_obj->p_color_formats_, &p_formats[i]);
          i++;
        }
    }
  p_obj->port_format_.eColorFormat
    = p_formats ? p_formats[0] : OMX_COLOR_FormatUnused;
  p_obj->port_format_.xFramerate = 0;

  return p_obj;
}

static void *
videoport_dtor (void * ap_obj)
{
  tiz_videoport_t * p_obj = ap_obj;
  assert (p_obj);

  tiz_vector_clear (p_obj->p_video_encodings_);
  tiz_vector_destroy (p_obj->p_video_encodings_);

  tiz_vector_clear (p_obj->p_color_formats_);
  tiz_vector_destroy (p_obj->p_color_formats_);

  return super_dtor (typeOf (ap_obj, "tizvideoport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
videoport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_videoport_t * p_obj = ap_obj;

  TIZ_TRACE (ap_hdl, "GetParameter [%s] on pid [%d] ...",
             tiz_idx_to_str (a_index), tiz_port_index (ap_obj));
  assert (ap_obj);

  switch (a_index)
    {
      case OMX_IndexParamVideoPortFormat:
        {
          OMX_VIDEO_PARAM_PORTFORMATTYPE * p_pft = ap_struct;
          OMX_VIDEO_CODINGTYPE * p_encoding = NULL;
          OMX_COLOR_FORMATTYPE * p_format = NULL;

          if (OMX_VIDEO_CodingUnused == p_obj->port_format_.eCompressionFormat)
            {
              if (p_pft->nIndex >= tiz_vector_length (p_obj->p_color_formats_))
                {
                  return OMX_ErrorNoMore;
                }

              p_format = tiz_vector_at (p_obj->p_color_formats_, p_pft->nIndex);
              assert (p_format);
              p_pft->eCompressionFormat = OMX_VIDEO_CodingUnused;
              p_pft->eColorFormat = *p_format;

              TIZ_TRACE (ap_hdl, "ColorFormat [%x]...", *p_format);
            }
          else
            {
              if (p_pft->nIndex
                  >= tiz_vector_length (p_obj->p_video_encodings_))
                {
                  return OMX_ErrorNoMore;
                }

              p_encoding
                = tiz_vector_at (p_obj->p_video_encodings_, p_pft->nIndex);
              assert (p_encoding);
              p_pft->eCompressionFormat = *p_encoding;

              TIZ_TRACE (ap_hdl, "CompressionFormat [%x] ", *p_encoding);
            }
          p_pft->xFramerate = p_obj->port_format_.xFramerate;
        }
        break;

      default:
        {
          /* Try the parent's indexes */
          return super_GetParameter (typeOf (ap_obj, "tizvideoport"), ap_obj,
                                     ap_hdl, a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
videoport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_videoport_t * p_obj = (tiz_videoport_t *) ap_obj;

  TIZ_TRACE (ap_hdl, "SetParameter [%s] on pid [%d] ...",
             tiz_idx_to_str (a_index), tiz_port_index (ap_obj));
  assert (p_obj);

  /* Do now allow changes to nFrameWidth or nFrameHeight if this is a slave
   * output port */
  if (OMX_IndexParamPortDefinition == a_index)
    {
      const tiz_port_t * p_base = ap_obj;
      OMX_PARAM_PORTDEFINITIONTYPE * p_portdef = ap_struct;

      if ((OMX_DirOutput == p_base->portdef_.eDir)
          && (p_base->opts_.mos_port != -1)
          && (p_base->opts_.mos_port != p_base->portdef_.nPortIndex)
          && ((p_base->portdef_.format.video.nFrameWidth
               != p_portdef->format.video.nFrameWidth)
              || (p_base->portdef_.format.video.nFrameHeight
                  != p_portdef->format.video.nFrameHeight)))
        {
          TIZ_ERROR (ap_hdl,
                     "[OMX_ErrorBadParameter] : PORT [%d] "
                     "SetParameter [OMX_IndexParamPortDefinition]... "
                     "Slave port, cannot update frame width or height",
                     tiz_port_dir (p_obj));
          assert (0);
          return OMX_ErrorBadParameter;
        }
    }

  return videoport_SetParameter_common (ap_obj, ap_hdl, a_index,
                                        ap_struct);
}

static OMX_ERRORTYPE
videoport_set_portdef_format (void * ap_obj,
                              const OMX_PARAM_PORTDEFINITIONTYPE * ap_pdef)
{
  tiz_videoport_t * p_obj = ap_obj;
  tiz_port_t * p_base = ap_obj;

  assert (p_obj);
  assert (ap_pdef);

  tiz_check_omx (update_video_coding_type (p_obj, ap_pdef->format.video.eCompressionFormat));

  tiz_check_omx (update_color_format_type (p_obj, ap_pdef->format.video.eColorFormat));

  p_base->portdef_.format.video.pNativeRender
    = ap_pdef->format.video.pNativeRender;
  p_base->portdef_.format.video.nFrameWidth = ap_pdef->format.video.nFrameWidth;
  p_base->portdef_.format.video.nFrameHeight
    = ap_pdef->format.video.nFrameHeight;
  p_base->portdef_.format.video.nStride = ap_pdef->format.video.nStride;
  p_base->portdef_.format.video.nSliceHeight
    = ap_pdef->format.video.nSliceHeight;
  p_base->portdef_.format.video.nBitrate = ap_pdef->format.video.nBitrate;
  p_base->portdef_.format.video.xFramerate = ap_pdef->format.video.xFramerate;
  p_base->portdef_.format.video.bFlagErrorConcealment
    = ap_pdef->format.video.bFlagErrorConcealment;

  /* Shadow copy is debatable; perhaps a deep copy would be needed. */
  p_base->portdef_.format.video.pNativeWindow
    = ap_pdef->format.video.pNativeWindow;

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] video.eCompressionFormat [%d]",
             tiz_port_index (ap_obj), ap_pdef->format.video.eCompressionFormat);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
videoport_apply_slaving_behaviour (void * ap_obj, void * ap_mos_port,
                                   const OMX_INDEXTYPE a_index,
                                   const OMX_PTR ap_struct,
                                   tiz_vector_t * ap_changed_idxs)
{
  tiz_videoport_t * p_obj = ap_obj;
  tiz_port_t * p_base = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj != NULL);
  assert (ap_struct != NULL);
  assert (ap_changed_idxs != NULL);

  TIZ_TRACE (handleOf(ap_obj), "slaving_behaviour [%s] on pid [%d] ...",
             tiz_idx_to_str (a_index), tiz_port_index (ap_obj));

  if (OMX_IndexParamPortDefinition == a_index)
    {
      const OMX_PARAM_PORTDEFINITIONTYPE * p_portdef = ap_struct;
      const OMX_U32 new_width = p_portdef->format.video.nFrameWidth;
      const OMX_U32 new_height = p_portdef->format.video.nFrameHeight;
      const OMX_S32 new_stride = p_portdef->format.video.nStride;
      const OMX_U32 new_slice_height = p_portdef->format.video.nSliceHeight;
      const OMX_U32 new_bit_rate = p_portdef->format.video.nBitrate;
      const OMX_U32 new_frame_rate = p_portdef->format.video.xFramerate;
      const OMX_U32 y_sz = new_width * new_height;
      const OMX_U32 u_sz = y_sz / 4;
      const OMX_U32 v_sz = u_sz;
      const OMX_U32 new_buf_sz = y_sz + u_sz + v_sz;
      OMX_BOOL portdef_changed = OMX_FALSE;

      TIZ_TRACE (handleOf (ap_obj),
                 "w[%d] h[%d] y[%d] u[%d] v[%d] ->  new_sz[%d] ", new_width,
                 new_height, y_sz, u_sz, v_sz, new_buf_sz);

      if ((p_base->portdef_.format.video.nFrameWidth != new_width)
          || (p_base->portdef_.format.video.nFrameHeight != new_height)
          || (p_base->portdef_.format.video.nStride != new_stride)
          || (p_base->portdef_.format.video.nSliceHeight != new_slice_height)
          || (p_base->portdef_.format.video.nBitrate != new_bit_rate)
          || (p_base->portdef_.format.video.xFramerate != new_frame_rate))
        {
          TIZ_TRACE (handleOf (ap_obj),
                     "portdef_changed = OMX_TRUE :"
                     "width/height (w[%u] h[%u] - w[%u] h[%u])"
                     "stride/slice height (st[%d] slh[%u] - st[%d] slg[%u])"
                     "bitrate/framerate (br[%d] fr[%u] - br[%d] fr[%u])",
                     p_portdef->format.video.nFrameWidth,
                     p_portdef->format.video.nFrameHeight,
                     p_base->portdef_.format.video.nFrameWidth,
                     p_base->portdef_.format.video.nFrameHeight,
                     p_portdef->format.video.nStride,
                     p_portdef->format.video.nSliceHeight,
                     p_base->portdef_.format.video.nStride,
                     p_base->portdef_.format.video.nSliceHeight,
                     p_portdef->format.video.nBitrate,
                     p_portdef->format.video.xFramerate,
                     p_base->portdef_.format.video.nBitrate,
                     p_base->portdef_.format.video.xFramerate);
          p_base->portdef_.format.video.nFrameWidth = new_width;
          p_base->portdef_.format.video.nFrameHeight = new_height;
          p_base->portdef_.format.video.nStride = new_stride;
          p_base->portdef_.format.video.nSliceHeight = new_slice_height;
          p_base->portdef_.format.video.nBitrate = new_bit_rate;
          p_base->portdef_.format.video.xFramerate = new_frame_rate;
          portdef_changed = OMX_TRUE;
        }

      /* Also update the port's minimum buffer size, if needed */
      if (new_buf_sz != p_base->portdef_.nBufferSize)
        {
          p_base->portdef_.nBufferSize = new_buf_sz;
          portdef_changed = OMX_TRUE;
          TIZ_TRACE (handleOf (ap_obj),
                     "portdef_changed = OMX_TRUE ->  buf_size");
        }

      if (OMX_TRUE == portdef_changed)
        {
          OMX_INDEXTYPE id = OMX_IndexParamPortDefinition;
          tiz_vector_push_back (ap_changed_idxs, &id);
        }

      TIZ_TRACE (handleOf (ap_obj),
                 " original pid [%d] this pid [%d] : [%s] -> "
                 "changed [OMX_IndexParamPortDefinition] nBufferSize [%d]...",
                 p_portdef->nPortIndex, p_base->portdef_.nPortIndex,
                 tiz_idx_to_str (a_index), p_base->portdef_.nBufferSize);
    }

  return rc;
}

static OMX_ERRORTYPE
videoport_SetParameter_internal (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                                 OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] SetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));
  switch (a_index)
    {
    case OMX_IndexParamVideoPortFormat:
      {
        rc = videoport_SetParameter_common (ap_obj, ap_hdl, a_index, ap_struct);
      }
      break;
    default:
      {
        /* Try the parent's indexes */
        rc = super_SetParameter (typeOf (ap_obj, "tizvideoport"), ap_obj,
                                 ap_hdl, a_index, ap_struct);
      }
      break;
    };
  return rc;
}

/*
 * tizvideoport_class
 */

static void *
videoport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizvideoport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_videoport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizvideoport_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizport), "tizvideoport_class", classOf (tizport),
     sizeof (tiz_videoport_class_t),
     /* TIZ_CLASS_COMMENT: class constructor */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: */
     ctor, videoport_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizvideoport_class;
}

void *
tiz_videoport_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizvideoport_class = tiz_get_type (ap_hdl, "tizvideoport_class");
  TIZ_LOG_CLASS (tizvideoport_class);
  void * tizvideoport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizvideoport_class, "tizvideoport", tizport, sizeof (tiz_videoport_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, videoport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, videoport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, videoport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, videoport_SetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_set_portdef_format, videoport_set_portdef_format,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_apply_slaving_behaviour, videoport_apply_slaving_behaviour,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_SetParameter_internal, videoport_SetParameter_internal,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizvideoport;
}
