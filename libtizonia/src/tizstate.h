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
 * @file   tizstate.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - FSM's state base class
 *
 *
 */

#ifndef TIZSTATE_H
#define TIZSTATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tizapi.h"
#include "tizloaded.h"
#include "tizloadedtoidle.h"
#include "tizwaitforresources.h"
#include "tizidle.h"
#include "tizidletoloaded.h"
#include "tizidletoexecuting.h"
#include "tizexecuting.h"
#include "tizexecutingtoidle.h"
#include "tizpause.h"
#include "tizpausetoidle.h"

void *
tiz_state_class_init (void * ap_tos, void * ap_hdl);
void *
tiz_state_init (void * ap_tos, void * ap_hdl);

OMX_ERRORTYPE
tiz_state_state_set (const void * p_obj, OMX_HANDLETYPE ap_hdl,
                     OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                     OMX_PTR ap_cmd_data);

OMX_ERRORTYPE
tiz_state_flush (const void * p_obj, OMX_HANDLETYPE ap_hdl,
                 OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data);

OMX_ERRORTYPE
tiz_state_disable (const void * p_obj, OMX_HANDLETYPE ap_hdl,
                   OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                   OMX_PTR ap_cmd_data);

OMX_ERRORTYPE
tiz_state_enable (const void * p_obj, OMX_HANDLETYPE ap_hdl,
                  OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data);

OMX_ERRORTYPE
tiz_state_mark (const void * p_obj, OMX_HANDLETYPE ap_hdl,
                OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data);

OMX_ERRORTYPE
tiz_state_trans_complete (const void * p_obj, OMX_PTR ap_servant,
                          OMX_STATETYPE a_new_state);

OMX_ERRORTYPE
tiz_state_tunneled_ports_status_update (void * ap_obj);

#ifdef __cplusplus
}
#endif

#endif /* TIZSTATE_H */
