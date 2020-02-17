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
 * @file   tizkernel_helpers.inl
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - kernel's helper functions
 *
 * @remark This file is meant to be included in the main tizkernel.c module to
 * create a single compilation unit.
 *
 */

#ifndef TIZKERNEL_HELPERS_INL
#define TIZKERNEL_HELPERS_INL

#ifndef S_SPLINT_S
#define TIZ_KRN_INIT_MSG_OOM(obj, hdl, msg, msgtype)    \
  do                                                    \
    {                                                   \
      msg = init_krn_message (obj, hdl, (msgtype));     \
      tiz_check_null_ret_oom (msg);                     \
    }                                                   \
  while (0)
#endif

/* NOTE: Start ignoring splint warnings in this section of code */
/*@ignore@*/

static void rm_callback_hdlr (void *ap_obj, tiz_event_pluggable_t *ap_event)
{
  if (ap_event)
    {
      tiz_mem_free (ap_event->p_data);
      tiz_mem_free (ap_event);
    }
}

/* TODO: Fix return code mess */
static void deliver_pluggable_event (OMX_U32 rid, OMX_HANDLETYPE ap_hdl)
{
  tiz_event_pluggable_t *p_event = (tiz_event_pluggable_t *)tiz_mem_calloc (
      1, sizeof(tiz_event_pluggable_t));
  OMX_U32 *p_rid = (OMX_U32 *)tiz_mem_calloc (1, sizeof(OMX_U32));

  if (p_event && p_rid)
    {
      *p_rid = rid;
      p_event->p_servant = tiz_get_krn (ap_hdl);
      p_event->p_data = p_rid;
      p_event->pf_hdlr = &rm_callback_hdlr;
      (void)tiz_comp_event_pluggable (ap_hdl, p_event);
    }
  else
    {
      tiz_mem_free (p_event);
      tiz_mem_free (p_rid);
    }
  /* This should return something */
}
/*@end@*/
/* NOTE: Stop ignoring splint warnings in this section  */

static void wait_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  deliver_pluggable_event (rid, ap_data);
}

static void preemption_req (OMX_U32 rid, OMX_PTR ap_data)
{
  deliver_pluggable_event (rid, ap_data);
}

static void preemption_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  deliver_pluggable_event (rid, ap_data);
}

static inline tiz_vector_t *get_ingress_lst (const tiz_krn_t *ap_obj,
                                             OMX_U32 a_pid)
{
  tiz_vector_t *p_list = NULL;
  assert (ap_obj);
  /* Grab the port's ingress list */
  p_list = tiz_vector_at (ap_obj->p_ingress_, a_pid);
  assert (p_list && *(tiz_vector_t **)p_list);
  return (*(tiz_vector_t **)p_list);
}

static inline tiz_vector_t *get_egress_lst (const tiz_krn_t *ap_obj,
                                            OMX_U32 a_pid)
{
  tiz_vector_t *p_list = NULL;
  assert (ap_obj);
  /* Grab the port's egress list */
  p_list = tiz_vector_at (ap_obj->p_egress_, a_pid);
  assert (p_list && *(tiz_vector_t **)p_list);
  return (*(tiz_vector_t **)p_list);
}

static inline OMX_PTR get_port (const tiz_krn_t *ap_obj, const OMX_U32 a_pid)
{
  OMX_PTR *pp_port = NULL;
  assert (ap_obj);
  /* Find the port.. */
  pp_port = tiz_vector_at (ap_obj->p_ports_, a_pid);
  assert (pp_port && *pp_port);
  return *pp_port;
}

static inline OMX_BUFFERHEADERTYPE *get_header (const tiz_vector_t *ap_list,
                                                OMX_U32 a_index)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  assert (ap_list);
  assert (a_index < tiz_vector_length (ap_list));
  /* Retrieve the header... */
  pp_hdr = tiz_vector_at (ap_list, a_index);
  assert (pp_hdr && *pp_hdr);
  return *pp_hdr;
}

static OMX_S32 move_to_ingress (void *ap_obj, OMX_U32 a_pid)
{

  tiz_krn_t *p_obj = ap_obj;
  tiz_vector_t *p_elist = NULL;
  tiz_vector_t *p_ilist = NULL;
  const OMX_S32 nports = tiz_vector_length (p_obj->p_ports_);
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (a_pid < nports);

  p_elist = tiz_vector_at (p_obj->p_egress_, a_pid);
  p_ilist = tiz_vector_at (p_obj->p_ingress_, a_pid);
  assert (p_elist && *(tiz_vector_t **)p_elist);
  p_elist = *(tiz_vector_t **)p_elist;
  assert (p_ilist && *(tiz_vector_t **)p_elist);
  p_ilist = *(tiz_vector_t **)p_ilist;
  rc = tiz_vector_append (p_ilist, p_elist);
  tiz_vector_clear (p_elist);

  if (OMX_ErrorNone != rc)
    {
      return -1;
    }

  return tiz_vector_length (p_ilist);
}

static OMX_S32 move_to_egress (void *ap_obj, OMX_U32 a_pid)
{
  tiz_krn_t *p_obj = ap_obj;
  const OMX_S32 nports = tiz_vector_length (p_obj->p_ports_);
  tiz_vector_t *p_elist = NULL;
  tiz_vector_t *p_ilist = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (a_pid < nports);

  p_elist = tiz_vector_at (p_obj->p_egress_, a_pid);
  p_ilist = tiz_vector_at (p_obj->p_ingress_, a_pid);
  assert (p_elist && *(tiz_vector_t **)p_elist);
  p_elist = *(tiz_vector_t **)p_elist;
  assert (p_ilist && *(tiz_vector_t **)p_ilist);
  p_ilist = *(tiz_vector_t **)p_ilist;
  rc = tiz_vector_append (p_elist, p_ilist);
  tiz_vector_clear (p_ilist);

  if (OMX_ErrorNone != rc)
    {
      return -1;
    }

  return tiz_vector_length (p_elist);
}

