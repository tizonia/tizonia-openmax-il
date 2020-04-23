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
 * @file   tizradiograph.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Generic radio streaming service graph
 *
 *
 */

#ifndef TIZRADIOGRAPH_HPP
#define TIZRADIOGRAPH_HPP

#include <boost/any.hpp>

#include "tizgraph.hpp"

namespace tiz
{
  namespace graph
  {
    // Forward declarations
    class cmd;
    class ops;

    class radiograph : public graph
    {

    public:
      explicit radiograph (const std::string &graph_name);
      ~radiograph ();

    protected:
      bool dispatch_cmd (const tiz::graph::cmd *p_cmd);

    protected:
      boost::any fsm_;
    };

  }  // namespace graph
}  // namespace tiz

#endif  // TIZRADIOGRAPH_HPP
