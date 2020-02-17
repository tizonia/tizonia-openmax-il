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
 * @file   httprmp3port.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia - Http renderer's specialised mp3 port
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

#include "httpr.h"
#include "httprmp3port.h"
#include "httprmp3port_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.mp3port"
#endif

/*
 * httprmp3port class
 */

static void *
httpr_mp3port_ctor (void * ap_obj, va_list * app)
{
  httpr_mp3port_t * p_obj
    = super_ctor (typeOf (ap_obj, "httprmp3port"), ap_obj, app);
  assert (p_obj);

  tiz_port_register_index (p_obj, OMX_TizoniaIndexParamIcecastMountpoint);
  tiz_port_register_index (p_obj, OMX_TizoniaIndexConfigIcecastMetadata);

  p_obj->mountpoint_.nSize = sizeof (OMX_TIZONIA_ICECASTMOUNTPOINTTYPE);
  p_obj->mountpoint_.nVersion.nVersion = OMX_VERSION;
  p_obj->mountpoint_.nPortIndex = 0;

  snprintf ((char *) p_obj->mountpoint_.cMountName,
            sizeof (p_obj->mountpoint_.cMountName), "/");
  snprintf ((char *) p_obj->mountpoint_.cStationName,
            sizeof (p_obj->mountpoint_.cStationName), "Tizonia Radio!");
  snprintf ((char *) p_obj->mountpoint_.cStationDescription,
            sizeof (p_obj->mountpoint_.cStationDescription),
            "Cool Radio Station");
  snprintf ((char *) p_obj->mountpoint_.cStationGenre,
            sizeof (p_obj->mountpoint_.cStationGenre), "Some punchy genre");
  snprintf ((char *) p_obj->mountpoint_.cStationUrl,
            sizeof (p_obj->mountpoint_.cStationUrl), "https://tizonia.org");

  p_obj->mountpoint_.eEncoding = OMX_AUDIO_CodingMP3;
  p_obj->mountpoint_.nIcyMetadataPeriod = ICE_DEFAULT_METADATA_INTERVAL;
  p_obj->mountpoint_.bBurstOnConnect = OMX_TRUE;
  p_obj->mountpoint_.nInitialBurstSize = ICE_INITIAL_BURST_SIZE;
  p_obj->mountpoint_.nMaxClients = ICE_MAX_CLIENTS_PER_MOUNTPOINT;

  p_obj->p_stream_title_ = NULL;

  return p_obj;
}

