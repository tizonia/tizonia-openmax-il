/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   tizcastmgr.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Chromecast manager class.
 *
 *
 */

#ifndef TIZCASTMGR_HPP
#define TIZCASTMGR_HPP

#include <string>

#include <boost/function.hpp>

#include <OMX_Core.h>
#include <tizplatform.h>

#include <tizchromecastctx_c.h>

#include "tizcastmgrfsm.hpp"
#include "tizcastmgrtypes.hpp"

// Forward declarations
class vector;
namespace boost
{
  class any;
}

namespace tiz
{
  namespace cast
  {

    // Forward declarations
    class cmd;
    class ops;

    /**
     *  @class mgr
     *  @brief The Chromecast manager class.
     *
     *  A class to manage the communication with a Chromecast device and cast
     *  audio to it.
     */
    class mgr
    {

      friend class ops;

    public:
      mgr (const std::string &device_name_or_ip, const uuid_t &uuid,
           const tiz_chromecast_ctx_t *p_cc_ctx, cast_status_cback_t cast_cb,
           media_status_cback_t media_cb, error_status_callback_t error_cb);
      virtual ~mgr ();

      /**
       * Initialise the cast manager.
       *
       * @pre This method must be called only once, before any call is made to
       * the other APIs.
       *
       * @return OMX_ErrorNone if initialisation was
       * successful. OMX_ErrorInsuficientResources otherwise.
       */
      OMX_ERRORTYPE init ();

      /**
       * Release all resources.
       *
       * @pre stop() has been called on this manager.
       *
       * @post Only init() can be called at this point.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      void deinit ();

      /**
       * Process a manager command.
       *
       * @return True if the manager's fsm has been terminated. Otherwise,
       * false is returned.
       */
      bool dispatch_cmd (const cmd *p_cmd);

      /**
       * If true, this manager is no longer needed and can be deinitialised and
       * destroyed.
       *
       * @return true if this manager's FSM has been terminated.
       */
      bool terminated () const;

      /**
       * Retrieve the uuid of the client associated to this manager.
       *
       * @return uuid
       */
      uuid_t uuid () const;

      /**
       * Retrieve the device name or ip address of the Chromecast associated
       * with this manager.
       *
       * @return uuid
       */
      std::string device_name_or_ip () const;

    private:
      OMX_ERRORTYPE start_fsm ();

      OMX_ERRORTYPE stop_fsm ();

      OMX_ERRORTYPE post_internal_cmd (const boost::any &any_event);

      OMX_ERRORTYPE cast_status_received ();

    private:
      ops *p_ops_;
      fsm fsm_;
      const std::string name_or_ip_;
      const uuid_t uuid_;
      const tiz_chromecast_ctx_t *p_cc_ctx_;
      cast_status_cback_t cast_cb_;
      media_status_cback_t media_cb_;
      error_status_callback_t error_cb_;
    };

    typedef boost::shared_ptr< mgr > mgr_ptr_t;

  }  // namespace cast
}  // namespace tiz

#endif  // TIZCASTMGR_HPP
