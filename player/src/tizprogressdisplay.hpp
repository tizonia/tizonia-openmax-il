/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @brief  Track progress display.
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
        if (_count < _expected_count)
        {
          if ((_count += increment) >= _next_tic_count)
          {
            display_tic ();
          }
          else
          {
            update_time ();
          }
        }
        return _count;
      }

      unsigned long operator++ ()
      {
        return operator+= (1);
      }

      unsigned long count () const
      {
        return _count;
      }

      unsigned long expected_count () const
      {
        return _expected_count;
      }

    private:
      std::string calc_len (unsigned long count);

      void display_tic ();

      void update_time ();

    private:
      std::ostream &m_os;      // may not be present in all imps
      const std::string m_s1;  // string is more general, safer than
      const std::string m_s2;  //  const char *, and efficiency or size are
      const std::string m_s3;  //  not issues
      std::string m_os_temp;

      unsigned long _count;
      unsigned long _expected_count;
      unsigned long _next_tic_count;
      unsigned int _tic;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZPROGRESSDISPLAY_HPP
