/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   pulsearprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - PCM audio renderer based on pulseaudio processor
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <assert.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "pulsear.h"
#include "pulsearprc.h"
#include "pulsearprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.pcm_renderer.prc"
#endif

/* This is for convenience. This macro assumes the existence of an
   "ap_prc" local variable */
#define goto_end_on_pa_error(expr)                         \
  do                                                       \
    {                                                      \
      int pa_error = 0;                                    \
      if ((pa_error = (expr)) < 0)                         \
        {                                                  \
          TIZ_ERROR (handleOf (ap_prc),                    \
                     "[OMX_ErrorInsufficientResources] : " \
                     "%s",                                 \
                     pa_strerror (pa_error));              \
          goto end;                                        \
        }                                                  \
    }                                                      \
  while (0)

/* This is for convenience. This macro assumes the existence of an
   "ap_prc" local variable */
#define goto_end_on_null(expr)                             \
  do                                                       \
    {                                                      \
      if (NULL == (expr))                                  \
        {                                                  \
          TIZ_ERROR (handleOf (ap_prc),                    \
                     "[OMX_ErrorInsufficientResources] : " \
                     "Expression returned NULL.");         \
          goto end;                                        \
        }                                                  \
    }                                                      \
  while (0)

/* Forward declarations */
static OMX_ERRORTYPE pulsear_prc_deallocate_resources (void *ap_obj);
static int init_pulseaudio_context (pulsear_prc_t *ap_prc);

static OMX_STRING pulseaudio_context_state_to_str (
    const pa_context_state_t a_state)
{
  switch (a_state)
    {
      case PA_CONTEXT_UNCONNECTED:
        return (OMX_STRING) "PA_CONTEXT_UNCONNECTED";
      case PA_CONTEXT_CONNECTING:
        return (OMX_STRING) "PA_CONTEXT_CONNECTING";
      case PA_CONTEXT_AUTHORIZING:
        return (OMX_STRING) "PA_CONTEXT_AUTHORIZING";
      case PA_CONTEXT_SETTING_NAME:
        return (OMX_STRING) "PA_CONTEXT_SETTING_NAME";
      case PA_CONTEXT_READY:
        return (OMX_STRING) "PA_CONTEXT_READY";
      case PA_CONTEXT_FAILED:
        return (OMX_STRING) "PA_CONTEXT_FAILED";
      case PA_CONTEXT_TERMINATED:
        return (OMX_STRING) "PA_CONTEXT_TERMINATED";
      default:
        break;
    }

  return (OMX_STRING) "Unknown PA context state";
}

static OMX_STRING pulseaudio_stream_state_to_str (
    const pa_stream_state_t a_state)
{
  switch (a_state)
    {
      case PA_STREAM_UNCONNECTED:
        return (OMX_STRING) "PA_STREAM_UNCONNECTED";
      case PA_STREAM_CREATING:
        return (OMX_STRING) "PA_STREAM_CREATING";
      case PA_STREAM_READY:
        return (OMX_STRING) "PA_STREAM_READY";
      case PA_STREAM_FAILED:
        return (OMX_STRING) "PA_STREAM_FAILED";
      case PA_STREAM_TERMINATED:
        return (OMX_STRING) "PA_STREAM_TERMINATED";
      default:
        break;
    };
  return (OMX_STRING) "Unknown PA stream state";
}

static OMX_STRING pulseaudio_operation_state_to_str (
    const pa_operation_state_t a_state)
{
  switch (a_state)
    {
      case PA_OPERATION_RUNNING:
        return (OMX_STRING) "PA_OPERATION_RUNNING";
      case PA_OPERATION_DONE:
        return (OMX_STRING) "PA_OPERATION_DONE";
      case PA_OPERATION_CANCELLED:
        return (OMX_STRING) "PA_OPERATION_CANCELLED";
      default:
        break;
    };
  return (OMX_STRING) "Unknown PA operation state";
}

static OMX_BUFFERHEADERTYPE *get_header (pulsear_prc_t *ap_prc)
{
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  assert (NULL != ap_prc);

  if (!ap_prc->port_disabled_)
    {
      if (!ap_prc->p_inhdr_)
        {
          (void)tiz_krn_claim_buffer (tiz_get_krn (handleOf (ap_prc)),
                                      ARATELIA_PCM_RENDERER_PORT_INDEX, 0,
                                      &ap_prc->p_inhdr_);
          if (ap_prc->p_inhdr_)
            {
              TIZ_TRACE (handleOf (ap_prc),
                         "Claimed HEADER [%p]...nFilledLen [%d]",
                         ap_prc->p_inhdr_, ap_prc->p_inhdr_->nFilledLen);
            }
        }
      p_hdr = ap_prc->p_inhdr_;
    }
  return p_hdr;
}

static OMX_ERRORTYPE release_header (pulsear_prc_t *ap_prc)
{
  assert (NULL != ap_prc);

  if (ap_prc->p_inhdr_)
    {
      ap_prc->p_inhdr_->nOffset = 0;
      TIZ_PRINTF_DBG_YEL ("Releasing buffer [%p] with size [%u].",
                          ap_prc->p_inhdr_,
                          (unsigned int)ap_prc->p_inhdr_->nFilledLen);
      tiz_check_omx_err (tiz_krn_release_buffer (
          tiz_get_krn (handleOf (ap_prc)), ARATELIA_PCM_RENDERER_PORT_INDEX,
          ap_prc->p_inhdr_));
      ap_prc->p_inhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE buffer_emptied (pulsear_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_inhdr_);
  assert (ap_prc->p_inhdr_->nFilledLen == 0);

  TIZ_TRACE (handleOf (ap_prc), "Releasing HEADER [%p] emptied",
             ap_prc->p_inhdr_);

  if ((ap_prc->p_inhdr_->nFlags & OMX_BUFFERFLAG_EOS) != 0)
    {
      TIZ_DEBUG (handleOf (ap_prc), "OMX_BUFFERFLAG_EOS in HEADER [%p]",
                 ap_prc->p_inhdr_);
      tiz_srv_issue_event ((OMX_PTR)ap_prc, OMX_EventBufferFlag, 0,
                           ap_prc->p_inhdr_->nFlags, NULL);
    }

  return release_header (ap_prc);
}

static OMX_ERRORTYPE render_pcm_data (pulsear_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  while (NULL != (p_hdr = get_header (ap_prc)) && ap_prc->pa_nbytes_ > 0)
    {
      TIZ_TRACE (handleOf (ap_prc), "HEADER [%p] nFilledLen [%d]", p_hdr,
                 p_hdr->nFilledLen);

      if (p_hdr->nFilledLen > 0)
        {
          const int bytes_to_write
              = MIN (ap_prc->pa_nbytes_, p_hdr->nFilledLen);
          assert (NULL != ap_prc->p_pa_loop_);
          assert (NULL != ap_prc->p_pa_context_);

          pa_threaded_mainloop_lock (ap_prc->p_pa_loop_);
          int result = pa_stream_write (
              ap_prc->p_pa_stream_, p_hdr->pBuffer + p_hdr->nOffset,
              bytes_to_write, NULL, 0, PA_SEEK_RELATIVE);
          TIZ_TRACE (handleOf (ap_prc),
                     "bytes_to_write [%d] pa_stream_write [%d]", bytes_to_write,
                     result);
          /* TODO : Check return code */
          (void)result;
          pa_threaded_mainloop_unlock (ap_prc->p_pa_loop_);
          p_hdr->nFilledLen -= bytes_to_write;
          p_hdr->nOffset += bytes_to_write;
          ap_prc->pa_nbytes_ -= bytes_to_write;
        }

      if (0 == p_hdr->nFilledLen)
        {
          rc = buffer_emptied (ap_prc);
          p_hdr = NULL;
        }
    }

  return rc;
}

static void pulseaudio_context_state_cback (struct pa_context *ap_context,
                                            void *ap_userdata)
{
  pulsear_prc_t *p_prc = ap_userdata;

  assert (NULL != p_prc);
  TIZ_TRACE (handleOf (p_prc), "[%s]", pulseaudio_context_state_to_str (
                                           pa_context_get_state (ap_context)));

  switch (pa_context_get_state (ap_context))
    {
      case PA_CONTEXT_READY:
      case PA_CONTEXT_TERMINATED:
      case PA_CONTEXT_FAILED:
        pa_threaded_mainloop_signal (p_prc->p_pa_loop_, 0);
        break;

      case PA_CONTEXT_UNCONNECTED:
      case PA_CONTEXT_CONNECTING:
      case PA_CONTEXT_AUTHORIZING:
      case PA_CONTEXT_SETTING_NAME:
      default:
        break;
    }
}

static void pulseaudio_context_subscribe_cback (
    pa_context *ap_context, pa_subscription_event_type_t a_event,
    uint32_t a_idx, void *ap_userdata)
{
  TIZ_TRACE (handleOf (ap_userdata), "");
}

static void pulseaudio_stream_state_cback_handler (
    OMX_PTR ap_prc, tiz_event_pluggable_t *ap_event)
{
  pulsear_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (p_prc->p_pa_loop_ && p_prc->p_pa_stream_ && ap_event->p_data)
    {
      p_prc->pa_state_ = *((pa_stream_state_t *)(ap_event->p_data));
      TIZ_TRACE (handleOf (ap_prc), "PA STREAM STATE : [%s]",
                 pulseaudio_stream_state_to_str (p_prc->pa_state_));
    }
  tiz_srv_soa_free (ap_prc, ap_event->p_data);
  tiz_srv_soa_free (ap_prc, ap_event);
}

static void pulseaudio_stream_state_cback (pa_stream *stream, void *userdata)
{
  pulsear_prc_t *p_prc = userdata;
  assert (NULL != p_prc);

  {
    tiz_event_pluggable_t *p_event
        = tiz_srv_soa_calloc (p_prc, sizeof(tiz_event_pluggable_t));
    if (NULL != p_event)
      {
        p_event->p_servant = p_prc;
        p_event->p_data = tiz_srv_soa_calloc (p_prc, sizeof(pa_stream_state_t));
        p_event->pf_hdlr = pulseaudio_stream_state_cback_handler;
        if (p_event->p_data)
          {
            *((pa_stream_state_t *)(p_event->p_data))
                = pa_stream_get_state (p_prc->p_pa_stream_);
          }
        tiz_comp_event_pluggable (handleOf (p_prc), p_event);
      }
  }
}

static void pulseaudio_stream_suspended_cback (pa_stream *stream,
                                               void *userdata)
{
  pulsear_prc_t *p_prc = userdata;
  assert (NULL != p_prc);
  assert (stream == p_prc->p_pa_stream_ || NULL == p_prc->p_pa_stream_);
  assert (NULL != p_prc->p_pa_loop_);
  TIZ_TRACE (handleOf (p_prc), "");
}

static void pulseaudio_stream_write_cback_handler (
    OMX_PTR ap_prc, tiz_event_pluggable_t *ap_event)
{
  pulsear_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);
  assert (NULL != ap_event->p_data);

  if (p_prc->p_pa_loop_)
    {
      assert (NULL != ap_event->p_data);
      TIZ_TRACE (handleOf (ap_prc), "BEFORE pa_nbytes_ [%d]",
                 p_prc->pa_nbytes_);
      p_prc->pa_nbytes_ += *((size_t *)ap_event->p_data);
      TIZ_TRACE (handleOf (ap_prc), "AFTER pa_nbytes_ [%d]", p_prc->pa_nbytes_);
      if (PA_STREAM_READY == p_prc->pa_state_)
        {
          (void)render_pcm_data (p_prc);
        }
    }
  tiz_srv_soa_free (ap_prc, ap_event->p_data);
  tiz_srv_soa_free (ap_prc, ap_event);
}

static void pulseaudio_stream_write_cback (pa_stream *stream, size_t nbytes,
                                           void *userdata)
{
  pulsear_prc_t *p_prc = userdata;
  assert (NULL != p_prc);

  if (p_prc->p_pa_loop_)
    {
      tiz_event_pluggable_t *p_event
          = tiz_srv_soa_calloc (p_prc, sizeof(tiz_event_pluggable_t));
      if (NULL != p_event)
        {
          p_event->p_servant = p_prc;
          p_event->p_data = tiz_srv_soa_calloc (p_prc, sizeof(nbytes));
          if (p_event->p_data)
            {
              *((size_t *)(p_event->p_data)) = nbytes;
            }
          p_event->pf_hdlr = pulseaudio_stream_write_cback_handler;
          tiz_comp_event_pluggable (handleOf (p_prc), p_event);
        }
      pa_threaded_mainloop_signal (p_prc->p_pa_loop_, 0);
    }
}

static void pulseaudio_stream_success_cback (pa_stream *s, int success,
                                             void *userdata)
{
  pulsear_prc_t *p_prc = userdata;
  assert (NULL != p_prc);
  assert (NULL != p_prc->p_pa_loop_);
  pa_threaded_mainloop_signal (p_prc->p_pa_loop_, 0);
}

static void deinit_pulseaudio_stream (pulsear_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  TIZ_TRACE (handleOf (ap_prc), "");
  if (NULL != ap_prc->p_pa_stream_)
    {
      pa_stream_set_suspended_callback (ap_prc->p_pa_stream_, NULL, NULL);
      pa_stream_set_state_callback (ap_prc->p_pa_stream_, NULL, NULL);
      pa_stream_set_write_callback (ap_prc->p_pa_stream_, NULL, NULL);
      pa_stream_disconnect (ap_prc->p_pa_stream_);
      pa_stream_unref (ap_prc->p_pa_stream_);
      ap_prc->p_pa_stream_ = NULL;
    }
}

static void deinit_pulseaudio_context (pulsear_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  TIZ_TRACE (handleOf (ap_prc), "");
  if (NULL != ap_prc->p_pa_context_)
    {
      pa_context_set_state_callback (ap_prc->p_pa_context_, NULL, NULL);
      pa_context_set_subscribe_callback (ap_prc->p_pa_context_, NULL, NULL);
      pa_context_disconnect (ap_prc->p_pa_context_);
      pa_context_unref (ap_prc->p_pa_context_);
      ap_prc->p_pa_context_ = NULL;
    }
}

