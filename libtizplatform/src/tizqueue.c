/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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
 * @file   tizqueue.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Message queue handling
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizplatform.h"

#include <assert.h>
#include <stdbool.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.queue"
#endif

#define TIZ_Q_GOTO_END_ON_ERROR(exp)                                       \
  do                                                                       \
    {                                                                      \
      OMX_ERRORTYPE rc_ = OMX_ErrorNone;                                   \
      if (OMX_ErrorNone != (rc_ = (exp)))                                  \
        {                                                                  \
          TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : %s", tiz_err_to_str (rc_)); \
          goto end;                                                        \
        }                                                                  \
    }                                                                      \
  while (0)

#define TIZ_Q_GOTO_END_ON_NULL(exp)                                         \
  do                                                                        \
    {                                                                       \
      if (NULL == ((exp)))                                                  \
        {                                                                   \
          TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorInsufficientResources]"); \
          goto end;                                                         \
        }                                                                   \
    }                                                                       \
  while (0)

typedef struct tiz_queue_item tiz_queue_item_t;
struct tiz_queue_item
{
  OMX_PTR p_data;
  tiz_queue_item_t * p_next;
};

struct tiz_queue
{
  /*@null@ */ tiz_queue_item_t * p_first;
  /*@null@ */ tiz_queue_item_t * p_last;
  OMX_S32 capacity;
  OMX_S32 length;
  tiz_mutex_t mutex;
  tiz_cond_t cond_full;
  tiz_cond_t cond_empty;
};

static inline void
deinit_queue_struct (/*@null@ */ tiz_queue_t * ap_q)
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
init_queue_struct (void)
{
  bool init_ok = false;
  tiz_queue_t * p_q = (tiz_queue_t *) tiz_mem_calloc (1, sizeof (tiz_queue_t));

  TIZ_Q_GOTO_END_ON_NULL (p_q);
  TIZ_Q_GOTO_END_ON_ERROR (tiz_mutex_init (&(p_q->mutex)));
  TIZ_Q_GOTO_END_ON_ERROR (tiz_cond_init (&(p_q->cond_full)));
  TIZ_Q_GOTO_END_ON_ERROR (tiz_cond_init (&(p_q->cond_empty)));
  p_q->p_first
    = (tiz_queue_item_t *) tiz_mem_calloc (1, sizeof (tiz_queue_item_t));
  TIZ_Q_GOTO_END_ON_NULL (p_q->p_first);

  /* All OK */
  init_ok = true;

end:

  if (!init_ok)
    {
      deinit_queue_struct (p_q);
      p_q = NULL;
    }

  return p_q;
}

OMX_ERRORTYPE
tiz_queue_init (tiz_queue_ptr_t * app_q, OMX_S32 a_capacity)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_queue_item_t * p_new_item = NULL;
  tiz_queue_item_t * p_cur_item = NULL;
  tiz_queue_t * p_q = NULL;

  assert (app_q);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "queue capacity [%d]", a_capacity);

  assert (a_capacity > 0);

  if ((p_q = init_queue_struct ()))
    {
      int i = 0;
      p_q->capacity = a_capacity;
      p_q->length = 0;

      p_cur_item = p_q->p_last = p_q->p_first;
      assert (p_cur_item);

      for (i = 0; i < (a_capacity - 1); ++i)
        {
          if ((p_new_item = (tiz_queue_item_t *) tiz_mem_calloc (
                 1, sizeof (tiz_queue_item_t))))
            {
              p_cur_item->p_next = p_new_item;
              p_cur_item = p_new_item;
            }
          else
            {
              TIZ_LOG (TIZ_PRIORITY_ERROR,
                       "[OMX_ErrorInsufficientResources]: "
                       "Could not instantiate queue items.");
              rc = OMX_ErrorInsufficientResources;

              /* Clean-up */
              while (p_q->p_first)
                {
                  p_cur_item = p_q->p_first->p_next;
                  tiz_mem_free ((OMX_PTR) p_q->p_first);
                  p_q->p_first = p_cur_item;
                }
              /* end loop  */
              break;
            }
        } /* for */

      if (OMX_ErrorNone == rc)
        {
          p_cur_item->p_next = p_q->p_first;
          TIZ_LOG (TIZ_PRIORITY_TRACE, "queue created [%p]", p_q);
        }
    }
  else
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "OMX_ErrorInsufficientResources: "
               "Could not instantiate queue struct.");
      rc = OMX_ErrorInsufficientResources;
    }

  if (OMX_ErrorNone == rc)
    {
      *app_q = p_q;
    }
  else
    {
      /* Clean-up */
      deinit_queue_struct (p_q);
      p_q = NULL;
    }

  return rc;
}

