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
 * @file   check_sem.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Semaphore API unit tests
 *
 *
 */

START_TEST (test_sem_init_and_destroy)
{

  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_sem_t sem;
  error = tiz_sem_init (&sem, 1);

  fail_if (error != OMX_ErrorNone);

  tiz_sem_destroy (&sem);

}
END_TEST

START_TEST (test_sem_post_and_wait)
{

  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_sem_t sem;
  error = tiz_sem_init (&sem, 1);

  fail_if (error != OMX_ErrorNone);

  tiz_sem_post (&sem);
  tiz_sem_wait (&sem);

  tiz_sem_destroy (&sem);

}
END_TEST

START_TEST (test_sem_init_null)
{
  OMX_ERRORTYPE error = tiz_sem_init (0, 0);
  (void)error;
}
END_TEST

START_TEST (test_sem_destroy_null)
{
  OMX_ERRORTYPE error = tiz_sem_destroy (0);
  (void)error;
}
END_TEST

START_TEST (test_sem_wait_null)
{
  OMX_ERRORTYPE error = tiz_sem_wait (0);
  (void)error;
}
END_TEST

START_TEST (test_sem_post_null)
{
  OMX_ERRORTYPE error = tiz_sem_post (0);
  (void)error;
}
END_TEST

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */

