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
 * @file   tizscloudgraph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  SoundCloud client graph implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include "tizscloudgraphops.hpp"
#include "tizscloudgraph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.scloud"
#endif

namespace graph = tiz::graph;

//
// scloud
//
graph::scloud::scloud () : tiz::graph::servicegraph ("scloudgraph")
{
}

graph::ops *graph::scloud::do_init ()
{
  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.audio_source.http");

  omx_comp_role_lst_t role_list;
  role_list.push_back ("audio_source.http.scloud");

  return new scloudops (this, comp_list, role_list);
}
