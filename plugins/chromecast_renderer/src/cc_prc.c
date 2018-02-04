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
 * @file   cc_prc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL Chromecast renderer - base processor class
 * implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <tizplatform.h>

#include <tizport.h>
#include <tizport_decls.h>
#include <tizport-macros.h>
#include <tizkernel.h>

#include "cc_prc.h"
#include "cc_prc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE
cc_prc_deallocate_resources (void *);

static OMX_ERRORTYPE
release_buffer (cc_gmusic_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_inhdr_)
    {
      tiz_check_omx (tiz_krn_release_buffer (
        tiz_get_krn (handleOf (ap_prc)),
        ARATELIA_CHROMECAST_RENDERER_PORT_INDEX, ap_prc->p_inhdr_));
      ap_prc->p_inhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
retrieve_cc_session_configuration (cc_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamChromecastSession, &(ap_prc->cc_session_));
}

/*
 * cc_prc
 */

static void *
cc_prc_ctor (void * ap_obj, va_list * app)
{
  cc_prc_t * p_prc = super_ctor (typeOf (ap_obj, "cc_prc"), ap_obj, app);
  assert (p_prc);
  TIZ_INIT_OMX_STRUCT (p_prc->cc_session_);
  TIZ_INIT_OMX_STRUCT (p_prc->pl_skip_);
  p_prc->p_cc_ = NULL;
  p_prc->cc_cast_status_ = ETizCcCastStatusUnknown;
  p_prc->cc_media_status_ = ETizCcMediaStatusUnknown;
  p_prc->p_cc_display_title_ = NULL;
  p_prc->p_cc_err_msg_ = NULL;
  p_prc->uri_changed_ = false;
  p_prc->volume_ = ARATELIA_CHROMECAST_RENDERER_DEFAULT_VOLUME_VALUE;
  return p_prc;
}

static void *
cc_prc_dtor (void * ap_obj)
{
  (void) cc_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "cc_prc"), ap_obj);
}

