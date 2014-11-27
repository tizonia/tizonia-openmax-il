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
 * @file   spfysrcprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Spotify client component
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <strings.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "spfysrc.h"
#include "spfysrcprc.h"
#include "spfysrcprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.spotify_source.prc"
#endif

/* This macro assumes the existence of an "ap_prc" local variable */
#define goto_end_on_sp_error(expr)                         \
  do                                                       \
    {                                                      \
      sp_error sperr = SP_ERROR_OK;                        \
      if ((sperr = (expr)) != SP_ERROR_OK)                 \
        {                                                  \
          TIZ_ERROR (handleOf (ap_prc),                    \
                     "[OMX_ErrorInsufficientResources] : " \
                     "%s",                                 \
                     sp_error_message (sperr));            \
          goto end;                                        \
        }                                                  \
    }                                                      \
  while (0)

/* Forward declarations */
static OMX_ERRORTYPE spfysrc_prc_deallocate_resources (void *ap_obj);

/* The application key, specific to each project. */
extern const uint8_t g_appkey[];
/* The size of the application key. */
extern const size_t g_appkey_size;

static const char *username = "jarubio2001";
static const char *password = "thepassword";
static const char *listname = "Amaranthe";

typedef struct spfy_logged_in_data spfy_logged_in_data_t;
struct spfy_logged_in_data
{
  sp_session *p_sess;
  sp_error error;
};

typedef struct spfy_music_delivery_data spfy_music_delivery_data_t;
struct spfy_music_delivery_data
{
  sp_session *p_sess;
  sp_audioformat *p_format;
  void *p_frames;
  int num_frames;
};

typedef struct spfy_playlist_added_removed_data
    spfy_playlist_added_removed_data_t;
struct spfy_playlist_added_removed_data
{
  sp_playlistcontainer *p_pc;
  sp_playlist *p_pl;
  int position;
};

typedef struct spfy_tracks_operation_data spfy_tracks_operation_data_t;
struct spfy_tracks_operation_data
{
  sp_playlist *p_pl;
  const int *p_tracks;
  int num_tracks;
  int position;
};

static void post_internal_event (spfysrc_prc_t *ap_prc,
                                 tiz_event_pluggable_hdlr_f apf_hdlr,
                                 void *ap_data)
{
  tiz_event_pluggable_t *p_event = NULL;
  assert (NULL != ap_prc);
  assert (NULL != apf_hdlr);
  assert (NULL != ap_data);

  p_event = tiz_mem_calloc (1, sizeof(tiz_event_pluggable_t));
  if (p_event)
    {
      p_event->p_servant = ap_prc;
      p_event->pf_hdlr = apf_hdlr;
      p_event->p_data = ap_data;
      tiz_comp_event_pluggable (handleOf (ap_prc), p_event);
    }
}

/**
 * Called on various events to start playback if it hasn't been started already.
 *
 * The function simply starts playing the first track of the playlist.
 */
static void start_playback (spfysrc_prc_t *ap_prc)
{
#define verify_or_return                            \
  (outcome, msg) do                                 \
  {                                                 \
    if (!outcome)                                   \
      {                                             \
        TIZ_DEBUG (handleOf (ap_prc), "[%s]", msg); \
        return;                                     \
      }                                             \
  }                                                 \
  while (0)

  sp_track *p_track = NULL;
  bool outcome = false;

  assert (NULL != ap_prc);

  verify_or_return ((ap_prc->p_sp_playlist_ != NULL), "No playlist. Waiting.");
  verify_or_return ((sp_playlist_num_tracks (ap_prc->p_sp_playlist_) > 0),
                    "No tracks in playlist. Waiting.");
  verify_or_return (
      (sp_playlist_num_tracks (ap_prc->p_sp_playlist_) < ap_prc->track_index_),
      "No more tracks in playlist. Waiting.");

  p_track = sp_playlist_track (ap_prc->p_sp_playlist_, ap_prc->track_index_);

  if (ap_prc->p_sp_track_ && p_track != ap_prc->p_sp_track_)
    {
      /* Someone changed the current track */
      /*       audio_fifo_flush(&g_audiofifo); */
      sp_session_player_unload (ap_prc->p_sp_session_);
      ap_prc->p_sp_track_ = NULL;
    }

  verify_or_return ((p_track != NULL), "Track is NULL. Waiting.");
  verify_or_return ((sp_track_error (p_track) == SP_ERROR_OK),
                    "Track error. Waiting.");

  if (ap_prc->p_sp_track_ == p_track)
    {
      return;
    }

  ap_prc->p_sp_track_ = p_track;

  TIZ_NOTICE (handleOf (ap_prc), "Now playing [%s]", sp_track_name (p_track));

  sp_session_player_load (ap_prc->p_sp_session_, p_track);
  sp_session_player_play (ap_prc->p_sp_session_, 1);
}

static void logged_in_cback_handler (OMX_PTR ap_prc,
                                     tiz_event_pluggable_t *ap_event)
{
  spfysrcprc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (ap_event->p_data)
    {
      spfy_logged_in_data_t *p_lg_data = ap_event->p_data;
      TIZ_NOTICE (handleOf (p_prc), "[%s]",
                  sp_error_message (p_lg_data->error));

      if (SP_ERROR_OK == p_lg_data->error)
        {
          int i = 0;
          int nplaylists = 0;
          sp_error sp_rc = SP_ERROR_OK;
          sp_playlistcontainer *pc
              = sp_session_playlistcontainer (p_lg_data->sess);
          assert (NULL != pc);

          sp_rc = sp_playlistcontainer_add_callbacks (
              pc, &(p_prc->sp_plct_cbacks_), p_prc);
          assert (SP_ERROR_OK == sp_rc);

          nplaylists = sp_playlistcontainer_num_playlists (pc);
          TIZ_NOTICE (handleOf (p_prc), "Looking at %d playlists", nplaylists);

          for (i = 0; i < nplaylists; ++i)
            {
              sp_playlist *pl = sp_playlistcontainer_playlist (pc, i);
              assert (NULL != pl);

              sp_prc = sp_playlist_add_callbacks (pl, &(p_prc->sp_pl_cbacks_),
                                                  p_prc);
              assert (SP_ERROR_OK == sp_rc);

              if (!strcasecmp (sp_playlist_name (pl), p_prc->p_playlist_name_))
                {
                  p_prc->p_sp_playlist_ = pl;
                  start_playback ();
                }
            }

          if (!p_prc->p_sp_playlist_)
            {
              TIZ_NOTICE (handleOf (p_prc),
                          "No such playlist. Waiting for one to pop up...");
            }
        }
    }
  tiz_mem_free (ap_event->p_data);
  tiz_mem_free (ap_event);
}

/**
 * This callback is called when an attempt to login has succeeded or failed.
 *
 * @sa sp_session_callbacks#logged_in
 */
static void logged_in (sp_session *sess, sp_error error)
{
  spfy_logged_in_data_t *p_data
      = tiz_mem_calloc (1, sizeof(spfy_logged_in_data_t));
  if (p_data)
    {
      p_data->p_sess = sess;
      p_data->error = error;
      post_internal_event (sp_session_userdata (sess), logged_in_cback_handler,
                           p_data);
    }
}

static void notify_main_thread_cback_handler (OMX_PTR ap_prc,
                                              tiz_event_pluggable_t *ap_event)
{
  spfysrcprc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (ap_event->p_data)
    {
      int next_timeout = 0;
      do
        {
          sp_session_process_events (ap_event->p_data, &next_timeout);
        }
      while (next_timeout == 0);
    }
  tiz_mem_free (ap_event);
}

/**
 * This callback is called from an internal libspotify thread to ask us to
 * reiterate the main loop.
 *
 * We notify the main thread using a condition variable and a protected
 *variable.
 *
 * @sa sp_session_callbacks#notify_main_thread
 */
static void notify_main_thread (sp_session *sess)
{
  post_internal_event (sp_session_userdata (sess),
                       notify_main_thread_cback_handler, sess);
}

static void music_delivery_cback_handler (OMX_PTR ap_prc,
                                          tiz_event_pluggable_t *ap_event)
{
  spfysrcprc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (ap_event->p_data)
    {
      size_t s;
      /* Buffer one second of audio */
      /*       if (af->qlen > format->sample_rate) */
      /*         { */
      /*           return 0; */
      /*         } */

      s = num_frames * sizeof(int16_t) * format->channels;

      afd = malloc (sizeof(*afd) + s);
      memcpy (afd->samples, frames, s);

      afd->nsamples = num_frames;

      afd->rate = format->sample_rate;
      afd->channels = format->channels;

      TAILQ_INSERT_TAIL (&af->q, afd, link);
      af->qlen += num_frames;

      /*       pthread_cond_signal (&af->cond); */
      /*       pthread_mutex_unlock (&af->mutex); */
    }
  tiz_mem_free (ap_event->p_data);
  tiz_mem_free (ap_event);
}

