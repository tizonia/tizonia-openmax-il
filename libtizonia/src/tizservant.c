/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizservant.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Servant class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizutils.h"
#include "tizservant.h"
#include "tizservant_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.servant"
#endif

typedef struct tiz_srv_watcher_id tiz_srv_watcher_id_t;
struct tiz_srv_watcher_id
{
  tiz_srv_t *p_srv;
  uint32_t id;
};

static OMX_S32 pqueue_cmp (OMX_PTR ap_left, OMX_PTR ap_right)
{
  /* Not planning to use tiz_pqueue_remove or tiz_pqueue_removep */
  assert (0);
  return 1;
}

static OMX_S32 watchers_map_compare_func (OMX_PTR ap_key1, OMX_PTR ap_key2)
{
  assert (NULL != ap_key1);
  assert (NULL != ap_key2);
  return (ap_key1 == ap_key2) ? 0 : ((ap_key1 < ap_key2) ? -1 : 1);
}

static void watchers_map_free_func (OMX_PTR ap_key, OMX_PTR ap_value)
{
  tiz_srv_watcher_id_t *p_id = (tiz_srv_watcher_id_t *)ap_value;
  tiz_srv_t *p_srv = p_id->p_srv;
  uint32_t id = p_id->id;
  assert (NULL != p_id);
  assert (NULL != p_srv);
  TIZ_TRACE (handleOf (p_srv), "Deleting watcher id [%d]", id);
  tiz_soa_free (p_srv->p_soa_, p_id);
}

static void destroy_watchers_map (tiz_srv_t *ap_srv)
{
  assert (NULL != ap_srv);
  if (ap_srv->p_watchers_)
    {
      while (!tiz_map_empty (ap_srv->p_watchers_))
        {
          tiz_map_erase_at (ap_srv->p_watchers_, 0);
        };
      tiz_map_destroy (ap_srv->p_watchers_);
      ap_srv->p_watchers_ = NULL;
    }
}

static inline bool is_watcher_active (tiz_srv_t *ap_srv, void *ap_watcher,
                                      uint32_t *ap_id)
{
  bool rc = false;
  tiz_srv_watcher_id_t *p_id = NULL;
  assert (NULL != ap_srv);
  if (ap_srv->p_watchers_ && ap_watcher)
    {
      p_id = tiz_map_find (ap_srv->p_watchers_, ap_watcher);
      rc = p_id ? true : false;
      if (ap_id)
        {
          *ap_id = p_id ? p_id->id : 0;
        }
    }
  return rc;
}

/*
 * tiz_srv
 */

static void *srv_ctor (void *ap_obj, va_list *app)
{
  tiz_srv_t *p_srv = super_ctor (typeOf (ap_obj, "tizsrv"), ap_obj, app);
  /* NOTE: The priority queue is initialised only when the allocator is set via
   * set_allocator */
  p_srv->p_pq_ = NULL;
  p_srv->p_soa_ = NULL;
  /* We also lazily initialise the watchers map, when the first watcher is
     allocated */
  p_srv->p_watchers_ = NULL;
  p_srv->watcher_id_ = 0;
  p_srv->p_appdata_ = NULL;
  p_srv->p_cbacks_ = NULL;
  return p_srv;
}

static void *srv_dtor (void *ap_obj)
{
  tiz_srv_t *p_srv = ap_obj;

  destroy_watchers_map (p_srv);

  if (NULL != p_srv->p_pq_)
    {
      OMX_PTR p_msg = NULL;

      /* Make sure we clean up any remaining items in the queue */
      while (0 < tiz_pqueue_length (p_srv->p_pq_))
        {
          if (OMX_ErrorNone != tiz_pqueue_receive (p_srv->p_pq_, &p_msg))
            {
              break;
            }
          assert (NULL != p_msg);
          tiz_soa_free (p_srv->p_soa_, p_msg);
        }

      tiz_pqueue_destroy (p_srv->p_pq_);
    }

  return super_dtor (typeOf (ap_obj, "tizsrv"), ap_obj);
}

static OMX_ERRORTYPE srv_set_allocator (void *ap_obj, tiz_soa_t *p_soa)
{
  tiz_srv_t *p_srv = ap_obj;
  assert (NULL != ap_obj);
  assert (NULL != p_soa);
  p_srv->p_soa_ = p_soa;
  return tiz_pqueue_init (&p_srv->p_pq_, 5, &pqueue_cmp, p_soa,
                          nameOf (ap_obj));
}

OMX_ERRORTYPE
tiz_srv_set_allocator (void *ap_obj, tiz_soa_t *p_soa)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->set_allocator);
  return class->set_allocator (ap_obj, p_soa);
}

static void srv_set_callbacks (void *ap_obj, OMX_PTR ap_appdata,
                               OMX_CALLBACKTYPE *ap_cbacks)
{
  tiz_srv_t *p_srv = ap_obj;
  assert (NULL != p_srv);
  p_srv->p_appdata_ = ap_appdata;
  p_srv->p_cbacks_ = ap_cbacks;
}

void tiz_srv_set_callbacks (void *ap_obj, OMX_PTR ap_appdata,
                            OMX_CALLBACKTYPE *ap_cbacks)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->set_callbacks);
  class->set_callbacks (ap_obj, ap_appdata, ap_cbacks);
}

static OMX_ERRORTYPE srv_tick (const void *ap_obj)
{
  tiz_srv_t *p_srv = (tiz_srv_t *)ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_msg = NULL;

  assert (NULL != p_srv);

  /* Receive the message */
  rc = tiz_pqueue_receive (p_srv->p_pq_, &p_msg);

  if (OMX_ErrorNone != rc)
    {
      TIZ_ERROR (handleOf (ap_obj), "tiz_pqueue_receive error [%s]...",
                 tiz_err_to_str (rc));
      goto end;
    }

  assert (NULL != p_msg);

  /* Process the message */
  rc = tiz_srv_dispatch_msg (p_srv, p_msg);

  if (OMX_ErrorNone != rc)
    {
      TIZ_ERROR (handleOf (ap_obj), "tiz_srv_dispatch_msg error [%s]...",
                 tiz_err_to_str (rc));
    }

end:

  /* We are done with this message */
  tiz_soa_free (p_srv->p_soa_, p_msg);

  if (OMX_ErrorNone != rc && OMX_ErrorNoMore != rc)
    {
      tiz_srv_issue_err_event (p_srv, rc);
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_srv_tick (const void *ap_obj)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->tick);
  return class->tick (ap_obj);
}

OMX_ERRORTYPE
tiz_srv_super_tick (const void *a_class, const void *ap_obj)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->tick);
  return superclass->tick (ap_obj);
}

static OMX_PTR srv_init_msg (void *ap_obj, size_t msg_sz)
{
  tiz_srv_t *p_srv = ap_obj;
  assert (NULL != p_srv);
  assert (NULL != p_srv->p_soa_);
  return tiz_soa_calloc (p_srv->p_soa_, msg_sz);
}

OMX_PTR
tiz_srv_init_msg (void *ap_obj, size_t msg_sz)
{
  const tiz_srv_class_t *class = classOf (ap_obj);

  assert (NULL != class->init_msg);
  return class->init_msg (ap_obj, msg_sz);
}

static OMX_ERRORTYPE srv_enqueue (const void *ap_obj, OMX_PTR ap_data,
                                  OMX_U32 a_priority)
{
  tiz_srv_t *p_srv = (tiz_srv_t *)ap_obj;
  assert (NULL != p_srv);
  return tiz_pqueue_send (p_srv->p_pq_, ap_data, a_priority);
}

OMX_ERRORTYPE
tiz_srv_enqueue (const void *ap_obj, OMX_PTR ap_data, OMX_U32 a_priority)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->enqueue);
  return class->enqueue (ap_obj, ap_data, a_priority);
}

OMX_ERRORTYPE
tiz_srv_super_enqueue (const void *a_class, const void *ap_obj, OMX_PTR ap_data,
                       OMX_U32 a_priority)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->enqueue);
  return superclass->enqueue (ap_obj, ap_data, a_priority);
}

static void srv_remove_from_queue (const void *ap_obj, tiz_pq_func_f apf_func,
                                   OMX_S32 a_data1, OMX_PTR ap_data2)
{
  tiz_srv_t *p_srv = (tiz_srv_t *)ap_obj;
  assert (NULL != p_srv);
  tiz_pqueue_remove_func (p_srv->p_pq_, apf_func, a_data1, ap_data2);
}

void tiz_srv_remove_from_queue (const void *ap_obj, tiz_pq_func_f apf_func,
                                OMX_S32 a_data1, OMX_PTR ap_data2)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->remove_from_queue);
  class->remove_from_queue (ap_obj, apf_func, a_data1, ap_data2);
}

void tiz_srv_super_remove_from_queue (const void *a_class, const void *ap_obj,
                                      tiz_pq_func_f apf_func, OMX_S32 a_data1,
                                      OMX_PTR ap_data2)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->remove_from_queue);
  superclass->remove_from_queue (ap_obj, apf_func, a_data1, ap_data2);
}

static OMX_ERRORTYPE srv_dispatch_msg (const void *ap_obj, OMX_PTR ap_data)
{
  /* This must be implemented by the children */
  assert (0);
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_srv_dispatch_msg (const void *ap_obj, OMX_PTR ap_data)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->dispatch_msg);
  return class->dispatch_msg (ap_obj, ap_data);
}

OMX_ERRORTYPE
tiz_srv_super_dispatch_msg (const void *a_class, const void *ap_obj,
                            OMX_PTR ap_data)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->dispatch_msg);
  return superclass->dispatch_msg (ap_obj, ap_data);
}

static bool srv_is_ready (const void *ap_obj)
{
  const tiz_srv_t *p_srv = ap_obj;
  assert (NULL != p_srv);
  return (tiz_pqueue_length (p_srv->p_pq_) > 0 ? OMX_TRUE : OMX_FALSE);
}

bool tiz_srv_is_ready (const void *ap_obj)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->is_ready);
  return class->is_ready (ap_obj);
}

bool tiz_srv_super_is_ready (const void *a_class, const void *ap_obj)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->is_ready);
  return superclass->is_ready (ap_obj);
}

static OMX_ERRORTYPE srv_allocate_resources (const void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_srv_allocate_resources (const void *ap_obj, OMX_U32 a_pid)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->allocate_resources);
  return class->allocate_resources (ap_obj, a_pid);
}

OMX_ERRORTYPE
tiz_srv_super_allocate_resources (const void *a_class, const void *ap_obj,
                                  OMX_U32 a_pid)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->allocate_resources);
  return superclass->allocate_resources (ap_obj, a_pid);
}

static OMX_ERRORTYPE srv_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_srv_deallocate_resources (const void *ap_obj)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->deallocate_resources);
  return class->deallocate_resources (ap_obj);
}

OMX_ERRORTYPE
tiz_srv_super_deallocate_resources (const void *a_class, const void *ap_obj)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->deallocate_resources);
  return superclass->deallocate_resources (ap_obj);
}

static OMX_ERRORTYPE srv_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_srv_prepare_to_transfer (const void *ap_obj, OMX_U32 a_pid)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->prepare_to_transfer);
  return class->prepare_to_transfer (ap_obj, a_pid);
}

OMX_ERRORTYPE
tiz_srv_super_prepare_to_transfer (const void *a_class, const void *ap_obj,
                                   OMX_U32 a_pid)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->prepare_to_transfer);
  return superclass->prepare_to_transfer (ap_obj, a_pid);
}

static OMX_ERRORTYPE srv_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_srv_transfer_and_process (const void *ap_obj, OMX_U32 a_pid)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->transfer_and_process);
  return class->transfer_and_process (ap_obj, a_pid);
}

OMX_ERRORTYPE
tiz_srv_super_transfer_and_process (const void *a_class, const void *ap_obj,
                                    OMX_U32 a_pid)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->transfer_and_process);
  return superclass->transfer_and_process (ap_obj, a_pid);
}

static OMX_ERRORTYPE srv_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_srv_stop_and_return (const void *ap_obj)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->stop_and_return);
  return class->stop_and_return (ap_obj);
}

OMX_ERRORTYPE
tiz_srv_super_stop_and_return (const void *a_class, const void *ap_obj)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->stop_and_return);
  return superclass->stop_and_return (ap_obj);
}

static void srv_issue_event (const void *ap_obj, OMX_EVENTTYPE a_event,
                             OMX_U32 a_data1, OMX_U32 a_data2,
                             /*@null@*/ OMX_PTR ap_eventdata)
{
  tiz_srv_t *p_srv = (tiz_srv_t *)ap_obj;
  assert (NULL != p_srv);
  assert (NULL != p_srv->p_cbacks_);
  assert (NULL != p_srv->p_cbacks_->EventHandler);
  /* NOTE: Start ignoring splint warnings in this section of code */
  /*@ignore@*/
  TIZ_NOTICE (handleOf (ap_obj), "[%s]", tiz_evt_to_str (a_event));
  (void)p_srv->p_cbacks_->EventHandler (handleOf (ap_obj), p_srv->p_appdata_,
                                        a_event, a_data1, a_data2,
                                        ap_eventdata);
  /*@end@*/
  /* NOTE: Stop ignoring splint warnings in this section  */
}

void tiz_srv_issue_event (const void *ap_obj, OMX_EVENTTYPE a_event,
                          OMX_U32 a_data1, OMX_U32 a_data2,
                          OMX_PTR ap_eventdata)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->issue_event);
  class->issue_event (ap_obj, a_event, a_data1, a_data2, ap_eventdata);
}

static void srv_issue_err_event (const void *ap_obj, OMX_ERRORTYPE a_error)
{
  TIZ_ERROR (handleOf (ap_obj), "OMX_EventError...[%s]",
             tiz_err_to_str (a_error));
  srv_issue_event (ap_obj, OMX_EventError, a_error, 0, 0);
}

void tiz_srv_issue_err_event (const void *ap_obj, OMX_ERRORTYPE a_error)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->issue_err_event);
  class->issue_err_event (ap_obj, a_error);
}

static void srv_issue_cmd_event (const void *ap_obj, OMX_COMMANDTYPE a_cmd,
                                 OMX_U32 a_pid, OMX_ERRORTYPE a_error)
{
  TIZ_NOTICE (handleOf (ap_obj),
              "[OMX_EventCmdComplete] [%s] PID [%d] [%s]",
              tiz_cmd_to_str (a_cmd), a_pid, tiz_err_to_str (a_error));
  srv_issue_event (ap_obj, OMX_EventCmdComplete, a_cmd, a_pid,
                   (OMX_PTR)a_error);
}

void tiz_srv_issue_cmd_event (const void *ap_obj, OMX_COMMANDTYPE a_cmd,
                              OMX_U32 a_pid, OMX_ERRORTYPE a_error)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->issue_cmd_event);
  class->issue_cmd_event (ap_obj, a_cmd, a_pid, a_error);
}

static void srv_issue_trans_event (void *ap_obj, OMX_STATETYPE a_state,
                                   OMX_ERRORTYPE a_error)
{
  TIZ_NOTICE (handleOf (ap_obj),
              "[OMX_EventCmdComplete] [OMX_CommandStateSet] [%s]",
              tiz_fsm_state_to_str (a_state));
  srv_issue_event (ap_obj, OMX_EventCmdComplete, OMX_CommandStateSet, a_state,
                   (OMX_PTR)a_error);
}

void tiz_srv_issue_trans_event (const void *ap_obj, OMX_STATETYPE a_state,
                                OMX_ERRORTYPE a_error)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->issue_trans_event);
  class->issue_trans_event (ap_obj, a_state, a_error);
}

static void srv_issue_buf_callback (const void *ap_obj,
                                    OMX_BUFFERHEADERTYPE *p_hdr, OMX_U32 pid,
                                    OMX_DIRTYPE dir, OMX_HANDLETYPE ap_tcomp)
{
  tiz_srv_t *p_srv = (tiz_srv_t *)ap_obj;
  assert (NULL != p_srv);
  assert (NULL != p_srv->p_cbacks_);
  assert (NULL != p_srv->p_cbacks_->EventHandler);
  if (ap_tcomp)
    {
      if (OMX_DirInput == dir)
        {
          TIZ_DEBUG (handleOf (ap_obj),
                     "[OMX_FillThisBuffer] : "
                     "HEADER [%p] BUFFER [%p] [%d:%d] [%s]",
                     p_hdr, p_hdr->pBuffer, p_hdr->nFilledLen,
                     p_hdr->nAllocLen, TIZ_CNAME(ap_tcomp));
          (void)OMX_FillThisBuffer (ap_tcomp, p_hdr);
        }
      else
        {
          TIZ_DEBUG (handleOf (ap_obj),
                     "[OMX_EmptyThisBuffer] : "
                     "HEADER [%p] BUFFER [%p] [F(%d):A(%d)] [%s]",
                     p_hdr, p_hdr->pBuffer, p_hdr->nFilledLen,
                     p_hdr->nAllocLen, TIZ_CNAME (ap_tcomp));
          (void)OMX_EmptyThisBuffer (ap_tcomp, p_hdr);
        }
    }

  else
    {
      OMX_ERRORTYPE (*fp_buf_done)
      (OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE *)
          = (dir == OMX_DirInput ? p_srv->p_cbacks_->EmptyBufferDone
                                 : p_srv->p_cbacks_->FillBufferDone);

      TIZ_DEBUG (handleOf (ap_obj), "[%] : HEADER [%p] BUFFER [%p]",
                 dir == OMX_DirInput ? "EmptyBufferDone" : "FillBufferDone",
                 p_hdr, p_hdr->pBuffer);

      (void)fp_buf_done (handleOf (ap_obj), p_srv->p_appdata_, p_hdr);
    }
}

void tiz_srv_issue_buf_callback (const void *ap_obj,
                                 OMX_BUFFERHEADERTYPE *p_hdr, OMX_U32 pid,
                                 OMX_DIRTYPE dir, OMX_HANDLETYPE ap_tcomp)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->issue_buf_callback);
  class->issue_buf_callback (ap_obj, p_hdr, pid, dir, ap_tcomp);
}

static OMX_ERRORTYPE srv_receive_pluggable_event (
    void *ap_obj, tiz_event_pluggable_t *ap_event)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_srv_receive_pluggable_event (void *ap_obj, tiz_event_pluggable_t *ap_event)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->receive_pluggable_event);
  return class->receive_pluggable_event (ap_obj, ap_event);
}

static void *srv_soa_calloc (void *ap_obj, size_t a_size)
{
  tiz_srv_t *p_srv = ap_obj;
  assert (NULL != p_srv);
  return tiz_soa_calloc (p_srv->p_soa_, a_size);
}

void *tiz_srv_soa_calloc (void *ap_obj, size_t a_size)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->soa_calloc);
  return class->soa_calloc (ap_obj, a_size);
}

static void srv_soa_free (void *ap_obj, void *ap_addr)
{
  tiz_srv_t *p_srv = ap_obj;
  assert (NULL != p_srv);
  tiz_soa_free (p_srv->p_soa_, ap_addr);
}

void tiz_srv_soa_free (void *ap_obj, void *ap_addr)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->soa_free);
  class->soa_free (ap_obj, ap_addr);
}

static OMX_ERRORTYPE srv_io_watcher_init (void *ap_obj,
                                          tiz_event_io_t **app_ev_io, int a_fd,
                                          tiz_event_io_event_t a_event,
                                          bool only_once)
{
  tiz_srv_t *p_srv = ap_obj;

  assert (NULL != p_srv);
  assert (NULL != app_ev_io);

  /* We lazily initialise the watchers map */
  if (!p_srv->p_watchers_)
    {
      tiz_check_omx_err (tiz_map_init (&(p_srv->p_watchers_),
                                       watchers_map_compare_func,
                                       watchers_map_free_func, p_srv->p_soa_));
    }
  tiz_check_omx_err (tiz_event_io_init (app_ev_io, handleOf (p_srv),
                                        tiz_comp_event_io, p_srv));
  assert (NULL != *app_ev_io);
  tiz_event_io_set (*app_ev_io, a_fd, a_event, only_once);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE tiz_srv_io_watcher_init (void *ap_obj, tiz_event_io_t **app_ev_io,
                                       int a_fd, tiz_event_io_event_t a_event,
                                       bool only_once)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->io_watcher_init);
  return class->io_watcher_init (ap_obj, app_ev_io, a_fd, a_event, only_once);
}

static OMX_ERRORTYPE srv_io_watcher_start (void *ap_obj,
                                           tiz_event_io_t *ap_ev_io)
{
  tiz_srv_t *p_srv = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  uint32_t id = 0;

  assert (NULL != p_srv);
  assert (NULL != ap_ev_io);
  assert (NULL != p_srv->p_watchers_);

  if (!is_watcher_active (p_srv, ap_ev_io, &id))
    {
      tiz_srv_watcher_id_t *p_id = tiz_soa_calloc (p_srv->p_soa_, sizeof(tiz_srv_watcher_id_t));
      if (p_id)
        {
          OMX_U32 index = 0;
          p_id->p_srv = p_srv;
          p_id->id = p_srv->watcher_id_++;
          id = p_id->id;
          index = tiz_map_size (p_srv->p_watchers_);
          tiz_check_omx_err (tiz_map_insert (p_srv->p_watchers_, ap_ev_io, p_id, &index));
          rc = tiz_event_io_start (ap_ev_io, id);
          TIZ_TRACE (handleOf (ap_obj), "started watcher with id [%d]", id);
        }
    }
  return rc;
}

OMX_ERRORTYPE tiz_srv_io_watcher_start (void *ap_obj, tiz_event_io_t *ap_ev_io)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->io_watcher_start);
  return class->io_watcher_start (ap_obj, ap_ev_io);
}

static OMX_ERRORTYPE srv_io_watcher_stop (void *ap_obj,
                                          tiz_event_io_t *ap_ev_io)
{
  tiz_srv_t *p_srv = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  uint32_t id = 0;

  assert (NULL != p_srv);

  if (is_watcher_active (p_srv, ap_ev_io, &id))
    {
      rc = tiz_event_io_stop (ap_ev_io);
      tiz_map_erase (p_srv->p_watchers_, ap_ev_io);
      TIZ_TRACE (handleOf (ap_obj), "stopped watcher with id [%d]", id);
    }
  return rc;
}

OMX_ERRORTYPE tiz_srv_io_watcher_stop (void *ap_obj, tiz_event_io_t *ap_ev_io)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->io_watcher_stop);
  return class->io_watcher_stop (ap_obj, ap_ev_io);
}

static void srv_io_watcher_destroy (void *ap_obj, tiz_event_io_t *ap_ev_io)
{
  tiz_srv_t *p_srv = ap_obj;

  assert (NULL != p_srv);

  if (ap_ev_io)
    {
      if (p_srv->p_watchers_)
        {
          (void)srv_io_watcher_stop (p_srv, ap_ev_io);
        }
      tiz_event_io_destroy (ap_ev_io);
      TIZ_TRACE (handleOf (ap_obj), "destroyed watcher [%p]", ap_ev_io);

    }
}

void tiz_srv_io_watcher_destroy (void *ap_obj, tiz_event_io_t *ap_ev_io)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->io_watcher_destroy);
  class->io_watcher_destroy (ap_obj, ap_ev_io);
}

static OMX_ERRORTYPE srv_timer_watcher_init (void *ap_obj,
                                             tiz_event_timer_t **app_ev_timer)
{
  tiz_srv_t *p_srv = ap_obj;

  assert (NULL != p_srv);
  assert (NULL != app_ev_timer);

  /* We lazily initialise the watchers map */
  if (!p_srv->p_watchers_)
    {
      tiz_check_omx_err (tiz_map_init (&(p_srv->p_watchers_),
                                       watchers_map_compare_func,
                                       watchers_map_free_func, p_srv->p_soa_));
    }

  return tiz_event_timer_init (app_ev_timer, handleOf (p_srv),
                               tiz_comp_event_timer, p_srv);
}

OMX_ERRORTYPE tiz_srv_timer_watcher_init (void *ap_obj,
                                          tiz_event_timer_t **app_ev_timer)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->timer_watcher_init);
  return class->timer_watcher_init (ap_obj, app_ev_timer);
}

static OMX_ERRORTYPE srv_timer_watcher_start (void *ap_obj,
                                              tiz_event_timer_t *ap_ev_timer,
                                              const double a_after,
                                              const double a_repeat)
{
  tiz_srv_t *p_srv = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  uint32_t id = 0;

  assert (NULL != p_srv);
  assert (NULL != ap_ev_timer);
  assert (NULL != p_srv->p_watchers_);

  if (!is_watcher_active (p_srv, ap_ev_timer, &id))
    {
      tiz_srv_watcher_id_t *p_id = tiz_soa_calloc (p_srv->p_soa_, sizeof(tiz_srv_watcher_id_t));
      if (p_id)
        {
          OMX_U32 index = 0;
          p_id->p_srv = p_srv;
          p_id->id = p_srv->watcher_id_++;
          id = p_id->id;
          tiz_event_timer_set (ap_ev_timer, a_after, a_repeat);
          index = tiz_map_size (p_srv->p_watchers_);
          tiz_check_omx_err (tiz_map_insert (p_srv->p_watchers_, ap_ev_timer, p_id, &index));
          rc = tiz_event_timer_start (ap_ev_timer, id);
          TIZ_TRACE (handleOf (ap_obj), "started timer watcher with id [%d]", id);
        }
    }
  return rc;
}

OMX_ERRORTYPE tiz_srv_timer_watcher_start (void *ap_obj,
                                           tiz_event_timer_t *ap_ev_timer,
                                           const double a_after,
                                           const double a_repeat)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->timer_watcher_start);
  return class->timer_watcher_start (ap_obj, ap_ev_timer, a_after, a_repeat);
}

static OMX_ERRORTYPE srv_timer_watcher_restart (void *ap_obj,
                                                tiz_event_timer_t *ap_ev_timer)
{
  tiz_srv_t *p_srv = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_srv_watcher_id_t *p_id = NULL;
  uint32_t id = 0;

  assert (NULL != p_srv);
  assert (NULL != ap_ev_timer);
  assert (NULL != p_srv->p_watchers_);

  if (is_watcher_active (p_srv, ap_ev_timer, &id))
    {
      tiz_map_erase (p_srv->p_watchers_, ap_ev_timer);
    }

  p_id = tiz_soa_calloc (p_srv->p_soa_, sizeof(tiz_srv_watcher_id_t));
  if (p_id)
    {
      OMX_U32 index = 0;
      p_id->p_srv = p_srv;
      p_id->id = p_srv->watcher_id_++;
      id = p_id->id;
      index = tiz_map_size (p_srv->p_watchers_);
      tiz_check_omx_err (tiz_map_insert (p_srv->p_watchers_, ap_ev_timer, p_id, &index));
      rc = tiz_event_timer_restart (ap_ev_timer, id);
      TIZ_TRACE (handleOf (ap_obj), "restarted watcher with id [%d]", id);
    }

  return rc;
}

OMX_ERRORTYPE tiz_srv_timer_watcher_restart (void *ap_obj,
                                             tiz_event_timer_t *ap_ev_timer)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->timer_watcher_restart);
  return class->timer_watcher_restart (ap_obj, ap_ev_timer);
}

static OMX_ERRORTYPE srv_timer_watcher_stop (void *ap_obj,
                                             tiz_event_timer_t *ap_ev_timer)
{
  tiz_srv_t *p_srv = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorBadParameter;
  uint32_t id = 0;

  assert (NULL != p_srv);

  if (is_watcher_active (p_srv, ap_ev_timer, &id))
    {
      rc = tiz_event_timer_stop (ap_ev_timer);
      tiz_map_erase (p_srv->p_watchers_, ap_ev_timer);
      TIZ_TRACE (handleOf (ap_obj), "stopped timer watcher id [%d]", id);
    }
  return rc;
}

OMX_ERRORTYPE tiz_srv_timer_watcher_stop (void *ap_obj,
                                          tiz_event_timer_t *ap_ev_timer)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->timer_watcher_stop);
  return class->timer_watcher_stop (ap_obj, ap_ev_timer);
}

static void srv_timer_watcher_destroy (void *ap_obj,
                                       tiz_event_timer_t *ap_ev_timer)
{
  tiz_srv_t *p_srv = ap_obj;
  assert (NULL != p_srv);
  if (ap_ev_timer)
    {
      if (p_srv->p_watchers_)
        {
          (void)srv_timer_watcher_stop (p_srv, ap_ev_timer);
        }
      tiz_event_timer_destroy (ap_ev_timer);
      TIZ_TRACE (handleOf (ap_obj), "destroyed watcher with [%p]", ap_ev_timer);
    }
}

void tiz_srv_timer_watcher_destroy (void *ap_obj,
                                    tiz_event_timer_t *ap_ev_timer)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->timer_watcher_destroy);
  class->timer_watcher_destroy (ap_obj, ap_ev_timer);
}

static OMX_ERRORTYPE srv_event_io (void *ap_obj, tiz_event_io_t *ap_ev_io,
                                   const uint32_t a_id, int a_fd, int a_events)
{
  tiz_srv_t *p_srv = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  uint32_t id = 0;
  assert (NULL != p_srv);
  assert (NULL != ap_ev_io);
  /* Notify an io event only if it is currently active */
  if (is_watcher_active (p_srv, ap_ev_io, &id) && a_id == id)
    {
      /* Remove from the map if it is level-triggered */
      if (tiz_event_io_is_level_triggered (ap_ev_io))
        {
          tiz_map_erase (p_srv->p_watchers_, ap_ev_io);
          TIZ_TRACE (handleOf (ap_obj), "stopped watcher id [%d]", id);
        }
      rc = tiz_srv_io_ready (p_srv, ap_ev_io, a_fd, a_events);
    }
  else
    {
      TIZ_TRACE (handleOf (ap_obj), "ignoring watcher id [%d] a_id [%d]", id, a_id);
    }
  return rc;
}

OMX_ERRORTYPE
tiz_srv_event_io (void *ap_obj, tiz_event_io_t *ap_ev_io, const uint32_t a_id,
                  int a_fd, int a_events)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->event_io);
  return class->event_io (ap_obj, ap_ev_io, a_id, a_fd, a_events);
}

