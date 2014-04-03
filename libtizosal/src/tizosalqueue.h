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
 * @file   tizosalqueue.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Message queue handling
 *
 *
 */

#ifndef TIZOSALQUEUE_H
#define TIZOSALQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup queue Message queue handling
 *
 * Thread-safe FIFO queue that can be accessed safely by any number of
 * threads.
 *
 * @ingroup Tizonia-OSAL
 */

#include <OMX_Core.h>
#include <OMX_Types.h>

#include "tizosalsync.h"

#define TIZ_QUEUE_MAX_ITEMS 20

/**
 * Queue opaque structure
 * @ingroup queue
 */
typedef struct tiz_queue tiz_queue_t;
typedef /*@null@ */ tiz_queue_t *tiz_queue_ptr_t;

/**
 * Initialize a new empty queue.
 *
 * @ingroup queue
 *
 * @param a_capacity Maximum number of items that can be added to the queue.
 *
 * @return OMX_ErrorNone if success, OMX_ErrorInsufficientResources otherwise.
 */
OMX_ERRORTYPE tiz_queue_init (/*@out@*/ tiz_queue_ptr_t *app_q,
                              OMX_S32 a_capacity);

/**
 * Destroy a queue. If ap_q is NULL, or the queue has already been detroyed
 * before, no operation is performed.
 *
 * @ingroup queue
 *
 */
void tiz_queue_destroy (/*@null@ */ tiz_queue_t *ap_q);

/**
 * Add an item onto the end of the queue. If the queue is full, it blocks
 * until a space becomes available.
 *
 * @ingroup queue
 *
 */
OMX_ERRORTYPE tiz_queue_send (tiz_queue_t *ap_q, OMX_PTR ap_data);

/**
 * Retrives an item from the head of the queue. If the queue is empty, it
 * blocks until an item becomes available.
 *
 * @ingroup queue
 *
 */
OMX_ERRORTYPE tiz_queue_receive (tiz_queue_t *ap_q, OMX_PTR *app_data);

/**
 * Retrives the number of items currently stored in the queue.
 *
 * @ingroup queue
 *
 */
OMX_S32 tiz_queue_length (tiz_queue_t *ap_q);

#ifdef __cplusplus
}
#endif

#endif /* TIZOSALQUEUE_H */
