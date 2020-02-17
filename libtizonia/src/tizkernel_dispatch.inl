/* -*-Mode: c; -*- */
/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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
 * @file   tizkernel_dispatch.inl
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - kernel's dispatch functions
 *
 * @remark This file is meant to be included in the main tizkernel.c module to
 * create a single compilation unit.
 *
 */

#ifndef TIZKERNEL_DISPATCH_INL
#define TIZKERNEL_DISPATCH_INL

static OMX_ERRORTYPE dispatch_port_disable (
    void *ap_obj, OMX_HANDLETYPE p_hdl, tiz_krn_msg_sendcommand_t *ap_msg_sc)
{
  tiz_krn_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_U32 pid = 0;
  OMX_S32 nports = 0;
  OMX_S32 i = 0;
  OMX_S32 nbufs = 0;

  assert (p_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  assert (ap_msg_sc);
  pid = ap_msg_sc->param1;

  TIZ_TRACE (p_hdl, "Port Disable on port [%d] ", pid);

  /* Verify the port index.. */
  if ((OMX_ALL != pid) && (check_pid (p_obj, pid) != OMX_ErrorNone))
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Record here the number of times we need to notify the IL client */
  p_obj->cmd_completion_count_ = (OMX_ALL == ap_msg_sc->param1) ? nports : 1;

  do
    {
      pid = ((OMX_ALL != ap_msg_sc->param1) ? ap_msg_sc->param1 : i);
      p_port = get_port (p_obj, pid);

      /* If port is already disabled, simply notify the command completion */
      if (TIZ_PORT_IS_DISABLED (p_port))
        {
          tiz_check_omx (
              complete_port_disable (p_obj, p_port, pid, OMX_ErrorNone));
          ++i;
          continue;
        }

      /* This will move this port's ETB or FTB messages currently queued in the
       * kernel's servant queue into the corresponding port ingress list. This
       * guarantees that all buffers received by the component on this port are
       * correctly returned during port stop */
      tiz_srv_remove_from_queue (ap_obj, &process_efb_from_servant_queue, i,
                                 p_obj);
      /* This will move this port's processor callbacks currently queued in the
       * kernel's servant queue into the corresponding port egress list. This
       * guarantees that all buffers held by the component on this port are
       * correctly returned during port stop */
      tiz_srv_remove_from_queue (ap_obj, &process_cbacks_from_servant_queue, i,
                                 p_obj);

      if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
        {
          /* Move buffers from egress to ingress */
          nbufs = move_to_ingress (p_obj, pid);
          if (nbufs < 0)
            {
              TIZ_ERROR (p_hdl,
                         "[OMX_ErrorInsufficientResources] : "
                         "on port [%d] while moving buffers to ingress list",
                         pid);
              return OMX_ErrorInsufficientResources;
            }

          if (tiz_port_buffer_count (p_port) != nbufs)
            {
              /* Some of the buffers aren't back yet */
              TIZ_PORT_SET_GOING_TO_DISABLED (p_port);
            }
          else
            {
              tiz_vector_t *p_hdr_lst = tiz_port_get_hdrs_list (p_port);
              tiz_vector_t *p_hdr_lst_copy = NULL;
              tiz_check_omx (tiz_vector_init (
                  &(p_hdr_lst_copy), sizeof(OMX_BUFFERHEADERTYPE *)));
              assert (p_hdr_lst_copy);
              tiz_check_omx (tiz_vector_append (p_hdr_lst_copy, p_hdr_lst));

              /* Depopulate the tunnel... */
              if (OMX_ErrorNone == (rc = tiz_port_depopulate (p_port)))
                {
                  const OMX_S32 nhdrs = tiz_vector_length (p_hdr_lst_copy);
                  OMX_S32 j = 0;
                  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;

                  for (j = 0; j < nhdrs; ++j)
                    {
                      pp_hdr = tiz_vector_at (p_hdr_lst_copy, j);
                      assert (pp_hdr && *pp_hdr);

                      TIZ_TRACE (p_hdl,
                                 "port [%d] depopulated - "
                                 "removing leftovers - nhdrs [%d] "
                                 "HEADER [%p] BUFFER [%p]...",
                                 pid, nhdrs, *pp_hdr, (*pp_hdr)->pBuffer);

                      tiz_srv_remove_from_queue (
                          ap_obj, &remove_buffer_from_servant_queue,
                          ETIZKrnMsgCallback, *pp_hdr);

                      {
                        const void *p_prc = tiz_get_prc (p_hdl);
                        /* NOTE : 2nd and 3rd parameters are dummy ones, the
                         * processor servant implementation of
                         * 'remove_from_queue' will replace them with the right
                         * values */
                        tiz_srv_remove_from_queue (p_prc, NULL, 0, *pp_hdr);
                      }
                    }
                }

              tiz_vector_clear (p_hdr_lst_copy);
              tiz_vector_destroy (p_hdr_lst_copy);

              if (OMX_ErrorNone != rc)
                {
                  TIZ_ERROR (p_hdl, "[%s] depopulating port [%d]",
                             tiz_err_to_str (rc), pid);
                  return rc;
                }

              tiz_check_omx (
                  complete_port_disable (p_obj, p_port, pid, OMX_ErrorNone));
            }
        }
      else
        {
          if (tiz_port_buffer_count (p_port) > 0)
            {
              /* Move headers from ingress to egress, ... */
              /* ....and clear their contents before doing that... */
              const OMX_S32 count = clear_hdr_contents (p_obj->p_ingress_, pid);

              TIZ_PORT_SET_GOING_TO_DISABLED (p_port);

              if (count > 0)
                {
                  if (move_to_egress (p_obj, pid) < 0)
                    {
                      TIZ_ERROR (p_hdl,
                                 "[OMX_ErrorInsufficientResources] : "
                                 "on port [%d]...",
                                 pid);
                      rc = OMX_ErrorInsufficientResources;
                    }
                }

              if (OMX_ErrorNone == rc)
                {
                  /* ... and finally flush them so that they leave the
                   * component ...  */
                  rc = flush_egress (p_obj, pid, OMX_FALSE);
                }

              if (TIZ_PORT_GET_CLAIMED_COUNT (p_port) > 0)
                {
                  /* We need to wait until the processor relinquishes all the
                   * buffers it is currently holding. */

                  TIZ_TRACE (p_hdl,
                             "port [%d] going to disabled - "
                             "claimed [%d]...",
                             pid, TIZ_PORT_GET_CLAIMED_COUNT (p_port));

                  /* Notify the processor servant... */
                  {
                    void *p_prc = tiz_get_prc (p_hdl);
                    rc = tiz_api_SendCommand (p_prc, p_hdl, ap_msg_sc->cmd, pid,
                                              ap_msg_sc->p_cmd_data);
                  }
                }
            }
          else
            {
              TIZ_TRACE (p_hdl, "port [%d] is disabled...", pid);
              tiz_check_omx (
                  complete_port_disable (p_obj, p_port, pid, OMX_ErrorNone));
            }
        }

      ++i;
    }
  while (OMX_ALL == ap_msg_sc->param1 && i < nports);

  return complete_ongoing_transitions (p_obj, p_hdl);
}

static OMX_ERRORTYPE dispatch_port_enable (void *ap_obj, OMX_HANDLETYPE p_hdl,
                                           tiz_krn_msg_sendcommand_t *ap_msg_pe)
{
  tiz_krn_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_U32 pid = 0;
  OMX_S32 nports = 0;
  OMX_S32 i = 0;
  tiz_fsm_state_id_t now = EStateMax;

  assert (p_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  assert (p_hdl);
  now = tiz_fsm_get_substate (tiz_get_fsm (p_hdl));

  assert (ap_msg_pe);
  pid = ap_msg_pe->param1;

  TIZ_TRACE (handleOf (p_obj), "Requested port enable for PORT [%d]", pid);

  /* Verify the port index.. */
  if ((OMX_ALL != pid) && (check_pid (p_obj, pid) != OMX_ErrorNone))
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Record here the number of times we need to notify the IL client */
  p_obj->cmd_completion_count_ = (OMX_ALL == ap_msg_pe->param1) ? nports : 1;

  do
    {
      pid = ((OMX_ALL != ap_msg_pe->param1) ? ap_msg_pe->param1 : i);
      p_port = get_port (p_obj, pid);

      if (TIZ_PORT_IS_ENABLED (p_port))
        {
          /* If port is already enabled, must notify the command completion */
          tiz_check_omx (
              complete_port_enable (p_obj, p_port, pid, OMX_ErrorNone));
          ++i;
          continue;
        }

      if (OMX_ErrorNone == rc)
        {
          if (EStateWaitForResources == now || EStateLoaded == now)
            {
              /* Complete OMX_CommandPortEnable on this port now */
              tiz_check_omx (
                  complete_port_enable (p_obj, p_port, pid, OMX_ErrorNone));
            }
          else
            {
              TIZ_PORT_SET_GOING_TO_ENABLED (p_port);
              if (OMX_ErrorNone
                  == (rc = tiz_srv_allocate_resources (ap_obj, pid)))
                {
                  if (ESubStateLoadedToIdle == now)
                    {
                      if (all_populated (p_obj))
                        {
                          /* Complete transition to OMX_StateIdle */
                          rc = tiz_fsm_complete_transition (
                              tiz_get_fsm (p_hdl), ap_obj, OMX_StateIdle);
                        }
                    }
                  else if (EStateExecuting == now)
                    {
                      if (OMX_ErrorNone == (rc = tiz_srv_prepare_to_transfer (
                                                ap_obj, pid)))
                        {
                          rc = tiz_srv_transfer_and_process (ap_obj, pid);
                        }
                    }
                }
            }
        }

      ++i;
    }
  while (OMX_ALL == ap_msg_pe->param1 && i < nports && OMX_ErrorNone == rc);

  return rc;
}

static OMX_ERRORTYPE dispatch_port_flush (void *ap_obj, OMX_HANDLETYPE ap_hdl,
                                          tiz_krn_msg_sendcommand_t *ap_msg_pf)
{
  tiz_krn_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_U32 pid = 0;
  OMX_S32 nports = 0;
  OMX_S32 i = 0;
  OMX_S32 nbufs = 0;
  tiz_fsm_state_id_t now = EStateMax;

  assert (ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  assert (ap_hdl);
  now = tiz_fsm_get_substate (tiz_get_fsm (ap_hdl));

  assert (ap_msg_pf);
  pid = ap_msg_pf->param1;

  TIZ_TRACE (ap_hdl, "Requested port flush on PORT [%d]", pid);

  /* Verify the port index */
  if ((OMX_ALL != pid) && (check_pid (p_obj, pid) != OMX_ErrorNone))
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Record here the number of times we need to notify the IL client */
  p_obj->cmd_completion_count_ = (OMX_ALL == ap_msg_pf->param1) ? nports : 1;

  /* My Flush matrix */
  /*  |---------------+---------------+---------+--------------------------| */
  /*  | Tunneled/     | Supplier/     | Input/  | Outcome                  | */
  /*  | Non-Tunneled? | Non-Supplier? | Output? |                          | */
  /*  |---------------+---------------+---------+--------------------------| */
  /*  | NT            | S             | I       | Return                   | */
  /*  | NT            | S             | O       | Return + zero nFilledLen | */
  /*  |---------------+---------------+---------+--------------------------| */
  /*  | T             | S             | I       | Return + zero nFilledLen | */
  /*  | T             | S             | O       | Hold + zero nFilledLen   | */
  /*  |---------------+---------------+---------+--------------------------| */
  /*  | NT            | NS            | I       | Return                   | */
  /*  | NT            | NS            | O       | Return + zero nFilledLen | */
  /*  |---------------+---------------+---------+--------------------------| */
  /*  | T             | NS            | I       | Return                   | */
  /*  | T             | NS            | O       | Return + zero nFilledLen | */
  /*  |---------------+---------------+---------+--------------------------| */

  do
    {
      pid = ((OMX_ALL != ap_msg_pf->param1) ? ap_msg_pf->param1 : i);
      p_port = get_port (p_obj, pid);

      if ((tiz_port_buffer_count (p_port) > 0) && TIZ_PORT_IS_ENABLED (p_port)
          && (now == EStateExecuting || now == EStatePause))
        {
          /* This will move this port's ETB or FTB messages currently queued in
           * the
           * kernel's servant queue into the corresponding port ingress list.
           * This
           * guarantees that all buffers received by the component on this port
           * are
           * correctly returned during port flush */
          tiz_srv_remove_from_queue (ap_obj, &process_efb_from_servant_queue, i,
                                     p_obj);

          /* This will move this port's processor callbacks currently queued in
           * the kernel's servant queue into the corresponding port egress
           * list. This guarantees that all buffers held by the component on
           * this port are correctly returned during port flush */
          tiz_srv_remove_from_queue (ap_obj, &process_cbacks_from_servant_queue,
                                     i, p_obj);

          if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
            {
              if (OMX_DirInput == tiz_port_dir (p_port))
                {
                  /* INPUT PORT: Move input headers from ingress to egress,
                   * ... */
                  /* ....and clear their contents before doing that... */
                  const OMX_S32 count
                      = clear_hdr_contents (p_obj->p_ingress_, pid);

                  if (count > 0)
                    {
                      nbufs = move_to_egress (p_obj, pid);
                      if (nbufs < 0)
                        {
                          TIZ_ERROR (ap_hdl,
                                     "[OMX_ErrorInsufficientResources] : "
                                     "on port [%d]...",
                                     pid);
                          rc = OMX_ErrorInsufficientResources;
                        }
                    }

                  if (OMX_ErrorNone == rc)
                    {
                      /* ... and finally flush them so that they go
                       * upstream...  */
                      rc = flush_egress (p_obj, pid, OMX_FALSE);
                    }
                }

              else
                {
                  /* OUTPUT PORT: Move output headers from egress to
                   * ingress... */
                  /* ....and clear their contents before doing that */
                  const OMX_S32 count
                      = clear_hdr_contents (p_obj->p_egress_, pid);
                  if (count > 0)
                    {
                      nbufs = move_to_ingress (p_obj, pid);
                      if (nbufs < 0)
                        {
                          TIZ_ERROR (ap_hdl,
                                     "[OMX_ErrorInsufficientResources] : "
                                     "on port [%d]...",
                                     pid);
                          rc = OMX_ErrorInsufficientResources;
                        }
                    }
                  /* Buffers are kept */
                }
            }

          else
            {
              /* Move (input or output) headers from ingress to egress... */
              /* ....but clear only output headers ... */
              if (OMX_DirInput == tiz_port_dir (p_port))
                {
                  (void)clear_hdr_contents (p_obj->p_egress_, pid);
                }

              nbufs = move_to_egress (p_obj, pid);
              if (nbufs < 0)
                {
                  TIZ_ERROR (ap_hdl,
                             "[OMX_ErrorInsufficientResources] : "
                             "on port [%d]...",
                             pid);
                  rc = OMX_ErrorInsufficientResources;
                }

              if (OMX_ErrorNone == rc)
                {
                  /* ... and finally flush them ...  */
                  rc = flush_egress (p_obj, pid, OMX_FALSE);
                }
            }
        }

      if (OMX_ErrorNone != rc)
        {
          /* Complete the command with an error event */
          TIZ_TRACE (ap_hdl, "[%s] : Flush command failed on port [%d]...",
                     tiz_err_to_str (rc), pid);
          tiz_check_omx (complete_port_flush (p_obj, p_port, pid, rc));
        }
      else
        {
          /* Check if the processor holds a buffer. Will need to wait until any
           * buffers held by the processor are relinquished.  */

          TIZ_TRACE (ap_hdl, "port [%d] claimed_count = [%d]...", pid,
                     TIZ_PORT_GET_CLAIMED_COUNT (p_port));

          if (TIZ_PORT_GET_CLAIMED_COUNT (p_port) == 0)
            {
              /* There are no buffers with the processor, then we can
               * sucessfully complete the OMX_CommandFlush command here. */
              tiz_check_omx (
                  complete_port_flush (p_obj, p_port, pid, OMX_ErrorNone));
            }
          else
            {
              /* We need to wait until the processor relinquishes all the
               * buffers it is currently holding. */
              TIZ_PORT_SET_FLUSH_IN_PROGRESS (p_port);

              /* Notify the processor servant... */
              {
                void *p_prc = tiz_get_prc (ap_hdl);
                rc = tiz_api_SendCommand (p_prc, ap_hdl, ap_msg_pf->cmd, pid,
                                          ap_msg_pf->p_cmd_data);
              }
            }
        }

      ++i;
    }
  while (OMX_ALL == ap_msg_pf->param1 && i < nports);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE dispatch_mark_buffer (void *ap_obj, OMX_HANDLETYPE p_hdl,
                                           tiz_krn_msg_sendcommand_t *ap_msg_sc)
{
  tiz_krn_t *p_obj = ap_obj;
  OMX_PTR p_port = NULL;
  const OMX_U32 pid = ap_msg_sc->param1;
  const OMX_MARKTYPE *p_mark = ap_msg_sc->p_cmd_data;

  /* TODO : Check whether pid can be OMX_ALL. For now assume it can't */

  /* Record here the number of times we need to notify the IL client */
  /*   assert (p_obj->cmd_completion_count_ == 0); */
  /*   p_obj->cmd_completion_count_ = 1; */

  p_port = get_port (p_obj, pid);

  /* Simply enqueue the mark in the port... */
  return tiz_port_store_mark (p_port, p_mark, OMX_TRUE); /* The port owns this
                                                          * mark */
}

static OMX_ERRORTYPE dispatch_cb (void *ap_obj, OMX_PTR ap_msg)
{
  tiz_krn_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_krn_msg_t *p_msg = ap_msg;
  tiz_krn_msg_callback_t *p_msg_cb = NULL;
  tiz_fsm_state_id_t now = (tiz_fsm_state_id_t)OMX_StateMax;
  tiz_vector_t *p_egress_lst = NULL;
  OMX_PTR p_port = NULL;
  OMX_S32 claimed_count = 0;
  OMX_HANDLETYPE p_hdl = NULL;
  OMX_U32 pid = 0;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  assert (p_obj);
  assert (p_msg);

  p_hdl = p_msg->p_hdl;
  assert (p_hdl);

  p_msg_cb = &(p_msg->cb);
  pid = p_msg_cb->pid;
  p_hdr = p_msg_cb->p_hdr;

  now = tiz_fsm_get_substate (tiz_get_fsm (p_hdl));

  TIZ_TRACE (p_hdl, "HEADER [%p] STATE [%s] ", p_hdr,
             tiz_fsm_state_to_str (now));

  /* Find the port.. */
  p_port = get_port (p_obj, pid);

  /* Grab the port's egress list */
  p_egress_lst = get_egress_lst (p_obj, pid);

  /* Buffers are not allowed to leave the component in OMX_StatePause, unless
   * the port is being explicitly flushed by the IL Client. If the port is not
   * being flushed and the component is paused, a dummy callback msg will be
   * added to the queue once the component transitions from OMX_StatePause to
   * OMX_StateExecuting. */
  if (EStatePause == now && !TIZ_PORT_IS_BEING_FLUSHED (p_port))
    {
      TIZ_TRACE (p_hdl, "OMX_StatePause -> Deferring HEADER [%p]", p_hdr);

      /* TODO: Double check whether this hack is needed anymore */
      if (NULL == p_hdr && OMX_DirMax == p_msg_cb->dir)
        {
          TIZ_TRACE (p_hdl, "Enqueueing another dummy callback...");
          rc = enqueue_callback_msg (p_obj, NULL, 0, OMX_DirMax);
        }
      else
        {
          /* ...add the header to the egress list... */
          if (OMX_ErrorNone
              != (rc = tiz_vector_push_back (p_egress_lst, &p_hdr)))
            {
              TIZ_ERROR (p_hdl,
                         "[%s] : Could not add HEADER [%p] "
                         "to port [%d] egress list",
                         tiz_err_to_str (rc), p_hdr, pid);
            }
          else
            {
              /* Now decrement by one the port's claimed buffers count */
              claimed_count = TIZ_PORT_DEC_CLAIMED_COUNT (p_port);
              (void)claimed_count; /* We don't need to use this value. WE do this
                                      to fool static analysers */
            }
        }

      return rc;
    }

  if (NULL == p_hdr && OMX_DirMax == p_msg_cb->dir)
    {
      /* If this is a dummy callback, simply flush the lists and return */
      return flush_egress (p_obj, OMX_ALL, OMX_FALSE);
    }

  /* ...add the header to the egress list... */
  if (OMX_ErrorNone != (rc = tiz_vector_push_back (p_egress_lst, &p_hdr)))
    {
      TIZ_ERROR (p_hdl,
                 "[%s] : Could not add header [%p] to "
                 "port [%d] egress list",
                 tiz_err_to_str (rc), p_hdr, pid);
    }

  if (OMX_ErrorNone == rc)
    {
      /* Now decrement by one the port's claimed buffers count */
      claimed_count = TIZ_PORT_DEC_CLAIMED_COUNT (p_port);

      if ((ESubStateExecutingToIdle == now || ESubStatePauseToIdle == now)
          && TIZ_PORT_IS_ENABLED_TUNNELED_AND_SUPPLIER (p_port))
        {
          int nbufs = 0;
          /* If we are moving to Idle, move the buffers to ingress so they
           * don't leave the component in the next step */
          nbufs = move_to_ingress (p_obj, pid);
          TIZ_TRACE (p_hdl, "nbufs [%d]", nbufs);
        }

      /* Here, we always flush the egress lists for ALL ports */
      if (OMX_ErrorNone != (rc = flush_egress (p_obj, OMX_ALL, OMX_FALSE)))
        {
          TIZ_ERROR (p_hdl, "[%s] : Could not flush the egress lists",
                     tiz_err_to_str (rc));
        }
    }

  /* Check in case we can complete at this point an ongoing flush or disable
   * command or state transition to OMX_StateIdle. */
  if (0 == claimed_count)
    {
      if (TIZ_PORT_IS_BEING_FLUSHED (p_port))
        {
          /* Notify flush complete */
          tiz_check_omx (complete_port_flush (p_obj, p_port, pid, rc));
        }

      if (all_buffers_returned (p_obj))
        {
          if ((ESubStateExecutingToIdle == now || ESubStatePauseToIdle == now)
              && (TIZ_KRN_MAY_INIT_EXE_TO_IDLE (ap_obj)))
            {
              clear_hdr_lsts (ap_obj, OMX_ALL);
              /* complete state transition to OMX_StateIdle */
              rc = tiz_fsm_complete_transition (tiz_get_fsm (p_hdl), p_obj,
                                                OMX_StateIdle);
            }
        }
    }

  return rc;
}

static OMX_ERRORTYPE depopulate_port (tiz_krn_t *ap_krn, OMX_PTR ap_port,
                                      const OMX_U32 a_pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_vector_t *p_hdr_lst = tiz_port_get_hdrs_list (ap_port);
  tiz_vector_t *p_hdr_lst_copy = NULL;

  /* Make a temp copy of the list of headers */
  tiz_check_omx (
      tiz_vector_init (&(p_hdr_lst_copy), sizeof(OMX_BUFFERHEADERTYPE *)));
  assert (p_hdr_lst_copy);

  if (OMX_ErrorNone == (rc = tiz_vector_append (p_hdr_lst_copy, p_hdr_lst)))
    {
      /* All buffers are back... now free headers on the other end */
      if (OMX_ErrorNone == (rc = tiz_port_depopulate (ap_port)))
        {
          const OMX_S32 nhdrs = tiz_vector_length (p_hdr_lst_copy);
          OMX_S32 i = 0;
          OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
          for (i = 0; i < nhdrs; ++i)
            {
              pp_hdr = tiz_vector_at (p_hdr_lst_copy, i);
              assert (pp_hdr && *pp_hdr);

              TIZ_TRACE (handleOf (ap_krn),
                         "port [%d] depopulated - "
                         "removing leftovers - nhdrs [%d] "
                         "HEADER [%p]...",
                         a_pid, nhdrs, *pp_hdr);

              tiz_srv_remove_from_queue (ap_krn,
                                         &remove_buffer_from_servant_queue,
                                         ETIZKrnMsgCallback, *pp_hdr);

              /* NOTE : 2nd and 3rd parameters are dummy ones, the
               * processor servant implementation of
               * 'remove_from_queue' will replace them with the right
               * values */
              tiz_srv_remove_from_queue (tiz_get_prc (handleOf (ap_krn)), NULL,
                                         0, *pp_hdr);
            }
        }
      tiz_vector_clear (p_hdr_lst_copy);
    }

  tiz_vector_destroy (p_hdr_lst_copy);

  return rc;
}

static OMX_ERRORTYPE dispatch_efb_port_disable_in_progress (
    tiz_krn_t *ap_krn, OMX_PTR ap_port, const OMX_U32 a_pid,
    const OMX_S32 a_nbufs)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_krn);
  assert (ap_port);
  assert (TIZ_PORT_IS_BEING_DISABLED (ap_port));

  if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (ap_port))
    {
      if (tiz_port_buffer_count (ap_port) == a_nbufs)
        {
          tiz_check_omx (depopulate_port (ap_krn, ap_port, a_pid));
          tiz_check_omx (
              complete_port_disable (ap_krn, ap_port, a_pid, OMX_ErrorNone));
        }
    }
  else
    {
      /* Bounce this buffer back */
      if (move_to_egress (ap_krn, a_pid) < 0)
        {
          TIZ_ERROR (handleOf (ap_krn),
                     "[OMX_ErrorInsufficientResources] : "
                     "on port [%d]...",
                     a_pid);
          rc = OMX_ErrorInsufficientResources;
        }
      else
        {
          rc = flush_egress (ap_krn, a_pid, OMX_FALSE);
        }
    }

  return rc;
}

static OMX_ERRORTYPE dispatch_efb (void *ap_obj, OMX_PTR ap_msg,
                                   tiz_krn_msg_class_t a_msg_class)
{
  tiz_krn_t *p_obj = ap_obj;
  tiz_krn_msg_t *p_msg = ap_msg;
  tiz_krn_msg_emptyfillbuffer_t *p_msg_ef = NULL;
  tiz_fsm_state_id_t now = EStateMax;
  OMX_PTR p_port = NULL;
  OMX_S32 nbufs = 0;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const OMX_DIRTYPE dir
      = a_msg_class == ETIZKrnMsgEmptyThisBuffer ? OMX_DirInput : OMX_DirOutput;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_U32 pid = 0;
  OMX_HANDLETYPE *p_hdl = NULL;

  assert (p_obj);
  assert (p_msg);

  p_msg_ef = &(p_msg->ef);
  assert (p_msg_ef);

  p_hdr = p_msg_ef->p_hdr;
  assert (p_hdr);

  p_hdl = p_msg->p_hdl;
  assert (p_hdl);

  now = tiz_fsm_get_substate (tiz_get_fsm (p_hdl));

  pid = a_msg_class == ETIZKrnMsgEmptyThisBuffer ? p_hdr->nInputPortIndex
                                                 : p_hdr->nOutputPortIndex;

  TIZ_TRACE (p_hdl, "HEADER [%p] BUFFER [%p] PID [%d]", p_hdr, p_hdr->pBuffer,
             pid);

  if (check_pid (p_obj, pid) != OMX_ErrorNone)
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Retrieve the port... */
  p_port = get_port (p_obj, pid);

  /* Add this buffer to the port's ingress hdr list */
  if (0 > (nbufs = add_to_buflst (p_obj, p_obj->p_ingress_, p_hdr, p_port)))
    {
      TIZ_ERROR (p_hdl,
                 "[OMX_ErrorInsufficientResources] : "
                 "on port [%d] while adding buffer to ingress list",
                 pid);
      return OMX_ErrorInsufficientResources;
    }

  assert (nbufs != 0);

  TIZ_TRACE (p_hdl, "ingress list length [%d]", nbufs);

  if (TIZ_PORT_IS_BEING_DISABLED (p_port))
    {
      return dispatch_efb_port_disable_in_progress (ap_obj, p_port, pid, nbufs);
    }

  if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port)
      && (ESubStateExecutingToIdle == now || ESubStatePauseToIdle == now))
    {
      /* The component is being stopped; check in case that this is the last
         buffer which needed to be returned because then the transition must be
         completed */
      if (all_buffers_returned (p_obj)
          && (TIZ_KRN_MAY_INIT_EXE_TO_IDLE (p_obj)))
        {
          TIZ_DEBUG (p_hdl,
                     "Back to idle "
                     "all buffers returned : [TRUE]");

          clear_hdr_lsts (p_obj, OMX_ALL);
          rc = tiz_fsm_complete_transition (tiz_get_fsm (p_hdl), p_obj,
                                            OMX_StateIdle);
        }
    }
  else
    {
      /* Notify the processor servant, except for when the component is in
         Pause state ... */
      if (EStatePause != now && TIZ_PORT_IS_ENABLED (p_port))
        {
          typedef OMX_ERRORTYPE (*pf_efb_t)(const void *ap_obj,
                                            OMX_HANDLETYPE ap_hdl,
                                            OMX_BUFFERHEADERTYPE *ap_buf);
          pf_efb_t pf_efb = (OMX_DirInput == dir ? tiz_api_EmptyThisBuffer
                                                 : tiz_api_FillThisBuffer);
          rc = pf_efb (tiz_get_prc (p_hdl), p_hdl, p_hdr);
        }
    }
  return rc;
}

