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
 * @file   icermp3port.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - Http renderer's specialised mp3 port
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <limits.h>

#include "icermp3port.h"
#include "icermp3port_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.mp3port"
#endif

#define ICE_METADATA_INTERVAL          16000
#define ICE_INITIAL_BURST_SIZE         80000
#define ICE_MAX_CLIENTS_PER_MOUNTPOINT 10

/*
 * icermp3port class
 */

static void *
icer_mp3port_ctor (void *ap_obj, va_list * app)
{
  icer_mp3port_t *p_obj = super_ctor (icermp3port, ap_obj, app);

  tiz_port_register_index (p_obj, OMX_TizoniaIndexParamIcecastMountpoint);
  tiz_port_register_index (p_obj, OMX_TizoniaIndexConfigIcecastMetadata);

  p_obj->mountpoint_.nSize             = sizeof (OMX_TIZONIA_ICECASTMOUNTPOINTTYPE);
  p_obj->mountpoint_.nVersion.nVersion = OMX_VERSION;
  p_obj->mountpoint_.nPortIndex        = 0;

  snprintf ((char *) p_obj->mountpoint_.cMountName,
            sizeof (p_obj->mountpoint_.cMountName), "/");
  snprintf ((char *) p_obj->mountpoint_.cStationName,
            sizeof (p_obj->mountpoint_.cStationName), "Tizonia Radio!");
  snprintf ((char *) p_obj->mountpoint_.cStationDescription,
            sizeof (p_obj->mountpoint_.cStationDescription), "Cool Radio Station");
  snprintf ((char *) p_obj->mountpoint_.cStationGenre,
            sizeof (p_obj->mountpoint_.cStationGenre), "Some punchy genre");
  snprintf ((char *) p_obj->mountpoint_.cStationUrl,
            sizeof (p_obj->mountpoint_.cStationUrl), "http://tizonia.org");

  p_obj->mountpoint_.eEncoding          = OMX_AUDIO_CodingMP3;
  p_obj->mountpoint_.nIcyMetadataPeriod = ICE_METADATA_INTERVAL;
  p_obj->mountpoint_.bBurstOnConnect    = OMX_TRUE;
  p_obj->mountpoint_.nBurstSize         = ICE_INITIAL_BURST_SIZE;
  p_obj->mountpoint_.nMaxClients        = ICE_MAX_CLIENTS_PER_MOUNTPOINT;

  p_obj->metadata_.nSize             = sizeof (OMX_TIZONIA_ICECASTMETADATATYPE);
  p_obj->metadata_.nVersion.nVersion = OMX_VERSION;
  p_obj->metadata_.nPortIndex        = 0;
  snprintf ((char *) p_obj->metadata_.cStreamTitle,
            sizeof (p_obj->metadata_.cStreamTitle), "This is the song's title");

  return p_obj;
}

static void *
icer_mp3port_dtor (void *ap_obj)
{
  return super_dtor (icermp3port, ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
icer_mp3port_GetParameter (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const icer_mp3port_t *p_obj = ap_obj;

  if (OMX_TizoniaIndexParamIcecastMountpoint == a_index)
    {
      memcpy (ap_struct, &(p_obj->mountpoint_),
              sizeof (OMX_TIZONIA_ICECASTMOUNTPOINTTYPE));
    }
  else
    {
      /* Delegate to the base port */
      return super_GetParameter (icermp3port,
                                 ap_obj, ap_hdl, a_index, ap_struct);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_mp3port_SetParameter (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  icer_mp3port_t *p_obj = (icer_mp3port_t *) ap_obj;
  TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamIcecastMountpoint == a_index)
    {
      memcpy (&(p_obj->mountpoint_), ap_struct,
              sizeof (OMX_TIZONIA_ICECASTMOUNTPOINTTYPE));
      p_obj->mountpoint_.cStationName [OMX_MAX_STRINGNAME_SIZE - 1]        = '\000';
      p_obj->mountpoint_.cStationDescription [OMX_MAX_STRINGNAME_SIZE - 1] = '\000';
      p_obj->mountpoint_.cStationGenre [OMX_MAX_STRINGNAME_SIZE - 1]       = '\000';
      p_obj->mountpoint_.cStationUrl [OMX_MAX_STRINGNAME_SIZE - 1]         = '\000';
      TIZ_LOGN (TIZ_TRACE, ap_hdl, "Station Name [%s]...",
                p_obj->mountpoint_.cStationName);
    }
  else
    {
      /* Try the parent's indexes */
      return super_SetParameter (icermp3port,
                                 ap_obj, ap_hdl, a_index, ap_struct);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_mp3port_GetConfig (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const icer_mp3port_t *p_obj = ap_obj;

  if (OMX_TizoniaIndexConfigIcecastMetadata == a_index)
    {
      memcpy (ap_struct, &(p_obj->metadata_),
              sizeof (OMX_TIZONIA_ICECASTMETADATATYPE));
    }
  else
    {
      /* Delegate to the base port */
      return super_GetConfig (icermp3port,
                                 ap_obj, ap_hdl, a_index, ap_struct);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_mp3port_SetConfig (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  icer_mp3port_t *p_obj = (icer_mp3port_t *) ap_obj;

  if (OMX_TizoniaIndexConfigIcecastMetadata == a_index)
    {
      memcpy (&(p_obj->metadata_), ap_struct,
              sizeof (OMX_TIZONIA_ICECASTMETADATATYPE));
      p_obj->metadata_.cStreamTitle [OMX_MAX_STRINGNAME_SIZE - 1] = '\000';
      TIZ_LOGN (TIZ_TRACE, ap_hdl, "Stream title [%s]...",
                p_obj->metadata_.cStreamTitle);
    }
  else
    {
      /* Delegate to the base port */
      return super_SetConfig (icermp3port,
                                 ap_obj, ap_hdl, a_index, ap_struct);
    }

  return OMX_ErrorNone;
}

/*
 * initialization
 */

const void *icermp3port;

void
icer_mp3port_init (void)
{
  if (!icermp3port)
    {
      tiz_mp3port_init ();
      icermp3port =
        factory_new
        (tizmp3port_class,
         "icermp3port",
         tizmp3port,
         sizeof (icer_mp3port_t),
         ctor, icer_mp3port_ctor,
         dtor, icer_mp3port_dtor,
         tiz_api_GetParameter, icer_mp3port_GetParameter,
         tiz_api_SetParameter, icer_mp3port_SetParameter,
         tiz_api_GetConfig, icer_mp3port_GetConfig,
         tiz_api_SetConfig, icer_mp3port_SetConfig,
         0);
    }
}