/**
 * This callback is used from libspotify whenever there is PCM data available.
 *
 * @sa sp_session_callbacks#music_delivery
 */
static int music_delivery (sp_session *sess, const sp_audioformat *format,
                           const void *frames, int num_frames)
{
  if (num_frames > 0)
    {
      spfy_music_delivery_data_t *p_data
          = tiz_mem_calloc (1, sizeof(spfy_music_delivery_data_t));
      if (p_data)
        {
          p_data->p_sess = sess;
          p_data->p_format = format;
          p_data->p_frames = frames;
          p_data->num_frames = num_frames;
          post_internal_event (sp_session_userdata (sess),
                               music_delivery_cback_handler, p_data);
        }
    }
  return num_frames;
}

static void end_of_track_cback_handler (OMX_PTR ap_prc,
                                        tiz_event_pluggable_t *ap_event)
{
  spfysrcprc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (ap_event->p_data)
    {
      /* TODO */
    }
  tiz_mem_free (ap_event);
}

/**
 * This callback is used from libspotify when the current track has ended
 *
 * @sa sp_session_callbacks#end_of_track
 */
static void end_of_track (sp_session *sess)
{
  post_internal_event (sp_session_userdata (sess), end_of_track_cback_handler,
                       sess);
}

static void play_token_lost_cback_handler (OMX_PTR ap_prc,
                                           tiz_event_pluggable_t *ap_event)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_event);

  if (p_prc->p_sp_track_ != NULL)
    {
      sp_session_player_unload (p_prc->p_sp_session_);
      p_prc->p_sp_track_ = NULL;
    }
  tiz_mem_free (ap_event);
}

/**
 * Notification that some other connection has started playing on this account.
 * Playback has been stopped.
 *
 * @sa sp_session_callbacks#play_token_lost
 */
static void play_token_lost (sp_session *sess)
{
  post_internal_event (sp_session_userdata (sess),
                       play_token_lost_cback_handler, sess);
}

/* --------------------  PLAYLIST CONTAINER CALLBACKS  --------------------- */

static void playlist_added_cback_handler (OMX_PTR ap_prc,
                                          tiz_event_pluggable_t *ap_event)
{
  spfysrcprc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (ap_event->p_data)
    {
      spfy_playlist_added_removed_data_t *p_data = ap_event->p_data;
      sp_playlist_add_callbacks (p_data->p_pl, &(p_prc->sp_pl_cbacks_), NULL);

      if (!strcasecmp (sp_playlist_name (p_data->p_pl),
                       p_prc->p_playlist_name_))
        {
          p_prc->p_sp_playlist_ = p_data->p_pl;
          start_playback ();
        }
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify, telling us a playlist was added to the playlist
 *container.
 *
 * We add our playlist callbacks to the newly added playlist.
 *
 * @param  pc            The playlist container handle
 * @param  pl            The playlist handle
 * @param  position      Index of the added playlist
 * @param  userdata      The opaque pointer
 */
static void playlist_added (sp_playlistcontainer *pc, sp_playlist *pl,
                            int position, void *userdata)
{
  spfy_playlist_added_removed_data_t *p_data
      = tiz_mem_calloc (1, sizeof(spfy_playlist_added_removed_data_t));
  if (p_data)
    {
      p_data->p_pc = pc;
      p_data->p_pl = pl;
      p_data->position = position;
      post_internal_event (userdata, playlist_added_cback_handler, p_data);
    }
}

static void playlist_removed_cback_handler (OMX_PTR ap_prc,
                                            tiz_event_pluggable_t *ap_event)
{
  spfysrcprc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (ap_event->p_data)
    {
      spfy_playlist_added_removed_data_t *p_data = ap_event->p_data;
      sp_playlist_remove_callbacks (p_data > p_pl, &(p_prc->sp_pl_cbacks_),
                                    NULL);
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify, telling us a playlist was removed from the playlist
 *container.
 *
 * This is the place to remove our playlist callbacks.
 *
 * @param  pc            The playlist container handle
 * @param  pl            The playlist handle
 * @param  position      Index of the removed playlist
 * @param  userdata      The opaque pointer
 */
static void playlist_removed (sp_playlistcontainer *pc, sp_playlist *pl,
                              int position, void *userdata)
{
  spfy_playlist_added_removed_data_t *p_data
      = tiz_mem_calloc (1, sizeof(spfy_playlist_added_removed_data_t));
  if (p_data)
    {
      p_data->p_pc = pc;
      p_data->p_pl = pl;
      p_data->position = position;
      post_internal_event (userdata, playlist_removed_cback_handler, p_data);
    }
}

/**
 * Callback from libspotify, telling us the rootlist is fully synchronized
 * We just print an informational message
 *
 * @param  pc            The playlist container handle
 * @param  userdata      The opaque pointer
 */
static void container_loaded (sp_playlistcontainer *pc, void *userdata)
{
  TIZ_DEBUG (handleOf (userdata), "Rootlist synchronized (%d playlists)",
             sp_playlistcontainer_num_playlists (pc));
}

static void tracks_added_cback_handler (OMX_PTR ap_prc,
                                        tiz_event_pluggable_t *ap_event)
{
  spfysrcprc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (ap_event->p_data)
    {
      spfy_tracks_operation_data_t *p_data = ap_event->p_data;
      if (p_data->p_pl == &(p_prc->p_sp_playlist_))
        {
          TIZ_DEBUG (handleOf (p_prc), "%d tracks were added",
                     p_data->num_tracks);
          start_playback ();
        }
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify, saying that a track has been added to a playlist.
 *
 * @param  pl          The playlist handle
 * @param  tracks      An array of track handles
 * @param  num_tracks  The number of tracks in the \c tracks array
 * @param  position    Where the tracks were inserted
 * @param  userdata    The opaque pointer
 */
static void tracks_added (sp_playlist *pl, sp_track *const *tracks,
                          int num_tracks, int position, void *userdata)
{
  spfy_tracks_operation_data_t *p_data
      = tiz_mem_calloc (1, sizeof(spfy_tracks_operation_data_t));
  if (p_data)
    {
      p_data->p_pl = pl;
      p_data->p_tracks = tracks;
      p_data->num_tracks = num_tracks;
      p_data->position = position;
      post_internal_event (userdata, tracks_added_cback_handler, p_data);
    }
}

static void tracks_removed_cback_handler (OMX_PTR ap_prc,
                                          tiz_event_pluggable_t *ap_event)
{
  spfysrcprc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (ap_event->p_data)
    {
      spfy_tracks_operation_data_t *p_data = ap_event->p_data;
      int i = 0;
      int k = 0;
      if (p_data->p_pl == &(p_prc->p_sp_playlist_))
        {
          for (i = 0; i < p_data->num_tracks; ++i)
            {
              if (p_data->tracks[i] < p_prc->track_index_)
                {
                  ++k;
                }
            }
          p_prc->track_index_ -= k;
          TIZ_DEBUG (handleOf (p_prc), "%d tracks have been removed",
                     p_data->num_tracks);
          start_playback ();
        }
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify, saying that a track has been added to a playlist.
 *
 * @param  pl          The playlist handle
 * @param  tracks      An array of track indices
 * @param  num_tracks  The number of tracks in the \c tracks array
 * @param  userdata    The opaque pointer
 */
static void tracks_removed (sp_playlist *pl, const int *tracks, int num_tracks,
                            void *userdata)
{
  spfy_tracks_operation_data_t *p_data
      = tiz_mem_calloc (1, sizeof(spfy_tracks_operation_data_t));
  if (p_data)
    {
      p_data->p_pl = pl;
      p_data->p_tracks = tracks;
      p_data->num_tracks = num_tracks;
      post_internal_event (userdata, tracks_removed_cback_handler, p_data);
    }
}

static void tracks_moved_cback_handler (OMX_PTR ap_prc,
                                        tiz_event_pluggable_t *ap_event)
{
  spfysrcprc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (ap_event->p_data)
    {
      spfy_tracks_operation_data_t *p_data = ap_event->p_data;
      if (p_data->p_pl == p_prc->p_sp_playlist_)
        {
          TIZ_DEBUG (handleOf (p_prc), "%d tracks were moved around",
                     num_tracks);
          start_playback ();
        }
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify, telling when tracks have been moved around in a
 *playlist.
 *
 * @param  pl            The playlist handle
 * @param  tracks        An array of track indices
 * @param  num_tracks    The number of tracks in the \c tracks array
 * @param  new_position  To where the tracks were moved
 * @param  userdata      The opaque pointer
 */
static void tracks_moved (sp_playlist *pl, const int *tracks, int num_tracks,
                          int new_position, void *userdata)
{
  spfy_tracks_operation_data_t *p_data
      = tiz_mem_calloc (1, sizeof(spfy_tracks_operation_data_t));
  if (p_data)
    {
      p_data->p_pl = pl;
      p_data->p_tracks = tracks;
      p_data->num_tracks = num_tracks;
      p_data->position = new_position;
      post_internal_event (userdata, tracks_moved_cback_handler, p_data);
    }
}

static void playlist_renamed_cback_handler (OMX_PTR ap_prc,
                                            tiz_event_pluggable_t *ap_event)
{
  spfysrcprc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (ap_event->p_data)
    {
      sp_playlist *pl = ap_event->p_data;
      const char *name = sp_playlist_name (pl);

      if (!strcasecmp (name, p_prc->p_playlist_name_))
        {
          p_prc->p_sp_playlist_ = pl;
          p_prc->track_index_ = 0;
          start_playback ();
        }
      else if (p_prc->p_sp_playlist_ == pl)
        {
          TIZ_DEBUG (handleOf (p_prc), "current playlist renamed to \"%s\".",
                     name);
          p_prc->p_sp_playlist_ = NULL;
          p_prc->p_sp_track_ = NULL;
          sp_session_player_unload (p_prc->p_sp_session_);
        }
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify. Something renamed the playlist.
 *
 * @param  pl            The playlist handle
 * @param  userdata      The opaque pointer
 */
static void playlist_renamed (sp_playlist *pl, void *userdata)
{
  post_internal_event (userdata, playlist_renamed_cback_handler, pl);
}

/*
 * spfysrcprc
 */

static void *spfysrc_prc_ctor (void *ap_obj, va_list *app)
{
  spfysrc_prc_t *p_prc
      = super_ctor (typeOf (ap_obj, "spfysrcprc"), ap_obj, app);

  p_prc->eos_ = false;
  p_prc->p_ev_timer_ = NULL;
  p_prc->track_index_ = 0;
  p_prc->p_playlist_name_ = listname;
  p_prc->p_user_name_ = username;
  p_prc->p_user_pass_ = password;
  p_prc->p_sp_session_ = NULL;

  tiz_mem_set ((OMX_PTR) & p_prc->sp_config_, 0, sizeof(p_prc->sp_config_));
  p_prc->sp_config_.api_version = SPOTIFY_API_VERSION;
  p_prc->sp_config_.cache_location = "tmp";
  p_prc->sp_config_.settings_location = "tmp";
  p_prc->sp_config_.application_key = g_appkey;
  p_prc->sp_config_.application_key_size = g_appkey_size;
  p_prc->sp_config_.user_agent = "tizonia-source-component";
  p_prc->sp_config_.callbacks = &(p_prc->sp_cbacks_);
  p_prc->sp_config_.userdata = p_prc;
  p_prc->sp_config_.compress_playlists = false;
  p_prc->sp_config_.dont_save_metadata_for_playlists = true;
  p_prc->sp_config_.initially_unload_playlists = false;

  tiz_mem_set ((OMX_PTR) & p_prc->sp_cbacks_, 0, sizeof(p_prc->sp_cbacks_));
  p_prc->sp_cbacks_.logged_in = &logged_in;
  p_prc->sp_cbacks_.notify_main_thread = &notify_main_thread;
  p_prc->sp_cbacks_.music_delivery = &music_delivery;
  p_prc->sp_cbacks_.metadata_updated = NULL;
  p_prc->sp_cbacks_.play_token_lost = &play_token_lost;
  p_prc->sp_cbacks_.log_message = NULL;
  p_prc->sp_cbacks_.end_of_track = &end_of_track;

  tiz_mem_set ((OMX_PTR) & p_prc->sp_plct_cbacks_, 0,
               sizeof(p_prc->sp_plct_cbacks_));
  p_prc->sp_plct_cbacks_.playlist_added = &playlist_added;
  p_prc->sp_plct_cbacks_.playlist_removed = &playlist_removed;
  p_prc->sp_plct_cbacks_.container_loaded = &container_loaded;

  tiz_mem_set ((OMX_PTR) & p_prc->sp_pl_cbacks_, 0,
               sizeof(p_prc->sp_pl_cbacks_));
  p_prc->sp_pl_cbacks_.tracks_added = &tracks_added;
  p_prc->sp_pl_cbacks_.tracks_removed = &tracks_removed;
  p_prc->sp_pl_cbacks_.tracks_moved = &tracks_moved;
  p_prc->sp_pl_cbacks_.playlist_renamed = &playlist_renamed;

  p_prc->p_sp_playlist_ = NULL;
  p_prc->p_sp_track_ = NULL;

  return p_prc;
}

static void *spfysrc_prc_dtor (void *ap_obj)
{
  (void)spfysrc_prc_deallocate_resources (ap_prc);
  return super_dtor (typeOf (ap_obj, "spfysrcprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE spfysrc_prc_allocate_resources (void *ap_obj,
                                                     OMX_U32 a_pid)
{
  spfysrc_prc_t *p_prc = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  sp_error sperr = = SP_ERROR_OK;

  assert (NULL != p_prc);
  assert (NULL == p_prc->p_ev_timer_);

  tiz_check_omx_err (tiz_srv_timer_watcher_init (p_prc, &(p_prc->p_ev_timer_)));

  /* Create session */
  goto_end_on_sp_error (
      sp_session_create (&(p_prc->sp_config_), p_prc->p_sp_session_));

  /* Initiate the login in the background */
  goto_end_on_sp_error (sp_session_login (sp, username, password,
                                          true, /* If true, the username /
                                                   password will be remembered
                                                   by libspotify */
                                          NULL));

  /* All OK */
  rc = OMX_ErrorNone;

end:

  return rc;
}

static OMX_ERRORTYPE spfysrc_prc_deallocate_resources (void *ap_obj)
{
  spfysrc_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  if (ap_prc->p_ev_timer_)
    {
      (void)tiz_srv_timer_watcher_stop (ap_prc, ap_prc->p_ev_timer_);
      p_prc->p_ev_timer_ = NULL;
    }
  (void)sp_session_release (p_prc->p_sp_session_);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE spfysrc_prc_prepare_to_transfer (void *ap_obj,
                                                      OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE spfysrc_prc_transfer_and_process (void *ap_obj,
                                                       OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE spfysrc_prc_stop_and_return (void *ap_obj)
{
  spfysrc_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  if (ap_prc->p_ev_timer_)
    {
      (void)tiz_srv_timer_watcher_stop (ap_prc, ap_prc->p_ev_timer_);
    }
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE spfysrc_prc_buffers_ready (const void *ap_obj)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  do
    {
      sp_session_process_events (sp, &next_timeout);
    }
  while (next_timeout == 0);

  if (next_timeout)
    {
      rc = tiz_srv_timer_watcher_start (ap_prc, ap_prc->p_ev_timer_,
                                        next_timeout / 1000,
                                        next_timeout / 1000);
    }
  return rc;
}

static OMX_ERRORTYPE spfysrc_prc_timer_ready (void *ap_prc,
                                              tiz_event_timer_t *ap_ev_timer,
                                              void *ap_arg, const uint32_t a_id)
{
  TIZ_TRACE (handleOf (ap_prc), "Received timer event");
  do
    {
      sp_session_process_events (sp, &next_timeout);
    }
  while (next_timeout == 0);

  if (next_timeout)
    {
      rc = tiz_srv_timer_watcher_start (ap_prc, ap_prc->p_ev_timer_,
                                        next_timeout / 1000,
                                        next_timeout / 1000);
    }

  return OMX_ErrorNone;
}

/*
 * spfysrc_prc_class
 */

static void *spfysrc_prc_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "spfysrcprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *spfysrc_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *spfysrcprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizprc), "spfysrcprc_class", classOf (tizprc),
       sizeof(spfysrc_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, spfysrc_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return spfysrcprc_class;
}

void *spfysrc_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *spfysrcprc_class = tiz_get_type (ap_hdl, "spfysrcprc_class");
  TIZ_LOG_CLASS (spfysrcprc_class);
  void *spfysrcprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (spfysrcprc_class, "spfysrcprc", tizprc, sizeof(spfysrc_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, spfysrc_prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, spfysrc_prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, spfysrc_prc_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, spfysrc_prc_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, spfysrc_prc_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, spfysrc_prc_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, spfysrc_prc_stop_and_return,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_timer_ready, spfysrc_prc_timer_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, spfysrc_prc_buffers_ready,
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return spfysrcprc;
}
