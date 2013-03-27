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

#include "tizbinaryport.h"
#include "tizbinaryport_decls.h"

#include "tizaudioport.h"
#include "tizvideoport.h"
#include "tizimageport.h"
#include "tizotherport.h"

#include "tizosal.h"
#include "tizutils.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.binaryport"
#endif

/*
 * tizbinaryport class
 */

static void *
binaryport_ctor (void *ap_obj, va_list * app)
{
  struct tizbinaryport *p_obj = NULL;
  tizport_options_t *p_opts = NULL;
  va_list app_copy;

  va_copy (app_copy, *app);
  p_obj = super_ctor (tizbinaryport, ap_obj, app);

  /* Grab the port options structure */
  p_opts = va_arg (app_copy, tizport_options_t *);
  assert (p_opts);

  switch (p_opts->domain)
    {
    case OMX_PortDomainAudio:
      {
        OMX_AUDIO_CODINGTYPE encodings[] = {
          OMX_AUDIO_CodingUnused,
          OMX_AUDIO_CodingMax
        };
        tizport_register_index (p_obj, OMX_IndexParamAudioPortFormat);
        init_tizaudioport ();
        p_obj->ip_port = factory_new (tizaudioport, p_opts, &encodings);
      }
      break;

    case OMX_PortDomainVideo:
      {
        OMX_VIDEO_PORTDEFINITIONTYPE portdef;
        OMX_VIDEO_CODINGTYPE encodings[] = {
          OMX_VIDEO_CodingUnused,
          OMX_VIDEO_CodingMax
        };
        OMX_COLOR_FORMATTYPE formats[] = {
          OMX_COLOR_FormatYUV420Planar,
          OMX_COLOR_FormatMax
        };
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

        tizport_register_index (p_obj, OMX_IndexParamVideoPortFormat);
        init_tizvideoport ();
        p_obj->ip_port = factory_new (tizvideoport, p_opts, &portdef,
                                      &encodings, &formats);
      }
      break;

    case OMX_PortDomainImage:
      {
        OMX_IMAGE_PORTDEFINITIONTYPE portdef;
        OMX_IMAGE_CODINGTYPE encodings[] = {
          OMX_IMAGE_CodingUnused,
          OMX_IMAGE_CodingMax
        };
        OMX_COLOR_FORMATTYPE formats[] = {
          OMX_COLOR_FormatYUV420Planar,
          OMX_COLOR_FormatMax
        };
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

        tizport_register_index (p_obj, OMX_IndexParamImagePortFormat);
        init_tizimageport ();
        p_obj->ip_port = factory_new (tizimageport, p_opts, &portdef,
                                      &encodings, &formats);
      }
      break;

    case OMX_PortDomainOther:
      {
        OMX_OTHER_FORMATTYPE formats[] = {
          OMX_OTHER_FormatBinary,
          OMX_OTHER_FormatMax
        };

        tizport_register_index (p_obj, OMX_IndexParamOtherPortFormat);
        init_tizotherport ();
        p_obj->ip_port = factory_new (tizotherport, p_opts, &formats);
      }
      break;

    default:
      assert (0);
    };

  va_end (app_copy);

  return p_obj;
}

static void *
binaryport_dtor (void *ap_obj)
{
  struct tizbinaryport *p_obj = ap_obj;
  assert (p_obj);
  factory_delete (p_obj->ip_port);
  return super_dtor (tizbinaryport, ap_obj);
}

/*
 * from tizapi
 */

static OMX_ERRORTYPE
binaryport_GetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tizbinaryport *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_LOG (TIZ_LOG_TRACE, "GetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamAudioPortFormat:
    case OMX_IndexParamVideoPortFormat:
    case OMX_IndexParamImagePortFormat:
    case OMX_IndexParamOtherPortFormat:
      {
        /* Delegate to the domain-specific port */
        if (OMX_ErrorUnsupportedIndex
            != (rc = tizapi_GetParameter (p_obj->ip_port,
                                          ap_hdl, a_index, ap_struct)))
          {
            return rc;
          }
      }
      /* NOTE: Fall through if GetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
    default:
      {
        /* Delegate to the base port */
        return super_GetParameter (tizbinaryport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
binaryport_SetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  struct tizbinaryport *p_obj = (struct tizbinaryport *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_LOG (TIZ_LOG_TRACE, "GetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamAudioPortFormat:
    case OMX_IndexParamVideoPortFormat:
    case OMX_IndexParamImagePortFormat:
    case OMX_IndexParamOtherPortFormat:
      {
        /* Delegate to the domain-specific port */
        if (OMX_ErrorUnsupportedIndex
            != (rc = tizapi_SetParameter (p_obj->ip_port,
                                          ap_hdl, a_index, ap_struct)))
          {
            return rc;
          }
      }

      /* NOTE: Fall through if GetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
    default:
      {
        /* Delegate to the base port */
        return super_SetParameter (tizbinaryport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

static OMX_BOOL
  binaryport_check_tunnel_compat
  (const void *ap_obj,
   OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
   OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  assert (ap_this_def);
  assert (ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "port [%d] check_tunnel_compat : "
               "Different domain found [%d]", p_obj->pid_,
               ap_other_def->eDomain);
      return OMX_FALSE;
    }

  return OMX_TRUE;
}

/*
 * tizbinaryport_class
 */

static void *
binaryport_class_ctor (void *ap_obj, va_list * app)
{
  struct tizbinaryport_class *p_obj =
    super_ctor (tizbinaryport_class, ap_obj, app);
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

const void *tizbinaryport, *tizbinaryport_class;

void
init_tizbinaryport (void)
{

  if (!tizbinaryport_class)
    {
      init_tizport ();
      tizbinaryport_class = factory_new (tizport_class,
                                         "tizbinaryport_class",
                                         tizport_class,
                                         sizeof (struct tizbinaryport_class),
                                         ctor, binaryport_class_ctor, 0);

    }

  if (!tizbinaryport)
    {
      init_tizport ();
      tizbinaryport =
        factory_new
        (tizbinaryport_class,
         "tizbinaryport",
         tizport,
         sizeof (struct tizbinaryport),
         ctor, binaryport_ctor,
         dtor, binaryport_dtor,
         tizapi_GetParameter, binaryport_GetParameter,
         tizapi_SetParameter, binaryport_SetParameter,
         tizport_check_tunnel_compat, binaryport_check_tunnel_compat, 0);
    }

}
