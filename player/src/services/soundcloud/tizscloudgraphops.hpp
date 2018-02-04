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
 * @file   tizscloudgraphops.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  SoundCloud client graph actions / operations
 *
 *
 */

#ifndef TIZSCLOUDGRAPHOPS_HPP
#define TIZSCLOUDGRAPHOPS_HPP

#include "tizgraphops.hpp"

namespace tiz
{
  namespace graph
  {
    class graph;

    class scloudops : public ops
    {
    public:
      scloudops (graph *p_graph, const omx_comp_name_lst_t &comp_lst,
                 const omx_comp_role_lst_t &role_lst);

    public:
      void do_enable_auto_detection (const int handle_id, const int port_id);
      void do_disable_comp_ports (const int comp_id, const int port_id);
      void do_configure_comp (const int comp_id);
      void do_load ();
      void do_configure ();
      void do_loaded2idle ();
      void do_idle2exe ();
      void do_reconfigure_tunnel (const int tunnel_id);
      void do_skip ();
      void do_retrieve_metadata ();

      bool is_fatal_error (const OMX_ERRORTYPE error) const;
      void do_record_fatal_error (const OMX_HANDLETYPE handle,
                                  const OMX_ERRORTYPE error,
                                  const OMX_U32 port,
                                  const OMX_PTR p_eventdata = NULL);

    private:
      OMX_ERRORTYPE switch_tunnel (
          const int tunnel_id, const OMX_COMMANDTYPE to_disabled_or_enabled);

    private:
      // re-implemented from the base class
      bool probe_stream_hook ();
      OMX_ERRORTYPE get_encoding_type_from_scloud_source ();
      OMX_ERRORTYPE apply_pcm_codec_info_from_decoder ();
      OMX_ERRORTYPE get_channels_and_rate_from_decoder (
          OMX_U32 &channels, OMX_U32 &sampling_rate,
          std::string &encoding_str) const;
      OMX_ERRORTYPE set_channels_and_rate_on_renderer (
          const OMX_U32 channels, const OMX_U32 sampling_rate,
          const std::string encoding_str);

      void do_reconfigure_first_tunnel ();
      void do_reconfigure_second_tunnel ();

    private:
      OMX_AUDIO_CODINGTYPE encoding_;
      OMX_AUDIO_PARAM_PCMMODETYPE renderer_pcmtype_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZSCLOUDGRAPHOPS_HPP
