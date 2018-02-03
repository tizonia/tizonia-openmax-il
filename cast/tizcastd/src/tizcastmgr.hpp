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
 * @file   tizcastmgr.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia cast manager thread
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

namespace tiz
{
  namespace cast
  {

    // Forward declarations
    class cmd;
    class vector;
    class ops;

    /**
     *  @class mgr
     *  @brief The cast manager class.
     *
     *  A cast manager instantiates a thread, an event loop and an associated
     *  command queue, to communicate with Chromecast devices and cast audio to
     *  them.
     */
    class mgr
    {

      friend class ops;

    public:
      mgr (const tiz_chromecast_ctx_t * p_cc_ctx,
           cast_status_cback_t cast_cb,
           media_status_cback_t media_cb,
           termination_callback_t termination_cb);
      virtual ~mgr ();

      /**
       * Initialise the cast manager thread.
       *
       * @pre This method must be called only once, before any call is made to
       * the other APIs.
       *
       * @post The cast manager thread is ready to process requests.
       *
       * @return OMX_ErrorNone if initialisation was
       * successful. OMX_ErrorInsuficientResources otherwise.
       */
      OMX_ERRORTYPE init ();

      /**
       * Destroy the manager thread and release all resources.
       *
       * @pre stop() has been called on this manager.
       *
       * @post Only init() can be called at this point.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      void deinit ();

      std::string get_name_or_ip ()
      {
        return name_or_ip_;
      }

      bool dispatch_cmd (const cmd *p_cmd);

    private:
      /**
       * Start processing the play list from the beginning.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE start_fsm ();

      /**
       * Exit the manager thread.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE stop_fsm ();

      OMX_ERRORTYPE cast_status_received ();

    private:
      ops *p_ops_;
      fsm fsm_;
      const tiz_chromecast_ctx_t * p_cc_ctx_;
      std::string name_or_ip_;
      cast_status_cback_t cast_cb_;
      media_status_cback_t media_cb_;
      termination_callback_t termination_cb_;
    };

    typedef boost::shared_ptr< mgr > mgr_ptr_t;

  }  // namespace cast
}  // namespace tiz

#endif  // TIZCASTMGR_HPP
