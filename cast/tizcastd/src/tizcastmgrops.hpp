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

#include <string>
#include <boost/function.hpp>

#include <OMX_Core.h>

#include <tizchromecast_c.h>

#define GMGR_OPS_RECORD_ERROR(err, str)                                     \
  do                                                                        \
  {                                                                         \
    error_msg_.assign (str);                                                \
    error_code_ = err;                                                      \
    TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : %s", tiz_err_to_str (error_code_), \
             error_msg_.c_str ());                                          \
  } while (0)

#define GMGR_OPS_BAIL_IF_ERROR(ptr, exp, str) \
  do                                          \
  {                                           \
    if (ptr)                                  \
    {                                         \
      OMX_ERRORTYPE rc_ = OMX_ErrorNone;      \
      if (OMX_ErrorNone != (rc_ = (exp)))     \
      {                                       \
        GMGR_OPS_RECORD_ERROR (rc_, str);     \
      }                                       \
    }                                         \
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
      virtual void do_connect ();
      virtual void do_disconnect ();
      virtual void do_load_url ();
      virtual void do_play ();
      virtual void do_stop ();
      virtual void do_pause ();
      virtual void do_volume_up ();
      virtual void do_volume_down ();
      virtual void do_mute ();
      virtual void do_unmute ();
      virtual void do_report_fatal_error (const OMX_ERRORTYPE error,
                                          const std::string &msg);
      virtual bool is_fatal_error (const OMX_ERRORTYPE error,
                                   const std::string &msg);

      OMX_ERRORTYPE internal_error () const;
      std::string internal_error_msg () const;

    protected:
      virtual tizcast_ptr_t get_cast (const std::string &uri);

    protected:
      mgr *p_mgr_;              // Not owned
//       termination_callback_t termination_cback_;
      OMX_ERRORTYPE error_code_;
      std::string error_msg_;
      tiz_chromecast_t *p_cc_;
    };
  }  // namespace castmgr
}  // namespace tiz

#endif  // TIZCASTMGROPS_HPP
