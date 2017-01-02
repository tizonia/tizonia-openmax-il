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
 * @file   tizbinaryport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - binaryport class implementation
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
#include "tizvideoport.h"
#include "tizimageport.h"
#include "tizotherport.h"

#include "tizbinaryport.h"
#include "tizbinaryport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.binaryport"
#endif

/*
 * tizbinaryport class
 */

static void *
binaryport_ctor (void * ap_obj, va_list * app)
{
  tiz_binaryport_t * p_obj = NULL;
  tiz_port_options_t * p_opts = NULL;
  va_list app_copy;

  assert (ap_obj);

  /* Make a copy of the incoming va_list before it gets parsed by the parent
     class:
     The expected arguments are:
     port_opts

     */
  va_copy (app_copy, *app);

  /* Now give the original to the base class */
  if (NULL
      == (p_obj = super_ctor (typeOf (ap_obj, "tizbinaryport"), ap_obj, app)))
    {
      return NULL;
    }

  /* Now, grab the port options structure */
  p_opts = va_arg (app_copy, tiz_port_options_t *);
  assert (p_opts);

  switch (p_opts->domain)
    {
      case OMX_PortDomainAudio:
        {
          OMX_AUDIO_CODINGTYPE encodings[]
            = {OMX_AUDIO_CodingUnused, OMX_AUDIO_CodingMax};

          tiz_check_omx_ret_null (
            tiz_port_register_index (p_obj, OMX_IndexParamAudioPortFormat));

          p_obj->p_port_
            = factory_new (typeOf (ap_obj, "tizaudioport"), p_opts, &encodings);
          if (NULL == p_obj->p_port_)
            {
              return NULL;
            }
        }
        break;

      case OMX_PortDomainVideo:
        {
          OMX_VIDEO_PORTDEFINITIONTYPE portdef;
          OMX_VIDEO_CODINGTYPE encodings[]
            = {OMX_VIDEO_CodingUnused, OMX_VIDEO_CodingMax};
          OMX_COLOR_FORMATTYPE formats[]
            = {OMX_COLOR_FormatYUV420Planar, OMX_COLOR_FormatMax};
          /* NOTE: No defaults are defined in the standard for the video output
         * port of the video_reader.binary component. So for the sake of
         * completeness, simply provide some default values here. */
          portdef.pNativeRender = NULL;
          portdef.nFrameWidth = 176;
          portdef.nFrameHeight = 144;
          portdef.nStride = 0;
          portdef.nSliceHeight = 0;
          portdef.nBitrate = 0;
          portdef.xFramerate = 15;
          portdef.bFlagErrorConcealment = OMX_FALSE;
          portdef.eCompressionFormat = OMX_VIDEO_CodingUnused;
          portdef.eColorFormat = OMX_COLOR_FormatYUV420Planar;
          portdef.pNativeWindow = NULL;

          tiz_check_omx_ret_null (
            tiz_port_register_index (p_obj, OMX_IndexParamVideoPortFormat));

          p_obj->p_port_ = factory_new (typeOf (ap_obj, "tizvideoport"), p_opts,
                                        &portdef, &encodings, &formats);
          if (NULL == p_obj->p_port_)
            {
              return NULL;
            }
        }
        break;

      case OMX_PortDomainImage:
        {
          OMX_IMAGE_PORTDEFINITIONTYPE portdef;
          OMX_IMAGE_CODINGTYPE encodings[]
            = {OMX_IMAGE_CodingUnused, OMX_IMAGE_CodingMax};
          OMX_COLOR_FORMATTYPE formats[]
            = {OMX_COLOR_FormatYUV420Planar, OMX_COLOR_FormatMax};
          /* NOTE: No defaults are defined in the standard for the image output
         * port of the image_reader.binary component. So for the sake of
         * completeness, simply provide some default values here. */
          portdef.pNativeRender = NULL;
          portdef.nFrameWidth = 176;
          portdef.nFrameHeight = 144;
          portdef.nStride = 0;
          portdef.nSliceHeight = 0;
          portdef.bFlagErrorConcealment = OMX_FALSE;
          portdef.eCompressionFormat = OMX_IMAGE_CodingUnused;
          portdef.eColorFormat = OMX_COLOR_FormatYUV420Planar;
          portdef.pNativeWindow = NULL;

          tiz_check_omx_ret_null (
            tiz_port_register_index (p_obj, OMX_IndexParamImagePortFormat));

          p_obj->p_port_ = factory_new (typeOf (ap_obj, "tizimageport"), p_opts,
                                        &portdef, &encodings, &formats);
          if (NULL == p_obj->p_port_)
            {
              return NULL;
            }
        }
        break;

      case OMX_PortDomainOther:
        {
          OMX_OTHER_FORMATTYPE formats[]
            = {OMX_OTHER_FormatBinary, OMX_OTHER_FormatMax};

          tiz_check_omx_ret_null (
            tiz_port_register_index (p_obj, OMX_IndexParamOtherPortFormat));

          p_obj->p_port_
            = factory_new (typeOf (ap_obj, "tizotherport"), p_opts, &formats);
          if (NULL == p_obj->p_port_)
            {
              return NULL;
            }
        }
        break;

      default:
        assert (0);
    };

  va_end (app_copy);

  return p_obj;
}

