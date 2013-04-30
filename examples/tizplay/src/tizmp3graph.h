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
 * @file   tizmp3graph.hh
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL mp3 graph class
 *
 *
 */

#ifndef TIZMP3GRAPH_HH
#define TIZMP3GRAPH_HH

#include "tizgraph.h"
#include "tizprobe.h"
#include "OMX_Audio.h"

#include <string>

class tizmp3graph : public tizgraph
{

public:

  tizmp3graph(tizprobe_ptr_t probe_ptr);

protected:

  OMX_ERRORTYPE do_load();
  OMX_ERRORTYPE do_configure(const std::string &uri = std::string());
  OMX_ERRORTYPE do_execute();
  OMX_ERRORTYPE do_pause();
  void do_unload();
  void do_signal();
};

#endif // TIZMP3GRAPH_HH

