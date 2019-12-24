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

#include <tizplatform.h>

#include "tizprogressdisplay.hpp"

#define COLOR_THEME "color-theme"
#define C13 "C13"
#define C14 "C14"
#define C15 "C15"
#define C16 "C16"
#define DEF_FG_COLOR "\033[39m"
#define DEF_BG_COLOR "\033[49m"

namespace graph = tiz::graph;

graph::ansi_color_sequence::ansi_color_sequence (const graph::default_color color)
: m_code ()
{
  char *p = NULL;
  switch (color)
  {
    case FG_MAGENTA:
    {
      p = (char *)tiz_rcfile_get_value (COLOR_THEME, C13);
    }
    break;
    case FG_LIGHT_GREY:
    {
      p = (char *)tiz_rcfile_get_value (COLOR_THEME, C14);
    }
    break;
    case BG_RED:
    {
      p = (char *)tiz_rcfile_get_value (COLOR_THEME, C15);
    }
    break;
    case BG_CYAN:
    {
      p = (char *)tiz_rcfile_get_value (COLOR_THEME, C16);
    }
    break;
    default:
    {
      assert (0);
    };
  };

  if (p)
  {
    m_code.assign (p);
  }
  else
  {
    m_code = boost::lexical_cast< std::string > (color);
  }
}

graph::progress_display::progress_display (unsigned long expected_count,
                                           std::ostream &os,
                                           const std::string &s1,
                                           const std::string &s2,
                                           const std::string &s3)
  : noncopyable (), m_os (os), m_s1 (s1), m_s2 (s2), m_s3 (s3), m_os_temp (), m_count(0), m_expected_count(0), m_next_tic_count(0), m_tic(0),
    m_pctg_bar_color (FG_MAGENTA), m_digits_color (FG_LIGHT_GREY), m_time_bg_color (BG_RED), m_progress_bar_color (BG_CYAN)
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
  m_count = m_next_tic_count = m_tic = 0;
  m_expected_count = expected_count;
  m_os_temp.clear ();
  std::string _total_elapsed_time = calc_len (expected_count);
  m_os << m_s1 << m_digits_color
       << "   0%   10   20   30   40   50   60   70   80   90   100%" << DEF_FG_COLOR
       << "\n"
       << m_digits_color << m_s2 << DEF_FG_COLOR << m_pctg_bar_color
       << "|----|----|----|----|----|----|----|----|----|----| " << DEF_FG_COLOR << m_digits_color
       << _total_elapsed_time << DEF_FG_COLOR
       << std::endl  // endl implies flush, which ensures display
       << m_s3;
  if (!m_expected_count)
  {
    m_expected_count = 1;  // prevent divide by zero
  }                       // restart
}

unsigned long graph::progress_display::count () const
{
  return m_count;
}

unsigned long graph::progress_display::expected_count () const
{
  return m_expected_count;
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
      (static_cast< double > (m_count) / m_expected_count) * 50.0);
  do
  {
    draw_tic ();
  } while (++m_tic < tics_needed);
  m_next_tic_count
      = static_cast< unsigned long > ((m_tic / 50.0) * m_expected_count);

  if (m_count == m_expected_count)
  {
    if (m_tic < 51)
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
  m_os << "\r" << m_s3 << m_progress_bar_color << m_os_temp << DEF_BG_COLOR << " " << m_time_bg_color
       << calc_len (m_count) << DEF_BG_COLOR << std::flush;
}