static void deinit_pulseaudio (pulsear_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  TIZ_TRACE (handleOf (ap_prc), "");
  if (NULL != ap_prc->p_pa_loop_)
    {
      pa_threaded_mainloop_stop (ap_prc->p_pa_loop_);
      deinit_pulseaudio_stream (ap_prc);
      deinit_pulseaudio_context (ap_prc);
      pa_threaded_mainloop_free (ap_prc->p_pa_loop_);
      ap_prc->p_pa_loop_ = NULL;
    }
}

static bool await_pulseaudio_context_connection (pulsear_prc_t *ap_prc)
{
  int rc = PA_ERR_UNKNOWN;
  bool done = false;

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_pa_loop_);

  TIZ_TRACE (handleOf (ap_prc), "");

  goto_end_on_pa_error (init_pulseaudio_context (ap_prc));
  assert (NULL != ap_prc->p_pa_context_);

  while (!done)
    {
      switch (pa_context_get_state (ap_prc->p_pa_context_))
        {
          case PA_CONTEXT_READY:
            /* good */
            rc = PA_OK;
            done = true;
            break;

          case PA_CONTEXT_UNCONNECTED:
          case PA_CONTEXT_TERMINATED:
          case PA_CONTEXT_FAILED:
            /* not good */
            done = true;
            break;

          case PA_CONTEXT_CONNECTING:
          case PA_CONTEXT_AUTHORIZING:
          case PA_CONTEXT_SETTING_NAME:
            /* wait */
            pa_threaded_mainloop_wait (ap_prc->p_pa_loop_);
            break;
          default:
            break;
        };
    }

end:

  if (PA_OK != rc)
    {
      deinit_pulseaudio_context (ap_prc);
    }
  TIZ_TRACE (handleOf (ap_prc), "[%s]",
             (rc == PA_OK ? "PA_OK" : "PA_ERR_UNKNOWN"));

  return rc;
}

static int init_pulseaudio_sample_spec (pulsear_prc_t *ap_prc,
                                        pa_sample_spec *ap_spec)
{
  OMX_ERRORTYPE omx_rc = OMX_ErrorNone;
  int rc = PA_ERR_UNKNOWN;

  assert (NULL != ap_prc);
  assert (NULL != ap_spec);

  /* Retrieve pcm params from the input port */
  TIZ_INIT_OMX_PORT_STRUCT (ap_prc->pcmmode_, ARATELIA_PCM_RENDERER_PORT_INDEX);
  if (OMX_ErrorNone != (omx_rc = tiz_api_GetParameter (
                            tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                            OMX_IndexParamAudioPcm, &ap_prc->pcmmode_)))
    {
      TIZ_ERROR (handleOf (ap_prc), "[%s]", tiz_err_to_str (omx_rc));
    }
  else
    {
      TIZ_NOTICE (
          handleOf (ap_prc),
          "nChannels = [%d] nBitPerSample = [%d] "
          "nSamplingRate = [%d] eNumData = [%d] eEndian = [%d] "
          "bInterleaved = [%s] ePCMMode = [%d]",
          ap_prc->pcmmode_.nChannels, ap_prc->pcmmode_.nBitPerSample,
          ap_prc->pcmmode_.nSamplingRate, ap_prc->pcmmode_.eNumData,
          ap_prc->pcmmode_.eEndian,
          ap_prc->pcmmode_.bInterleaved == OMX_TRUE ? "OMX_TRUE" : "OMX_FALSE",
          ap_prc->pcmmode_.ePCMMode);

      if (ap_prc->pcmmode_.nBitPerSample == 24)
        {
          ap_spec->format = PA_SAMPLE_S24NE;
        }
      /* NOTE: this is a hack to allow float pcm streams coming from the the
         vorbis or opusfile decoders */
      else if (ap_prc->pcmmode_.nBitPerSample == 32)
        {
          ap_spec->format = PA_SAMPLE_S32NE;
        }
      else
        {
          ap_spec->format = PA_SAMPLE_S16NE;
        }

      ap_spec->rate = ap_prc->pcmmode_.nSamplingRate;
      ap_spec->channels = ap_prc->pcmmode_.nChannels;

      /* all good */
      rc = PA_OK;
    }

  return rc;
}

/* Pulseaudio mainloop lock must have been acquired before calling this
   function */
