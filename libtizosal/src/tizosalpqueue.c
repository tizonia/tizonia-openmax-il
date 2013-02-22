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
 * @file   tizosalpqueue.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Priority queue
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include "tizosalpqueue.h"
#include "tizosalmem.h"
#include "tizosallog.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.osal.pqueue"
#endif

typedef struct tiz_pqueue_item tiz_pqueue_item_t;
struct tiz_pqueue_item
{
  void *p_data;
  OMX_S32 priority;
  tiz_pqueue_item_t *p_next;
  tiz_pqueue_item_t *p_prev;
};

struct tiz_pqueue
{
  /*@dependent@ */ tiz_pqueue_item_t **pp_store;
  /*@dependent@ *//*@null@ */ tiz_pqueue_item_t *p_first;
  /*@dependent@ *//*@null@ */ tiz_pqueue_item_t *p_last;
  OMX_S32 length;
  OMX_S32 max_priority;
  tiz_pq_cmp_f pf_cmp;
  tiz_soa_t *p_soa;
  char *p_str;
};

static /*@null@ */ void *
pqueue_calloc ( /*@null@ */ tiz_soa_t * p_soa, size_t a_size)
{
  return NULL != p_soa
    ? tiz_soa_calloc (p_soa, a_size) : tiz_mem_calloc (1, a_size);
}

static inline void
pqueue_free (tiz_soa_t * p_soa, void *ap_addr)
{
  NULL != p_soa ? tiz_soa_free (p_soa, ap_addr) : tiz_mem_free (ap_addr);
}

static inline void
hook_after (tiz_pqueue_t * p_q,
            tiz_pqueue_item_t * p_cur, tiz_pqueue_item_t * p_new)
{
  tiz_pqueue_item_t *p_tmp;

  p_tmp = p_cur->p_next;
  p_cur->p_next = p_new;
  p_new->p_prev = p_cur;
  if (p_tmp)
    {
      p_new->p_next = p_tmp;
      p_tmp->p_prev = p_new;
    }
  else
    {
      p_q->p_first = p_new;
    }
}

static inline void
hook_before (tiz_pqueue_t * p_q,
             tiz_pqueue_item_t * p_cur, tiz_pqueue_item_t * p_new)
{
  tiz_pqueue_item_t *p_tmp;

  p_tmp = p_cur->p_prev;
  p_cur->p_prev = p_new;
  p_new->p_next = p_cur;
  if (p_tmp)
    {
      p_new->p_prev = p_tmp;
      p_tmp->p_next = p_new;
    }
  else
    {
      p_q->p_last = p_new;
    }
}

OMX_ERRORTYPE
tiz_pqueue_init (tiz_pqueue_t ** pp_q,
                 OMX_S32 a_max_priority, tiz_pq_cmp_f a_pf_cmp,
                 tiz_soa_t * ap_soa, const char *ap_str)
{
  tiz_pqueue_t *p_q = NULL;

  TIZ_LOG (TIZ_LOG_TRACE, "Initializing [%s] pqueue with max_priority [%d]",
           ap_str, a_max_priority);

  assert (pp_q != NULL);
  assert (a_max_priority >= 0);
  assert (a_pf_cmp != NULL);

  if (NULL == (p_q = (tiz_pqueue_t *)
               pqueue_calloc (ap_soa, sizeof (tiz_pqueue_t))))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* There is one pointer per priority category */
  if (NULL == (p_q->pp_store = (tiz_pqueue_item_t **)
               pqueue_calloc (ap_soa, (size_t) (a_max_priority + 1) *
                              sizeof (tiz_pqueue_item_t *))))
    {
      pqueue_free (ap_soa, p_q);
      p_q = NULL;
      return OMX_ErrorInsufficientResources;
    }

  p_q->p_first = NULL;
  p_q->p_last = NULL;
  p_q->length = 0;
  p_q->max_priority = a_max_priority;
  p_q->pf_cmp = a_pf_cmp;
  p_q->p_soa = ap_soa;
  p_q->p_str = ap_str != NULL ? strndup (ap_str, 20) : NULL;
  *pp_q = p_q;

  return OMX_ErrorNone;
}

void
tiz_pqueue_destroy (tiz_pqueue_t * p_q)
{
  if (p_q)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Destroying [%s] pqueue [%p]", p_q->p_str, p_q);

      assert (p_q->p_first == p_q->p_last);
      assert (p_q->p_first == NULL);
      assert (p_q->length == 0);

      pqueue_free (p_q->p_soa, p_q->pp_store);
      pqueue_free (p_q->p_soa, p_q);
      tiz_mem_free (p_q->p_str);
    }
}

OMX_ERRORTYPE
tiz_pqueue_send (tiz_pqueue_t * p_q, void *ap_data, OMX_S32 a_priority)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_pqueue_item_t *p_new = NULL;

  assert (NULL != p_q);
  assert (a_priority >= 0);
  assert (a_priority <= p_q->max_priority);

  if (NULL
      == (p_new = (tiz_pqueue_item_t *)
          pqueue_calloc (p_q->p_soa, sizeof (tiz_pqueue_item_t))))
    {
      TIZ_LOG (TIZ_LOG_ERROR, "[OMX_ErrorInsufficientResources] : "
               "pqueue[%p] length [%d]. Could not allocate a queue item.",
               p_q, p_q->length);
      rc = OMX_ErrorInsufficientResources;
    }
  else
    {
      OMX_S32 prev_prio = 0;
      OMX_S32 next_prio = 0;
      tiz_pqueue_item_t *p_cur = NULL;

      p_cur = p_q->pp_store[a_priority];

      prev_prio = a_priority - 1;
      /* Find the next priority downwards. This will make prev_prio equal to
       * the next priority downwards or -1 if none */
      while ((prev_prio >= 0) && (NULL == p_q->pp_store[prev_prio]))
        {
          prev_prio--;
        }

      next_prio = a_priority + 1;
      /* Find the next priority upwards. This will make next_prio equal to next
       * prio up or max+1 if none */
      while ((next_prio <= p_q->max_priority)
             && (NULL == p_q->pp_store[next_prio]))
        {
          next_prio++;
        }

      if (NULL == p_q->pp_store[a_priority])
        {
          p_q->pp_store[a_priority] = p_new;
        }

      if ((prev_prio >= 0) && (NULL != p_q->pp_store[prev_prio]))
        {
          hook_after (p_q, p_q->pp_store[prev_prio], p_new);

        }
      else if ((next_prio <= p_q->max_priority)
               && (NULL != p_q->pp_store[next_prio]))
        {
          hook_before (p_q, p_q->pp_store[next_prio], p_new);
        }
      else
        {
          if (p_q->pp_store[a_priority] != p_new)
            {
              p_cur = p_q->p_last;
              if (NULL != p_cur)
                {
                  p_cur->p_prev = p_new;
                  p_new->p_next = p_cur;
                }
            }
          else
            {
              p_q->p_first = p_new;
            }

          p_q->p_last = p_new;

        }

      p_new->p_data = ap_data;
      p_new->priority = a_priority;
      p_q->length++;

      TIZ_LOG (TIZ_LOG_TRACE, "[%s] pq[%p] len[%d] fst [%p] lst [%p]",
               p_q->p_str, p_q, p_q->length, p_q->p_first, p_q->p_last);

      assert (NULL != p_q->p_first);
      assert (NULL != p_q->p_last);
    }

  return rc;
}

OMX_ERRORTYPE
tiz_pqueue_receive (tiz_pqueue_t * p_q, void * *app_data)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_q);
  assert (NULL != app_data);

  TIZ_LOG (TIZ_LOG_TRACE, "[%s], pq[%p] len[%d] fst [%p] lst [%p]",
           p_q->p_str, p_q, p_q->length, p_q->p_first, p_q->p_last);

  if (0 >= p_q->length)
    {
      assert (p_q->p_first == p_q->p_last);
      rc = OMX_ErrorNoMore;
    }
  else
    {
      tiz_pqueue_item_t *p_cur = NULL;
      tiz_pqueue_item_t *p_prev = NULL;

      p_cur = p_q->p_first;
      assert (NULL != p_cur);
      p_prev = p_cur->p_prev;
      p_q->p_first = p_prev;

      if (p_prev)
        {
          TIZ_LOG (TIZ_LOG_TRACE,
                   "[%s], pq[%p] len[%d] fst [%p] lst [%p] p_prev[%p]",
                   p_q->p_str, p_q, p_q->length, p_q->p_first, p_q->p_last,
                   p_prev);

          p_prev->p_next = NULL;
        }
      else
        {
          p_q->p_last = NULL;
        }

      if (p_q->pp_store[p_cur->priority] == p_cur)
        {
          if ((NULL != p_prev) && (p_prev->priority == p_cur->priority))
            {
              p_q->pp_store[p_cur->priority] = p_prev;
            }
          else
            {
              p_q->pp_store[p_cur->priority] = NULL;
            }
        }

      p_q->length--;
      *app_data = p_cur->p_data;
      pqueue_free (p_q->p_soa, p_cur);

      assert (p_q->length >= 0);
      assert (p_q->length >
              0 ? (NULL != p_q->p_first && NULL != p_q->p_last) : 1);
    }

  TIZ_LOG (TIZ_LOG_TRACE, "[%s], pq[%p] len[%d] fst [%p] lst [%p]",
           p_q->p_str, p_q, p_q->length, p_q->p_first, p_q->p_last);

  return rc;
}

OMX_ERRORTYPE
tiz_pqueue_remove (tiz_pqueue_t * p_q, void *ap_data)
{
  OMX_ERRORTYPE rc = OMX_ErrorNoMore;
  tiz_pqueue_item_t *p_cur = NULL;
  tiz_pqueue_item_t *p_prev = NULL;
  tiz_pqueue_item_t *p_next = NULL;

  assert (NULL != p_q);
  assert (NULL != ap_data);

  p_cur = p_q->p_first;
  while (NULL != p_cur)
    {
      if (p_q->pf_cmp (p_cur->p_data, ap_data) == 0)
        {
          p_prev = p_cur->p_prev;
          p_next = p_cur->p_next;

          if (NULL != p_prev)
            {
              p_prev->p_next = p_next;
            }

          if (NULL != p_next)
            {
              p_next->p_prev = p_prev;
            }

          if (p_q->p_first == p_cur)
            {
              p_q->p_first = p_prev;
            }

          if (p_q->p_last == p_cur)
            {
              p_q->p_last = p_next;
            }

          if (p_q->pp_store[p_cur->priority] == p_cur)
            {
              if ((NULL != p_prev) && (p_prev->priority == p_cur->priority))
                {
                  p_q->pp_store[p_cur->priority] = p_prev;
                }
              else
                {
                  p_q->pp_store[p_cur->priority] = NULL;
                }
            }

          pqueue_free (p_q->p_soa, p_cur);
          p_q->length--;
          rc = OMX_ErrorNone;
          /* DONE */
          break;
        }
      p_cur = p_cur->p_prev;
    }

  TIZ_LOG (TIZ_LOG_TRACE, "pq[%p] len[%d] fst [%p] lst [%p]",
           p_q, p_q->length, p_q->p_first, p_q->p_last);

  return rc;
}

OMX_ERRORTYPE
tiz_pqueue_removep (tiz_pqueue_t * p_q, void *ap_data, OMX_S32 a_priority)
{
  OMX_ERRORTYPE rc = OMX_ErrorNoMore;
  tiz_pqueue_item_t *p_cur = NULL;
  tiz_pqueue_item_t *p_prev = NULL;
  tiz_pqueue_item_t *p_next = NULL;

  TIZ_LOG (TIZ_LOG_TRACE, "p_q [%p] ap_data [%p] "
           "a_priority [%d]", p_q, ap_data, a_priority);

  assert (NULL != p_q);
  assert (ap_data != NULL);
  assert (a_priority >= 0);
  assert (a_priority <= p_q->max_priority);

  p_cur = p_q->pp_store[a_priority];
  while (NULL != p_cur && p_cur->priority == a_priority)
    {
      if (p_q->pf_cmp (p_cur->p_data, ap_data) == 0)
        {
          p_prev = p_cur->p_prev;
          p_next = p_cur->p_next;

          if (p_prev)
            {
              p_prev->p_next = p_next;
            }

          if (p_next)
            {
              p_next->p_prev = p_prev;
            }

          if (p_q->p_first == p_cur)
            {
              p_q->p_first = p_prev;
            }

          if (p_q->p_last == p_cur)
            {
              p_q->p_last = p_next;
            }

          if (p_q->pp_store[p_cur->priority] == p_cur)
            {
              if ((NULL != p_prev) && (p_prev->priority == p_cur->priority))
                {
                  p_q->pp_store[p_cur->priority] = p_prev;
                }
              else
                {
                  p_q->pp_store[p_cur->priority] = NULL;
                }
            }

          pqueue_free (p_q->p_soa, p_cur);
          p_q->length--;
          rc = OMX_ErrorNone;
          /* DONE */
          break;
        }
      p_cur = p_cur->p_prev;
    }

  TIZ_LOG (TIZ_LOG_TRACE, "pq[%p] len[%d] fst [%p] lst [%p]",
           p_q, p_q->length, p_q->p_first, p_q->p_last);

  return rc;
}

