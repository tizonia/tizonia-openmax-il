/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   tizhttpservgraphfsm.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  HTTP server graph fsm
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizhttpservgraphfsm.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.httpservgraph.fsm"
#endif

namespace hsfsm = tiz::graph::hsfsm;

char const* const hsfsm::pstate(hsfsm::fsm const& p)
{
  return hsfsm::state_names[p.current_state()[0]];
}

