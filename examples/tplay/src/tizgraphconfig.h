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

class tizgraphconfig;
typedef boost::shared_ptr<tizgraphconfig> tizgraphconfig_ptr_t;

class tizgraphconfig
{

public:

  tizgraphconfig (const uri_list_t & uris)
    : uris_ (uris)
  {}

  virtual ~tizgraphconfig () {}

  uri_list_t get_uris () const {return uris_;}
  void set_uris (const uri_list_t & uris)  {uris_ = uris;}

protected:

  uri_list_t uris_;

};

#endif // TIZGRAPHCONFIG_H
