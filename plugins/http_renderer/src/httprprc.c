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
 * @file   httprprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia - HTTP renderer processor class
 * implementation
 *
 * NOTE: This is work in progress!
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <OMX_Core.h>

#include <tizkernel.h>

#include "httpr.h"
#include "httprprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.prc"
#endif

static OMX_ERRORTYPE
httpr_prc_config_change (const void * ap_prc, OMX_U32 a_pid,
                         OMX_INDEXTYPE a_config_idx);

static void
release_buffers (httpr_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_server_ && ap_prc->p_inhdr_)
    {
      httpr_srv_release_buffers (ap_prc->p_server_);
    }
  assert (NULL == ap_prc->p_inhdr_);
}

static OMX_BUFFERHEADERTYPE *
buffer_needed (void * ap_arg)
{
  httpr_prc_t * p_prc = ap_arg;
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  assert (p_prc);

  if (!p_prc->port_disabled_)
    {
      if (!p_prc->p_inhdr_)
        {
          (void) tiz_krn_claim_buffer (tiz_get_krn (handleOf (p_prc)),
                                       ARATELIA_HTTP_RENDERER_PORT_INDEX, 0,
                                       &p_prc->p_inhdr_);
          if (p_prc->p_inhdr_)
            {
              TIZ_TRACE (handleOf (p_prc),
                         "Claimed HEADER [%p]...nFilledLen [%d]",
                         p_prc->p_inhdr_, p_prc->p_inhdr_->nFilledLen);
            }
        }
      p_hdr = p_prc->p_inhdr_;
    }

  /*   p_prc->awaiting_buffers_ = p_hdr ? false : true; */
  return p_hdr;
}

static void
buffer_emptied (OMX_BUFFERHEADERTYPE * ap_hdr, void * ap_arg)
{
  httpr_prc_t * p_prc = ap_arg;

  assert (p_prc);
  assert (ap_hdr);
  assert (p_prc->p_inhdr_ == ap_hdr);
  assert (ap_hdr->nFilledLen == 0);

  ap_hdr->nOffset = 0;
  TIZ_TRACE (handleOf (p_prc), "HEADER [%p]", ap_hdr);

  if ((ap_hdr->nFlags & OMX_BUFFERFLAG_EOS) != 0)
    {
      TIZ_TRACE (handleOf (p_prc), "OMX_BUFFERFLAG_EOS in HEADER [%p]", ap_hdr);
      tiz_srv_issue_event ((OMX_PTR) p_prc, OMX_EventBufferFlag,
                           ARATELIA_HTTP_RENDERER_PORT_INDEX, ap_hdr->nFlags,
                           NULL);
    }

  tiz_krn_release_buffer (tiz_get_krn (handleOf (p_prc)),
                          ARATELIA_HTTP_RENDERER_PORT_INDEX, ap_hdr);
  p_prc->p_inhdr_ = NULL;
}

