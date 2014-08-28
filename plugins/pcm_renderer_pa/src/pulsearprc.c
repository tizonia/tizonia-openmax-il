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

static void pulseaudio_context_state_cback (struct pa_context *context,
                                            void *userdata)
{
}

static void pulseaudio_context_subscribe_cback (pa_context *context,
                                                pa_subscription_event_type_t t,
                                                uint32_t idx, void *userdata)
{
}

static void pulseaudio_stream_state_cback_handler (
    OMX_PTR ap_prc, tiz_event_pluggable_t *ap_event)
{
  pulsear_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  tiz_srv_soa_free (ap_prc, ap_event);
}

static void pulseaudio_stream_state_cback (pa_stream *stream, void *userdata)
{
  pulsear_prc_t *p_prc = userdata;
  assert (NULL != p_prc);
  tiz_event_pluggable_t *p_event
      = tiz_srv_soa_calloc (p_prc, sizeof(tiz_event_pluggable_t));
  if (NULL != p_event)
    {
      p_event->p_servant = p_prc;
      p_event->p_data = NULL;
      p_event->pf_hdlr = pulseaudio_stream_state_cback_handler;
      tiz_comp_event_pluggable (handleOf (p_prc), p_event);
    }
}

static void pulseaudio_stream_suspended_cback (pa_stream *stream,
                                               void *userdata)
{
}

static void pulseaudio_stream_write_cback (pa_stream *stream, size_t nbytes,
                                           void *userdata)
{
}

/* static void pulseaudio_stream_success_cback (pa_stream *s, int success, */
/*                                              void *userdata) */
/* { */
/* } */

static void deinit_pulseaudio_stream (pulsear_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
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

  return rc;
}

/* pa mainloop lock must have been locked before calling this function */
static int init_pulseaudio_stream (pulsear_prc_t *ap_prc)
{
  int rc = PA_ERR_UNKNOWN;
  pa_sample_spec spec;

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_pa_loop_);
  assert (NULL != ap_prc->p_pa_context_);

  if (NULL == ap_prc->p_pa_context_)
    {
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

      spec.format = PA_SAMPLE_S16NE;
      spec.rate = 44100;
      spec.channels = 2;

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
  else
    {
      /* All good */
      rc = PA_OK;
    }

  return rc;
}

/* pa mainloop lock must have been locked before calling this function */
static int init_pulseaudio_context (pulsear_prc_t *ap_prc)
{
  int rc = PA_ERR_UNKNOWN;

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_pa_loop_);

  if (NULL == ap_prc->p_pa_context_)
    {
      ap_prc->p_pa_context_
          = pa_context_new (pa_threaded_mainloop_get_api (ap_prc->p_pa_loop_),
                            ARATELIA_PCM_RENDERER_PULSEAUDIO_APP_NAME);
      goto_end_on_null (ap_prc->p_pa_context_);

      pa_context_set_state_callback (ap_prc->p_pa_context_,
                                     pulseaudio_context_state_cback, ap_prc);
      pa_context_set_subscribe_callback (
          ap_prc->p_pa_context_, pulseaudio_context_subscribe_cback, ap_prc);

      goto_end_on_pa_error (pa_context_connect (
          ap_prc->p_pa_context_, NULL, /* Connect to the default server */
          (pa_context_flags_t)0, NULL));

      /* All good */
      rc = PA_OK;

    end:

      if (PA_OK != rc)
        {
          deinit_pulseaudio_context (ap_prc);
        }
    }
  else
    {
      /* All good */
      rc = PA_OK;
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

  /* Instantiate the pulseaudio threaded main loop */
  ap_prc->p_pa_loop_ = pa_threaded_mainloop_new ();
  tiz_check_null_ret_oom (ap_prc->p_pa_loop_);

  pa_threaded_mainloop_lock (ap_prc->p_pa_loop_);

  /* Start the main loop thread */
  goto_end_on_pa_error (pa_threaded_mainloop_start (ap_prc->p_pa_loop_));

  /* Create the pulseaudio context and connect to it */
  goto_end_on_pa_error (init_pulseaudio_context (ap_prc));

  /* Create the pulsaudio playback stream  */
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

/*
 * pulsearprc
 */

static void *pulsear_prc_ctor (void *ap_prc, va_list *app)
{
  pulsear_prc_t *p_prc
      = super_ctor (typeOf (ap_prc, "pulsearprc"), ap_prc, app);
  p_prc->p_pa_loop_ = NULL;
  p_prc->p_pa_context_ = NULL;
  p_prc->p_pa_stream_ = NULL;
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
  return init_pulseaudio (ap_prc);
}

static OMX_ERRORTYPE pulsear_prc_deallocate_resources (void *ap_prc)
{
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
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE pulsear_prc_stop_and_return (void *ap_prc)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE pulsear_prc_buffers_ready (const void *ap_prc)
{
  return OMX_ErrorNone;
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
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return pulsearprc;
}
