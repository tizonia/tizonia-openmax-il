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

#include <assert.h>

#include "tizservant.h"
#include "tizservant_decls.h"
#include "tizscheduler.h"

#include "tizosal.h"
#include "tizutils.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.servant"
#endif

/*
 * tizservant
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
  struct tizservant *p_obj = super_ctor (tizservant, ap_obj, app);
  /* NOTE: The priority queue is initialised only when the allocator is set via
     set_allocator */
  p_obj->p_pq_ = NULL;
  p_obj->p_soa_ = NULL;
  p_obj->p_hdl_ = va_arg (*app, OMX_COMPONENTTYPE *);
  p_obj->p_appdata_ = NULL;
  p_obj->p_cbacks_ = NULL;
  return p_obj;
}

static void *
servant_dtor (void *ap_obj)
{
  struct tizservant *p_obj = ap_obj;

  if (NULL != p_obj->p_pq_)
    {
      OMX_PTR p_msg = NULL;
      int qlen = tiz_pqueue_length (p_obj->p_pq_);

      /* Make sure we clean up any remaining items in the queue */
      while (0 < (qlen = tiz_pqueue_length (p_obj->p_pq_)))
        {
          if (OMX_ErrorNone != tiz_pqueue_receive (p_obj->p_pq_, &p_msg))
            {
              break;
            }
          tiz_soa_free (p_obj->p_soa_, p_msg);
        }

      tiz_pqueue_destroy (p_obj->p_pq_);
    }

  return super_dtor (tizservant, ap_obj);
}

static void
servant_set_allocator (void *ap_obj, tiz_soa_t *p_soa)
{
  struct tizservant *p_obj = ap_obj;
  p_obj->p_soa_ = p_soa;
  /* TODO: Check ret code */
  tiz_pqueue_init (&p_obj->p_pq_, 5, &pqueue_cmp, p_soa, nameOf (ap_obj));
}

void
tizservant_set_allocator (void *ap_obj, tiz_soa_t *p_soa)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->set_allocator);
  class->set_allocator (ap_obj, p_soa);
}

static void
servant_set_callbacks (void *ap_obj,
                           OMX_PTR ap_appdata, OMX_CALLBACKTYPE * ap_cbacks)
{
  struct tizservant *p_obj = ap_obj;
  p_obj->p_appdata_ = ap_appdata;
  p_obj->p_cbacks_ = ap_cbacks;
}

void
tizservant_set_callbacks (void *ap_obj, OMX_PTR ap_appdata,
                          OMX_CALLBACKTYPE * ap_cbacks)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->set_callbacks);
  class->set_callbacks (ap_obj, ap_appdata, ap_cbacks);
}

static OMX_ERRORTYPE
servant_tick (const void *ap_obj)
{
  struct tizservant *p_obj = (struct tizservant *) ap_obj;
  OMX_PTR p_msg = NULL;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;

  for (;;)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Receiving msgs : queue length [%d]...",
                 tiz_pqueue_length (p_obj->p_pq_));
      if (OMX_ErrorNone !=
          (ret_val = tiz_pqueue_receive (p_obj->p_pq_, &p_msg)))
        {
          TIZ_LOG (TIZ_LOG_TRACE, "tiz_pqueue_receive error [%s]...",
                     tiz_err_to_str (ret_val));
          break;
        }

      /* Process the message */
      ret_val = tizservant_dispatch_msg (p_obj, p_msg);

      /* We are done with this message */
      tiz_soa_free (p_obj->p_soa_, p_msg);

      if (OMX_ErrorNone != ret_val)
        {
          TIZ_LOG (TIZ_LOG_TRACE, "tizservant_dispatch_msg error [%s]...",
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
      tizservant_issue_err_event (p_obj, ret_val);
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizservant_tick (const void *ap_obj)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->tick);
  return class->tick (ap_obj);
}

OMX_ERRORTYPE
tizservant_super_tick (const void *a_class, const void *ap_obj)
{
  const struct tizservant_class *superclass = super (a_class);

  assert (ap_obj && superclass->tick);
  return superclass->tick (ap_obj);
}

static OMX_PTR
servant_init_msg (void *ap_obj, size_t msg_sz)
{
  struct tizservant *p_obj = ap_obj;
  assert (NULL != p_obj->p_soa_);
  return tiz_soa_calloc (p_obj->p_soa_, msg_sz);
}

OMX_PTR
tizservant_init_msg (void *ap_obj, size_t msg_sz)
{
  const struct tizservant_class *class = classOf (ap_obj);

  assert (class->init_msg);
  return class->init_msg (ap_obj, msg_sz);
}

static OMX_ERRORTYPE
servant_enqueue (const void *ap_obj, OMX_PTR ap_data, OMX_U32 a_priority)
{
  struct tizservant *p_obj = (struct tizservant *) ap_obj;
  return tiz_pqueue_send (p_obj->p_pq_, ap_data, a_priority);
}

OMX_ERRORTYPE
tizservant_enqueue (const void *ap_obj, OMX_PTR ap_data, OMX_U32 a_priority)
{
  const struct tizservant_class *class = classOf (ap_obj);

  assert (class->enqueue);
  return class->enqueue (ap_obj, ap_data, a_priority);
}

OMX_ERRORTYPE
tizservant_super_enqueue (const void *a_class,
                          const void *ap_obj,
                          OMX_PTR ap_data, OMX_U32 a_priority)
{
  const struct tizservant_class *superclass = super (a_class);
  assert (ap_obj && superclass->enqueue);
  return superclass->enqueue (ap_obj, ap_data, a_priority);
}

static OMX_ERRORTYPE
servant_remove_from_queue (const void *ap_obj, tiz_pq_func_f apf_func,
                               OMX_S32 a_data1, OMX_PTR ap_data2)
{
  struct tizservant *p_obj = (struct tizservant *) ap_obj;
  return tiz_pqueue_remove_func (p_obj->p_pq_, apf_func, a_data1, ap_data2);
}

OMX_ERRORTYPE
tizservant_remove_from_queue (const void *ap_obj, tiz_pq_func_f apf_func,
                              OMX_S32 a_data1, OMX_PTR ap_data2)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->remove_from_queue);
  return class->remove_from_queue (ap_obj, apf_func, a_data1, ap_data2);
}

OMX_ERRORTYPE
tizservant_super_remove_from_queue (const void *a_class,
                                    const void *ap_obj,
                                    tiz_pq_func_f apf_func,
                                    OMX_S32 a_data1,
                                    OMX_PTR ap_data2)
{
  const struct tizservant_class *superclass = super (a_class);
  assert (ap_obj && superclass->remove_from_queue);
  return superclass->remove_from_queue (ap_obj, apf_func, a_data1, ap_data2);
}

static OMX_ERRORTYPE
servant_dispatch_msg (const void *ap_obj, OMX_PTR ap_data)
{
  /* This is to be implemented by the children */
  assert(0);
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tizservant_dispatch_msg (const void *ap_obj, OMX_PTR ap_data)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->dispatch_msg);
  return class->dispatch_msg (ap_obj, ap_data);
}

OMX_ERRORTYPE
tizservant_super_dispatch_msg (const void *a_class,
                               const void *ap_obj, OMX_PTR ap_data)
{
  const struct tizservant_class *superclass = super (a_class);

  assert (ap_obj && superclass->dispatch_msg);
  return superclass->dispatch_msg (ap_obj, ap_data);
}

static OMX_HANDLETYPE
servant_get_hdl (const void *ap_obj)
{
  struct tizservant *p_obj = (struct tizservant *) ap_obj;
  return p_obj->p_hdl_;
}

OMX_HANDLETYPE
tizservant_get_hdl (const void *ap_obj)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->get_hdl);
  return class->get_hdl (ap_obj);
}

OMX_HANDLETYPE
tizservant_super_get_hdl (const void *a_class, const void *ap_obj)
{
  const struct tizservant_class *superclass = super (a_class);

  assert (ap_obj && superclass->get_hdl);
  return superclass->get_hdl (ap_obj);
}

static OMX_BOOL
servant_is_ready (const void *ap_obj)
{
  const struct tizservant *p_obj = ap_obj;
  return (tiz_pqueue_length (p_obj->p_pq_) > 0 ? OMX_TRUE : OMX_FALSE);
}

OMX_BOOL
tizservant_is_ready (const void *ap_obj)
{
  const struct tizservant_class *class = classOf (ap_obj);

  assert (class->is_ready);
  return class->is_ready (ap_obj);
}

OMX_BOOL
tizservant_super_is_ready (const void *a_class, const void *ap_obj)
{
  const struct tizservant_class *superclass = super (a_class);

  assert (ap_obj && superclass->is_ready);
  return superclass->is_ready (ap_obj);
}

static OMX_ERRORTYPE
servant_allocate_resources (const void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tizservant_allocate_resources (const void *ap_obj, OMX_U32 a_pid)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->allocate_resources);
  return class->allocate_resources (ap_obj, a_pid);
}

