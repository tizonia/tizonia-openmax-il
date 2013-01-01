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
 * @file   tizfsm_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - OMX IL FSM implementation
 *
 *
 */

#ifndef TIZFSM_DECLS_H
#define TIZFSM_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tizfsm.h"
#include "tizservant_decls.h"

  struct tizfsm
  {
    /* Object */
    const struct tizservant _;
    void *p_states_[EStateMax];
    tizfsm_state_id_t cur_state_id_;
    tizfsm_state_id_t canceled_substate_id_;
    void *p_current_state_;
  };

  struct tizfsm_class
  {
    /* Class */
    const struct tizservant_class _;
    OMX_ERRORTYPE (*set_state) (void *p_obj,
                                tizfsm_state_id_t a_new_state,
                                tizfsm_state_id_t a_canceled_substate);
    OMX_ERRORTYPE (*complete_transition) (const void *p_obj,
                                          const void * ap_servant,
                                          OMX_STATETYPE a_new_state);
    tizfsm_state_id_t (*get_substate) (const void *ap_obj);
  };

#ifdef __cplusplus
}
#endif

#endif /* TIZFSM_DECLS_H */
