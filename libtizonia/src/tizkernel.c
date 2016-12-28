/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   tizkernel.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - kernel class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <OMX_Types.h>
#include <OMX_TizoniaExt.h>

#include "tizfsm.h"
#include "tizport.h"
#include "tizconfigport.h"
#include "tizport-macros.h"
#include "tizutils.h"

#include "tizkernel.h"
#include "tizkernel_decls.h"
#include "tizkernel_internal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.krn"
#endif

/* Forward declarations */
static bool
all_populated (const void * ap_obj);
static bool
all_depopulated (const void * ap_obj);
static bool
all_buffers_returned (void * ap_obj);
/* static OMX_BOOL all_disabled (const void *ap_obj); */
static OMX_ERRORTYPE
dispatch_sc (void * ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE
dispatch_etb (void * ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE
dispatch_ftb (void * ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE
dispatch_cb (void * ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE
dispatch_pe (void * ap_obj, OMX_PTR ap_msg);

static OMX_ERRORTYPE
dispatch_state_set (void * ap_obj, OMX_HANDLETYPE ap_hdl,
                    tiz_krn_msg_sendcommand_t * ap_msg_sc);
static OMX_ERRORTYPE
dispatch_port_disable (void * ap_obj, OMX_HANDLETYPE ap_hdl,
                       tiz_krn_msg_sendcommand_t * ap_msg_sc);
static OMX_ERRORTYPE
dispatch_port_enable (void * ap_obj, OMX_HANDLETYPE ap_hdl,
                      tiz_krn_msg_sendcommand_t * ap_msg_sc);
static OMX_ERRORTYPE
dispatch_port_flush (void * ap_obj, OMX_HANDLETYPE ap_hdl,
                     tiz_krn_msg_sendcommand_t * ap_msg_sc);
static OMX_ERRORTYPE
dispatch_mark_buffer (void * ap_obj, OMX_HANDLETYPE ap_hdl,
                      tiz_krn_msg_sendcommand_t * ap_msg_sc);
static OMX_ERRORTYPE
flush_marks (void * ap_obj, OMX_PTR ap_port);

static OMX_ERRORTYPE
check_pid (const tiz_krn_t * ap_obj, OMX_U32 a_pid);
static inline OMX_PTR
get_port (const tiz_krn_t * ap_obj, const OMX_U32 a_pid);
static OMX_S32
add_to_buflst (void * ap_obj, tiz_vector_t * ap_dst2darr,
               const OMX_BUFFERHEADERTYPE * ap_hdr, const void * ap_port);

static const tiz_krn_msg_dispatch_f tiz_krn_msg_to_fnt_tbl[] = {
  dispatch_sc, dispatch_etb, dispatch_ftb, dispatch_cb, dispatch_pe,
};

static const tiz_krn_msg_dispatch_sc_f tiz_krn_msg_dispatch_sc_to_fnt_tbl[] = {
  dispatch_state_set,   dispatch_port_flush,  dispatch_port_disable,
  dispatch_port_enable, dispatch_mark_buffer,
};

static const tiz_krn_msg_str_t tiz_krn_msg_to_str_tbl[] = {
  {ETIZKrnMsgSendCommand, "ETIZKrnMsgSendCommand"},
  {ETIZKrnMsgEmptyThisBuffer, "ETIZKrnMsgEmptyThisBuffer"},
  {ETIZKrnMsgFillThisBuffer, "ETIZKrnMsgFillThisBuffer"},
  {ETIZKrnMsgCallback, "ETIZKrnMsgCallback"},
  {ETIZKrnMsgPluggableEvent, "ETIZKrnMsgPluggableEvent"},
  {ETIZKrnMsgMax, "ETIZKrnMsgMax"},
};

#include "tizkernel_helpers.inl"
#include "tizkernel_dispatch.inl"

static OMX_ERRORTYPE
init_ports_and_lists (void * ap_obj)
{
  tiz_krn_t * p_obj = ap_obj;
  OMX_PORT_PARAM_TYPE null_param
    = {sizeof (OMX_PORT_PARAM_TYPE), {.nVersion = OMX_VERSION}, 0, 0};

  tiz_check_omx_ret_oom (
    tiz_vector_init (&(p_obj->p_ports_), sizeof (OMX_PTR)));
  tiz_check_omx_ret_oom (
    tiz_vector_init (&(p_obj->p_ingress_), sizeof (tiz_vector_t *)));
  tiz_check_omx_ret_oom (
    tiz_vector_init (&(p_obj->p_egress_), sizeof (tiz_vector_t *)));

  p_obj->p_cport_ = NULL;
  p_obj->p_proc_ = NULL;
  p_obj->eos_ = false;
  p_obj->rm_ = 0;
  p_obj->rm_cbacks_.pf_waitend = &wait_complete;
  p_obj->rm_cbacks_.pf_preempt = &preemption_req;
  p_obj->rm_cbacks_.pf_preempt_end = &preemption_complete;
  p_obj->rm_inited_ = false;
  p_obj->audio_init_ = null_param;
  p_obj->image_init_ = null_param;
  p_obj->video_init_ = null_param;
  p_obj->other_init_ = null_param;
  p_obj->cmd_completion_count_ = 0;
  p_obj->accept_use_buffer_notified_ = false;
  p_obj->accept_buffer_exchange_notified_ = false;
  p_obj->may_transition_exe2idle_notified_ = false;

  return OMX_ErrorNone;
}

static void
deinit_ports_and_lists (void * ap_obj)
{
  tiz_krn_t * p_obj = ap_obj;
  OMX_PTR * pp_port = NULL;
  tiz_vector_t * p_list = NULL;

  /* delete the config port */
  factory_delete (p_obj->p_cport_);
  p_obj->p_cport_ = NULL;

  /* delete all regular (non-config) ports */
  while (tiz_vector_length (p_obj->p_ports_) > 0)
    {
      pp_port = tiz_vector_back (p_obj->p_ports_);
      assert (pp_port);
      factory_delete (*pp_port);
      tiz_vector_pop_back (p_obj->p_ports_);
    }
  tiz_vector_destroy (p_obj->p_ports_);
  p_obj->p_ports_ = NULL;

  /* delete the ingress and egress lists */
  while (tiz_vector_length (p_obj->p_ingress_) > 0)
    {
      p_list = *(tiz_vector_t **) tiz_vector_back (p_obj->p_ingress_);
      tiz_vector_clear (p_list);
      tiz_vector_destroy (p_list);
      tiz_vector_pop_back (p_obj->p_ingress_);
    }
  tiz_vector_destroy (p_obj->p_ingress_);
  p_obj->p_ingress_ = NULL;

  while (tiz_vector_length (p_obj->p_egress_) > 0)
    {
      p_list = *(tiz_vector_t **) tiz_vector_back (p_obj->p_egress_);
      tiz_vector_clear (p_list);
      tiz_vector_destroy (p_list);
      tiz_vector_pop_back (p_obj->p_egress_);
    }
  tiz_vector_destroy (p_obj->p_egress_);
  p_obj->p_egress_ = NULL;
}

/*
 * tiz_krn construction / destruction
 */

static void *
krn_ctor (void * ap_obj, va_list * app)
{
  tiz_krn_t * p_obj = super_ctor (typeOf (ap_obj, "tizkrn"), ap_obj, app);
  tiz_check_omx_ret_null (init_ports_and_lists (p_obj));
  return p_obj;
}

static void *
krn_dtor (void * ap_obj)
{
  deinit_ports_and_lists (ap_obj);
  return super_dtor (typeOf (ap_obj, "tizkrn"), ap_obj);
}

/*
 * From tiz_api class
 */

static OMX_ERRORTYPE
krn_GetComponentVersion (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_STRING ap_comp_name,
                         OMX_VERSIONTYPE * ap_comp_version,
                         OMX_VERSIONTYPE * ap_spec_version,
                         OMX_UUIDTYPE * ap_comp_uuid)
{
  const tiz_krn_t * p_obj = ap_obj;
  assert (ap_obj);
  /* Delegate to the config port */
  return tiz_api_GetComponentVersion (p_obj->p_cport_, ap_hdl, ap_comp_name,
                                      ap_comp_version, ap_spec_version,
                                      ap_comp_uuid);
}

static OMX_ERRORTYPE
krn_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                  OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_krn_t * p_obj = ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_obj);

  TIZ_TRACE (ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  /* Find the port that holds the data */
  if (OMX_ErrorNone
      == (rc = tiz_krn_find_managing_port (p_obj, a_index, ap_struct, &p_port)))
    {
      assert (p_port);
      /* Delegate to that port */
      return tiz_api_GetParameter (p_port, ap_hdl, a_index, ap_struct);
    }

  if (OMX_ErrorUnsupportedIndex != rc)
    {
      TIZ_ERROR (ap_hdl,
                 "[%s] : Could not retrieve the managing "
                 "port for index [%s]",
                 tiz_err_to_str (rc), tiz_idx_to_str (a_index));
      return rc;
    }

  {
    OMX_PORT_PARAM_TYPE * p_struct = (OMX_PORT_PARAM_TYPE *) ap_struct;

    switch (a_index)
      {

        case OMX_IndexParamAudioInit:
          {
            *p_struct = p_obj->audio_init_;
            break;
          }

        case OMX_IndexParamVideoInit:
          {
            *p_struct = p_obj->video_init_;
            break;
          }

        case OMX_IndexParamImageInit:
          {
            *p_struct = p_obj->image_init_;
            break;
          }

        case OMX_IndexParamOtherInit:
          {
            *p_struct = p_obj->other_init_;
            break;
          }

        default:
          {
            TIZ_ERROR (ap_hdl, "[OMX_ErrorUnsupportedIndex] : [0x%08x] = [%s]...",
                       a_index, tiz_idx_to_str (a_index));
            return OMX_ErrorUnsupportedIndex;
          }
      };
  }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
krn_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                  OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_krn_t * p_obj = ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_obj);

  TIZ_TRACE (ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  /* Find the port that holds the data */
  if (OMX_ErrorNone
      == (rc = tiz_krn_find_managing_port (p_obj, a_index, ap_struct, &p_port)))
    {
      OMX_U32 mos_pid = 0; /* master's or slave's pid */
      assert (p_port);
      /* Delegate to the port */
      rc = tiz_api_SetParameter (p_port, ap_hdl, a_index, ap_struct);

      if (OMX_ErrorNone == rc && !TIZ_PORT_IS_CONFIG_PORT (p_port))
        {
          if (tiz_port_is_master_or_slave (p_port, &mos_pid) == true)
            {
              OMX_PTR * pp_mos_port = NULL;
              OMX_PTR p_mos_port = NULL;
              tiz_vector_t * p_changed_idxs = NULL;

              /* Retrieve the master or slave's port... */
              pp_mos_port = tiz_vector_at (p_obj->p_ports_, mos_pid);
              assert (pp_mos_port && *pp_mos_port);
              p_mos_port = *pp_mos_port;

              tiz_check_omx (
                tiz_vector_init (&(p_changed_idxs), sizeof (OMX_INDEXTYPE)));
              assert (p_changed_idxs);
              rc = tiz_port_apply_slaving_behaviour (
                p_mos_port, p_port, a_index, ap_struct, p_changed_idxs);

              if (OMX_ErrorNone == rc)
                {
                  const OMX_S32 nidxs = tiz_vector_length (p_changed_idxs);
                  int i = 0;

                  for (; i < nidxs; ++i)
                    {
                      OMX_INDEXTYPE * p_idx = tiz_vector_at (p_changed_idxs, i);
                      assert (p_idx != NULL);
                      /* Trigger here a port settings changed event */
                      tiz_srv_issue_event (p_obj, OMX_EventPortSettingsChanged,
                                           mos_pid, *p_idx, NULL);
                    }
                }

              tiz_vector_clear (p_changed_idxs);
              tiz_vector_destroy (p_changed_idxs);
            }
        }

      return rc;
    }

  if (OMX_ErrorUnsupportedIndex != rc)
    {
      TIZ_ERROR (ap_hdl,
                 "[%s] : Could not retrieve the managing "
                 "port for index [%s]",
                 tiz_err_to_str (rc), tiz_idx_to_str (a_index));
      return rc;
    }

  switch (a_index)
    {
      case OMX_IndexParamAudioInit:
      case OMX_IndexParamVideoInit:
      case OMX_IndexParamImageInit:
      case OMX_IndexParamOtherInit:
        {
          /* OMX_PORT_PARAM_TYPE structures are read only */
        }
      /*@fallthrough@*/
      default:
        {
          TIZ_ERROR (ap_hdl, "OMX_ErrorUnsupportedIndex [0x%08x]...", a_index);
          rc = OMX_ErrorUnsupportedIndex;
        }
    };

  return rc;
}

static OMX_ERRORTYPE
krn_GetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
               OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_krn_t * p_obj = ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_obj);

  TIZ_TRACE (ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  /* Find the port that holds the data */
  if (OMX_ErrorNone
      == (rc = tiz_krn_find_managing_port (p_obj, a_index, ap_struct, &p_port)))
    {
      /* Delegate to that port */
      assert (p_port);
      return tiz_api_GetConfig (p_port, ap_hdl, a_index, ap_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
krn_SetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
               OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_krn_t * p_obj = (tiz_krn_t *) ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_obj);

  /* Find the port that holds the data */
  if (OMX_ErrorNone
      == (rc = tiz_krn_find_managing_port (p_obj, a_index, ap_struct, &p_port)))
    {
      /* Delegate to that port */
      assert (p_port);
      rc = tiz_api_SetConfig (p_port, ap_hdl, a_index, ap_struct);
    }

  if (OMX_ErrorNone != rc)
    {
      TIZ_ERROR (ap_hdl, "[%s] : SetConfig (%s) ...", tiz_err_to_str (rc),
                 tiz_idx_to_str (a_index));
      return rc;
    }

  if (OMX_IndexConfigTunneledPortStatus == a_index)
    {
      const OMX_CONFIG_TUNNELEDPORTSTATUSTYPE * p_port_status
        = (OMX_CONFIG_TUNNELEDPORTSTATUSTYPE *) ap_struct;
      if (false == p_obj->accept_use_buffer_notified_
          && (p_port_status->nTunneledPortStatus
              & OMX_PORTSTATUS_ACCEPTUSEBUFFER)
               > 0
          && TIZ_KRN_MAY_INIT_ALLOC_PHASE (p_obj))
        {
          TIZ_TRACE (ap_hdl, "update fsm : TIZ_KRN_MAY_INIT_ALLOC_PHASE");
          p_obj->accept_use_buffer_notified_ = true;
          tiz_check_omx (
            tiz_fsm_tunneled_ports_status_update (tiz_get_fsm (ap_hdl)));
        }
      else if ((false == p_obj->accept_buffer_exchange_notified_)
               && (p_port_status->nTunneledPortStatus
                   & OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE)
                    > 0
               && TIZ_KRN_MAY_EXCHANGE_BUFFERS (p_obj))
        {
          TIZ_TRACE (ap_hdl, "update fsm : TIZ_KRN_MAY_EXCHANGE_BUFFERS");
          /*           p_obj->accept_buffer_exchange_notified_ = true; */
          tiz_check_omx (
            tiz_fsm_tunneled_ports_status_update (tiz_get_fsm (ap_hdl)));
        }
      else if ((false == p_obj->may_transition_exe2idle_notified_)
               && (p_port_status->nTunneledPortStatus
                   & OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN)
                    > 0
               && (TIZ_KRN_MAY_INIT_EXE_TO_IDLE (p_obj)))
        {
          TIZ_TRACE (ap_hdl, "update fsm : TIZ_KRN_MAY_INIT_EXE_TO_IDLE");
          p_obj->may_transition_exe2idle_notified_ = true;
          tiz_check_omx (
            tiz_fsm_tunneled_ports_status_update (tiz_get_fsm (ap_hdl)));
        }
      else
        {
          TIZ_TRACE (
            ap_hdl,
            "TIZ_KRN_MAY - NO NOTIFICATION!!! "
            "use_buffer [%s] buffer_exchange [%s] exe2idle [%s]",
            p_obj->accept_use_buffer_notified_ ? "TRUE" : "FALSE",
            p_obj->accept_buffer_exchange_notified_ ? "TRUE" : "FALSE",
            p_obj->may_transition_exe2idle_notified_ ? "TRUE" : "FALSE");
        }
    }
  else
    {
      /* There has been a successful update to a config structures, so let's
         tell the processor about it. */
      void * p_prc = tiz_get_prc (handleOf (p_obj));
      rc = tiz_api_SetConfig (p_prc, ap_hdl, a_index, ap_struct);
    }

  return rc;
}

static OMX_ERRORTYPE
krn_GetExtensionIndex (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                       OMX_STRING ap_param_name, OMX_INDEXTYPE * ap_index_type)
{
  const tiz_krn_t * p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_ERRORTYPE rc = OMX_ErrorUnsupportedIndex;
  OMX_PTR p_port = NULL;
  OMX_S32 i = 0;

  assert (ap_obj);

  nports = tiz_vector_length (p_obj->p_ports_);

  TIZ_TRACE (ap_hdl, "GetExtensionIndex [%s] nports [%d]...", ap_param_name,
             nports);

  /* Check every port to see if the extension is supported... */
  for (i = 0; i < nports && OMX_ErrorUnsupportedIndex == rc; ++i)
    {
      p_port = get_port (p_obj, i);
      rc = tiz_api_GetExtensionIndex (p_port, ap_hdl, ap_param_name,
                                      ap_index_type);
    }

  if (OMX_ErrorUnsupportedIndex == rc)
    {
      /* Now check the config port */
      rc = tiz_api_GetExtensionIndex (p_obj->p_cport_, ap_hdl, ap_param_name,
                                      ap_index_type);
    }

  return rc;
}

static OMX_ERRORTYPE
krn_SendCommand (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                 OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  tiz_krn_msg_t * p_msg = NULL;
  tiz_krn_msg_sendcommand_t * p_msg_sc = NULL;

  assert (ap_obj);

  TIZ_TRACE (ap_hdl, "SendCommand [%s]", tiz_cmd_to_str (a_cmd));

  TIZ_KRN_INIT_MSG_OOM (ap_obj, ap_hdl, p_msg, ETIZKrnMsgSendCommand);

  assert (p_msg);
  p_msg_sc = &(p_msg->sc);
  p_msg_sc->cmd = a_cmd;
  p_msg_sc->param1 = a_param1;
  p_msg_sc->p_cmd_data = ap_cmd_data;
  return tiz_srv_enqueue (ap_obj, p_msg, cmd_to_priority (a_cmd));
}

static OMX_ERRORTYPE
krn_ComponentTunnelRequest (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                            OMX_U32 a_pid, OMX_HANDLETYPE ap_thdl,
                            OMX_U32 a_tpid, OMX_TUNNELSETUPTYPE * ap_tsetup)
{
  const tiz_krn_t * p_obj = ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_obj);

  if (check_pid (p_obj, a_pid) != OMX_ErrorNone)
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Retrieve the port... */
  p_port = get_port (p_obj, a_pid);

  /* Check tunnel being torn down... */
  if (NULL == ap_thdl)
    {
      /* Delegate to the port */
      rc = tiz_api_ComponentTunnelRequest (p_port, ap_hdl, a_pid, ap_thdl,
                                           a_tpid, ap_tsetup);

      return rc;
    }

  /* Check port being re-tunnelled... */
  if (TIZ_PORT_IS_TUNNELED (p_port))
    {
      /* TODO */
    }

  /* Delegate to the port... */
  if (OMX_ErrorNone != (rc = tiz_api_ComponentTunnelRequest (
                          p_port, ap_hdl, a_pid, ap_thdl, a_tpid, ap_tsetup)))
    {
      TIZ_ERROR (ap_hdl,
                 "[%s] : While delegating "
                 "ComponentTunnelRequest to port [%d]",
                 tiz_err_to_str (rc), a_pid);
      return rc;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
krn_UseBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
               OMX_BUFFERHEADERTYPE ** app_hdr, OMX_U32 a_pid,
               OMX_PTR ap_apppriv, OMX_U32 a_size, OMX_U8 * ap_buf)
{
  tiz_krn_t * p_obj = (tiz_krn_t *) ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_fsm_state_id_t now = EStateMax;

  assert (ap_obj);

  now = tiz_fsm_get_substate (tiz_get_fsm (ap_hdl));

  if (check_pid (p_obj, a_pid) != OMX_ErrorNone)
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Retrieve the port... */
  p_port = get_port (p_obj, a_pid);

  /* Check that in case of tunnelling, this port is not a buffer supplier... */
  if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
    {
      TIZ_ERROR (ap_hdl,
                 "[OMX_ErrorBadPortIndex] : Bad port index"
                 "(port is tunneled)...");
      return OMX_ErrorBadPortIndex;
    }

  /* Now delegate to the port... */
  {
    const bool was_being_enabled = TIZ_PORT_IS_BEING_ENABLED (p_port);
    if (OMX_ErrorNone
        != (rc = tiz_api_UseBuffer (p_port, ap_hdl, app_hdr, a_pid, ap_apppriv,
                                    a_size, ap_buf)))
      {
        TIZ_ERROR (ap_hdl,
                   "[%s] : While delegating UseBuffer "
                   "to port [%d]",
                   tiz_err_to_str (rc), a_pid);
        return rc;
      }

    if (was_being_enabled && TIZ_PORT_IS_POPULATED (p_port))
      {
        tiz_check_omx (
          complete_port_enable (p_obj, p_port, a_pid, OMX_ErrorNone));
      }
  }

  if (OMX_ErrorNone == rc && all_populated (p_obj))
    {
      TIZ_TRACE (ap_hdl, "AllPortsPopulated : [TRUE]");
      if (ESubStateLoadedToIdle == now)
        {
          rc = tiz_fsm_complete_transition (tiz_get_fsm (ap_hdl), ap_obj,
                                            OMX_StateIdle);
        }
    }

  return rc;
}

static OMX_ERRORTYPE
krn_UseEGLImage (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                 OMX_BUFFERHEADERTYPE ** app_hdr, OMX_U32 a_pid,
                 OMX_PTR ap_apppriv, OMX_U8 * ap_eglimage)
{
  tiz_krn_t * p_obj = (tiz_krn_t *) ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_fsm_state_id_t now = EStateMax;

  assert (ap_obj);

  now = tiz_fsm_get_substate (tiz_get_fsm (ap_hdl));

  if (check_pid (p_obj, a_pid) != OMX_ErrorNone)
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Retrieve the port... */
  p_port = get_port (p_obj, a_pid);

  /* Check that in case of tunnelling, this port is not a buffer supplier... */
  if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
    {
      TIZ_ERROR (ap_hdl,
                 "[OMX_ErrorBadPortIndex] : Bad port index"
                 "(port is tunneled)...");
      return OMX_ErrorBadPortIndex;
    }

  /* Now delegate to the port... */
  {
    const bool was_being_enabled = TIZ_PORT_IS_BEING_ENABLED (p_port);
    if (OMX_ErrorNone
        != (rc = tiz_api_UseEGLImage (p_port, ap_hdl, app_hdr, a_pid,
                                      ap_apppriv, ap_eglimage)))
      {
        TIZ_ERROR (ap_hdl,
                   "[%s] : While delegating UseEGLImage "
                   "to port [%d]",
                   tiz_err_to_str (rc), a_pid);
        return rc;
      }

    if (was_being_enabled && TIZ_PORT_IS_POPULATED (p_port))
      {
        tiz_check_omx (
          complete_port_enable (p_obj, p_port, a_pid, OMX_ErrorNone));
      }
  }

  if (OMX_ErrorNone == rc && all_populated (p_obj))
    {
      TIZ_TRACE (ap_hdl, "AllPortsPopulated : [TRUE]");
      if (ESubStateLoadedToIdle == now)
        {
          rc = tiz_fsm_complete_transition (tiz_get_fsm (ap_hdl), ap_obj,
                                            OMX_StateIdle);
        }
    }

  return rc;
}

static OMX_ERRORTYPE
krn_AllocateBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                    OMX_BUFFERHEADERTYPE ** app_hdr, OMX_U32 a_pid,
                    OMX_PTR ap_apppriv, OMX_U32 a_size)
{
  tiz_krn_t * p_obj = (tiz_krn_t *) ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_fsm_state_id_t now = EStateMax;

  assert (ap_obj);

  now = tiz_fsm_get_substate (tiz_get_fsm (ap_hdl));

  TIZ_TRACE (ap_hdl, "AllocateBuffer...");

  if (check_pid (p_obj, a_pid) != OMX_ErrorNone)
    {
      return OMX_ErrorBadPortIndex;
    }

  /* Grab the port here... */
  p_port = get_port (p_obj, a_pid);

  /* Check that in case of tunnelling, this port is not a buffer supplier... */
  if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
    {
      TIZ_ERROR (ap_hdl,
                 "[OMX_ErrorBadPortIndex] : port [%d] "
                 "is supplier...",
                 a_pid);
      return OMX_ErrorBadPortIndex;
    }

  /* Now delegate to the port... */
  {
    const bool was_being_enabled = TIZ_PORT_IS_BEING_ENABLED (p_port);
    if (OMX_ErrorNone
        != (rc = tiz_api_AllocateBuffer (p_port, ap_hdl, app_hdr, a_pid,
                                         ap_apppriv, a_size)))
      {
        TIZ_ERROR (ap_hdl,
                   "[%s] : While delegating AllocateBuffer "
                   "to port [%d]",
                   tiz_err_to_str (rc), a_pid);
        return rc;
      }

    if (was_being_enabled && TIZ_PORT_IS_POPULATED (p_port))
      {
        tiz_check_omx (
          complete_port_enable (p_obj, p_port, a_pid, OMX_ErrorNone));
      }
  }

  if (OMX_ErrorNone == rc && all_populated (p_obj))
    {
      TIZ_TRACE (ap_hdl, "AllPortsPopulated : [TRUE]");
      if (ESubStateLoadedToIdle == now)
        {
          rc = tiz_fsm_complete_transition (tiz_get_fsm (ap_hdl), ap_obj,
                                            OMX_StateIdle);
        }
    }

  if (OMX_ErrorNone == rc)
    {
      assert (*app_hdr);
    }

  return rc;
}

static OMX_ERRORTYPE
krn_FreeBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl, OMX_U32 a_pid,
                OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const tiz_krn_t * p_obj = ap_obj;
  OMX_PTR p_port = NULL;
  bool issue_unpop = false;
  tiz_fsm_state_id_t now = EStateMax;

  assert (ap_obj);

  now = tiz_fsm_get_substate (tiz_get_fsm (ap_hdl));

  if (check_pid (p_obj, a_pid) != OMX_ErrorNone)
    {
      return OMX_ErrorBadPortIndex;
    }

  p_port = get_port (p_obj, a_pid);

  TIZ_TRACE (ap_hdl, "FreeBuffer : PORT [%d] STATE [%s]", a_pid,
             tiz_fsm_state_to_str (now));

  /* Check that in case of tunnelling, this is not buffer supplier... */
  if (TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
    {
      TIZ_ERROR (ap_hdl,
                 "[OMX_ErrorBadPortIndex] : port [%d] "
                 "is supplier...",
                 a_pid);
      return OMX_ErrorBadPortIndex;
    }

  if (ESubStateIdleToLoaded != now && TIZ_PORT_IS_ENABLED (p_port)
      && TIZ_PORT_IS_POPULATED (p_port))
    {
      /* The port should be disabled. */
      /* The buffer deallocation will raise an OMX_ErrorPortUnpopulated  */
      /* error in the current state. */
      issue_unpop = true;
    }

  {
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    OMX_S32 buf_count;
    const bool was_being_disabled = TIZ_PORT_IS_BEING_DISABLED (p_port);
    /* Delegate to the port... */
    if (OMX_ErrorNone
        != (rc = tiz_api_FreeBuffer (p_port, ap_hdl, a_pid, ap_hdr)))
      {
        TIZ_DEBUG (ap_hdl,
                   "[%s] when delegating FreeBuffer to "
                   "the port",
                   tiz_err_to_str (rc));
        return rc;
      }

    if (issue_unpop)
      {
        tiz_srv_issue_err_event (p_obj, OMX_ErrorPortUnpopulated);
      }

    buf_count = tiz_port_buffer_count (p_port);

    if (buf_count <= 0 && was_being_disabled)
      {
        tiz_check_omx (complete_port_disable ((tiz_krn_t *) p_obj, p_port,
                                              a_pid, OMX_ErrorNone));
      }
  }

  return complete_ongoing_transitions (p_obj, ap_hdl);
}

static OMX_ERRORTYPE
krn_EmptyThisBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                     OMX_BUFFERHEADERTYPE * ap_hdr)
{
  tiz_krn_msg_t * p_msg = NULL;
  tiz_krn_msg_emptyfillbuffer_t * p_etb = NULL;

  assert (ap_obj);

  TIZ_TRACE (ap_hdl, "HEADER [%p] BUFFER [%p] PORT [%d]", ap_hdr,
             ap_hdr->pBuffer, ap_hdr->nInputPortIndex);

  TIZ_KRN_INIT_MSG_OOM (ap_obj, ap_hdl, p_msg, ETIZKrnMsgEmptyThisBuffer);

  assert (p_msg);
  p_etb = &(p_msg->ef);
  p_etb->p_hdr = ap_hdr;
  return tiz_srv_enqueue (ap_obj, p_msg, 2);
}

static OMX_ERRORTYPE
krn_FillThisBuffer (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                    OMX_BUFFERHEADERTYPE * ap_hdr)
{
  tiz_krn_msg_t * p_msg = NULL;
  tiz_krn_msg_emptyfillbuffer_t * p_msg_ftb = NULL;

  assert (ap_obj);

  TIZ_TRACE (ap_hdl, "HEADER [%p] BUFFER [%p] PORT [%d]", ap_hdr,
             ap_hdr->pBuffer, ap_hdr->nOutputPortIndex);

  TIZ_KRN_INIT_MSG_OOM (ap_obj, ap_hdl, p_msg, ETIZKrnMsgFillThisBuffer);

  assert (p_msg);
  p_msg_ftb = &(p_msg->ef);
  p_msg_ftb->p_hdr = ap_hdr;
  return tiz_srv_enqueue (ap_obj, p_msg, 2);
}

/*
 * from tiz_srv api
 */

static OMX_ERRORTYPE
krn_dispatch_msg (const void * ap_obj, OMX_PTR ap_msg)
{
  tiz_krn_t * p_obj = (tiz_krn_t *) ap_obj;
  tiz_krn_msg_t * p_msg = ap_msg;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);
  assert (p_msg);

  TIZ_TRACE (handleOf (p_obj), "Processing [%s]...",
             krn_msg_to_str (p_msg->class));

  assert (p_msg->class < ETIZKrnMsgMax);
  rc = tiz_krn_msg_to_fnt_tbl[p_msg->class]((OMX_PTR) p_obj, p_msg);
  return rc;
}

static OMX_ERRORTYPE
krn_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  tiz_krn_t * p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_U32 pid = 0;
  OMX_S32 i = 0;

  assert (ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  TIZ_TRACE (handleOf (p_obj), "port index [%d]...", a_pid);

  /* Verify the port index.. */
  if ((OMX_ALL != a_pid) && (check_pid (p_obj, a_pid) != OMX_ErrorNone))
    {
      return OMX_ErrorBadPortIndex;
    }

  do
    {
      pid = ((OMX_ALL != a_pid) ? a_pid : i);
      p_port = get_port (p_obj, pid);

      /* This function will do nothing if it doesn't have to, e.g. because the
       * port isn't tunneled, or is disabled, etc. */
      tiz_port_update_tunneled_status (p_port, OMX_PORTSTATUS_ACCEPTUSEBUFFER);

      TIZ_TRACE (handleOf (p_obj),
                 "pid [%d] enabled [%s] tunneled [%s] "
                 "supplier [%s] populated [%s]..",
                 pid, TIZ_PORT_IS_ENABLED (p_port) ? "YES" : "NO",
                 TIZ_PORT_IS_TUNNELED (p_port) ? "YES" : "NO",
                 TIZ_PORT_IS_SUPPLIER (p_port) ? "YES" : "NO",
                 TIZ_PORT_IS_POPULATED (p_port) ? "YES" : "NO");

      if (TIZ_PORT_IS_ENABLED_TUNNELED_SUPPLIER_AND_NOT_POPULATED (p_port))
        {
          const bool being_enabled = TIZ_PORT_IS_BEING_ENABLED (p_port);
          if (OMX_ErrorNone != (rc = tiz_port_populate (p_port)))
            {
              TIZ_ERROR (handleOf (p_obj), "[%s] : While populating port [%d] ",
                         tiz_err_to_str (rc), pid);
              return rc;
            }

          if (being_enabled && TIZ_PORT_IS_POPULATED_AND_ENABLED (p_port))
            {
              tiz_check_omx (
                complete_port_enable (p_obj, p_port, pid, OMX_ErrorNone));
            }
        }

      ++i;
    }
  while (OMX_ALL == a_pid && i < nports);

  return rc;
}

static OMX_ERRORTYPE
krn_deallocate_resources (void * ap_obj)
{
  tiz_krn_t * p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_S32 i = 0;

  assert (ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  for (i = 0; i < nports; ++i)
    {
      p_port = get_port (p_obj, i);

      if (TIZ_PORT_IS_ENABLED_TUNNELED_AND_SUPPLIER (p_port))
        {
          if (OMX_ErrorNone != (rc = tiz_port_depopulate (p_port)))
            {
              break;
            }
        }
    }

  TIZ_TRACE (handleOf (p_obj), "[%s] : ALL depopulated [%s]...",
             tiz_err_to_str (rc), all_depopulated (p_obj) ? "TRUE" : "FALSE");
  return rc;
}

static OMX_ERRORTYPE
krn_prepare_to_transfer (void * ap_obj, OMX_U32 a_pid)
{
  tiz_krn_t * p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_U32 pid = 0;
  OMX_S32 i = 0;

  assert (ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  TIZ_TRACE (handleOf (p_obj), "PORT [%d]", a_pid);

  if ((OMX_ALL != a_pid) && (check_pid (p_obj, a_pid) != OMX_ErrorNone))
    {
      return OMX_ErrorBadPortIndex;
    }

  clear_hdr_lsts (p_obj, a_pid);

  do
    {
      pid = ((OMX_ALL != a_pid) ? a_pid : i);
      p_port = get_port (p_obj, pid);

      if (TIZ_PORT_IS_ENABLED_TUNNELED_AND_SUPPLIER (p_port))
        {
          const OMX_DIRTYPE dir = tiz_port_dir (p_port);
          tiz_vector_t *p_dst2darr, *p_srclst = tiz_port_get_hdrs_list (p_port);
          assert (OMX_DirInput == dir || OMX_DirOutput == dir);

          /* Input port -> Add header to egress list... */
          /* Output port -> Add header to ingress list... */
          p_dst2darr
            = (OMX_DirInput == dir ? p_obj->p_egress_ : p_obj->p_ingress_);

          if (OMX_ErrorNone
              != (rc = append_buflsts (p_dst2darr, p_srclst, pid)))
            {
              TIZ_ERROR (handleOf (p_obj),
                         "[%s] : on port [%d] while appending "
                         "buffer lists",
                         tiz_err_to_str (rc), pid);
              return rc;
            }
        }

      ++i;
    }
  while (OMX_ALL == a_pid && i < nports);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
krn_transfer_and_process (void * ap_obj, OMX_U32 a_pid)
{
  tiz_krn_t * p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_PTR p_port = NULL;
  OMX_U32 pid = 0;
  OMX_S32 i = 0;

  assert (ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  TIZ_TRACE (handleOf (p_obj), "PORT [%d]", a_pid);

  if ((OMX_ALL != a_pid) && (check_pid (p_obj, a_pid) != OMX_ErrorNone))
    {
      return OMX_ErrorBadPortIndex;
    }

  do
    {
      pid = ((OMX_ALL != a_pid) ? a_pid : i);
      p_port = get_port (p_obj, pid);

      /* This function will do nothing if it doesn't have to, e.g. because the
       * port isn't tunneled, or is disabled, etc. */
      tiz_port_update_tunneled_status (p_port,
                                       OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE);
      tiz_check_omx (flush_egress (p_obj, pid, OMX_FALSE));
      tiz_check_omx (propagate_ingress (p_obj, pid));
      i++;
    }
  while (OMX_ALL == a_pid && i < nports);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
krn_stop_and_return (void * ap_obj)
{
  tiz_krn_t * p_obj = ap_obj;
  OMX_S32 nports = 0;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_port = NULL;
  OMX_S32 i = 0, nbufs = 0;

  assert (ap_obj);
  nports = tiz_vector_length (p_obj->p_ports_);

  TIZ_TRACE (handleOf (p_obj), "stop and return...[%p]", ap_obj);

  for (i = 0; i < nports && OMX_ErrorNone == rc; ++i)
    {
      p_port = get_port (p_obj, i);

      if (TIZ_PORT_IS_DISABLED (p_port) || tiz_port_buffer_count (p_port) <= 0)
        {
          continue;
        }

      /* This will move any ETB or FTB messages currently queued in the
         kernel's servant queue into the corresponding port ingress list. This
         guarantees that all buffers received by the component are correctly
         returned during stop */
      tiz_srv_remove_from_queue (ap_obj, &process_efb_from_servant_queue,
                                 OMX_ALL, p_obj);

      /* This will move any processor callbacks currently queued in the
         kernel's servant queue into the corresponding port egress list. This
         guarantees that all buffers held by the component are correctly
         returned during stop */
      tiz_srv_remove_from_queue (ap_obj, &process_cbacks_from_servant_queue,
                                 OMX_ALL, p_obj);

      /* OMX_CommandFlush is used to notify the processor servant that it has
         to return some buffers ... */
      if (TIZ_PORT_GET_CLAIMED_COUNT (p_port) >= 0)
        {
          void * p_prc = tiz_get_prc (handleOf (p_obj));
          if (OMX_ErrorNone
              != (rc = tiz_api_SendCommand (p_prc, handleOf (p_obj),
                                            OMX_CommandFlush, i, NULL)))
            {
              break;
            }
        }

      if (TIZ_PORT_IS_ENABLED_TUNNELED_AND_SUPPLIER (p_port))
        {
          /* Move buffers from egress to ingress */
          nbufs = move_to_ingress (p_obj, i);
          TIZ_TRACE (handleOf (p_obj), "Moved [%d] tunnel buffers to ingress",
                     nbufs);
          if (nbufs < 0)
            {
              TIZ_ERROR (handleOf (p_obj),
                         "[OMX_ErrorInsufficientResources] - nbufs [%d]",
                         nbufs);
              rc = OMX_ErrorInsufficientResources;
            }

          /* This function will do nothing if the port is disabled, for
           * example */
          tiz_port_update_tunneled_status (
            p_port, OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN);

          continue;
        }

      /* Move buffers from ingress to egress */
      nbufs = move_to_egress (p_obj, i);
      TIZ_TRACE (handleOf (p_obj), "Moved [%d] non-tunnel buffers to egress",
                 nbufs);
      if (nbufs < 0)
        {
          TIZ_ERROR (handleOf (p_obj),
                     "[OMX_ErrorInsufficientResources] - nbufs [%d]", nbufs);
          rc = OMX_ErrorInsufficientResources;
        }

      if (OMX_ErrorNone == rc)
        {
          rc = flush_egress (p_obj, i, OMX_FALSE);
        }

      if (OMX_ErrorNone == rc)
        {
          /* Flush buffer marks and complete commands as required */
          rc = flush_marks (p_obj, p_port);
        }
    }

  if (OMX_ErrorNone != rc)
    {
      TIZ_ERROR (handleOf (p_obj), "[%s]", tiz_err_to_str (rc));
    }

  return rc;
}

static OMX_ERRORTYPE
krn_receive_pluggable_event (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                             tiz_event_pluggable_t * ap_event)
{
  tiz_krn_msg_t * p_msg = NULL;
  tiz_krn_msg_plg_event_t * p_plgevt = NULL;

  assert (ap_obj);

  TIZ_KRN_INIT_MSG_OOM (ap_obj, ap_hdl, p_msg, ETIZKrnMsgPluggableEvent);

  assert (p_msg);
  p_plgevt = &(p_msg->pe);
  p_plgevt->p_event = ap_event;
  return tiz_srv_enqueue (ap_obj, p_msg, 2);
}

/*
 * API from tiz_krn
 */

static OMX_ERRORTYPE
krn_register_port (void * ap_obj, OMX_PTR ap_port, const bool ais_config)
{
  tiz_krn_t * p_obj = ap_obj;

  assert (ap_obj);
  assert (ap_port);

  if (ais_config)
    {
      assert (NULL == p_obj->p_cport_);
      p_obj->p_cport_ = ap_port;
      tiz_port_set_index (ap_port, TIZ_PORT_CONFIG_PORT_INDEX);
      return OMX_ErrorNone;
    }

  {
    /* Create the corresponding ingress and egress lists */
    tiz_vector_t * p_in_list = NULL;
    tiz_vector_t * p_out_list = NULL;
    OMX_U32 pid = 0;
    tiz_check_omx (
      tiz_vector_init (&(p_in_list), sizeof (OMX_BUFFERHEADERTYPE *)));
    assert (p_in_list);
    tiz_check_omx (
      tiz_vector_init (&(p_out_list), sizeof (OMX_BUFFERHEADERTYPE *)));
    assert (p_out_list);
    tiz_check_omx (tiz_vector_push_back (p_obj->p_ingress_, &p_in_list));
    tiz_check_omx (tiz_vector_push_back (p_obj->p_egress_, &p_out_list));

    pid = tiz_vector_length (p_obj->p_ports_);
    tiz_port_set_index (ap_port, pid);

    switch (tiz_port_domain (ap_port))
      {
        case OMX_PortDomainAudio:
          {
            if (0 == p_obj->audio_init_.nPorts)
              {
                p_obj->audio_init_.nStartPortNumber = pid;
              }
            p_obj->audio_init_.nPorts++;
            break;
          }

        case OMX_PortDomainVideo:
          {
            if (0 == p_obj->video_init_.nPorts)
              {
                p_obj->video_init_.nStartPortNumber = pid;
              }
            p_obj->video_init_.nPorts++;
            break;
          }

        case OMX_PortDomainImage:
          {
            if (0 == p_obj->image_init_.nPorts)
              {
                p_obj->image_init_.nStartPortNumber = pid;
              }
            p_obj->image_init_.nPorts++;
            break;
          }

        case OMX_PortDomainOther:
          {
            if (0 == p_obj->other_init_.nPorts)
              {
                p_obj->other_init_.nStartPortNumber = pid;
              }
            p_obj->other_init_.nPorts++;
            break;
          }

        default:
          {
            assert (0);
          }
      };
    TIZ_TRACE (handleOf (p_obj),
               "audio ports [%d], video ports [%d], "
               "image ports [%d], other ports [%d]",
               p_obj->audio_init_.nPorts, p_obj->video_init_.nPorts,
               p_obj->image_init_.nPorts, p_obj->other_init_.nPorts);
    /* TODO Assert that this port is not repeated in the array */
    return tiz_vector_push_back (p_obj->p_ports_, &ap_port);
  }
}

OMX_ERRORTYPE
tiz_krn_register_port (const void * ap_obj, OMX_PTR ap_port,
                       const bool ais_config)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->register_port);
  return class->register_port (ap_obj, ap_port, ais_config);
}

OMX_ERRORTYPE
tiz_krn_super_register_port (const void * a_class, const void * ap_obj,
                             OMX_PTR ap_port, const bool ais_config)
{
  const tiz_krn_class_t * superclass = super (a_class);
  assert (ap_obj && superclass->register_port);
  return superclass->register_port (ap_obj, ap_port, ais_config);
}

static void *
krn_get_port (const void * ap_obj, const OMX_U32 a_pid)
{
  const tiz_krn_t * p_obj = ap_obj;
  OMX_S32 num_ports = 0;

  assert (ap_obj);
  num_ports = tiz_vector_length (p_obj->p_ports_);

  if (num_ports <= a_pid)
    {
      return NULL;
    }

  return get_port (p_obj, a_pid);
}

void *
tiz_krn_get_port (const void * ap_obj, const OMX_U32 a_pid)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->get_port);
  return class->get_port (ap_obj, a_pid);
}

OMX_ERRORTYPE
krn_find_managing_port (const tiz_krn_t * ap_krn, const OMX_INDEXTYPE a_index,
                        const OMX_PTR ap_struct, OMX_PTR * app_port)
{
  OMX_ERRORTYPE rc = OMX_ErrorUnsupportedIndex;
  OMX_PTR * pp_port = NULL;
  OMX_U32 * p_port_index;

  assert (ap_krn);
  assert (app_port);
  assert (ap_struct);

  if (OMX_ErrorNone == tiz_port_find_index (ap_krn->p_cport_, a_index))
    {
      *app_port = ap_krn->p_cport_;
      TIZ_TRACE (handleOf (ap_krn),
                 "[%s] : Config port being searched. "
                 "Returning...",
                 tiz_idx_to_str (a_index));
      return OMX_ErrorNone;
    }
  else
    {
      OMX_S32 i = 0;
      OMX_S32 num_ports = tiz_vector_length (ap_krn->p_ports_);
      for (i = 0; i < num_ports; ++i)
        {
          pp_port = tiz_vector_at (ap_krn->p_ports_, i);
          if (OMX_ErrorNone == tiz_port_find_index (*pp_port, a_index))
            {
              rc = OMX_ErrorNone;
              break;
            }
        }

      if (OMX_ErrorNone == rc)
        {
          /* Now we retrieve the port index from the struct. */
          /* TODO: This is not the best way to do this */
          p_port_index = (OMX_U32 *) ap_struct
                         + sizeof (OMX_U32) / sizeof (OMX_U32)
                         + sizeof (OMX_VERSIONTYPE) / sizeof (OMX_U32);

          if (OMX_ErrorNone != (rc = check_pid (ap_krn, *p_port_index)))
            {
              return rc;
            }

          TIZ_TRACE (handleOf (ap_krn), "[%s] : Found in port index [%d]...",
                     tiz_idx_to_str (a_index), *p_port_index);

          pp_port = tiz_vector_at (ap_krn->p_ports_, *p_port_index);
          *app_port = *pp_port;
          return rc;
        }
    }

  TIZ_TRACE (handleOf (ap_krn), "[%s] : Could not find the managing port...",
             tiz_idx_to_str (a_index));

  return rc;
}

OMX_ERRORTYPE
tiz_krn_find_managing_port (const void * ap_obj, const OMX_INDEXTYPE a_index,
                            const OMX_PTR ap_struct, OMX_PTR * app_port)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->find_managing_port);
  return class->find_managing_port (ap_obj, a_index, ap_struct, app_port);
}

static tiz_krn_population_status_t
krn_get_population_status (const void * ap_obj, const OMX_U32 a_pid,
                           OMX_BOOL * ap_may_be_fully_unpopulated)
{
  const tiz_krn_t * p_obj = ap_obj;
  tiz_krn_population_status_t status = ETIZKrnFullyPopulated;

  assert (ap_obj);

  if (OMX_ALL == a_pid)
    {
      if (all_populated (p_obj))
        {
          status = ETIZKrnFullyPopulated;
        }
      else if (all_depopulated (p_obj))
        {
          status = ETIZKrnFullyUnpopulated;
        }
      else
        {
          OMX_S32 i, nports = tiz_vector_length (p_obj->p_ports_);
          OMX_PTR p_port = NULL;

          assert (ap_may_be_fully_unpopulated);

          status = ETIZKrnUnpopulated;
          *ap_may_be_fully_unpopulated = OMX_TRUE;

          /* Loop through all regular (non-config) ports */
          for (i = 0; i < nports; ++i)
            {
              p_port = get_port (p_obj, i);

              if (tiz_port_buffer_count (p_port) > 0
                  && !TIZ_PORT_IS_SUPPLIER (p_port)
                  && TIZ_PORT_IS_TUNNELED (p_port))
                {
                  /* There is a non-supplier, tunneled port that is being
                   * populated. This means we cannot be fully unpopulated
                   * without help from the tunneled component */
                  *ap_may_be_fully_unpopulated = OMX_FALSE;
                  break;
                }
            }
        }
    }
  else
    {
      OMX_S32 nports = tiz_vector_length (p_obj->p_ports_);
      OMX_PTR p_port = NULL;

      assert (a_pid < nports);

      p_port = get_port (p_obj, a_pid);

      if (TIZ_PORT_IS_POPULATED (p_port))
        {
          status = ETIZKrnFullyPopulated;
        }
      else if (tiz_port_buffer_count (p_port) == 0)
        {
          status = ETIZKrnFullyUnpopulated;
        }
      else
        {
          assert (ap_may_be_fully_unpopulated);

          status = ETIZKrnUnpopulated;
          *ap_may_be_fully_unpopulated = OMX_TRUE;

          if (!TIZ_PORT_IS_SUPPLIER (p_port))
            {
              /* A non-supplier port that is being populated */
              *ap_may_be_fully_unpopulated = OMX_FALSE;
            }
        }
    }

  return status;
}

tiz_krn_population_status_t
tiz_krn_get_population_status (const void * ap_obj, const OMX_U32 a_pid,
                               OMX_BOOL * ap_may_be_fully_unpopulated)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->get_population_status);
  return class->get_population_status (ap_obj, a_pid,
                                       ap_may_be_fully_unpopulated);
}

tiz_krn_population_status_t
tiz_krn_super_get_population_status (const void * a_class, const void * ap_obj,
                                     const OMX_U32 a_pid,
                                     OMX_BOOL * ap_may_be_fully_unpopulated)
{
  const tiz_krn_class_t * superclass = super (a_class);
  assert (ap_obj && superclass->get_population_status);
  return superclass->get_population_status (ap_obj, a_pid,
                                            ap_may_be_fully_unpopulated);
}

static OMX_ERRORTYPE
krn_select (const void * ap_obj, const OMX_U32 a_nports, tiz_pd_set_t * ap_set)
{
  const tiz_krn_t * p_obj = ap_obj;
  OMX_S32 i = 0;
  OMX_S32 nports = 0;
  tiz_vector_t * p_list = NULL;

  assert (ap_obj);
  assert (ap_set);

  nports = tiz_vector_length (p_obj->p_ports_);

  if (a_nports < nports)
    {
      nports = a_nports;
    }

  /* Loop through the first nports in the ingress list */
  for (i = 0; i < nports; ++i)
    {
      p_list = get_ingress_lst (p_obj, i);
      if (tiz_vector_length (p_list) > 0)
        {
          TIZ_PD_SET (i, ap_set);
        }
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_krn_select (const void * ap_obj, const OMX_U32 a_nports,
                /*@out@*/ tiz_pd_set_t * ap_set)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->select);
  return class->select (ap_obj, a_nports, ap_set);
}

