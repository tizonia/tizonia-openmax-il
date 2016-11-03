/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   tizmuxerport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - muxerport class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizplatform.h>

#include "tizutils.h"
#include "tizpcmport.h"
#include "tizvideoport.h"
#include "tizmuxerport.h"
#include "tizmuxerport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.muxerport"
#endif

static void *
alloc_pcm_port (tiz_muxerport_t * ap_obj, tiz_port_options_t * ap_opts,
                OMX_AUDIO_CODINGTYPE a_encodings[], va_list * ap_args)
{
  OMX_AUDIO_PARAM_PCMMODETYPE * p_pcmmode = NULL;
  OMX_AUDIO_CONFIG_VOLUMETYPE * p_volume = NULL;
  OMX_AUDIO_CONFIG_MUTETYPE * p_mute = NULL;

  assert (ap_obj);
  assert (ap_opts);
  assert (a_encodings);
  assert (ap_args);

  /* Register the PCM port indexes, so this port receives the get/set
               requests */
  tiz_check_omx_ret_null (
    tiz_port_register_index (ap_obj, OMX_IndexParamAudioPcm));
  tiz_check_omx_ret_null (
    tiz_port_register_index (ap_obj, OMX_IndexConfigAudioVolume));
  tiz_check_omx_ret_null (
    tiz_port_register_index (ap_obj, OMX_IndexConfigAudioMute));

  /* Get the OMX_AUDIO_PARAM_PCMMODETYPE structure (mandatory argument) */
  p_pcmmode = va_arg (*ap_args, OMX_AUDIO_PARAM_PCMMODETYPE *);
  assert (p_pcmmode);

  /* Get the OMX_AUDIO_CONFIG_VOLUMETYPE structure (mandatory argument) */
  p_volume = va_arg (*ap_args, OMX_AUDIO_CONFIG_VOLUMETYPE *);
  assert (p_volume);

  /* Get the OMX_AUDIO_CONFIG_MUTETYPE structure (mandatory argument) */
  p_mute = va_arg (*ap_args, OMX_AUDIO_CONFIG_MUTETYPE *);
  assert (p_mute);

  TIZ_TRACE (handleOf (ap_obj), "p_volume->sVolume.nValue [%d]",
             p_volume->sVolume.nValue);

  return factory_new (typeOf (ap_obj, "tizpcmport"), ap_opts, a_encodings,
                      p_pcmmode, p_volume, p_mute);
}

static void *
alloc_opus_port (tiz_muxerport_t * ap_obj, tiz_port_options_t * ap_opts,
                 OMX_AUDIO_CODINGTYPE a_encodings[], va_list * ap_args)
{
  OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE * p_opusmode = NULL;

  assert (ap_obj);
  assert (ap_opts);
  assert (a_encodings);
  assert (ap_args);

  /* Register the OPUS port indexes, for when this port receives the get/set
   requests */
  tiz_check_omx_ret_null (
    tiz_port_register_index (ap_obj, OMX_TizoniaIndexParamAudioOpus));

  /* Get the OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE structure (mandatory argument) */
  p_opusmode = va_arg (*ap_args, OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE *);
  assert (p_opusmode);

  return factory_new (typeOf (ap_obj, "tizopusport"), ap_opts, a_encodings,
                      p_opusmode);
}

/*
 * tizmuxerport class
 */

static void *
muxerport_ctor (void * ap_obj, va_list * app)
{
  tiz_muxerport_t * p_obj = NULL;
  tiz_port_options_t * p_opts = NULL;
  va_list app_copy;

  /* Make a copy of the incoming va_list before it gets parsed by the parent
     class:
     The expected arguments are:
     port_opts

     */
  va_copy (app_copy, *app);

  /* Now give the original to the base class */
  if ((p_obj = super_ctor (typeOf (ap_obj, "tizmuxerport"), ap_obj, app)))
    {

      /* Grab the port options structure (mandatory argument) */
      p_opts = va_arg (app_copy, tiz_port_options_t *);
      assert (p_opts);

      TIZ_TRACE (handleOf (ap_obj), "min_buf_size [%d]", p_opts->min_buf_size);

      switch (p_opts->domain)
        {
          case OMX_PortDomainAudio:
            {
              OMX_AUDIO_CODINGTYPE * p_encodings = NULL;

              /* Get the array of OMX_AUDIO_CODINGTYPE values  (mandatory argument) */
              p_encodings = va_arg (app_copy, OMX_AUDIO_CODINGTYPE *);
              assert (p_encodings);

              switch (p_encodings[0])
                {
                  case OMX_AUDIO_CodingPCM:
                    {
                      /* Let's instantiate am PCM port */
                      p_obj->p_port_ = alloc_pcm_port (p_obj, p_opts,
                                                       p_encodings, &app_copy);
                    }
                    break;
                  default:
                    {
                      if (OMX_AUDIO_CodingOPUS == p_encodings[0])
                        {
                          /* Let's instantiate an OPUS port */
                          p_obj->p_port_ = alloc_opus_port (
                            p_obj, p_opts, p_encodings, &app_copy);
                        }
                    }
                    break;
                };
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

              if (!(p_obj->p_port_
                    = factory_new (typeOf (ap_obj, "tizvideoport"), p_opts,
                                   p_portdef, p_encodings, p_formats)))
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
muxerport_dtor (void * ap_obj)
{
  tiz_muxerport_t * p_obj = ap_obj;
  assert (p_obj);
  factory_delete (p_obj->p_port_);
  return super_dtor (typeOf (ap_obj, "tizmuxerport"), ap_obj);
}

/*
 * from tiz_api
 */
static OMX_ERRORTYPE
muxerport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_muxerport_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] GetParameter [%s]",
             tiz_port_index (ap_obj), tiz_idx_to_str (a_index));
  assert (ap_obj);

  switch (a_index)
    {
      case OMX_IndexParamAudioPortFormat:
      case OMX_IndexParamVideoPortFormat:
      case OMX_IndexParamAudioPcm:
      case OMX_IndexConfigAudioVolume:
      case OMX_IndexConfigAudioMute:
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
          /* But before delegating, check if this is an extension index */
          if (OMX_TizoniaIndexParamAudioOpus == a_index)
            {
              /* Delegate to the domain-specific port */
              if (OMX_ErrorUnsupportedIndex
                  != (rc = tiz_api_GetParameter (p_obj->p_port_, ap_hdl,
                                                 a_index, ap_struct)))
                {
                  return rc;
                }
            }

          /* Delegate to the base port */
          return super_GetParameter (typeOf (ap_obj, "tizmuxerport"), ap_obj,
                                     ap_hdl, a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
muxerport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_muxerport_t * p_obj = (tiz_muxerport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] SetParameter [%s]",
             tiz_port_index (ap_obj), tiz_idx_to_str (a_index));
  assert (ap_obj);

  switch (a_index)
    {
      case OMX_IndexParamAudioPortFormat:
      case OMX_IndexParamVideoPortFormat:
      case OMX_IndexParamAudioPcm:
      case OMX_IndexConfigAudioVolume:
      case OMX_IndexConfigAudioMute:
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
          /* But before delegating, check if this is an extension index */
          if (OMX_TizoniaIndexParamAudioOpus == a_index)
            {
              /* Delegate to the domain-specific port */
              if (OMX_ErrorUnsupportedIndex
                  != (rc = tiz_api_SetParameter (p_obj->p_port_, ap_hdl,
                                                 a_index, ap_struct)))
                {
                  return rc;
                }
            }

          /* Delegate to the base port */
          return super_SetParameter (typeOf (ap_obj, "tizmuxerport"), ap_obj,
                                     ap_hdl, a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
muxerport_GetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                     OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_muxerport_t * p_obj = ap_obj;
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
          rc = super_GetConfig (typeOf (ap_obj, "tizmuxerport"), ap_obj, ap_hdl,
                                a_index, ap_struct);
        }
    };

  return rc;
}

static OMX_ERRORTYPE
muxerport_SetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                     OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_muxerport_t * p_obj = (tiz_muxerport_t *) ap_obj;
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
          rc = super_SetConfig (typeOf (ap_obj, "tizmuxerport"), ap_obj, ap_hdl,
                                a_index, ap_struct);
        }
    };

  return rc;
}

static bool
muxerport_check_tunnel_compat (const void * ap_obj,
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
 * tizmuxerport_class
 */

static void *
muxerport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizmuxerport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_muxerport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizmuxerport_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizport), "tizmuxerport_class", classOf (tizport),
     sizeof (tiz_muxerport_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, muxerport_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizmuxerport_class;
}

void *
tiz_muxerport_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizmuxerport_class = tiz_get_type (ap_hdl, "tizmuxerport_class");
  TIZ_LOG_CLASS (tizmuxerport_class);
  void * tizmuxerport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizmuxerport_class, "tizmuxerport", tizport, sizeof (tiz_muxerport_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, muxerport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, muxerport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, muxerport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, muxerport_SetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetConfig, muxerport_GetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetConfig, muxerport_SetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_check_tunnel_compat, muxerport_check_tunnel_compat,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizmuxerport;
}
