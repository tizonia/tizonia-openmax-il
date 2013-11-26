/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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

#include "tizservant.h"
#include "tizservant_decls.h"
#include "tizscheduler.h"
#include "tizutils.h"

#include "tizosal.h"

#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.servant"
#endif

/*
 * tiz_srv
 */

static OMX_S32
pqueue_cmp (OMX_PTR ap_left, OMX_PTR ap_right)
{
  /* Not planning to use tiz_pqueue_remove or tiz_pqueue_removep */
  assert (0);
  return 1;
}

static void *
servant_ctor (void *ap_obj, va_list * app)
{
  tiz_srv_t *p_obj = super_ctor (typeOf (ap_obj, "tizsrv"), ap_obj, app);
  /* NOTE: The priority queue is initialised only when the allocator is set via
   * set_allocator */
  p_obj->p_pq_ = NULL;
  p_obj->p_soa_ = NULL;
  p_obj->p_appdata_ = NULL;
  p_obj->p_cbacks_ = NULL;
  return p_obj;
}

static void *
servant_dtor (void *ap_obj)
{
  tiz_srv_t *p_obj = ap_obj;

  if (NULL != p_obj->p_pq_)
    {
      OMX_PTR p_msg = NULL;

      /* Make sure we clean up any remaining items in the queue */
      while (0 < tiz_pqueue_length (p_obj->p_pq_))
        {
          if (OMX_ErrorNone != tiz_pqueue_receive (p_obj->p_pq_, &p_msg))
            {
              break;
            }
          assert (NULL != p_msg);
          tiz_soa_free (p_obj->p_soa_, p_msg);
        }

      tiz_pqueue_destroy (p_obj->p_pq_);
    }

  return super_dtor (typeOf (ap_obj, "tizsrv"), ap_obj);
}

static OMX_ERRORTYPE
servant_set_allocator (void *ap_obj, tiz_soa_t * p_soa)
{
  tiz_srv_t *p_obj = ap_obj;
  assert (NULL != ap_obj);
  assert (NULL != p_soa);
  p_obj->p_soa_ = p_soa;
  return tiz_pqueue_init (&p_obj->p_pq_, 5, &pqueue_cmp, p_soa,
                          nameOf (ap_obj));
}

OMX_ERRORTYPE
tiz_srv_set_allocator (void *ap_obj, tiz_soa_t * p_soa)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->set_allocator);
  return class->set_allocator (ap_obj, p_soa);
}

static void
servant_set_callbacks (void *ap_obj,
                       OMX_PTR ap_appdata, OMX_CALLBACKTYPE * ap_cbacks)
{
  tiz_srv_t *p_obj = ap_obj;
  p_obj->p_appdata_ = ap_appdata;
  p_obj->p_cbacks_ = ap_cbacks;
}

void
tiz_srv_set_callbacks (void *ap_obj, OMX_PTR ap_appdata,
                          OMX_CALLBACKTYPE * ap_cbacks)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->set_callbacks);
  class->set_callbacks (ap_obj, ap_appdata, ap_cbacks);
}

static OMX_ERRORTYPE
servant_tick (const void *ap_obj)
{
  tiz_srv_t *p_obj = (tiz_srv_t *) ap_obj;
  OMX_PTR p_msg = NULL;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;

  for (;;)
    {
      if (OMX_ErrorNone !=
          (ret_val = tiz_pqueue_receive (p_obj->p_pq_, &p_msg)))
        {
          TIZ_ERROR (handleOf (ap_obj),
                    "tiz_pqueue_receive error [%s]...",
                    tiz_err_to_str (ret_val));
          break;
        }
      assert (NULL != p_msg);

      /* Process the message */
      ret_val = tiz_srv_dispatch_msg (p_obj, p_msg);

      /* We are done with this message */
      tiz_soa_free (p_obj->p_soa_, p_msg);

      if (OMX_ErrorNone != ret_val)
        {
          TIZ_ERROR (handleOf (ap_obj),
                    "tiz_srv_dispatch_msg error [%s]...",
                    tiz_err_to_str (ret_val));
          break;
        }



      /* TODO: This is a temporary solution. */
      /* This function can be overloaded by derived classes. */
      /* Remove the for loop or break statement once the default behaviour */
      /* completely decided .... */
      break;
    }

  if (OMX_ErrorNone != ret_val && OMX_ErrorNoMore != ret_val)
    {
      tiz_srv_issue_err_event (p_obj, ret_val);
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

static OMX_PTR
servant_init_msg (void *ap_obj, size_t msg_sz)
{
  tiz_srv_t *p_obj = ap_obj;
  assert (NULL != p_obj->p_soa_);
  return tiz_soa_calloc (p_obj->p_soa_, msg_sz);
}

OMX_PTR
tiz_srv_init_msg (void *ap_obj, size_t msg_sz)
{
  const tiz_srv_class_t *class = classOf (ap_obj);

  assert (NULL != class->init_msg);
  return class->init_msg (ap_obj, msg_sz);
}

static OMX_ERRORTYPE
servant_enqueue (const void *ap_obj, OMX_PTR ap_data, OMX_U32 a_priority)
{
  tiz_srv_t *p_obj = (tiz_srv_t *) ap_obj;
  return tiz_pqueue_send (p_obj->p_pq_, ap_data, a_priority);
}

OMX_ERRORTYPE
tiz_srv_enqueue (const void *ap_obj, OMX_PTR ap_data, OMX_U32 a_priority)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->enqueue);
  return class->enqueue (ap_obj, ap_data, a_priority);
}

OMX_ERRORTYPE
tiz_srv_super_enqueue (const void *a_class,
                          const void *ap_obj,
                          OMX_PTR ap_data, OMX_U32 a_priority)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->enqueue);
  return superclass->enqueue (ap_obj, ap_data, a_priority);
}

static void
servant_remove_from_queue (const void *ap_obj, tiz_pq_func_f apf_func,
                           OMX_S32 a_data1, OMX_PTR ap_data2)
{
  tiz_srv_t *p_obj = (tiz_srv_t *) ap_obj;
  tiz_pqueue_remove_func (p_obj->p_pq_, apf_func, a_data1, ap_data2);
}

void
tiz_srv_remove_from_queue (const void *ap_obj, tiz_pq_func_f apf_func,
                              OMX_S32 a_data1, OMX_PTR ap_data2)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->remove_from_queue);
  class->remove_from_queue (ap_obj, apf_func, a_data1, ap_data2);
}

void
tiz_srv_super_remove_from_queue (const void *a_class,
                                 const void *ap_obj,
                                 tiz_pq_func_f apf_func,
                                 OMX_S32 a_data1, OMX_PTR ap_data2)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->remove_from_queue);
  superclass->remove_from_queue (ap_obj, apf_func, a_data1, ap_data2);
}

static OMX_ERRORTYPE
servant_dispatch_msg (const void *ap_obj, OMX_PTR ap_data)
{
  /* This is to be implemented by the children */
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
tiz_srv_super_dispatch_msg (const void *a_class,
                               const void *ap_obj, OMX_PTR ap_data)
{
  const tiz_srv_class_t *superclass = super (a_class);

  assert (NULL != ap_obj && NULL != superclass->dispatch_msg);
  return superclass->dispatch_msg (ap_obj, ap_data);
}

static bool
servant_is_ready (const void *ap_obj)
{
  const tiz_srv_t *p_obj = ap_obj;
  return (tiz_pqueue_length (p_obj->p_pq_) > 0 ? OMX_TRUE : OMX_FALSE);
}

bool
tiz_srv_is_ready (const void *ap_obj)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->is_ready);
  return class->is_ready (ap_obj);
}

bool
tiz_srv_super_is_ready (const void *a_class, const void *ap_obj)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->is_ready);
  return superclass->is_ready (ap_obj);
}

static OMX_ERRORTYPE
servant_allocate_resources (const void *ap_obj, OMX_U32 a_pid)
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

static OMX_ERRORTYPE
servant_deallocate_resources (void *ap_obj)
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

static OMX_ERRORTYPE
servant_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
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
tiz_srv_super_prepare_to_transfer (const void *a_class,
                                      const void *ap_obj, OMX_U32 a_pid)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->prepare_to_transfer);
  return superclass->prepare_to_transfer (ap_obj, a_pid);
}

