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
#define goto_end_on_pa_error(expr)                              \
  do {                                                          \
    int pa_error = 0;                                           \
    pa_threaded_mainloop_lock (ap_prc->p_pa_loop_);             \
    if ((pa_error = (expr)) < 0)                                \
      {                                                         \
        TIZ_ERROR (handleOf (ap_prc),                           \
                   "[OMX_ErrorInsufficientResources] : "        \
                   "%s",  pa_strerror (pa_error));              \
        goto end;                                               \
      }                                                         \
  } while (0)

/* Forward declaration */
static OMX_ERRORTYPE pulsear_prc_deallocate_resources (void *ap_obj);

static void
pulse_output_context_state_cb(struct pa_context *context, void *userdata)
{
}

static void
pulse_output_subscribe_cb(pa_context *context, pa_subscription_event_type_t t,
                          uint32_t idx, void *userdata)
{
}

static void
deinit_pulseaudio_context (pulsear_prc_t *ap_prc)
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

static int
init_pulseaudio_context (pulsear_prc_t *ap_prc)
{
  int rc = PA_ERR_UNKNOWN;

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_pa_loop_);

  ap_prc->p_pa_context_ = pa_context_new (pa_threaded_mainloop_get_api (ap_prc->p_pa_loop_),
                                          ARATELIA_PCM_RENDERER_PULSEAUDIO_NAME);
  if (NULL == ap_prc->p_pa_context_)
    {
      TIZ_ERROR (handleOf (ap_prc), "[OMX_ErrorInsufficientResources] : "
                 "Unable to create the pulseaudio context.");
      goto end;
    }

  pa_context_set_state_callback(ap_prc->p_pa_context_,
                                pulse_output_context_state_cb, ap_prc);
  pa_context_set_subscribe_callback(ap_prc->p_pa_context_,
                                    pulse_output_subscribe_cb, ap_prc);

  goto_end_on_pa_error (pa_context_connect (ap_prc->p_pa_context_,
                                            NULL, /* Connect to the default server */
                                            (pa_context_flags_t)0,
                                            NULL));

  /* All good */
  rc = PA_OK;

 end:

  if (PA_OK != rc)
    {
      deinit_pulseaudio_context (ap_prc);
    }

  return rc;
}

static void
deinit_pulseaudio (pulsear_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  if (NULL != ap_prc->p_pa_loop_)
    {
      pa_threaded_mainloop_stop(ap_prc->p_pa_loop_);

      deinit_pulseaudio_context (ap_prc);

      pa_threaded_mainloop_free(ap_prc->p_pa_loop_);
      ap_prc->p_pa_loop_ = NULL;
    }
}

static OMX_ERRORTYPE
init_pulseaudio (pulsear_prc_t *ap_prc)
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
  goto_end_on_pa_error (init_pulseaudio_context(ap_prc));

  /* All good */
  rc = OMX_ErrorNone;

 end:

  pa_threaded_mainloop_unlock(ap_prc->p_pa_loop_);

  if (OMX_ErrorNone != rc)
    {
      deinit_pulseaudio (ap_prc);
    }

  return rc;
}

/*
 * pulsearprc
 */

static void *
pulsear_prc_ctor (void *ap_prc, va_list * app)
{
  pulsear_prc_t *p_prc = super_ctor (typeOf (ap_prc, "pulsearprc"), ap_prc, app);
  p_prc->p_pa_loop_ = NULL;
  p_prc->p_pa_context_ = NULL;
  p_prc->p_pa_stream_ = NULL;
  return p_prc;
}

static void *
pulsear_prc_dtor (void *ap_prc)
{
  (void) pulsear_prc_deallocate_resources (ap_prc);
  return super_dtor (typeOf (ap_prc, "pulsearprc"), ap_prc);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
pulsear_prc_allocate_resources (void *ap_prc, OMX_U32 a_pid)
{
  return init_pulseaudio (ap_prc);
}

static OMX_ERRORTYPE
pulsear_prc_deallocate_resources (void *ap_prc)
{
  deinit_pulseaudio (ap_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
pulsear_prc_prepare_to_transfer (void *ap_prc, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
pulsear_prc_transfer_and_process (void *ap_prc, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
pulsear_prc_stop_and_return (void *ap_prc)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
pulsear_prc_buffers_ready (const void *ap_prc)
{
  return OMX_ErrorNone;
}

/*
 * pulsear_prc_class
 */

static void *
pulsear_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "pulsearprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
pulsear_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * pulsearprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "pulsearprc_class", classOf (tizprc), sizeof (pulsear_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, pulsear_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return pulsearprc_class;
}

void *
pulsear_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * pulsearprc_class = tiz_get_type (ap_hdl, "pulsearprc_class");
  TIZ_LOG_CLASS (pulsearprc_class);
  void * pulsearprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (pulsearprc_class, "pulsearprc", tizprc, sizeof (pulsear_prc_t),
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