OMX_ERRORTYPE
tiz_krn_super_select (const void * a_class, const void * ap_obj,
                      const OMX_U32 a_nports, tiz_pd_set_t * ap_set)
{
  const tiz_krn_class_t * superclass = super (a_class);
  assert (ap_obj && superclass->select);
  return superclass->select (ap_obj, a_nports, ap_set);
}

static OMX_ERRORTYPE
krn_claim_buffer (const void * ap_obj, const OMX_U32 a_pid, const OMX_U32 a_pos,
                  OMX_BUFFERHEADERTYPE ** app_hdr)
{
  tiz_krn_t * p_obj = (tiz_krn_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  tiz_vector_t * p_list = NULL;
  OMX_PTR p_port = NULL;

  assert (ap_obj);
  assert (check_pid (p_obj, a_pid) == OMX_ErrorNone);
  assert (app_hdr);

  /* Find the port.. */
  p_port = get_port (p_obj, a_pid);

  TIZ_TRACE (handleOf (p_obj), "port's [%d] a_pos [%d] buf count [%d]...",
             a_pid, a_pos, tiz_port_buffer_count (p_port));

  /* Buffers can't be claimed in OMX_StatePause state */
  assert (EStatePause != tiz_fsm_get_substate (tiz_get_fsm (handleOf (p_obj))));

  /* Buffers can't be claimed on an disabled port */
  assert (TIZ_PORT_IS_ENABLED (p_port));

  /* Buffer position shall not be larger or equal than the buffer count */
  assert (a_pos < tiz_port_buffer_count (p_port));

  /* Grab the port's ingress list  */
  p_list = get_ingress_lst (p_obj, a_pid);

  /* Ingress list's size shall not be larger than the port's buffer count */
  assert (tiz_vector_length (p_list) <= tiz_port_buffer_count (p_port));

  /* Only try to retrieve the buffer if that position exists in the list */
  if (a_pos < tiz_vector_length (p_list))
    {
      OMX_DIRTYPE pdir = OMX_DirMax;

      /* Retrieve the header from the port's ingress list... */
      p_hdr = get_header (p_list, a_pos);
      assert (p_hdr);

      TIZ_TRACE (handleOf (p_obj),
                 "port's [%d] HEADER [%p] BUFFER [%p] ingress "
                 "list length [%d]...",
                 a_pid, p_hdr, p_hdr->pBuffer, tiz_vector_length (p_list));

      pdir = tiz_port_dir (p_port);

      if (OMX_DirOutput == pdir)
        {
          /* If it's an output port and allocator, ask the port to allocate the
           * actual
           * buffer, in case pre-announcements have been disabled on this port. This
           * function call has no effect if pre-announcements are enabled on the
           * port. */
          if (TIZ_PORT_IS_ALLOCATOR (p_port))
            {
              tiz_check_omx (tiz_port_populate_header (p_port, p_hdr));
            }
          /* Make sure there is no data from a previous transition into
             OMX_StateExecuting */
          tiz_clear_header (p_hdr);
        }

      /* ... and delete it from the list */
      tiz_vector_erase (p_list, a_pos, 1);

      /* Now increment by one the claimed buffers count on this port */
      (void) TIZ_PORT_INC_CLAIMED_COUNT (p_port);

      /* ...and if its an input buffer, mark the header, if any marks
       * available... */
      if (OMX_DirInput == pdir)
        {
          /* NOTE: tiz_port_mark_buffer returns OMX_ErrorNone if the port marked
           * the
           * buffer with one of its own marks */
          if (OMX_ErrorNone == (rc = tiz_port_mark_buffer (p_port, p_hdr)))
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

  *app_hdr = p_hdr;

  if (OMX_ErrorNone != rc)
    {
      TIZ_ERROR (handleOf (p_obj), "[%s]", tiz_err_to_str (rc));
    }

  return rc;
}

OMX_ERRORTYPE
tiz_krn_claim_buffer (const void * ap_obj, const OMX_U32 a_pid,
                      const OMX_U32 a_pos, OMX_BUFFERHEADERTYPE ** app_hdr)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->claim_buffer);
  return class->claim_buffer (ap_obj, a_pid, a_pos, app_hdr);
}

OMX_ERRORTYPE
tiz_krn_super_claim_buffer (const void * a_class, const void * ap_obj,
                            const OMX_U32 a_pid, const OMX_U32 a_pos,
                            OMX_BUFFERHEADERTYPE ** app_hdr)
{
  const tiz_krn_class_t * superclass = super (a_class);
  assert (ap_obj && superclass->claim_buffer);
  return superclass->claim_buffer (ap_obj, a_pid, a_pos, app_hdr);
}

static OMX_ERRORTYPE
krn_release_buffer (const void * ap_obj, const OMX_U32 a_pid,
                    OMX_BUFFERHEADERTYPE * ap_hdr)
{
  tiz_krn_t * p_obj = (tiz_krn_t *) ap_obj;
  tiz_vector_t * p_list = NULL;
  OMX_PTR p_port = NULL;

  assert (ap_obj);
  assert (ap_hdr);
  assert (check_pid (p_obj, a_pid) == OMX_ErrorNone);

  /* Buffer headers associated to EGLImages don't have an associated buffer */
  if (ap_hdr->pBuffer)
    {
      assert ((ap_hdr->nOffset + ap_hdr->nFilledLen) <= ap_hdr->nAllocLen);
    }

  /* Find the port.. */
  p_port = get_port (p_obj, a_pid);

  /* TODO : Verify that the header effectively belongs to the given port */
  /* assert (tiz_port_find_buffer(p_port, ap_hdr, is_owned)); */

  /* Grab the port's egress list */
  p_list = get_egress_lst (p_obj, a_pid);

  TIZ_TRACE (handleOf (p_obj), "HEADER [%p] pid [%d] egress length [%d]...",
             ap_hdr, a_pid, tiz_vector_length (p_list));

  assert (tiz_vector_length (p_list) < tiz_port_buffer_count (p_port));

  return enqueue_callback_msg (p_obj, ap_hdr, a_pid, tiz_port_dir (p_port));
}

OMX_ERRORTYPE
tiz_krn_release_buffer (const void * ap_obj, const OMX_U32 a_pid,
                        OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->release_buffer);
  return class->release_buffer (ap_obj, a_pid, ap_hdr);
}

OMX_ERRORTYPE
tiz_krn_super_release_buffer (const void * a_class, const void * ap_obj,
                              const OMX_U32 a_pid,
                              OMX_BUFFERHEADERTYPE * ap_hdr)
{
  const tiz_krn_class_t * superclass = super (a_class);
  assert (ap_obj && superclass->release_buffer);
  return superclass->release_buffer (ap_obj, a_pid, ap_hdr);
}

static OMX_ERRORTYPE
krn_claim_eglimage (const void * ap_obj, const OMX_U32 a_pid,
                    const OMX_BUFFERHEADERTYPE * ap_hdr, OMX_PTR * app_eglimage)
{
  tiz_krn_t * p_obj = (tiz_krn_t *) ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_obj);
  assert (ap_hdr);
  assert (app_eglimage);

  if (check_pid (p_obj, a_pid) != OMX_ErrorNone)
    {
      return OMX_ErrorBadPortIndex;
    }

  if (!ap_hdr || !app_eglimage)
    {
      return OMX_ErrorBadParameter;
    }

  /* Grab the port here... */
  p_port = get_port (p_obj, a_pid);
  if (p_port)
    {
      *app_eglimage = tiz_port_get_eglimage (p_port, ap_hdr);
    }

  if (!(*app_eglimage))
    {
      rc = OMX_ErrorInsufficientResources;
    }

  return rc;
}

OMX_ERRORTYPE
tiz_krn_claim_eglimage (const void * ap_obj, const OMX_U32 a_pid,
                        const OMX_BUFFERHEADERTYPE * ap_hdr,
                        OMX_PTR * app_eglimage)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->claim_eglimage);
  return class->claim_eglimage (ap_obj, a_pid, ap_hdr, app_eglimage);
}

OMX_ERRORTYPE
tiz_krn_super_claim_eglimage (const void * a_class, const void * ap_obj,
                              const OMX_U32 a_pid,
                              const OMX_BUFFERHEADERTYPE * ap_hdr,
                              OMX_PTR * app_eglimage)
{
  const tiz_krn_class_t * superclass = super (a_class);
  assert (ap_obj && superclass->claim_eglimage);
  return superclass->claim_eglimage (ap_obj, a_pid, ap_hdr, app_eglimage);
}

/* TODO: Do not ignore return codes */
static void
krn_deregister_all_ports (void * ap_obj)
{
  tiz_krn_t * p_obj = (tiz_krn_t *) ap_obj;
  /* Reset kernel data structures */
  deinit_ports_and_lists (p_obj);
  (void) init_ports_and_lists (ap_obj);
}

void
tiz_krn_deregister_all_ports (void * ap_obj)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->deregister_all_ports);
  class->deregister_all_ports (ap_obj);
}

void
tiz_krn_super_deregister_all_ports (const void * a_class, void * ap_obj)
{
  const tiz_krn_class_t * superclass = super (a_class);
  assert (ap_obj && superclass->deregister_all_ports);
  superclass->deregister_all_ports (ap_obj);
}

static void
krn_reset_tunneled_ports_status (void * ap_obj,
                                 const OMX_U32 a_port_status_flag)
{
  tiz_krn_t * p_obj = ap_obj;

  assert (ap_obj);

  switch (a_port_status_flag)
    {
      case OMX_PORTSTATUS_ACCEPTUSEBUFFER:
        {
          TIZ_TRACE (handleOf (p_obj),
                     "Reset [OMX_PORTSTATUS_ACCEPTUSEBUFFER]...");
          p_obj->accept_use_buffer_notified_ = false;
        }
        break;

      case OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE:
        {
          TIZ_TRACE (handleOf (p_obj),
                     "Reset [OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE]...");
          p_obj->accept_buffer_exchange_notified_ = false;
        }
        break;

      case OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN:
        {
          TIZ_TRACE (handleOf (p_obj),
                     "Reset [OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN]...");
          p_obj->may_transition_exe2idle_notified_ = false;
        }
        break;

      default:
        {
          assert (0);
        }
    };

  {
    OMX_S32 i = tiz_vector_length (p_obj->p_ports_) - 1;

    do
      {
        tiz_port_reset_tunneled_port_status_flag (get_port (p_obj, i),
                                                  a_port_status_flag);
      }
    while (i-- != 0);
  }
}

void
tiz_krn_reset_tunneled_ports_status (void * ap_obj,
                                     const OMX_U32 a_port_status_flag)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->reset_tunneled_ports_status);
  class->reset_tunneled_ports_status (ap_obj, a_port_status_flag);
}

void
tiz_krn_super_reset_tunneled_ports_status (const void * a_class, void * ap_obj,
                                           const OMX_U32 a_port_status_flag)
{
  const tiz_krn_class_t * superclass = super (a_class);
  assert (ap_obj && superclass->reset_tunneled_ports_status);
  superclass->reset_tunneled_ports_status (ap_obj, a_port_status_flag);
}

static bool
is_ready_for_alloc_phase (const tiz_krn_t * ap_obj)
{
  bool outcome = true;

  assert (ap_obj);

  {
    const OMX_S32 nports = tiz_vector_length (ap_obj->p_ports_);
    OMX_S32 i = 0;
    OMX_PTR p_port = NULL;
    for (i = 0; i < nports; ++i)
      {
        p_port = get_port (ap_obj, i);
        if (p_port)
          {
            if (TIZ_PORT_IS_ENABLED (p_port)
                && TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
              {
                if (!TIZ_PORT_MAY_CALL_USE_BUFFER (p_port))
                  {
                    return false;
                  }
              }
          }
      }
  }

  return outcome;
}

static bool
is_ready_to_exchange_buffers (const tiz_krn_t * ap_obj)
{
  bool outcome = true;

  assert (ap_obj);

  {
    const OMX_S32 nports = tiz_vector_length (ap_obj->p_ports_);
    OMX_S32 i = 0;
    OMX_PTR p_port = NULL;
    for (i = 0; i < nports; ++i)
      {
        p_port = get_port (ap_obj, i);
        if (p_port)
          {
            if (TIZ_PORT_IS_ENABLED (p_port)
                && TIZ_PORT_IS_TUNNELED_AND_SUPPLIER (p_port))
              {
                if (!TIZ_PORT_MAY_EXCHANGE_BUFFERS (p_port))
                  {
                    return false;
                  }
              }
          }
      }
  }

  return outcome;
}

static bool
is_ready_for_exe_to_idle (const tiz_krn_t * ap_obj)
{
  bool outcome = true;

  assert (ap_obj);

  {
    const OMX_S32 nports = tiz_vector_length (ap_obj->p_ports_);
    OMX_S32 i = 0;
    OMX_PTR p_port = NULL;
    for (i = 0; i < nports; ++i)
      {
        p_port = get_port (ap_obj, i);
        if (p_port)
          {
            if (TIZ_PORT_IS_ENABLED (p_port) && TIZ_PORT_IS_TUNNELED (p_port))
              {
                if (!TIZ_PORT_MAY_INITIATE_EXE_TO_IDLE (p_port))
                  {
                    return false;
                  }
              }
          }
      }
  }

  return outcome;
}

static bool
krn_get_restriction_status (const void * ap_obj,
                            const tiz_krn_restriction_t a_restriction)
{
  bool rc = false;
  assert (a_restriction < ETIZKrnMayMax);

  switch (a_restriction)
    {
      case ETIZKrnMayInitiateAllocPhase:
        {
          rc = is_ready_for_alloc_phase (ap_obj);
        }
        break;

      case ETIZKrnMayExchangeBuffers:
        {
          rc = is_ready_to_exchange_buffers (ap_obj);
        }
        break;

      case ETIZKrnMayInitiateExeToIdle:
        {
          rc = is_ready_for_exe_to_idle (ap_obj);
        }
        break;
      default:
        {
          assert (0);
        }
    };

  return rc;
}

bool
tiz_krn_get_restriction_status (const void * ap_obj,
                                const tiz_krn_restriction_t a_restriction)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->get_restriction_status);
  return class->get_restriction_status (ap_obj, a_restriction);
}

static void
krn_clear_metadata (void * ap_obj)
{
  tiz_krn_t * p_obj = ap_obj;
  assert (p_obj);
  tiz_configport_clear_metadata (p_obj->p_cport_);
}

void
tiz_krn_clear_metadata (void * ap_obj)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->clear_metadata);
  return class->clear_metadata (ap_obj);
}

static OMX_ERRORTYPE
krn_store_metadata (void * ap_obj,
                    const OMX_CONFIG_METADATAITEMTYPE * ap_meta_item)
{
  tiz_krn_t * p_obj = ap_obj;
  assert (p_obj);
  return tiz_configport_store_metadata (p_obj->p_cport_, ap_meta_item);
}

OMX_ERRORTYPE
tiz_krn_store_metadata (void * ap_obj,
                        const OMX_CONFIG_METADATAITEMTYPE * ap_meta_item)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->store_metadata);
  return class->store_metadata (ap_obj, ap_meta_item);
}

static OMX_ERRORTYPE
krn_SetParameter_internal (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                           OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_krn_t * p_obj = ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_obj);

  TIZ_TRACE (ap_hdl, "[%s]...", tiz_idx_to_str (a_index));

  /* Find the port that holds the data */
  if (OMX_ErrorNone
      == (rc = tiz_krn_find_managing_port (p_obj, a_index, ap_struct, &p_port)))
    {
      assert (p_port);
      /* Delegate to the port */
      rc = tiz_port_SetParameter_internal (p_port, ap_hdl, a_index, ap_struct);
    }
  return rc;
}

OMX_ERRORTYPE
tiz_krn_SetParameter_internal (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                               OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->SetParameter_internal);
  return class->SetParameter_internal (ap_obj, ap_hdl, a_index, ap_struct);
}

static OMX_ERRORTYPE
krn_SetConfig_internal (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_krn_t * p_obj = (tiz_krn_t *) ap_obj;
  OMX_PTR p_port = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_obj);

  /* Find the port that holds the data */
  if (OMX_ErrorNone
      == (rc = tiz_krn_find_managing_port (p_obj, a_index, ap_struct, &p_port)))
    {
      /* Delegate to that port */
      assert (p_port);
      rc = tiz_port_SetConfig_internal (p_port, ap_hdl, a_index, ap_struct);
    }
  return rc;
}

OMX_ERRORTYPE
tiz_krn_SetConfig_internal (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                            OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_krn_class_t * class = classOf (ap_obj);
  assert (class->SetConfig_internal);
  return class->SetConfig_internal (ap_obj, ap_hdl, a_index, ap_struct);
}

/*
 * tiz_krn_class
 */

static void *
krn_class_ctor (void * ap_obj, va_list * app)
{
  tiz_krn_class_t * p_obj
    = super_ctor (typeOf (ap_obj, "tizkrn_class"), ap_obj, app);
  typedef void (*voidf) ();
  voidf selector = NULL;
  va_list ap;
  va_copy (ap, *app);

  /* NOTE: Start ignoring splint warnings in this section of code */
  /*@ignore@*/
  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) tiz_krn_register_port)
        {
          *(voidf *) &p_obj->register_port = method;
        }
      else if (selector == (voidf) tiz_krn_get_port)
        {
          *(voidf *) &p_obj->get_port = method;
        }
      else if (selector == (voidf) tiz_krn_find_managing_port)
        {
          *(voidf *) &p_obj->find_managing_port = method;
        }
      else if (selector == (voidf) tiz_krn_get_population_status)
        {
          *(voidf *) &p_obj->get_population_status = method;
        }
      else if (selector == (voidf) tiz_krn_select)
        {
          *(voidf *) &p_obj->select = method;
        }
      else if (selector == (voidf) tiz_krn_claim_buffer)
        {
          *(voidf *) &p_obj->claim_buffer = method;
        }
      else if (selector == (voidf) tiz_krn_release_buffer)
        {
          *(voidf *) &p_obj->release_buffer = method;
        }
      else if (selector == (voidf) tiz_krn_claim_eglimage)
        {
          *(voidf *) &p_obj->claim_eglimage = method;
        }
      else if (selector == (voidf) tiz_krn_deregister_all_ports)
        {
          *(voidf *) &p_obj->deregister_all_ports = method;
        }
      else if (selector == (voidf) tiz_krn_reset_tunneled_ports_status)
        {
          *(voidf *) &p_obj->reset_tunneled_ports_status = method;
        }
      else if (selector == (voidf) tiz_krn_get_restriction_status)
        {
          *(voidf *) &p_obj->get_restriction_status = method;
        }
      else if (selector == (voidf) tiz_krn_clear_metadata)
        {
          *(voidf *) &p_obj->clear_metadata = method;
        }
      else if (selector == (voidf) tiz_krn_store_metadata)
        {
          *(voidf *) &p_obj->store_metadata = method;
        }
      else if (selector == (voidf) tiz_krn_SetParameter_internal)
        {
          *(voidf *) &p_obj->SetParameter_internal = method;
        }
      else if (selector == (voidf) tiz_krn_SetConfig_internal)
        {
          *(voidf *) &p_obj->SetConfig_internal = method;
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
tiz_krn_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizsrv = tiz_get_type (ap_hdl, "tizsrv");
  void * tizkrn_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizsrv), "tizkrn_class", classOf (tizsrv),
     sizeof (tiz_krn_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, krn_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizkrn_class;
}

void *
tiz_krn_init (void * ap_tos, void * ap_hdl)
{
  void * tizsrv = tiz_get_type (ap_hdl, "tizsrv");
  void * tizkrn_class = tiz_get_type (ap_hdl, "tizkrn_class");
  TIZ_LOG_CLASS (tizkrn_class);
  void * tizkrn = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizkrn_class, "tizkrn", tizsrv, sizeof (tiz_krn_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, krn_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, krn_dtor,
     /* TIZ_CLASS_COMMENT: GetComponentVersion */
     tiz_api_GetComponentVersion, krn_GetComponentVersion,
     /* TIZ_CLASS_COMMENT: GetParameter */
     tiz_api_GetParameter, krn_GetParameter,
     /* TIZ_CLASS_COMMENT: SetParameter */
     tiz_api_SetParameter, krn_SetParameter,
     /* TIZ_CLASS_COMMENT: GetConfig */
     tiz_api_GetConfig, krn_GetConfig,
     /* TIZ_CLASS_COMMENT: SetConfig */
     tiz_api_SetConfig, krn_SetConfig,
     /* TIZ_CLASS_COMMENT: GetExtensionIndex */
     tiz_api_GetExtensionIndex, krn_GetExtensionIndex,
     /* TIZ_CLASS_COMMENT: SendCommand */
     tiz_api_SendCommand, krn_SendCommand,
     /* TIZ_CLASS_COMMENT: ComponentTunnelRequest */
     tiz_api_ComponentTunnelRequest, krn_ComponentTunnelRequest,
     /* TIZ_CLASS_COMMENT: UseBuffer */
     tiz_api_UseBuffer, krn_UseBuffer,
     /* TIZ_CLASS_COMMENT: UseEGLImage */
     tiz_api_UseEGLImage, krn_UseEGLImage,
     /* TIZ_CLASS_COMMENT: AllocateBuffer */
     tiz_api_AllocateBuffer, krn_AllocateBuffer,
     /* TIZ_CLASS_COMMENT: FreeBuffer */
     tiz_api_FreeBuffer, krn_FreeBuffer,
     /* TIZ_CLASS_COMMENT: EmptyThisBuffer */
     tiz_api_EmptyThisBuffer, krn_EmptyThisBuffer,
     /* TIZ_CLASS_COMMENT: FillThisBuffer */
     tiz_api_FillThisBuffer, krn_FillThisBuffer,
     /* TIZ_CLASS_COMMENT: dispatch_msg */
     tiz_srv_dispatch_msg, krn_dispatch_msg,
     /* TIZ_CLASS_COMMENT: allocate_resources */
     tiz_srv_allocate_resources, krn_allocate_resources,
     /* TIZ_CLASS_COMMENT: deallocate_resources */
     tiz_srv_deallocate_resources, krn_deallocate_resources,
     /* TIZ_CLASS_COMMENT: prepare_to_transfer */
     tiz_srv_prepare_to_transfer, krn_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: transfer_and_process */
     tiz_srv_transfer_and_process, krn_transfer_and_process,
     /* TIZ_CLASS_COMMENT: stop_and_return */
     tiz_srv_stop_and_return, krn_stop_and_return,
     /* TIZ_CLASS_COMMENT: receive_pluggable_event */
     tiz_srv_receive_pluggable_event, krn_receive_pluggable_event,
     /* TIZ_CLASS_COMMENT: register_port */
     tiz_krn_register_port, krn_register_port,
     /* TIZ_CLASS_COMMENT: get_port */
     tiz_krn_get_port, krn_get_port,
     /* TIZ_CLASS_COMMENT: find_managing_port */
     tiz_krn_find_managing_port, krn_find_managing_port,
     /* TIZ_CLASS_COMMENT: get_population_status */
     tiz_krn_get_population_status, krn_get_population_status,
     /* TIZ_CLASS_COMMENT: select */
     tiz_krn_select, krn_select,
     /* TIZ_CLASS_COMMENT: claim_buffer */
     tiz_krn_claim_buffer, krn_claim_buffer,
     /* TIZ_CLASS_COMMENT: release_buffer */
     tiz_krn_release_buffer, krn_release_buffer,
     /* TIZ_CLASS_COMMENT: claim_eglimage */
     tiz_krn_claim_eglimage, krn_claim_eglimage,
     /* TIZ_CLASS_COMMENT: deregister_all_ports */
     tiz_krn_deregister_all_ports, krn_deregister_all_ports,
     /* TIZ_CLASS_COMMENT: tunneled_ports_status */
     tiz_krn_reset_tunneled_ports_status, krn_reset_tunneled_ports_status,
     /* TIZ_CLASS_COMMENT: restriction_status */
     tiz_krn_get_restriction_status, krn_get_restriction_status,
     /* TIZ_CLASS_COMMENT: */
     tiz_krn_clear_metadata, krn_clear_metadata,
     /* TIZ_CLASS_COMMENT: */
     tiz_krn_store_metadata, krn_store_metadata,
     /* TIZ_CLASS_COMMENT: */
     tiz_krn_SetParameter_internal, krn_SetParameter_internal,
     /* TIZ_CLASS_COMMENT: */
     tiz_krn_SetConfig_internal, krn_SetConfig_internal,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizkrn;
}
