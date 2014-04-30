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
 * @file   tizlog.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - Logging API implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <alloca.h>

#include <log4c.h>
#include <log4c/appender.h>
#include <log4c/appender_type_rollingfile.h>
#include <log4c/rollingpolicy.h>

#include "tizlog.h"

typedef struct user_locinfo user_locinfo_t;
struct user_locinfo
{
  int pid;
  int tid;
  const char *cname;
  char *cbuf;
};

static const char *log_layout_format (const log4c_layout_t *a_layout,
                                          const log4c_logging_event_t *a_event)
{
  static char buffer[4096];
  user_locinfo_t *uloc = NULL;

  if (a_event->evt_loc->loc_data != NULL)
    {
      struct tm tm;
      gmtime_r (&a_event->evt_timestamp.tv_sec, &tm);
      uloc = (user_locinfo_t *)a_event->evt_loc->loc_data;

      if (NULL == uloc->cname)
        {
          snprintf (buffer, sizeof(buffer),
                    "%02d-%02d-%04d %02d:%02d:%02d.%03ld - "
                    "[PID:%i][TID:%i] [%s] [%s] [%s:%s:%i] --- %s\n",
                    tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour,
                    tm.tm_min, tm.tm_sec, a_event->evt_timestamp.tv_usec / 1000,
                    uloc->pid, uloc->tid,
                    log4c_priority_to_string (a_event->evt_priority),
                    a_event->evt_category, a_event->evt_loc->loc_file,
                    a_event->evt_loc->loc_function, a_event->evt_loc->loc_line,
                    a_event->evt_msg);
        }
      else
        {
          /* TODO: 4096 - this value needs be learnt at project configuration
           * time */
          snprintf (uloc->cbuf, 4096,
                    "%02d-%02d-%04d %02d:%02d:%02d.%03ld - "
                    "[PID:%i][TID:%i] [%s] [%s] [%s:%s:%i] --- %s\n",
                    tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour,
                    tm.tm_min, tm.tm_sec, a_event->evt_timestamp.tv_usec / 1000,
                    uloc->pid, uloc->tid,
                    log4c_priority_to_string (a_event->evt_priority),
                    uloc->cname, a_event->evt_loc->loc_file,
                    a_event->evt_loc->loc_function, a_event->evt_loc->loc_line,
                    a_event->evt_msg);

          return uloc->cbuf;
        }
    }
  else
    {
      sprintf (buffer, "[%s] [%s] [%s:%s:%i] --- %s\n",
               log4c_priority_to_string (a_event->evt_priority),
               a_event->evt_category, a_event->evt_loc->loc_file,
               a_event->evt_loc->loc_function, a_event->evt_loc->loc_line,
               a_event->evt_msg);
    }

  return buffer;
}

const log4c_layout_type_t tizonia_log_layout
    = { "tiz_layout", log_layout_format, };

static const log4c_layout_type_t *const layout_types[] = { &tizonia_log_layout };

static int nlayout_types
    = (int)(sizeof(layout_types) / sizeof(layout_types[0]));

static int log_formatters_init ()
{
  int rc = 0;
  int i = 0;

  for (i = 0; i < nlayout_types; i++)
    {
      log4c_layout_type_set (layout_types[i]);
    }

  return rc;
}

int tiz_log_init ()
{
#ifndef WITHOUT_LOG4C
  log_formatters_init ();
  return (log4c_init ());
#else
  return 0;
#endif
}

void tiz_log_set_unique_rolling_file (const char* ap_logdir, const char * ap_file_prefix)
{
  assert (NULL != ap_logdir);
  assert (NULL != ap_file_prefix);

  if (NULL == ap_logdir || NULL == ap_file_prefix)
    {
      return;
    }

  {
    log4c_appender_t *app = log4c_appender_get ("tizlogfile");

    if (NULL != app)
      {
        rollingfile_udata_t *rfup = log4c_appender_get_udata(app);
        if (NULL != rfup)
          {
            char buffer[128];
            pid_t pid = getpid ();
            rollingfile_udata_set_logdir(rfup, (char *) ap_logdir);
            snprintf (buffer, 128, "%s.%i.%s", ap_file_prefix, pid, "log");
            rollingfile_udata_set_files_prefix(rfup, buffer);

            /* recover a rollingpolicy instance with this name */
            log4c_rollingpolicy_t *rollingpolicyp
              = log4c_rollingpolicy_get("tizrolling");
            if (NULL != rollingpolicyp)
              {
                /* connect that policy to this rollingfile appender conf */
                rollingfile_udata_set_policy(rfup, rollingpolicyp);
                log4c_appender_set_udata(app, rfup);

                /* allow the policy to initialize itself */
                log4c_rollingpolicy_init(rollingpolicyp, rfup);
              }
          }
      }
  }
}

int tiz_log_deinit ()
{
#ifndef WITHOUT_LOG4C
  return (log4c_fini ());
#else
  return 0;
#endif
}

void tiz_log (const char *ap_file, int a_line, const char *ap_func,
              const char *ap_cat_name, int a_priority, const char *ap_cname,
              char *ap_cbuf, const char *ap_format, ...)
{
#ifndef WITHOUT_LOG4C
  log4c_location_info_t locinfo;
  user_locinfo_t user_locinfo;

  const log4c_category_t *p_category = log4c_category_get (ap_cat_name);
  if (log4c_category_is_priority_enabled (p_category, a_priority))
    {
      /* TODO: 4096 - this value should to be obtained at config time */
      char *buffer = alloca (4096);
      user_locinfo.pid = getpid ();
      user_locinfo.tid = syscall (SYS_gettid);
      user_locinfo.cname = ap_cname;
      user_locinfo.cbuf = ap_cbuf;
      locinfo.loc_file = ap_file;
      locinfo.loc_line = a_line;
      locinfo.loc_function = ap_func;
      /*          locinfo.loc_data = NULL; */
      locinfo.loc_data = &user_locinfo;

      va_list va;
      va_start (va, ap_format);
      vsprintf (buffer, ap_format, va);
      va_end (va);
      log4c_category_log_locinfo (p_category, &locinfo, a_priority, "%s",
                                  buffer);
    }
#else

  va_list va;
  va_start (va, ap_format);
  vprintf (ap_format, va);
  va_end (va);
  printf ("\n");

#endif
}

/*  TODO: Allow override the logging configuration via command line */
/*        const int overwrite = 1; */
/*        setenv("LOG4C_PRIORITY", "error", overwrite); */
/*        setenv("LOG4C_APPENDER", "stderr", overwrite); */
