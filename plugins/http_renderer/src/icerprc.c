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
 * @file   icerprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - HTTP renderer processor class
 * implementation
 *
 * NOTE: This is work in progress!
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "OMX_Core.h"
#include "OMX_TizoniaExt.h"

#include "tizkernel.h"
#include "tizscheduler.h"

#include "icerprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.prc"
#endif

static OMX_ERRORTYPE
stream_to_clients (icer_prc_t *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);

  if (icer_con_get_listeners_count (ap_obj->p_server_) == 0)
    {
      return OMX_ErrorNone;
    }

  if (ap_obj->p_server_ && !ap_obj->server_is_full_)
    {
      rc = icer_con_write_to_listeners (ap_obj->p_server_);

      switch (rc)
        {
        case OMX_ErrorNoMore:
          {
            /* Socket send buffers are full */
            ap_obj->server_is_full_ = true;
            TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s] : ", tiz_err_to_str (rc));
            rc = OMX_ErrorNone;
          }
          break;

        case OMX_ErrorOverflow:
          {
            /* Trying not send too much data. */
            ap_obj->server_is_full_ = false;
            TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s] : ", tiz_err_to_str (rc));
            rc = OMX_ErrorNone;
          }
          break;

        case OMX_ErrorNone:
          {
            /* More data needed */
            ap_obj->server_is_full_ = false;
            TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s] : ", tiz_err_to_str (rc));
          }
          break;

        case OMX_ErrorNotReady:
          {
            /* No connected clients yet */
            ap_obj->server_is_full_ = false;
            TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s] : ", tiz_err_to_str (rc));
            rc = OMX_ErrorNone;
          }
          break;

        default:
          {
            TIZ_LOGN (TIZ_TRACE, ap_hdl, "[%s] : Error while writing to "
                      "clients ...", tiz_err_to_str (rc));
          }
          break;

        };
    }

  return rc;
}

static OMX_BUFFERHEADERTYPE *
buffer_needed (void *ap_arg)
{
  icer_prc_t *p_obj = ap_arg;

  assert (NULL != p_obj);

  if (!(p_obj->eos_))
    {
      if (NULL != p_obj->p_inhdr_ && p_obj->p_inhdr_->nFilledLen > 0)
        {
          return p_obj->p_inhdr_;
        }
      else
        {
          const tiz_srv_t *p_parent = ap_arg;
          tiz_pd_set_t ports;
          void *p_krn = NULL;

          assert (false == p_obj->server_is_full_);

          assert (NULL != p_parent->p_hdl_);
          p_krn = tiz_get_krn (tiz_srv_get_hdl (p_obj));

          TIZ_PD_ZERO (&ports);
          if (OMX_ErrorNone == tiz_krn_select (p_krn, 1, &ports))
            {
              if (TIZ_PD_ISSET (0, &ports))
                {
                  if (OMX_ErrorNone == tiz_krn_claim_buffer
                      (p_krn, 0, 0, &p_obj->p_inhdr_))
                    {
                      TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (p_obj),
                                "Claimed HEADER [%p]...nFilledLen [%d]",
                                p_obj->p_inhdr_, p_obj->p_inhdr_->nFilledLen);
                      return p_obj->p_inhdr_;
                    }
                }
            }
        }
    }

  return NULL;
}

static void
buffer_emptied (OMX_BUFFERHEADERTYPE * ap_hdr, void *ap_arg)
{
  icer_prc_t *p_obj = ap_arg;

  assert (NULL != p_obj);
  assert (NULL != ap_hdr);
  assert (p_obj->p_inhdr_ == ap_hdr);
  assert (ap_hdr->nFilledLen == 0);

  TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (p_obj), "HEADER [%p] emptied", ap_hdr);

  p_obj->server_is_full_ = false;
  ap_hdr->nOffset = 0;

  if ((ap_hdr->nFlags & OMX_BUFFERFLAG_EOS) != 0)
    {
      TIZ_LOG (TIZ_TRACE, "OMX_BUFFERFLAG_EOS in HEADER [%p]", ap_hdr);
      tiz_srv_issue_event ((OMX_PTR) p_obj,
                           OMX_EventBufferFlag,
                           0, ap_hdr->nFlags, NULL);
    }

  tiz_krn_release_buffer (tiz_get_krn (tiz_srv_get_hdl (p_obj)), 0, ap_hdr);
  p_obj->p_inhdr_ = NULL;
}

