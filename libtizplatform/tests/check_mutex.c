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
 * @file   check_mutex.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Mutex API unit tests
 *
 *
 */

START_TEST (test_mutex_init_and_destroy)
{

  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_mutex_t mutex;
  error = tiz_mutex_init (&mutex);

  fail_if (error != OMX_ErrorNone);

  tiz_mutex_destroy (&mutex);

}
END_TEST

START_TEST (test_mutex_lock_and_unlock)
{

  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_mutex_t mutex;
  error = tiz_mutex_init (&mutex);

  fail_if (error != OMX_ErrorNone);

  tiz_mutex_lock (&mutex);
  tiz_mutex_unlock (&mutex);

  tiz_mutex_destroy (&mutex);

}
END_TEST

START_TEST (test_mutex_init_null)
{
  OMX_ERRORTYPE error = tiz_mutex_init (0);
  (void)error;
}
END_TEST

START_TEST (test_mutex_destroy_null)
{
  OMX_ERRORTYPE error = tiz_mutex_destroy (0);
  fail_if (OMX_ErrorNone != error);
}
END_TEST

START_TEST (test_mutex_lock_null)
{
  OMX_ERRORTYPE error = tiz_mutex_lock (0);
  (void)error;
}
END_TEST

START_TEST (test_mutex_unlock_null)
{
  OMX_ERRORTYPE error = tiz_mutex_unlock (0);
  (void)error;
}
END_TEST

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */
