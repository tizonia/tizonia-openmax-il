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
 * @file   tizdirble.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Dirble client library
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <boost/lexical_cast.hpp>

#include "tizdirble.hpp"

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
  void init_dirble (boost::python::object &py_main,
                    boost::python::object &py_global)
  {
    Py_Initialize ();

    // Import the Dirble proxy module
    py_main = bp::import ("tizdirbleproxy");

    // Retrieve the main module's namespace
    py_global = py_main.attr ("__dict__");
  }

  void start_dirble (boost::python::object &py_global,
                     boost::python::object &py_dirble_proxy,
                     const std::string &api_key)
  {
    bp::object pydirbleproxy = py_global["tizdirbleproxy"];
    py_dirble_proxy
        = pydirbleproxy (api_key.c_str ());
  }
}

tizdirble::tizdirble (const std::string &api_key)
  : api_key_ (api_key)
{
}

tizdirble::~tizdirble ()
{
}

int tizdirble::init ()
{
  int rc = 0;
  try_catch_wrapper (init_dirble (py_main_, py_global_));
  return rc;
}

int tizdirble::start ()
{
  int rc = 0;
  try_catch_wrapper (
      start_dirble (py_global_, py_dirble_proxy_, api_key_));
  return rc;
}

void tizdirble::stop ()
{
  int rc = 0;
  // try_catch_wrapper (py_dirble_proxy_.attr ("logout")());
  (void)rc;
}

void tizdirble::deinit ()
{
  // boost::python doesn't support Py_Finalize() yet!
}

int tizdirble::play_popular_stations ()
{
  int rc = 0;
  try_catch_wrapper (py_dirble_proxy_.attr ("enqueue_popular_stations")());
  return rc;
}

int tizdirble::play_stations (const std::string &query)
{
  int rc = 0;
  try_catch_wrapper (
      py_dirble_proxy_.attr ("enqueue_stations")(bp::object (query)));
  return rc;
}

int tizdirble::play_category (const std::string &category)
{
  int rc = 0;
  try_catch_wrapper (
      py_dirble_proxy_.attr ("enqueue_category")(bp::object (category)));
  return rc;
}

int tizdirble::play_country (const std::string &country_code)
{
  int rc = 0;
  try_catch_wrapper (
      py_dirble_proxy_.attr ("enqueue_country")(bp::object (country_code)));
  return rc;
}

const char *tizdirble::get_next_url (const bool a_remove_current_url)
{
  current_url_.clear ();
  try
    {
      if (a_remove_current_url)
        {
          py_dirble_proxy_.attr ("remove_current_url")();
        }
      const char *p_next_url
          = bp::extract< char const * >(py_dirble_proxy_.attr ("next_url")());
      current_url_.assign (p_next_url);
      if (!p_next_url || get_current_station ())
        {
          current_url_.clear ();
        }
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

const char *tizdirble::get_prev_url (const bool a_remove_current_url)
{
  current_url_.clear ();
  try
    {
      if (a_remove_current_url)
        {
          py_dirble_proxy_.attr ("remove_current_url")();
        }
      const char *p_prev_url
          = bp::extract< char const * >(py_dirble_proxy_.attr ("prev_url")());
      current_url_.assign (p_prev_url);
      if (!p_prev_url || get_current_station ())
        {
          current_url_.clear ();
        }
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

const char *tizdirble::get_current_station_name ()
{
  return current_station_name_.empty () ? NULL : current_station_name_.c_str ();
}

const char *tizdirble::get_current_station_country ()
{
  return current_station_country_.empty () ? NULL : current_station_country_.c_str ();
}

const char *tizdirble::get_current_station_category ()
{
  return current_station_category_.empty () ? NULL : current_station_category_.c_str ();
}

const char *tizdirble::get_current_station_website ()
{
  return current_station_website_.empty () ? NULL : current_station_website_.c_str ();
}

void tizdirble::clear_queue ()
{
  int rc = 0;
  try_catch_wrapper (py_dirble_proxy_.attr ("clear_queue")());
  (void)rc;
}

void tizdirble::set_playback_mode (const playback_mode mode)
{
  int rc = 0;
  switch(mode)
    {
    case PlaybackModeNormal:
      {
        try_catch_wrapper (py_dirble_proxy_.attr ("set_play_mode")("NORMAL"));
      }
      break;
    case PlaybackModeShuffle:
      {
        try_catch_wrapper (py_dirble_proxy_.attr ("set_play_mode")("SHUFFLE"));
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

int tizdirble::get_current_station ()
{
  int rc = 1;
  current_station_name_.clear ();
  current_station_country_.clear ();

  const bp::tuple &info1 = bp::extract< bp::tuple >(
      py_dirble_proxy_.attr ("current_station_name_and_country")());
  const char *p_name = bp::extract< char const * >(info1[0]);
  const char *p_country = bp::extract< char const * >(info1[1]);

  if (p_name)
    {
      current_station_name_.assign (p_name);
    }
  if (p_country)
    {
      current_station_country_.assign (p_country);
    }

  const char *p_category = bp::extract< char const * >(
      py_dirble_proxy_.attr ("current_station_category")());
  if (p_category)
    {
      current_station_category_.assign (p_category);
    }

  const char *p_website = bp::extract< char const * >(
      py_dirble_proxy_.attr ("current_station_website")());
  if (p_website)
    {
      current_station_website_.assign (p_website);
    }

//   int duration
//       = bp::extract< int >(py_dirble_proxy_.attr ("current_track_duration")());

//   int seconds = 0;
//   current_duration_.clear ();
//   if (duration)
//     {
//       duration /= 1000;
//       seconds = duration % 60;
//       int minutes = (duration - seconds) / 60;
//       int hours = 0;
//       if (minutes >= 60)
//         {
//           int total_minutes = minutes;
//           minutes = total_minutes % 60;
//           hours = (total_minutes - minutes) / 60;
//         }

//       if (hours > 0)
//         {
//           current_duration_.append (boost::lexical_cast< std::string >(hours));
//           current_duration_.append ("h:");
//         }

//       if (minutes > 0)
//         {
//           current_duration_.append (
//               boost::lexical_cast< std::string >(minutes));
//           current_duration_.append ("m:");
//         }
//     }

//   char seconds_str[3];
//   sprintf (seconds_str, "%02i", seconds);
//   current_duration_.append (seconds_str);
//   current_duration_.append ("s");

//   const int track_year = bp::extract< int >(py_dirble_proxy_.attr ("current_track_year")());
//   current_track_year_.assign (boost::lexical_cast< std::string >(track_year));

//   const char *p_track_permalink = bp::extract< char const * >(
//       py_dirble_proxy_.attr ("current_track_permalink")());
//   if (p_track_permalink)
//     {
//       current_track_permalink_.assign (p_track_permalink);
//     }

//   const char *p_track_license = bp::extract< char const * >(
//       py_dirble_proxy_.attr ("current_track_license")());
//   if (p_track_license)
//     {
//       current_track_license_.assign (p_track_license);
//     }

//   const int track_likes = bp::extract< int >(py_dirble_proxy_.attr ("current_track_likes")());
//   current_track_likes_.assign (boost::lexical_cast< std::string >(track_likes));

  if (p_name)
     {
        rc = 0;
     }

  return rc;
}
