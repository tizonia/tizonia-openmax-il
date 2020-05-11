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
 * @file   tiziheart.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Iheart client library
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <iostream>

#include "tiziheart.hpp"

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
  int check_deps ()
  {
    int rc = 1;
    Py_Initialize ();

    try
      {
        // Import the Tizonia Plex proxy module
        bp::object py_main = bp::import ("__main__");

        // Retrieve the main module's namespace
        bp::object py_global = py_main.attr ("__dict__");

        // Check the existence of the 'joblib' module
        bp::object ignored = exec (
            "import importlib\n"
            "spec = importlib.util.find_spec('joblib')\n"
            "if not spec:\n raise ValueError\n",
            py_global);

        // Check the existence of the 'fuzzywuzzy' module
        bp::object ignored2 = exec (
            "import importlib\n"
            "spec = importlib.util.find_spec('fuzzywuzzy')\n"
            "if not spec:\n raise ValueError\n",
            py_global);

        rc = 0;
      }
    catch (bp::error_already_set &e)
      {
        PyErr_PrintEx (0);
        std::cerr << std::string (
            "\nPython modules 'joblib' or 'fuzzywuzzy' not found."
            "\nPlease make sure these are installed correctly.\n");
      }
    catch (...)
      {
        std::cerr << std::string ("Unknown exception caught");
      }
    return rc;
  }

  void init_iheart (boost::python::object &py_main,
                    boost::python::object &py_global)
  {
    // Import the Iheart proxy module
    py_main = bp::import ("tiziheartproxy");

    // Retrieve the main module's namespace
    py_global = py_main.attr ("__dict__");
  }

  void start_iheart (boost::python::object &py_global,
                     boost::python::object &py_iheart_proxy)
  {
    bp::object pyiheartproxy = py_global["tiziheartproxy"];
    py_iheart_proxy = pyiheartproxy ();
  }
}

tiziheart::tiziheart ()
  : current_url_ (),
    current_radio_index_ (),
    current_queue_length_ ()
{
}

tiziheart::~tiziheart ()
{
}

int tiziheart::init ()
{
  int rc = 0;
  if (0 == (rc = check_deps ()))
    {
      try_catch_wrapper (init_iheart (py_main_, py_global_));
    }
  return rc;
}

int tiziheart::start ()
{
  int rc = 0;
  try_catch_wrapper (start_iheart (py_global_, py_iheart_proxy_));
  return rc;
}

void tiziheart::stop ()
{
  int rc = 0;
  // try_catch_wrapper (py_iheart_proxy_.attr ("logout")());
  (void)rc;
}

void tiziheart::deinit ()
{
  // boost::python doesn't support Py_Finalize() yet!
}

int tiziheart::play_radios (const std::string &query,
                            const std::string &keywords1,
                            const std::string &keywords2,
                            const std::string &keywords3)
{
  int rc = 0;
  try_catch_wrapper (py_iheart_proxy_.attr ("enqueue_radios") (
      bp::object (query), bp::object (keywords1), bp::object (keywords2),
      bp::object (keywords3)));
  return rc;
}

