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
 * @file   gmusiccfgport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief A specialised config port class for the Google music source component
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

#include "gmusiccfgport.h"
#include "gmusiccfgport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_source.cfgport.gmusic"
#endif

/*
 * gmusiccfgport class
 */

static void *
gmusic_cfgport_ctor (void * ap_obj, va_list * app)
{
  gmusic_cfgport_t * p_obj
    = super_ctor (typeOf (ap_obj, "gmusiccfgport"), ap_obj, app);

  assert (p_obj);

  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_TizoniaIndexParamAudioGmusicSession));
  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_TizoniaIndexParamAudioGmusicPlaylist));

  /* Initialize the OMX_TIZONIA_AUDIO_PARAM_GMUSICSESSIONTYPE structure */
  TIZ_INIT_OMX_STRUCT (p_obj->session_);
  snprintf ((char *) p_obj->session_.cUserName,
            sizeof (p_obj->session_.cUserName), "tizonia");
  snprintf ((char *) p_obj->session_.cUserPassword,
            sizeof (p_obj->session_.cUserPassword), "pass");
  snprintf ((char *) p_obj->session_.cDeviceId,
            sizeof (p_obj->session_.cDeviceId), "deviceId");

  /* Initialize the OMX_TIZONIA_AUDIO_PARAM_GMUSICPLAYLISTTYPE structure */
  TIZ_INIT_OMX_STRUCT (p_obj->playlist_);
  snprintf ((char *) p_obj->playlist_.cPlaylistName,
            sizeof (p_obj->playlist_.cPlaylistName), "playlist");
  /* No need to initialise this: */
  /* snprintf ((char *) p_obj->playlist_.cAdditionalKeywords, */
  /*             sizeof (p_obj->playlist_.cAdditionalKeywords), ""); */
  p_obj->playlist_.ePlaylistType = OMX_AUDIO_GmusicPlaylistTypeUnknown;
  p_obj->playlist_.bShuffle = OMX_FALSE;
  p_obj->playlist_.bUnlimitedSearch = OMX_FALSE;

  return p_obj;
}

static void *
gmusic_cfgport_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "gmusiccfgport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
gmusic_cfgport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                             OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const gmusic_cfgport_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamAudioGmusicSession == a_index)
    {
      memcpy (ap_struct, &(p_obj->session_),
              sizeof (OMX_TIZONIA_AUDIO_PARAM_GMUSICSESSIONTYPE));
    }
  else if (OMX_TizoniaIndexParamAudioGmusicPlaylist == a_index)
    {
      memcpy (ap_struct, &(p_obj->playlist_),
              sizeof (OMX_TIZONIA_AUDIO_PARAM_GMUSICPLAYLISTTYPE));
    }
  else
    {
      /* Delegate to the base port */
      rc = super_GetParameter (typeOf (ap_obj, "gmusiccfgport"), ap_obj, ap_hdl,
                               a_index, ap_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
gmusic_cfgport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                             OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  gmusic_cfgport_t * p_obj = (gmusic_cfgport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamAudioGmusicSession == a_index)
    {
      memcpy (&(p_obj->session_), ap_struct,
              sizeof (OMX_TIZONIA_AUDIO_PARAM_GMUSICSESSIONTYPE));
      p_obj->session_.cUserName[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->session_.cUserPassword[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->session_.cDeviceId[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      TIZ_TRACE (ap_hdl, "Gmusic User Name [%s]...", p_obj->session_.cUserName);
    }
  else if (OMX_TizoniaIndexParamAudioGmusicPlaylist == a_index)
    {
      memcpy (&(p_obj->playlist_), ap_struct,
              sizeof (OMX_TIZONIA_AUDIO_PARAM_GMUSICPLAYLISTTYPE));
      p_obj->playlist_.cPlaylistName[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      TIZ_TRACE (ap_hdl, "Gmusic playlist [%s]...",
                 p_obj->playlist_.cPlaylistName);
      p_obj->playlist_.cAdditionalKeywords[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      TIZ_TRACE (ap_hdl, "Gmusic AdditionalKeywords [%s]...",
                 p_obj->playlist_.cAdditionalKeywords);
    }
  else
    {
      /* Delegate to the base port */
      rc = super_SetParameter (typeOf (ap_obj, "gmusiccfgport"), ap_obj, ap_hdl,
                               a_index, ap_struct);
    }

  return rc;
}

/*
 * gmusic_cfgport_class
 */

static void *
gmusic_cfgport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "gmusiccfgport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
gmusic_cfgport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * gmusiccfgport_class
    = factory_new (classOf (tizconfigport), "gmusiccfgport_class",
                   classOf (tizconfigport), sizeof (gmusic_cfgport_class_t),
                   ap_tos, ap_hdl, ctor, gmusic_cfgport_class_ctor, 0);
  return gmusiccfgport_class;
}

void *
gmusic_cfgport_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * gmusiccfgport_class = tiz_get_type (ap_hdl, "gmusiccfgport_class");
  TIZ_LOG_CLASS (gmusiccfgport_class);
  void * gmusiccfgport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (gmusiccfgport_class, "gmusiccfgport", tizconfigport,
     sizeof (gmusic_cfgport_t),
     /* TIZ_CLASS_COMMENT: class constructor */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, gmusic_cfgport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, gmusic_cfgport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, gmusic_cfgport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, gmusic_cfgport_SetParameter,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return gmusiccfgport;
}
