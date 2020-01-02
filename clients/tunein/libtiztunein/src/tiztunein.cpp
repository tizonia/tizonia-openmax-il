/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   tiztunein.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Tunein client library
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/lexical_cast.hpp>
#include <iostream>

#include "tiztunein.hpp"

namespace bp = boost::python;

/* This macro assumes the existence of an "int rc" local variable */
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
          rc = 1;                                                \
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
  void init_tunein (boost::python::object &py_main,
                    boost::python::object &py_global)
  {
    Py_Initialize ();

    // Import the Tunein proxy module
    py_main = bp::import ("tiztuneinproxy");

    // Retrieve the main module's namespace
    py_global = py_main.attr ("__dict__");
  }

  void start_tunein (boost::python::object &py_global,
                     boost::python::object &py_tunein_proxy)
  {
    bp::object pytuneinproxy = py_global["tiztuneinproxy"];
    py_tunein_proxy = pytuneinproxy ();
  }
}

tiztunein::tiztunein ()
{
}

tiztunein::~tiztunein ()
{
}

int tiztunein::init ()
{
  int rc = 0;
  try_catch_wrapper (init_tunein (py_main_, py_global_));
  return rc;
}

int tiztunein::start ()
{
  int rc = 0;
  try_catch_wrapper (start_tunein (py_global_, py_tunein_proxy_));
  return rc;
}

void tiztunein::stop ()
{
  int rc = 0;
  // try_catch_wrapper (py_tunein_proxy_.attr ("logout")());
  (void)rc;
}

void tiztunein::deinit ()
{
  // boost::python doesn't support Py_Finalize() yet!
}

int tiztunein::play_radios (const std::string &query)
{
  int rc = 0;
  try_catch_wrapper (
      py_tunein_proxy_.attr ("enqueue_radios") (bp::object (query)));
  return rc;
}

int tiztunein::play_category (const std::string &category)
{
  int rc = 0;
  try_catch_wrapper (
      py_tunein_proxy_.attr ("enqueue_category") (bp::object (category)));
  return rc;
}

const char *tiztunein::get_next_url (const bool a_remove_current_url)
{
  current_url_.clear ();
  try
    {
      if (a_remove_current_url)
        {
          py_tunein_proxy_.attr ("remove_current_url") ();
        }
      current_url_
          = bp::extract< std::string > (py_tunein_proxy_.attr ("next_url") ());
      get_current_radio ();
    }
  catch (bp::error_already_set &e)
    {
      PyErr_PrintEx (0);
    }
  catch (...)
    {
    }
  return current_url_.empty () ? NULL : current_url_.c_str ();
}

const char *tiztunein::get_prev_url (const bool a_remove_current_url)
{
  current_url_.clear ();
  try
    {
      if (a_remove_current_url)
        {
          py_tunein_proxy_.attr ("remove_current_url") ();
        }
      current_url_
          = bp::extract< std::string > (py_tunein_proxy_.attr ("prev_url") ());
      get_current_radio ();
    }
  catch (bp::error_already_set &e)
    {
      PyErr_PrintEx (0);
    }
  catch (...)
    {
    }
  return current_url_.empty () ? NULL : current_url_.c_str ();
}

const char *tiztunein::get_current_radio_name ()
{
  return current_radio_name_.empty () ? NULL : current_radio_name_.c_str ();
}

const char *tiztunein::get_current_radio_description ()
{
  return current_radio_description_.empty () ? NULL : current_radio_description_.c_str ();
}

const char *tiztunein::get_current_radio_country ()
{
  return current_radio_country_.empty () ? NULL
                                           : current_radio_country_.c_str ();
}

const char *tiztunein::get_current_radio_category ()
{
  return current_radio_category_.empty ()
             ? NULL
             : current_radio_category_.c_str ();
}

const char *tiztunein::get_current_radio_website ()
{
  return current_radio_website_.empty () ? NULL
                                           : current_radio_website_.c_str ();
}

const char *tiztunein::get_current_radio_bitrate ()
{
  return current_radio_bitrate_.empty () ? NULL
                                           : current_radio_bitrate_.c_str ();
}

const char *tiztunein::get_current_radio_stream_url ()
{
  return current_url_.empty () ? NULL : current_url_.c_str ();
}

const char *tiztunein::get_current_radio_thumbnail_url ()
{
  return current_radio_thumbnail_url_.empty () ? NULL : current_radio_thumbnail_url_.c_str ();
}

void tiztunein::clear_queue ()
{
  int rc = 0;
  try_catch_wrapper (py_tunein_proxy_.attr ("clear_queue") ());
  (void)rc;
}

void tiztunein::set_playback_mode (const playback_mode mode)
{
  int rc = 0;
  switch (mode)
    {
      case PlaybackModeNormal:
        {
          try_catch_wrapper (
              py_tunein_proxy_.attr ("set_play_mode") ("NORMAL"));
        }
        break;
      case PlaybackModeShuffle:
        {
          try_catch_wrapper (
              py_tunein_proxy_.attr ("set_play_mode") ("SHUFFLE"));
        }
        break;
      default:
        {
          assert (0);
        }
        break;
    };
  (void)rc;
}

void tiztunein::set_search_mode (const search_mode mode)
{
  int rc = 0;
  switch (mode)
    {
      case SearchModeAll:
        {
          try_catch_wrapper (
              py_tunein_proxy_.attr ("set_search_mode") ("ALL"));
        }
        break;
      case SearchModeStations:
        {
          try_catch_wrapper (
              py_tunein_proxy_.attr ("set_search_mode") ("STATIONS"));
        }
        break;
      case SearchModeShows:
        {
          try_catch_wrapper (
              py_tunein_proxy_.attr ("set_search_mode") ("SHOWS"));
        }
        break;
      default:
        {
          assert (0);
        }
        break;
    };
  (void)rc;
}

void tiztunein::get_current_radio ()
{
  current_radio_name_.clear ();
  current_radio_country_.clear ();

  current_radio_name_ = bp::extract< std::string > (
      py_tunein_proxy_.attr ("current_radio_name") ());

  current_radio_description_ = bp::extract< std::string > (
      py_tunein_proxy_.attr ("current_radio_description") ());

  current_radio_category_ = bp::extract< std::string > (
      py_tunein_proxy_.attr ("current_radio_category") ());

  const int bitrate = bp::extract< int > (
      py_tunein_proxy_.attr ("current_radio_bitrate") ());
  current_radio_bitrate_.assign (
      boost::lexical_cast< std::string > (bitrate));

  current_radio_thumbnail_url_ = bp::extract< std::string > (
      py_tunein_proxy_.attr ("current_radio_thumbnail_url") ());

}