static OMX_ERRORTYPE srv_event_timer (void *ap_obj,
                                      tiz_event_timer_t *ap_ev_timer,
                                      const uint32_t a_id)
{
  tiz_srv_t *p_srv = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  uint32_t id = 0;
  assert (NULL != p_srv);
  assert (NULL != ap_ev_timer);
  /* Notify a timer event only if it is currently active */
  if (is_watcher_active (p_srv, ap_ev_timer, &id) && a_id == id)
    {
      TIZ_TRACE (handleOf (ap_obj), "signaling timer watcher id [%d]", id);
      rc = tiz_srv_timer_ready (p_srv, ap_ev_timer);
    }
  else
    {
      TIZ_TRACE (handleOf (ap_obj), "ignoring timer watcher id [%d] a_id [%d]", id, a_id);
    }
  return rc;
}

OMX_ERRORTYPE
tiz_srv_event_timer (void *ap_obj, tiz_event_timer_t *ap_ev_timer,
                     const uint32_t a_id)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->event_timer);
  return class->event_timer (ap_obj, ap_ev_timer, a_id);
}

static OMX_ERRORTYPE srv_event_stat (void *ap_obj, tiz_event_stat_t *ap_ev_stat,
                                     int a_events)
{
  /* TODO */
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_srv_event_stat (void *ap_obj, tiz_event_stat_t *ap_ev_stat,
                    const uint32_t a_id, int a_events)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->event_stat);
  return class->event_stat (ap_obj, ap_ev_stat, a_id, a_events);
}

static OMX_ERRORTYPE srv_io_ready (const void *ap_obj, tiz_event_io_t *ap_ev_io,
                                   int a_fd, int a_events)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_srv_io_ready (void *ap_obj, tiz_event_io_t *ap_ev_io, int a_fd,
                  int a_events)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->io_ready);
  return class->io_ready (ap_obj, ap_ev_io, a_fd, a_events);
}

static OMX_ERRORTYPE srv_timer_ready (void *ap_obj,
                                      tiz_event_timer_t *ap_ev_timer)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_srv_timer_ready (void *ap_obj, tiz_event_timer_t *ap_ev_timer)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->timer_ready);
  return class->timer_ready (ap_obj, ap_ev_timer);
}

static OMX_ERRORTYPE srv_stat_ready (void *ap_obj, tiz_event_stat_t *ap_ev_stat,
                                     int a_events)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_srv_stat_ready (void *ap_obj, tiz_event_stat_t *ap_ev_stat, int a_events)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->stat_ready);
  return class->stat_ready (ap_obj, ap_ev_stat, a_events);
}

/*
 * tiz_srv_class
 */