static OMX_ERRORTYPE
servant_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
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
tiz_srv_super_transfer_and_process (const void *a_class,
                                       const void *ap_obj, OMX_U32 a_pid)
{
  const tiz_srv_class_t *superclass = super (a_class);
  assert (NULL != ap_obj && NULL != superclass->transfer_and_process);
  return superclass->transfer_and_process (ap_obj, a_pid);
}

static OMX_ERRORTYPE
servant_stop_and_return (void *ap_obj)
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


static void
servant_issue_event (const void *ap_obj, OMX_EVENTTYPE a_event,
                     OMX_U32 a_data1, OMX_U32 a_data2,
                     /*@null@*/ OMX_PTR ap_eventdata)
{
  tiz_srv_t *p_obj = (tiz_srv_t *) ap_obj;
  assert (NULL != p_obj);
  assert (NULL != p_obj->p_cbacks_);
  assert (NULL != p_obj->p_cbacks_->EventHandler);
  /* NOTE: Start ignoring splint warnings in this section of code */
  /*@ignore@*/
  TIZ_NOTICE (handleOf (ap_obj), "[%s]", tiz_evt_to_str (a_event));
  (void) p_obj->p_cbacks_->EventHandler (handleOf (ap_obj),
                                         p_obj->p_appdata_,
                                         a_event, a_data1, a_data2, ap_eventdata);
  /*@end@*/
  /* NOTE: Stop ignoring splint warnings in this section  */
}

void
tiz_srv_issue_event (const void *ap_obj, OMX_EVENTTYPE a_event,
                        OMX_U32 a_data1, OMX_U32 a_data2, OMX_PTR ap_eventdata)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->issue_event);
  class->issue_event (ap_obj, a_event, a_data1, a_data2, ap_eventdata);
}

static void
servant_issue_err_event (const void *ap_obj, OMX_ERRORTYPE a_error)
{
  tiz_srv_t *p_obj = (tiz_srv_t *) ap_obj;
  assert (NULL != p_obj);
  TIZ_ERROR (handleOf (ap_obj),
            "OMX_EventError...[%s]", tiz_err_to_str (a_error));
  servant_issue_event (ap_obj, OMX_EventError, a_error, 0, 0);
}

void
tiz_srv_issue_err_event (const void *ap_obj, OMX_ERRORTYPE a_error)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->issue_err_event);
  class->issue_err_event (ap_obj, a_error);
}

static void
servant_issue_cmd_event (const void *ap_obj, OMX_COMMANDTYPE a_cmd,
                         OMX_U32 a_pid, OMX_ERRORTYPE a_error)
{
  tiz_srv_t *p_obj = (tiz_srv_t *) ap_obj;
  assert (NULL != p_obj);
  TIZ_NOTICE (handleOf (ap_obj),
            "OMX_EventCmdComplete...[%s] pid [%d] error [%s]",
            tiz_cmd_to_str (a_cmd), a_pid, tiz_err_to_str (a_error));
  servant_issue_event (ap_obj, OMX_EventCmdComplete, a_cmd, a_pid,
                       (OMX_PTR) a_error);
}

void
tiz_srv_issue_cmd_event (const void *ap_obj, OMX_COMMANDTYPE a_cmd,
                            OMX_U32 a_pid, OMX_ERRORTYPE a_error)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->issue_cmd_event);
  class->issue_cmd_event (ap_obj, a_cmd, a_pid, a_error);
}

static void
servant_issue_trans_event (void *ap_obj, OMX_STATETYPE a_state,
                           OMX_ERRORTYPE a_error)
{
  tiz_srv_t *p_obj = (tiz_srv_t *) ap_obj;
  assert (NULL != p_obj);
  TIZ_NOTICE (handleOf (ap_obj),
            "OMX_EventCmdComplete...[OMX_CommandStateSet] [%s]",
            tiz_fsm_state_to_str (a_state));
  servant_issue_event (ap_obj, OMX_EventCmdComplete, OMX_CommandStateSet,
                       a_state, (OMX_PTR) a_error);
}

void
tiz_srv_issue_trans_event (const void *ap_obj, OMX_STATETYPE a_state,
                              OMX_ERRORTYPE a_error)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->issue_trans_event);
  class->issue_trans_event (ap_obj, a_state, a_error);
}

