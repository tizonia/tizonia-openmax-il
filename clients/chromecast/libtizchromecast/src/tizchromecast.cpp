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
 * @file   tizchromecast.cpp
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

#include "tizchromecast.hpp"

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
  void init_chromecast (bp::object &py_main, bp::object &py_global)
  {
    Py_Initialize ();

    // Import the Chromecast proxy module
    py_main = bp::import ("tizchromecastproxy");

    // Retrieve the main module's namespace
    py_global = py_main.attr ("__dict__");
  }

  void start_chromecast (const bp::object &py_global, bp::object &py_cc_proxy,
                         const std::string &name_or_ip)
  {
    bp::object pychromecastproxy = py_global["tizchromecastproxy"];
    py_cc_proxy = pychromecastproxy (name_or_ip.c_str ());
  }
}

tizchromecast::tizchromecast (const std::string &name_or_ip)
  : name_or_ip_ (name_or_ip)
{
}

tizchromecast::~tizchromecast ()
{
}

int tizchromecast::init ()
{
  int rc = 0;
  try_catch_wrapper (init_chromecast (py_main_, py_global_));
  return rc;
}

int tizchromecast::start ()
{
  int rc = 0;
  try_catch_wrapper (start_chromecast (py_global_, py_cc_proxy_, name_or_ip_));
  typedef boost::function< void(std::string) > handler_fn;
  handler_fn cast_status_handler (
      boost::bind (&tizchromecast::new_cast_status, this, _1));
  handler_fn media_status_handler (
      boost::bind (&tizchromecast::new_media_status, this, _1));
  try_catch_wrapper (
      py_cc_proxy_.attr ("start") (bp::make_function (cast_status_handler),
                                   bp::make_function (media_status_handler)));
  return rc;
}

void tizchromecast::stop ()
{
  int rc = 0;
  try_catch_wrapper (py_cc_proxy_.attr ("stop") ());
  (void)rc;
}

void tizchromecast::deinit ()
{
  // boost::python doesn't support Py_Finalize() yet!
}

int tizchromecast::media_load (const std::string &url,
                               const std::string &content_type,
                               const std::string &title)
{
  int rc = 0;
  try_catch_wrapper (py_cc_proxy_.attr ("media_load") (
      bp::object (url), bp::object (content_type), bp::object (title)));
  return rc;
}

int tizchromecast::media_play ()
{
  int rc = 0;
  try_catch_wrapper (py_cc_proxy_.attr ("media_play") ());
  return rc;
}

int tizchromecast::media_stop ()
{
  int rc = 0;
  try_catch_wrapper (py_cc_proxy_.attr ("media_stop") ());
  return rc;
}

int tizchromecast::media_pause ()
{
  int rc = 0;
  try_catch_wrapper (py_cc_proxy_.attr ("media_pause") ());
  return rc;
}

int tizchromecast::media_volume_up ()
{
  int rc = 0;
  try_catch_wrapper (py_cc_proxy_.attr ("media_vol_up") ());
  return rc;
}

int tizchromecast::media_volume_down ()
{
  int rc = 0;
  try_catch_wrapper (py_cc_proxy_.attr ("media_vol_down") ());
  return rc;
}

int tizchromecast::media_mute ()
{
  int rc = 0;
  try_catch_wrapper (py_cc_proxy_.attr ("media_mute") ());
  return rc;
}

int tizchromecast::media_unmute ()
{
  int rc = 0;
  try_catch_wrapper (py_cc_proxy_.attr ("media_unmute") ());
  return rc;
}

void tizchromecast::new_cast_status (const std::string &status)
{
  std::cout << "tizchromecast::new_cast_status: " << status << " pid "<< getpid() << std::endl;
}

void tizchromecast::new_media_status (const std::string &status)
{
  std::cout << "tizchromecast::new_media_status: " << status << " pid "<< getpid() << std::endl;
}

// int tizchromecast::get_current_track ()
// {
//   int rc = 1;
//   current_user_.clear ();
//   current_title_.clear ();

//   const bp::tuple &info1 = bp::extract< bp::tuple >(
//       py_cc_proxy_.attr ("current_track_title_and_user")());
//   const char *p_user = bp::extract< char const * >(info1[0]);
//   const char *p_title = bp::extract< char const * >(info1[1]);

//   if (p_user)
//     {
//       current_user_.assign (p_user);
//     }
//   if (p_title)
//     {
//       current_title_.assign (p_title);
//     }

//   int duration
//       = bp::extract< int >(py_cc_proxy_.attr ("current_track_duration")());

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
//           current_duration_.append (boost::lexical_cast< std::string
//           >(hours));
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

//   const int track_year = bp::extract< int >(py_cc_proxy_.attr
//   ("current_track_year")());
//   current_track_year_.assign (boost::lexical_cast< std::string
//   >(track_year));

//   const char *p_track_permalink = bp::extract< char const * >(
//       py_cc_proxy_.attr ("current_track_permalink")());
//   if (p_track_permalink)
//     {
//       current_track_permalink_.assign (p_track_permalink);
//     }

//   const char *p_track_license = bp::extract< char const * >(
//       py_cc_proxy_.attr ("current_track_license")());
//   if (p_track_license)
//     {
//       current_track_license_.assign (p_track_license);
//     }

//   const int track_likes = bp::extract< int >(py_cc_proxy_.attr
//   ("current_track_likes")());
//   current_track_likes_.assign (boost::lexical_cast< std::string
//   >(track_likes));

//   if (p_user || p_title)
//     {
//       rc = 0;
//     }

//   return rc;
// }