static void *
httpr_mp3port_dtor (void * ap_obj)
{
  httpr_mp3port_t * p_obj = ap_obj;
  assert (p_obj);
  tiz_mem_free (p_obj->p_stream_title_);
  return super_dtor (typeOf (ap_obj, "httprmp3port"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
httpr_mp3port_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                            OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const httpr_mp3port_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  assert (p_obj);

  if (OMX_TizoniaIndexParamIcecastMountpoint == a_index)
    {
      memcpy (ap_struct, &(p_obj->mountpoint_),
              sizeof (OMX_TIZONIA_ICECASTMOUNTPOINTTYPE));
    }
  else
    {
      /* Delegate to the base port */
      rc = super_GetParameter (typeOf (ap_obj, "httprmp3port"), ap_obj, ap_hdl,
                               a_index, ap_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
httpr_mp3port_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                            OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  httpr_mp3port_t * p_obj = (httpr_mp3port_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  assert (p_obj);

  if (OMX_TizoniaIndexParamIcecastMountpoint == a_index)
    {
      memcpy (&(p_obj->mountpoint_), ap_struct,
              sizeof (OMX_TIZONIA_ICECASTMOUNTPOINTTYPE));
      p_obj->mountpoint_.cStationName[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->mountpoint_.cStationDescription[OMX_MAX_STRINGNAME_SIZE - 1]
        = '\0';
      p_obj->mountpoint_.cStationGenre[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      p_obj->mountpoint_.cStationUrl[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
      TIZ_TRACE (ap_hdl, "Station Name [%s]...",
                 p_obj->mountpoint_.cStationName);
    }
  else
    {
      /* Try the parent's indexes */
      rc = super_SetParameter (typeOf (ap_obj, "httprmp3port"), ap_obj, ap_hdl,
                               a_index, ap_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
httpr_mp3port_GetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const httpr_mp3port_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  assert (p_obj);

  if (OMX_TizoniaIndexConfigIcecastMetadata == a_index)
    {
      OMX_TIZONIA_ICECASTMETADATATYPE * p_metadata
        = (OMX_TIZONIA_ICECASTMETADATATYPE *) ap_struct;

      p_metadata->nVersion.nVersion = OMX_VERSION;

      if (p_obj->p_stream_title_)
        {
          OMX_U32 metadata_buf_size = p_metadata->nSize - sizeof (OMX_U32)
                                      - sizeof (OMX_VERSIONTYPE)
                                      - sizeof (OMX_U32);
          OMX_U32 stream_title_len = strnlen (
            p_obj->p_stream_title_, OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE);

          assert (stream_title_len < OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE);
          if (metadata_buf_size < (stream_title_len + 1)
              && metadata_buf_size < OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE)
            {
              rc = OMX_ErrorBadParameter;
            }
          else
            {
              strncpy ((char *) p_metadata->cStreamTitle,
                       p_obj->p_stream_title_, stream_title_len);
              p_metadata->cStreamTitle[stream_title_len] = '\0';
            }
        }
      else
        {
          p_metadata->cStreamTitle[0] = '\0';
        }
    }
  else
    {
      /* Delegate to the base port */
      rc = super_GetConfig (typeOf (ap_obj, "httprmp3port"), ap_obj, ap_hdl,
                            a_index, ap_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
httpr_mp3port_SetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  httpr_mp3port_t * p_obj = (httpr_mp3port_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  assert (p_obj);

  if (OMX_TizoniaIndexConfigIcecastMetadata == a_index)
    {
      OMX_TIZONIA_ICECASTMETADATATYPE * p_metadata
        = (OMX_TIZONIA_ICECASTMETADATATYPE *) ap_struct;
      OMX_U32 stream_title_len
        = strnlen ((char *) p_metadata->cStreamTitle,
                   OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE + 1);
      if (stream_title_len > OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE)
        {
          rc = OMX_ErrorBadParameter;
        }
      else
        {
          TIZ_TRACE (ap_hdl, "stream_title_len [%d] Stream title [%s]...",
                     stream_title_len, p_metadata->cStreamTitle);

          tiz_mem_free (p_obj->p_stream_title_);
          p_obj->p_stream_title_ = tiz_mem_calloc (1, stream_title_len + 1);
          if (p_obj->p_stream_title_)
            {
              strncpy (p_obj->p_stream_title_,
                       (char *) p_metadata->cStreamTitle, stream_title_len);
              p_obj->p_stream_title_[stream_title_len] = '\0';
            }

          TIZ_TRACE (ap_hdl, "stream_title_len [%d] Stream title [%s]...",
                     stream_title_len, p_obj->p_stream_title_);
        }
    }
  else
    {
      /* Delegate to the base port */
      rc = super_SetConfig (typeOf (ap_obj, "httprmp3port"), ap_obj, ap_hdl,
                            a_index, ap_struct);
    }

  return rc;
}

/*
 * httpr_mp3port_class
 */

static void *
httpr_mp3port_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "httprmp3port_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
httpr_mp3port_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizmp3port = tiz_get_type (ap_hdl, "tizmp3port");
  void * httprmp3port_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizmp3port), "httprmp3port_class", classOf (tizmp3port),
     sizeof (httpr_mp3port_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, httpr_mp3port_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return httprmp3port_class;
}

void *
httpr_mp3port_init (void * ap_tos, void * ap_hdl)
{
  void * tizmp3port = tiz_get_type (ap_hdl, "tizmp3port");
  void * httprmp3port_class = tiz_get_type (ap_hdl, "httprmp3port_class");
  TIZ_LOG_CLASS (httprmp3port_class);
  void * httprmp3port = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (httprmp3port_class, "httprmp3port", tizmp3port, sizeof (httpr_mp3port_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, httpr_mp3port_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, httpr_mp3port_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, httpr_mp3port_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, httpr_mp3port_SetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetConfig, httpr_mp3port_GetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetConfig, httpr_mp3port_SetConfig,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return httprmp3port;
}
