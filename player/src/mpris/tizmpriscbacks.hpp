/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   tizmpriscbacks.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  MPRIS interface - property containers
 *
 *
 */

#ifndef TIZMPRISCBACKS_HPP
#define TIZMPRISCBACKS_HPP

#include <map>
#include <string>
#include <vector>

#include <boost/function.hpp>

#include <tizplatform.h>

namespace tiz
{
  namespace control
  {
    class mpris_callbacks
    {
    public:
      typedef boost::function< OMX_ERRORTYPE() > cback_func_t;
      typedef boost::function< OMX_ERRORTYPE(double) > cback_vol_func_t;

    public:
      mpris_callbacks (cback_func_t play,
                       cback_func_t next,
                       cback_func_t previous,
                       cback_func_t pause,
                       cback_func_t playpause,
                       cback_func_t stop,
                       cback_func_t quit,
                       cback_vol_func_t volume)
        :
        play_ (play),
        next_ (next),
        previous_ (previous),
        pause_ (pause),
        playpause_ (playpause),
        stop_ (stop),
        quit_ (quit),
        volume_ (volume)
      {}

    public:
      cback_func_t play_;
      cback_func_t next_;
      cback_func_t previous_;
      cback_func_t pause_;
      cback_func_t playpause_;
      cback_func_t stop_;
      cback_func_t quit_;
      cback_vol_func_t volume_;
    };

    typedef class mpris_callbacks mpris_callbacks_t;
    typedef boost::shared_ptr< mpris_callbacks_t > mpris_callbacks_ptr_t;
  }  // namespace control
}  // namespace tiz

#endif  // TIZMPRISCBACKS_HPP
