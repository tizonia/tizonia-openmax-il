/* -*-Mode: c++; -*- */
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
 * @file   tizhttpservgraph.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL HTTP Streaming Server - graph operations
 *
 *
 */

#ifndef TIZHTTPSERVOPS_H
#define TIZHTTPSERVOPS_H

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
      void do_omx_exe2pause ();
      void do_omx_pause2exe ();
      void do_volume (const int step);
      void do_mute ();

      void do_configure_server ();
      void do_configure_station ();
      void do_configure_stream ();
      void do_source_omx_loaded2idle ();
      void do_source_omx_idle2exe ();
      void do_source_omx_exe2idle ();
      void do_source_omx_idle2loaded ();
      void do_disable_tunnel ();
      void do_enable_tunnel ();
      bool is_initial_configuration () const;
      void do_flag_initial_config_done ();

    private:
      OMX_ERRORTYPE configure_server ();
      OMX_ERRORTYPE configure_station ();
      OMX_ERRORTYPE configure_stream_metadata ();
      OMX_ERRORTYPE transition_source (const OMX_STATETYPE to_state);
      OMX_ERRORTYPE transition_tunnel (
          const OMX_COMMANDTYPE to_disabled_or_enabled);

    private:
      bool is_initial_configuration_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZHTTPSERVOPS_H
