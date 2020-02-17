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
 * @file   tizvorbisgraph.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL vorbis decoder graph
 *
 *
 */

#ifndef TIZVORBISGRAPH_HPP
#define TIZVORBISGRAPH_HPP

#include "tizdecgraph.hpp"
#include "tizgraphops.hpp"

namespace tiz
{
  namespace graph
  {
    class vorbisdecoder : public decoder
    {

    public:
      vorbisdecoder ();

    protected:
      ops *do_init ();
    };

    class vorbisdecops : public ops
    {
    public:
      vorbisdecops (graph *p_graph, const omx_comp_name_lst_t &comp_lst,
                    const omx_comp_role_lst_t &role_lst);

    public:
      void do_disable_comp_ports (const int comp_id, const int port_id);
      void do_probe ();
      bool is_port_settings_evt_required () const;
      bool is_disabled_evt_required () const;
      void do_configure ();

    protected:
      OMX_ERRORTYPE set_vorbis_settings ();
      void get_pcm_codec_info (OMX_AUDIO_PARAM_PCMMODETYPE &pcmtype);

    protected:
      bool need_port_settings_changed_evt_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZVORBISGRAPH_HPP
