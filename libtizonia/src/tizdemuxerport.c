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
 * @file   tizdemuxerport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - demuxerport class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizplatform.h>

#include "tizutils.h"
#include "tizaudioport.h"
#include "tizaudioport_decls.h"
#include "tizvideoport.h"
#include "tizvideoport_decls.h"
#include "tizpcmport.h"
#include "tizvideoport.h"
#include "tizdemuxerport.h"
#include "tizdemuxerport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.demuxerport"
#endif

/* TODO: Remove duplicate. This function already exists in tizaudioport.c */
static inline OMX_ERRORTYPE
update_audio_coding_type (void * ap_obj, const OMX_AUDIO_CODINGTYPE a_encoding)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_demuxerport_t * p_obj = ap_obj;
  tiz_audioport_t *p_audio_port = NULL;
  tiz_port_t * p_base = ap_obj;

  assert (ap_obj);
  p_audio_port = p_obj->p_port_;

  if (a_encoding >= OMX_AUDIO_CodingMax)
    {
      TIZ_ERROR (handleOf (ap_obj),
                 "[OMX_ErrorBadParameter] : "
                 "(Bad encoding [0x%08x]...)",
                 a_encoding);
      rc = OMX_ErrorBadParameter;
      goto end;
    }

  if (!tiz_vector_find (p_audio_port->p_encodings_, (const OMX_PTR) (&a_encoding)))
    {
      TIZ_ERROR (handleOf (ap_obj),
                 "[OMX_ErrorUnsupportedSetting] : "
                 "(Encoding not supported [0x%08x]...)",
                 a_encoding);
      rc = OMX_ErrorUnsupportedSetting;
      goto end;
    }

  /* All well */

  /* Update this port's OMX_AUDIO_PARAM_PORTFORMATTYPE structure */
  p_audio_port->port_format_.eEncoding = a_encoding;

  /* Now update the base class' OMX_PARAM_PORTDEFINITIONTYPE */
  p_base->portdef_.format.audio.eEncoding = a_encoding;

end:

  return rc;
}

/* TODO: Remove duplicate. This function already exists in tizvideoport.c */
static inline OMX_ERRORTYPE
update_video_coding_type (void * ap_obj, const OMX_VIDEO_CODINGTYPE a_encoding)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_demuxerport_t * p_obj = ap_obj;
  tiz_videoport_t * p_video_port = NULL;
  tiz_port_t * p_base = ap_obj;

  assert (ap_obj);
  p_video_port = p_obj->p_port_;

  if (a_encoding >= OMX_VIDEO_CodingMax)
    {
      TIZ_ERROR (handleOf (ap_obj),
                 "[OMX_ErrorBadParameter] : "
                 "(Bad encoding [0x%08x]...)",
                 a_encoding);
      rc = OMX_ErrorBadParameter;
      goto end;
    }

  if (!tiz_vector_find (p_video_port->p_video_encodings_, (const OMX_PTR) (&a_encoding)))
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
  p_video_port->port_format_.eCompressionFormat = a_encoding;

  /* Now update the base class' OMX_PARAM_PORTDEFINITIONTYPE */
  p_base->portdef_.format.video.eCompressionFormat = a_encoding;

end:

  return rc;
}

/* TODO: Remove duplicate. This function already exists in tizvideoport.c */
static inline OMX_ERRORTYPE
update_color_format_type (void * ap_obj, const OMX_COLOR_FORMATTYPE a_color_format)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_demuxerport_t * p_obj = ap_obj;
  tiz_videoport_t * p_video_port = NULL;
  tiz_port_t * p_base = ap_obj;

  assert (ap_obj);
  p_video_port = p_obj->p_port_;

  if (a_color_format >= OMX_COLOR_FormatMax)
    {
      TIZ_ERROR (handleOf (ap_obj),
                 "[OMX_ErrorBadParameter] : "
                 "(Bad color format [0x%08x]...)",
                 a_color_format);
      rc = OMX_ErrorBadParameter;
      goto end;
    }

  if (!tiz_vector_find (p_video_port->p_color_formats_, (const OMX_PTR) (&a_color_format)))
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
  p_video_port->port_format_.eColorFormat = a_color_format;

  /* Now update the base class' OMX_PARAM_PORTDEFINITIONTYPE */
  p_base->portdef_.format.video.eColorFormat = a_color_format;

