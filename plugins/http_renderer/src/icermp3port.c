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

#define ICE_DEFAULT_METADATA_INTERVAL  16000
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

  p_obj->mountpoint_.nSize
    = sizeof (OMX_TIZONIA_ICECASTMOUNTPOINTTYPE);
  p_obj->mountpoint_.nVersion.nVersion = OMX_VERSION;
  p_obj->mountpoint_.nPortIndex        = 0;

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
            sizeof (p_obj->mountpoint_.cStationUrl), "http://tizonia.org");

  p_obj->mountpoint_.eEncoding          = OMX_AUDIO_CodingMP3;
  p_obj->mountpoint_.nIcyMetadataPeriod = ICE_DEFAULT_METADATA_INTERVAL;
  p_obj->mountpoint_.bBurstOnConnect    = OMX_TRUE;
  p_obj->mountpoint_.nInitialBurstSize  = ICE_INITIAL_BURST_SIZE;
  p_obj->mountpoint_.nMaxClients        = ICE_MAX_CLIENTS_PER_MOUNTPOINT;

  p_obj->p_stream_title_ = NULL;

  return p_obj;
}

static void *
icer_mp3port_dtor (void *ap_obj)
{
  icer_mp3port_t *p_obj = ap_obj;
  tiz_mem_free (p_obj->p_stream_title_);
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
  TIZ_TRACE (ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  if (OMX_TizoniaIndexParamIcecastMountpoint == a_index)
    {
      memcpy (&(p_obj->mountpoint_), ap_struct,
              sizeof (OMX_TIZONIA_ICECASTMOUNTPOINTTYPE));
      p_obj->mountpoint_
        .cStationName [OMX_MAX_STRINGNAME_SIZE - 1]        = '\000';
      p_obj->mountpoint_
        .cStationDescription [OMX_MAX_STRINGNAME_SIZE - 1] = '\000';
      p_obj->mountpoint_
        .cStationGenre [OMX_MAX_STRINGNAME_SIZE - 1]       = '\000';
      p_obj->mountpoint_
        .cStationUrl [OMX_MAX_STRINGNAME_SIZE - 1]         = '\000';
      TIZ_TRACE (ap_hdl, "Station Name [%s]...",
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
      OMX_TIZONIA_ICECASTMETADATATYPE *p_metadata
        = (OMX_TIZONIA_ICECASTMETADATATYPE *) ap_struct;

      p_metadata->nVersion.nVersion = OMX_VERSION;

      if (NULL != p_obj->p_stream_title_)
        {
          OMX_U32 metadata_buf_size = p_metadata->nSize
            - sizeof (OMX_U32) - sizeof (OMX_VERSIONTYPE) - sizeof (OMX_U32);
          OMX_U32 stream_title_len
            = strnlen (p_obj->p_stream_title_,
                       OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE);

          assert (stream_title_len  < OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE);
          if (metadata_buf_size < (stream_title_len + 1)
              && metadata_buf_size < OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE)
            {
              return OMX_ErrorBadParameter;
            }

          if (NULL != p_metadata->cStreamTitle)
            {
              strncpy ((char *) p_metadata->cStreamTitle, p_obj->p_stream_title_,
                       stream_title_len);
              p_metadata->cStreamTitle[stream_title_len] = '\000';
            }
        }
      else
        {
          p_metadata->cStreamTitle[0] = '\000';
        }
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
      OMX_TIZONIA_ICECASTMETADATATYPE *p_metadata
        = (OMX_TIZONIA_ICECASTMETADATATYPE *) ap_struct;
      OMX_U32 stream_title_len
        = strnlen ((char *) p_metadata->cStreamTitle,
                   OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE + 1);
      if (stream_title_len > OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE)
        {
          return OMX_ErrorBadParameter;
        }

      TIZ_TRACE (ap_hdl, "stream_title_len [%d] Stream title [%s]...",
                stream_title_len, p_metadata->cStreamTitle);

      tiz_mem_free (p_obj->p_stream_title_);
      p_obj->p_stream_title_ = tiz_mem_calloc (1, stream_title_len + 1);
      if (NULL != p_obj->p_stream_title_)
        {
          strncpy (p_obj->p_stream_title_,
                   (char *) p_metadata->cStreamTitle, stream_title_len);
          p_obj->p_stream_title_[stream_title_len] = '\000';
        }

      TIZ_TRACE (ap_hdl, "stream_title_len [%d] Stream title [%s]...",
                stream_title_len, p_obj->p_stream_title_);
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

OMX_ERRORTYPE
icer_mp3port_init (void)
{
  if (!icermp3port)
    {
      tiz_check_omx_err_ret_oom (tiz_mp3port_init ());
      tiz_check_null_ret_oom
        (icermp3port =
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
          0));
    }
  return OMX_ErrorNone;
}
