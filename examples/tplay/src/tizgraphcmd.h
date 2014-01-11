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
 * @file   tizgraphcmd.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph command class.
 *
 *
 */

#ifndef TIZGRAPHCMD_H
#define TIZGRAPHCMD_H

#include "tizgraphtypes.h"

#include <boost/assign/list_of.hpp>

// TODO: Consider refactoring this class to something nicer, perhaps
// using sub-command classes.

class tizgraphcmd
{

public:

  enum cmd_type
  {
    ETIZGraphCmdLoad,
    ETIZGraphCmdConfig,
    ETIZGraphCmdExecute,
    ETIZGraphCmdPause,
    ETIZGraphCmdSeek,
    ETIZGraphCmdSkip,
    ETIZGraphCmdVolume,
    ETIZGraphCmdMute,
    ETIZGraphCmdUnload,
    ETIZGraphCmdEos,
    ETIZGraphCmdError,
    ETIZGraphCmdMax
  };

  tizgraphcmd (const cmd_type type,
               const tizgraphconfig_ptr_t config,
               const OMX_HANDLETYPE handle = NULL,
               const int jump = 0,
               const OMX_ERRORTYPE error = OMX_ErrorNone);

  cmd_type get_type () const {return type_;}
  tizgraphconfig_ptr_t get_config () const {return config_;}
  OMX_HANDLETYPE get_handle () const {return handle_;}
  int get_jump () const {return jump_;};
  OMX_ERRORTYPE get_error () const {return error_;};

  const char * c_str () const;

private:

  const cmd_type type_;
  tizgraphconfig_ptr_t config_;
  OMX_HANDLETYPE handle_;
  int jump_;
  OMX_ERRORTYPE error_;
};

#endif // TIZGRAPHCMD_H
