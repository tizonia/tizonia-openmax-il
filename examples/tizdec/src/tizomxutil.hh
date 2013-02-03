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
 * @file   tizdec.hh
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - OMX IL graph utility functions
 *
 *
 */

#ifndef TIZOMXUTIL_HH
#define TIZOMXUTIL_HH

#include <vector>
#include <string>

#include "OMX_Core.h"

class tizomxutil
{
public:


  static void init();
  static void deinit();
  static OMX_ERRORTYPE list_comps(std::vector<std::string>& components);
  static OMX_ERRORTYPE roles_of_comp(const OMX_STRING comp,
                                     std::vector<std::string>& roles);
  static OMX_ERRORTYPE comps_of_role(const OMX_STRING role,
                                     std::vector<std::string>& components);

};

#endif // TIZOMXUTIL_HH