static OMX_S32 add_to_buflst (void *ap_obj, tiz_vector_t *ap_dst2darr,
                              const OMX_BUFFERHEADERTYPE *ap_hdr,
                              const void *ap_port)
{
  const tiz_krn_t *p_obj = ap_obj;
  tiz_vector_t *p_list = NULL;
  const OMX_U32 pid = tiz_port_index (ap_port);

  assert (ap_obj);
  assert (ap_dst2darr);
  assert (ap_hdr);
  assert (tiz_vector_length (ap_dst2darr) >= pid);

  p_list = tiz_vector_at (ap_dst2darr, pid);
  assert (p_list && *(tiz_vector_t **)p_list);
  p_list = *(tiz_vector_t **)p_list;

  TIZ_TRACE (handleOf (p_obj),
             "HEADER [%p] BUFFER [%p] PID [%d] "
             "list size [%d] buf count [%d]",
             ap_hdr, ap_hdr->pBuffer, pid, tiz_vector_length (p_list),
             tiz_port_buffer_count (ap_port));

  assert (tiz_vector_length (p_list) < tiz_port_buffer_count (ap_port));

  if (OMX_ErrorNone != tiz_vector_push_back (p_list, &ap_hdr))
    {
      return -1;
    }
  else
    {
      assert (tiz_vector_length (p_list) <= tiz_port_buffer_count (ap_port));
      return tiz_vector_length (p_list);
    }
}

static OMX_S32 clear_hdr_contents (tiz_vector_t *ap_hdr_lst, OMX_U32 a_pid)
{
  tiz_vector_t *p_list = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_S32 i, hdr_count = 0;

  assert (ap_hdr_lst);
  assert (tiz_vector_length (ap_hdr_lst) >= a_pid);

  p_list = tiz_vector_at (ap_hdr_lst, a_pid);
  assert (p_list && *(tiz_vector_t **)p_list);
  p_list = *(tiz_vector_t **)p_list;

  hdr_count = tiz_vector_length (p_list);
  for (i = 0; i < hdr_count; ++i)
    {
      p_hdr = get_header (p_list, i);
      tiz_clear_header (p_hdr);
    }

  return hdr_count;
}

static OMX_ERRORTYPE append_buflsts (tiz_vector_t *ap_dst2darr,
                                     const tiz_vector_t *ap_srclst,
                                     OMX_U32 a_pid)
{
  tiz_vector_t *p_list = NULL;
  assert (ap_dst2darr);
  assert (ap_srclst);
  assert (tiz_vector_length (ap_dst2darr) >= a_pid);

  p_list = tiz_vector_at (ap_dst2darr, a_pid);
  assert (p_list && *(tiz_vector_t **)p_list);
  p_list = *(tiz_vector_t **)p_list;

  /* Make sure the list is empty, before appending anything */
  tiz_vector_clear (p_list);

  return tiz_vector_append (p_list, ap_srclst);
}

