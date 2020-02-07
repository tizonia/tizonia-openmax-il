/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   tizhttpservgraph.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL HTTP Streaming Server - graph operations
 *
 *
 */

#ifndef TIZHTTPSERVOPS_HPP
#define TIZHTTPSERVOPS_HPP

#include "tizgraphops.hpp"

namespace tiz
{
  namespace graph
  {
    class graph;

    class httpservops : public ops
    {
    public:
      httpservops (graph *p_graph, const omx_comp_name_lst_t &comp_lst,
                   const omx_comp_role_lst_t &role_lst);

    public:
      void do_probe ();
      void do_exe2pause ();
      void do_pause2exe ();
      void do_volume (const int step);
      void do_mute ();

      void do_configure_server ();
      void do_configure_station ();
      void do_configure_stream ();
      bool is_initial_configuration () const;
      void do_flag_initial_config_done ();

    private:
      OMX_ERRORTYPE configure_server ();
      OMX_ERRORTYPE configure_station ();
      OMX_ERRORTYPE configure_stream_metadata ();
      OMX_ERRORTYPE switch_tunnel (const int tunnel_id,
          const OMX_COMMANDTYPE to_disabled_or_enabled);

    private:
      void get_mp3_codec_info (OMX_AUDIO_PARAM_MP3TYPE &mp3type);
      // re-implemented from the base class
      bool probe_stream_hook ();

    private:
      bool is_initial_configuration_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZHTTPSERVOPS_HPP
