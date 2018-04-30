/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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

#include <tizcasttypes.h>

#include "chromecastrnd.h"
#include "cc_prc.h"
#include "cc_prc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer.prc"
#endif

#define CONTENT_TYPE "audio/mpeg"
#define DISPLAY_TITLE "Tizonia Audio Stream"

typedef struct cc_status_event_data
{
  unsigned int status;
  int volume;
  char * p_err_msg;
} cc_status_event_data_t;

#define on_cc_error_ret_omx_oom(expr)                                        \
  do                                                                         \
    {                                                                        \
      tiz_cast_error_t cc_error = 0;                                         \
      if (TIZ_CAST_SUCCESS != (cc_error = (expr)))                           \
        {                                                                    \
          TIZ_ERROR (handleOf (p_prc),                                       \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "libtizcastclient");                                    \
          return OMX_ErrorInsufficientResources;                             \
        }                                                                    \
    }                                                                        \
  while (0)

/* Forward declarations */
static OMX_ERRORTYPE
prc_deallocate_resources (void *);
static OMX_ERRORTYPE
load_next_url (cc_prc_t *);
static OMX_ERRORTYPE
obtain_next_url (cc_prc_t *, int);
static OMX_ERRORTYPE
store_chromecast_metadata (cc_prc_t *);
static void
deliver_stored_metadata (cc_prc_t *);
static OMX_ERRORTYPE
store_error_msg (cc_prc_t * ap_prc, const char *);

static void
post_chromecast_event (cc_prc_t * ap_prc, tiz_event_pluggable_hdlr_f apf_hdlr,
                       unsigned int a_status, int a_volume,
                       const char * ap_err_msg)
{
  tiz_event_pluggable_t * p_event = NULL;
  cc_status_event_data_t * p_status = NULL;
  assert (ap_prc);
  assert (apf_hdlr);

  p_event = tiz_mem_calloc (1, sizeof (tiz_event_pluggable_t));
  p_status = tiz_mem_calloc (1, sizeof (cc_status_event_data_t));
  if (p_event && p_status)
    {
      p_event->p_servant = ap_prc;
      p_event->pf_hdlr = apf_hdlr;
      p_status->status = a_status;
      p_status->volume = a_volume;
      p_status->p_err_msg
        = (ap_err_msg != NULL ? strndup (ap_err_msg, strlen (ap_err_msg))
                              : NULL);
      p_event->p_data = p_status;
      tiz_comp_event_pluggable (handleOf (ap_prc), p_event);
    }
  else
    {
      tiz_mem_free (p_event);
      tiz_mem_free (p_status);
    }
}

static void
cast_status_handler (OMX_PTR ap_prc, tiz_event_pluggable_t * ap_event)
{
  cc_prc_t * p_prc = ap_prc;
  cc_status_event_data_t * p_event_data = NULL;
  tiz_cast_client_cast_status_t status = ETizCcCastStatusUnknown;
  int volume = 0;
  bool need_song_metadata_update = false;
  bool need_chromecast_metadata_update = false;

  assert (p_prc);
  assert (ap_event);
  assert (ap_event->p_data);

  p_event_data = ap_event->p_data;
  status = p_event_data->status;
  volume = p_event_data->volume;

  TIZ_DEBUG (handleOf (p_prc), "current status [%s] current volume [%d]",
             tiz_cast_client_cast_status_str (p_prc->cc_cast_status_),
             p_prc->volume_);
  TIZ_DEBUG (handleOf (p_prc), "new status [%s] new volume [%d]",
             tiz_cast_client_cast_status_str (status), volume);

  /* Clear previous metatada items */
  (void) tiz_krn_clear_metadata (tiz_get_krn (handleOf (p_prc)));

  if (ETizCcCastStatusNowCasting == p_prc->cc_cast_status_
      && ETizCcCastStatusReadyToCast == status)
    {
      need_song_metadata_update = true;
      /* End of stream, skip to next track */
      (void) obtain_next_url (p_prc, 1);
      /* Load the new URL */
      (void) load_next_url (p_prc);
    }

  if ((p_prc->cc_cast_status_ != status) || (volume != p_prc->volume_))
    {
      need_chromecast_metadata_update = true;
    }

  p_prc->cc_cast_status_ = status;
  p_prc->volume_ = p_event_data->volume;
  if (need_chromecast_metadata_update)
    {
      store_chromecast_metadata (p_prc);
    }

  if (need_song_metadata_update || need_chromecast_metadata_update)
    {
      deliver_stored_metadata (p_prc);
    }

  tiz_mem_free (ap_event->p_data);
  tiz_mem_free (ap_event);
}

static void
cc_cast_status_cback (void * ap_user_data,
                      tiz_cast_client_cast_status_t a_status, int a_volume)
{
  cc_prc_t * p_prc = ap_user_data;
  assert (p_prc);
  post_chromecast_event (p_prc, cast_status_handler, a_status, a_volume, NULL);
}

static void
media_status_handler (OMX_PTR ap_prc, tiz_event_pluggable_t * ap_event)
{
  cc_prc_t * p_prc = ap_prc;
  cc_status_event_data_t * p_event_data = NULL;
  tiz_cast_client_media_status_t status = ETizCcMediaStatusUnknown;

  assert (p_prc);
  assert (ap_event);
  assert (ap_event->p_data);

  p_event_data = ap_event->p_data;
  status = (tiz_cast_client_media_status_t) p_event_data->status;
  TIZ_DEBUG (handleOf (p_prc), "status [%s]",
             tiz_cast_client_media_status_str (status));

  if (p_prc->cc_media_status_ != status)
    {
      p_prc->cc_media_status_ = status;
      /* Clear previous metatada items */
      (void) tiz_krn_clear_metadata (tiz_get_krn (handleOf (p_prc)));
      store_chromecast_metadata (p_prc);
      deliver_stored_metadata (p_prc);
    }

  tiz_mem_free (ap_event->p_data);
  tiz_mem_free (ap_event);
}

static void
cc_media_status_cback (void * ap_user_data,
                       tiz_cast_client_media_status_t a_status, int a_volume)
{
  cc_prc_t * p_prc = ap_user_data;
  assert (p_prc);
  post_chromecast_event (p_prc, media_status_handler, a_status, a_volume, NULL);
}

static void
error_status_handler (OMX_PTR ap_prc, tiz_event_pluggable_t * ap_event)
{
  cc_prc_t * p_prc = ap_prc;
  cc_status_event_data_t * p_event_data = NULL;
  tiz_cast_client_error_status_t status = ETizCcErrorStatusNoError;

  assert (p_prc);
  assert (ap_event);
  assert (ap_event->p_data);

  p_event_data = ap_event->p_data;
  status = (tiz_cast_client_error_status_t) p_event_data->status;
  TIZ_DEBUG (handleOf (p_prc), "status [%s]",
             tiz_cast_client_error_status_str (status));

  if (ETizCcErrorStatusNoError != status)
    {
      store_chromecast_metadata (p_prc);
      if (OMX_ErrorNone
          == store_error_msg (p_prc, tiz_cast_client_error_status_str (status)))
        {
          TIZ_DEBUG (handleOf (p_prc), "with_data [%s]", p_prc->p_cc_err_msg_);
          (void) tiz_srv_issue_err_event_with_data (
            (OMX_PTR) ap_prc, OMX_ErrorInsufficientResources,
            p_prc->p_cc_err_msg_);
        }
      else
        {
          TIZ_DEBUG (handleOf (p_prc), "without _data");
          (void) tiz_srv_issue_err_event ((OMX_PTR) ap_prc,
                                          OMX_ErrorInsufficientResources);
        }
    }
  tiz_mem_free (p_event_data->p_err_msg);
  tiz_mem_free (ap_event->p_data);
  tiz_mem_free (ap_event);
}

static void
cc_error_status_cback (void * ap_user_data,
                       tiz_cast_client_error_status_t a_status,
                       const char * ap_err_msg)
{
  cc_prc_t * p_prc = ap_user_data;
  assert (p_prc);
  post_chromecast_event (p_prc, error_status_handler, a_status, 0, ap_err_msg);
}

static OMX_ERRORTYPE
release_buffer (cc_prc_t * ap_prc)
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
retrieve_cc_session (cc_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamChromecastSession, &(ap_prc->cc_session_));
}

static OMX_ERRORTYPE
obtain_next_url (cc_prc_t * ap_prc, int a_skip_value)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const long pathname_max = PATH_MAX + NAME_MAX;

  assert (ap_prc);

  if (!ap_prc->p_uri_param_)
    {
      ap_prc->p_uri_param_ = tiz_mem_calloc (
        1, sizeof (OMX_PARAM_CONTENTURITYPE) + pathname_max + 1);
    }

  tiz_check_null_ret_oom (ap_prc->p_uri_param_);

  ap_prc->p_uri_param_->nSize
    = sizeof (OMX_PARAM_CONTENTURITYPE) + pathname_max + 1;
  ap_prc->p_uri_param_->nVersion.nVersion = OMX_VERSION;

  {
    const char * p_next_url = a_skip_value > 0 ? cc_prc_get_next_url (ap_prc)
                                               : cc_prc_get_prev_url (ap_prc);
    tiz_check_null_ret_oom (p_next_url);

    {
      const OMX_U32 url_len = strnlen (p_next_url, pathname_max);
      TIZ_TRACE (handleOf (ap_prc), "URL [%s]", p_next_url);

      /* Verify we are getting an http scheme */
      if (!p_next_url || !url_len
          || (memcmp (p_next_url, "http://", 7) != 0
              && memcmp (p_next_url, "https://", 8) != 0))
        {
          rc = OMX_ErrorContentURIError;
        }
      else
        {
          strncpy ((char *) ap_prc->p_uri_param_->contentURI, p_next_url,
                   url_len);
          ap_prc->p_uri_param_->contentURI[url_len] = '\0';

          /* Song metadata is now available, update the IL client */
          rc = cc_prc_store_stream_metadata (ap_prc);
          ap_prc->uri_changed_ = true;
        }
    }
  }

  return rc;
}

static OMX_ERRORTYPE
load_next_url (cc_prc_t * p_prc)
{
  assert (p_prc);
  assert (p_prc->p_cc_);
  assert (p_prc->p_uri_param_);
  assert (p_prc->p_uri_param_->contentURI);
  if (p_prc->p_cc_ && p_prc->p_uri_param_
      && (const char *) p_prc->p_uri_param_->contentURI)
    {
      on_cc_error_ret_omx_oom (tiz_cast_client_load_url (
        p_prc->p_cc_, (const char *) p_prc->p_uri_param_->contentURI,
        CONTENT_TYPE,
        (p_prc->p_cc_display_title_ ? p_prc->p_cc_display_title_
                                    : DISPLAY_TITLE),
        cc_prc_get_current_stream_album_art_url (p_prc)));
      p_prc->uri_changed_ = false;
    }
  return OMX_ErrorNone;
}

static void
deliver_stored_metadata (cc_prc_t * ap_prc)
{
  assert (ap_prc);

  TIZ_DEBUG (handleOf (ap_prc), "deliver_stored_metadata");

  /* Signal that a new set of metatadata items is available */
  (void) tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventIndexSettingChanged,
                              OMX_ALL, /* no particular port associated */
                              OMX_IndexConfigMetadataItem, /* index of the
                                                             struct that has
                                                             been modififed */
                              NULL);
}

