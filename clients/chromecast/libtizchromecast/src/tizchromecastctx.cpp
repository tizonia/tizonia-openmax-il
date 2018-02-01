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
 * @file   tizchromecastctx.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Chromecast client library
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <unistd.h>

#include <boost/lexical_cast.hpp>
#include <iostream>

#include "tizchromecastctx.hpp"

namespace bp = boost::python;

#define try_catch_wrapper(expr)                                  \
  do                                                             \
    {                                                            \
      try                                                        \
        {                                                        \
          (expr);                                                \
        }                                                        \
      catch (bp::error_already_set & e)                          \
        {                                                        \
          PyErr_PrintEx (0);                                     \
        }                                                        \
      catch (const std::exception &e)                            \
        {                                                        \
          std::cerr << e.what ();                                \
        }                                                        \
      catch (...)                                                \
        {                                                        \
          std::cerr << std::string ("Unknown exception caught"); \
        }                                                        \
    }                                                            \
  while (0)

namespace
{
  void init_cc_ctx (bp::object &py_main, bp::object &py_global, bp::object &py_chromecastproxy)
  {
    if (!Py_IsInitialized ())
      {
        Py_Initialize ();
      }

    // Import the Chromecast proxy module
    py_main = bp::import ("tizchromecastproxy");

    // Retrieve the main module's namespace
    py_global = py_main.attr ("__dict__");

    py_chromecastproxy = py_global["tizchromecastproxy"];
  }
}

tizchromecastctx::tizchromecastctx ()
{
  try_catch_wrapper (init_cc_ctx (py_main_, py_global_, py_chromecastproxy_));
}

tizchromecastctx::~tizchromecastctx ()
{
  // boost::python doesn't support Py_Finalize() yet!
}

bp::object & tizchromecastctx::get_cc_proxy (
    const std::string &arg /* = std::string() */) const
{
  if (!arg.empty ())
    {
      py_cc_proxy_instance_ = py_chromecastproxy_ (arg.c_str ());
    }
  return py_cc_proxy_instance_;
}