static void *
binaryport_dtor (void * ap_obj)
{
  tiz_binaryport_t * p_obj = ap_obj;
  assert (p_obj);
  factory_delete (p_obj->p_port_);
  return super_dtor (typeOf (ap_obj, "tizbinaryport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
binaryport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_binaryport_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));
  assert (p_obj);

  switch (a_index)
    {
      case OMX_IndexParamAudioPortFormat:
      case OMX_IndexParamVideoPortFormat:
      case OMX_IndexParamImagePortFormat:
      case OMX_IndexParamOtherPortFormat:
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
          rc = super_GetParameter (typeOf (ap_obj, "tizbinaryport"), ap_obj,
                                   ap_hdl, a_index, ap_struct);
        }
    };

  return rc;
}

static OMX_ERRORTYPE
binaryport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_binaryport_t * p_obj = (tiz_binaryport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "PORT [%d] SetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));
  assert (p_obj);

  switch (a_index)
    {
      case OMX_IndexParamAudioPortFormat:
      case OMX_IndexParamVideoPortFormat:
      case OMX_IndexParamImagePortFormat:
      case OMX_IndexParamOtherPortFormat:
        {
          /* Delegate to the domain-specific port */
          if (OMX_ErrorUnsupportedIndex
              != (rc = tiz_api_SetParameter (p_obj->p_port_, ap_hdl, a_index,
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
          rc = super_SetParameter (typeOf (ap_obj, "tizbinaryport"), ap_obj,
                                   ap_hdl, a_index, ap_struct);
        }
    };

  return rc;
}

static OMX_ERRORTYPE
binaryport_set_portdef_format (void * ap_obj,
                              const OMX_PARAM_PORTDEFINITIONTYPE * ap_pdef)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_binaryport_t * p_obj = ap_obj;
  assert (p_obj);
  assert (ap_pdef);
  return rc;
}

static bool
binaryport_check_tunnel_compat (const void * ap_obj,
                                OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                                OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t * p_obj = (tiz_port_t *) ap_obj;

  assert (ap_this_def);
  assert (ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_ERROR (handleOf (ap_obj),
                 "port [%d] check_tunnel_compat : Different domain found [%d]",
                 p_obj->pid_, ap_other_def->eDomain);
      return false;
    }

  return true;
}

/*
 * tizbinaryport_class
 */

static void *
binaryport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizbinaryport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_binaryport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizbinaryport_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizport), "tizbinaryport_class", classOf (tizport),
     sizeof (tiz_binaryport_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, binaryport_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizbinaryport_class;
}

void *
tiz_binaryport_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizbinaryport_class = tiz_get_type (ap_hdl, "tizbinaryport_class");
  TIZ_LOG_CLASS (tizbinaryport_class);
  void * tizbinaryport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizbinaryport_class, "tizbinaryport", tizport, sizeof (tiz_binaryport_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, binaryport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, binaryport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, binaryport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, binaryport_SetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_set_portdef_format, binaryport_set_portdef_format,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_check_tunnel_compat, binaryport_check_tunnel_compat,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizbinaryport;
}