static OMX_ERRORTYPE dispatch_etb (void *ap_obj, OMX_PTR ap_msg)
{
  return dispatch_efb (ap_obj, ap_msg, ETIZKrnMsgEmptyThisBuffer);
}

static OMX_ERRORTYPE dispatch_ftb (void *ap_obj, OMX_PTR ap_msg)
{
  return dispatch_efb (ap_obj, ap_msg, ETIZKrnMsgFillThisBuffer);
}

static OMX_ERRORTYPE dispatch_pe (void *ap_obj, OMX_PTR ap_msg)
{
  tiz_krn_msg_t *p_msg = ap_msg;
  tiz_krn_msg_plg_event_t *p_msg_pe = NULL;

  assert (ap_obj);
  assert (p_msg);

  p_msg_pe = &(p_msg->pe);
  assert (p_msg_pe);

  /* TODO : Should this return something? */
  p_msg_pe->p_event->pf_hdlr ((OMX_PTR)ap_obj, p_msg_pe->p_event);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE dispatch_sc (void *ap_obj, OMX_PTR ap_msg)
{
  tiz_krn_t *p_obj = ap_obj;
  tiz_krn_msg_t *p_msg = ap_msg;
  tiz_krn_msg_sendcommand_t *p_msg_sc = NULL;

  assert (p_obj);
  assert (p_msg);

  p_msg_sc = &(p_msg->sc);
  assert (p_msg_sc);
  assert (p_msg_sc->cmd <= OMX_CommandMarkBuffer);

  TIZ_TRACE (handleOf (p_obj), "Processing [%s]...",
             tiz_cmd_to_str (p_msg_sc->cmd));

  return tiz_krn_msg_dispatch_sc_to_fnt_tbl[p_msg_sc->cmd](p_obj, p_msg->p_hdl,
                                                           p_msg_sc);
}

static OMX_ERRORTYPE dispatch_idle_to_loaded (tiz_krn_t *ap_krn,
                                              bool * ap_done)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_done);

  rc = tiz_srv_deallocate_resources (ap_krn);

  if (OMX_ErrorNone == rc)
    {
      rc = release_rm_resources (ap_krn, handleOf(ap_krn));
    }

  if (OMX_ErrorNone == rc)
    {
      /* Uninitialize the Resource Manager hdl */
      rc = deinit_rm (ap_krn, handleOf(ap_krn));
    }

  *ap_done = (OMX_ErrorNone == rc && all_depopulated (ap_krn))
    ? true
    : false;

  /* TODO: If all ports are depopulated, kick off removal of buffer
   * callbacks from kernel and proc servants queues  */

  return rc;
}

static OMX_ERRORTYPE dispatch_loaded_to_idle (tiz_krn_t *ap_krn,
                                              bool * ap_done)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_done);

  /* Before allocating any resources, we need to initialize the
   * Resource Manager hdl */
  rc = init_rm (ap_krn, handleOf(ap_krn));

  if (OMX_ErrorNone == rc)
    {
      rc = acquire_rm_resources (ap_krn, handleOf(ap_krn));
    }

  if (OMX_ErrorNone == rc)
    {
      rc = tiz_srv_allocate_resources (ap_krn, OMX_ALL);
    }

  *ap_done = (OMX_ErrorNone == rc && all_populated (ap_krn)) ? OMX_TRUE
    : false;

  return rc;
}

static OMX_ERRORTYPE dispatch_exe_or_pause_to_idle (tiz_krn_t *ap_krn,
                                              bool * ap_done)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_done);

  rc = tiz_srv_stop_and_return (ap_krn);
  *ap_done = (OMX_ErrorNone == rc
          && all_buffers_returned ((tiz_krn_t *)ap_krn))
    ? true
    : false;
  if (*ap_done)
    {
      clear_hdr_lsts (ap_krn, OMX_ALL);
    }

  return rc;
}

static OMX_ERRORTYPE dispatch_idle_to_exe (tiz_krn_t *ap_krn,
                                           bool * ap_done)
{
  assert (ap_done);
  *ap_done = true;
  return tiz_srv_prepare_to_transfer (ap_krn, OMX_ALL);
}

static OMX_ERRORTYPE dispatch_pause_to_exe (tiz_krn_t *ap_krn,
                                            bool * ap_done)
{
  assert (ap_done);
  *ap_done = true;
  /* Enqueue a dummy callback msg to be processed ...   */
  /* ...in case there are headers present in the egress lists... */
  return enqueue_callback_msg (ap_krn, NULL, 0, OMX_DirMax);
}

static OMX_ERRORTYPE dispatch_exe_to_exe (tiz_krn_t *ap_krn,
                                          bool * ap_done)
{
  assert (ap_done);
  /* NOTE: not done yet here */
  *ap_done = false;
  return tiz_srv_transfer_and_process (ap_krn, OMX_ALL);
}

static OMX_ERRORTYPE dispatch_exe_or_idle_to_pause (tiz_krn_t *ap_krn,
                                                    bool * ap_done)
{
  assert (ap_done);
  *ap_done = true;
  /* TODO: Consider to add here the removal of buffer indications from the
   * processor queue */
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE dispatch_true (tiz_krn_t *ap_krn,
                                    bool * ap_done)
{
  assert (ap_done);
  *ap_done = true;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE dispatch_false (tiz_krn_t *ap_krn,
                                    bool * ap_done)
{
  return OMX_ErrorIncorrectStateTransition;
}

static inline tiz_krn_msg_dispatch_state_set_f
lookup_state_set_transition (const void *ap_obj,
                             const OMX_STATETYPE a_current_state,
                             const OMX_STATETYPE a_next_state)
{
  assert (a_current_state >= OMX_StateLoaded);
  assert (a_current_state <= OMX_StateWaitForResources);
  assert (a_next_state >= OMX_StateLoaded);
  assert (a_next_state <= OMX_StateWaitForResources);

  TIZ_TRACE (handleOf (ap_obj), "Requested transition [%s] -> [%s]",
             tiz_state_to_str (a_current_state), tiz_state_to_str (a_next_state));

  return tiz_krn_state_set_dispatch_tbl[a_current_state][a_next_state];
}

static OMX_ERRORTYPE dispatch_state_set (void *ap_obj, OMX_HANDLETYPE ap_hdl,
                                         tiz_krn_msg_sendcommand_t *ap_msg_sc)
{
  tiz_krn_t *p_krn = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_STATETYPE now = OMX_StateMax;
  OMX_STATETYPE next = OMX_StateMax;
  bool done = false;
  tiz_krn_msg_dispatch_state_set_f p_action_f = NULL;

  assert (ap_obj);
  assert (ap_msg_sc);

  /* Obtain the current state */
  tiz_check_omx (tiz_api_GetState (tiz_get_fsm (ap_hdl), ap_hdl, &now));

  /* ...and this is the next state */
  next =  ap_msg_sc->param1;

  /* find the action */
  p_action_f = lookup_state_set_transition (p_krn, now, next);
  assert (p_action_f);
  rc = p_action_f (p_krn, &done);

  if (OMX_ErrorIncorrectStateTransition == rc)
    {
      TIZ_ERROR (ap_hdl, "unhandled transition : [%s] -> [%s]",
                 tiz_state_to_str (now), tiz_state_to_str (next));
      assert (false && ("Unhandled transition in kernel servant"));
    }

  if (OMX_ErrorNone == rc && done)
    {
      rc = tiz_fsm_complete_transition (tiz_get_fsm (ap_hdl), ap_obj,
                                        ap_msg_sc->param1);
    }

  return rc;
}

#endif /* TIZKERNEL_DISPATCH_INL */