static int init_pulseaudio_stream (pulsear_prc_t *ap_prc)
{
  int rc = PA_ERR_UNKNOWN;

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_pa_loop_);
  assert (NULL != ap_prc->p_pa_context_);

  TIZ_TRACE (handleOf (ap_prc), "");

  if (ap_prc->p_pa_stream_)
    {
      /* Nothing to do here. All good */
      rc = PA_OK;
      return rc;
    }

  {
    pa_sample_spec spec;
    switch (pa_context_get_state (ap_prc->p_pa_context_))
      {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_TERMINATED:
        case PA_CONTEXT_FAILED:
          deinit_pulseaudio_context (ap_prc);
          break;
        case PA_CONTEXT_READY:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
          break;
        default:
          break;
      };

    goto_end_on_pa_error (await_pulseaudio_context_connection (ap_prc));

    goto_end_on_pa_error (init_pulseaudio_sample_spec (ap_prc, &spec));

    ap_prc->p_pa_stream_ = pa_stream_new (
        ap_prc->p_pa_context_, ARATELIA_PCM_RENDERER_PULSEAUDIO_STREAM_NAME,
        &spec, NULL);
    goto_end_on_null (ap_prc->p_pa_stream_);

    pa_stream_set_suspended_callback (
        ap_prc->p_pa_stream_, pulseaudio_stream_suspended_cback, ap_prc);
    pa_stream_set_state_callback (ap_prc->p_pa_stream_,
                                  pulseaudio_stream_state_cback, ap_prc);
    pa_stream_set_write_callback (ap_prc->p_pa_stream_,
                                  pulseaudio_stream_write_cback, ap_prc);

    goto_end_on_pa_error (pa_stream_connect_playback (
        ap_prc->p_pa_stream_,
        ARATELIA_PCM_RENDERER_PULSEAUDIO_SINK_NAME, /* Name of the sink to
                                                       connect to, or NULL for
                                                       default */
        NULL,   /* Buffering attributes, or NULL for default */
        0,      /* Additional flags, or 0 for default */
        NULL,   /* Initial volume, or NULL for default */
        NULL)); /* Synchronize this stream with the specified one, or NULL for
                   a standalone stream  */

    /* All good */
    rc = PA_OK;

  end:

    if (PA_OK != rc)
      {
        deinit_pulseaudio_stream (ap_prc);
      }
  }

  return rc;
}

/* Pulseaudio mainloop lock must have been acquired before calling this
   function */
static int init_pulseaudio_context (pulsear_prc_t *ap_prc)
{
  int rc = PA_ERR_UNKNOWN;

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_pa_loop_);

  TIZ_TRACE (handleOf (ap_prc), "p_pa_context_= [%p]", ap_prc->p_pa_context_);
  if (ap_prc->p_pa_context_)
    {
      /* Nothing to do here. All good */
      rc = PA_OK;
      return rc;
    }

  /* Instantiate a pulsaudio context */
  ap_prc->p_pa_context_
      = pa_context_new (pa_threaded_mainloop_get_api (ap_prc->p_pa_loop_),
                        ARATELIA_PCM_RENDERER_PULSEAUDIO_APP_NAME);
  goto_end_on_null (ap_prc->p_pa_context_);

  /* Establish the state change callback */
  pa_context_set_state_callback (ap_prc->p_pa_context_,
                                 pulseaudio_context_state_cback, ap_prc);
  /* Establish the pulseaudio server event subscription callback */
  pa_context_set_subscribe_callback (
      ap_prc->p_pa_context_, pulseaudio_context_subscribe_cback, ap_prc);

  /* Connect the context to the pulseaudio server */
  goto_end_on_pa_error (pa_context_connect (
      ap_prc->p_pa_context_, NULL, /* Connect to the default server */
      (pa_context_flags_t)0, NULL));

  /* All good */
  rc = PA_OK;
  TIZ_TRACE (handleOf (ap_prc), "[PA_OK]");

end:

  if (PA_OK != rc)
    {
      deinit_pulseaudio_context (ap_prc);
    }

  return rc;
}

static OMX_ERRORTYPE init_pulseaudio (pulsear_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;

  setenv ("PULSE_PROP_media.role", "music", true);
  setenv ("PULSE_PROP_application.icon_name", "tizonia", true);

  assert (NULL != ap_prc);
  assert (NULL == ap_prc->p_pa_loop_);
  assert (NULL == ap_prc->p_pa_context_);

  TIZ_TRACE (handleOf (ap_prc), "");

  /* Instantiate the pulseaudio threaded main loop */
  ap_prc->p_pa_loop_ = pa_threaded_mainloop_new ();
  tiz_check_null_ret_oom (ap_prc->p_pa_loop_);

  pa_threaded_mainloop_lock (ap_prc->p_pa_loop_);

  /* Start the main loop thread */
  goto_end_on_pa_error (pa_threaded_mainloop_start (ap_prc->p_pa_loop_));

  /* Create the pulseaudio context and connect it to the server */
  goto_end_on_pa_error (init_pulseaudio_context (ap_prc));

  /* Create a pulsaudio playback stream */
  goto_end_on_pa_error (init_pulseaudio_stream (ap_prc));

  /* All good */
  rc = OMX_ErrorNone;

end:

  pa_threaded_mainloop_unlock (ap_prc->p_pa_loop_);

  if (OMX_ErrorNone != rc)
    {
      deinit_pulseaudio (ap_prc);
    }

  return rc;
}

static bool pulseaudio_wait_for_operation (pulsear_prc_t *ap_prc,
                                           struct pa_operation *ap_op)
{
  pa_operation_state_t op_state = PA_OPERATION_CANCELLED;

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_pa_loop_);
  assert (NULL != ap_op);

  while (PA_OPERATION_RUNNING == (op_state = pa_operation_get_state (ap_op)))
    {
      pa_threaded_mainloop_wait (ap_prc->p_pa_loop_);
    }
  pa_operation_unref (ap_op);
  TIZ_TRACE (handleOf (ap_prc), "PA operation state [%s]",
             pulseaudio_operation_state_to_str (op_state));
  return (PA_OPERATION_DONE == op_state);
}

static inline OMX_ERRORTYPE do_flush (pulsear_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  if (ap_prc->p_pa_loop_ && ap_prc->p_pa_stream_ && PA_STREAM_READY
                                                    == ap_prc->pa_state_)
    {
      pa_operation *p_op = NULL;
      pa_threaded_mainloop_lock (ap_prc->p_pa_loop_);
      p_op = pa_stream_flush (ap_prc->p_pa_stream_,
                              pulseaudio_stream_success_cback, ap_prc);
      if (p_op)
        {
          if (!pulseaudio_wait_for_operation (ap_prc, p_op))
            {
              TIZ_ERROR (handleOf (ap_prc), "Operation wait failed.");
            }
        }
      pa_threaded_mainloop_unlock (ap_prc->p_pa_loop_);
    }
  /* Release any buffers held  */
  return release_header (ap_prc);
}

static bool set_pa_sink_volume (pulsear_prc_t *ap_prc, const long a_volume)
{
  bool rc = false;
  assert (NULL != ap_prc);

  if (PA_STREAM_READY == ap_prc->pa_state_ && ap_prc->p_pa_loop_
      && ap_prc->p_pa_context_ && ap_prc->p_pa_stream_)
    {
      struct pa_cvolume cvolume;
      pa_operation *p_op = NULL;

      (void)pa_cvolume_set (&cvolume, ap_prc->pa_vol_.channels,
                            (pa_volume_t)a_volume * PA_VOLUME_NORM / 100 + 0.5);
      pa_threaded_mainloop_lock (ap_prc->p_pa_loop_);
      p_op = pa_context_set_sink_input_volume (
          ap_prc->p_pa_context_, pa_stream_get_index (ap_prc->p_pa_stream_),
          &cvolume, NULL, NULL);
      if (!p_op)
        {
          TIZ_TRACE (handleOf (ap_prc), "Unable to set pulsaudio volume");
        }
      else
        {
          rc = true;
          ap_prc->pa_vol_ = cvolume;
        }
      pa_threaded_mainloop_unlock (ap_prc->p_pa_loop_);
    }
  return rc;
}

static void toggle_mute (pulsear_prc_t *ap_prc, const bool a_mute)
{
  assert (NULL != ap_prc);

  {
    long new_volume = (a_mute ? 0 : ap_prc->volume_);
    TIZ_TRACE (handleOf (ap_prc), "new volume = %ld - ap_prc->volume_ [%d]",
               new_volume, ap_prc->volume_);
    set_pa_sink_volume (ap_prc, new_volume);
  }
}

static void set_volume (pulsear_prc_t *ap_prc, const long a_volume)
{
  if (set_pa_sink_volume (ap_prc, a_volume))
    {
      assert (NULL != ap_prc);
      ap_prc->volume_ = a_volume;
      TIZ_TRACE (handleOf (ap_prc), "ap_prc->volume_ = %ld", ap_prc->volume_);
    }
}

static void prepare_volume_ramp (pulsear_prc_t *ap_prc)
{
  pa_volume_t vol = ARATELIA_PCM_RENDERER_DEFAULT_VOLUME_VALUE;
  assert (NULL != ap_prc);
  (void)pa_cvolume_init (&(ap_prc->pa_vol_));
  (void)pa_cvolume_set (&(ap_prc->pa_vol_), ap_prc->pcmmode_.nChannels, vol);
  ap_prc->ramp_volume_ = ARATELIA_PCM_RENDERER_DEFAULT_VOLUME_VALUE;
  ap_prc->ramp_step_count_ = ARATELIA_PCM_RENDERER_DEFAULT_RAMP_STEP_COUNT;
  ap_prc->ramp_step_ = (double)ap_prc->ramp_volume_
                       / (double)ap_prc->ramp_step_count_;
  TIZ_TRACE (handleOf (ap_prc), "ramp_step_ = [%d] ramp_step_count_ = [%d]",
             ap_prc->ramp_step_, ap_prc->ramp_step_count_);
}

static OMX_ERRORTYPE start_volume_ramp (pulsear_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_ev_timer_);
  ap_prc->ramp_volume_ = 0;
  TIZ_TRACE (handleOf (ap_prc), "ramp_volume_ = [%d]", ap_prc->ramp_volume_);
  tiz_check_omx_err (tiz_event_timer_start (ap_prc->p_ev_timer_));
  return OMX_ErrorNone;
}

static void stop_volume_ramp (pulsear_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  if (NULL != ap_prc->p_ev_timer_)
    {
      (void)tiz_event_timer_stop (ap_prc->p_ev_timer_);
    }
}

static OMX_ERRORTYPE apply_ramp_step (pulsear_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  if (ap_prc->ramp_step_count_-- > 0)
    {
      ap_prc->ramp_volume_ += ap_prc->ramp_step_;
      TIZ_TRACE (handleOf (ap_prc), "ramp_volume_ = [%d]",
                 ap_prc->ramp_volume_);
      set_volume (ap_prc, ap_prc->ramp_volume_);
    }
  else
    {
      stop_volume_ramp (ap_prc);
    }
  return OMX_ErrorNone;
}

/*
 * pulsearprc
 */

static void *pulsear_prc_ctor (void *ap_prc, va_list *app)
{
  pulsear_prc_t *p_prc
      = super_ctor (typeOf (ap_prc, "pulsearprc"), ap_prc, app);
  p_prc->p_inhdr_ = NULL;
  p_prc->port_disabled_ = false;
  p_prc->p_pa_loop_ = NULL;
  p_prc->p_pa_context_ = NULL;
  p_prc->p_pa_stream_ = NULL;
  p_prc->pa_state_ = PA_STREAM_UNCONNECTED;
  p_prc->pa_nbytes_ = 0;
  p_prc->p_ev_timer_ = NULL;
  p_prc->gain_ = ARATELIA_PCM_RENDERER_DEFAULT_GAIN_VALUE;
  p_prc->volume_ = ARATELIA_PCM_RENDERER_DEFAULT_VOLUME_VALUE;
  p_prc->ramp_step_ = 0;
  p_prc->ramp_step_count_ = ARATELIA_PCM_RENDERER_DEFAULT_RAMP_STEP_COUNT;
  p_prc->ramp_volume_ = 0;
  return p_prc;
}

static void *pulsear_prc_dtor (void *ap_prc)
{
  (void)pulsear_prc_deallocate_resources (ap_prc);
  return super_dtor (typeOf (ap_prc, "pulsearprc"), ap_prc);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE pulsear_prc_allocate_resources (void *ap_prc,
                                                     OMX_U32 a_pid)
{
  pulsear_prc_t *p_prc = ap_prc;

  assert (NULL != p_prc);
  assert (NULL == p_prc->p_ev_timer_);

  tiz_check_omx_err (tiz_event_timer_init (
      &(p_prc->p_ev_timer_), handleOf (p_prc), tiz_comp_event_timer, p_prc));
  tiz_event_timer_set (p_prc->p_ev_timer_, 0.2, 0.2);
  return init_pulseaudio (ap_prc);
}

static OMX_ERRORTYPE pulsear_prc_deallocate_resources (void *ap_prc)
{
  pulsear_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  tiz_event_timer_destroy (p_prc->p_ev_timer_);
  p_prc->p_ev_timer_ = NULL;
  deinit_pulseaudio (ap_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE pulsear_prc_prepare_to_transfer (void *ap_prc,
                                                      OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE pulsear_prc_transfer_and_process (void *ap_prc,
                                                       OMX_U32 a_pid)
{
  pulsear_prc_t *p_prc = ap_prc;
  prepare_volume_ramp (p_prc);
  tiz_check_omx_err (start_volume_ramp (p_prc));
  tiz_check_omx_err (apply_ramp_step (p_prc));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE pulsear_prc_stop_and_return (void *ap_prc)
{
  return do_flush (ap_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE pulsear_prc_buffers_ready (const void *ap_prc)
{
  pulsear_prc_t *p_prc = (pulsear_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_prc);

  if (PA_STREAM_READY == p_prc->pa_state_)
    {
      rc = render_pcm_data (p_prc);
    }
  return rc;
}

static OMX_ERRORTYPE pulsear_prc_timer_ready (void *ap_prc,
                                              tiz_event_timer_t *ap_ev_timer,
                                              void *ap_arg)
{
  TIZ_TRACE (handleOf (ap_prc), "Received timer event");
  return apply_ramp_step (ap_prc);
}

static OMX_ERRORTYPE pulsear_prc_pause (const void *ap_prc)
{
  pulsear_prc_t *p_prc = (pulsear_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != p_prc);
  TIZ_TRACE (handleOf (p_prc), "PAUSED...");
  return rc;
}

static OMX_ERRORTYPE pulsear_prc_resume (const void *ap_prc)
{
  pulsear_prc_t *p_prc = (pulsear_prc_t *)ap_prc;
  assert (NULL != p_prc);
  TIZ_TRACE (handleOf (p_prc), "RESUMING ALSA device...");
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE pulsear_prc_port_flush (const void *ap_obj,
                                             OMX_U32 TIZ_UNUSED (a_pid))
{
  return do_flush ((pulsear_prc_t *)ap_obj);
}

static OMX_ERRORTYPE pulsear_prc_port_disable (const void *ap_prc,
                                               OMX_U32 TIZ_UNUSED (a_pid))
{
  pulsear_prc_t *p_prc = (pulsear_prc_t *)ap_prc;
  assert (NULL != p_prc);
  TIZ_TRACE (handleOf (p_prc), "Received port disable");
  stop_volume_ramp (p_prc);
  p_prc->port_disabled_ = true;
  /* Release any buffers held  */
  return release_header ((pulsear_prc_t *)ap_prc);
}

static OMX_ERRORTYPE pulsear_prc_port_enable (const void *ap_obj,
                                              OMX_U32 TIZ_UNUSED (a_pid))
{
  pulsear_prc_t *p_prc = (pulsear_prc_t *)ap_obj;
  assert (NULL != p_prc);
  TIZ_TRACE (handleOf (p_prc), "Received port emable");
  p_prc->port_disabled_ = false;
  /*   if (NULL != p_prc->p_pcm_hdl) */
  {
    /*       log_alsa_pcm_state (p_prc); */
    /*       tiz_check_omx_err (pulsear_prc_prepare_to_transfer (p_prc,
     * OMX_ALL)); */
    /*       tiz_check_omx_err (pulsear_prc_transfer_and_process (p_prc,
     * OMX_ALL)); */
  }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE pulsear_prc_config_change (void *ap_obj, OMX_U32 a_pid,
                                                OMX_INDEXTYPE a_config_idx)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  pulsear_prc_t *p_prc = ap_obj;

  assert (NULL != ap_obj);

  if (ARATELIA_PCM_RENDERER_PORT_INDEX == a_pid)
    {
      if (OMX_IndexConfigAudioVolume == a_config_idx)
        {
          OMX_AUDIO_CONFIG_VOLUMETYPE volume;
          TIZ_INIT_OMX_PORT_STRUCT (volume, ARATELIA_PCM_RENDERER_PORT_INDEX);
          tiz_check_omx_err (tiz_api_GetConfig (
              tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
              OMX_IndexConfigAudioVolume, &volume));
          TIZ_TRACE (
              handleOf (p_prc),
              "[OMX_IndexConfigAudioVolume] : volume.sVolume.nValue = %ld",
              volume.sVolume.nValue);
          if (volume.sVolume.nValue <= ARATELIA_PCM_RENDERER_MAX_VOLUME_VALUE
              && volume.sVolume.nValue
                 >= ARATELIA_PCM_RENDERER_MIN_VOLUME_VALUE)
            {
              /* TODO: Volume should be done by adjusting the gain, not ALSA's
               * master volume! */
              set_volume (p_prc, volume.sVolume.nValue);
            }
        }
      else if (OMX_IndexConfigAudioMute == a_config_idx)
        {
          OMX_AUDIO_CONFIG_MUTETYPE mute;
          TIZ_INIT_OMX_PORT_STRUCT (mute, ARATELIA_PCM_RENDERER_PORT_INDEX);
          tiz_check_omx_err (tiz_api_GetConfig (
              tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
              OMX_IndexConfigAudioMute, &mute));
          /* TODO: Volume should be done by adjusting the gain, not ALSA's
           * master volume! */
          TIZ_TRACE (handleOf (p_prc),
                     "[OMX_IndexConfigAudioMute] : bMute = [%s]",
                     (mute.bMute == OMX_FALSE ? "FALSE" : "TRUE"));
          toggle_mute (p_prc, mute.bMute == OMX_TRUE ? true : false);
        }
    }
  return rc;
}

/*
 * pulsear_prc_class
 */

static void *pulsear_prc_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "pulsearprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *pulsear_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *pulsearprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizprc), "pulsearprc_class", classOf (tizprc),
       sizeof(pulsear_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, pulsear_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return pulsearprc_class;
}

void *pulsear_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *pulsearprc_class = tiz_get_type (ap_hdl, "pulsearprc_class");
  TIZ_LOG_CLASS (pulsearprc_class);
  void *pulsearprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (pulsearprc_class, "pulsearprc", tizprc, sizeof(pulsear_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, pulsear_prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, pulsear_prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, pulsear_prc_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, pulsear_prc_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, pulsear_prc_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, pulsear_prc_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, pulsear_prc_stop_and_return,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, pulsear_prc_buffers_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_timer_ready, pulsear_prc_timer_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_pause, pulsear_prc_pause,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_resume, pulsear_prc_resume,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_flush, pulsear_prc_port_flush,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_disable, pulsear_prc_port_disable,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_enable, pulsear_prc_port_enable,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_config_change, pulsear_prc_config_change,
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return pulsearprc;
}
