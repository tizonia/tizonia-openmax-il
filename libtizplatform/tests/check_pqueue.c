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
 * @file   check_pqueue.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Non-synchronized priority queue API unit tests
 *
 *
 */

static void
pqueue_dump_item (const char *ap_str, OMX_PTR ap_data, OMX_S32 a_priority,
                  OMX_PTR ap_cur, OMX_PTR ap_next, OMX_PTR ap_prev)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] data [%d] priority [%d] cur [%p] "
           "next [%p] prev [%p]", ap_str,
             *(int *) ap_data, a_priority, ap_cur, ap_next, ap_prev);
}

static OMX_S32
pqueue_cmp (OMX_PTR ap_left, OMX_PTR ap_right)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "left [%d] right [%d]",
             *(int *) ap_left, *(int *) ap_right);

  if ((*(int *) ap_left) == (*(int *) ap_right))
    {
      return 0;
    }
  else if ((*(int *) ap_left) < (*(int *) ap_right))
    {
      return -1;
    }

  return 1;

}

START_TEST (test_pqueue_init_and_destroy)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_pqueue_t *p_queue = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_pqueue_init_and_destroy");

  error = tiz_pqueue_init (&p_queue, 10, &pqueue_cmp, NULL, "tizkrn");

  fail_if (p_queue == NULL);
  fail_if (error != OMX_ErrorNone);

  tiz_pqueue_destroy (p_queue);
}
END_TEST

START_TEST (test_pqueue_send_and_receive_one_group)
{
  OMX_U32 i;
  OMX_PTR p_received = NULL;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int *p_item = NULL;
  tiz_pqueue_t *p_queue = NULL;
  tiz_soa_t *p_soa = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_pqueue_send_and_receive_one_group");

  fail_if (tiz_soa_init (&p_soa) != OMX_ErrorNone);

  error = tiz_pqueue_init (&p_queue, 0, &pqueue_cmp, p_soa, "tizkrn");

  fail_if (error != OMX_ErrorNone);

  for (i = 0; i < 10; i++)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;
      error = tiz_pqueue_send (p_queue, p_item, 0);
      fail_if (error != OMX_ErrorNone);
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "queue length %d",
             tiz_pqueue_length (p_queue));
  fail_if (10 != tiz_pqueue_length (p_queue));

  for (i = 0; i <= 10; i++)
    {
      error = tiz_pqueue_receive (p_queue, &p_received);
      if (i > 9)
        {
          fail_if (error != OMX_ErrorNoMore);
        }
      else
        {
          fail_if (error != OMX_ErrorNone);
        }

      fail_if (p_received == NULL);
      p_item = (int *) p_received;
      if (i < 9)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "item [%d]", *p_item);
          fail_if (*p_item != i);
          tiz_mem_free (p_received);
        }
    }

  tiz_pqueue_destroy (p_queue);
  tiz_soa_destroy (p_soa);
}
END_TEST

START_TEST (test_pqueue_send_and_receive_two_groups)
{

  OMX_U32 i;
  OMX_PTR p_received = NULL;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int *p_item = NULL;
  tiz_pqueue_t *p_queue = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_pqueue_send_and_receive_two_groups");

  error = tiz_pqueue_init (&p_queue, 1, &pqueue_cmp, NULL, "tizkrn");

  fail_if (error != OMX_ErrorNone);

  for (i = 0; i < 10; i++)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;
      error = tiz_pqueue_send (p_queue, p_item, i / 5);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "error [%p]", error);
      fail_if (error != OMX_ErrorNone);
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "queue length %d",
             tiz_pqueue_length (p_queue));
  fail_if (10 != tiz_pqueue_length (p_queue));

  for (i = 0; i < 5; i++)
    {
      error = tiz_pqueue_receive (p_queue, &p_received);
      fail_if (error != OMX_ErrorNone);
      fail_if (p_received == NULL);
      p_item = (int *) p_received;
      TIZ_LOG (TIZ_PRIORITY_TRACE, "item [%d]", *p_item);
      fail_if (*p_item != i);
      tiz_mem_free (p_received);
    }

  for (i = 5; i <= 10; i++)
    {
      error = tiz_pqueue_receive (p_queue, &p_received);
      if (i > 9)
        {
          fail_if (error != OMX_ErrorNoMore);
        }
      else
        {
          fail_if (error != OMX_ErrorNone);
        }

      fail_if (p_received == NULL);
      p_item = (int *) p_received;

      if (i < 9)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "item [%d]", *p_item);
          fail_if (*p_item != i);
          tiz_mem_free (p_received);
        }
    }

  tiz_pqueue_destroy (p_queue);
}
END_TEST

START_TEST (test_pqueue_send_and_receive_three_groups)
{

  OMX_U32 i;
  OMX_PTR p_received = NULL;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int *p_item = NULL;
  tiz_pqueue_t *p_queue = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_pqueue_send_and_receive_three_groups");

  error = tiz_pqueue_init (&p_queue, 2, &pqueue_cmp, NULL, "tizkrn");

  fail_if (error != OMX_ErrorNone);

  for (i = 0; i < 3; i++)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;
      error = tiz_pqueue_send (p_queue, p_item, 0);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "error [%X]", error);
      fail_if (error != OMX_ErrorNone);
    }

  for (i = 3; i < 7; i++)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;
      error = tiz_pqueue_send (p_queue, p_item, 1);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "error [%X]", error);
      fail_if (error != OMX_ErrorNone);
    }

  for (i = 7; i < 10; i++)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;
      error = tiz_pqueue_send (p_queue, p_item, 2);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "error [%X]", error);
      fail_if (error != OMX_ErrorNone);
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "queue length %d",
             tiz_pqueue_length (p_queue));
  fail_if (10 != tiz_pqueue_length (p_queue));

  for (i = 0; i < 3; i++)
    {
      error = tiz_pqueue_receive (p_queue, &p_received);
      fail_if (error != OMX_ErrorNone);
      fail_if (p_received == NULL);
      p_item = (int *) p_received;
      TIZ_LOG (TIZ_PRIORITY_TRACE, "item [%d]", *p_item);
      fail_if (*p_item != i);
      tiz_mem_free (p_received);
    }

  for (i = 3; i < 7; i++)
    {
      error = tiz_pqueue_receive (p_queue, &p_received);
      fail_if (error != OMX_ErrorNone);
      fail_if (p_received == NULL);
      p_item = (int *) p_received;
      TIZ_LOG (TIZ_PRIORITY_TRACE, "item [%d]", *p_item);
      fail_if (*p_item != i);
      tiz_mem_free (p_received);
    }

  for (i = 7; i <= 10; i++)
    {
      error = tiz_pqueue_receive (p_queue, &p_received);
      if (i > 9)
        {
          fail_if (error != OMX_ErrorNoMore);
        }
      else
        {
          fail_if (error != OMX_ErrorNone);
        }

      fail_if (p_received == NULL);
      p_item = (int *) p_received;

      if (i < 10)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "item [%d]", *p_item);
          fail_if (*p_item != i);
          tiz_mem_free (p_received);
        }
    }

  tiz_pqueue_destroy (p_queue);

}
END_TEST

START_TEST (test_pqueue_first)
{

  OMX_S32 i;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_pqueue_t *p_queue = NULL;
  OMX_PTR p_received = NULL;
  int *p_item = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_pqueue_first");

  error = tiz_pqueue_init (&p_queue, 2, &pqueue_cmp, NULL, "tizkrn");

  fail_if (error != OMX_ErrorNone);

  error = tiz_pqueue_first (p_queue, &p_received);

  fail_if (error != OMX_ErrorNoMore);

  for (i = 2; i >= 0; i--)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;
      error = tiz_pqueue_send (p_queue, p_item, i);
      fail_if (error != OMX_ErrorNone);
    }

  error = tiz_pqueue_first (p_queue, &p_received);
  fail_if (error != OMX_ErrorNone);
  p_item = (int *) p_received;
  TIZ_LOG (TIZ_PRIORITY_TRACE, "*p_item [%d]", *p_item);
  fail_if (*p_item != 0);

  for (i = 0; i <= 2; i++)
    {
      error = tiz_pqueue_receive (p_queue, &p_received);
      fail_if (error != OMX_ErrorNone);
      p_item = (int *) p_received;
      TIZ_LOG (TIZ_PRIORITY_TRACE, "*p_item [%d]", *p_item);
      fail_if (*p_item != i);
      tiz_mem_free (p_received);
    }

  tiz_pqueue_destroy (p_queue);

}
END_TEST

START_TEST (test_pqueue_remove)
{

  OMX_S32 i;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_pqueue_t *p_queue = NULL;
  OMX_PTR p_received = NULL;
  int *p_item = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_pqueue_remove");

  error = tiz_pqueue_init (&p_queue, 2, &pqueue_cmp, NULL, "tizkrn");

  fail_if (error != OMX_ErrorNone);

  error = tiz_pqueue_first (p_queue, &p_received);

  fail_if (error != OMX_ErrorNoMore);

  for (i = 2; i >= 0; i--)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;
      error = tiz_pqueue_send (p_queue, p_item, i);
      fail_if (error != OMX_ErrorNone);
    }

  tiz_pqueue_dump (p_queue, &pqueue_dump_item);

  error = tiz_pqueue_remove (p_queue, p_item);
  fail_if (error != OMX_ErrorNone);
  fail_if (tiz_pqueue_length (p_queue) != 2);

  tiz_pqueue_dump (p_queue, &pqueue_dump_item);

  for (i = 1; i <= 2; i++)
    {
      error = tiz_pqueue_receive (p_queue, &p_received);
      fail_if (error != OMX_ErrorNone);
      p_item = (int *) p_received;
      TIZ_LOG (TIZ_PRIORITY_TRACE, "*p_item [%d]", *p_item);
      fail_if (*p_item != i);
      tiz_mem_free (p_received);
    }

  tiz_pqueue_destroy (p_queue);

}
END_TEST

START_TEST (test_pqueue_removep)
{

  OMX_S32 i;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_pqueue_t *p_queue = NULL;
  OMX_PTR p_received = NULL;
  int *p_item = NULL;
  int *p_item_cat_one = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_pqueue_removep");

  error = tiz_pqueue_init (&p_queue, 2, &pqueue_cmp, NULL, "tizkrn");

  fail_if (error != OMX_ErrorNone);

  error = tiz_pqueue_first (p_queue, &p_received);

  fail_if (error != OMX_ErrorNoMore);

  for (i = 2; i >= 0; i--)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;
      error = tiz_pqueue_send (p_queue, p_item, i);
      fail_if (error != OMX_ErrorNone);
      if (1 == i)
        {
          p_item_cat_one = p_item;
        }
    }

  tiz_pqueue_dump (p_queue, &pqueue_dump_item);

  error = tiz_pqueue_removep (p_queue, p_item_cat_one, 1);
  fail_if (error != OMX_ErrorNone);
  fail_if (tiz_pqueue_length (p_queue) != 2);

  tiz_pqueue_dump (p_queue, &pqueue_dump_item);

  for (i = 0; i < 2; i++)
    {
      error = tiz_pqueue_receive (p_queue, &p_received);
      fail_if (error != OMX_ErrorNone);
      p_item = (int *) p_received;
      TIZ_LOG (TIZ_PRIORITY_TRACE, "*p_item [%d]", *p_item);
      if (0 == i)
        fail_if (*p_item != 0);
      if (1 == i)
        fail_if (*p_item != 2);
      tiz_mem_free (p_received);
    }

  tiz_pqueue_destroy (p_queue);

}
END_TEST

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */
