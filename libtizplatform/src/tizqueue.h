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
 * @file   tizqueue.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Message queue handling
 *
 *
 */

#ifndef TIZQUEUE_H
#define TIZQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tizqueue Message queue handling
 *
 * Thread-safe FIFO queue.
 *
 * @ingroup libtizplatform
 */

#include <OMX_Core.h>
#include <OMX_Types.h>

#include "tizsync.h"

/**
 * Queue opaque structure.
 * @ingroup tizqueue
 */
typedef struct tiz_queue tiz_queue_t;
typedef /*@null@ */ tiz_queue_t *tiz_queue_ptr_t;

/**
 * Initialize a new empty queue.
 *
 * @ingroup tizqueue
 *
 * @param a_capacity Maximum number of items that can be send into the queue.
 *
 * @return OMX_ErrorNone if success, OMX_ErrorInsufficientResources otherwise.
 */
OMX_ERRORTYPE tiz_queue_init (/*@out@*/ tiz_queue_ptr_t *app_q,
                              OMX_S32 a_capacity);

/**
 * Destroy a queue. If ap_q is NULL, or the queue has already been detroyed
 * before, no operation is performed.
 *
 * @ingroup tizqueue
 *
 */
void tiz_queue_destroy (/*@null@ */ tiz_queue_t *ap_q);

/**
 * Add an item onto the end of the queue. If the queue is full, it blocks
 * until a space becomes available.
 *
 * @ingroup tizqueue
 *
 */
OMX_ERRORTYPE tiz_queue_send (tiz_queue_t *ap_q, OMX_PTR ap_data);

/**
 * Retrieve an item from the head of the queue. If the queue is empty, it
 * blocks until an item becomes available.
 *
 * @ingroup tizqueue
 *
 */
OMX_ERRORTYPE tiz_queue_receive (tiz_queue_t *ap_q, OMX_PTR *app_data);

/**
 * Retrieve the maximum number of items that can be stored in the queue.
 *
 * @ingroup tizqueue
 *
 */
OMX_S32 tiz_queue_capacity (tiz_queue_t *ap_q);

/**
 * Retrieve the number of items currently stored in the queue.
 *
 * @ingroup tizqueue
 *
 */
OMX_S32 tiz_queue_length (tiz_queue_t *ap_q);

#ifdef __cplusplus
}
#endif

#endif /* TIZQUEUE_H */
