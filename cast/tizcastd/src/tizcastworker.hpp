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
 * @file   tizcastworker.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia's Chromecast daemon worker thread
 *
 *
 */

#ifndef TIZCASTWORKER_HPP
#define TIZCASTWORKER_HPP

#include <map>
#include <string>

#include <boost/function.hpp>

#include <OMX_Core.h>
#include <tizplatform.h>

#include <tizchromecastctx_c.h>

#include "tizcastmgrtypes.hpp"

namespace tiz
{
  namespace cast
  {

    // Forward declarations
    void *thread_func (void *p_arg);
    class mgr;
    class cmd;
    class vector;

    /**
     *  @class worker
     *  @brief The Chromecast daemon worker thread class.
     *
     *  A worker class that instantiates its own thread, event loop and
     *  associated command queue, to communicate with Chromecast devices and
     *  cast audio to them.
     */
    class worker
    {

      friend void *thread_func (void *);
      friend class ops;

    public:
      worker (cast_status_cback_t cast_cb, media_status_cback_t media_cb,
              error_status_callback_t error_cb);
      virtual ~worker ();

      /**
       * Initialise the worker thread.
       *
       * @pre This method must be called only once, before any call is made to
       * the other APIs.
       *
       * @post The cast worker thread is ready to process requests.
       *
       * @return OMX_ErrorNone if initialisation was
       * successful. OMX_ErrorInsuficientResources otherwise.
       */
      OMX_ERRORTYPE init ();

      /**
       * Destroy the worker thread and release all resources.
       *
       * @pre stop() has been called on this worker.
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
       * @pre init() has been called on this worker.

       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE connect (const std::vector< uint8_t > &uuid,
                             const std::string &name_or_ip);

      /**
       * Halt processing of the playlist.
       *
       * @pre init() has been called on this worker.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE disconnect (const std::vector< uint8_t > &uuid);

      /**
       * Process the next item in the playlist.
       *
       * @pre init() has been called on this worker.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE load_url (const std::vector< uint8_t > &uuid,
                              const std::string &url,
                              const std::string &mime_type,
                              const std::string &title,
                              const std::string &album_art);

      /**
       * Process the previous item in the playlist.
       *
       * @pre init() has been called on this worker.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE play (const std::vector< uint8_t > &uuid);

      /**
       *
       * @pre init() has been called on this worker.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE stop (const std::vector< uint8_t > &uuid);

      /**
       *
       * @pre init() has been called on this worker.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE pause (const std::vector< uint8_t > &uuid);

      /**
       * Set the volume level (0-100).
       *
       * @pre init() has been called on this worker.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE volume_set (const std::vector< uint8_t > &uuid, int volume);

      /**
       * Increments or decrements the volume by steps.
       *
       * @pre init() has been called on this worker.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE volume_up (const std::vector< uint8_t > &uuid);

      /**
       * Changes the volume to the specified value. 1.0 is maximum volume and
       * 0.0 means mute.
       *
       * @pre init() has been called on this worker.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE volume_down (const std::vector< uint8_t > &uuid);

      /**
       * Mute/unmute toggle.
       *
       * @pre init() has been called on this worker.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE mute (const std::vector< uint8_t > &uuid);

      /**
       * Pause the processing of the current item in the playlist.
       *
       * @pre init() has been called on this worker.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE unmute (const std::vector< uint8_t > &uuid);

    private:
      OMX_ERRORTYPE cast_status_received ();

      OMX_ERRORTYPE create_cmd_queue ();

      void destroy_cmd_queue ();

      OMX_ERRORTYPE post_cmd (cmd *p_cmd);

      void remove_client (const uuid_t &uuid, tiz::cast::mgr *p_mgr);

      void purge_old_clients (const std::string &device_name_or_ip);

      static bool dispatch_cmd (worker *p_worker, const cmd *p_cmd);

      static void poll_mgrs (worker *p_worker, const cmd *p_cmd);

    private:
      struct client_info
      {
        client_info () : uuid_ (), p_cast_mgr_ (NULL)
        {
        }

        client_info (std::vector< unsigned char > client_uuid,
                     tiz::cast::mgr *p_cast_mgr)
          : uuid_ (client_uuid), p_cast_mgr_ (p_cast_mgr)
        {
        }

        bool operator< (const client_info &rhs) const
        {
          return (uuid_ < rhs.uuid_);
        }

        bool operator== (const client_info &rhs) const
        {
          return (uuid_ == rhs.uuid_);
        }

        // Data members
        uuid_t uuid_;
        tiz::cast::mgr *p_cast_mgr_;  // Not owned
      };

    private:
      typedef std::map< uuid_t, client_info > clients_map_t;
      typedef std::pair< uuid_t, client_info > clients_pair_t;

    private:
      clients_map_t clients_;
      tiz_chromecast_ctx_t *p_cc_ctx_;
      cast_status_cback_t cast_cb_;
      media_status_cback_t media_cb_;
      error_status_callback_t error_cb_;
      tiz_thread_t thread_;
      tiz_mutex_t mutex_;
      tiz_sem_t sem_;
      tiz_queue_t *p_queue_;
    };

    typedef boost::shared_ptr< worker > worker_ptr_t;

  }  // namespace cast
}  // namespace tiz

#endif  // TIZCASTWORKER_HPP
