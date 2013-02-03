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

#include "OMX_Audio.h"

#include "tizgraph.hh"

class tizmp3graph : public tizgraph
{

public:

  tizmp3graph() : tizgraph(3) {};
  ~tizmp3graph() {};

  OMX_ERRORTYPE instantiate(const component_names_t &comp_list);
  OMX_ERRORTYPE configure(const OMX_STRING file_uri);
  OMX_ERRORTYPE execute();
  void destroy();

private:

  void probe(const OMX_STRING file_uri,
             OMX_AUDIO_PARAM_MP3TYPE &mp3type) const;

};

#endif // TIZMP3GRAPH_HH