static inline OMX_ERRORTYPE
retrieve_mp3_settings (const void * ap_prc,
                       OMX_AUDIO_PARAM_MP3TYPE * ap_mp3type)
{
  const httpr_prc_t * p_prc = ap_prc;
  assert (ap_prc);
  assert (ap_mp3type);

  /* Retrieve the mp3 settings from the input port */
  TIZ_INIT_OMX_PORT_STRUCT (*ap_mp3type, ARATELIA_HTTP_RENDERER_PORT_INDEX);
  tiz_check_omx (tiz_api_GetParameter (tiz_get_krn (handleOf (p_prc)),
                                           handleOf (p_prc),
                                           OMX_IndexParamAudioMp3, ap_mp3type));
  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE
retrieve_mountpoint_settings (const void * ap_prc,
                              OMX_TIZONIA_ICECASTMOUNTPOINTTYPE * ap_mountpoint)
{
  const httpr_prc_t * p_prc = ap_prc;
  assert (p_prc);
  assert (ap_mountpoint);

  /* Retrieve the mountpoint settings from the input port */
  TIZ_INIT_OMX_PORT_STRUCT (*ap_mountpoint, ARATELIA_HTTP_RENDERER_PORT_INDEX);
  tiz_check_omx (tiz_api_GetParameter (
    tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
    OMX_TizoniaIndexParamIcecastMountpoint, ap_mountpoint));
  return OMX_ErrorNone;
}

/*
 * httprprc
 */

static void *
httpr_prc_ctor (void * ap_prc, va_list * app)
{
  httpr_prc_t * p_prc = super_ctor (typeOf (ap_prc, "httprprc"), ap_prc, app);
  assert (p_prc);
  p_prc->mount_name_ = NULL;
  p_prc->port_disabled_ = false;
  p_prc->p_server_ = NULL;
  p_prc->p_inhdr_ = NULL;
  return p_prc;
}

static void *
httpr_prc_dtor (void * ap_prc)
{
  return super_dtor (typeOf (ap_prc, "httprprc"), ap_prc);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
httpr_prc_allocate_resources (void * ap_prc, OMX_U32 a_pid)
{
  httpr_prc_t * p_prc = ap_prc;
  assert (p_prc);

  /* Retrieve http server configuration from the component's config port */
  TIZ_INIT_OMX_STRUCT (p_prc->server_info_);
  tiz_check_omx (tiz_api_GetParameter (
    tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
    OMX_TizoniaIndexParamHttpServer, &p_prc->server_info_));

  return httpr_srv_init (
    &(p_prc->p_server_), p_prc, p_prc->server_info_.cBindAddress, /* if this is
                                                            * null, the
                                                            * server will
                                                            * listen on
                                                            * all
                                                            * interfaces. */
    p_prc->server_info_.nListeningPort, p_prc->server_info_.nMaxClients,
    buffer_emptied, buffer_needed, p_prc);
}

static OMX_ERRORTYPE
httpr_prc_deallocate_resources (void * ap_prc)
{
  httpr_prc_t * p_prc = ap_prc;
  assert (p_prc);
  httpr_srv_destroy (p_prc->p_server_);
  p_prc->p_server_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
httpr_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid)
{
  httpr_prc_t * p_prc = ap_prc;

  assert (p_prc);

  /* Obtain mp3 settings from port */
  tiz_check_omx (retrieve_mp3_settings (ap_prc, &(p_prc->mp3type_)));

  httpr_srv_set_mp3_settings (p_prc->p_server_, p_prc->mp3type_.nBitRate,
                              p_prc->mp3type_.nChannels,
                              p_prc->mp3type_.nSampleRate);

  /* Obtain mount point and station-related information */
  tiz_check_omx (
    retrieve_mountpoint_settings (ap_prc, &(p_prc->mountpoint_)));

  httpr_srv_set_mountpoint_settings (
    p_prc->p_server_, p_prc->mountpoint_.cMountName,
    p_prc->mountpoint_.cStationName, p_prc->mountpoint_.cStationDescription,
    p_prc->mountpoint_.cStationGenre, p_prc->mountpoint_.cStationUrl,
    p_prc->mountpoint_.nIcyMetadataPeriod,
    (p_prc->mountpoint_.bBurstOnConnect == OMX_TRUE
       ? p_prc->mountpoint_.nInitialBurstSize
       : 0),
    p_prc->mountpoint_.nMaxClients);

  tiz_check_omx (
    httpr_prc_config_change (p_prc, ARATELIA_HTTP_RENDERER_PORT_INDEX,
                             OMX_TizoniaIndexConfigIcecastMetadata));

  return httpr_srv_start (p_prc->p_server_);
}

static OMX_ERRORTYPE
httpr_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
httpr_prc_stop_and_return (void * ap_prc)
{
  httpr_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  rc = httpr_srv_stop (p_prc->p_server_);
  release_buffers (p_prc);
  return rc;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
httpr_prc_buffers_ready (const void * ap_prc)
{
  httpr_prc_t * p_prc = (httpr_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (p_prc->p_server_)
    {
      rc = httpr_srv_buffer_event (p_prc->p_server_);
    }
  return rc;
}

static OMX_ERRORTYPE
httpr_prc_io_ready (void * ap_prc, tiz_event_io_t * ap_ev_io, int a_fd,
                    int a_events)
{
  httpr_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (p_prc->p_server_)
    {
      httpr_srv_io_event (p_prc->p_server_, a_fd);
    }
  return rc;
}

static OMX_ERRORTYPE
httpr_prc_timer_ready (void * ap_prc, tiz_event_timer_t * ap_ev_timer,
                       void * ap_arg, const uint32_t aid)
{
  httpr_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (p_prc->p_server_)
    {
      rc = httpr_srv_timer_event (p_prc->p_server_);
    }
  return rc;
}

static OMX_ERRORTYPE
httpr_prc_port_enable (const void * ap_prc, OMX_U32 a_pid)
{
  httpr_prc_t * p_prc = (httpr_prc_t *) ap_prc;

  assert (ap_prc);
  assert (ARATELIA_HTTP_RENDERER_PORT_INDEX == a_pid);

  p_prc->port_disabled_ = false;

  tiz_check_omx (retrieve_mp3_settings (p_prc, &(p_prc->mp3type_)));
  httpr_srv_set_mp3_settings (p_prc->p_server_, p_prc->mp3type_.nBitRate,
                              p_prc->mp3type_.nChannels,
                              p_prc->mp3type_.nSampleRate);
  tiz_check_omx (
    httpr_prc_config_change (p_prc, ARATELIA_HTTP_RENDERER_PORT_INDEX,
                             OMX_TizoniaIndexConfigIcecastMetadata));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
httpr_prc_port_disable (const void * ap_prc, OMX_U32 a_pid)
{
  httpr_prc_t * p_prc = (httpr_prc_t *) ap_prc;
  p_prc->port_disabled_ = true;
  release_buffers (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
httpr_prc_config_change (const void * ap_prc, const OMX_U32 a_pid,
                         const OMX_INDEXTYPE a_config_idx)
{
  const httpr_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_prc);

  if (p_prc->p_server_ && OMX_TizoniaIndexConfigIcecastMetadata == a_config_idx
      && ARATELIA_HTTP_RENDERER_PORT_INDEX == a_pid)
    {
      OMX_TIZONIA_ICECASTMETADATATYPE * p_metadata
        = (OMX_TIZONIA_ICECASTMETADATATYPE *) tiz_mem_calloc (
          1, sizeof (OMX_TIZONIA_ICECASTMETADATATYPE)
               + OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE + 1);

      tiz_check_null_ret_oom (p_metadata);

      /* Retrieve the updated icecast metadata from the input port */
      TIZ_INIT_OMX_PORT_STRUCT (*p_metadata, ARATELIA_HTTP_RENDERER_PORT_INDEX);
      p_metadata->nSize = sizeof (OMX_TIZONIA_ICECASTMETADATATYPE)
                          + OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE;

      if (OMX_ErrorNone
          != (rc = tiz_api_GetConfig (
                tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
                OMX_TizoniaIndexConfigIcecastMetadata, p_metadata)))
        {
          TIZ_ERROR (handleOf (p_prc),
                     "[%s] : Error retrieving "
                     "OMX_TizoniaIndexConfigIcecastMetadata from port",
                     tiz_err_to_str (rc));
        }
      else
        {
          httpr_srv_set_stream_title (p_prc->p_server_,
                                      p_metadata->cStreamTitle);
        }

      tiz_mem_free (p_metadata);
      p_metadata = NULL;
    }
  return rc;
}

/*
 * httpr_prc_class
 */

static void *
httpr_prc_class_ctor (void * ap_prc, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_prc, "httprprc_class"), ap_prc, app);
}

/*
 * initialization
 */

void *
httpr_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * httprprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "httprprc_class", classOf (tizprc),
     sizeof (httpr_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, httpr_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value */
     0);
  return httprprc_class;
}

void *
httpr_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * httprprc_class = tiz_get_type (ap_hdl, "httprprc_class");
  TIZ_LOG_CLASS (httprprc_class);
  void * httprprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (httprprc_class, "httprprc", tizprc, sizeof (httpr_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, httpr_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, httpr_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, httpr_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, httpr_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, httpr_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, httpr_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, httpr_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_io_ready, httpr_prc_io_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_timer_ready, httpr_prc_timer_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, httpr_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, httpr_prc_port_enable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, httpr_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_config_change, httpr_prc_config_change,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return httprprc;
}
