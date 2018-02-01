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
 * @file   tizchromecastctx.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Chromecast client library
 *
 *
 */

#ifndef TIZCHROMECASTCTX_HPP
#define TIZCHROMECASTCTX_HPP

#include <boost/python.hpp>

#include <string>

class tizchromecastctx
{
public:
  tizchromecastctx ();
  ~tizchromecastctx ();
  boost::python::object & get_cc_proxy (const std::string& arg
                                        = std::string ()) const;

private:
  boost::python::object py_main_;
  boost::python::object py_global_;
  mutable boost::python::object py_chromecastproxy_;
  mutable boost::python::object py_cc_proxy_instance_;
};

#endif  // TIZCHROMECASTCTX_HPP
