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
 * @file   tizgraphmgrops.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph manager operations
 *
 *
 */

#ifndef TIZGRAPHMGROPS_H
#define TIZGRAPHMGROPS_H

#include "tizgraphtypes.h"
#include "tizplaylist.h"

#include <OMX_Core.h>

#include <string>
#include <boost/function.hpp>

namespace tiz
{
  namespace graphmgr
  {
    // forward decl
    class mgr;

    /**
     *  @class ops
     *  @brief The graph manager operations class.
     *
     */
    class ops
    {

    public:

      typedef boost::function<
      void (OMX_ERRORTYPE, std::string)> error_callback_t;

    public:

      ops (mgr * p_mgr, const uri_lst_t &file_list,
           const error_callback_t &error_cback);
      virtual ~ops ();

      void deinit ();

    public:

      void do_load ();
      void do_execute ();
      void do_unload ();

      void do_next ();
      void do_prev ();
      void do_fwd ();
      void do_rwd ();
      void do_vol_up ();
      void do_vol_down ();
      void do_mute ();
      void do_pause ();
      void do_report_fatal_error (const OMX_ERRORTYPE error,
                                  const std::string &msg);

      OMX_ERRORTYPE get_internal_error () const;
      std::string get_internal_error_msg () const;

    protected:

      tizgraph_ptr_t get_graph (const std::string & uri);

    protected:

      mgr *p_mgr_;
      tizplaylist_t playlist_;
      tizgraphconfig_ptr_t graph_config_;
      tizgraph_ptr_map_t graph_registry_;
      tizgraph_ptr_t p_managed_graph_;
      error_callback_t error_cback_;
      OMX_ERRORTYPE error_code_;
      std::string error_msg_;

    };
  } // namespace graphmgr
} // namespace tiz

#endif // TIZGRAPHMGROPS_H
