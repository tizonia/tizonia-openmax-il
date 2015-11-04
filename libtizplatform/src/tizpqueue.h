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
 * @file   tizpqueue.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Priority queue
 *
 *
 */

#ifndef TIZPQUEUE_H
#define TIZPQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup tizpqueue Priority message queue handling
 *
 * Non-synchronized priority queue. External synchronisation is required in
 * case it needs to be accessed safely from multiple threads.
 *
 * @ingroup libtizplatform
 */

#include <OMX_Core.h>
#include <OMX_Types.h>

#include "tizsoa.h"

/**
 * Priority queue opaque structure.
 * @ingroup tizpqueue
 */
typedef struct tiz_pqueue tiz_pqueue_t;

/**
 * \typedef The comparison function to be used by the removal functions
 * tiz_pqueue_remove and tiz_pqueue_removep.
 *
 * @return Must return an integer less than, equal to, or greater than zero
 * if the left argument is considered to be respectively less than, equal to,
 * or greater than the right one.
 *
 */
typedef OMX_S32 (*tiz_pq_cmp_f)(void *ap_left, void *ap_right);

/**
 * \typedef A comparison function to be used by the removal function
 * tiz_pqueue_remove_func.
 *
 * @param ap_elem The item from the queue that is being compared
 *
 * @param a_data1 User-provided integer (see tiz_pqueue_remove_func)
 *
 * @param ap_data2 User-provided data (see tiz_pqueue_remove_func)
 *
 * @return Must return OMX_TRUE if ap_elem is considered to match the search
 * criteria. OMX_FALSE otherwise.
 *
 */
typedef OMX_BOOL (*tiz_pq_func_f)(void *ap_elem, OMX_S32 a_data1,
                                  void *ap_data2);

/**
 * \typedef Function callback that receives the contents of a node from the
 * queue.
 *
 * @param ap_name The queue's name
 *
 * @param ap_data Data stored in the node
 *
 * @param a_priority Priority group of the node
 *
 * @param ap_cur The address of the current node
 *
 * @param ap_next The address of the next node (equal or lower priority)
 *
 * @param ap_prev The address of the previous node (equal or higher priority)
 *
 */
typedef void (*tiz_pq_dump_item_f)(const char *ap_name, void *ap_data,
                                   OMX_S32 a_priority, void *ap_cur,
                                   void *ap_next, void *ap_prev);

#define TIZ_PQUEUE_MAX_NAME_LEN 20
/**
 * Initialize a new empty priority queue with up to a_max_prio + 1 different
 * priorities (i.e. priority groups [0..a_max_prio]). 0 is the highest
 * priority.
 *
 * @ingroup tizpqueue
 *
 * @param app_pq A handle to the priority queue object to be initialized
 *
 * @param a_max_prio Maximum number of priority groups that can be stored
 * in the queue
 *
 * @param apf_cmp A comparison function (used by tiz_pqueue_remove and
 * tiz_pqueue_removep functions)
 *
 * @param ap_soa The Tizonia's small object allocator to allocate from. Or
 * NULL if the Tizonia's default allocation/deallocation routines should be
 * used instead
 *
 * @ap_name A string of up to TIZ_PQUEUE_MAX_NAME_LEN characters that can be
 * used during debugging to identify this queue
 *
 * @return OMX_ErrorNone if success, OMX_ErrorInsufficientResources otherwise
 */
OMX_ERRORTYPE tiz_pqueue_init (tiz_pqueue_t **app_pq, OMX_S32 a_max_prio,
                               tiz_pq_cmp_f apf_cmp, tiz_soa_t *ap_soa,
                               const char *ap_name);

/**
 * Destroy a priority queue.
 *
 * @param ap_pq The priority queue to be destroyed. If NULL, or if the queue
 * has already been detroyed, no operation is performed.
 *
 * @pre The queue must be empty before calling this function

 * @ingroup tizpqueue
 *
 */
void tiz_pqueue_destroy (/*@null@ */ tiz_pqueue_t *ap_pq);

/**
 * Add an item to the end of the priority group a_prio.
 *
 * @ingroup tizpqueue
 *
 * @return OMX_ErrorNone if success, OMX_ErrorInsufficientResources otherwise
 *
 */
OMX_ERRORTYPE tiz_pqueue_send (tiz_pqueue_t *ap_pq, void *ap_data,
                               OMX_S32 a_prio);

/**
 * Receive the first item from the queue. The item received is no longer in
 * the queue.
 *
 * @return OMX_ErrorNone if success, OMX_ErrorNoMore if empty
 *
 * @ingroup tizpqueue
 *
 */
OMX_ERRORTYPE tiz_pqueue_receive (tiz_pqueue_t *ap_pq, void **app_data);

/**
 * Remove an item from the queue. The item is found using the comparison
 * function provided at initialization time (see tiz_pqueue_init).
 *
 * @return OMX_ErrorNone if success, OMX_ErrorNoMore if not found
 *
 * @ingroup tizpqueue
 *
 */
OMX_ERRORTYPE tiz_pqueue_remove (tiz_pqueue_t *ap_pq, void *ap_data);

/**
 * Remove an item from the queue. The item is found using the comparison
 * function provided at initialization time (see tiz_pqueue_init). The search
 * is restricted to the priority group a_priority
 *
 * @return OMX_ErrorNone if success, OMX_ErrorNoMore if not found
 *
 * @ingroup tizpqueue
 *
 */
OMX_ERRORTYPE tiz_pqueue_removep (tiz_pqueue_t *ap_pq, void *ap_data,
                                  OMX_S32 a_priority);

/**
 * Remove from the queue all the items found using the comparison function
 * apf_func.
 *
 * @ingroup tizpqueue
 *
 */
void tiz_pqueue_remove_func (tiz_pqueue_t *ap_pq, tiz_pq_func_f apf_func,
                             OMX_S32 a_data1, void *ap_data2);

/**
 * Return a reference to the first item in the queue.
 *
 * @return OMX_ErrorNone if success, OMX_ErrorNoMore if none found
 *
 * @ingroup tizpqueue
 *
 */
OMX_ERRORTYPE tiz_pqueue_first (tiz_pqueue_t *ap_pq, void **app_data);

/**
 * Return the number of items currently in the queue.
 *
 * @return Number of items in the queue
 *
 * @ingroup tizpqueue
 *
 */
OMX_S32 tiz_pqueue_length (const tiz_pqueue_t *ap_pq);

/**
 * This function dumps the contents of all the nodes in the queue using the
 * apf_dump callback function.
 *
 * @return The number of nodes processed.
 *
 * @ingroup tizpqueue
 *
 */
OMX_S32 tiz_pqueue_dump (tiz_pqueue_t *ap_pq, tiz_pq_dump_item_f apf_dump);

#ifdef __cplusplus
}
#endif

#endif /* TIZPQUEUE_H */
