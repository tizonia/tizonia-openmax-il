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
 * @file   tizcastworker.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia's Chromecast daemon worker thread implementation.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <algorithm>
#include <vector>

#include <boost/foreach.hpp>

#include <OMX_Component.h>

#include <tizmacros.h>
#include <tizplatform.h>

#include "tizcastmgr.hpp"
#include "tizcastmgrcmd.hpp"
#include "tizcastworker.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.worker"
#endif

#define TIZ_CAST_WORKER_QUEUE_MAX_ITEMS 30

namespace cast = tiz::cast;

void *cast::thread_func (void *p_arg)
{
  worker *p_worker = static_cast< worker * > (p_arg);
  void *p_data = NULL;
  bool done = false;
  int poll_time_ms = 100;  // ms
  // Pre-allocated poll command
  uuid_t null_uuid;
  cast::cmd cmd (null_uuid, cast::poll_evt (poll_time_ms));

  assert (p_worker);

  (void)tiz_thread_setname (&(p_worker->thread_), (char *)"cast");
  tiz_check_omx_ret_null (tiz_sem_post (&(p_worker->sem_)));

  while (!done)
  {
    tiz_queue_timed_receive (p_worker->p_queue_, &p_data, poll_time_ms);

    // Dispatch events from the command queue
    if (p_data)
    {
      cast::cmd *p_cmd = static_cast< cast::cmd * > (p_data);
      done = cast::worker::dispatch_cmd (p_worker, p_cmd);
      delete p_cmd;
      p_data = NULL;
    }

    // This is to poll the chromecast sockets periodically
    if (!done)
    {
      cast::worker::poll_mgrs (p_worker, &cmd);
    }
  }

  tiz_check_omx_ret_null (tiz_sem_post (&(p_worker->sem_)));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Cast daemon worker thread exiting...");

  return NULL;
}

//
// worker
//
cast::worker::worker (cast_status_cback_t cast_cb,
                      media_status_cback_t media_cb,
                      termination_callback_t termination_cb)
  : p_cc_ctx_ (NULL),
    cast_cb_ (cast_cb),
    media_cb_ (media_cb),
    termination_cb_ (termination_cb),
    thread_ (),
    mutex_ (),
    sem_ (),
    p_queue_ (NULL)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
  int rc = tiz_chromecast_ctx_init (&(p_cc_ctx_));
  assert (0 == rc);
}

cast::worker::~worker ()
{
  deinit_cmd_queue ();
  BOOST_FOREACH (const clients_pair_t &client, clients_)
  {
    cast::mgr *p_mgr = client.second.p_cast_mgr_;
    p_mgr->deinit ();
    delete p_mgr;
  }
  tiz_chromecast_ctx_destroy (&(p_cc_ctx_));
}

OMX_ERRORTYPE
cast::worker::init ()
{
  // Init command queue infrastructure
  tiz_check_omx_ret_oom (init_cmd_queue ());

  // Create the manager's thread
  tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_ret_oom (tiz_thread_create (&thread_, 0, 0, thread_func, this));
  tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));

  // Let's wait until this manager's thread is ready to receive requests
  tiz_check_omx_ret_oom (tiz_sem_wait (&sem_));

  return OMX_ErrorNone;
}

void cast::worker::deinit ()
{
  cast::uuid_t null_uuid;
  post_cmd (new cast::cmd (null_uuid, cast::quit_evt ()));
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "Waiting until stopped...");
  static_cast< void > (tiz_sem_wait (&sem_));
  void *p_result = NULL;
  static_cast< void > (tiz_thread_join (&thread_, &p_result));
}

OMX_ERRORTYPE
cast::worker::connect (const std::vector< uint8_t > &uuid,
                       const std::string &name_or_ip)
{
  return post_cmd (new cast::cmd (uuid, cast::connect_evt (name_or_ip)));
}

OMX_ERRORTYPE
cast::worker::disconnect (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (uuid, cast::disconnect_evt ()));
}

OMX_ERRORTYPE
cast::worker::load_url (const std::vector< uint8_t > &uuid,
                        const std::string &url, const std::string &mime_type,
                        const std::string &title, const std::string &album_art)
{
  return post_cmd (new cast::cmd (
      uuid, cast::load_url_evt (url, mime_type, title, album_art)));
}

