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
 * @brief  Chromecast client daemon - Types and constants
 *
 *
 */

#ifndef TIZCASTMGRTYPES_H
#define TIZCASTMGRTYPES_H

#include <boost/function.hpp>

#include <tizchromecasttypes.h>
namespace tiz
{
  namespace castmgr
  {

    typedef boost::function< void() > cast_status_received_cback_t;

    typedef boost::function< void(const std::string& device_or_ip,
                                  const tiz_chromecast_cast_status_t a_status,
                                  const int a_volume) >
        cast_status_cback_t;
    typedef boost::function< void(const std::string& device_or_ip,
                                  const tiz_chromecast_media_status_t a_status,
                                  const int a_volume) >
        media_status_cback_t;

  }  // namespace castmgr
}  // namespace tiz

#endif  // TIZCASTMGRTYPES_H