void
tiz_queue_destroy (/*@null@ */ tiz_queue_t * p_q)
{
  if (p_q)
    {
      tiz_queue_item_t * p_cur_item = 0;
      int i = 0;

      for (i = 0; p_q->p_first && i < (p_q->capacity - 1); ++i)
        {
          p_cur_item = p_q->p_first->p_next;
          tiz_mem_free (p_q->p_first);
          p_q->p_first = p_cur_item;
        }

      deinit_queue_struct (p_q);
    }
}

OMX_ERRORTYPE
tiz_queue_send (tiz_queue_t * p_q, OMX_PTR ap_data)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_q);

  tiz_check_omx_ret_oom (tiz_mutex_lock (&(p_q->mutex)));

  assert (p_q->p_last);
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

  tiz_check_omx_ret_oom (tiz_mutex_unlock (&(p_q->mutex)));
  tiz_check_omx_ret_oom (tiz_cond_broadcast (&(p_q->cond_empty)));

  return rc;
}

OMX_ERRORTYPE
tiz_queue_receive (tiz_queue_t * p_q, OMX_PTR * app_data)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_q);
  assert (app_data);

  tiz_check_omx_ret_oom (tiz_mutex_lock (&(p_q->mutex)));

  assert (!(p_q->length < 0));

  while (p_q->length == 0)
    {
      rc = tiz_cond_wait (&(p_q->cond_empty), &(p_q->mutex));
    }

  if (OMX_ErrorNone == rc)
    {
      assert (p_q->p_first);
      assert (p_q->p_first->p_data);
      *app_data = p_q->p_first->p_data;
      p_q->p_first->p_data = 0;
      p_q->p_first = p_q->p_first->p_next;
      p_q->length--;
    }

  tiz_check_omx_ret_oom (tiz_mutex_unlock (&(p_q->mutex)));
  tiz_check_omx_ret_oom (tiz_cond_broadcast (&(p_q->cond_full)));

  return rc;
}

OMX_ERRORTYPE
tiz_queue_timed_receive (tiz_queue_t * p_q, OMX_PTR * app_data,
                         OMX_U32 a_millis)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_q);
  assert (app_data);

  tiz_check_omx_ret_oom (tiz_mutex_lock (&(p_q->mutex)));

  assert (!(p_q->length < 0));

  while (p_q->length == 0)
    {
      rc = tiz_cond_timedwait (&(p_q->cond_empty), &(p_q->mutex), a_millis);
      if (OMX_ErrorTimeout == rc)
        {
          break;
        }
    }

  if (OMX_ErrorNone == rc || (OMX_ErrorTimeout == rc && p_q->length > 0))
    {
      assert (p_q->p_first);
      assert (p_q->p_first->p_data);
      *app_data = p_q->p_first->p_data;
      p_q->p_first->p_data = 0;
      p_q->p_first = p_q->p_first->p_next;
      p_q->length--;
    }

  tiz_check_omx_ret_oom (tiz_mutex_unlock (&(p_q->mutex)));
  tiz_check_omx_ret_oom (tiz_cond_broadcast (&(p_q->cond_full)));

  return rc;
}

OMX_S32
tiz_queue_capacity (tiz_queue_t * p_q)
{
  OMX_S32 capacity = 0;

  assert (p_q);

  tiz_check_omx_ret_oom (tiz_mutex_lock (&(p_q->mutex)));

  capacity = p_q->capacity;

  tiz_check_omx_ret_oom (tiz_mutex_unlock (&(p_q->mutex)));

  return capacity;
}

OMX_S32
tiz_queue_length (tiz_queue_t * p_q)
{
  OMX_S32 length = 0;

  assert (p_q);

  tiz_check_omx_ret_oom (tiz_mutex_lock (&(p_q->mutex)));

  length = p_q->length;

  tiz_check_omx_ret_oom (tiz_mutex_unlock (&(p_q->mutex)));

  return length;
}
