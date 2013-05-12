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
 * @file   tizstreamsrvgraph.hh
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL HTTP Streaming Server graph class
 *
 *
 */

#ifndef TIZSTREAMSRVGRAPH_HH
#define TIZSTREAMSRVGRAPH_HH

#include "tizgraph.h"
#include "tizprobe.h"
#include "OMX_Audio.h"

#include <string>

class tizstreamsrvgraph : public tizgraph
{

public:

  tizstreamsrvgraph (tizprobe_ptr_t probe_ptr);

protected:

  OMX_ERRORTYPE do_load ();
  OMX_ERRORTYPE do_configure (const uri_list_t &uri_list = uri_list_t ());
  OMX_ERRORTYPE do_execute ();
  OMX_ERRORTYPE do_pause ();
  OMX_ERRORTYPE do_seek ();
  OMX_ERRORTYPE do_skip (const int jump);
  OMX_ERRORTYPE do_volume ();
  void          do_eos (const OMX_HANDLETYPE handle);
  void          do_unload ();

private:

  OMX_ERRORTYPE configure_streamsrv_graph ();

private:

  uri_list_t file_list_;
  int current_file_index_;
};

#endif // TIZSTREAMSRVGRAPH_HH

