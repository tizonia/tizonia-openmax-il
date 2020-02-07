/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   tizprogressdisplay.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Track progress display implementation.
 *
 */

#include <cstdlib>
#include <unistd.h>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/progress.hpp>

#include "tizprogressdisplay.hpp"

namespace graph = tiz::graph;

namespace
{
  enum color_code
  {
    FG_RED = 31,
    FG_GREEN = 32,
    FG_YELLOW = 33,
    FG_BLUE = 34,
    FG_MAGENTA = 36,
    FG_LIGHT_GREY = 37,
    FG_DEFAULT = 39,
    FG_LIGHT_RED = 91,
    BG_RED = 41,
    BG_GREEN = 42,
    BG_BLUE = 44,
    BG_CYAN = 46,
    BG_LIGHT_YELLOW = 103,
    BG_LIGHT_MAGENTA = 105,
    BG_DEFAULT = 49
  };
  class color_modifier
  {
    color_code code;

  public:
    color_modifier (color_code pCode) : code (pCode)
    {
    }
    friend std::ostream &operator<< (std::ostream &os,
                                     const color_modifier &mod)
    {
      return os << "\033[" << mod.code << "m";
    }
  };

  color_modifier magenta (FG_MAGENTA);
  color_modifier green (BG_CYAN);
  color_modifier lgrey (FG_LIGHT_GREY);
  color_modifier blue (BG_RED);
  color_modifier def (FG_DEFAULT);
  color_modifier defbg (BG_DEFAULT);
}

graph::progress_display::progress_display (unsigned long expected_count,
                                           std::ostream &os,
                                           const std::string &s1,
                                           const std::string &s2,
                                           const std::string &s3)
  // os is hint; implementation may ignore, particularly in embedded
  // systems
  : noncopyable (), m_os (os), m_s1 (s1), m_s2 (s2), m_s3 (s3), m_os_temp ()
{
  restart (expected_count);
}

graph::progress_display::~progress_display ()
{
  m_os << std::endl << std::endl;  // endl implies flush, which ensures display
}

void graph::progress_display::restart (unsigned long expected_count)
//  Effects: display appropriate scale
//  Postconditions: count()==0, expected_count()==expected_count
{
  _count = _next_tic_count = _tic = 0;
  _expected_count = expected_count;
  m_os_temp.clear ();
  std::string _total_elapsed_time = calc_len (expected_count);
  m_os << m_s1 << lgrey
       << "   0%   10   20   30   40   50   60   70   80   90   100%" << def
       << "\n"
       << lgrey << m_s2 << def << magenta
       << "|----|----|----|----|----|----|----|----|----|----| " << def << lgrey
       << _total_elapsed_time << def
       << std::endl  // endl implies flush, which ensures display
       << m_s3;
  if (!_expected_count)
  {
    _expected_count = 1;  // prevent divide by zero
  }                       // restart
}

unsigned long graph::progress_display::count () const
{
  return _count;
}

unsigned long graph::progress_display::expected_count () const
{
  return _expected_count;
}

std::string graph::progress_display::calc_len (unsigned long count)
{
  std::string length_str;
  int seconds = count % 60;
  int minutes = (count - seconds) / 60;
  int hours = 0;
  if (minutes >= 60)
  {
    int total_minutes = minutes;
    minutes = total_minutes % 60;
    hours = (total_minutes - minutes) / 60;
  }

  if (hours > 0)
  {
    length_str.append (boost::lexical_cast< std::string > (hours));
    length_str.append ("h:");
  }

  if (minutes > 0)
  {
    length_str.append (boost::lexical_cast< std::string > (minutes));
    length_str.append ("m:");
  }

  char seconds_str[6];
  sprintf (seconds_str, "%02i", seconds);
  length_str.append (seconds_str);
  length_str.append ("s");

  return length_str;
}

void graph::progress_display::add_tic ()
{
  // use of floating point ensures that both large and small counts
  // work correctly.  static_cast<>() is also used several places
  // to suppress spurious compiler warnings.
  unsigned int tics_needed = static_cast< unsigned int > (
      (static_cast< double > (_count) / _expected_count) * 50.0);
  do
  {
    draw_tic ();
  } while (++_tic < tics_needed);
  _next_tic_count
      = static_cast< unsigned long > ((_tic / 50.0) * _expected_count);

  if (_count == _expected_count)
  {
    if (_tic < 51)
    {
      draw_tic ();
    }
    m_os << std::endl;
  }
}

void graph::progress_display::draw_tic ()
{
  m_os_temp.append (" ");
  refresh_tic ();
}

void graph::progress_display::refresh_tic ()
{
  m_os << "\r" << m_s3 << green << m_os_temp << defbg << " " << blue
       << calc_len (_count) << defbg << std::flush;
}
