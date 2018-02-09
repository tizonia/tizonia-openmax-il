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
 * @file   tizcastmgrtypes.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Chromecast daemon - Types and constants
 *
 *
 */

#ifndef TIZCASTMGRTYPES_H
#define TIZCASTMGRTYPES_H


#include <string>
#include <vector>

#include <boost/function.hpp>

#include <tizchromecasttypes.h>

namespace tiz
{
  namespace cast
  {

    typedef std::vector< unsigned char > uuid_t;

    typedef boost::function< void() > cast_status_received_cback_t;

    typedef boost::function< void(const uuid_t& uuid,
                                  const tiz_chromecast_cast_status_t a_status,
                                  const int a_volume) >
        cast_status_cback_t;

    typedef boost::function< void(const uuid_t& uuid,
                                  const tiz_chromecast_media_status_t a_status,
                                  const int a_volume) >
        media_status_cback_t;

    typedef boost::function< void(const uuid_t& uuid,
                                  const uint32_t& status,
                                  const std::string& error_str) >
        termination_callback_t;

  }  // namespace cast
}  // namespace tiz

#endif  // TIZCASTMGRTYPES_H
