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
 * @file   tizprc_internal.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - processor class - private types
 *
 *
 */

#ifndef TIZPRC_INTERNAL_H
#define TIZPRC_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>

/* Forward declarations */
static OMX_ERRORTYPE dispatch_sc (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_br (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_config (void *ap_obj, OMX_PTR ap_msg);
static OMX_ERRORTYPE dispatch_dr (void *ap_obj, OMX_PTR ap_msg);

typedef struct tiz_prc_msg_sendcommand tiz_prc_msg_sendcommand_t;

static OMX_ERRORTYPE dispatch_state_set (const void *ap_obj,
                                         OMX_HANDLETYPE p_hdl,
                                         tiz_prc_msg_sendcommand_t *ap_msg_sc);
static OMX_ERRORTYPE dispatch_port_disable (
    const void *ap_obj, OMX_HANDLETYPE p_hdl,
    tiz_prc_msg_sendcommand_t *ap_msg_sc);
static OMX_ERRORTYPE dispatch_port_enable (
    const void *ap_obj, OMX_HANDLETYPE p_hdl,
    tiz_prc_msg_sendcommand_t *ap_msg_sc);
static OMX_ERRORTYPE dispatch_port_flush (const void *ap_obj,
                                          OMX_HANDLETYPE p_hdl,
                                          tiz_prc_msg_sendcommand_t *ap_msg_sc);

/**
 * Top-level command types understood by the processor servant. Do not
 * re-order, it used to index the tiz_prc_msg_to_fnt_tbl table.
 */
typedef enum tiz_prc_msg_class tiz_prc_msg_class_t;
enum tiz_prc_msg_class
{
  ETIZPrcMsgSendCommand = 0,
  ETIZPrcMsgBuffersReady,
  ETIZPrcMsgConfig,
  ETIZPrcMsgDeferredResume,
  ETIZPrcMsgMax,
};

struct tiz_prc_msg_sendcommand
{
  OMX_COMMANDTYPE cmd;
  OMX_U32 param1;
  OMX_PTR p_cmd_data;
};

typedef struct tiz_prc_msg_buffersready tiz_prc_msg_buffersready_t;
struct tiz_prc_msg_buffersready
{
  OMX_BUFFERHEADERTYPE *p_buffer;
  OMX_U32 pid;
};

typedef struct tiz_prc_msg_configchange tiz_prc_msg_configchange_t;
struct tiz_prc_msg_configchange
{
  OMX_U32 pid;
  OMX_INDEXTYPE index;
};

typedef struct tiz_prc_msg_deferredresume tiz_prc_msg_deferredresume_t;
struct tiz_prc_msg_deferredresume
{
  OMX_STATETYPE target_state;
};

/**
 * Message structure for commands in the processor servant.
 */
typedef struct tiz_prc_msg tiz_prc_msg_t;
struct tiz_prc_msg
{
  OMX_HANDLETYPE p_hdl;
  tiz_prc_msg_class_t class;
  union
  {
    tiz_prc_msg_sendcommand_t sc;
    tiz_prc_msg_buffersready_t br;
    tiz_prc_msg_configchange_t cc;
    tiz_prc_msg_deferredresume_t dr;
  };
};

typedef OMX_ERRORTYPE (*tiz_prc_msg_dispatch_f)(void *ap_obj, OMX_PTR ap_msg);

/**
 * Function table for the top-level command types understood by the processor
 * servant. Do not re-order, it is indexed by the tiz_prc_msg_class_t indices.
 */
static const tiz_prc_msg_dispatch_f tiz_prc_msg_to_fnt_tbl[]
    = { dispatch_sc, dispatch_br, dispatch_config, dispatch_dr };

typedef OMX_ERRORTYPE (*tiz_prc_msg_dispatch_sc_f)(
    const void *ap_obj, OMX_HANDLETYPE p_hdl,
    tiz_prc_msg_sendcommand_t *ap_msg_sc);

/**
 * Function table for the OMX_SendCommand message types understood by the
 * processor servant. Do not re-order, it is indexed by the OMX_COMMANDTYPE
 * indices. NOTE that OMX_CommandMarkBuffer is not handled.
 */
static const tiz_prc_msg_dispatch_sc_f tiz_prc_msg_dispatch_sc_to_fnt_tbl[]
    = { dispatch_state_set, dispatch_port_flush, dispatch_port_disable,
        dispatch_port_enable };

typedef struct tiz_prc_msg_str tiz_prc_msg_str_t;
struct tiz_prc_msg_str
{
  tiz_prc_msg_class_t msg;
  OMX_STRING str;
};

static const tiz_prc_msg_str_t tiz_prc_msg_to_str_tbl[]
= { { ETIZPrcMsgSendCommand, "ETIZPrcMsgSendCommand" },
    { ETIZPrcMsgBuffersReady, "ETIZPrcMsgBuffersReady" },
    { ETIZPrcMsgConfig, "ETIZPrcMsgConfig" },
    { ETIZPrcMsgDeferredResume, "ETIZPrcMsgDeferredResume" },
    { ETIZPrcMsgMax, "ETIZPrcMsgMax" }, };



/*
 * OMX_CommandStateSet dispatching
 */

typedef OMX_ERRORTYPE (*tiz_prc_msg_dispatch_state_set_f)(tiz_prc_t *ap_prc,
                                                          bool * ap_done);
/* Forward declarations */
static OMX_ERRORTYPE dispatch_idle_to_loaded (tiz_prc_t *ap_prc,
                                                bool * ap_done);
static OMX_ERRORTYPE dispatch_loaded_to_idle (tiz_prc_t *ap_prc,
                                              bool * ap_done);
static OMX_ERRORTYPE dispatch_exe_or_pause_to_idle (tiz_prc_t *ap_prc,
                                                    bool * ap_done);
static OMX_ERRORTYPE dispatch_idle_to_exe (tiz_prc_t *ap_prc,
                                           bool * ap_done);
static OMX_ERRORTYPE dispatch_pause_to_exe (tiz_prc_t *ap_prc,
                                            bool * ap_done);
static OMX_ERRORTYPE dispatch_exe_to_exe (tiz_prc_t *ap_prc,
                                            bool * ap_done);
static OMX_ERRORTYPE dispatch_exe_or_idle_to_pause (tiz_prc_t *ap_prc,
                                                    bool * ap_done);
static OMX_ERRORTYPE dispatch_true (tiz_prc_t *ap_prc,
                                    bool * ap_done);
static OMX_ERRORTYPE dispatch_false (tiz_prc_t *ap_prc,
                                     bool * ap_done);

/**
 * Function table for the processor servant state machine. Do not re-order, it
 * is indexed by the OMX_STATETYPE indices.
 */
static const tiz_prc_msg_dispatch_state_set_f tiz_prc_state_set_dispatch_tbl[][OMX_StateWaitForResources + 1] =
  {
    /* From reserved */
    /* reserved , OMX_StateLoaded , OMX_StateIdle , OMX_StateExecuting , OMX_StatePause , OMX_StateWaitForResources */
    {dispatch_false, dispatch_false, dispatch_false , dispatch_false, dispatch_false, dispatch_false},

    /* From Loaded */
    /* reserved , OMX_StateLoaded , OMX_StateIdle , OMX_StateExecuting , OMX_StatePause , OMX_StateWaitForResources */
    {dispatch_false, dispatch_false, dispatch_loaded_to_idle , dispatch_false, dispatch_false, dispatch_true},

    /* From Idle */
    /* reserved , OMX_StateLoaded , OMX_StateIdle , OMX_StateExecuting , OMX_StatePause , OMX_StateWaitForResources */
    {dispatch_false, dispatch_idle_to_loaded, dispatch_false, dispatch_idle_to_exe, dispatch_exe_or_idle_to_pause, dispatch_false},

    /* From Exe */
    /* reserved , OMX_StateLoaded , OMX_StateIdle , OMX_StateExecuting , OMX_StatePause , OMX_StateWaitForResources */
    {dispatch_false, dispatch_false, dispatch_exe_or_pause_to_idle, dispatch_exe_to_exe, dispatch_exe_or_idle_to_pause, dispatch_false},

    /* From Pause */
    /* reserved , OMX_StateLoaded , OMX_StateIdle , OMX_StateExecuting , OMX_StatePause , OMX_StateWaitForResources */
    {dispatch_false, dispatch_false, dispatch_exe_or_pause_to_idle, dispatch_pause_to_exe, dispatch_false, dispatch_false},

    /* From WaitForResources */
    /* reserved , OMX_StateLoaded , OMX_StateIdle , OMX_StateExecuting , OMX_StatePause , OMX_StateWaitForResources */
    {dispatch_false, dispatch_true, dispatch_false, dispatch_false, dispatch_false, dispatch_false},
  };

#ifdef __cplusplus
}
#endif

#endif /* TIZPRC_INTERNAL_H */
