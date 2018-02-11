/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   tizthread.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - Thread/task management
 *
 *
 */

#ifndef TIZTHREAD_H
#define TIZTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tizthread Thread/task management
 *
 * Simple thread/task management wrapper functions.
 *
 * @ingroup libtizplatform
 */

#include <OMX_Core.h>
#include <OMX_Types.h>

#include "tizsync.h"

/**
 * Thread hdl
 * @ingroup tizthread
 */
typedef OMX_U32 tiz_thread_t;

/**
 * Create a new thread, starting with execution of a_pf_routine getting
 * passed ap_arg.  The new hdl is stored in *ap_thread.
 *
 * @ingroup tizthread
 *
 * @return OMX_ErrorNone if success, OMX_ErrorUndefined otherwise.
 */
OMX_ERRORTYPE
tiz_thread_create (tiz_thread_t * ap_thread, size_t a_stack_size,
                   OMX_U32 a_priority, OMX_PTR (*a_pf_routine) (OMX_PTR),
                   OMX_PTR ap_arg);

/**
 * Make the calling thread wait for the termination of the thread ap_thread.
 * The exit status of the thread is stored in *app_result.
 *
 * @ingroup tizthread
 *
 * @return OMX_ErrorNone if success, OMX_ErrorUndefined otherwise.
 */
OMX_ERRORTYPE
tiz_thread_join (tiz_thread_t * ap_thread, void ** app_result);

/**
 * Set the name of a thread.
 *
 * @ingroup tizthread
 *
 * @return 0 if success, -1 otherwise.
 */
OMX_ERRORTYPE
tiz_thread_setname (tiz_thread_t * ap_thread, const OMX_STRING a_name);

/**
 * Terminate the calling thread.
 *
 * @ingroup tizthread
 */
void
tiz_thread_exit (OMX_PTR a_status);

/**
 * Get the thread id of the calling thread.
 *
 * @ingroup tizthread
 *
 * @return The thread id
 */
OMX_S32
tiz_thread_id (void);

/**
 * Sleep for the specified number of micro seconds.
 *
 * @ingroup tizthread
 *
 * @return 0 if success, -1 otherwise.
 */
OMX_S32
tiz_sleep (OMX_U32 a_usec);

#ifdef __cplusplus
}
#endif

#endif /* TIZTHREAD_H */
