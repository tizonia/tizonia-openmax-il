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
 * @file   tizosalsync.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Semaphore, mutex, and conditional variable
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizosal.h"

#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <assert.h>

#include <pthread.h>
#include <semaphore.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.osal.sync"
#endif

#define SEM_SUCCESS 0
#define PTHREAD_SUCCESS 0

OMX_ERRORTYPE
tiz_sem_init (tiz_sem_t * app_sem, OMX_U32 a_value)
{
  sem_t *p_sem = 0;

  assert (app_sem);

  if (!(p_sem = (sem_t *) tiz_mem_alloc (sizeof (sem_t))))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorInsufficientResources");
      return OMX_ErrorInsufficientResources;
    }

  if (SEM_SUCCESS != sem_init (p_sem, 0, a_value))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (errno));
      return OMX_ErrorUndefined;
    }

  *app_sem = p_sem;

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_sem_destroy (tiz_sem_t * app_sem)
{
  sem_t *p_sem;

  assert (app_sem);
  p_sem = *app_sem;

  if (p_sem && (SEM_SUCCESS != sem_destroy (p_sem)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (errno));
      return OMX_ErrorUndefined;
    }

  tiz_mem_free (p_sem);
  *app_sem = 0;

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_sem_wait (tiz_sem_t * app_sem)
{
  sem_t *p_sem;

  assert (NULL != app_sem);
  assert (NULL != *app_sem);

  p_sem = *app_sem;

  if (SEM_SUCCESS != sem_wait (p_sem))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (errno));
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_sem_post (tiz_sem_t * app_sem)
{
  sem_t *p_sem;

  assert (NULL != app_sem);
  assert (NULL != *app_sem);

  p_sem = *app_sem;

  if (SEM_SUCCESS != sem_post (p_sem))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (errno));
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_sem_getvalue (tiz_sem_t * app_sem, OMX_S32 * ap_sval)
{
  sem_t *p_sem;

  assert (NULL != app_sem);
  assert (NULL != *app_sem);

  p_sem = *app_sem;

  if (SEM_SUCCESS != sem_getvalue (p_sem, (int *) ap_sval))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (errno));
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_mutex_init (tiz_mutex_t * app_mutex)
{
  pthread_mutex_t *p_mutex;
  int error = 0;

  assert (NULL != app_mutex);

  if (!(p_mutex =
        (pthread_mutex_t *) tiz_mem_alloc (sizeof (pthread_mutex_t))))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorInsufficientResources");
      return OMX_ErrorInsufficientResources;
    }

  if (PTHREAD_SUCCESS != (error = pthread_mutex_init (p_mutex, NULL)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  *app_mutex = p_mutex;

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_mutex_destroy (tiz_mutex_t * app_mutex)
{
  pthread_mutex_t *p_mutex;
  int error = 0;

  assert (NULL != app_mutex);
  p_mutex = *app_mutex;

  if (p_mutex
      && (PTHREAD_SUCCESS != (error = pthread_mutex_destroy (p_mutex))))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  tiz_mem_free (p_mutex);
  *app_mutex = 0;

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_mutex_lock (tiz_mutex_t * app_mutex)
{
  pthread_mutex_t *p_mutex;
  int error;

  assert (NULL != app_mutex);
  assert (NULL != *app_mutex);

  p_mutex = *app_mutex;

  if (PTHREAD_SUCCESS != (error = pthread_mutex_lock (p_mutex)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_mutex_unlock (tiz_mutex_t * app_mutex)
{
  pthread_mutex_t *p_mutex;
  int error;

  assert (NULL != app_mutex);
  assert (NULL != *app_mutex);

  p_mutex = *app_mutex;

  if (PTHREAD_SUCCESS != (error = pthread_mutex_unlock (p_mutex)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_cond_init (tiz_cond_t * app_cond)
{
  pthread_cond_t *p_cond;
  int error;

  assert (NULL != app_cond);

  if (!(p_cond = (pthread_cond_t *) tiz_mem_alloc (sizeof (pthread_cond_t))))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorInsufficientResources");
      return OMX_ErrorInsufficientResources;
    }

  if (PTHREAD_SUCCESS != (error = pthread_cond_init (p_cond, NULL)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      assert (EINVAL != error);
      return OMX_ErrorUndefined;
    }

  *app_cond = p_cond;

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_cond_destroy (tiz_cond_t * app_cond)
{
  pthread_cond_t *p_cond;
  int error = 0;

  assert (NULL != app_cond);
  p_cond = *app_cond;

  if (p_cond && (PTHREAD_SUCCESS != (error = pthread_cond_destroy (p_cond))))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  tiz_mem_free (p_cond);
  *app_cond = 0;

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_cond_signal (tiz_cond_t * app_cond)
{
  pthread_cond_t *p_cond;
  int error = 0;

  assert (NULL != app_cond);
  assert (NULL != *app_cond);

  p_cond = *app_cond;

  if (PTHREAD_SUCCESS != (error = pthread_cond_signal (p_cond)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_cond_broadcast (tiz_cond_t * app_cond)
{
  pthread_cond_t *p_cond;
  int error = 0;

  assert (NULL != app_cond);
  assert (NULL != *app_cond);

  p_cond = *app_cond;

  if (PTHREAD_SUCCESS != (error = pthread_cond_broadcast (p_cond)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_cond_wait (tiz_cond_t * app_cond, tiz_mutex_t * app_mutex)
{
  pthread_cond_t *p_cond;
  pthread_mutex_t *p_mutex;
  int error = 0;

  assert (NULL != app_cond);
  assert (NULL != app_mutex);
  assert (NULL != *app_cond);
  assert (NULL != *app_mutex);

  p_cond = *app_cond;
  p_mutex = *app_mutex;

  if (PTHREAD_SUCCESS != (error = pthread_cond_wait (p_cond, p_mutex)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_cond_timedwait (tiz_cond_t * app_cond, tiz_mutex_t * app_mutex,
                    OMX_U32 a_millis)
{
  pthread_cond_t *p_cond;
  pthread_mutex_t *p_mutex;
  int error = 0;
  OMX_U32 timeout_us;
  struct timespec timeout;
  struct timeval now;

  assert (NULL != app_cond);
  assert (NULL != app_mutex);
  assert (NULL != *app_cond);
  assert (NULL != *app_mutex);

  p_cond = *app_cond;
  p_mutex = *app_mutex;

  gettimeofday (&now, NULL);
  timeout_us = now.tv_usec + 1000 * a_millis;
  timeout.tv_sec = now.tv_sec + timeout_us / 1000000;
  timeout.tv_nsec = (timeout_us % 1000000) * 1000;

  if (PTHREAD_SUCCESS != (error =
                          pthread_cond_timedwait (p_cond, p_mutex, &timeout)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_rwmutex_init (tiz_rwmutex_t * app_rwmutex)
{
  pthread_rwlock_t *p_mutex;
  int error = 0;

  assert (NULL != app_rwmutex);

  if (!(p_mutex =
        (pthread_rwlock_t *) tiz_mem_alloc (sizeof (pthread_rwlock_t))))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorInsufficientResources");
      return OMX_ErrorInsufficientResources;
    }

  if (PTHREAD_SUCCESS != (error = pthread_rwlock_init (p_mutex, NULL)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  *app_rwmutex = p_mutex;

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_rwmutex_destroy (tiz_rwmutex_t * app_rwmutex)
{
  pthread_rwlock_t *p_mutex;
  int error = 0;

  assert (NULL != app_rwmutex);
  p_mutex = *app_rwmutex;

  if (p_mutex
      && (PTHREAD_SUCCESS != (error = pthread_rwlock_destroy (p_mutex))))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  tiz_mem_free (p_mutex);
  *app_rwmutex = 0;

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_rwmutex_rdlock (tiz_rwmutex_t * app_rwmutex)
{
  pthread_rwlock_t *p_mutex;
  int error;

  assert (NULL != app_rwmutex);
  assert (NULL != *app_rwmutex);

  p_mutex = *app_rwmutex;

  if (PTHREAD_SUCCESS != (error = pthread_rwlock_rdlock (p_mutex)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_rwmutex_wrlock (tiz_rwmutex_t * app_rwmutex)
{
  pthread_rwlock_t *p_mutex;
  int error;

  assert (NULL != app_rwmutex);
  assert (NULL != *app_rwmutex);

  p_mutex = *app_rwmutex;

  if (PTHREAD_SUCCESS != (error = pthread_rwlock_wrlock (p_mutex)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tiz_rwmutex_unlock (tiz_rwmutex_t * app_rwmutex)
{
  pthread_rwlock_t *p_mutex;
  int error;

  assert (NULL != app_rwmutex);
  assert (NULL != *app_rwmutex);

  p_mutex = *app_rwmutex;

  if (PTHREAD_SUCCESS != (error = pthread_rwlock_unlock (p_mutex)))
    {
      TIZ_LOG (TIZ_ERROR, "OMX_ErrorUndefined : %s", strerror (error));
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}
