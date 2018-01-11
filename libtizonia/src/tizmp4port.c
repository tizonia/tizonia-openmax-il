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
 * @file   tizmp4port.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - mp4port class implementation
 *
 * NOTE: This port implementation is work in progress!
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <limits.h>

#include <tizplatform.h>

#include "tizmp4port.h"
#include "tizmp4port_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.mp4port"
#endif

/*
 * tizmp4port class
 */

static void *
mp4port_ctor (void * ap_obj, va_list * app)
{
  tiz_mp4port_t * p_obj
    = super_ctor (typeOf (ap_obj, "tizmp4port"), ap_obj, app);
  assert (p_obj);

  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_TizoniaIndexParamAudioOpus));

  return p_obj;
}

static void *
mp4port_dtor (void * ap_obj)
{
  tiz_mp4port_t * p_obj = ap_obj;
  assert (p_obj);
  return super_dtor (typeOf (ap_obj, "tizmp4port"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
mp4port_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                       OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_mp4port_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));

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
      case OMX_IndexParamAudioPcm:
        {
          /* TODO */
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

  return rc;
}

static OMX_ERRORTYPE
mp4port_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                       OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_mp4port_t * p_obj = (tiz_mp4port_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] SetParameter [%s]",
             tiz_port_index (ap_obj), tiz_idx_to_str (a_index));

  switch (a_index)
    {
      case OMX_IndexConfigAudioMute:
        {
          /* TODO: */
        }

      /* NOTE: Fall through if GetParameter returned
       * OMX_ErrorUnsupportedIndex. So that we delegate to the parent */
      /*@fallthrough@*/
      default:
        {
          /* Try the parent's indexes */
          rc = super_SetParameter (typeOf (ap_obj, "tizmp4port"), ap_obj,
                                   ap_hdl, a_index, ap_struct);
        }
    };

  return rc;
}

static bool
mp4port_check_tunnel_compat (const void * ap_obj,
                              OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                              OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t * p_obj = (tiz_port_t *) ap_obj;

  assert (ap_this_def);
  assert (ap_other_def);

  if (ap_other_def->eDomain != OMX_PortDomainAudio
      && ap_other_def->eDomain != OMX_PortDomainVideo
      && ap_other_def->eDomain != OMX_PortDomainOther)
    {
      TIZ_ERROR (handleOf (ap_obj),
                 "port [%d] check_tunnel_compat : "
                 "Audio or Video domain not found, instead found domain [%d]",
                 p_obj->pid_, ap_other_def->eDomain);
      return false;
    }

  if ((ap_other_def->eDomain == OMX_PortDomainAudio
       && ap_other_def->format.audio.eEncoding != OMX_AUDIO_CodingMP4)
      && (ap_other_def->eDomain == OMX_PortDomainVideo
          && ap_other_def->format.video.eCompressionFormat
               != OMX_AUDIO_CodingMP4)
      && (ap_other_def->eDomain == OMX_PortDomainOther
          && ap_other_def->format.other.eFormat
               != OMX_OTHER_FormatBinary))
    {
      TIZ_ERROR (handleOf (ap_obj),
                 "port [%d] check_tunnel_compat : "
                 "Unknown encoding found [%d]",
                 p_obj->pid_, ap_other_def->format.audio.eEncoding);
      return false;
    }

  TIZ_TRACE (handleOf (ap_obj), "port [%d] check_tunnel_compat [OK]",
             p_obj->pid_);

  return true;
}

/*
 * tiz_mp4port_class
 */

static void *
tiz_mp4port_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizmp4port_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_mp4port_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizmp4port_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizport), "tizmp4port_class", classOf (tizport),
     sizeof (tiz_mp4port_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, tiz_mp4port_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizmp4port_class;
}

void *
tiz_mp4port_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizmp4port_class = tiz_get_type (ap_hdl, "tizmp4port_class");
  TIZ_LOG_CLASS (tizmp4port_class);
  void * tizmp4port = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizmp4port_class, "tizmp4port", tizport, sizeof (tiz_mp4port_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, mp4port_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, mp4port_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, mp4port_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, mp4port_SetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_check_tunnel_compat, mp4port_check_tunnel_compat,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizmp4port;
}
