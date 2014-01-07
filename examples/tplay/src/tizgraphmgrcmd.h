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
 * @file   tizgraphmgrcmd.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph manager command class.
 *
 *
 */

#ifndef TIZGRAPHMGRCMD_H
#define TIZGRAPHMGRCMD_H

class tizgraphmgrcmd
{

 public:

  enum cmd_type
  {
    ETIZGraphMgrCmdStart,
    ETIZGraphMgrCmdNext,
    ETIZGraphMgrCmdPrev,
    ETIZGraphMgrCmdFwd,
    ETIZGraphMgrCmdRwd,
    ETIZGraphMgrCmdVolumeUp,
    ETIZGraphMgrCmdVolumeDown,
    ETIZGraphMgrCmdPause,
    ETIZGraphMgrCmdStop,
    ETIZGraphMgrCmdGraphEop,
    ETIZGraphMgrCmdGraphError,
    ETIZGraphMgrCmdMax
  };

 public:

  tizgraphmgrcmd (const cmd_type type);

  cmd_type get_type () const {return type_;}

  const char * c_str () const;

 private:

  const cmd_type type_;

};

#endif // TIZGRAPHMGRCMD_H