OMX_ERRORTYPE
tiz_pqueue_remove_func (tiz_pqueue_t * p_q, tiz_pq_func_f a_pf_func,
                        OMX_S32 a_data1, void *ap_data2)
{
  tiz_pqueue_item_t *p_cur = NULL;
  tiz_pqueue_item_t *p_prev = NULL;
  tiz_pqueue_item_t *p_next = NULL;

  assert (NULL != p_q);
  assert (NULL != a_pf_func);
  assert (NULL != ap_data2);

  TIZ_LOG (TIZ_LOG_TRACE, "pq[%p] len[%d] fst [%p] lst [%p]",
           p_q, p_q->length, p_q->p_first, p_q->p_last);

  p_cur = p_q->p_first;
  while (NULL != p_cur)
    {
      if (OMX_TRUE == a_pf_func (p_cur->p_data, a_data1, ap_data2))
        {
          p_prev = p_cur->p_prev;
          p_next = p_cur->p_next;

          if (NULL != p_prev)
            {
              p_prev->p_next = p_next;
            }

          if (NULL != p_next)
            {
              p_next->p_prev = p_prev;
            }

          if (p_q->p_first == p_cur)
            {
              p_q->p_first = p_prev;
            }

          if (p_q->p_last == p_cur)
            {
              p_q->p_last = p_next;
            }

          if (p_q->pp_store[p_cur->priority] == p_cur)
            {
              if ((NULL != p_prev) && (p_prev->priority == p_cur->priority))
                {
                  p_q->pp_store[p_cur->priority] = p_prev;
                }
              else
                {
                  p_q->pp_store[p_cur->priority] = NULL;
                }
            }

          {
            tiz_pqueue_item_t *p_to_delete = p_cur;
            p_cur = p_cur->p_prev;
            pqueue_free (p_q->p_soa, p_to_delete);
            p_q->length--;
            /* NOTE: We continue here to remove as many matching items as
             * possible */
          }
        }
      else
        {
          p_cur = p_cur->p_prev;
        }
    }

  return OMX_ErrorNoMore;
}

OMX_ERRORTYPE
tiz_pqueue_first (tiz_pqueue_t * p_q, void * *app_data)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_q);
  assert (NULL != app_data);

  if (0 >= p_q->length)
    {
      assert (p_q->p_first == p_q->p_last);
      rc = OMX_ErrorNoMore;
    }
  else
    {
      assert (NULL != p_q->p_first);
      *app_data = p_q->p_first->p_data;
    }

  return rc;
}

OMX_S32
tiz_pqueue_length (const tiz_pqueue_t * p_q)
{
  assert (NULL != p_q);
  return p_q->length;
}

OMX_S32
tiz_pqueue_print (tiz_pqueue_t * p_q, tiz_pq_print_item_f a_pf_print)
{
  tiz_pqueue_item_t *p_first = NULL;
  OMX_S32 count = 0;

  TIZ_LOG (TIZ_LOG_TRACE, "p_q [%p]", p_q);

  assert (NULL != p_q);
  assert (NULL != a_pf_print);

  p_first = p_q->p_first;
  while (NULL != p_first)
    {
      a_pf_print (p_first->p_data, p_first->priority);
      p_first = p_first->p_prev;
      count++;
    }

  return count;
}

OMX_S32
tiz_pqueue_dump (tiz_pqueue_t * p_q, tiz_pq_dump_item_f a_pf_dump)
{
  tiz_pqueue_item_t *p_first = NULL;
  OMX_S32 count = 0;

  assert (NULL != p_q);
  assert (NULL != a_pf_dump);

  TIZ_LOG (TIZ_LOG_TRACE, "p_q [%p] first [%p] last [%p] "
           "length [%d] max_priority [%d]", p_q, p_q->p_first,
           p_q->p_last, p_q->length, p_q->max_priority);

  p_first = p_q->p_first;
  while (NULL != p_first)
    {
      a_pf_dump (p_first->p_data, p_first->priority,
                 p_first, p_first->p_next, p_first->p_prev);
      p_first = p_first->p_prev;
      count++;
    }

  return count;
}
