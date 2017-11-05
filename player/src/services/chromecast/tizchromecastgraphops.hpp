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
 * @file   tizchromecastgraphops.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Chromecast rendering graph actions / operations
 *
 *
 */

#ifndef TIZCHROMECASTGRAPHOPS_HPP
#define TIZCHROMECASTGRAPHOPS_HPP

#include <boost/function.hpp>

#include "tizgraphops.hpp"

namespace tiz
{
  namespace graph
  {
    class graph;

    class chromecastops : public ops
    {
    public:
      chromecastops (graph *p_graph, const omx_comp_name_lst_t &comp_lst,
                 const omx_comp_role_lst_t &role_lst);

    public:
      void do_load ();
      void do_configure ();
      void do_loaded2idle ();
      void do_idle2exe ();
      void do_skip ();
      void do_retrieve_metadata ();

      bool is_fatal_error (const OMX_ERRORTYPE error) const;
      void do_record_fatal_error (const OMX_HANDLETYPE handle,
                                  const OMX_ERRORTYPE error,
                                  const OMX_U32 port);

    private:
      void do_configure_gmusic ();
      void do_configure_scloud ();
      void do_configure_dirble ();
      void do_configure_youtube ();

      OMX_ERRORTYPE get_encoding_type_from_chromecast_source ();

    private:
      typedef boost::function< void() > config_func_t;

    private:
      // re-implemented from the base class
      bool probe_stream_hook ();

    private:
      OMX_AUDIO_CODINGTYPE encoding_;
      config_func_t config_func_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZCHROMECASTGRAPHOPS_HPP
