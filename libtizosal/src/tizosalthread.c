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
 * @file   tizosalthread.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Thread/task management
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE

#include "tizosal.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.osal.thread"
#endif

#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN (2 * 1024 * 1024)     /* Currently, this is the minimum
                                                 * in glibc for x86 32 and 64
                                                 * archs */
#endif

#define PTHREAD_SUCCESS 0

OMX_ERRORTYPE
tiz_thread_create (tiz_thread_t * ap_thread,
                   size_t a_stack_size,
                   OMX_U32 a_priority,
                   OMX_PTR (*a_pf_routine) (OMX_PTR), OMX_PTR ap_arg)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  pthread_attr_t custom_attr;
  struct sched_param priority_holder;
  size_t stack_size;
  int error = 0;

  assert (NULL != ap_thread);
  assert (NULL != a_pf_routine);

  /* TODO: Check for wrong priority values */
  /*    int    sched_get_priority_max(int); */
  /*    int    sched_get_priority_min(int); */

  if (PTHREAD_SUCCESS != (error = pthread_attr_init (&custom_attr)))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorInsufficientResources] : "
               "Could not initialize the thread attributes (%s).",
               strerror (error));
      rc = OMX_ErrorInsufficientResources;
    }
  else
    {
      stack_size = (a_stack_size < PTHREAD_STACK_MIN) ?
        PTHREAD_STACK_MIN : a_stack_size;

      /* set stack size */
      if (PTHREAD_SUCCESS
          != (error = pthread_attr_setstacksize (&custom_attr, stack_size)))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorInsufficientResources] : "
                   "Could not sets the stack size attribute (%s).",
                   strerror (error));
          rc = OMX_ErrorInsufficientResources;
        }
      else
        {
          /* set a priority relative to the default priority */
          if (a_priority != 0)
            {
              priority_holder.sched_priority = 0;
              (void) pthread_attr_getschedparam (&custom_attr,
                                                 &priority_holder);
              priority_holder.sched_priority += a_priority;
              if (PTHREAD_SUCCESS
                  != (error = pthread_attr_setschedparam (&custom_attr,
                                                          &priority_holder)))
                {
                  TIZ_LOG (TIZ_PRIORITY_ERROR, "Could not set the thread "
                           "priority (%s). Continuing...", strerror (error));
                }
            }

          /* Leave detachstate attribute with its default value: */
          /*    PTHREAD_CREATE_JOINABLE */

          /* create thread */
          if (PTHREAD_SUCCESS
              != (error = pthread_create (ap_thread, &custom_attr,
                                          a_pf_routine, (void *) ap_arg)))
            {
              TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorInsufficientResources] : "
                       "Could not create the thread (%s). ",
                       strerror (error));
              rc = OMX_ErrorInsufficientResources;
            }
        }

      (void) pthread_attr_destroy (&custom_attr);
    }

  return rc;
}

void
tiz_thread_exit (OMX_PTR a_status)
{
  pthread_exit (a_status);
}

OMX_ERRORTYPE
tiz_thread_join (tiz_thread_t * ap_thread, void **app_result)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int error = 0;

  assert (NULL != ap_thread);
  assert (NULL != app_result);

  if (PTHREAD_SUCCESS != (error = pthread_join (*ap_thread, app_result)))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "Could not join the thread (%s). "
               "Leaving with OMX_ErrorUndefined.", strerror (error));
      rc = OMX_ErrorUndefined;
    }

  return rc;
}

OMX_S32
tiz_thread_id (void)
{
  return syscall (SYS_gettid);
}

OMX_ERRORTYPE
tiz_thread_setname (tiz_thread_t * ap_thread, const OMX_STRING a_name)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int error = 0;

  assert (NULL != ap_thread);
  assert (NULL != a_name);

  if (PTHREAD_SUCCESS != (error = pthread_setname_np (*ap_thread, a_name)))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "Could not set the thread's name (%s). "
               "Leaving with OMX_ErrorUndefined.", strerror (error));
      rc = OMX_ErrorUndefined;
    }

  return rc;
}

OMX_S32
tiz_sleep (OMX_U32 usec)
{
  OMX_S32 rc = 0;

  if (1000000 < usec)
    {
      rc = usleep (1000000);
    }
  else
    {
      rc = usleep ((useconds_t) usec);
    }

  if (0 != rc)
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "OMX_ErrorUndefined : ");
    }

  return rc;
}
