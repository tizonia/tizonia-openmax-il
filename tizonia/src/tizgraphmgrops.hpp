/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraphmgrops.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph manager operations
 *
 *
 */

#ifndef TIZGRAPHMGROPS_HPP
#define TIZGRAPHMGROPS_HPP

#include <string>
#include <boost/function.hpp>

#include <OMX_Core.h>

#include "tizgraphtypes.hpp"
#include "tizplaybackstatus.hpp"
#include "tizplaylist.hpp"

#define GMGR_OPS_RECORD_ERROR(err, str)                                     \
  do                                                                        \
  {                                                                         \
    error_msg_.assign (str);                                                \
    error_code_ = err;                                                      \
    TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : %s", tiz_err_to_str (error_code_), \
             error_msg_.c_str ());                                          \
  } while (0)

#define GMGR_OPS_BAIL_IF_ERROR(ptr, exp, str) \
  do                                          \
  {                                           \
    if (ptr)                                  \
    {                                         \
      OMX_ERRORTYPE rc_ = OMX_ErrorNone;      \
      if (OMX_ErrorNone != (rc_ = (exp)))     \
      {                                       \
        GMGR_OPS_RECORD_ERROR (rc_, str);     \
      }                                       \
    }                                         \
  } while (0)

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
      typedef boost::function< void(OMX_ERRORTYPE, std::string) >
          termination_callback_t;

    public:
      ops (mgr *p_mgr, const tizplaylist_ptr_t &playlist,
           const termination_callback_t &termination_cback);
      virtual ~ops ();

      void deinit ();

    public:
      virtual void do_load ();
      virtual void do_execute ();
      virtual void do_stop ();
      virtual void do_unload ();
      virtual void do_next ();
      virtual void do_prev ();
      virtual void do_fwd ();
      virtual void do_rwd ();
      virtual void do_vol_up ();
      virtual void do_vol_down ();
      virtual void do_vol (const double vol);
      virtual void do_mute ();
      virtual void do_pause ();
      virtual void do_report_fatal_error (const OMX_ERRORTYPE error,
                                          const std::string &msg);
      virtual void do_end_of_play ();
      virtual void do_update_control_ifcs (const control::playback_status_t status);
      virtual void do_update_metadata (const track_metadata_map_t &metadata);
      virtual void do_update_volume (const int volume);
      virtual bool is_fatal_error (const OMX_ERRORTYPE error,
                                   const std::string &msg);

      OMX_ERRORTYPE internal_error () const;
      std::string internal_error_msg () const;

      tizplaylist_ptr_t find_next_sub_list () const;

    protected:
      virtual tizgraph_ptr_t get_graph (const std::string &uri);

    protected:
      mgr *p_mgr_;              // Not owned
      tizplaylist_ptr_t playlist_;
      tizplaylist_ptr_t next_playlist_;
      tizgraphconfig_ptr_t graph_config_;
      tizgraph_ptr_map_t graph_registry_;
      tizgraph_ptr_t p_managed_graph_;
      termination_callback_t termination_cback_;
      OMX_ERRORTYPE error_code_;
      std::string error_msg_;
    };
  }  // namespace graphmgr
}  // namespace tiz

#endif  // TIZGRAPHMGROPS_HPP
