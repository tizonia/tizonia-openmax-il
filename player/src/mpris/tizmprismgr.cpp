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
 * @file   tizmprismgr.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  MPRIS interface manager - implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include <dbus-c++/dbus.h>

#include <OMX_Component.h>

#include <tizplatform.h>
#include <tizmacros.h>

#include "tizmprisif.hpp"
#include "tizmprismgr.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.control.mprismgr"
#endif

#define TIZ_MPRISMGR_QUEUE_MAX_ITEMS 30

namespace control = tiz::control;

namespace
{
  // Bus name
  const char *TIZONIA_MPRIS_BUS_NAME = "org.mpris.MediaPlayer2.tizonia";

  tiz::control::mpris_mediaplayer2_player_props_t * gp_player_props = NULL;

  std::string get_unique_bus_name ()
  {
    std::string bus_name (TIZONIA_MPRIS_BUS_NAME);
    // Append the process id to make a unique bus name
    bus_name.append (".pid-");
    bus_name.append (boost::lexical_cast< std::string >(getpid ()));
    return bus_name;
  }

  void player_props_pipe_handler (const void *p_arg, void *p_buffer,
                                  unsigned int nbyte)
  {
    tiz::control::mprisif * p_mif = static_cast< tiz::control::mprisif * >(const_cast<void *>(p_arg));
    if  (p_mif && gp_player_props)
      {
        p_mif->UpdatePlayerProps (*gp_player_props);
      }
  }

}

void *control::thread_func (void *p_arg)
{
  mprismgr *p_mgr = static_cast< mprismgr * >(p_arg);
  void *p_data = NULL;
  bool done = false;

  assert (p_mgr);

  (void)tiz_thread_setname (&(p_mgr->thread_), (char *)"mprismgr");
  tiz_check_omx_ret_null (tiz_sem_post (&(p_mgr->sem_)));

  while (!done)
  {
    TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS thread receiving...");
    tiz_check_omx_ret_null (tiz_queue_receive (p_mgr->p_queue_, &p_data));

    assert (p_data);

    cmd *p_cmd = static_cast< cmd * >(p_data);
    done = mprismgr::dispatch_cmd (p_mgr, p_cmd);

    delete p_cmd;
  }

  tiz_check_omx_ret_null (tiz_sem_post (&(p_mgr->sem_)));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS interface thread exiting...");

  return NULL;
}

//
// mprismgr
//
control::mprismgr::mprismgr (const mpris_mediaplayer2_props_t &props,
                             const mpris_mediaplayer2_player_props_t &player_props,
                             const mpris_callbacks_t &cbacks,
                             playback_events_t &playback_events)
  : props_ (props),
    player_props_ (player_props),
    cbacks_ (cbacks),
    p_dispatcher_ (NULL),
    p_player_props_pipe_ (NULL),
    p_dbus_timeout_ (NULL),
    p_dbus_connection_ (NULL),
    playback_connections_ (),
    thread_ (),
    mutex_ (),
    sem_ (),
    p_queue_ (NULL)
{
  gp_player_props = &player_props_;
  connect_slots (playback_events);
}

control::mprismgr::~mprismgr ()
{
  gp_player_props = NULL;
  delete p_dbus_timeout_;
  p_dbus_timeout_ = NULL;
  // NOTE: We need to leak this object. Its deletion produces a crash in
  // dbus-c++
  //
  // delete p_dbus_connection_;
}

OMX_ERRORTYPE
control::mprismgr::init ()
{
  assert (!p_queue_ && !p_dispatcher_);

  // Init command queue infrastructure
  tiz_check_omx_ret_oom (init_cmd_queue ());

  // Create the manager's thread
  tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_ret_oom (
      tiz_thread_create (&thread_, 0, 0, thread_func, this));
  tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));

  // Let's wait until the manager's thread is ready to receive requests
  tiz_check_omx_ret_oom (tiz_sem_wait (&sem_));

  // Create the DBus dispatcher
  p_dispatcher_ = new Tiz::DBus::BusDispatcher ();
  tiz_check_null_ret_oom (p_dispatcher_);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
control::mprismgr::start ()
{
  return post_cmd (new control::cmd (control::cmd::ETIZMprisMgrCmdStart));
}

OMX_ERRORTYPE
control::mprismgr::stop ()
{
  // Before sending the command to stop the thread, we have to make the DBUS
  // dispatcher leave its event loop.
  assert (p_dispatcher_);
  p_dispatcher_->leave ();
  return post_cmd (new control::cmd (control::cmd::ETIZMprisMgrCmdStop));
}

