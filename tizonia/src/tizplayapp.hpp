/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   tizplayapp.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  tizonia app wrapper
 *
 *
 */

#ifndef TIZPLAYAPP_HPP
#define TIZPLAYAPP_HPP

#include <string.h>
#include <vector>

#include <OMX_Core.h>

#include "tizprogramopts.hpp"

namespace tiz
{
  class playapp
  {
  public:
    playapp (int argc, char* argv[]);
    ~playapp ();

    int run ();

  private:
    void set_option_handlers ();

    OMX_ERRORTYPE check_daemon_mode () const;

    OMX_ERRORTYPE unique_log_file () const;
    OMX_ERRORTYPE print_debug_info () const;
    OMX_ERRORTYPE list_of_comps () const;
    OMX_ERRORTYPE roles_of_comp () const;
    OMX_ERRORTYPE comp_of_role () const;
    OMX_ERRORTYPE decode_local ();
    OMX_ERRORTYPE serve_stream ();
    OMX_ERRORTYPE decode_stream ();
    OMX_ERRORTYPE spotify_stream ();

    void print_banner () const;

  private:
    programopts popts_;
  };
}
#endif  // TIZPLAYAPP_HPP