static void
servant_issue_buf_callback (const void *ap_obj,
                            OMX_BUFFERHEADERTYPE * p_hdr,
                            OMX_U32 pid,
                            OMX_DIRTYPE dir, OMX_HANDLETYPE ap_tcomp)
{
  tiz_srv_t *p_obj = (tiz_srv_t *) ap_obj;
  assert (NULL != p_obj);
  assert (NULL != p_obj->p_cbacks_);
  assert (NULL != p_obj->p_cbacks_->EventHandler);
  if (ap_tcomp)
    {
      if (OMX_DirInput == dir)
        {
          TIZ_DEBUG (handleOf (ap_obj), "[OMX_FillThisBuffer] : "
                     "HEADER [%p] BUFFER [%p] ap_tcomp [%p]",
                     p_hdr, p_hdr->pBuffer, ap_tcomp);
          (void) OMX_FillThisBuffer (ap_tcomp, p_hdr);
        }
      else
        {
          TIZ_DEBUG (handleOf (ap_obj), "[OMX_EmptyThisBuffer] : "
                     "HEADER [%p] BUFFER [%p] ap_tcomp [%p]",
                     p_hdr, p_hdr->pBuffer, ap_tcomp);
          (void) OMX_EmptyThisBuffer (ap_tcomp, p_hdr);
        }
    }

  else
    {
      OMX_ERRORTYPE (*fp_buf_done)
        (OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE *) =
        (dir == OMX_DirInput ?
         p_obj->p_cbacks_->EmptyBufferDone : p_obj->p_cbacks_->FillBufferDone);

      TIZ_DEBUG (handleOf (ap_obj), "[%] : HEADER [%p] BUFFER [%p]",
                 dir == OMX_DirInput ? "EmptyBufferDone" : "FillBufferDone",
                 p_hdr, p_hdr->pBuffer);

      (void) fp_buf_done (handleOf (ap_obj), p_obj->p_appdata_, p_hdr);
    }

}

void
tiz_srv_issue_buf_callback (const void *ap_obj,
                               OMX_BUFFERHEADERTYPE * p_hdr,
                               OMX_U32 pid,
                               OMX_DIRTYPE dir, OMX_HANDLETYPE ap_tcomp)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->issue_buf_callback);
  class->issue_buf_callback (ap_obj, p_hdr, pid, dir, ap_tcomp);
}

static OMX_ERRORTYPE
servant_receive_pluggable_event (const void *ap_obj,
                                 OMX_HANDLETYPE ap_hdl, tiz_event_pluggable_t * ap_event)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_srv_receive_pluggable_event (const void *ap_obj,
                                    OMX_HANDLETYPE ap_hdl,
                                    tiz_event_pluggable_t * ap_event)
{
  const tiz_srv_class_t *class = classOf (ap_obj);
  assert (NULL != class->receive_pluggable_event);
  return class->receive_pluggable_event (ap_obj, ap_hdl, ap_event);
}


/*
 * tiz_srv_class
 */

