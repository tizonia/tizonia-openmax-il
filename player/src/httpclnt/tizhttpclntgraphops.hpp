/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   tizhttpclntgraphops.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  HTTP client graph actions / operations
 *
 *
 */

#ifndef TIZHTTPCLNTGRAPHOPS_HPP
#define TIZHTTPCLNTGRAPHOPS_HPP

#include "tizgraphops.hpp"

namespace tiz
{
  namespace graph
  {
    class graph;

    class httpclntops : public ops
    {
    public:
      httpclntops (graph *p_graph, const omx_comp_name_lst_t &comp_lst,
                   const omx_comp_role_lst_t &role_lst);

    public:
      void do_enable_auto_detection (const int handle_id, const int port_id);
      void do_disable_comp_ports (const int comp_id, const int port_id);
      void do_configure_comp (const int comp_id);
      void do_load ();
      void do_configure ();
      void do_exe2pause ();
      void do_pause2exe ();
      void do_loaded2idle ();
      void do_idle2exe ();
      void do_reconfigure_tunnel (const int tunnel_id);

    private:
      OMX_ERRORTYPE switch_tunnel (
          const int tunnel_id, const OMX_COMMANDTYPE to_disabled_or_enabled);

    private:
      OMX_ERRORTYPE add_decoder_to_component_list (
          omx_comp_name_lst_t &comp_list, omx_comp_role_lst_t &role_list);
      // re-implemented from the base class
      bool probe_stream_hook ();
      void dump_stream_metadata ();
      OMX_ERRORTYPE get_encoding_type_from_http_source ();
      OMX_ERRORTYPE apply_pcm_codec_info_from_http_source ();
      OMX_ERRORTYPE get_channels_and_rate_from_http_source (
          OMX_U32 &channels, OMX_U32 &sampling_rate,
          std::string &encoding_str) const;
      OMX_ERRORTYPE set_channels_and_rate_on_decoder (
          const OMX_U32 channels, const OMX_U32 sampling_rate);
      OMX_ERRORTYPE set_channels_and_rate_on_renderer (
          const OMX_U32 channels, const OMX_U32 sampling_rate,
          const std::string encoding_str);

    private:
      OMX_AUDIO_CODINGTYPE encoding_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZHTTPCLNTGRAPHOPS_HPP
