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
 * @file   tizhttpclntmgr.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  A manager for the HTTP client graph
 *
 *
 */

#ifndef TIZHTTPCLNTMGR_HPP
#define TIZHTTPCLNTMGR_HPP

#include "tizgraphtypes.hpp"
#include "tizgraphmgr.hpp"

namespace tiz
{
  namespace graphmgr
  {
    class httpclntmgrops;

    /**
     *  @class httpclntmgr
     *  @brief The http client graph manager class.
     *
     */
    class httpclntmgr : public mgr
    {
      friend class httpclntmgrops;

    public:
      httpclntmgr (tizgraphconfig_ptr_t config);
      virtual ~httpclntmgr ();

    private:
      ops *do_init (const tizplaylist_ptr_t &playlist,
                    const error_callback_t &error_cback);

    private:
      tizgraphconfig_ptr_t config_;
    };

    typedef boost::shared_ptr< httpclntmgr > httpclntmgr_ptr_t;

    class httpclntmgrops : public ops
    {
    public:
      httpclntmgrops (mgr *p_mgr, const tizplaylist_ptr_t &playlist,
                      const error_callback_t &error_cback);

      void do_load ();
      void do_execute ();

    private:
      tizgraph_ptr_t get_graph (const std::string &uri);
    };
  }  // namespace graphmgr
}  // namespace tiz

#endif  // TIZHTTPCLNTMGR_HPP
