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

#include "tizdemuxerport.h"
#include "tizdemuxerport_decls.h"

#include "tizpcmport.h"
#include "tizvideoport.h"

#include "tizosal.h"
#include "tizutils.h"

#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.demuxerport"
#endif

/*
 * tizdemuxerport class
 */

static void *
demuxerport_ctor (void *ap_obj, va_list * app)
{
  tiz_demuxerport_t *p_obj = NULL;
  tiz_port_options_t *p_opts = NULL;
  va_list app_copy;

  va_copy (app_copy, *app);
  if (NULL !=  (p_obj = super_ctor (tizdemuxerport, ap_obj, app)))
    {
      /* Register the demuxer port-specific indexes  */
      tiz_check_omx_err_ret_null
        (tiz_port_register_index (p_obj, OMX_IndexParamNumAvailableStreams));
      tiz_check_omx_err_ret_null
        (tiz_port_register_index (p_obj, OMX_IndexParamActiveStream));

      /* Grab the port options structure */
      p_opts = va_arg (app_copy, tiz_port_options_t *);
      assert (NULL != p_opts);

      switch (p_opts->domain)
        {
        case OMX_PortDomainAudio:
          {
            /* Let's instantiate a PCM port */
            OMX_AUDIO_PARAM_PCMMODETYPE *p_pcmmode = NULL;
            OMX_AUDIO_CONFIG_VOLUMETYPE *p_volume = NULL;
            OMX_AUDIO_CONFIG_MUTETYPE *p_mute = NULL;

            /* TODO: Extract this from the va_list */
            OMX_AUDIO_CODINGTYPE encodings[] = {
              OMX_AUDIO_CodingUnused,
              OMX_AUDIO_CodingMax
            };

            tiz_check_omx_err_ret_null
              (tiz_port_register_index (p_obj, OMX_IndexParamAudioPcm));
            tiz_check_omx_err_ret_null
              (tiz_port_register_index (p_obj, OMX_IndexConfigAudioVolume));
            tiz_check_omx_err_ret_null
              (tiz_port_register_index (p_obj, OMX_IndexConfigAudioMute));

            /* Get the OMX_AUDIO_PARAM_PCMMODETYPE structure */
            p_pcmmode = va_arg (*app, OMX_AUDIO_PARAM_PCMMODETYPE *);

            /* Get the OMX_AUDIO_CONFIG_VOLUMETYPE structure */
            p_volume = va_arg (*app, OMX_AUDIO_CONFIG_VOLUMETYPE *);

            /* Get the OMX_AUDIO_CONFIG_MUTETYPE structure */
            p_mute = va_arg (*app, OMX_AUDIO_CONFIG_MUTETYPE *);

            tiz_check_omx_err_ret_null (tiz_pcmport_init ());
            if (NULL == (p_obj->p_port_
                         = factory_new (tizpcmport, tiz_api_get_hdl (ap_obj),
                                        p_opts, &encodings,
                                        p_pcmmode, p_volume, p_mute)))
              {
                return NULL;
              }
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
            /* NOTE: No defaults are defined in the standard for the video
             * output port of the video_reader.demuxer component. So for the
             * sake of completeness, simply provide some default values
             * here. */
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

            tiz_check_omx_err_ret_null
              (tiz_port_register_index (p_obj, OMX_IndexParamVideoPortFormat));
            tiz_check_omx_err_ret_null (tiz_videoport_init ());
            if (NULL == (p_obj->p_port_
                         = factory_new (tizvideoport, tiz_api_get_hdl (ap_obj),
                                        p_opts, &portdef,
                                        &encodings, &formats)))
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
demuxerport_dtor (void *ap_obj)
{
  tiz_demuxerport_t *p_obj = ap_obj;
  assert (p_obj);
  factory_delete (p_obj->p_port_);
  return super_dtor (tizdemuxerport, ap_obj);
}

/*
 * from tiz_api
 */
static OMX_ERRORTYPE
demuxerport_GetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_demuxerport_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj),
            "GetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamNumAvailableStreams:
    case OMX_IndexParamActiveStream:
      {
        /* Only the processor knows about available or active streams. So lets
           get the processor to fill this info for us. */
        void *p_prc = tiz_get_prc (ap_hdl);
        assert (NULL != p_prc);
        if (OMX_ErrorNone != (rc = tiz_api_GetParameter (p_prc, ap_hdl,
                                                         a_index, ap_struct)))
          {
            TIZ_LOGN (TIZ_ERROR, ap_hdl, "[%s] : Error retrieving [%s] "
                      "from the processor", tiz_err_to_str (rc),
                      tiz_idx_to_str (a_index));
            return rc;
          }
      }
      break;

    case OMX_IndexParamAudioPortFormat:
    case OMX_IndexParamVideoPortFormat:
    case OMX_IndexParamAudioPcm:
      {
        /* Delegate to the domain-specific port */
        if (OMX_ErrorUnsupportedIndex
            != (rc = tiz_api_GetParameter (p_obj->p_port_,
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
        return super_GetParameter (tizdemuxerport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
demuxerport_SetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_demuxerport_t *p_obj = (tiz_demuxerport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj),
            "SetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamNumAvailableStreams:
      {
        /* This is a read-only index. Simply ignore it. */
        TIZ_LOGN (TIZ_NOTICE, ap_hdl, "Ignoring read-only index [%s] ",
                  tiz_idx_to_str (a_index));
      }
      break;

    case OMX_IndexParamActiveStream:
      {
        /* Only the processor knows about active streams. So lets
           get the processor update this info for us. */
        void *p_prc = tiz_get_prc (ap_hdl);
        assert (NULL != p_prc);
        if (OMX_ErrorUnsupportedIndex
            != (rc = tiz_api_SetParameter (p_prc, ap_hdl,
                                           a_index, ap_struct)))
          {
            TIZ_LOGN (TIZ_ERROR, ap_hdl, "[%s] : Error setting [%s] "
                      "on the processor", tiz_err_to_str (rc),
                      tiz_idx_to_str (a_index));
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
            != (rc = tiz_api_SetParameter (p_obj->p_port_,
                                           ap_hdl, a_index, ap_struct)))
          {
            return rc;
          }
      }

      /* NOTE: Fall through if SetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
    default:
      {
        /* Delegate to the base port */
        return super_SetParameter (tizdemuxerport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
demuxerport_GetConfig (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                       OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_demuxerport_t *p_obj = ap_obj;

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj),
            "GetConfig [%s]...", tiz_idx_to_str (a_index));

  assert (NULL != ap_obj);

  switch (a_index)
    {
    case OMX_IndexConfigAudioVolume:
    case OMX_IndexConfigAudioMute:
      {
        OMX_ERRORTYPE rc = OMX_ErrorNone;
        /* Delegate to the domain-specific port */
        if (OMX_ErrorUnsupportedIndex
            != (rc = tiz_api_GetConfig (p_obj->p_port_,
                                           ap_hdl, a_index, ap_struct)))
          {
            return rc;
          }
      }

      /* NOTE: Fall through if GetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
    default:
      {
        /* Try the parent's indexes */
        return super_GetConfig (tizdemuxerport,
                                ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
demuxerport_SetConfig (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_demuxerport_t *p_obj = (tiz_demuxerport_t *) ap_obj;

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj),
            "SetConfig [%s]...", tiz_idx_to_str (a_index));

  assert (NULL != ap_obj);

  switch (a_index)
    {
    case OMX_IndexConfigAudioVolume:
    case OMX_IndexConfigAudioMute:
      {
        OMX_ERRORTYPE rc = OMX_ErrorNone;
        /* TODO: Delegate this to the processor */
        if (OMX_ErrorUnsupportedIndex
            != (rc = tiz_api_SetConfig (p_obj->p_port_,
                                        ap_hdl, a_index, ap_struct)))
          {
            return rc;
          }
      }

      /* NOTE: Fall through if GetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
    default:
      {
        /* Try the parent's indexes */
        return super_SetConfig (tizdemuxerport,
                                ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

static OMX_BOOL
demuxerport_check_tunnel_compat
(const void *ap_obj,
 OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
 OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t *p_obj = (tiz_port_t *) ap_obj;
  assert (ap_this_def);
  assert (ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj),
                "port [%d] check_tunnel_compat : Different domain found [%d]",
                p_obj->pid_, ap_other_def->eDomain);
      return OMX_FALSE;
    }

  return OMX_TRUE;
}

/*
 * tizdemuxerport_class
 */

static void *
demuxerport_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (tizdemuxerport_class, ap_obj, app);
}

/*
 * initialization
 */

const void *tizdemuxerport, *tizdemuxerport_class;

OMX_ERRORTYPE
tiz_demuxerport_init (void)
{
  if (!tizdemuxerport_class)
    {
      tiz_check_omx_err_ret_oom (tiz_port_init ());
      tiz_check_null_ret_oom
        (tizdemuxerport_class = factory_new (tizport_class,
                                            "tizdemuxerport_class",
                                            tizport_class,
                                            sizeof (tiz_demuxerport_class_t),
                                            ctor, demuxerport_class_ctor, 0));
    }

  if (!tizdemuxerport)
    {
      tiz_check_omx_err_ret_oom (tiz_port_init ());
      tiz_check_null_ret_oom
        (tizdemuxerport =
         factory_new
         (tizdemuxerport_class,
          "tizdemuxerport",
          tizport,
          sizeof (tiz_demuxerport_t),
          ctor, demuxerport_ctor,
          dtor, demuxerport_dtor,
          tiz_api_GetParameter, demuxerport_GetParameter,
          tiz_api_SetParameter, demuxerport_SetParameter,
          tiz_api_GetConfig, demuxerport_GetConfig,
          tiz_api_SetConfig, demuxerport_SetConfig,
          tiz_port_check_tunnel_compat, demuxerport_check_tunnel_compat, 0));
    }
  return OMX_ErrorNone;
}