void control::mprismgr::deinit ()
{
  delete p_dispatcher_;
  p_dispatcher_ = NULL;
  static_cast< void >(tiz_sem_wait (&sem_));
  void *p_result = NULL;
  static_cast< void >(tiz_thread_join (&thread_, &p_result));
  deinit_cmd_queue ();
}

void control::mprismgr::playback_status_changed (const playback_status_t status)
{
  if (p_player_props_pipe_)
    {
      if (control::Playing == status)
        {
          player_props_.playback_status_ = "Playing";
        }
      else if (control::Paused == status)
        {
          player_props_.playback_status_ = "Paused";
        }
      else if (control::Stopped == status)
        {
          player_props_.playback_status_ = "Stopped";
        }
      p_player_props_pipe_->write(&player_props_, sizeof (player_props_));
    }
}

void control::mprismgr::loop_status_changed (const loop_status_t status)
{
  // NOT IMPLEMENTED
}

void control::mprismgr::metadata_changed (const track_metadata_map_t &metadata)
{
  // TODO
  //   player_props_.metadata_ = metadata;
  //   p_player_props_pipe_->write(&player_props_, sizeof (player_props_));
}

void control::mprismgr::volume_changed (const double volume)
{
  player_props_.volume_ = volume;
  p_player_props_pipe_->write(&player_props_, sizeof (player_props_));
}

OMX_ERRORTYPE
control::mprismgr::init_cmd_queue ()
{
  tiz_check_omx_ret_oom (tiz_mutex_init (&mutex_));
  tiz_check_omx_ret_oom (tiz_sem_init (&sem_, 0));
  tiz_check_omx_ret_oom (tiz_queue_init (&p_queue_, TIZ_MPRISMGR_QUEUE_MAX_ITEMS));
  return OMX_ErrorNone;
}

void control::mprismgr::deinit_cmd_queue ()
{
  tiz_mutex_destroy (&mutex_);
  tiz_sem_destroy (&sem_);
  tiz_queue_destroy (p_queue_);
}

OMX_ERRORTYPE
control::mprismgr::post_cmd (control::cmd *p_cmd)
{
  assert (p_cmd);
  assert (p_queue_);

  tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_ret_oom (tiz_queue_send (p_queue_, p_cmd));
  tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));

  return OMX_ErrorNone;
}

bool control::mprismgr::dispatch_cmd (control::mprismgr *p_mgr,
                                      const control::cmd *p_cmd)
{
  bool terminated = false;
  assert (p_mgr);
  assert (p_cmd);

  if (p_mgr->p_dispatcher_)
  {
    if (p_cmd->is_start ())
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS processing START cmd...");
      Tiz::DBus::default_dispatcher = p_mgr->p_dispatcher_;
      p_mgr->p_dbus_timeout_
          = new Tiz::DBus::DefaultTimeout (100, false, p_mgr->p_dispatcher_);
      p_mgr->p_dbus_connection_
          = new Tiz::DBus::Connection (Tiz::DBus::Connection::SessionBus ());
      p_mgr->p_dbus_connection_->request_name (get_unique_bus_name ().c_str ());
      mprisif mif (*(p_mgr->p_dbus_connection_), p_mgr->props_,
                   p_mgr->player_props_, p_mgr->cbacks_);
      p_mgr->p_player_props_pipe_
          = p_mgr->p_dispatcher_->add_pipe (player_props_pipe_handler, &mif);
      p_mgr->p_dispatcher_->enter ();
      TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS dispatcher done...");
    }
    else if (p_cmd->is_stop ())
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS processing STOP cmd...");
      p_mgr->disconnect_slots ();
      p_mgr->p_dispatcher_->del_pipe (p_mgr->p_player_props_pipe_);
      p_mgr->p_player_props_pipe_ = NULL;
      terminated = true;
      TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS interface terminating...");
    }
  }
  return terminated;
}

void control::mprismgr::connect_slots (
    playback_events_t &playback_events)
{
  playback_connections_.playback_ = playback_events.playback_.connect (
      boost::bind (&tiz::control::mprismgr::playback_status_changed, this, _1));
  playback_connections_.loop_ = playback_events.loop_.connect (
      boost::bind (&tiz::control::mprismgr::loop_status_changed, this, _1));
  playback_connections_.metadata_ = playback_events.metadata_.connect (
      boost::bind (&tiz::control::mprismgr::metadata_changed, this, _1));
  playback_connections_.volume_ = playback_events.volume_.connect (
      boost::bind (&tiz::control::mprismgr::volume_changed, this, _1));
}

void control::mprismgr::disconnect_slots ()
{
  playback_connections_.playback_.disconnect ();
  playback_connections_.loop_.disconnect ();
  playback_connections_.metadata_.disconnect ();
  playback_connections_.volume_.disconnect ();
}