end:

  return rc;
}

/*
 * tizdemuxerport class
 */

static void *
demuxerport_ctor (void * ap_obj, va_list * app)
{
  tiz_demuxerport_t * p_obj = NULL;
  tiz_port_options_t * p_opts = NULL;
  va_list app_copy;

  /* Make a copy of the incoming va_list before it gets parsed by the parent
     class:
     The expected arguments are:
     tiz_port_options_t

     */
  va_copy (app_copy, *app);

  /* Now give the original to the base class */
  if ((p_obj = super_ctor (typeOf (ap_obj, "tizdemuxerport"), ap_obj, app)))
    {
      /* Register the demuxer-specific indexes  */
      tiz_check_omx_ret_null (
        tiz_port_register_index (p_obj, OMX_IndexParamNumAvailableStreams));
      tiz_check_omx_ret_null (
        tiz_port_register_index (p_obj, OMX_IndexParamActiveStream));

      /* Grab the port options structure (mandatory argument) */
      p_opts = va_arg (app_copy, tiz_port_options_t *);
      assert (p_opts);

      TIZ_TRACE (handleOf (ap_obj), "min_buf_size [%d]", p_opts->min_buf_size);

      switch (p_opts->domain)
        {
          case OMX_PortDomainAudio:
            {
              /* Let's instantiate a PCM port */
              OMX_AUDIO_CODINGTYPE * p_encodings = NULL;

              /* Get the array of OMX_AUDIO_CODINGTYPE values (mandatory argument) */
              p_encodings = va_arg (app_copy, OMX_AUDIO_CODINGTYPE *);
              assert (p_encodings);

              p_obj->p_port_
                = factory_new (typeOf (ap_obj, "tizaudioport"), p_opts,
                               p_encodings);
              if (!p_obj->p_port_)
                {
                  return NULL;
                }
            }
            break;

          case OMX_PortDomainVideo:
            {
              OMX_VIDEO_PORTDEFINITIONTYPE * p_portdef = NULL;
              OMX_VIDEO_CODINGTYPE * p_encodings = NULL;
              OMX_COLOR_FORMATTYPE * p_formats = NULL;

              /* Register the raw video port indexes, so this port receives the
               get/set requests */
              tiz_check_omx_ret_null (
                tiz_port_register_index (p_obj, OMX_IndexParamVideoPortFormat));

              /* Get the OMX_VIDEO_PORTDEFINITIONTYPE structure (mandatory argument) */
              p_portdef = va_arg (app_copy, OMX_VIDEO_PORTDEFINITIONTYPE *);
              assert (p_portdef);

              /* Get the array of OMX_VIDEO_CODINGTYPE values (mandatory argument) */
              p_encodings = va_arg (app_copy, OMX_VIDEO_CODINGTYPE *);
              assert (p_encodings);

              /* Get the array of OMX_COLOR_FORMATTYPE values (mandatory argument) */
              p_formats = va_arg (app_copy, OMX_COLOR_FORMATTYPE *);
              assert (p_formats);

              if (NULL == (p_obj->p_port_ = factory_new (
                             typeOf (ap_obj, "tizvideoport"), p_opts, p_portdef,
                             p_encodings, p_formats)))
                {
                  return NULL;
                }
            }
            break;

          default:
            assert (0);
        };
    }
  va_end (app_copy);

  return p_obj;
}

static void *
demuxerport_dtor (void * ap_obj)
{
  tiz_demuxerport_t * p_obj = ap_obj;
  assert (p_obj);
  factory_delete (p_obj->p_port_);
  return super_dtor (typeOf (ap_obj, "tizdemuxerport"), ap_obj);
}

/*
 * from tiz_api
 */
