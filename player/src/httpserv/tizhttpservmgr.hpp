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
 * @file   tizhttpservmgr.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  A manager for the HTTP server graph
 *
 *
 */

#ifndef TIZHTTPSERVMGR_HPP
#define TIZHTTPSERVMGR_HPP

#include "tizgraphtypes.hpp"
#include "tizgraphmgr.hpp"

namespace tiz
{
  namespace graphmgr
  {
    class httpservmgrops;
    class graphmgr_capabilities;

    /**
     *  @class httpservmgr
     *  @brief The http server graph manager class.
     *
     */
    class httpservmgr : public mgr
    {
      friend class httpservmgrops;

    public:
      httpservmgr (tizgraphconfig_ptr_t config);
      virtual ~httpservmgr ();

    private:
      ops *do_init (const tizplaylist_ptr_t &playlist,
                    const termination_callback_t &termination_cback,
                    graphmgr_capabilities &graphmgr_caps);

    private:
      tizgraphconfig_ptr_t config_;
    };

    typedef boost::shared_ptr< httpservmgr > httpservmgr_ptr_t;

    class httpservmgrops : public ops
    {
    public:
      httpservmgrops (mgr *p_mgr, const tizplaylist_ptr_t &playlist,
                      const termination_callback_t &termination_cback);

      void do_load ();
      void do_execute ();

    private:
      tizgraph_ptr_t get_graph (const std::string &uri);
    };
  }  // namespace graphmgr
}  // namespace tiz

#endif  // TIZHTTPSERVMGR_HPP