static void *
servant_class_ctor (void *ap_obj, va_list * app)
{
  tiz_srv_class_t *p_obj = super_ctor (typeOf (ap_obj, "tizsrv_class"), ap_obj, app);
  typedef void (*voidf) ();
  voidf selector = NULL;
  va_list ap;
  va_copy (ap, *app);

  /* NOTE: Start ignoring splint warnings in this section of code */
  /*@ignore@*/
  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) tiz_srv_set_allocator)
        {
          *(voidf *) & p_obj->set_allocator = method;
        }
      else if (selector == (voidf) tiz_srv_set_callbacks)
        {
          *(voidf *) & p_obj->set_callbacks = method;
        }
      else if (selector == (voidf) tiz_srv_tick)
        {
          *(voidf *) & p_obj->tick = method;
        }
      else if (selector == (voidf) tiz_srv_init_msg)
        {
          *(voidf *) & p_obj->init_msg = method;
        }
      else if (selector == (voidf) tiz_srv_enqueue)
        {
          *(voidf *) & p_obj->enqueue = method;
        }
      else if (selector == (voidf) tiz_srv_remove_from_queue)
        {
          *(voidf *) & p_obj->remove_from_queue = method;
        }
      else if (selector == (voidf) tiz_srv_dispatch_msg)
        {
          *(voidf *) & p_obj->dispatch_msg = method;
        }
      else if (selector == (voidf) tiz_srv_is_ready)
        {
          *(voidf *) & p_obj->is_ready = method;
        }
      else if (selector == (voidf) tiz_srv_allocate_resources)
        {
          *(voidf *) & p_obj->allocate_resources = method;
        }
      else if (selector == (voidf) tiz_srv_deallocate_resources)
        {
          *(voidf *) & p_obj->deallocate_resources = method;
        }
      else if (selector == (voidf) tiz_srv_prepare_to_transfer)
        {
          *(voidf *) & p_obj->prepare_to_transfer = method;
        }
      else if (selector == (voidf) tiz_srv_transfer_and_process)
        {
          *(voidf *) & p_obj->transfer_and_process = method;
        }
      else if (selector == (voidf) tiz_srv_stop_and_return)
        {
          *(voidf *) & p_obj->stop_and_return = method;
        }
      else if (selector == (voidf) tiz_srv_issue_event)
        {
          *(voidf *) & p_obj->issue_event = method;
        }
      else if (selector == (voidf) tiz_srv_issue_err_event)
        {
          *(voidf *) & p_obj->issue_err_event = method;
        }
      else if (selector == (voidf) tiz_srv_issue_cmd_event)
        {
          *(voidf *) & p_obj->issue_cmd_event = method;
        }
      else if (selector == (voidf) tiz_srv_issue_trans_event)
        {
          *(voidf *) & p_obj->issue_trans_event = method;
        }
      else if (selector == (voidf) tiz_srv_issue_buf_callback)
        {
          *(voidf *) & p_obj->issue_buf_callback = method;
        }
      else if (selector == (voidf) tiz_srv_receive_pluggable_event)
        {
          *(voidf *) & p_obj->receive_pluggable_event = method;
        }

    }
  /*@end@*/
  /* NOTE: Stop ignoring splint warnings in this section  */

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

void *
tiz_srv_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizapi = tiz_get_type (ap_hdl, "tizapi");
  void * tizsrv_class = factory_new (classOf (tizapi),
                                     "tizsrv_class",
                                     classOf (tizapi),
                                     sizeof (tiz_srv_class_t),
                                     ap_tos, ap_hdl,
                                     ctor, servant_class_ctor, 0);
  return tizsrv_class;
}

void *
tiz_srv_init (void * ap_tos, void * ap_hdl)
{
  void * tizapi       = tiz_get_type (ap_hdl, "tizapi");
  void * tizsrv_class = tiz_get_type (ap_hdl, "tizsrv_class");
  TIZ_LOG_CLASS (tizsrv_class);
  void * tizsrv =
    factory_new
    (tizsrv_class,
     "tizsrv",
     tizapi,
     sizeof (tiz_srv_t),
     ap_tos, ap_hdl,
     ctor, servant_ctor,
     dtor, servant_dtor,
     tiz_srv_set_allocator, servant_set_allocator,
     tiz_srv_set_callbacks, servant_set_callbacks,
     tiz_srv_tick, servant_tick,
     tiz_srv_init_msg, servant_init_msg,
     tiz_srv_enqueue, servant_enqueue,
     tiz_srv_remove_from_queue, servant_remove_from_queue,
     tiz_srv_dispatch_msg, servant_dispatch_msg,
     tiz_srv_is_ready, servant_is_ready,
     tiz_srv_allocate_resources, servant_allocate_resources,
     tiz_srv_deallocate_resources, servant_deallocate_resources,
     tiz_srv_prepare_to_transfer, servant_prepare_to_transfer,
     tiz_srv_transfer_and_process, servant_transfer_and_process,
     tiz_srv_stop_and_return, servant_stop_and_return,
     tiz_srv_issue_event, servant_issue_event,
     tiz_srv_issue_err_event, servant_issue_err_event,
     tiz_srv_issue_cmd_event, servant_issue_cmd_event,
     tiz_srv_issue_trans_event, servant_issue_trans_event,
     tiz_srv_issue_buf_callback, servant_issue_buf_callback,
     tiz_srv_receive_pluggable_event,
     servant_receive_pluggable_event, 0);

  return tizsrv;
}