/*
 * icerprc
 */

static void *
icer_proc_ctor (void *ap_obj, va_list * app)
{
  icer_prc_t *p_obj      = super_ctor (icerprc, ap_obj, app);
  p_obj->bind_address_   = NULL;
  p_obj->lstn_port_      = 0;
  p_obj->mount_name_     = NULL;
  p_obj->max_clients_    = 0;
  p_obj->burst_size_     = 65536;
  p_obj->server_is_full_ = false;
  p_obj->eos_            = false;
  p_obj->lstn_sockfd_    = ICE_RENDERER_SOCK_ERROR;
  p_obj->p_server_       = NULL;
  p_obj->p_inhdr_        = NULL;
  /* p_obj->mp3type */
  return p_obj;
}

static void *
icer_proc_dtor (void *ap_obj)
{
  return super_dtor (icerprc, ap_obj);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
icer_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  icer_prc_t *p_obj = ap_obj;
  OMX_HANDLETYPE p_hdl = tiz_srv_get_hdl (p_obj);
  void *p_krn = tiz_get_krn (p_hdl);
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_TIZONIA_PARAM_HTTPSERVERTYPE httpsrv;

  assert (NULL != ap_obj);
  assert (NULL != p_krn);

  /* Retrieve http server configuration from the component's config port */
  httpsrv.nSize = sizeof (OMX_TIZONIA_PARAM_HTTPSERVERTYPE);
  httpsrv.nVersion.nVersion = OMX_VERSION;
  if (OMX_ErrorNone
      != (rc = tiz_api_GetParameter (p_krn, p_hdl,
                                     OMX_TizoniaIndexParamHttpServer,
                                     &httpsrv)))
    {
      TIZ_LOGN (TIZ_TRACE, p_hdl, "[%s] : Error retrieving "
                "HTTPSERVERTYPE from port", tiz_err_to_str (rc));
      return rc;
    }

  TIZ_LOGN (TIZ_TRACE, p_hdl, "nListeningPort = [%d] nMaxClients = [%d] ",
            httpsrv.nListeningPort, httpsrv.nMaxClients);

  p_obj->lstn_port_   = httpsrv.nListeningPort;
  p_obj->max_clients_ = httpsrv.nMaxClients;

  return icer_con_server_init (&(p_obj->p_server_), p_hdl,
                               p_obj->bind_address_, p_obj->lstn_port_,
                               p_obj->max_clients_, buffer_emptied,
                               buffer_needed, p_obj);
}

static OMX_ERRORTYPE
icer_proc_deallocate_resources (void *ap_obj)
{
  icer_prc_t *p_obj = ap_obj;
  assert (NULL != ap_obj);
  icer_con_server_destroy (p_obj->p_server_);
  p_obj->p_server_ = NULL;
  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE
retrieve_mp3_settings (void *ap_obj, OMX_AUDIO_PARAM_MP3TYPE *ap_mp3type)
{
  const icer_prc_t *p_obj = ap_obj;
  OMX_HANDLETYPE p_hdl = tiz_srv_get_hdl (p_obj);
  void *p_krn = tiz_get_krn (p_hdl);
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != ap_mp3type);

  /* Retrieve the mp3 settings from the input port */
  ap_mp3type->nSize             = sizeof (OMX_AUDIO_PARAM_MP3TYPE);
  ap_mp3type->nVersion.nVersion = OMX_VERSION;
  ap_mp3type->nPortIndex        = 0;
  if (OMX_ErrorNone
      != (rc = tiz_api_GetParameter (p_krn, p_hdl, OMX_IndexParamAudioMp3,
                                     ap_mp3type)))
    {
      TIZ_LOGN (TIZ_TRACE, p_hdl, "[%s] : Error retrieving "
                "OMX_IndexParamAudioMp3 from port", tiz_err_to_str (rc));
    }

  return rc;
}

static OMX_ERRORTYPE
icer_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  icer_prc_t *p_obj   = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);

  TIZ_LOGN (TIZ_TRACE, tiz_srv_get_hdl (p_obj),
           "Server starts listening on port [%d]", p_obj->lstn_port_);
  p_obj->lstn_sockfd_ = icer_con_get_server_fd (p_obj->p_server_);

  if (OMX_ErrorNone != (rc = retrieve_mp3_settings (ap_obj, &(p_obj->mp3type))))
    {
      return rc;
    }

  icer_con_set_mp3_settings (p_obj->p_server_, p_obj->mp3type.nBitRate,
                             p_obj->mp3type.nChannels, p_obj->mp3type.nSampleRate);

  return icer_con_start_listening (p_obj->p_server_);
}

static OMX_ERRORTYPE
icer_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_proc_stop_and_return (void *ap_obj)
{
  icer_prc_t *p_obj = ap_obj;
  assert (NULL != ap_obj);
  return icer_con_stop_listening (p_obj->p_server_);
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
icer_proc_buffers_ready (const void *ap_obj)
{
  icer_prc_t *p_obj = (icer_prc_t *) ap_obj;
  return stream_to_clients (p_obj, tiz_srv_get_hdl (p_obj));
}

static OMX_ERRORTYPE
icer_io_ready (void *ap_obj,
               tiz_event_io_t * ap_ev_io, int a_fd, int a_events)
{
  icer_prc_t *p_obj = ap_obj;
  OMX_HANDLETYPE p_hdl = tiz_srv_get_hdl (p_obj);
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_obj);
  TIZ_LOGN (TIZ_TRACE, p_hdl, "Received io event on socket fd [%d] "
            "lstn_sockfd_ [%d]", a_fd, p_obj->lstn_sockfd_);

  if (a_fd == p_obj->lstn_sockfd_)
    {
      rc = icer_con_accept_connection (p_obj->p_server_);
      if (OMX_ErrorInsufficientResources != rc)
        {
          rc = OMX_ErrorNone;
        }
    }
  else
    {
      if (a_events & TIZ_EVENT_WRITE || a_events & TIZ_EVENT_READ_OR_WRITE)
        {
          p_obj->server_is_full_ = false;
        }
      rc = stream_to_clients (p_obj, p_hdl);
    }

  return rc;
}

static OMX_ERRORTYPE
icer_timer_ready (void *ap_obj, tiz_event_timer_t * ap_ev_timer,
                        void *ap_arg)
{
  icer_prc_t *p_obj = ap_obj;
  assert (NULL != p_obj);
  TIZ_LOGN (TIZ_NOTICE, tiz_srv_get_hdl (p_obj), "Received timer event ");
  p_obj->server_is_full_ = false;
  return stream_to_clients (p_obj, tiz_srv_get_hdl (p_obj));
}

static OMX_ERRORTYPE
icer_proc_port_enable (const void *ap_obj, OMX_U32 a_pid)
{
  icer_prc_t *p_obj = (icer_prc_t *) ap_obj;
  OMX_HANDLETYPE p_hdl = tiz_srv_get_hdl (p_obj);
  void *p_krn = tiz_get_krn (p_hdl);
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != p_krn);

  assert (0 == a_pid);

  if (OMX_ErrorNone != (rc = retrieve_mp3_settings (p_obj, &(p_obj->mp3type))))
    {
      return rc;
    }

  icer_con_set_mp3_settings (p_obj->p_server_, p_obj->mp3type.nBitRate,
                             p_obj->mp3type.nChannels, p_obj->mp3type.nSampleRate);

  return OMX_ErrorNone;
}


/*
 * initialization
 */

const void *icerprc;

void
icer_prc_init (void)
{
  if (!icerprc)
    {
      tiz_prc_init ();
      icerprc =
        factory_new
        (tizprc_class,
         "icerprc",
         tizprc,
         sizeof (icer_prc_t),
         ctor, icer_proc_ctor,
         dtor, icer_proc_dtor,
         tiz_prc_buffers_ready, icer_proc_buffers_ready,
         tiz_prc_port_enable, icer_proc_port_enable,
         tiz_srv_allocate_resources, icer_proc_allocate_resources,
         tiz_srv_deallocate_resources, icer_proc_deallocate_resources,
         tiz_srv_prepare_to_transfer, icer_proc_prepare_to_transfer,
         tiz_srv_transfer_and_process, icer_proc_transfer_and_process,
         tiz_srv_stop_and_return, icer_proc_stop_and_return,
         tiz_prc_io_ready, icer_io_ready,
         tiz_prc_timer_ready, icer_timer_ready, 0);
    }
}
