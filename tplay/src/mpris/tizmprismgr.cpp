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

#include <OMX_Component.h>

#include <tizplatform.h>
#include <tizmacros.h>

#include "tizmprisif.hpp"
#include "tizmprismgr.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.control.mprismgr"
#endif

namespace control = tiz::control;

namespace
{
  // Bus name
  const char *TPLAY_MPRIS_BUS_NAME = "org.mpris.MediaPlayer2.tplay";

  std::string get_unique_bus_name ()
  {
    std::string bus_name (TPLAY_MPRIS_BUS_NAME);
    // Append the process id to make a unique bus name
//     bus_name.append (".");
//     bus_name.append (boost::lexical_cast< std::string >(getpid ()));
    return bus_name;
  }

  void player_props_pipe_handler (const void *p_arg, void *p_buffer,
                                  unsigned int nbyte)
  {
    tiz::control::mprisif * p_mif = static_cast< tiz::control::mprisif * >(const_cast<void *>(p_arg));
    tiz::control::mpris_mediaplayer2_player_props_t *p_props
      = static_cast< tiz::control::mpris_mediaplayer2_player_props_t * >(p_buffer);
    assert (NULL != p_mif);
    assert (NULL != p_props);
    p_mif->UpdatePlayerProps (*p_props);
  }

}

void *control::thread_func (void *p_arg)
{
  mprismgr *p_mgr = static_cast< mprismgr * >(p_arg);
  void *p_data = NULL;
  bool done = false;

  assert (NULL != p_mgr);

  (void)tiz_thread_setname (&(p_mgr->thread_), (char *)"tizmprismgr");
  tiz_check_omx_err_ret_null (tiz_sem_post (&(p_mgr->sem_)));

  while (!done)
  {
    TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS thread receiving...");
    tiz_check_omx_err_ret_null (tiz_queue_receive (p_mgr->p_queue_, &p_data));

    assert (NULL != p_data);

    cmd *p_cmd = static_cast< cmd * >(p_data);
    done = mprismgr::dispatch_cmd (p_mgr, p_cmd);

    delete p_cmd;
  }

  tiz_check_omx_err_ret_null (tiz_sem_post (&(p_mgr->sem_)));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS interface thread exiting...");

  return NULL;
}

//
// mprismgr
//
control::mprismgr::mprismgr (const mpris_mediaplayer2_props_t &props,
                             const mpris_mediaplayer2_player_props_t &player_props,
                             const mpris_callbacks_t &cbacks)
  : props_ (props),
    player_props_ (player_props),
    cbacks_ (cbacks),
    dispatcher_ (),
    p_player_props_pipe_ (NULL),
    p_connection_ (NULL),
    thread_ (),
    mutex_ (),
    sem_ (),
    p_queue_ (NULL)
{
}

control::mprismgr::~mprismgr ()
{
  // NOTE: We need to leak this object. Its deletion produces a crash in
  // dbus-c++
  //
  // delete p_connection_;
}

OMX_ERRORTYPE
control::mprismgr::init ()
{
  // Init command queue infrastructure
  tiz_check_omx_err_ret_oom (init_cmd_queue ());

  // Create the manager's thread
  tiz_check_omx_err_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_err_ret_oom (
      tiz_thread_create (&thread_, 0, 0, thread_func, this));
  tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&mutex_));

  // Let's wait until the manager's thread is ready to receive requests
  tiz_check_omx_err_ret_oom (tiz_sem_wait (&sem_));

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
control::mprismgr::start ()
{
  return post_cmd (new control::cmd (control::cmd::ETIZMprisMgrCmdStart));
}

void
control::mprismgr::update_player_properties (
    const mpris_mediaplayer2_player_props_t &player_props)
{
  if (NULL != p_player_props_pipe_)
    {
      // TODO: Serialise the properties structure
      p_player_props_pipe_->write('\0', 1);
    }
}

OMX_ERRORTYPE
control::mprismgr::stop ()
{
  // Before sending the command to stop the thread, we have to make the DBUS
  // dispatcher leave its event loop.
  dispatcher_.leave ();
  return post_cmd (new control::cmd (control::cmd::ETIZMprisMgrCmdStop));
}

void control::mprismgr::deinit ()
{
  static_cast< void >(tiz_sem_wait (&sem_));
  void *p_result = NULL;
  static_cast< void >(tiz_thread_join (&thread_, &p_result));
  deinit_cmd_queue ();
}

OMX_ERRORTYPE
control::mprismgr::init_cmd_queue ()
{
  tiz_check_omx_err_ret_oom (tiz_mutex_init (&mutex_));
  tiz_check_omx_err_ret_oom (tiz_sem_init (&sem_, 0));
  tiz_check_omx_err_ret_oom (tiz_queue_init (&p_queue_, 10));
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
  assert (NULL != p_cmd);
  assert (NULL != p_queue_);

  tiz_check_omx_err_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_err_ret_oom (tiz_queue_send (p_queue_, p_cmd));
  tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&mutex_));

  return OMX_ErrorNone;
}

bool control::mprismgr::dispatch_cmd (control::mprismgr *p_mgr,
                                      const control::cmd *p_cmd)
{
  bool terminated = false;
  assert (NULL != p_mgr);
  assert (NULL != p_cmd);

  DBus::BusDispatcher &dispatcher = p_mgr->dispatcher_;
  if (p_cmd->is_start ())
  {
    TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS processing START cmd...");
    DBus::default_dispatcher = &(dispatcher);
    p_mgr->p_connection_ =
      new DBus::Connection(DBus::Connection::SessionBus());
    p_mgr->p_connection_->request_name (get_unique_bus_name ().c_str ());
    mprisif mif (*(p_mgr->p_connection_), p_mgr->props_, p_mgr->player_props_, p_mgr->cbacks_);
    p_mgr->p_player_props_pipe_ = dispatcher.add_pipe (player_props_pipe_handler, &mif);
    dispatcher.enter ();
    TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS dispatcher done...");
  }
  else if (p_cmd->is_stop ())
  {
    TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS processing STOP cmd...");
    dispatcher.del_pipe (p_mgr->p_player_props_pipe_);
    p_mgr->p_player_props_pipe_ = NULL;
    terminated = true;
    TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS interface terminating...");
  }
  return terminated;
}