static OMX_ERRORTYPE
store_chromecast_metadata (cc_prc_t * ap_prc)
{
  assert (ap_prc);

  TIZ_DEBUG (handleOf (ap_prc), "store_chromecast_metadata");

  /* Artist and song title */
  {
    char cast_name_or_ip[OMX_MAX_STRINGNAME_SIZE];
    char status_line[OMX_MAX_STRINGNAME_SIZE];
    snprintf (cast_name_or_ip, OMX_MAX_STRINGNAME_SIZE, "  %s",
              (char *) ap_prc->cc_session_.cNameOrIpAddr);
    snprintf (status_line, OMX_MAX_STRINGNAME_SIZE,
              "(%s) (Media:%s) (Vol:%ld)",
              tiz_cast_client_cast_status_str (ap_prc->cc_cast_status_),
              tiz_cast_client_media_status_str (ap_prc->cc_media_status_),
              ap_prc->volume_);
    tiz_check_omx (
      cc_prc_store_stream_metadata_item (ap_prc, cast_name_or_ip, status_line));
  }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
store_error_msg (cc_prc_t * ap_prc, const char * ap_err_msg)
{
  assert (ap_prc);
  if (ap_err_msg)
    {
      tiz_mem_free (ap_prc->p_cc_err_msg_);
      ap_prc->p_cc_err_msg_ = strndup (ap_err_msg, OMX_MAX_STRINGNAME_SIZE);
      tiz_check_null_ret_oom (ap_prc->p_cc_err_msg_);
    }
  return OMX_ErrorNone;
}

static inline void
delete_uri (cc_prc_t * ap_prc)
{
  assert (ap_prc);
  tiz_mem_free (ap_prc->p_uri_param_);
  ap_prc->p_uri_param_ = NULL;
}

static void
set_volume (cc_prc_t * ap_prc, const long a_volume)
{
  assert (ap_prc);
  assert (ap_prc->p_cc_);
  TIZ_DEBUG (handleOf (ap_prc), "ap_prc->volume_ [%d]", ap_prc->volume_);
  if (a_volume > ap_prc->volume_)
    {
      tiz_cast_client_volume_up (ap_prc->p_cc_);
    }
  else if (a_volume < ap_prc->volume_)
    {
      tiz_cast_client_volume_down (ap_prc->p_cc_);
    }
  ap_prc->volume_ = a_volume;
}

static void
toggle_mute (cc_prc_t * ap_prc, const bool a_mute)
{
  assert (ap_prc);

  if (a_mute)
    {
      tiz_cast_client_mute (ap_prc->p_cc_);
    }
  else
    {
      tiz_cast_client_unmute (ap_prc->p_cc_);
    }
}

/*
 * cc_prc
 */

static void *
prc_ctor (void * ap_obj, va_list * app)
{
  cc_prc_t * p_prc = super_ctor (typeOf (ap_obj, "cc_prc"), ap_obj, app);
  assert (p_prc);
  TIZ_INIT_OMX_STRUCT (p_prc->cc_session_);
  TIZ_INIT_OMX_STRUCT (p_prc->pl_skip_);
  p_prc->p_uri_param_ = NULL;
  p_prc->p_inhdr_ = NULL;
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
prc_dtor (void * ap_obj)
{
  (void) tiz_srv_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "cc_prc"), ap_obj);
}

static OMX_ERRORTYPE
prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  cc_prc_t * p_prc = ap_obj;
  assert (p_prc);

  tiz_check_omx (retrieve_cc_session (p_prc));
  TIZ_TRACE (handleOf (p_prc), "cNameOrIpAddr  : [%s]",
             p_prc->cc_session_.cNameOrIpAddr);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
prc_deallocate_resources (void * ap_prc)
{
  cc_prc_t * p_prc = ap_prc;
  TIZ_TRACE (handleOf (p_prc), "p_prc->p_cc_  : [%p]",
             p_prc->p_cc_);
  assert (p_prc);
  delete_uri (p_prc);
  tiz_cast_client_destroy (p_prc->p_cc_);
  p_prc->p_cc_ = NULL;
  tiz_mem_free (p_prc->p_cc_display_title_);
  p_prc->p_cc_display_title_ = NULL;
  tiz_mem_free (p_prc->p_cc_err_msg_);
  p_prc->p_cc_err_msg_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid)
{
  cc_prc_t * p_prc = ap_prc;
  assert (ap_prc);

  /* Clear previous metatada items */
  (void) tiz_krn_clear_metadata (tiz_get_krn (handleOf (p_prc)));
  tiz_check_omx (obtain_next_url (p_prc, 1));
  deliver_stored_metadata (p_prc);

  /* Lazy instantiation of the cast client object */
  if (!p_prc->p_cc_)
    {
      OMX_UUIDTYPE cc_uuid;
      tiz_cast_client_callbacks_t cast_cbacks
        = {cc_cast_status_cback, cc_media_status_cback, cc_error_status_cback};
      bzero (&(cc_uuid), 128);
      tiz_uuid_generate (&(cc_uuid));
      TIZ_DEBUG (handleOf (p_prc), "cNameOrIpAddr [%s]",
                 (const char *) p_prc->cc_session_.cNameOrIpAddr);
      on_cc_error_ret_omx_oom (tiz_cast_client_init (
        &(p_prc->p_cc_), (const char *) p_prc->cc_session_.cNameOrIpAddr,
        &(cc_uuid), &cast_cbacks, p_prc));
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid)
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
prc_stop_and_return (void * ap_prc)
{
  cc_prc_t * p_prc = ap_prc;
  assert (p_prc);
  assert (p_prc->p_cc_);
  (void) tiz_cast_client_stop (p_prc->p_cc_);
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE
prc_buffers_ready (const void * ap_prc)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
prc_pause (const void * ap_prc)
{
  cc_prc_t * p_prc = (cc_prc_t *) ap_prc;
  assert (p_prc);
  assert (p_prc->p_cc_);
  on_cc_error_ret_omx_oom (tiz_cast_client_pause (p_prc->p_cc_));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
prc_resume (const void * ap_prc)
{
  cc_prc_t * p_prc = (cc_prc_t *) ap_prc;
  assert (p_prc);
  assert (p_prc->p_cc_);
  on_cc_error_ret_omx_oom (tiz_cast_client_play (p_prc->p_cc_));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
prc_config_change (void * ap_prc, OMX_U32 a_pid, OMX_INDEXTYPE a_config_idx)
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
      /* Clear previous metatada items */
      (void) tiz_krn_clear_metadata (tiz_get_krn (handleOf (p_prc)));
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
prc_port_flush (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  cc_prc_t * p_prc = (cc_prc_t *) ap_obj;
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE
prc_port_disable (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  cc_prc_t * p_prc = (cc_prc_t *) ap_obj;
  assert (p_prc);
  /* Release any buffers held  */
  return release_buffer ((cc_prc_t *) ap_obj);
}

static const char *
prc_get_next_url (const void * p_obj)
{
  /* To be implemented in derived classes */
  return NULL;
}

const char *
cc_prc_get_next_url (const void * ap_obj)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->get_next_url);
  return class->get_next_url (ap_obj);
}

static const char *
prc_get_prev_url (const void * p_obj)
{
  /* To be implemented in derived classes */
  return NULL;
}

const char *
cc_prc_get_prev_url (const void * ap_obj)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->get_prev_url);
  return class->get_prev_url (ap_obj);
}

static const char *
prc_get_current_stream_album_art_url (const void * p_obj)
{
  /* To be implemented in derived classes */
  return NULL;
}

const char *
cc_prc_get_current_stream_album_art_url (const void * ap_obj)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->get_current_stream_album_art_url);
  return class->get_current_stream_album_art_url (ap_obj);
}

static OMX_ERRORTYPE
prc_store_stream_metadata (const void * p_obj)
{
  /* To be implemented in derived classes */
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
cc_prc_store_stream_metadata (const void * ap_obj)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->store_stream_metadata);
  return class->store_stream_metadata (ap_obj);
}

