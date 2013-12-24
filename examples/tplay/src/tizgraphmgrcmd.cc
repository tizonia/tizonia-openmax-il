/* -*-Mode: c++; -*- */
/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraphmgrcmd.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph manager command class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizgraphmgrcmd.h"

#include <boost/assign/list_of.hpp>

#include <string>
#include <vector>

#include <assert.h>

namespace // Unnamed namespace
{

  typedef struct graph_mgr_cmd_str graph_mgr_cmd_str_t;
  struct graph_mgr_cmd_str
  {
    graph_mgr_cmd_str (tizgraphmgrcmd::cmd_type a_cmd, std::string a_str)
      : cmd (a_cmd), str (a_str)
    {}
    tizgraphmgrcmd::cmd_type cmd;
    const std::string str;
  };

  const std::vector<graph_mgr_cmd_str_t> graph_mgr_cmd_to_str_tbl
  = boost::assign::list_of
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdStart, "ETIZGraphMgrCmdStart"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdNext, "ETIZGraphMgrCmdNext"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdPrev, "ETIZGraphMgrCmdPrev"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdFwd, "ETIZGraphMgrCmdFwd"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdRwd, "ETIZGraphMgrCmdRwd"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdVolume, "ETIZGraphMgrCmdVolume"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdPause, "ETIZGraphMgrCmdPause"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdStop, "ETIZGraphMgrCmdStop"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdGraphEop, "ETIZGraphMgrCmdGraphEop"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdGraphError, "ETIZGraphMgrCmdGraphError"))
    (graph_mgr_cmd_str_t (tizgraphmgrcmd::ETIZGraphMgrCmdMax, "ETIZGraphMgrCmdMax"));

}

tizgraphmgrcmd::tizgraphmgrcmd (const cmd_type type)
  : type_ (type)
{
  assert (type_ < ETIZGraphMgrCmdMax);
}

/*@observer@*/ const char *
tizgraphmgrcmd::c_str () const
{
  const size_t count = graph_mgr_cmd_to_str_tbl.size ();
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (graph_mgr_cmd_to_str_tbl[i].cmd == type_)
        {
          return graph_mgr_cmd_to_str_tbl[i].str.c_str ();
        }
    }
  return "Unknown Graph Manager command";
}