static OMX_ERRORTYPE
cc_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  cc_prc_t * p_prc = ap_obj;
  assert (p_prc);
  tiz_check_omx (retrieve_gm_session_configuration (p_prc));
  tiz_check_omx (retrieve_playlist (p_prc));
  tiz_check_omx (retrieve_cc_session_configuration (p_prc));

  TIZ_TRACE (handleOf (p_prc), "cUserName  : [%s]",
             p_prc->gm_session_.cUserName);
  TIZ_TRACE (handleOf (p_prc), "cUserPassword  : [%s]",
             p_prc->gm_session_.cUserPassword);
  TIZ_TRACE (handleOf (p_prc), "cDeviceId  : [%s]",
             p_prc->gm_session_.cDeviceId);
  TIZ_TRACE (handleOf (p_prc), "cNameOrIpAddr  : [%s]",
             p_prc->cc_session_.cNameOrIpAddr);

  on_gmusic_error_ret_omx_oom (tiz_gmusic_init (
    &(p_prc->p_gm_), (const char *) p_prc->gm_session_.cUserName,
    (const char *) p_prc->gm_session_.cUserPassword,
    (const char *) p_prc->gm_session_.cDeviceId));

  tiz_check_omx (enqueue_playlist_items (p_prc));
  clear_stored_metadata (p_prc);
  tiz_check_omx (obtain_next_url (p_prc, 1));
  deliver_stored_metadata (p_prc);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_prc_deallocate_resources (void * ap_prc)
{
  cc_prc_t * p_prc = ap_prc;
  assert (p_prc);
  delete_uri (p_prc);
  tiz_gmusic_destroy (p_prc->p_gm_);
  p_prc->p_gm_ = NULL;
  tiz_cast_client_destroy (p_prc->p_cc_);
  p_prc->p_cc_ = NULL;
  tiz_mem_free (p_prc->p_cc_display_title_);
  p_prc->p_cc_display_title_ = NULL;
  tiz_mem_free (p_prc->p_cc_err_msg_);
  p_prc->p_cc_err_msg_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid)
{
  cc_prc_t * p_prc = ap_prc;
  assert (ap_prc);

  /* Lazy instantiation of the cast client object */
  if (!p_prc->p_cc_)
    {
      OMX_UUIDTYPE cc_uuid;
      tiz_cast_client_callbacks_t cast_cbacks
        = {cc_cast_status_cback, cc_media_status_cback, cc_error_status_cback};
      bzero (&(cc_uuid), 128);
      tiz_uuid_generate (&(cc_uuid));
      on_cc_error_ret_omx_oom (tiz_cast_client_init (
        &(p_prc->p_cc_), (const char *) p_prc->cc_session_.cNameOrIpAddr,
        &(cc_uuid), &cast_cbacks, p_prc));
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid)
{
  cc_prc_t * p_prc = ap_prc;
  assert (p_prc);
  assert (p_prc->p_cc_);
  assert (p_prc->p_uri_param_);
  assert ((const char *) p_prc->p_uri_param_->contentURI);
  TIZ_DEBUG (handleOf (p_prc), "transfer_and_process");
  if (p_prc->uri_changed_)
    {
      tiz_check_omx (load_next_url (p_prc));
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_prc_stop_and_return (void * ap_prc)
{
  cc_prc_t * p_prc = ap_prc;
  assert (p_prc);
  assert (p_prc->p_cc_);
  (void) tiz_cast_client_stop (p_prc->p_cc_);
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE
cc_prc_buffers_ready (const void * ap_prc)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_prc_pause (const void * ap_prc)
{
  cc_prc_t * p_prc = (cc_prc_t *) ap_prc;
  assert (p_prc);
  assert (p_prc->p_cc_);
  on_cc_error_ret_omx_oom (tiz_cast_client_pause (p_prc->p_cc_));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_prc_resume (const void * ap_prc)
{
  cc_prc_t * p_prc = (cc_prc_t *) ap_prc;
  assert (p_prc);
  assert (p_prc->p_cc_);
  on_cc_error_ret_omx_oom (tiz_cast_client_play (p_prc->p_cc_));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_prc_config_change (void * ap_prc, OMX_U32 a_pid, OMX_INDEXTYPE a_config_idx)
{
  cc_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_prc);
  if (ARATELIA_CHROMECAST_RENDERER_PORT_INDEX == a_pid)
    {
      if (OMX_IndexConfigAudioVolume == a_config_idx)
        {
          OMX_AUDIO_CONFIG_VOLUMETYPE volume;
          TIZ_INIT_OMX_PORT_STRUCT (volume,
                                    ARATELIA_CHROMECAST_RENDERER_PORT_INDEX);
          tiz_check_omx (
            tiz_api_GetConfig (tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
                               OMX_IndexConfigAudioVolume, &volume));
          TIZ_DEBUG (
            handleOf (p_prc),
            "[OMX_IndexConfigAudioVolume] : volume.sVolume.nValue = %ld",
            volume.sVolume.nValue);
          if (volume.sVolume.nValue
                <= ARATELIA_CHROMECAST_RENDERER_MAX_VOLUME_VALUE
              && volume.sVolume.nValue
                   >= ARATELIA_CHROMECAST_RENDERER_MIN_VOLUME_VALUE)
            {
              set_volume (p_prc, volume.sVolume.nValue);
            }
        }
      else if (OMX_IndexConfigAudioMute == a_config_idx)
        {
          OMX_AUDIO_CONFIG_MUTETYPE mute;
          TIZ_INIT_OMX_PORT_STRUCT (mute,
                                    ARATELIA_CHROMECAST_RENDERER_PORT_INDEX);
          tiz_check_omx (tiz_api_GetConfig (tiz_get_krn (handleOf (p_prc)),
                                            handleOf (p_prc),
                                            OMX_IndexConfigAudioMute, &mute));
          TIZ_DEBUG (handleOf (p_prc),
                     "[OMX_IndexConfigAudioMute] : bMute = [%s]",
                     (mute.bMute == OMX_FALSE ? "FALSE" : "TRUE"));
          toggle_mute (p_prc, mute.bMute == OMX_TRUE ? true : false);
        }
    }

  if (OMX_TizoniaIndexConfigPlaylistSkip == a_config_idx && p_prc->p_cc_)
    {
      TIZ_INIT_OMX_STRUCT (p_prc->pl_skip_);
      tiz_check_omx (tiz_api_GetConfig (
        tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
        OMX_TizoniaIndexConfigPlaylistSkip, &p_prc->pl_skip_));
      clear_stored_metadata (p_prc);
      p_prc->pl_skip_.nValue > 0 ? obtain_next_url (p_prc, 1)
                                 : obtain_next_url (p_prc, -1);
      deliver_stored_metadata (p_prc);
      /* Load the new URL */
      tiz_check_omx (load_next_url (p_prc));
    }
  return rc;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
cc_prc_port_flush (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  cc_prc_t * p_prc = (cc_prc_t *) ap_obj;
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE
cc_prc_port_disable (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  cc_prc_t * p_prc = (cc_prc_t *) ap_obj;
  assert (p_prc);
  /* Release any buffers held  */
  return release_buffer ((cc_prc_t *) ap_obj);
}

/*
 * cc_prc_class
 */

static void *
cc_prc_class_ctor (void * ap_obj, va_list * app)
{
  cc_prc_class_t * p_obj
    = super_ctor (typeOf (ap_obj, "cc_prc_class"), ap_obj, app);
  typedef void (*voidf) ();
  voidf selector = NULL;
  va_list ap;
  va_copy (ap, *app);

  /* NOTE: Start ignoring splint warnings in this section of code */
  /*@ignore@*/
  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      (void)method;
    }
  /*@end@*/
  /* NOTE: Stop ignoring splint warnings in this section  */

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

void *
cc_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * cc_prc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "cc_prc_class", classOf (tizprc),
     sizeof (cc_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return cc_prc_class;
}

void *
cc_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * cc_prc_class = tiz_get_type (ap_hdl, "cc_prc_class");
  TIZ_LOG_CLASS (cc_prc_class);
  void * filterprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_prc_class, "cc_prc", tizprc, sizeof (cc_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, cc_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, cc_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, cc_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, cc_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, cc_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, cc_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, cc_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_pause, cc_prc_pause,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_resume, cc_prc_resume,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_config_change, cc_prc_config_change,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, cc_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, cc_prc_port_disable,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return filterprc;
}