static OMX_ERRORTYPE
prc_store_stream_metadata_item (const void * ap_obj, const char * ap_header_name,
                              const char * ap_header_info)
{
  cc_prc_t * p_prc = (cc_prc_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_CONFIG_METADATAITEMTYPE * p_meta = NULL;
  size_t metadata_len = 0;
  size_t info_len = 0;

  assert (p_prc);
  if (ap_header_name && ap_header_info)
    {
      info_len = strnlen (ap_header_info, OMX_MAX_STRINGNAME_SIZE - 1) + 1;
      metadata_len = sizeof (OMX_CONFIG_METADATAITEMTYPE) + info_len;

      if (NULL
          == (p_meta = (OMX_CONFIG_METADATAITEMTYPE *) tiz_mem_calloc (
                1, metadata_len)))
        {
          rc = OMX_ErrorInsufficientResources;
        }
      else
        {
          const size_t name_len
            = strnlen (ap_header_name, OMX_MAX_STRINGNAME_SIZE - 1) + 1;
          strncpy ((char *) p_meta->nKey, ap_header_name, name_len - 1);
          p_meta->nKey[name_len - 1] = '\0';
          p_meta->nKeySizeUsed = name_len;

          strncpy ((char *) p_meta->nValue, ap_header_info, info_len - 1);
          p_meta->nValue[info_len - 1] = '\0';
          p_meta->nValueMaxSize = info_len;
          p_meta->nValueSizeUsed = info_len;

          p_meta->nSize = metadata_len;
          p_meta->nVersion.nVersion = OMX_VERSION;
          p_meta->eScopeMode = OMX_MetadataScopeAllLevels;
          p_meta->nScopeSpecifier = 0;
          p_meta->nMetadataItemIndex = 0;
          p_meta->eSearchMode = OMX_MetadataSearchValueSizeByIndex;
          p_meta->eKeyCharset = OMX_MetadataCharsetASCII;
          p_meta->eValueCharset = OMX_MetadataCharsetASCII;

          rc = tiz_krn_store_metadata (tiz_get_krn (handleOf (p_prc)), p_meta);
        }
    }
  return rc;
}

OMX_ERRORTYPE
cc_prc_store_stream_metadata_item (const void * ap_obj,
                                 const char * ap_header_name,
                                 const char * ap_header_info)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->store_stream_metadata_item);
  return class->store_stream_metadata_item (ap_obj, ap_header_name,
                                          ap_header_info);
}

static OMX_ERRORTYPE
prc_store_display_title (const void * ap_obj, const char * ap_artist,
                         const char * ap_title)
{
  cc_prc_t * p_prc = (cc_prc_t *) ap_obj;
  assert (p_prc);
  if (ap_artist && ap_title)
    {
      tiz_mem_free (p_prc->p_cc_display_title_);
      p_prc->p_cc_display_title_ = tiz_mem_calloc (1, OMX_MAX_STRINGNAME_SIZE);
      tiz_check_null_ret_oom (p_prc->p_cc_display_title_);
      snprintf (p_prc->p_cc_display_title_, OMX_MAX_STRINGNAME_SIZE - 1,
                "%s - %s", ap_artist, ap_title);
    }
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
cc_prc_store_display_title (const void * ap_obj, const char * ap_artist,
                            const char * ap_title)
{
  const cc_prc_class_t * class = classOf (ap_obj);
  assert (class->store_display_title);
  return class->store_display_title (ap_obj, ap_artist, ap_title);
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
      if (selector == (voidf) cc_prc_get_next_url)
        {
          *(voidf *) &p_obj->get_next_url = method;
        }
      else if (selector == (voidf) cc_prc_get_prev_url)
        {
          *(voidf *) &p_obj->get_prev_url = method;
        }
      else if (selector == (voidf) cc_prc_get_current_stream_album_art_url)
        {
          *(voidf *) &p_obj->get_current_stream_album_art_url = method;
        }
      else if (selector == (voidf) cc_prc_store_stream_metadata)
        {
          *(voidf *) &p_obj->store_stream_metadata = method;
        }
      else if (selector == (voidf) cc_prc_store_stream_metadata_item)
        {
          *(voidf *) &p_obj->store_stream_metadata_item = method;
        }
      else if (selector == (voidf) cc_prc_store_display_title)
        {
          *(voidf *) &p_obj->store_display_title = method;
        }
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
     ctor, prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_pause, prc_pause,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_resume, prc_resume,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_config_change, prc_config_change,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_next_url, prc_get_next_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_prev_url, prc_get_prev_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_current_stream_album_art_url, prc_get_current_stream_album_art_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_store_stream_metadata, prc_store_stream_metadata,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_store_stream_metadata_item, prc_store_stream_metadata_item,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_store_display_title, prc_store_display_title,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return filterprc;
}