static OMX_ERRORTYPE
demuxerport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                          OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_demuxerport_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] GetParameter [%s]",
             tiz_port_index (ap_obj), tiz_idx_to_str (a_index));
  assert (ap_obj);

  switch (a_index)
    {
      case OMX_IndexParamNumAvailableStreams:
      case OMX_IndexParamActiveStream:
        {
          /* Only the processor knows about available or active streams. So lets
           get the processor to fill this info for us. */
          void * p_prc = tiz_get_prc (ap_hdl);
          assert (p_prc);
          if (OMX_ErrorNone != (rc = tiz_api_GetParameter (p_prc, ap_hdl,
                                                           a_index, ap_struct)))
            {
              TIZ_ERROR (ap_hdl,
                         "[%s] : Error retrieving [%s] "
                         "from the processor",
                         tiz_err_to_str (rc), tiz_idx_to_str (a_index));
              return rc;
            }
        }
        break;

      case OMX_IndexParamAudioPortFormat:
      case OMX_IndexParamVideoPortFormat:
        {
          /* Delegate to the domain-specific port */
          if (OMX_ErrorUnsupportedIndex
              != (rc = tiz_api_GetParameter (p_obj->p_port_, ap_hdl, a_index,
                                             ap_struct)))
            {
              return rc;
            }
        }
      /* NOTE: Fall through if GetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
      /*@fallthrough@*/
      default:
        {
          /* Delegate to the base port */
          return super_GetParameter (typeOf (ap_obj, "tizdemuxerport"), ap_obj,
                                     ap_hdl, a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
demuxerport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                          OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_demuxerport_t * p_obj = (tiz_demuxerport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] SetParameter [%s]",
             tiz_port_index (ap_obj), tiz_idx_to_str (a_index));
  assert (ap_obj);

  switch (a_index)
    {
      case OMX_IndexParamNumAvailableStreams:
        {
          /* This is a read-only index. Simply ignore it. */
          TIZ_NOTICE (ap_hdl, "Ignoring read-only index [%s] ",
                      tiz_idx_to_str (a_index));
        }
        break;

      case OMX_IndexParamActiveStream:
        {
          /* Only the processor knows about active streams. So lets
           get the processor update this info for us. */
          void * p_prc = tiz_get_prc (ap_hdl);
          assert (p_prc);
          if (OMX_ErrorUnsupportedIndex
              != (rc
                  = tiz_api_SetParameter (p_prc, ap_hdl, a_index, ap_struct)))
            {
              TIZ_ERROR (ap_hdl,
                         "[%s] : Error setting [%s] "
                         "on the processor",
                         tiz_err_to_str (rc), tiz_idx_to_str (a_index));
              return rc;
            }
        }
        break;

      case OMX_IndexParamAudioPortFormat:
      case OMX_IndexParamVideoPortFormat:
        {
          /* Delegate to the domain-specific port */
          assert (p_obj->p_port_);
          if (OMX_ErrorUnsupportedIndex
              != (rc = tiz_api_SetParameter (p_obj->p_port_, ap_hdl, a_index,
                                             ap_struct)))
            {
              return rc;
            }
        }

      /* NOTE: Fall through if SetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
      /*@fallthrough@*/
      default:
        {
          /* Delegate to the base port */
          TIZ_DEBUG(ap_hdl, "delegating to base port");
          return super_SetParameter (typeOf (ap_obj, "tizdemuxerport"), ap_obj,
                                     ap_hdl, a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
demuxerport_GetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                       OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_demuxerport_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] GetConfig [%s]",
             tiz_port_index (ap_obj), tiz_idx_to_str (a_index));
  assert (ap_obj);

  switch (a_index)
    {
      case OMX_IndexConfigAudioVolume:
      case OMX_IndexConfigAudioMute:
        {
          /* Delegate to the domain-specific port */
          if (OMX_ErrorUnsupportedIndex
              != (rc = tiz_api_GetConfig (p_obj->p_port_, ap_hdl, a_index,
                                          ap_struct)))
            {
              return rc;
            }
        }

      /* NOTE: Fall through if GetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
      /*@fallthrough@*/
      default:
        {
          /* Try the parent's indexes */
          rc = super_GetConfig (typeOf (ap_obj, "tizdemuxerport"), ap_obj,
                                ap_hdl, a_index, ap_struct);
        }
    };

  return rc;
}

static OMX_ERRORTYPE
demuxerport_SetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                       OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_demuxerport_t * p_obj = (tiz_demuxerport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] SetConfig [%s]",
             tiz_port_index (ap_obj), tiz_idx_to_str (a_index));
  assert (ap_obj);

  switch (a_index)
    {
      case OMX_IndexConfigAudioVolume:
      case OMX_IndexConfigAudioMute:
        {
          /* TODO: Delegate this to the processor */
          if (OMX_ErrorUnsupportedIndex
              != (rc = tiz_api_SetConfig (p_obj->p_port_, ap_hdl, a_index,
                                          ap_struct)))
            {
              return rc;
            }
        }

      /* NOTE: Fall through if GetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
      /*@fallthrough@*/
      default:
        {
          /* Try the parent's indexes */
          rc = super_SetConfig (typeOf (ap_obj, "tizdemuxerport"), ap_obj,
                                ap_hdl, a_index, ap_struct);
        }
    };

  return rc;
}

