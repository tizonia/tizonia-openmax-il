/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   tizdaemon.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - Utility to encapsulate the steps required to
 * turn a process into a deamon.
 *
 */

#ifndef TIZDAEMON_HPP
#define TIZDAEMON_HPP

namespace tiz
{
  class daemon
  {
  public:
    /**
     *  This function performs the steps required to turn a process into a
     *  deamon.
     *
     *  @return 0 on success, -1 on error
     */
    static int daemonize ();
  };
}  // namespace tiz

#endif  // TIZDAEMON_HPP