const char *tiziheart::get_url (const int a_position)
{
  try
    {
      int queue_length = get_current_queue_length_as_int();
      current_url_.clear ();
      if (queue_length > 0 && a_position >= 0 && queue_length >= a_position)
        {
          const int pos = (0 == a_position) ? queue_length : a_position;
          current_url_ = bp::extract< std::string > (
              py_iheart_proxy_.attr ("get_url") (bp::object (pos)));
          get_current_radio ();
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

const char *tiziheart::get_next_url (const bool a_remove_current_url)
{
  current_url_.clear ();
  try
    {
      if (a_remove_current_url)
        {
          py_iheart_proxy_.attr ("remove_current_url") ();
        }
      current_url_
          = bp::extract< std::string > (py_iheart_proxy_.attr ("next_url") ());
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

const char *tiziheart::get_prev_url (const bool a_remove_current_url)
{
  current_url_.clear ();
  try
    {
      if (a_remove_current_url)
        {
          py_iheart_proxy_.attr ("remove_current_url") ();
        }
      current_url_
          = bp::extract< std::string > (py_iheart_proxy_.attr ("prev_url") ());
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

const char *tiziheart::get_current_radio_name ()
{
  return current_radio_name_.empty () ? NULL : current_radio_name_.c_str ();
}

const char *tiziheart::get_current_radio_description ()
{
  return current_radio_description_.empty () ? NULL : current_radio_description_.c_str ();
}

const char *tiziheart::get_current_radio_city ()
{
  return current_radio_city_.empty ()
             ? NULL
             : current_radio_city_.c_str ();
}

const char *tiziheart::get_current_radio_state ()
{
  return current_radio_state_.empty () ? NULL : current_radio_state_.c_str ();
}

const char *tiziheart::get_current_radio_audio_encoding ()
{
  return current_radio_audio_encoding_.empty () ? NULL : current_radio_audio_encoding_.c_str ();
}

const char *tiziheart::get_current_radio_website_url ()
{
  return current_radio_website_url_.empty ()
             ? NULL
             : current_radio_website_url_.c_str ();
}

const char *tiziheart::get_current_radio_stream_url ()
{
  return current_url_.empty () ? NULL : current_url_.c_str ();
}

const char *tiziheart::get_current_radio_thumbnail_url ()
{
  return current_radio_thumbnail_url_.empty () ? NULL : current_radio_thumbnail_url_.c_str ();
}

void tiziheart::clear_queue ()
{
  int rc = 0;
  try_catch_wrapper (py_iheart_proxy_.attr ("clear_queue") ());
  (void)rc;
}

void tiziheart::print_queue ()
{
  int rc = 0;
  try_catch_wrapper (py_iheart_proxy_.attr ("print_queue") ());
  (void)rc;
}

const char *tiziheart::get_current_radio_index ()
{
  obtain_current_queue_progress ();
  return current_radio_index_.empty () ? NULL : current_radio_index_.c_str ();
}

const char *tiziheart::get_current_queue_length ()
{
  obtain_current_queue_progress ();
  return current_queue_length_.empty () ? NULL : current_queue_length_.c_str ();
}

int tiziheart::get_current_queue_length_as_int ()
{
  obtain_current_queue_progress ();
  if (current_queue_length_.empty())
    {
      return 0;
    }
  boost::algorithm::trim(current_queue_length_);
  return boost::lexical_cast< int > (current_queue_length_);
}

const char *tiziheart::get_current_queue_progress ()
{
    const bp::tuple &queue_info = bp::extract< bp::tuple > (
      py_iheart_proxy_.attr ("current_radio_queue_index_and_queue_length") ());
  const int queue_index = bp::extract< int > (queue_info[0]);
  const int queue_length = bp::extract< int > (queue_info[1]);
  current_radio_index_.assign (
      boost::lexical_cast< std::string > (queue_index));
  current_queue_length_.assign (
      boost::lexical_cast< std::string > (queue_length));

  current_queue_progress_.assign (get_current_radio_index ());
  current_queue_progress_.append (" of ");
  current_queue_progress_.append (get_current_queue_length ());
  return current_queue_progress_.c_str ();
}

void tiziheart::set_playback_mode (const playback_mode mode)
{
  int rc = 0;
  switch (mode)
    {
      case PlaybackModeNormal:
        {
          try_catch_wrapper (
              py_iheart_proxy_.attr ("set_play_mode") ("NORMAL"));
        }
        break;
      case PlaybackModeShuffle:
        {
          try_catch_wrapper (
              py_iheart_proxy_.attr ("set_play_mode") ("SHUFFLE"));
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

void tiziheart::get_current_radio ()
{
  current_radio_name_.clear ();

  obtain_current_queue_progress ();

  current_radio_name_ = bp::extract< std::string > (
      py_iheart_proxy_.attr ("current_radio_name") ());

  current_radio_description_ = bp::extract< std::string > (
      py_iheart_proxy_.attr ("current_radio_description") ());

  current_radio_city_ = bp::extract< std::string > (
      py_iheart_proxy_.attr ("current_radio_city") ());

  current_radio_state_ = bp::extract< std::string > (
      py_iheart_proxy_.attr ("current_radio_state") ());

  current_radio_audio_encoding_ = bp::extract< std::string > (
      py_iheart_proxy_.attr ("current_radio_audio_encoding") ());

  current_radio_website_url_ = bp::extract< std::string > (
      py_iheart_proxy_.attr ("current_radio_website_url") ());

  current_radio_thumbnail_url_ = bp::extract< std::string > (
      py_iheart_proxy_.attr ("current_radio_thumbnail_url") ());

}

void tiziheart::obtain_current_queue_progress ()
{
  current_radio_index_.clear ();
  current_queue_length_.clear ();

  const bp::tuple &queue_info = bp::extract< bp::tuple > (
      py_iheart_proxy_.attr ("current_radio_queue_index_and_queue_length") ());
  const int queue_index = bp::extract< int > (queue_info[0]);
  const int queue_length = bp::extract< int > (queue_info[1]);
  current_radio_index_.assign (
      boost::lexical_cast< std::string > (queue_index));
  current_queue_length_.assign (
      boost::lexical_cast< std::string > (queue_length));
}
