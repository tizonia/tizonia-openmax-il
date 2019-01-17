/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   tizchromecastgraphfsm.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Chromecast renderer graph fsm
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizchromecastgraphfsm.hpp"

char const* const tiz::graph::ccfsm::pstate(tiz::graph::ccfsm::fsm const& p)
{
  return tiz::graph::ccfsm::state_names[p.current_state()[0]];
}