static void clear_hdr_lsts (void *ap_obj, const OMX_U32 a_pid)
{
  tiz_krn_t *p_obj = ap_obj;
  tiz_vector_t *p_list = NULL;
  OMX_S32 i = 0;
  OMX_U32 pid = 0;
  OMX_S32 nports = 0;

  assert (ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  do
    {
      pid = ((OMX_ALL != a_pid) ? a_pid : i);

      p_list = tiz_vector_at (p_obj->p_ingress_, pid);
      assert (p_list && *(tiz_vector_t **)p_list);
      p_list = *(tiz_vector_t **)p_list;
      tiz_vector_clear (p_list);

      p_list = tiz_vector_at (p_obj->p_egress_, pid);
      assert (p_list && *(tiz_vector_t **)p_list);
      p_list = *(tiz_vector_t **)p_list;
      tiz_vector_clear (p_list);

      ++i;
    }
  while (OMX_ALL == pid && i < nports);
}

static OMX_ERRORTYPE check_pid (const tiz_krn_t *ap_obj, OMX_U32 a_pid)
{
  assert (ap_obj);

  if (a_pid >= tiz_vector_length (ap_obj->p_ports_))
    {
      TIZ_ERROR (handleOf (ap_obj), "[OMX_ErrorBadPortIndex] : port [%d]...",
                 a_pid);
      return OMX_ErrorBadPortIndex;
    }

  return OMX_ErrorNone;
}

static inline OMX_U32 cmd_to_priority (OMX_COMMANDTYPE a_cmd)
{
  OMX_U32 prio = 0;

  switch (a_cmd)
    {
      case OMX_CommandStateSet:
      case OMX_CommandFlush:
      case OMX_CommandPortDisable:
      case OMX_CommandPortEnable:
      case OMX_CommandMarkBuffer:
        {
          prio = 0;
        }
        break;

      default:
        assert (0);
        break;
    };

  return prio;
}

static OMX_ERRORTYPE propagate_ingress (void *ap_obj, OMX_U32 a_pid)
{
  tiz_krn_t *p_obj = ap_obj;
  void *p_prc = NULL;
  tiz_vector_t *p_list = NULL;
  OMX_PTR p_port = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_S32 i = 0;
  OMX_S32 j = 0;
  OMX_S32 nbufs = 0;
  OMX_U32 pid = 0;
  OMX_DIRTYPE pdir = OMX_DirMax;
  OMX_S32 nports = 0;
  OMX_HANDLETYPE p_hdl = handleOf (p_obj);

  assert (ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);
  p_prc = tiz_get_prc (p_hdl);

  do
    {
      /* Grab the port */
      pid = ((OMX_ALL != a_pid) ? a_pid : i);
      p_port = get_port (p_obj, pid);

      /* Get port direction */
      pdir = tiz_port_dir (p_port);

      /* Grab the port's ingress list */
      p_list = get_ingress_lst (p_obj, pid);
      TIZ_TRACE (handleOf (p_obj), "port [%d]'s ingress list length [%d]...",
                 pid, tiz_vector_length (p_list));

      nbufs = tiz_vector_length (p_list);
      for (j = 0; j < nbufs; ++j)
        {
          /* Retrieve the header... */
          p_hdr = get_header (p_list, j);

          TIZ_TRACE (handleOf (p_obj), "Dispatching HEADER [%p] BUFFER [%p]",
                     p_hdr, p_hdr->pBuffer);

          /* ... delegate to the processor... */
          if (OMX_DirInput == pdir)
            {
              assert (p_hdr->nInputPortIndex == pid);
              tiz_check_omx (tiz_api_EmptyThisBuffer (p_prc, p_hdl, p_hdr));
            }
          else
            {
              assert (p_hdr->nOutputPortIndex == pid);
              /* Make sure there is no data from a previous transition into
                 OMX_StateExecuting */
              tiz_clear_header (p_hdr);
              /* TOOD: Do we need to reset any other header fields? */
              tiz_check_omx (tiz_api_FillThisBuffer (p_prc, p_hdl, p_hdr));
            }

          /* ... and keep the header in the list. */
        }

      ++i;
    }
  while (OMX_ALL == pid && i < nports);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE transfer_mark (void *ap_obj, const OMX_MARKTYPE *ap_mark)
{
  const tiz_krn_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_S32 nports = 0;
  OMX_PTR p_port = NULL;
  OMX_S32 i = 0;

  assert (ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  for (i = 0; i < nports && OMX_ErrorNone == rc; ++i)
    {
      p_port = get_port (p_obj, i);

      if (OMX_DirOutput == tiz_port_dir (p_port))
        {
          rc = tiz_port_store_mark (p_port, ap_mark, OMX_FALSE);
        }
    }

  return rc;
}

static OMX_ERRORTYPE complete_mark_buffer (void *ap_obj, OMX_PTR ap_port,
                                           OMX_U32 a_pid, OMX_ERRORTYPE a_error)
{
  tiz_krn_t *p_obj = ap_obj;

  assert (ap_obj);
  assert (ap_port);

  /* Complete the OMX_CommandMarkBuffer command */
  (void)tiz_srv_issue_cmd_event (p_obj, OMX_CommandMarkBuffer, a_pid, a_error);

  /* Decrement the completion counter */
  /*   assert (p_obj->cmd_completion_count_ > 0); */
  /*   if (--p_obj->cmd_completion_count_ == 0) */
  /*     { */
  /*       OMX_HANDLETYPE p_hdl = handleOf (p_obj); */
  /*       tiz_fsm_complete_command (tiz_get_fsm (p_hdl), p_obj, */
  /*                                OMX_CommandMarkBuffer); */
  /*     } */

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE process_marks (void *ap_obj, OMX_BUFFERHEADERTYPE *ap_hdr,
                                    OMX_U32 a_pid, OMX_COMPONENTTYPE *ap_hdl)
{
  tiz_krn_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_S32 nports = 0;
  OMX_PTR p_port = NULL;

  assert (ap_obj);
  assert (ap_hdr);

  nports = tiz_vector_length (p_obj->p_ports_);

  assert (a_pid < nports);

  p_port = get_port (p_obj, a_pid);

  /* Look for buffer marks to be signalled or propagated */
  if (ap_hdr->hMarkTargetComponent)
    {
      /* See if this component is the buffer mark target component... */
      if (ap_hdr->hMarkTargetComponent == ap_hdl)
        {
          /* Return the mark to the IL Client */
          tiz_srv_issue_event (ap_obj, OMX_EventMark, 0, 0, ap_hdr->pMarkData);

          /* Remove the mark from the header as it has been delivered */
          /* to the client... */
          ap_hdr->hMarkTargetComponent = 0;
          ap_hdr->pMarkData = 0;
        }
      else
        {
          /* Buffer mark propagation logic */
          /* If port is output, do nothing */
          /* If port is input, transfer its mark to all the output ports in the
           * component */
          const OMX_DIRTYPE dir = tiz_port_dir (p_port);
          if (dir == OMX_DirInput)
            {
              const OMX_MARKTYPE mark
                  = { ap_hdr->hMarkTargetComponent, ap_hdr->pMarkData };
              rc = transfer_mark (ap_obj, &mark);

              /* Remove the mark from the processed header... */
              ap_hdr->hMarkTargetComponent = 0;
              ap_hdr->pMarkData = 0;
            }
        }
    }

  else
    {
      /* No mark found. If port is input, nothing to do. */
      /* If port if output, mark the buffer, if any marks available... */
      const OMX_DIRTYPE dir = tiz_port_dir (p_port);
      if (dir == OMX_DirOutput)
        {
          /* NOTE: tiz_port_mark_buffer returns OMX_ErrorNone if the port marked
           * the buffer with one of its own marks */
          if (OMX_ErrorNone == (rc = tiz_port_mark_buffer (p_port, ap_hdr)))
            {
              /* Successfully complete here the OMX_CommandMarkBuffer command */
              tiz_check_omx (
                  complete_mark_buffer (p_obj, p_port, a_pid, OMX_ErrorNone));
            }
          else
            {
              /* These two return codes are not actual errors. */
              if (OMX_ErrorNoMore == rc || OMX_ErrorNotReady == rc)
                {
                  rc = OMX_ErrorNone;
                }
            }
        }
    }

  return rc;
}

static OMX_ERRORTYPE flush_marks (void *ap_obj, OMX_PTR ap_port)
{
  tiz_krn_t *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE hdr;

  assert (ap_obj);
  assert (ap_port);

  /* Use a dummy header to flush all marks in the port */
  do
    {
      (void)tiz_mem_set (&hdr, 0, sizeof(OMX_BUFFERHEADERTYPE));
      hdr.nVersion.nVersion = (OMX_U32)OMX_VERSION;
      hdr.hMarkTargetComponent = NULL;
      hdr.pMarkData = NULL;
      /* tiz_port_mark_buffer returns OMX_ErrorNone if the port owned the
       * mark. If the mark is not owned, it returns OMX_ErrorNotReady. When no
       * marks are found, it returns OMX_ErrorNoMore */
      if (OMX_ErrorNone == (rc = tiz_port_mark_buffer (ap_port, &hdr)))
        {
          OMX_ERRORTYPE complet_mark_rc = OMX_ErrorNone;
          /* Need to complete the mark buffer command with an error */
          if (OMX_ErrorNone != (complet_mark_rc = complete_mark_buffer (
                                    p_obj, ap_port, tiz_port_index (ap_port),
                                    OMX_ErrorPortUnpopulated)))
            {
              break;
            }
        }
    }
  while (OMX_ErrorNoMore != rc);

  if (OMX_ErrorNoMore == rc)
    {
      rc = OMX_ErrorNone;
    }

  return rc;
}

static OMX_ERRORTYPE flush_egress (void *ap_obj, const OMX_U32 a_pid,
                                   const OMX_BOOL a_clear)
{
  tiz_krn_t *p_obj = ap_obj;
  tiz_vector_t *p_list = NULL;
  OMX_PTR p_port = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_S32 i = 0;
  OMX_U32 pid = 0;
  OMX_DIRTYPE pdir = OMX_DirMax;
  OMX_HANDLETYPE p_hdl = handleOf (p_obj);
  OMX_HANDLETYPE p_thdl = NULL;
  OMX_S32 nports = 0;

  assert (ap_obj);

  nports = tiz_vector_length (p_obj->p_ports_);

  do
    {
      /* Grab the port */
      pid = ((OMX_ALL != a_pid) ? a_pid : i);
      p_port = get_port (p_obj, pid);

      /* Get port direction and tunnel info */
      pdir = tiz_port_dir (p_port);
      p_thdl = tiz_port_get_tunnel_comp (p_port);

      /* Grab the port's egress list */
      p_list = get_egress_lst (p_obj, pid);

      TIZ_TRACE (p_hdl,
                 "pid [%d] loop index=[%d] egress length [%d] "
                 "- p_thdl [%p]...",
                 pid, i, tiz_vector_length (p_list), p_thdl);

      while (tiz_vector_length (p_list) > 0)
        {
          /* Retrieve the header... */
          p_hdr = get_header (p_list, 0);

          TIZ_TRACE (p_hdl, "HEADER [%p] BUFFER [%p]", p_hdr, p_hdr->pBuffer);

          /* ... issue the callback... */
          {

            /* If it's an input port and allocator, ask the port to
             * allocate the actual buffer, in case pre-announcements have
             * been disabled on this port. This function call has no effect
             * if pre-announcements are enabled on the port. */
            if (OMX_DirInput == pdir && TIZ_PORT_IS_ALLOCATOR (p_port))
              {
                tiz_check_omx (tiz_port_populate_header (p_port, p_hdr));
              }

            /* Propagate buffer marks... */
            tiz_check_omx (process_marks (p_obj, p_hdr, pid, p_hdl));

            if (OMX_TRUE == a_clear)
              {
                tiz_clear_header (p_hdr);
              }
            else
              {
                if ((p_hdr->nFlags & OMX_BUFFERFLAG_EOS) != 0)
                  {

                    /* Automatically report EOS event on output ports, but only
                     * once...  */
                    if ((OMX_DirOutput == pdir) && false == p_obj->eos_)
                      {
                        TIZ_NOTICE (p_hdl,
                                    "OMX_BUFFERFLAG_EOS on "
                                    "port [%d]...",
                                    pid);

                        /* ... flag EOS ... */
                        p_obj->eos_ = true;
                        tiz_srv_issue_event ((OMX_PTR)ap_obj,
                                             OMX_EventBufferFlag, pid,
                                             p_hdr->nFlags, NULL);
                      }
                    /* Automatically clear EOS flag on input ports so that the
                       flag
                       does not get propagated upstream (this is a safety
                       measure,
                       as the component's processor is "usually" responsible for
                       clearing the EOS flag. */
                    else if (OMX_DirInput == pdir)
                      {
                        /* Clear the EOS flag */
                        p_hdr->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
                      }
                  }
              }

            /* get rid of the buffer */
            tiz_srv_issue_buf_callback ((OMX_PTR)ap_obj, p_hdr, pid, pdir,
                                        p_thdl);
            /* ... and delete it from the list. */
            tiz_vector_erase (p_list, 0, 1);
          }
        }
      ++i;
    }
  while (OMX_ALL == a_pid && i < nports);

  return OMX_ErrorNone;
}

static const OMX_STRING krn_msg_to_str (tiz_krn_msg_class_t a_msg)
{
  const OMX_S32 count = sizeof(tiz_krn_msg_to_str_tbl)
                        / sizeof(tiz_krn_msg_str_t);
  OMX_S32 i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_krn_msg_to_str_tbl[i].msg == a_msg)
        {
          return tiz_krn_msg_to_str_tbl[i].str;
        }
    }

  return "Unknown kernel message";
}

static OMX_ERRORTYPE complete_port_disable (void *ap_obj, OMX_PTR ap_port,
                                            OMX_U32 a_pid,
                                            OMX_ERRORTYPE a_error)
{
  tiz_krn_t *p_obj = ap_obj;

  assert (ap_obj);
  assert (ap_port);

  /* Set disabled flag */
  TIZ_PORT_SET_DISABLED (ap_port);

  /* Decrement the completion counter */
  assert (p_obj->cmd_completion_count_ > 0);
  p_obj->cmd_completion_count_--;

  if (p_obj->cmd_completion_count_ > 0)
    {
      /* Complete the OMX_CommandPortDisable command here */
      (void)tiz_srv_issue_cmd_event (p_obj, OMX_CommandPortDisable, a_pid,
                                     a_error);
    }

  /* If the completion count is zero, let the FSM complete, as it will know
   * whether this a cancelation or not. */
  if (p_obj->cmd_completion_count_ == 0)
    {
      OMX_HANDLETYPE p_hdl = handleOf (p_obj);
      tiz_check_omx (tiz_fsm_complete_command (
          tiz_get_fsm (p_hdl), p_obj, OMX_CommandPortDisable, a_pid));
    }

  /* Flush buffer marks and complete commands as required */
  return flush_marks (p_obj, ap_port);
}

static OMX_ERRORTYPE complete_port_enable (void *ap_obj, OMX_PTR ap_port,
                                           OMX_U32 a_pid, OMX_ERRORTYPE a_error)
{
  tiz_krn_t *p_obj = ap_obj;

  assert (ap_obj);
  assert (ap_port);

  /* Set enabled flag */
  TIZ_PORT_SET_ENABLED (ap_port);

  /* Now it is time to notify the processor servant that a port is being
     enabled (telling the processor should be the last thing we do during the
     port enable sequence). */
  tiz_check_omx (tiz_api_SendCommand (tiz_get_prc (handleOf (p_obj)),
                                          handleOf (p_obj),
                                          OMX_CommandPortEnable, a_pid, NULL));

  /* Decrement the completion counter */
  assert (p_obj->cmd_completion_count_ > 0);
  p_obj->cmd_completion_count_--;

  if (p_obj->cmd_completion_count_ > 0)
    {
      /* Complete the OMX_CommandPortEnable command here */
      (void)tiz_srv_issue_cmd_event (p_obj, OMX_CommandPortEnable, a_pid,
                                     a_error);
    }

  /* If the completion count is zero, let the FSM complete, as it will know
   * whether this a cancelation or not. */
  if (p_obj->cmd_completion_count_ == 0)
    {
      OMX_HANDLETYPE p_hdl = handleOf (p_obj);
      tiz_check_omx (tiz_fsm_complete_command (
          tiz_get_fsm (p_hdl), p_obj, OMX_CommandPortEnable, a_pid));
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE complete_port_flush (void *ap_obj, OMX_PTR ap_port,
                                          OMX_U32 a_pid, OMX_ERRORTYPE a_error)
{
  tiz_krn_t *p_obj = ap_obj;

  assert (ap_obj);
  assert (ap_port);

  TIZ_PORT_CLEAR_FLUSH_IN_PROGRESS (ap_port);

  /* Complete the OMX_CommandFlush command */
  (void)tiz_srv_issue_cmd_event (p_obj, OMX_CommandFlush, a_pid, a_error);

  /* Decrement the completion counter */
  assert (p_obj->cmd_completion_count_ > 0);
  if (--p_obj->cmd_completion_count_ == 0)
    {
      OMX_HANDLETYPE p_hdl = handleOf (p_obj);
      tiz_check_omx (tiz_fsm_complete_command (tiz_get_fsm (p_hdl), p_obj,
                                                   OMX_CommandFlush, a_pid));
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE complete_ongoing_transitions (const void *ap_obj,
                                                   OMX_HANDLETYPE ap_hdl)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const tiz_fsm_state_id_t cur_state
      = tiz_fsm_get_substate (tiz_get_fsm (ap_hdl));

  assert (ap_obj);
  assert (ap_hdl);

  if ((ESubStateIdleToLoaded == cur_state) && all_depopulated (ap_obj))
    {
      TIZ_TRACE (ap_hdl, "AllPortsDepopulated : [TRUE]");
      /* TODO : Review this */
      /* If all ports are depopulated, kick off removal of buffer
       * callbacks from servants kernel and proc queues  */
      rc = tiz_fsm_complete_transition (tiz_get_fsm (ap_hdl), ap_obj,
                                        OMX_StateLoaded);
    }
  else if ((ESubStateLoadedToIdle == cur_state) && all_populated (ap_obj))
    {
      TIZ_TRACE (ap_hdl, "AllPortsPopulated : [TRUE]");
      rc = tiz_fsm_complete_transition (tiz_get_fsm (ap_hdl), ap_obj,
                                        OMX_StateIdle);
    }
  return rc;
}

static OMX_BOOL remove_buffer_from_servant_queue (OMX_PTR ap_elem,
                                                  OMX_S32 a_data1,
                                                  OMX_PTR ap_data2)
{
  OMX_BOOL rc = OMX_FALSE;
  tiz_krn_msg_t *p_msg = ap_elem;
  const OMX_BUFFERHEADERTYPE *p_hdr = ap_data2;

  assert (ap_elem);
  assert (ap_data2);

  if (p_msg->class == a_data1)
    {
      tiz_krn_msg_callback_t *p_msg_c = &(p_msg->cb);
      if (p_hdr == p_msg_c->p_hdr)
        {
          /* Found, return TRUE so this item will be removed from the servant
           * queue */
          TIZ_TRACE (p_msg->p_hdl, "Found HEADER [%p]", p_hdr);
          rc = OMX_TRUE;
        }
    }

  return rc;
}

static OMX_BOOL process_efb_from_servant_queue (OMX_PTR ap_elem,
                                                OMX_S32 a_data1,
                                                OMX_PTR ap_data2)
{
  OMX_BOOL rc = OMX_FALSE;
  tiz_krn_msg_t *p_msg = ap_elem;
  tiz_krn_t *p_obj = ap_data2;

  assert (ap_elem);
  assert (ap_data2);

  if (p_msg->class == ETIZKrnMsgEmptyThisBuffer
      || p_msg->class == ETIZKrnMsgFillThisBuffer)
    {
      tiz_krn_msg_emptyfillbuffer_t *p_msg_ef = &(p_msg->ef);
      OMX_BUFFERHEADERTYPE *p_hdr = NULL;
      OMX_U32 pid = 0;
      OMX_HANDLETYPE *p_hdl = NULL;

      p_hdr = p_msg_ef->p_hdr;
      assert (p_hdr);

      p_hdl = p_msg->p_hdl;
      assert (p_hdl);

      pid = p_msg->class == ETIZKrnMsgEmptyThisBuffer ? p_hdr->nInputPortIndex
                                                      : p_hdr->nOutputPortIndex;

      if (OMX_ALL == a_data1 || pid == a_data1)
        {
          OMX_PTR p_port = NULL;
          TIZ_TRACE (p_hdl, "HEADER [%p] BUFFER [%p] PID [%d]", p_hdr,
                     p_hdr->pBuffer, pid);

          assert (check_pid (p_obj, pid) == OMX_ErrorNone);

          /* Retrieve the port... */
          p_port = get_port (p_obj, pid);

          /* Add this buffer to the ingress hdr list */
          if (0 < add_to_buflst (p_obj, p_obj->p_ingress_, p_hdr, p_port))
            {
              rc = OMX_TRUE;
            }
          else
            {
              TIZ_ERROR (p_hdl,
                         "Error on port [%d] while "
                         "adding buffer to ingress list",
                         pid);
            }
        }
    }

  return rc;
}

static OMX_BOOL process_cbacks_from_servant_queue (OMX_PTR ap_elem,
                                                   OMX_S32 a_data1,
                                                   OMX_PTR ap_data2)
{
  OMX_BOOL rc = OMX_FALSE;
  tiz_krn_msg_t *p_msg = ap_elem;
  tiz_krn_t *p_obj = ap_data2;

  assert (ap_elem);
  assert (ap_data2);

  if (p_msg->class == ETIZKrnMsgCallback)
    {
      tiz_krn_msg_callback_t *p_msg_cb = &(p_msg->cb);
      OMX_BUFFERHEADERTYPE *p_hdr = NULL;
      OMX_U32 pid = 0;
      OMX_HANDLETYPE *p_hdl = NULL;

      p_hdr = p_msg_cb->p_hdr;
      assert (p_hdr);

      p_hdl = p_msg->p_hdl;
      assert (p_hdl);

      pid = p_msg_cb->pid;

      if (OMX_ALL == a_data1 || pid == a_data1)
        {
          OMX_PTR p_port = NULL;
          TIZ_TRACE (p_hdl, "HEADER [%p] BUFFER [%p] PID [%d]", p_hdr,
                     p_hdr->pBuffer, pid);

          assert (check_pid (p_obj, pid) == OMX_ErrorNone);

          /* Retrieve the port... */
          p_port = get_port (p_obj, pid);

          /* Add this buffer to the egress hdr list */
          if (0 < add_to_buflst (p_obj, p_obj->p_egress_, p_hdr, p_port))
            {
              (void)TIZ_PORT_DEC_CLAIMED_COUNT (p_port);
              rc = OMX_TRUE;
            }
          else
            {
              TIZ_ERROR (p_hdl,
                         "Error on port [%d] while "
                         "adding buffer to egress list",
                         pid);
            }
        }
    }

  return rc;
}

/* NOTE: Start ignoring splint warnings in this section of code */
/*@ignore@*/
static inline tiz_krn_msg_t *init_krn_message (
    const void *ap_obj, OMX_HANDLETYPE ap_hdl,
    const tiz_krn_msg_class_t a_msg_class)
{
  tiz_krn_t *p_obj = (tiz_krn_t *)ap_obj;
  tiz_krn_msg_t *p_msg = NULL;

  assert (ap_obj);
  assert (a_msg_class < ETIZKrnMsgMax);

  if (NULL == (p_msg = tiz_srv_init_msg (p_obj, sizeof(tiz_krn_msg_t))))
    {
      TIZ_ERROR (ap_hdl,
                 "[OMX_ErrorInsufficientResources] : "
                 "Could not allocate message [%s]",
                 krn_msg_to_str (a_msg_class));
    }
  else
    {
      p_msg->p_hdl = ap_hdl;
      p_msg->class = a_msg_class;
    }

  return p_msg;
}
/*@end@*/
/* NOTE: Stop ignoring splint warnings in this section  */

static OMX_ERRORTYPE enqueue_callback_msg (
    const void *ap_obj,
    /*@null@*/ OMX_BUFFERHEADERTYPE *ap_hdr, const OMX_U32 a_pid,
    const OMX_DIRTYPE a_dir)
{
  tiz_krn_t *p_obj = (tiz_krn_t *)ap_obj;
  tiz_krn_msg_t *p_msg = NULL;
  tiz_krn_msg_callback_t *p_msg_cb = NULL;

  assert (ap_obj);

  /* NOTE: Start ignoring splint warnings in this section of code */
  /*@ignore@*/
  TIZ_TRACE (handleOf (p_obj),
             "callback : HEADER [%p] BUFFER [%p] "
             "PID [%d] DIR [%s]",
             ap_hdr != NULL ? ap_hdr : 0, ap_hdr ? ap_hdr->pBuffer : 0, a_pid,
             tiz_dir_to_str (a_dir));
  /*@end@*/
  /* NOTE: Stop ignoring splint warnings in this section  */

  TIZ_KRN_INIT_MSG_OOM (p_obj, handleOf (p_obj), p_msg, ETIZKrnMsgCallback);

  assert (p_msg);
  p_msg_cb = &(p_msg->cb);
  p_msg_cb->p_hdr = ap_hdr;
  p_msg_cb->pid = a_pid;
  p_msg_cb->dir = a_dir;
  return tiz_srv_enqueue (ap_obj, p_msg, 1);
}

static OMX_ERRORTYPE init_rm (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  tiz_krn_t *p_obj = (tiz_krn_t *)ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_obj);
  assert (ap_hdl);

  if (!p_obj->rm_inited_
      && (0 == tiz_rcfile_compare_value ("resource-management", "enabled", "true")))
    {
      OMX_U8 comp_name[OMX_MAX_STRINGNAME_SIZE];
      OMX_VERSIONTYPE comp_ver;
      OMX_VERSIONTYPE spec_ver;
      OMX_UUIDTYPE uuid;
      OMX_PRIORITYMGMTTYPE primgmt;
      tiz_rm_error_t rmrc = TIZ_RM_SUCCESS;

      rc = tiz_api_GetComponentVersion (p_obj->p_cport_, ap_hdl,
                                        (OMX_STRING)(&comp_name), &comp_ver,
                                        &spec_ver, &uuid);
      if (OMX_ErrorNone != rc)
        {
          TIZ_ERROR (ap_hdl,
                     "[%s] : while retrieving the component's "
                     "name...",
                     tiz_err_to_str (rc));
          return rc;
        }

      comp_name[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';

      primgmt.nSize = (OMX_U32)sizeof(OMX_PRIORITYMGMTTYPE);
      primgmt.nVersion.nVersion = OMX_VERSION;
      primgmt.nGroupPriority = 0;
      primgmt.nGroupID = 0;

      if (OMX_ErrorNone
          != (rc = tiz_api_GetConfig (p_obj->p_cport_, ap_hdl,
                                      OMX_IndexConfigPriorityMgmt, &primgmt)))
        {
          TIZ_ERROR (ap_hdl,
                     "[%s] : while retrieving "
                     "OMX_IndexConfigPriorityMgmt...",
                     tiz_err_to_str (rc));
          return rc;
        }

      if (TIZ_RM_SUCCESS
          != (rmrc = tiz_rm_proxy_init (&p_obj->rm_, (OMX_STRING)(&comp_name),
                                        (const OMX_UUIDTYPE *)&uuid, &primgmt,
                                        &p_obj->rm_cbacks_, ap_hdl)))
        {
          TIZ_ERROR (ap_hdl,
                     "[OMX_ErrorInsufficientResources] : "
                     "RM error [%d]...",
                     rmrc);
          return OMX_ErrorInsufficientResources;
        }

      /* NOTE: Start ignoring splint warnings in this section of code */
      /*@ignore@*/
      TIZ_TRACE (ap_hdl, "[%s] [%p] : RM init'ed", comp_name, ap_hdl);
      /*@end@*/
      /* NOTE: Stop ignoring splint warnings in this section  */
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE deinit_rm (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  tiz_krn_t *p_obj = (tiz_krn_t *)ap_obj;

  assert (ap_obj);
  assert (ap_hdl);

  if (p_obj->rm_inited_)
    {
      tiz_rm_error_t rmrc = TIZ_RM_SUCCESS;
      if (TIZ_RM_SUCCESS != (rmrc = tiz_rm_proxy_destroy (&p_obj->rm_)))
        {
          /* TODO: Translate into a proper error code, especially OOM error  */
          TIZ_ERROR (
              ap_hdl,
              "[OMX_ErrorUndefined] : RM proxy deinitialization failed...");
          return OMX_ErrorUndefined;
        }
      p_obj->rm_inited_ = false;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE acquire_rm_resources (const void *ap_obj,
                                           OMX_HANDLETYPE ap_hdl)
{
  tiz_krn_t *p_obj = (tiz_krn_t *)ap_obj;
  tiz_rm_error_t rmrc = TIZ_RM_SUCCESS;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  /* Request permission to use the RM-based resources */
  if (p_obj->rm_inited_
      && TIZ_RM_SUCCESS
      != (rmrc = tiz_rm_proxy_acquire (&p_obj->rm_, TIZ_RM_RESOURCE_DUMMY, 1)))
    {
      switch (rmrc)
        {
          case TIZ_RM_PREEMPTION_IN_PROGRESS:
            {
              rc = OMX_ErrorResourcesPreempted;
            }
            break;

          case TIZ_RM_NOT_ENOUGH_RESOURCE_AVAILABLE:
          default:
            {
              rc = OMX_ErrorInsufficientResources;
            }
        };

      TIZ_ERROR (ap_hdl,
                 "[%s] : While acquiring resource - "
                 "RM error [%d]...",
                 tiz_err_to_str (rc), rmrc);
    }

  return rc;
}

static OMX_ERRORTYPE release_rm_resources (const void *ap_obj,
                                           OMX_HANDLETYPE ap_hdl)
{
  tiz_krn_t *p_obj = (tiz_krn_t *)ap_obj;
  tiz_rm_error_t rmrc = TIZ_RM_SUCCESS;

  if (p_obj->rm_inited_
      && TIZ_RM_SUCCESS
      != (rmrc = tiz_rm_proxy_release (&p_obj->rm_, TIZ_RM_RESOURCE_DUMMY, 1)))
    {
      TIZ_TRACE (ap_hdl, "Resource release failed - RM error [%d]...", rmrc);
    }

  return OMX_ErrorNone;
}

static bool all_populated (const void *ap_obj)
{
  const tiz_krn_t *p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_PTR p_port = NULL;
  OMX_U32 i = 0;

  assert (ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  for (i = 0; i < nports; ++i)
    {
      p_port = get_port (p_obj, i);

      TIZ_TRACE (
          handleOf (p_obj), "PORT [%d] is [%s] and [%s]", i,
          TIZ_PORT_IS_ENABLED (p_port) ? "ENABLED" : "NOT ENABLED",
          TIZ_PORT_IS_POPULATED (p_port) ? "POPULATED" : "NOT POPULATED");

      if (TIZ_PORT_IS_ENABLED (p_port) && !(TIZ_PORT_IS_POPULATED (p_port)))
        {
          TIZ_TRACE (handleOf (p_obj),
                     "ALL ENABLED ports are populated = [OMX_FALSE]");
          return false;
        }
    }

  TIZ_TRACE (handleOf (p_obj), "ALL ENABLED ports are populated = [OMX_TRUE]");

  return true;
}

static bool all_depopulated (const void *ap_obj)
{
  const tiz_krn_t *p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_PTR p_port = NULL;
  OMX_U32 i;

  assert (ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  for (i = 0; i < nports; ++i)
    {
      p_port = get_port (p_obj, i);
      if (tiz_port_buffer_count (p_port) > 0)
        {
          TIZ_TRACE (handleOf (p_obj), "ALL DEPOPULATED = [FALSE]");
          return false;
        }
    }

  TIZ_TRACE (handleOf (p_obj), "ALL DEPOPULATED = [TRUE]");

  return true;
}

static bool all_buffers_returned (void *ap_obj)
{
  tiz_krn_t *p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_PTR p_port = NULL;
  tiz_vector_t *p_list = NULL;
  OMX_U32 i;
  OMX_S32 nbuf = 0, nbufin = 0;

  assert (ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  for (i = 0; i < nports; ++i)
    {
      p_port = get_port (p_obj, i);

      nbuf = tiz_port_buffer_count (p_port);

      if (TIZ_PORT_IS_DISABLED (p_port) || nbuf <= 0)
        {
          continue;
        }

      if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
        {
          p_list = get_ingress_lst (p_obj, i);

          if ((nbufin = tiz_vector_length (p_list)) != nbuf)
            {
              int j = 0;
              OMX_BUFFERHEADERTYPE *p_hdr = NULL;

              TIZ_TRACE (handleOf (p_obj),
                         "Port [%d] : awaiting buffers (only "
                         " [%d] out of [%d] have arrived)",
                         i, nbufin, nbuf);

              for (j = 0; j < nbufin; ++j)
                {
                  p_hdr = get_header (p_list, j);

                  TIZ_TRACE (handleOf (p_obj), "HEADER [%p] BUFFER [%p]", p_hdr,
                             p_hdr->pBuffer);
                }

              return false;
            }
        }
      else
        {
          const OMX_S32 claimed_count = TIZ_PORT_GET_CLAIMED_COUNT (p_port);
          if (claimed_count > 0)
            {
              TIZ_TRACE (handleOf (p_obj),
                         "Port [%d] : still need to "
                         "return [%d] buffers",
                         i, claimed_count);
              return false;
            }
        }
    }

  TIZ_TRACE (handleOf (p_obj), "ALL BUFFERS returned = [TRUE]...");

  p_obj->eos_ = false;

  return true;
}

/* static OMX_BOOL */
/* all_disabled (const void *ap_obj) */
/* { */
/*   const tiz_krn_t *p_obj = ap_obj; */
/*   OMX_S32 nports = 0; */
/*   OMX_PTR p_port = NULL; */
/*   OMX_U32 i; */

/*   assert (ap_obj); */
/*   nports = tiz_vector_length (p_obj->p_ports_); */

/*   for (i = 0; i < nports; ++i) */
/*     { */
/*       p_port = get_port (p_obj, i); */
/*       if (TIZ_PORT_IS_ENABLED (p_port)) */
/*         { */
/*           TIZ_TRACE (handleOf (p_obj), */
/*                     "ALL DISABLED = [OMX_FALSE]"); */
/*           return OMX_FALSE; */
/*         } */
/*     } */

/*   TIZ_TRACE (handleOf (p_obj), */
/*             "ALL DISABLED = [OMX_TRUE]"); */

/*   return OMX_TRUE; */
/* } */

#endif /* TIZKERNEL_HELPERS_INL */