OMX_ERRORTYPE
cast::worker::play (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (uuid, cast::play_evt ()));
}

OMX_ERRORTYPE
cast::worker::stop (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (uuid, cast::stop_evt ()));
}

OMX_ERRORTYPE
cast::worker::pause (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (uuid, cast::pause_evt ()));
}

OMX_ERRORTYPE
cast::worker::volume_set (const std::vector< uint8_t > &uuid, int volume)
{
  return post_cmd (new cast::cmd (uuid, cast::volume_evt (volume)));
}

OMX_ERRORTYPE
cast::worker::volume_up (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (uuid, cast::volume_up_evt ()));
}

OMX_ERRORTYPE
cast::worker::volume_down (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (uuid, cast::volume_down_evt ()));
}

OMX_ERRORTYPE
cast::worker::mute (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (uuid, cast::mute_evt ()));
}

OMX_ERRORTYPE
cast::worker::unmute (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (uuid, cast::unmute_evt ()));
}

//
// Private methods
//

OMX_ERRORTYPE
cast::worker::cast_status_received ()
{
  cast::uuid_t null_uuid;
  return post_cmd (new cast::cmd (null_uuid, cast::cast_status_evt ()));
}

OMX_ERRORTYPE
cast::worker::init_cmd_queue ()
{
  tiz_check_omx_ret_oom (tiz_mutex_init (&mutex_));
  tiz_check_omx_ret_oom (tiz_sem_init (&sem_, 0));
  tiz_check_omx_ret_oom (
      tiz_queue_init (&p_queue_, TIZ_CAST_WORKER_QUEUE_MAX_ITEMS));
  return OMX_ErrorNone;
}

void cast::worker::deinit_cmd_queue ()
{
  tiz_mutex_destroy (&mutex_);
  tiz_sem_destroy (&sem_);
  tiz_queue_destroy (p_queue_);
}

OMX_ERRORTYPE
cast::worker::post_cmd (cast::cmd *p_cmd)
{
  assert (p_cmd);
  assert (p_queue_);

  tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_ret_oom (tiz_queue_send (p_queue_, p_cmd));
  tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));

  return OMX_ErrorNone;
}

bool cast::worker::dispatch_cmd (cast::worker *p_worker, const cast::cmd *p_cmd)
{
  cast::mgr *p_mgr = NULL;

  assert (p_worker);
  assert (p_cmd);

  clients_map_t &clients = p_worker->clients_;
  const uuid_t &uuid = p_cmd->uuid ();
  if (clients.count (uuid))
  {
    p_mgr = clients[uuid].p_cast_mgr_;
  }
  else
  {
    p_mgr = new tiz::cast::mgr (uuid, p_worker->p_cc_ctx_, p_worker->cast_cb_,
                                p_worker->media_cb_, p_worker->termination_cb_);

    assert (p_mgr);
    p_mgr->init ();

    std::pair< clients_map_t::iterator, bool > rv
        = clients.insert (std::make_pair (uuid, client_info (uuid, p_mgr)));

    assert (rv.second);

    char uuid_str[128];
    tiz_uuid_str (&(uuid[0]), uuid_str);
    TIZ_LOG (TIZ_PRIORITY_NOTICE,
             "Successfully registered client with uuid [%s]...", uuid_str);
  }

  if (p_mgr->dispatch_cmd (p_cmd))
  {
    // The manager has terminated
    p_mgr->deinit ();
    clients.erase (uuid);
    delete p_mgr;
    p_mgr = NULL;
  }

  return false;
}

void cast::worker::poll_mgrs (cast::worker *p_worker, const cast::cmd *p_cmd)
{
  int i = 0;
  assert (p_worker);
  assert (p_cmd);

  clients_map_t &clients = p_worker->clients_;

  BOOST_FOREACH (const clients_pair_t &clnt, clients)
  {
    cast::mgr *p_mgr = clnt.second.p_cast_mgr_;
    assert (p_mgr);
    if (!p_mgr->terminated ())
    {
      (void)p_mgr->dispatch_cmd (p_cmd);
    }
    ++i;
  }
    TIZ_LOG (TIZ_PRIORITY_NOTICE,
             "CLIENTS [%d]...", i);
}
