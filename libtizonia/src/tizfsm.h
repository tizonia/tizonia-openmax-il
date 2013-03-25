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
 * @file   tizfsm.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - OMX IL FSM
 *
 *
 */

#ifndef TIZFSM_H
#define TIZFSM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tizservant.h"

  typedef enum tizfsm_state_id tizfsm_state_id_t;
  enum tizfsm_state_id
  {
    EStateReserved_0x00000000 = OMX_StateReserved_0x00000000,
    EStateLoaded = OMX_StateLoaded,
    EStateIdle = OMX_StateIdle,
    EStateExecuting = OMX_StateExecuting,
    EStatePause = OMX_StatePause,
    EStateWaitForResources = OMX_StateWaitForResources,
    ESubStateLoadedToIdle,
    ESubStateIdleToLoaded,
    ESubStateIdleToExecuting,
    ESubStateExecutingToIdle,
    ESubStatePauseToIdle,
    EStateMax
  };

  /* factory_new(tizfsm, ...) */
  extern const void *tizfsm, *tizfsm_class;

  OMX_ERRORTYPE tizfsm_set_state (void *ap_obj,
                                  tizfsm_state_id_t a_new_state,
                                  tizfsm_state_id_t a_canceled_state);

  OMX_ERRORTYPE tizfsm_complete_transition (void *ap_obj,
                                            const void *ap_servant,
                                            OMX_STATETYPE a_new_state);

  OMX_ERRORTYPE tizfsm_complete_command (void *ap_obj,
                                         const void *ap_servant,
                                         OMX_COMMANDTYPE a_cmd,
                                         OMX_U32 a_param1);

  tizfsm_state_id_t tizfsm_get_substate (const void *ap_obj);

  OMX_ERRORTYPE tizfsm_tunneled_ports_status_update (void *ap_obj);

  void init_tizfsm (void);

#ifdef __cplusplus
}
#endif

#endif                          /* TIZFSM_H */
