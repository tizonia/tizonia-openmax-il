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
 * @file   spfysrccfgport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  A specialised config port class for the Spotify source component
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

#include "spfysrc.h"
#include "spfysrccfgport.h"
#include "spfysrccfgport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.spotify_source.cfgport"
#endif

/*
 * spfysrccfgport class
 */

static void *
spfysrc_cfgport_ctor (void * ap_obj, va_list * app)
{
  spfysrc_cfgport_t * p_obj
    = super_ctor (typeOf (ap_obj, "spfysrccfgport"), ap_obj, app);

  assert (p_obj);

  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_TizoniaIndexParamAudioSpotifySession));
  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_TizoniaIndexParamAudioSpotifyPlaylist));

  /* Initialize the OMX_TIZONIA_AUDIO_PARAM_SPOTIFYSESSIONTYPE structure */
  TIZ_INIT_OMX_STRUCT (p_obj->spotifysession_);
  snprintf ((char *) p_obj->spotifysession_.cUserName,
            sizeof (p_obj->spotifysession_.cUserName), "tizonia");
  snprintf ((char *) p_obj->spotifysession_.cUserPassword,
            sizeof (p_obj->spotifysession_.cUserPassword), "pass");
  p_obj->spotifysession_.cProxyServer[0] = '\0';
  p_obj->spotifysession_.cProxyUserName[0] = '\0';
  p_obj->spotifysession_.cProxyPassword[0] = '\0';
  p_obj->spotifysession_.bRememberCredentials = OMX_TRUE;
  p_obj->spotifysession_.bRecoverLostToken = OMX_FALSE;
  p_obj->spotifysession_.bAllowExplicitTracks = OMX_FALSE;
  p_obj->spotifysession_.ePreferredBitRate = OMX_AUDIO_SpotifyBitrate320Kbps;
  p_obj->spotifysession_.eConnectionType
    = OMX_AUDIO_SpotifyConnectionMobileWired;

  /* Initialize the OMX_TIZONIA_AUDIO_PARAM_SPOTIFYPLAYLISTTYPE structure */
  TIZ_INIT_OMX_STRUCT (p_obj->playlist_);
  snprintf ((char *) p_obj->playlist_.cPlaylistName,
            sizeof (p_obj->playlist_.cPlaylistName), "playlist");
  p_obj->playlist_.bShuffle = OMX_FALSE;
  p_obj->playlist_.ePlaylistType = OMX_AUDIO_SpotifyPlaylistTypeUnknown;
  snprintf ((char *) p_obj->playlist_.cPlaylistOwner,
            sizeof (p_obj->playlist_.cPlaylistOwner), "owner");

  return p_obj;
}

static void *
spfysrc_cfgport_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "spfysrccfgport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
spfysrc_cfgport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                              OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const spfysrc_cfgport_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamAudioSpotifySession == a_index)
    {
      memcpy (ap_struct, &(p_obj->spotifysession_),
              sizeof (OMX_TIZONIA_AUDIO_PARAM_SPOTIFYSESSIONTYPE));
    }
  else if (OMX_TizoniaIndexParamAudioSpotifyPlaylist == a_index)
    {
      memcpy (ap_struct, &(p_obj->playlist_),
              sizeof (OMX_TIZONIA_AUDIO_PARAM_SPOTIFYPLAYLISTTYPE));
    }
  else
    {
      /* Delegate to the base port */
      rc = super_GetParameter (typeOf (ap_obj, "spfysrccfgport"), ap_obj,
                               ap_hdl, a_index, ap_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
spfysrc_cfgport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                              OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  spfysrc_cfgport_t * p_obj = (spfysrc_cfgport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamAudioSpotifySession == a_index)
    {
      memcpy (&(p_obj->spotifysession_), ap_struct,
              sizeof (OMX_TIZONIA_AUDIO_PARAM_SPOTIFYSESSIONTYPE));
      p_obj->spotifysession_.cUserName[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->spotifysession_.cUserPassword[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->spotifysession_.cProxyServer[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->spotifysession_.cProxyUserName[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->spotifysession_.cProxyPassword[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
    }
  else if (OMX_TizoniaIndexParamAudioSpotifyPlaylist == a_index)
    {
      memcpy (&(p_obj->playlist_), ap_struct,
              sizeof (OMX_TIZONIA_AUDIO_PARAM_SPOTIFYPLAYLISTTYPE));
      p_obj->playlist_.cPlaylistName[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->playlist_.cPlaylistOwner[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      TIZ_TRACE (ap_hdl, "Spotify playlist [%s] owner [%s]...",
                 p_obj->playlist_.cPlaylistName,
                 p_obj->playlist_.cPlaylistOwner);
    }
  else
    {
      /* Delegate to the base port */
      rc = super_SetParameter (typeOf (ap_obj, "spfysrccfgport"), ap_obj,
                               ap_hdl, a_index, ap_struct);
    }

  return rc;
}

/*
 * spfysrc_cfgport_class
 */

static void *
spfysrc_cfgport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "spfysrccfgport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
spfysrc_cfgport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * spfysrccfgport_class
    = factory_new (classOf (tizconfigport), "spfysrccfgport_class",
                   classOf (tizconfigport), sizeof (spfysrc_cfgport_class_t),
                   ap_tos, ap_hdl, ctor, spfysrc_cfgport_class_ctor, 0);
  return spfysrccfgport_class;
}

void *
spfysrc_cfgport_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * spfysrccfgport_class = tiz_get_type (ap_hdl, "spfysrccfgport_class");
  TIZ_LOG_CLASS (spfysrccfgport_class);
  void * spfysrccfgport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (spfysrccfgport_class, "spfysrccfgport", tizconfigport,
     sizeof (spfysrc_cfgport_t),
     /* TIZ_CLASS_COMMENT: class constructor */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, spfysrc_cfgport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, spfysrc_cfgport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, spfysrc_cfgport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, spfysrc_cfgport_SetParameter,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return spfysrccfgport;
}
