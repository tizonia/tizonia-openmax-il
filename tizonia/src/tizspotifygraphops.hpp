/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizspotifygraphops.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Spotify client graph actions / operations
 *
 *
 */

#ifndef TIZSPOTIFYGRAPHOPS_HPP
#define TIZSPOTIFYGRAPHOPS_HPP

#include "tizgraphops.hpp"

namespace tiz
{
  namespace graph
  {
    class graph;

    class spotifyops : public ops
    {
    public:
      spotifyops (graph *p_graph, const omx_comp_name_lst_t &comp_lst,
                  const omx_comp_role_lst_t &role_lst);

    public:
      void do_enable_auto_detection (const int handle_id,
                                     const int port_id);
      void do_disable_ports ();
      void do_configure_source ();
      void do_load ();
      void do_configure ();
      void do_omx_loaded2idle ();
      void do_omx_idle2exe ();
      void do_reconfigure_tunnel ();
      void do_skip ();

      // These are spotifyops-specific methods
      void do_retrieve_metadata ();
      void do_sink_omx_idle2exe ();
      void do_sink_omx_exe2idle ();

    private:
      OMX_ERRORTYPE transition_sink (const OMX_STATETYPE to_state);
      OMX_ERRORTYPE transition_tunnel (
          const int tunnel_id, const OMX_COMMANDTYPE to_disabled_or_enabled);
      OMX_ERRORTYPE set_spotify_user_and_pass (const OMX_HANDLETYPE handle,
                                               const std::string &user,
                                               const std::string &pass);
      OMX_ERRORTYPE set_spotify_playlist (const OMX_HANDLETYPE handle,
                                          const std::string &playlist);

    private:
      // re-implemented from the base class
      bool probe_stream_hook ();
      void dump_stream_metadata ();
      OMX_ERRORTYPE dump_metadata_item (const OMX_U32 index);
      OMX_ERRORTYPE get_encoding_type_from_spotify_source ();
      OMX_ERRORTYPE apply_pcm_codec_info_from_spotify_source ();
      OMX_ERRORTYPE get_channels_and_rate_from_spotify_source (
          OMX_U32 &channels, OMX_U32 &sampling_rate,
          std::string &encoding_str) const;
      OMX_ERRORTYPE set_channels_and_rate_on_renderer (
          const OMX_U32 channels, const OMX_U32 sampling_rate,
          const std::string encoding_str);

    private:
      OMX_AUDIO_CODINGTYPE encoding_;
      OMX_AUDIO_PARAM_PCMMODETYPE renderer_pcmtype_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZSPOTIFYGRAPHOPS_HPP
