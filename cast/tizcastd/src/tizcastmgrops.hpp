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
 * @file   tizcastmgrops.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Cast manager operations
 *
 *
 */

#ifndef TIZCASTMGROPS_HPP
#define TIZCASTMGROPS_HPP

#include <boost/function.hpp>
#include <string>

#include <OMX_Core.h>

#include <tizchromecast_c.h>

#define CAST_MGR_OPS_RECORD_ERROR(err, str)                              \
  do                                                                     \
  {                                                                      \
    error_msg_.assign (str);                                             \
    error_code_ = err;                                                   \
    TIZ_LOG (TIZ_PRIORITY_ERROR, "[%d] : %s", err, error_msg_.c_str ()); \
  } while (0)

#define CAST_MGR_OPS_BAIL_IF_ERROR(exp, str) \
  do                                         \
  {                                          \
    int rc_ = 0;                             \
    if (0 != (rc_ = (exp)))                  \
    {                                        \
      CAST_MGR_OPS_RECORD_ERROR (rc_, str);  \
    }                                        \
  } while (0)

namespace tiz
{
  namespace castmgr
  {
    // forward decl
    class mgr;

    /**
     *  @class ops
     *  @brief The cast manager operations class.
     *
     */
    class ops
    {

    public:
      typedef boost::function< void(OMX_ERRORTYPE, std::string) >
          termination_callback_t;

    public:
      ops (mgr *p_mgr);
      virtual ~ops ();

      void deinit ();

    public:
      virtual void do_connect (const std::string &name_or_ip);
      virtual void do_disconnect ();
      virtual void do_load_url (const std::string &url,
                                const std::string &mime_type,
                                const std::string &title);
      virtual void do_play ();
      virtual void do_stop ();
      virtual void do_pause ();
      virtual void do_volume_up ();
      virtual void do_volume_down ();
      virtual void do_mute ();
      virtual void do_unmute ();
      virtual void do_report_fatal_error (const int error,
                                          const std::string &msg);
      virtual bool is_fatal_error (const int error,
                                   const std::string &msg);

      int internal_error () const;
      std::string internal_error_msg () const;

    protected:
      mgr *p_mgr_;  // Not owned
                    //       termination_callback_t termination_cback_;
      int error_code_;
      std::string error_msg_;
      tiz_chromecast_t *p_cc_;
    };
  }  // namespace castmgr
}  // namespace tiz

#endif  // TIZCASTMGROPS_HPP
