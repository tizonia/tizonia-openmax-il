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
 * @file   tizprogressdisplay.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Track progress display (based on boost::progress_display).
 *
 */

#ifndef TIZPROGRESSDISPLAY_HPP
#define TIZPROGRESSDISPLAY_HPP

#include <iostream>
#include <cmath>

#include <boost/noncopyable.hpp>

namespace tiz
{
  namespace graph
  {
    enum default_color
    {
      FG_MAGENTA = 36,
      FG_LIGHT_GREY = 37,
      BG_RED = 41,
      BG_CYAN = 46
    };

    class ansi_color_sequence
    {
    public:
      explicit ansi_color_sequence (const default_color color);

      friend std::ostream &operator<< (std::ostream &os,
                                       const ansi_color_sequence &mod)
      {
        return os << "\033[" << mod.m_code << "m";
      }

    private:
      std::string m_code;
    };

    class progress_display : private boost::noncopyable
    {
    public:
      explicit progress_display (unsigned long expected_count,
                                 std::ostream &os = std::cout,
                                 const std::string &s1
                                 = "  \n",  // leading strings
                                 const std::string &s2 = "0s ",
                                 const std::string &s3 = "   ");
      ~progress_display();

      void restart(unsigned long expected_count);

      unsigned long operator+= (unsigned long increment)
      //  Effects: Display appropriate progress tic if needed.
      //  Postconditions: count()== original count() + increment
      //  Returns: count().
      {
        if (m_count < m_expected_count)
        {
          if ((m_count += increment) >= m_next_tic_count)
          {
            add_tic ();
          }
          else
          {
            refresh_tic ();
          }
        }
        return m_count;
      }

      unsigned long operator++ ()
      {
        return operator+= (1);
      }

      unsigned long count () const;

      unsigned long expected_count () const;

    private:
      std::string calc_len (unsigned long count);

      void add_tic ();

      void draw_tic ();

      void refresh_tic ();

    private:
      std::ostream &m_os;      // may not be present in all imps
      const std::string m_s1;  // string is more general, safer than
      const std::string m_s2;  //  const char *, and efficiency or size are
      const std::string m_s3;  //  not issues
      std::string m_os_temp;

      unsigned long m_count;
      unsigned long m_expected_count;
      unsigned long m_next_tic_count;
      unsigned int m_tic;

      ansi_color_sequence m_pctg_bar_color;
      ansi_color_sequence m_pctg_digits_color;
      ansi_color_sequence m_elapsed_time_color;
      ansi_color_sequence m_progress_bar_color;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZPROGRESSDISPLAY_HPP
