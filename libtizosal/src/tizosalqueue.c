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
 * @file   tizosalqueue.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Message queue handling
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdbool.h>

#include "tizosalqueue.h"
#include "tizosalmem.h"
#include "tizosallog.h"
#include "tizosalutils.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.osal.queue"
#endif


typedef struct tiz_queue_item tiz_queue_item_t;
struct tiz_queue_item
{
  OMX_PTR p_data;
  tiz_queue_item_t *p_next;
};

struct tiz_queue
{
  /*@null@ */ tiz_queue_item_t *p_first;
  /*@null@ */ tiz_queue_item_t *p_last;
  OMX_S32 capacity;
  OMX_S32 length;
  tiz_mutex_t mutex;
  tiz_cond_t cond_full;
  tiz_cond_t cond_empty;
};

static inline void
deinit_queue_struct ( /*@null@ */ tiz_queue_t * ap_q)
{
  /* Clean-up */
  if (ap_q)
    {
      (void) tiz_cond_destroy (&(ap_q->cond_empty));
      (void) tiz_cond_destroy (&(ap_q->cond_full));
      (void) tiz_mutex_destroy (&(ap_q->mutex));
      tiz_mem_free (ap_q);
    }
}

/*@null@*/ static tiz_queue_t *
init_queue_struct ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_queue_t *p_q = NULL;
  bool init_failed = false;

  if ((p_q = (tiz_queue_t *) tiz_mem_calloc (1, sizeof (tiz_queue_t))))
    {
      if ((rc = tiz_mutex_init (&(p_q->mutex))) == OMX_ErrorNone)
        {
          if ((rc = tiz_cond_init (&(p_q->cond_full))) == OMX_ErrorNone)
            {
              if ((rc = tiz_cond_init (&(p_q->cond_empty))) == OMX_ErrorNone)
                {
                  p_q->p_first =
                    (tiz_queue_item_t *)
                    tiz_mem_calloc (1, sizeof (tiz_queue_item_t));
                  if (!(p_q->p_first))
                    {
                      TIZ_LOG (TIZ_LOG_ERROR, "Could not create first item.");
                      init_failed = true;
                    }
                }
              else
                {
                  TIZ_LOG (TIZ_LOG_ERROR,
                           "Could not create empty cond variable.");
                  init_failed = true;
                }
            }
          else
            {
              TIZ_LOG (TIZ_LOG_ERROR, "Could not create full cond variable.");
              init_failed = true;
            }
        }
      else
        {
          TIZ_LOG (TIZ_LOG_ERROR, "Could not create mutex.");
          init_failed = true;
        }
    }
  else
    {
      TIZ_LOG (TIZ_LOG_ERROR, "Could not instantiate queue struct.");
      init_failed = true;
    }

  if (init_failed)
    {
      deinit_queue_struct (p_q);
      p_q = 0;
    }

  return p_q;
}

OMX_ERRORTYPE
tiz_queue_init (tiz_queue_ptr_t * app_q, OMX_S32 a_capacity)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_queue_item_t *p_new_item = NULL;
  tiz_queue_item_t *p_cur_item = NULL;
  tiz_queue_t *p_q = NULL;

  assert (NULL != app_q);

  TIZ_LOG (TIZ_LOG_TRACE, "queue capacity [%d]", a_capacity);

  assert (a_capacity > 0);

  if (NULL != (p_q = init_queue_struct ()))
    {
      int i = 0;
      p_q->capacity = a_capacity;
      p_q->length = 0;

      p_cur_item = p_q->p_last = p_q->p_first;
      assert (NULL != p_cur_item);

      for (i = 0; i < (TIZ_QUEUE_MAX_ITEMS - 2); i++)
        {
          if (NULL != (p_new_item =
                       (tiz_queue_item_t *)
                       tiz_mem_calloc (1, sizeof (tiz_queue_item_t))))
            {
              p_cur_item->p_next = p_new_item;
              p_cur_item = p_new_item;
            }
          else
            {
              TIZ_LOG (TIZ_LOG_ERROR, "[OMX_ErrorInsufficientResources]: "
                       "Could not instantiate queue items.");
              rc = OMX_ErrorInsufficientResources;

              /* Clean-up */
              while (NULL != p_q->p_first)
                {
                  p_cur_item = p_q->p_first->p_next;
                  tiz_mem_free ((OMX_PTR) p_q->p_first);
                  p_q->p_first = p_cur_item;
                }
              /* end loop  */
              break;
            }
        }                       /* for */

      if (OMX_ErrorNone == rc)
        {
          p_cur_item->p_next = p_q->p_first;
          TIZ_LOG (TIZ_LOG_TRACE, "queue created [%p]", p_q);
        }
    }
  else
    {
      TIZ_LOG (TIZ_LOG_ERROR, "OMX_ErrorInsufficientResources: "
               "Could not instantiate queue struct.");
      rc = OMX_ErrorInsufficientResources;
    }

  if (OMX_ErrorNone != rc)
    {
      /* Clean-up */
      deinit_queue_struct (p_q);
    }

  *app_q = p_q;
  return rc;
}

void
tiz_queue_destroy ( /*@null@ */ tiz_queue_t * p_q)
{
  if (p_q)
    {
      tiz_queue_item_t *p_cur_item = 0;
      int i = 0;

      for (i = 0; NULL != p_q->p_first && i < (TIZ_QUEUE_MAX_ITEMS - 2); ++i)
        {
          p_cur_item = p_q->p_first->p_next;
          tiz_mem_free (p_q->p_first);
          p_q->p_first = p_cur_item;
        }

      if (p_q->p_first)
        {
          tiz_mem_free (p_q->p_first);
          p_q->p_first = NULL;
        }

      deinit_queue_struct (p_q);
    }
}

OMX_ERRORTYPE
tiz_queue_send (tiz_queue_t * p_q, OMX_PTR ap_data)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_q);

  TIZ_UTIL_TEST_ERR_OOM (tiz_mutex_lock (&(p_q->mutex)));

  assert (NULL != p_q->p_last);
  assert (NULL == (p_q->p_last->p_data));
  assert (p_q->length <= p_q->capacity);

  while (p_q->length == p_q->capacity)
    {
      rc = tiz_cond_wait (&(p_q->cond_full), &(p_q->mutex));
    }

  if (OMX_ErrorNone == rc)
    {
      p_q->p_last->p_data = ap_data;
      p_q->p_last = p_q->p_last->p_next;
      p_q->length++;
    }

  TIZ_UTIL_TEST_ERR_OOM (tiz_mutex_unlock (&(p_q->mutex)));
  TIZ_UTIL_TEST_ERR_OOM (tiz_cond_broadcast (&(p_q->cond_empty)));

  return rc;
}

OMX_ERRORTYPE
tiz_queue_receive (tiz_queue_t * p_q, OMX_PTR * app_data)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_q);
  assert (NULL != app_data);

  TIZ_UTIL_TEST_ERR_OOM (tiz_mutex_lock (&(p_q->mutex)));

  assert (!(p_q->length < 0));

  while (p_q->length == 0)
    {
      rc = tiz_cond_wait (&(p_q->cond_empty), &(p_q->mutex));
    }

  if (OMX_ErrorNone == rc)
    {
      assert (NULL != p_q->p_first);
      assert (NULL != p_q->p_first->p_data);
      *app_data = p_q->p_first->p_data;
      p_q->p_first->p_data = 0;
      p_q->p_first = p_q->p_first->p_next;
      p_q->length--;
    }

  TIZ_UTIL_TEST_ERR_OOM (tiz_mutex_unlock (&(p_q->mutex)));
  TIZ_UTIL_TEST_ERR_OOM (tiz_cond_broadcast (&(p_q->cond_full)));

  return rc;
}

OMX_S32
tiz_queue_length (tiz_queue_t * p_q)
{
  OMX_S32 length = 0;

  assert (NULL != p_q);

  TIZ_UTIL_TEST_ERR_OOM (tiz_mutex_lock (&(p_q->mutex)));

  length = p_q->length;

  TIZ_UTIL_TEST_ERR_OOM (tiz_mutex_unlock (&(p_q->mutex)));

  return length;
}
