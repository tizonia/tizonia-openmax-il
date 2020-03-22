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
 * @file   tiziheartmgr.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Manager for the Iheart client graph
 *
 *
 */

#ifndef TIZIHEARTMGR_HPP
#define TIZIHEARTMGR_HPP

#include "tizgraphtypes.hpp"
#include "tizgraphmgrcaps.hpp"
#include "tizgraphmgr.hpp"

namespace tiz
{
  namespace graphmgr
  {
    class iheartmgrops;
    class graphmgr_capabilities;

    /**
     *  @class iheartmgr
     *  @brief The Iheart client graph manager class.
     *
     */
    class iheartmgr : public mgr
    {
      friend class iheartmgrops;

    public:
      iheartmgr (tizgraphconfig_ptr_t config);
      virtual ~iheartmgr ();

    private:
      ops *do_init (const tizplaylist_ptr_t &playlist,
                    const termination_callback_t &termination_cback,
                    graphmgr_capabilities &graphmgr_caps);

    private:
      tizgraphconfig_ptr_t config_;
    };

    typedef boost::shared_ptr< iheartmgr > iheartmgr_ptr_t;

    class iheartmgrops : public ops
    {
    public:
      iheartmgrops (mgr *p_mgr, const tizplaylist_ptr_t &playlist,
                      const termination_callback_t &termination_cback);

      void do_load ();
      void do_execute ();

    private:
      bool is_fatal_error (const OMX_ERRORTYPE error,
                           const std::string &msg);
      tizgraph_ptr_t get_graph (const std::string &uri);
    };
  }  // namespace graphmgr
}  // namespace tiz

#endif  // TIZIHEARTMGR_HPP
