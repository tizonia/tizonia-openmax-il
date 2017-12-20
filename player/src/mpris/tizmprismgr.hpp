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
#include <boost/signals2/connection.hpp>

#include <tizplatform.h>
#include <OMX_Core.h>

#include <tizgraphtypes.hpp>

#include "tizplaybackevents.hpp"

#include "tizmprisprops.hpp"
#include "tizmpriscbacks.hpp"

namespace Tiz
{
namespace DBus
{
  class BusDispatcher;
  class DefaultTimeout;
  class Connection;
  class Pipe;
}
}

namespace tiz
{
  namespace control
  {

    // Forward declarations
    void *thread_func (void *p_arg);

    struct cmd
    {
    public:
      enum mpris_mgr_cmd
      {
        ETIZMprisMgrCmdStart = 0,
        ETIZMprisMgrCmdUpdateProps,
        ETIZMprisMgrCmdUpdatePlayerProps,
        ETIZMprisMgrCmdStop
      };
      typedef enum mpris_mgr_cmd mpris_mgr_cmd_t;

    public:
      explicit cmd (mpris_mgr_cmd_t mgr_cmd) : mgr_cmd_ (mgr_cmd)
      {
      }

      bool is_start () const
      {
        return (mgr_cmd_ == ETIZMprisMgrCmdStart);
      }

      bool is_stop () const
      {
        return (mgr_cmd_ == ETIZMprisMgrCmdStop);
      }

    private:
      const mpris_mgr_cmd_t mgr_cmd_;
    };

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
      mprismgr (const mpris_mediaplayer2_props_t &props,
                const mpris_mediaplayer2_player_props_t &player_props,
                const mpris_callbacks_t &cbacks,
                playback_events_t &playback_events);
      virtual ~mprismgr ();

      /**
       * Initialise the MPRIS' DBUS dispatcher thread.
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
       * Start processing DBUS messages.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE start ();

      /**
       * Halt the processing of the DBUS messages.
       *
       * @pre start() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE stop ();

      /**
       * Release all the DBUS resources and destroy the MPRIS thread.
       *
       * @pre init() has been called on this manager.
       *
       * @post Only init() can be called at this point.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      void deinit ();

    protected:
      void playback_status_changed (const playback_status_t status);
      void loop_status_changed (const loop_status_t status);
      void metadata_changed (const track_metadata_map_t &metadata);
      void volume_changed (const double volume);

    protected:
      mpris_mediaplayer2_props_t props_;
      mpris_mediaplayer2_player_props_t player_props_;
      const mpris_callbacks_t cbacks_;
      Tiz::DBus::BusDispatcher *p_dispatcher_;
      Tiz::DBus::Pipe *p_player_props_pipe_; // Not owned
      Tiz::DBus::DefaultTimeout *p_dbus_timeout_;
      Tiz::DBus::Connection *p_dbus_connection_;

    private:
      OMX_ERRORTYPE init_cmd_queue ();
      void deinit_cmd_queue ();
      OMX_ERRORTYPE post_cmd (cmd *p_cmd);
      static bool dispatch_cmd (mprismgr *p_mgr, const cmd *p_cmd);
      void connect_slots (playback_events_t &playback_events);
      void disconnect_slots ();

    private:
      struct playback_connections
      {
        playback_connections ()
          :
          playback_(),
          loop_(),
          metadata_(),
          volume_ ()
        {}
      public:
        boost::signals2::connection playback_;
        boost::signals2::connection loop_;
        boost::signals2::connection metadata_;
        boost::signals2::connection volume_;
      };
      typedef struct playback_connections playback_connections_t;
    private:
      playback_connections_t playback_connections_;

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
