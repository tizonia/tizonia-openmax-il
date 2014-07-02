/* -*-Mode: c++; -*- */
/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   tizmprismgr.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  MPRIS interface manager
 *
 *
 */

#ifndef TIZMPRISMGR_HPP
#define TIZMPRISMGR_HPP

#include <stdint.h>
#include <string>
#include <boost/function.hpp>

#include <tizplatform.h>
#include <OMX_Core.h>

#include "tizgraphtypes.hpp"
#include "tizmprisprops.hpp"

namespace tiz
{
  namespace control
  {

    // Forward declarations
    void *thread_func (void *p_arg);
    class cmd;

    /**
     *  @class mprismgr
     *  @brief The MPRIS interface handler.
     *
     *  A graph manager uses this class to instantiate an MPRIS control
     *  interface.
     */
    class mprismgr
    {

      friend void *thread_func (void *);

    public:
      mprismgr (mpris_mediaplayer2_props_ptr_t mp2_props_ptr,
                mpris_mediaplayer2_player_props_ptr_t mp2_player_props_ptr);
      virtual ~mprismgr ();

      /**
       * Initialise MPRIS' DBUS dispatcher thread.
       *
       * @pre This method must be called only once, before any call is made to
       * the other APIs.
       *
       * @post The MPRIS thread is ready to process requests.
       *
       * @return OMX_ErrorNone if initialisation was
       * successful. OMX_ErrorInsuficientResources otherwise.
       */
      OMX_ERRORTYPE init ();

      /**
       * Destroy the MPRIS thread and release all resources.
       *
       * @pre init() has been called on this object.
       *
       * @post Only init() can be called at this point.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      void deinit ();

      /**
       * Start processing the play list from the beginning.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE start ();

      /**
       * Halts processing of the playlist.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE stop ();

    protected:
      mpris_mediaplayer2_props_ptr_t props_ptr_;
      mpris_mediaplayer2_player_props_ptr_t player_props_ptr_;

    private:
      OMX_ERRORTYPE init_cmd_queue ();
      void deinit_cmd_queue ();
      OMX_ERRORTYPE post_cmd (cmd *p_cmd);
      static bool dispatch_cmd (mgr *p_mgr, const cmd *p_cmd);

    private:
      tiz_thread_t thread_;
      tiz_mutex_t mutex_;
      tiz_sem_t sem_;
      tiz_queue_t *p_queue_;
    };

    typedef boost::shared_ptr< mprismgr > mprismgr_ptr_t;

  }  // namespace control
}  // namespace tiz

#endif  // TIZMPRISMGR_HPP
