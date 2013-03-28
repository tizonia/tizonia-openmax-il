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
 * @file   check_queue.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Synchronized queue API unit tests
 *
 *
 */


START_TEST (test_queue_init_and_destroy)
{

  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_queue_t *p_queue = NULL;

  error = tiz_queue_init (&p_queue, 10);

  fail_if (error != OMX_ErrorNone);

  tiz_queue_destroy (p_queue);

}
END_TEST

START_TEST (test_queue_send_and_receive)
{

  OMX_U32 i;
  OMX_PTR p_received = NULL;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int *p_item = NULL;
  tiz_queue_t *p_queue = NULL;;

  error = tiz_queue_init (&p_queue, 10);

  fail_if (error != OMX_ErrorNone);

  for (i = 0; i < 10; i++)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;
      error = tiz_queue_send (p_queue, p_item);
      fail_if (error != OMX_ErrorNone);
      TIZ_LOG (TIZ_TRACE, "queue length [%d]",
                 tiz_queue_length (p_queue));
    }

  TIZ_LOG (TIZ_TRACE, "queue length [%d]",
             tiz_queue_length (p_queue));
  fail_if (10 != tiz_queue_length (p_queue));

  for (i = 0; i < 10; i++)
    {
      error = tiz_queue_receive (p_queue, &p_received);
      fail_if (error != OMX_ErrorNone);
      fail_if (p_received == NULL);
      p_item = (int *) p_received;
      fail_if (*p_item != i);
      tiz_mem_free (p_received);
    }

  tiz_queue_destroy (p_queue);

}
END_TEST

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */
