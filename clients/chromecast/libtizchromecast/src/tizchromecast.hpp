/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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
 * @file   tizchromecast.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Chromecast client library
 *
 *
 */

#ifndef TIZCHROMECAST_HPP
#define TIZCHROMECAST_HPP

// ============================================================================
// Enable support for boost::function
// See http://stackoverflow.com/a/18648366/3962537
// ----------------------------------------------------------------------------
#include <boost/function.hpp>
#include <boost/function_types/components.hpp>
// ----------------------------------------------------------------------------
namespace boost
{
  namespace python
  {
    namespace detail
    {
      // ----------------------------------------------------------------------------
      // get_signature overloads must be declared before including
      // boost/python.hpp.  The declaration must be visible at the
      // point of definition of various Boost.Python templates during
      // the first phase of two phase lookup.  Boost.Python invokes the
      // get_signature function via qualified-id, thus ADL is disabled.
      // ----------------------------------------------------------------------------
      /// @brief Get the signature of a boost::function.
      template < typename Signature >
      inline typename boost::function_types::components< Signature >::type
          get_signature (boost::function< Signature > &, void * = 0)
      {
        return typename boost::function_types::components< Signature >::type ();
      }
      // ----------------------------------------------------------------------------
    }
  }
}  // namespace boost::python::detail
   // ============================================================================

#include <boost/python.hpp>

#include <string>

#include "tizchromecasttypes.h"

class tizchromecastctx;

class tizchromecast
{
public:
  tizchromecast (const tizchromecastctx &cc_ctx, const std::string &name_or_ip,
                 const tiz_chromecast_callbacks_t *ap_cbacks,
                 void *ap_user_data);
  ~tizchromecast ();

  tiz_chromecast_error_t init ();
  tiz_chromecast_error_t start ();
  void stop ();
  void deinit ();

  tiz_chromecast_error_t poll_socket (int a_poll_time_ms);

  tiz_chromecast_error_t media_load (const std::string &url,
                                     const std::string &content_type,
                                     const std::string &title,
                                     const std::string &album_art);
  tiz_chromecast_error_t media_play ();
  tiz_chromecast_error_t media_stop ();
  tiz_chromecast_error_t media_pause ();
  tiz_chromecast_error_t media_volume (int volume);
  tiz_chromecast_error_t media_volume_up ();
  tiz_chromecast_error_t media_volume_down ();
  tiz_chromecast_error_t media_mute ();
  tiz_chromecast_error_t media_unmute ();

  void new_cast_status (const std::string &, const float &);
  void new_media_status (const std::string &, const int &);

private:
  const tizchromecastctx &cc_ctx_;
  std::string name_or_ip_;
  std::string url_;
  std::string content_type_;
  std::string title_;
  tiz_chromecast_callbacks_t cbacks_;
  void *p_user_data_;
};

#endif  // TIZCHROMECAST_HPP
