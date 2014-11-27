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
 * @file   spfysrcprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Spotify client processor declarations
 *
 *
 */

#ifndef SPFYSRCPRC_DECLS_H
#define SPFYSRCPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <libspotify/api.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>

typedef struct spfysrc_prc spfysrc_prc_t;
struct spfysrc_prc
{
  /* Object */
  const tiz_prc_t _;
  bool eos_;
  tiz_event_timer_t *p_ev_timer_;
  int track_index_;
  const char *p_playlist_name_;
  const char *p_user_name_;
  const char *p_user_pass_;
  sp_session *p_sp_session_;
  sp_session_config sp_config_;        /* The session configuration */
  sp_session_callbacks sp_cbacks_;     /* The session callbacks */
  sp_playlist_callbacks sp_pl_cbacks_; /* The callbacks we are interested in
                                          for individual playlists */
  sp_playlistcontainer_callbacks sp_plct_cbacks_; /* The playlist container
                                                     callbacks */
  sp_playlist *p_sp_playlist_; /* Handle to the playlist currently being
                                 played */
  sp_track *p_sp_track_;         /* Handle to the curren track */
};

typedef struct spfysrc_prc_class spfysrc_prc_class_t;
struct spfysrc_prc_class
{
  /* Class */
  const tiz_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* SPFYSRCPRC_DECLS_H */
