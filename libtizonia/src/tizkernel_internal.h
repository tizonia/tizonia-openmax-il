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
 * @file   tizkernel_internal.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - kernel class - private types and declarations
 *
 *
 */

#ifndef TIZKERNEL_INTERNAL_H
#define TIZKERNEL_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>

  /*
   * OMX_CommandStateSet dispatching
   */

typedef OMX_ERRORTYPE (*tiz_krn_msg_dispatch_state_set_f)(tiz_krn_t *ap_krn,
                                                          bool * ap_done);
  /* Forward declarations */
static OMX_ERRORTYPE dispatch_idle_to_loaded (tiz_krn_t *ap_krn,
                                                bool * ap_done);
static OMX_ERRORTYPE dispatch_loaded_to_idle (tiz_krn_t *ap_krn,
                                              bool * ap_done);
static OMX_ERRORTYPE dispatch_exe_or_pause_to_idle (tiz_krn_t *ap_krn,
                                                    bool * ap_done);
static OMX_ERRORTYPE dispatch_idle_to_exe (tiz_krn_t *ap_krn,
                                           bool * ap_done);
static OMX_ERRORTYPE dispatch_pause_to_exe (tiz_krn_t *ap_krn,
                                            bool * ap_done);
static OMX_ERRORTYPE dispatch_exe_to_exe (tiz_krn_t *ap_krn,
                                            bool * ap_done);
static OMX_ERRORTYPE dispatch_exe_or_idle_to_pause (tiz_krn_t *ap_krn,
                                                    bool * ap_done);
static OMX_ERRORTYPE dispatch_true (tiz_krn_t *ap_krn,
                                    bool * ap_done);
static OMX_ERRORTYPE dispatch_false (tiz_krn_t *ap_krn,
                                     bool * ap_done);

/**
 * Function table for the kernel servant state machine. Do not re-order, it
 * is indexed by the OMX_STATETYPE indices.
 */
static const tiz_krn_msg_dispatch_state_set_f tiz_krn_state_set_dispatch_tbl[][OMX_StateWaitForResources + 1] =
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

#endif /* TIZKERNEL_INTERNAL_H */
