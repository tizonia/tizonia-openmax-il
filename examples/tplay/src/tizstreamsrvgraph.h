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
 * @file   tizstreamsrvgraph.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL HTTP Streaming Server graph class
 *
 *
 */

#ifndef TIZSTREAMSRVGRAPH_H
#define TIZSTREAMSRVGRAPH_H

#include "tizgraph.h"
#include "tizgraphconfig.h"
#include "tizprobe.h"

class tizstreamsrvgraph : public tizgraph
{

public:

  tizstreamsrvgraph (tizprobe_ptr_t probe_ptr);

protected:

  OMX_ERRORTYPE do_load ();
  OMX_ERRORTYPE do_configure (const tizgraphconfig_ptr_t &config);
  OMX_ERRORTYPE do_execute ();
  OMX_ERRORTYPE do_pause ();
  OMX_ERRORTYPE do_seek ();
  OMX_ERRORTYPE do_skip (const int jump);
  OMX_ERRORTYPE do_volume (const int step);
  void          do_error (const OMX_ERRORTYPE error);
  void          do_eos (const OMX_HANDLETYPE handle);
  void          do_unload ();

  OMX_ERRORTYPE probe_uri (const int uri_index, const bool quiet = false);

private:

  OMX_ERRORTYPE configure_server ();
  OMX_ERRORTYPE configure_station ();
  OMX_ERRORTYPE configure_stream ();

};

#endif // TIZSTREAMSRVGRAPH_H

