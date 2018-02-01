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
 * @file   tizcastworker.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia cast daemon worker thread
 *
 *
 */

#ifndef TIZCASTWORKER_HPP
#define TIZCASTWORKER_HPP

#include <string>

#include <boost/function.hpp>

#include <OMX_Core.h>
#include <tizplatform.h>

#include <tizchromecastctx_c.h>

namespace tiz
{
  namespace cast
  {

    // Forward declarations
    void *thread_func (void *p_arg);
    class cmd;
    class vector;
    class ops;

    /**
     *  @class worker
     *  @brief The cast daemon worker thread class.
     *
     *  A cast manager instantiates a thread, an event loop and an associated
     *  command queue, to communicate with Chromecast devices and cast audio to
     *  them.
     */
    class worker
    {

      friend void *thread_func (void *);
      friend class ops;

    public:
      worker (const tiz_chromecast_ctx_t * p_cc_ctx,
           const std::string &name_or_ip, cast_status_cback_t cast_cb,
           media_status_cback_t media_cb,
           termination_callback_t termination_cb);
      virtual ~worker ();

      /**
       * Initialise the manager thread.
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

      /**
       * Start processing the play list from the beginning.
       *
       * @pre init() has been called on this manager.

       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE connect ();

      /**
       * Halt processing of the playlist.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE disconnect ();

      /**
       * Process the next item in the playlist.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE load_url (const std::string &url,
                              const std::string &mime_type,
                              const std::string &title,
                              const std::string &album_art);

      /**
       * Process the previous item in the playlist.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE play ();

      /**
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE stop ();

      /**
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE pause ();

      /**
       * Set the volume level (0-100).
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE volume_set (int volume);

      /**
       * Increments or decrements the volume by steps.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE volume_up ();

      /**
       * Changes the volume to the specified value. 1.0 is maximum volume and
       * 0.0 means mute.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE volume_down ();

      /**
       * Mute/unmute toggle.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE mute ();

      /**
       * Pause the processing of the current item in the playlist.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE unmute ();

      std::string get_name_or_ip ()
      {
        return name_or_ip_;
      }

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

      OMX_ERRORTYPE init_cmd_queue ();
      void deinit_cmd_queue ();
      OMX_ERRORTYPE post_cmd (cmd *p_cmd);

      static bool dispatch_cmd (mgr *p_mgr, const cmd *p_cmd);

    private:
      const tiz_chromecast_ctx_t * p_cc_ctx_;
      std::string name_or_ip_;
      cast_status_cback_t cast_cb_;
      media_status_cback_t media_cb_;
      termination_callback_t termination_cb_;
      tiz_thread_t thread_;
      tiz_mutex_t mutex_;
      tiz_sem_t sem_;
      tiz_queue_t *p_queue_;
    };

    typedef boost::shared_ptr< worker > worker_ptr_t;

  }  // namespace cast
}  // namespace tiz

#endif  // TIZCASTWORKER_HPP
