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
#include <boost/algorithm/string/replace.hpp>
#include <boost/progress.hpp>

#include <tizplatform.h>

#include "tizprogressdisplay.hpp"

#define COLOR_THEMES "color-themes"
#define ACTIVE_THEME "active-theme"
#define C13 "C13"
#define C14 "C14"
#define C15 "C15"
#define C16 "C16"
#define RESET_COLOR "\x1B[0m"

namespace graph = tiz::graph;

graph::ansi_color_sequence::ansi_color_sequence (
    const graph::default_color color)
  : m_code ()
{
#define CASE_COLOR_(COLOR_ENUM, COLOR)                                 \
  case COLOR_ENUM:                                                     \
  {                                                                    \
    if (p_active_theme)                                                \
    {                                                                  \
      (void)strncat (color_name, COLOR, OMX_MAX_STRINGNAME_SIZE - 2);  \
      p = (char *)tiz_rcfile_get_value (COLOR_THEMES, color_name);     \
    }                                                                  \
  }                                                                    \
  break

  char color_name[OMX_MAX_STRINGNAME_SIZE];
  char *p = NULL;
  const char *p_active_theme
      = tiz_rcfile_get_value (COLOR_THEMES, ACTIVE_THEME);
  if (p_active_theme)
  {
    (void)strcpy (color_name, p_active_theme);
    (void)strcat (color_name, ".");
  }

  switch (color)
  {
    CASE_COLOR_ (FG_MAGENTA, C13);
    CASE_COLOR_ (FG_LIGHT_GREY, C14);
    CASE_COLOR_ (BG_RED, C15);
    CASE_COLOR_ (BG_CYAN, C16);
    default:
    {
      assert (0);
    };
  };

  if (p)
  {
    m_code.assign (p);
    boost::replace_all (m_code, ",", ";");
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
    m_pctg_bar_color (FG_MAGENTA), m_pctg_digits_color (FG_LIGHT_GREY), m_elapsed_time_color (BG_RED), m_progress_bar_color (BG_CYAN)
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
  m_os << m_s1 << "   "
       << m_pctg_digits_color << "0%" << RESET_COLOR << "   "
       << m_pctg_digits_color << "10" << RESET_COLOR << "   "
       << m_pctg_digits_color << "20" << RESET_COLOR << "   "
       << m_pctg_digits_color << "30" << RESET_COLOR << "   "
       << m_pctg_digits_color << "40" << RESET_COLOR << "   "
       << m_pctg_digits_color << "50" << RESET_COLOR << "   "
       << m_pctg_digits_color << "60" << RESET_COLOR << "   "
       << m_pctg_digits_color << "70" << RESET_COLOR << "   "
       << m_pctg_digits_color << "80" << RESET_COLOR << "   "
       << m_pctg_digits_color << "90" << RESET_COLOR << "   "
       << m_pctg_digits_color << "100%" << RESET_COLOR
       << "\n"
       << m_pctg_digits_color << m_s2 << RESET_COLOR
       << m_pctg_bar_color << "|----|----|----|----|----|----|----|----|----|----|" << RESET_COLOR << " "
       << m_pctg_digits_color << _total_elapsed_time << RESET_COLOR
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
  m_os << "\r" << m_s3 << m_progress_bar_color << m_os_temp << RESET_COLOR << " " << m_elapsed_time_color
       << calc_len (m_count) << RESET_COLOR << std::flush;
}
