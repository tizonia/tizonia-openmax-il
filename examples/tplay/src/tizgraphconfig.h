/* -*-Mode: c++; -*- */
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
 * @file   tizgraphconfig.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph configuration base class
 *
 *
 */

#ifndef TIZGRAPHCONFIG_H
#define TIZGRAPHCONFIG_H

#include "tizgraphtypes.h"
#include <boost/shared_ptr.hpp>

namespace tiz
{
  namespace graph
  {

    class config
    {

    public:

      explicit config (const uri_lst_t & uris, const bool continuous_playback = true)
        : uris_ (uris), continuous_playback_ (continuous_playback)
      {}

      virtual ~config () {}

      uri_lst_t get_uris () const {return uris_;}
      void set_uris (const uri_lst_t & uris)  {uris_ = uris;}

      bool continuous_playback () const {return continuous_playback_;}

    protected:

      uri_lst_t uris_;
      bool continuous_playback_;
    };

  } // namespace graph
} // namespace tiz

#endif // TIZGRAPHCONFIG_H