OMX_ERRORTYPE
tizservant_super_allocate_resources (const void *a_class, const void *ap_obj,
                                     OMX_U32 a_pid)
{
  const struct tizservant_class *superclass = super (a_class);
  assert (ap_obj && superclass->allocate_resources);
  return superclass->allocate_resources (ap_obj, a_pid);
}

static OMX_ERRORTYPE
servant_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tizservant_deallocate_resources (const void *ap_obj)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->deallocate_resources);
  return class->deallocate_resources (ap_obj);
}

OMX_ERRORTYPE
tizservant_super_deallocate_resources (const void *a_class, const void *ap_obj)
{
  const struct tizservant_class *superclass = super (a_class);
  assert (ap_obj && superclass->deallocate_resources);
  return superclass->deallocate_resources (ap_obj);
}

static OMX_ERRORTYPE
servant_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tizservant_prepare_to_transfer (const void *ap_obj, OMX_U32 a_pid)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->prepare_to_transfer);
  return class->prepare_to_transfer (ap_obj, a_pid);
}

OMX_ERRORTYPE
tizservant_super_prepare_to_transfer (const void *a_class,
                                      const void *ap_obj, OMX_U32 a_pid)
{
  const struct tizservant_class *superclass = super (a_class);
  assert (ap_obj && superclass->prepare_to_transfer);
  return superclass->prepare_to_transfer (ap_obj, a_pid);
}

static OMX_ERRORTYPE
servant_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tizservant_transfer_and_process (const void *ap_obj, OMX_U32 a_pid)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->transfer_and_process);
  return class->transfer_and_process (ap_obj, a_pid);
}

OMX_ERRORTYPE
tizservant_super_transfer_and_process (const void *a_class,
                                       const void *ap_obj, OMX_U32 a_pid)
{
  const struct tizservant_class *superclass = super (a_class);
  assert (ap_obj && superclass->transfer_and_process);
  return superclass->transfer_and_process (ap_obj, a_pid);
}

static OMX_ERRORTYPE
servant_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tizservant_stop_and_return (const void *ap_obj)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->stop_and_return);
  return class->stop_and_return (ap_obj);
}

OMX_ERRORTYPE
tizservant_super_stop_and_return (const void *a_class, const void *ap_obj)
{
  const struct tizservant_class *superclass = super (a_class);
  assert (ap_obj && superclass->stop_and_return);
  return superclass->stop_and_return (ap_obj);
}


static void
servant_issue_event (const void *ap_obj, OMX_EVENTTYPE a_event,
                         OMX_U32 a_data1, OMX_U32 a_data2,
                         OMX_PTR ap_eventdata)
{
  struct tizservant *p_obj = (struct tizservant *) ap_obj;
  assert (p_obj);
  assert (p_obj->p_cbacks_);
  assert (p_obj->p_cbacks_->EventHandler);
  p_obj->p_cbacks_->EventHandler (p_obj->p_hdl_,
                                 p_obj->p_appdata_,
                                 a_event, a_data1, a_data2, ap_eventdata);
}

void
tizservant_issue_event (const void *ap_obj, OMX_EVENTTYPE a_event,
                        OMX_U32 a_data1, OMX_U32 a_data2,
                        OMX_PTR ap_eventdata)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->issue_event);
  class->issue_event (ap_obj, a_event, a_data1, a_data2, ap_eventdata);
}

static void
servant_issue_err_event (const void *ap_obj, OMX_ERRORTYPE a_error)
{
  struct tizservant *p_obj = (struct tizservant *) ap_obj;
  assert (p_obj);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(p_obj->p_hdl_),
                 TIZ_CBUF(p_obj->p_hdl_),
                 "OMX_EventError...[%s]", tiz_err_to_str (a_error));
  servant_issue_event (ap_obj, OMX_EventError, a_error, 0, 0);
}

void
tizservant_issue_err_event (const void *ap_obj, OMX_ERRORTYPE a_error)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->issue_err_event);
  class->issue_err_event (ap_obj, a_error);
}

static void
servant_issue_cmd_event (const void *ap_obj, OMX_COMMANDTYPE a_cmd,
                             OMX_U32 a_pid, OMX_ERRORTYPE a_error)
{
  struct tizservant *p_obj = (struct tizservant *) ap_obj;
  assert (p_obj);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(p_obj->p_hdl_),
                 TIZ_CBUF(p_obj->p_hdl_),
                 "OMX_EventCmdComplete...[%s] pid [%d]",
                 tiz_cmd_to_str (a_cmd), a_pid);
  servant_issue_event (ap_obj, OMX_EventCmdComplete, a_cmd, a_pid,
                           (OMX_PTR) a_error);
}

void
tizservant_issue_cmd_event (const void *ap_obj, OMX_COMMANDTYPE a_cmd,
                            OMX_U32 a_pid, OMX_ERRORTYPE a_error)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->issue_cmd_event);
  class->issue_cmd_event (ap_obj, a_cmd, a_pid, a_error);
}

static void
servant_issue_trans_event (void *ap_obj, OMX_STATETYPE a_state,
                               OMX_ERRORTYPE a_error)
{
  struct tizservant *p_obj = (struct tizservant *) ap_obj;
  assert (p_obj);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(p_obj->p_hdl_),
                 TIZ_CBUF(p_obj->p_hdl_),
                 "OMX_EventCmdComplete...[OMX_CommandStateSet] [%s]",
                 tiz_fsm_state_to_str (a_state));

  servant_issue_event (ap_obj, OMX_EventCmdComplete, OMX_CommandStateSet,
                           a_state, (OMX_PTR) a_error);
}

void
tizservant_issue_trans_event (const void *ap_obj, OMX_STATETYPE a_state,
                              OMX_ERRORTYPE a_error)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->issue_trans_event);
  class->issue_trans_event (ap_obj, a_state, a_error);
}

static void
servant_issue_buf_callback (const void *ap_obj,
                                OMX_BUFFERHEADERTYPE * p_hdr,
                                OMX_U32 pid,
                                OMX_DIRTYPE dir, OMX_HANDLETYPE ap_tcomp)
{
  struct tizservant *p_obj = (struct tizservant *) ap_obj;
  assert (p_obj);
  assert (p_obj->p_cbacks_);
  assert (p_obj->p_cbacks_->EventHandler);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(p_obj->p_hdl_),
                 TIZ_CBUF(p_obj->p_hdl_),
                 "HEADER [%p] BUFFER [%p] ap_tcomp [%p]",
                 p_hdr, p_hdr->pBuffer, ap_tcomp);

  if (ap_tcomp)
    {
      if (OMX_DirInput == dir)
        {
          TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(p_obj->p_hdl_),
                         TIZ_CBUF(p_obj->p_hdl_), "OMX_FillThisBuffer");
          OMX_FillThisBuffer (ap_tcomp, p_hdr);
        }
      else
        {
          TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME(p_obj->p_hdl_),
                         TIZ_CBUF(p_obj->p_hdl_), "OMX_EmptyThisBuffer");
          OMX_EmptyThisBuffer (ap_tcomp, p_hdr);
        }
    }

  else
    {
      OMX_ERRORTYPE (*fp_buf_done)
        (OMX_HANDLETYPE, OMX_PTR, OMX_BUFFERHEADERTYPE *) =
        (dir == OMX_DirInput ?
         p_obj->p_cbacks_->EmptyBufferDone : p_obj->p_cbacks_->FillBufferDone);

      fp_buf_done (p_obj->p_hdl_, p_obj->p_appdata_, p_hdr);
    }

}

void
tizservant_issue_buf_callback (const void *ap_obj,
                               OMX_BUFFERHEADERTYPE * p_hdr,
                               OMX_U32 pid,
                               OMX_DIRTYPE dir, OMX_HANDLETYPE ap_tcomp)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->issue_buf_callback);
  class->issue_buf_callback (ap_obj, p_hdr, pid, dir, ap_tcomp);
}

static OMX_ERRORTYPE
servant_receive_pluggable_event (const void *ap_obj,
                                     OMX_HANDLETYPE ap_hdl,
                                     tizevent_t * ap_event)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tizservant_receive_pluggable_event (const void *ap_obj,
                                    OMX_HANDLETYPE ap_hdl,
                                    tizevent_t * ap_event)
{
  const struct tizservant_class *class = classOf (ap_obj);
  assert (class->receive_pluggable_event);
  class->receive_pluggable_event (ap_obj, ap_hdl, ap_event);
  return OMX_ErrorNone;
}


/*
 * tizservant_class
 */

static void *
servant_class_ctor (void *ap_obj, va_list * app)
{
  struct tizservant_class *p_obj = super_ctor (tizservant_class, ap_obj, app);
  typedef void (*voidf) ();
  voidf selector;
  va_list ap;
  va_copy(ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) tizservant_set_allocator)
        {
          *(voidf *) & p_obj->set_allocator = method;
        }
      else if (selector == (voidf) tizservant_set_callbacks)
        {
          *(voidf *) & p_obj->set_callbacks = method;
        }
      else if (selector == (voidf) tizservant_tick)
        {
          *(voidf *) & p_obj->tick = method;
        }
      else if (selector == (voidf) tizservant_init_msg)
        {
          *(voidf *) & p_obj->init_msg = method;
        }
      else if (selector == (voidf) tizservant_enqueue)
        {
          *(voidf *) & p_obj->enqueue = method;
        }
      else if (selector == (voidf) tizservant_remove_from_queue)
        {
          *(voidf *) & p_obj->remove_from_queue = method;
        }
      else if (selector == (voidf) tizservant_dispatch_msg)
        {
          *(voidf *) & p_obj->dispatch_msg = method;
        }
      else if (selector == (voidf) tizservant_get_hdl)
        {
          *(voidf *) & p_obj->get_hdl = method;
        }
      else if (selector == (voidf) tizservant_is_ready)
        {
          *(voidf *) & p_obj->is_ready = method;
        }
      else if (selector == (voidf) tizservant_allocate_resources)
        {
          *(voidf *) & p_obj->allocate_resources = method;
        }
      else if (selector == (voidf) tizservant_deallocate_resources)
        {
          *(voidf *) & p_obj->deallocate_resources = method;
        }
      else if (selector == (voidf) tizservant_prepare_to_transfer)
        {
          *(voidf *) & p_obj->prepare_to_transfer = method;
        }
      else if (selector == (voidf) tizservant_transfer_and_process)
        {
          *(voidf *) & p_obj->transfer_and_process = method;
        }
      else if (selector == (voidf) tizservant_stop_and_return)
        {
          *(voidf *) & p_obj->stop_and_return = method;
        }
      else if (selector == (voidf) tizservant_issue_event)
        {
          *(voidf *) & p_obj->issue_event = method;
        }
      else if (selector == (voidf) tizservant_issue_err_event)
        {
          *(voidf *) & p_obj->issue_err_event = method;
        }
      else if (selector == (voidf) tizservant_issue_cmd_event)
        {
          *(voidf *) & p_obj->issue_cmd_event = method;
        }
      else if (selector == (voidf) tizservant_issue_trans_event)
        {
          *(voidf *) & p_obj->issue_trans_event = method;
        }
      else if (selector == (voidf) tizservant_issue_buf_callback)
        {
          *(voidf *) & p_obj->issue_buf_callback = method;
        }
      else if (selector == (voidf) tizservant_receive_pluggable_event)
        {
          *(voidf *) & p_obj->receive_pluggable_event = method;
        }

    }

  va_end(ap);
  return p_obj;
}

/*
 * initialization
 */

const void *tizservant, *tizservant_class;

void
init_tizservant (void)
{

  if (!tizservant_class)
    {
      init_tizapi ();
      tizservant_class = factory_new (tizapi_class,
                                      "tizservant_class",
                                      tizapi_class,
                                      sizeof (struct tizservant_class),
                                      ctor, servant_class_ctor, 0);

    }

  if (!tizservant)
    {
      init_tizapi ();
      tizservant =
        factory_new
        (tizservant_class,
         "tizservant",
         tizapi,
         sizeof (struct tizservant),
         ctor, servant_ctor,
         dtor, servant_dtor,
         tizservant_set_allocator, servant_set_allocator,
         tizservant_set_callbacks, servant_set_callbacks,
         tizservant_tick, servant_tick,
         tizservant_init_msg, servant_init_msg,
         tizservant_enqueue, servant_enqueue,
         tizservant_remove_from_queue, servant_remove_from_queue,
         tizservant_dispatch_msg, servant_dispatch_msg,
         tizservant_get_hdl, servant_get_hdl,
         tizservant_is_ready, servant_is_ready,
         tizservant_allocate_resources, servant_allocate_resources,
         tizservant_deallocate_resources, servant_deallocate_resources,
         tizservant_prepare_to_transfer, servant_prepare_to_transfer,
         tizservant_transfer_and_process, servant_transfer_and_process,
         tizservant_stop_and_return, servant_stop_and_return,
         tizservant_issue_event, servant_issue_event,
         tizservant_issue_err_event, servant_issue_err_event,
         tizservant_issue_cmd_event, servant_issue_cmd_event,
         tizservant_issue_trans_event, servant_issue_trans_event,
         tizservant_issue_buf_callback, servant_issue_buf_callback,
         tizservant_receive_pluggable_event,
         servant_receive_pluggable_event, 0);
    }

}
