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
 * @file   tizgraphcmd.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph manager command class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizgraphcmd.h"

#include <string>
#include <vector>

#include <assert.h>

namespace // Unnamed namespace
{

  typedef struct graph_cmd_str graph_cmd_str_t;
  struct graph_cmd_str
  {
    graph_cmd_str (tizgraphcmd::cmd_type a_cmd, std::string a_str)
      : cmd (a_cmd), str (a_str)
    {}
    tizgraphcmd::cmd_type cmd;
    const std::string str;
  };

  const std::vector<graph_cmd_str_t> graph_cmd_to_str_tbl
  = boost::assign::list_of
    (graph_cmd_str_t (tizgraphcmd::ETIZGraphCmdLoad, "ETIZGraphCmdLoad"))
    (graph_cmd_str_t (tizgraphcmd::ETIZGraphCmdConfig, "ETIZGraphCmdConfig"))
    (graph_cmd_str_t (tizgraphcmd::ETIZGraphCmdExecute, "ETIZGraphCmdExecute"))
    (graph_cmd_str_t (tizgraphcmd::ETIZGraphCmdPause, "ETIZGraphCmdPause"))
    (graph_cmd_str_t (tizgraphcmd::ETIZGraphCmdSeek, "ETIZGraphCmdSeek"))
    (graph_cmd_str_t (tizgraphcmd::ETIZGraphCmdSkip, "ETIZGraphCmdSkip"))
    (graph_cmd_str_t (tizgraphcmd::ETIZGraphCmdVolume, "ETIZGraphCmdVolume"))
    (graph_cmd_str_t (tizgraphcmd::ETIZGraphCmdVolume, "ETIZGraphCmdMute"))
    (graph_cmd_str_t (tizgraphcmd::ETIZGraphCmdUnload, "ETIZGraphCmdUnload"))
    (graph_cmd_str_t (tizgraphcmd::ETIZGraphCmdEos, "ETIZGraphCmdEos"))
    (graph_cmd_str_t (tizgraphcmd::ETIZGraphCmdError, "ETIZGraphCmdError"))
    (graph_cmd_str_t (tizgraphcmd::ETIZGraphCmdMax, "ETIZGraphCmdMax"));

}

tizgraphcmd::tizgraphcmd (const cmd_type type,
                          const tizgraphconfig_ptr_t config,
                          const OMX_HANDLETYPE handle, /* = NULL */
                          const int jump, /* = 0 */
                          const OMX_ERRORTYPE error /* = OMX_ErrorNone */)
  : type_ (type),
    config_ (config),
    handle_ (handle),
    jump_ (jump),
    error_ (error)
{
  assert (type_ < ETIZGraphCmdMax);
}

/*@observer@*/ const char *
tizgraphcmd::c_str () const
{
  const size_t count = graph_cmd_to_str_tbl.size ();
  size_t i = 0;
  
  for (i = 0; i < count; ++i)
    {
      if (graph_cmd_to_str_tbl[i].cmd == type_)
        {
          return graph_cmd_to_str_tbl[i].str.c_str ();
        }
    }
  return "Unknown Graph command";
}