static void *srv_class_ctor (void *ap_obj, va_list *app)
{
  tiz_srv_class_t *p_srv
      = super_ctor (typeOf (ap_obj, "tizsrv_class"), ap_obj, app);
  typedef void (*voidf)();
  voidf selector = NULL;
  va_list ap;
  va_copy (ap, *app);

  /* NOTE: Start ignoring splint warnings in this section of code */
  /*@ignore@*/
  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf)tiz_srv_set_allocator)
        {
          *(voidf *)&p_srv->set_allocator = method;
        }
      else if (selector == (voidf)tiz_srv_set_callbacks)
        {
          *(voidf *)&p_srv->set_callbacks = method;
        }
      else if (selector == (voidf)tiz_srv_tick)
        {
          *(voidf *)&p_srv->tick = method;
        }
      else if (selector == (voidf)tiz_srv_init_msg)
        {
          *(voidf *)&p_srv->init_msg = method;
        }
      else if (selector == (voidf)tiz_srv_enqueue)
        {
          *(voidf *)&p_srv->enqueue = method;
        }
      else if (selector == (voidf)tiz_srv_remove_from_queue)
        {
          *(voidf *)&p_srv->remove_from_queue = method;
        }
      else if (selector == (voidf)tiz_srv_dispatch_msg)
        {
          *(voidf *)&p_srv->dispatch_msg = method;
        }
      else if (selector == (voidf)tiz_srv_is_ready)
        {
          *(voidf *)&p_srv->is_ready = method;
        }
      else if (selector == (voidf)tiz_srv_allocate_resources)
        {
          *(voidf *)&p_srv->allocate_resources = method;
        }
      else if (selector == (voidf)tiz_srv_deallocate_resources)
        {
          *(voidf *)&p_srv->deallocate_resources = method;
        }
      else if (selector == (voidf)tiz_srv_prepare_to_transfer)
        {
          *(voidf *)&p_srv->prepare_to_transfer = method;
        }
      else if (selector == (voidf)tiz_srv_transfer_and_process)
        {
          *(voidf *)&p_srv->transfer_and_process = method;
        }
      else if (selector == (voidf)tiz_srv_stop_and_return)
        {
          *(voidf *)&p_srv->stop_and_return = method;
        }
      else if (selector == (voidf)tiz_srv_issue_event)
        {
          *(voidf *)&p_srv->issue_event = method;
        }
      else if (selector == (voidf)tiz_srv_issue_err_event)
        {
          *(voidf *)&p_srv->issue_err_event = method;
        }
      else if (selector == (voidf)tiz_srv_issue_cmd_event)
        {
          *(voidf *)&p_srv->issue_cmd_event = method;
        }
      else if (selector == (voidf)tiz_srv_issue_trans_event)
        {
          *(voidf *)&p_srv->issue_trans_event = method;
        }
      else if (selector == (voidf)tiz_srv_issue_buf_callback)
        {
          *(voidf *)&p_srv->issue_buf_callback = method;
        }
      else if (selector == (voidf)tiz_srv_receive_pluggable_event)
        {
          *(voidf *)&p_srv->receive_pluggable_event = method;
        }
      else if (selector == (voidf)tiz_srv_soa_calloc)
        {
          *(voidf *)&p_srv->soa_calloc = method;
        }
      else if (selector == (voidf)tiz_srv_soa_free)
        {
          *(voidf *)&p_srv->soa_free = method;
        }
      else if (selector == (voidf)tiz_srv_io_watcher_init)
        {
          *(voidf *)&p_srv->io_watcher_init = method;
        }
      else if (selector == (voidf)tiz_srv_io_watcher_start)
        {
          *(voidf *)&p_srv->io_watcher_start = method;
        }
      else if (selector == (voidf)tiz_srv_io_watcher_stop)
        {
          *(voidf *)&p_srv->io_watcher_stop = method;
        }
      else if (selector == (voidf)tiz_srv_io_watcher_destroy)
        {
          *(voidf *)&p_srv->io_watcher_destroy = method;
        }
      else if (selector == (voidf)tiz_srv_timer_watcher_init)
        {
          *(voidf *)&p_srv->timer_watcher_init = method;
        }
      else if (selector == (voidf)tiz_srv_timer_watcher_start)
        {
          *(voidf *)&p_srv->timer_watcher_start = method;
        }
      else if (selector == (voidf)tiz_srv_timer_watcher_restart)
        {
          *(voidf *)&p_srv->timer_watcher_restart = method;
        }
      else if (selector == (voidf)tiz_srv_timer_watcher_stop)
        {
          *(voidf *)&p_srv->timer_watcher_stop = method;
        }
      else if (selector == (voidf)tiz_srv_timer_watcher_destroy)
        {
          *(voidf *)&p_srv->timer_watcher_destroy = method;
        }
      else if (selector == (voidf)tiz_srv_event_io)
        {
          *(voidf *)&p_srv->event_io = method;
        }
      else if (selector == (voidf)tiz_srv_event_timer)
        {
          *(voidf *)&p_srv->event_timer = method;
        }
      else if (selector == (voidf)tiz_srv_event_stat)
        {
          *(voidf *)&p_srv->event_stat = method;
        }
      else if (selector == (voidf)tiz_srv_io_ready)
        {
          *(voidf *)&p_srv->io_ready = method;
        }
      else if (selector == (voidf)tiz_srv_timer_ready)
        {
          *(voidf *)&p_srv->timer_ready = method;
        }
      else if (selector == (voidf)tiz_srv_stat_ready)
        {
          *(voidf *)&p_srv->stat_ready = method;
        }
    }
  /*@end@*/
  /* NOTE: Stop ignoring splint warnings in this section  */

  va_end (ap);
  return p_srv;
}

/*
 * initialization
 */

void *tiz_srv_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizapi = tiz_get_type (ap_hdl, "tizapi");
  void *tizsrv_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizapi), "tizsrv_class", classOf (tizapi),
       sizeof(tiz_srv_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, srv_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return tizsrv_class;
}

void *tiz_srv_init (void *ap_tos, void *ap_hdl)
{
  void *tizapi = tiz_get_type (ap_hdl, "tizapi");
  void *tizsrv_class = tiz_get_type (ap_hdl, "tizsrv_class");
  TIZ_LOG_CLASS (tizsrv_class);
  void *tizsrv = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (tizsrv_class, "tizsrv", tizapi, sizeof(tiz_srv_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, srv_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, srv_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_set_allocator, srv_set_allocator,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_set_callbacks, srv_set_callbacks,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_tick, srv_tick,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_init_msg, srv_init_msg,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_enqueue, srv_enqueue,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_remove_from_queue, srv_remove_from_queue,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_dispatch_msg, srv_dispatch_msg,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_is_ready, srv_is_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, srv_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, srv_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, srv_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, srv_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, srv_stop_and_return,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_issue_event, srv_issue_event,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_issue_err_event, srv_issue_err_event,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_issue_cmd_event, srv_issue_cmd_event,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_issue_trans_event, srv_issue_trans_event,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_issue_buf_callback, srv_issue_buf_callback,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_receive_pluggable_event, srv_receive_pluggable_event,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_soa_calloc, srv_soa_calloc,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_soa_free, srv_soa_free,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_io_watcher_init, srv_io_watcher_init,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_io_watcher_start, srv_io_watcher_start,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_io_watcher_stop, srv_io_watcher_stop,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_io_watcher_destroy, srv_io_watcher_destroy,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_timer_watcher_init, srv_timer_watcher_init,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_timer_watcher_start, srv_timer_watcher_start,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_timer_watcher_restart, srv_timer_watcher_restart,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_timer_watcher_stop, srv_timer_watcher_stop,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_timer_watcher_destroy, srv_timer_watcher_destroy,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_event_io, srv_event_io,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_event_timer, srv_event_timer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_event_stat, srv_event_stat,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_io_ready, srv_io_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_timer_ready, srv_timer_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stat_ready, srv_stat_ready,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);

  return tizsrv;
}
