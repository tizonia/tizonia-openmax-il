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
 * @file   tizplaybacksignals.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Media player signals
 *
 *
 */

#ifndef TIZPLAYBACKSIGNALS_HPP
#define TIZPLAYBACKSIGNALS_HPP

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/signals2/signal.hpp>

#include <tizplatform.h>

namespace tiz
{
  namespace control
  {
    typedef class playback_signals playback_signals_t;
    class playback_signals
    {
    public:
      playback_signals ();

    public:
      boost::signals2::signal<void (std::string)> PlaybackStatus_;
      boost::signals2::signal<void (std::string)> LoopStatus_;
      boost::signals2::signal<void (std::string)> Metadata_;
      boost::signals2::signal<void (double)> Volume_;
    };

    typedef boost::shared_ptr< playback_signals_t >
        playback_signals_ptr_t;
    typedef boost::scoped_ptr< playback_signals_t >
        playback_signals_scoped_ptr_t;
  }  // namespace control
}  // namespace tiz

#endif  // TIZPLAYBACKSIGNALS_HPP
