/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraphmgrcaps.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph Manager capabilities
 *
 *
 */

#ifndef TIZGRAPHMGRCAPS_HPP
#define TIZGRAPHMGRCAPS_HPP

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace tiz
{
  namespace graphmgr
  {
    class graphmgr_capabilities
    {
    public:
      bool can_quit_;
      bool can_raise_;
      bool has_track_list_;
      std::string identity_;
      std::vector< std::string > uri_schemes_;
      std::vector< std::string > mime_types_;
      double minimum_rate_;
      double maximum_rate_;
      bool can_go_next_;
      bool can_go_previous_;
      bool can_play_;
      bool can_pause_;
      bool can_seek_;
      bool can_control_;
    };

    typedef class graphmgr_capabilities graphmgr_capabilities_t;
    typedef boost::shared_ptr< graphmgr_capabilities > graphmgr_capabilities_ptr_t;

  }  // namespace graphmgr
}  // namespace tiz

#endif  // TIZGRAPHMGRCAPS_HPP
