/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   scloudcfgport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief A specialised config port class for the SoundCloud source component
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <limits.h>

#include <tizplatform.h>

#include "scloudcfgport.h"
#include "scloudcfgport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_source.cfgport.scloud"
#endif

/*
 * scloudcfgport class
 */

static void *
scloud_cfgport_ctor (void * ap_obj, va_list * app)
{
  scloud_cfgport_t * p_obj
    = super_ctor (typeOf (ap_obj, "scloudcfgport"), ap_obj, app);

  assert (p_obj);

  tiz_check_omx_ret_null (tiz_port_register_index (
    p_obj, OMX_TizoniaIndexParamAudioSoundCloudSession));
  tiz_check_omx_ret_null (tiz_port_register_index (
    p_obj, OMX_TizoniaIndexParamAudioSoundCloudPlaylist));

  /* Initialize the OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDSESSIONTYPE structure */
  TIZ_INIT_OMX_STRUCT (p_obj->session_);
  snprintf ((char *) p_obj->session_.cUserName,
            sizeof (p_obj->session_.cUserName), "tizonia");
  snprintf ((char *) p_obj->session_.cUserPassword,
            sizeof (p_obj->session_.cUserPassword), "pass");
  snprintf ((char *) p_obj->session_.cUserOauthToken,
            sizeof (p_obj->session_.cUserOauthToken),
            "1-111111-11111111-11111111111111");

  /* Initialize the OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDPLAYLISTTYPE structure */
  TIZ_INIT_OMX_STRUCT (p_obj->playlist_);
  snprintf ((char *) p_obj->playlist_.cPlaylistName,
            sizeof (p_obj->playlist_.cPlaylistName), "playlist");
  p_obj->playlist_.ePlaylistType = OMX_AUDIO_SoundCloudPlaylistTypeUnknown;
  p_obj->playlist_.bShuffle = OMX_FALSE;

  return p_obj;
}

static void *
scloud_cfgport_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "scloudcfgport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
scloud_cfgport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                             OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const scloud_cfgport_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamAudioSoundCloudSession == a_index)
    {
      memcpy (ap_struct, &(p_obj->session_),
              sizeof (OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDSESSIONTYPE));
    }
  else if (OMX_TizoniaIndexParamAudioSoundCloudPlaylist == a_index)
    {
      memcpy (ap_struct, &(p_obj->playlist_),
              sizeof (OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDPLAYLISTTYPE));
    }
  else
    {
      /* Delegate to the base port */
      rc = super_GetParameter (typeOf (ap_obj, "scloudcfgport"), ap_obj, ap_hdl,
                               a_index, ap_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
scloud_cfgport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                             OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  scloud_cfgport_t * p_obj = (scloud_cfgport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamAudioSoundCloudSession == a_index)
    {
      memcpy (&(p_obj->session_), ap_struct,
              sizeof (OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDSESSIONTYPE));
      p_obj->session_.cUserName[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->session_.cUserPassword[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->session_.cUserOauthToken[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      TIZ_TRACE (ap_hdl, "SoundCloud User's OAuth Token [%s]...",
                 p_obj->session_.cUserOauthToken);
    }
  else if (OMX_TizoniaIndexParamAudioSoundCloudPlaylist == a_index)
    {
      memcpy (&(p_obj->playlist_), ap_struct,
              sizeof (OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDPLAYLISTTYPE));
      p_obj->playlist_.cPlaylistName[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      TIZ_TRACE (ap_hdl, "SoundCloud playlist [%s]...",
                 p_obj->playlist_.cPlaylistName);
    }
  else
    {
      /* Delegate to the base port */
      rc = super_SetParameter (typeOf (ap_obj, "scloudcfgport"), ap_obj, ap_hdl,
                               a_index, ap_struct);
    }

  return rc;
}

/*
 * scloud_cfgport_class
 */

static void *
scloud_cfgport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "scloudcfgport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
scloud_cfgport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * scloudcfgport_class
    = factory_new (classOf (tizconfigport), "scloudcfgport_class",
                   classOf (tizconfigport), sizeof (scloud_cfgport_class_t),
                   ap_tos, ap_hdl, ctor, scloud_cfgport_class_ctor, 0);
  return scloudcfgport_class;
}

void *
scloud_cfgport_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * scloudcfgport_class = tiz_get_type (ap_hdl, "scloudcfgport_class");
  TIZ_LOG_CLASS (scloudcfgport_class);
  void * scloudcfgport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (scloudcfgport_class, "scloudcfgport", tizconfigport,
     sizeof (scloud_cfgport_t),
     /* TIZ_CLASS_COMMENT: class constructor */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, scloud_cfgport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, scloud_cfgport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, scloud_cfgport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, scloud_cfgport_SetParameter,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return scloudcfgport;
}
