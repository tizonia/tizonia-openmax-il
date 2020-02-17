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
 * @file   tuneincfgport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief A specialised config port class for the Tunein source component
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

#include "tuneincfgport.h"
#include "tuneincfgport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_source.cfgport.tunein"
#endif

/*
 * tuneincfgport class
 */

static void *
tunein_cfgport_ctor (void * ap_obj, va_list * app)
{
  tunein_cfgport_t * p_obj
    = super_ctor (typeOf (ap_obj, "tuneincfgport"), ap_obj, app);

  assert (p_obj);

  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_TizoniaIndexParamAudioTuneinSession));
  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_TizoniaIndexParamAudioTuneinPlaylist));

  /* Initialize the OMX_TIZONIA_AUDIO_PARAM_TUNEINSESSIONTYPE structure */
  TIZ_INIT_OMX_STRUCT (p_obj->session_);

  /* Initialize the OMX_TIZONIA_AUDIO_PARAM_TUNEINPLAYLISTTYPE structure */
  TIZ_INIT_OMX_STRUCT (p_obj->playlist_);
  snprintf ((char *) p_obj->playlist_.cPlaylistName,
            sizeof (p_obj->playlist_.cPlaylistName), "playlist");
  p_obj->playlist_.cAdditionalKeywords1[0] = '\0';
  p_obj->playlist_.cAdditionalKeywords2[0] = '\0';
  p_obj->playlist_.cAdditionalKeywords3[0] = '\0';
  p_obj->playlist_.ePlaylistType = OMX_AUDIO_TuneinPlaylistTypeUnknown;
  p_obj->playlist_.eSearchType = OMX_AUDIO_TuneinSearchTypeAll;
  p_obj->playlist_.bShuffle = OMX_FALSE;

  return p_obj;
}

static void *
tunein_cfgport_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tuneincfgport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
tunein_cfgport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                             OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tunein_cfgport_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamAudioTuneinSession == a_index)
    {
      memcpy (ap_struct, &(p_obj->session_),
              sizeof (OMX_TIZONIA_AUDIO_PARAM_TUNEINSESSIONTYPE));
    }
  else if (OMX_TizoniaIndexParamAudioTuneinPlaylist == a_index)
    {
      memcpy (ap_struct, &(p_obj->playlist_),
              sizeof (OMX_TIZONIA_AUDIO_PARAM_TUNEINPLAYLISTTYPE));
    }
  else
    {
      /* Delegate to the base port */
      rc = super_GetParameter (typeOf (ap_obj, "tuneincfgport"), ap_obj, ap_hdl,
                               a_index, ap_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
tunein_cfgport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                             OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tunein_cfgport_t * p_obj = (tunein_cfgport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamAudioTuneinSession == a_index)
    {
      memcpy (&(p_obj->session_), ap_struct,
              sizeof (OMX_TIZONIA_AUDIO_PARAM_TUNEINSESSIONTYPE));
    }
  else if (OMX_TizoniaIndexParamAudioTuneinPlaylist == a_index)
    {
      memcpy (&(p_obj->playlist_), ap_struct,
              sizeof (OMX_TIZONIA_AUDIO_PARAM_TUNEINPLAYLISTTYPE));
      p_obj->playlist_.cPlaylistName[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->playlist_.cAdditionalKeywords1[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->playlist_.cAdditionalKeywords2[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->playlist_.cAdditionalKeywords3[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      TIZ_TRACE (ap_hdl, "Tunein playlist [%s]...",
                 p_obj->playlist_.cPlaylistName);
    }
  else
    {
      /* Delegate to the base port */
      rc = super_SetParameter (typeOf (ap_obj, "tuneincfgport"), ap_obj, ap_hdl,
                               a_index, ap_struct);
    }

  return rc;
}

/*
 * tunein_cfgport_class
 */

static void *
tunein_cfgport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tuneincfgport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tunein_cfgport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * tuneincfgport_class
    = factory_new (classOf (tizconfigport), "tuneincfgport_class",
                   classOf (tizconfigport), sizeof (tunein_cfgport_class_t),
                   ap_tos, ap_hdl, ctor, tunein_cfgport_class_ctor, 0);
  return tuneincfgport_class;
}

void *
tunein_cfgport_init (void * ap_tos, void * ap_hdl)
{
  void * tizconfigport = tiz_get_type (ap_hdl, "tizconfigport");
  void * tuneincfgport_class = tiz_get_type (ap_hdl, "tuneincfgport_class");
  TIZ_LOG_CLASS (tuneincfgport_class);
  void * tuneincfgport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tuneincfgport_class, "tuneincfgport", tizconfigport,
     sizeof (tunein_cfgport_t),
     /* TIZ_CLASS_COMMENT: class constructor */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, tunein_cfgport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, tunein_cfgport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, tunein_cfgport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, tunein_cfgport_SetParameter,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tuneincfgport;
}