static OMX_ERRORTYPE
demuxerport_set_portdef_format (void * ap_obj,
                              const OMX_PARAM_PORTDEFINITIONTYPE * ap_pdef)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_demuxerport_t * p_obj = ap_obj;
  tiz_port_t * p_base = ap_obj;

  assert (p_obj);
  assert (ap_pdef);

  if (OMX_PortDomainAudio == p_base->portdef_.eDomain)
    {
      tiz_check_omx (update_audio_coding_type (p_obj, ap_pdef->format.audio.eEncoding));
      p_base->portdef_.format.audio.pNativeRender
        = ap_pdef->format.audio.pNativeRender;
      p_base->portdef_.format.audio.bFlagErrorConcealment
        = ap_pdef->format.audio.bFlagErrorConcealment;
      TIZ_TRACE (handleOf (ap_obj), "PORT [%d] audio.eEncoding [%d]",
                 tiz_port_index (ap_obj), ap_pdef->format.audio.eEncoding);
    }
  else if (OMX_PortDomainVideo == p_base->portdef_.eDomain)
    {
      tiz_check_omx (update_video_coding_type (
        p_obj, ap_pdef->format.video.eCompressionFormat));

      tiz_check_omx (
        update_color_format_type (p_obj, ap_pdef->format.video.eColorFormat));

      p_base->portdef_.format.video.pNativeRender
        = ap_pdef->format.video.pNativeRender;
      p_base->portdef_.format.video.nFrameWidth
        = ap_pdef->format.video.nFrameWidth;
      p_base->portdef_.format.video.nFrameHeight
        = ap_pdef->format.video.nFrameHeight;
      p_base->portdef_.format.video.nStride = ap_pdef->format.video.nStride;
      p_base->portdef_.format.video.nSliceHeight
        = ap_pdef->format.video.nSliceHeight;
      p_base->portdef_.format.video.nBitrate = ap_pdef->format.video.nBitrate;
      p_base->portdef_.format.video.xFramerate
        = ap_pdef->format.video.xFramerate;
      p_base->portdef_.format.video.bFlagErrorConcealment
        = ap_pdef->format.video.bFlagErrorConcealment;

      /* Shadow copy is debatable; perhaps a deep copy would be needed. */
      p_base->portdef_.format.video.pNativeWindow
        = ap_pdef->format.video.pNativeWindow;

      TIZ_TRACE (handleOf (ap_obj), "PORT [%d] video.eCompressionFormat [%d]",
                 tiz_port_index (ap_obj),
                 ap_pdef->format.video.eCompressionFormat);
    }
  else
    {
      assert (0);
    }
  return rc;
}

static bool
demuxerport_check_tunnel_compat (const void * ap_obj,
                                 OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                                 OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t * p_obj = (tiz_port_t *) ap_obj;
  assert (ap_this_def);
  assert (ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_ERROR (handleOf (ap_obj),
                 "PORT [%d] check_tunnel_compat : Different domain found [%d]",
                 p_obj->pid_, ap_other_def->eDomain);
      return false;
    }

  return true;
}

/*
 * tizdemuxerport_class
 */

static void *
demuxerport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizdemuxerport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_demuxerport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizdemuxerport_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizport), "tizdemuxerport_class", classOf (tizport),
     sizeof (tiz_demuxerport_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, demuxerport_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizdemuxerport_class;
}

void *
tiz_demuxerport_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizdemuxerport_class = tiz_get_type (ap_hdl, "tizdemuxerport_class");
  TIZ_LOG_CLASS (tizdemuxerport_class);
  void * tizdemuxerport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizdemuxerport_class, "tizdemuxerport", tizport,
     sizeof (tiz_demuxerport_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, demuxerport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, demuxerport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, demuxerport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, demuxerport_SetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetConfig, demuxerport_GetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetConfig, demuxerport_SetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_set_portdef_format, demuxerport_set_portdef_format,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_check_tunnel_compat, demuxerport_check_tunnel_compat,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizdemuxerport;
}
