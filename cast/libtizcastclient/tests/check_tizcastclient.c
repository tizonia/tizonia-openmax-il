/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   check_rm_client.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - CAST client unit tests
 *
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <sys/types.h>
#include <limits.h>

#include "tizplatform.h"
#include "OMX_Core.h"
#include "OMX_Types.h"

#include "tizcastclient_c.h"
#include "tizcasttypes.h"

#include "check_tizcastclient.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.rm.client.check"
#endif

#define CAST_CLIENT_TEST_TIMEOUT 15

char *pg_rmdb_path     = NULL;
char *pg_sqlite_script = NULL;
char *pg_init_script   = NULL;
char *pg_dump_script   = NULL;
char *pg_rmd_path      = NULL;

#define COMPONENT1_NAME "OMX.Aratelia.ilcore.test_component"
#define COMPONENT1_PRIORITY 3
#define COMPONENT1_GROUP_ID 300

#define COMPONENT2_NAME "OMX.Aratelia.tizonia.test_component"
#define COMPONENT2_PRIORITY 2
#define COMPONENT2_GROUP_ID 200

#define INFINITE_WAIT 0xffffffff
/* duration of event timeout in msec when we expect event to be set */
#define TIMEOUT_EXPECTING_SUCCESS 500
/* duration of event timeout in msec when we don't expect event to be set */
#define TIMEOUT_EXPECTING_FAILURE 2000

typedef void *cc_ctx_t;
typedef struct check_common_context check_common_context_t;
struct check_common_context
{
  OMX_BOOL signaled;
  OMX_U32 rid;
  tiz_cast_t *pp_rm;
  tiz_mutex_t mutex;
  tiz_cond_t cond;
};

static void
setup (void)
{
  pg_rmdb_path = strndup (tiz_rcfile_get_value("resource-management", "rmdb"),
                          PATH_MAX);
  pg_sqlite_script = strndup (tiz_rcfile_get_value("resource-management",
                                                   "rmdb.sqlite_script"),
                              PATH_MAX);
  pg_init_script = strndup (tiz_rcfile_get_value("resource-management",
                                                "rmdb.init_script"),
                           PATH_MAX);
  pg_rmd_path = strndup (tiz_rcfile_get_value("resource-management",
                                             "rmd.path"),
                        PATH_MAX);
  pg_dump_script = strndup (tiz_rcfile_get_value("resource-management",
                                                "rmdb.dbdump_script"),
                           PATH_MAX);

  if (!pg_rmdb_path || !pg_sqlite_script
      || !pg_init_script || !pg_rmd_path || !pg_dump_script)
    {
      TIZ_LOG(TIZ_PRIORITY_TRACE, "Test data not available...");
      fail_if(0);
    }
  else
    {
      TIZ_LOG(TIZ_PRIORITY_TRACE, "CAST daemon [%s] ...", pg_rmd_path);
    }

}

static void
teardown (void)
{
  tiz_mem_free (pg_rmdb_path);
  tiz_mem_free (pg_sqlite_script);
  tiz_mem_free (pg_init_script);
  tiz_mem_free (pg_rmd_path);
  tiz_mem_free (pg_dump_script);
}

static bool
refresh_rm_db (void)
{
  bool rv = false;

  /* Re-fresh the rm db */
  size_t total_len = strlen (pg_init_script)
    + strlen (pg_sqlite_script)
    + strlen (pg_rmdb_path) + 4;
  char *p_cmd = tiz_mem_calloc (1, total_len);

  if (p_cmd)
    {
      snprintf(p_cmd, total_len -1, "%s %s %s",
               pg_init_script, pg_sqlite_script, pg_rmdb_path);
      if (-1 != system (p_cmd))
        {
          TIZ_LOG(TIZ_PRIORITY_TRACE, "Successfully run [%s] script...", p_cmd);
          rv = true;
        }
      else
        {
          TIZ_LOG(TIZ_PRIORITY_TRACE,
                  "Error while executing db init shell script...");
        }
      tiz_mem_free (p_cmd);
    }

  return rv;
}

static bool
dump_rmdb (const char * p_dest_path)
{
  bool rv = false;

  /* Dump the rm db */
  size_t total_len = strlen (pg_dump_script)
    + strlen (pg_rmdb_path)
    + strlen (p_dest_path) + 4;
  char *p_cmd = tiz_mem_calloc (1, total_len);

  if (p_cmd)
    {
      snprintf(p_cmd, total_len -1, "%s %s %s",
               pg_dump_script, pg_rmdb_path, p_dest_path);
      if (-1 != system (p_cmd))
        {
          TIZ_LOG(TIZ_PRIORITY_TRACE, "Successfully run [%s] script...", p_cmd);
          rv = true;
        }
      else
        {
          TIZ_LOG(TIZ_PRIORITY_TRACE,
                  "Error while executing db dump shell script...");
        }
      tiz_mem_free (p_cmd);
    }

  return rv;
}

static OMX_ERRORTYPE
_ctx_init (cc_ctx_t * app_ctx)
{
  check_common_context_t *p_ctx =
    tiz_mem_alloc (sizeof (check_common_context_t));

  if (!p_ctx)
    {
      return OMX_ErrorInsufficientResources;
    }

  p_ctx->signaled = OMX_FALSE;
  p_ctx->rid = 0;
  p_ctx->pp_rm = NULL;

  if (tiz_mutex_init (&p_ctx->mutex))
    {
      tiz_mem_free (p_ctx);
      return OMX_ErrorInsufficientResources;
    }

  if (tiz_cond_init (&p_ctx->cond))
    {
      tiz_mutex_destroy (&p_ctx->mutex);
      tiz_mem_free (p_ctx);
      return OMX_ErrorInsufficientResources;
    }

  * app_ctx = p_ctx;

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
_ctx_destroy (cc_ctx_t * app_ctx)
{
  check_common_context_t *p_ctx = NULL;
  assert (app_ctx);
  p_ctx = * app_ctx;

  if (tiz_mutex_lock (&p_ctx->mutex))
    {
      return OMX_ErrorBadParameter;
    }

  tiz_cond_destroy (&p_ctx->cond);
  tiz_mutex_unlock (&p_ctx->mutex);
  tiz_mutex_destroy (&p_ctx->mutex);

  tiz_mem_free (p_ctx);

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
_ctx_signal (cc_ctx_t * app_ctx)
{
  check_common_context_t *p_ctx = NULL;
  assert (app_ctx);
  p_ctx = * app_ctx;

  if (tiz_mutex_lock (&p_ctx->mutex))
    {
      return OMX_ErrorBadParameter;
    }

  p_ctx->signaled = OMX_TRUE;
  tiz_cond_signal (&p_ctx->cond);
  tiz_mutex_unlock (&p_ctx->mutex);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
_ctx_wait (cc_ctx_t * app_ctx, OMX_U32 a_millis, OMX_BOOL * ap_has_timedout)
{
  int retcode;
  check_common_context_t *p_ctx = NULL;
  assert (app_ctx);
  p_ctx = * app_ctx;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "a_millis [%u]", a_millis);

  * ap_has_timedout = OMX_FALSE;

  if (tiz_mutex_lock (&p_ctx->mutex))
    {
      return OMX_ErrorBadParameter;
    }

  if (0 == a_millis)
    {
      if (!p_ctx->signaled)
        {
          * ap_has_timedout = OMX_TRUE;
        }
    }

  else if (INFINITE_WAIT == a_millis)
    {
      while (!p_ctx->signaled)
        {
          tiz_cond_wait (&p_ctx->cond, &p_ctx->mutex);
        }
    }

  else
    {
      while (!p_ctx->signaled)
        {
          retcode = tiz_cond_timedwait (&p_ctx->cond,
                                          &p_ctx->mutex, a_millis);

          /* TODO: Change this to OMX_ErrorTimeout */
          if (retcode == OMX_ErrorUndefined && !p_ctx->signaled)
            {
              * ap_has_timedout = OMX_TRUE;
              break;
            }
        }
    }

  tiz_mutex_unlock (&p_ctx->mutex);

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
_ctx_reset (cc_ctx_t * app_ctx)
{
  check_common_context_t *p_ctx = NULL;
  assert (app_ctx);
  p_ctx = * app_ctx;

  if (tiz_mutex_lock (&p_ctx->mutex))
    {
      return OMX_ErrorBadParameter;
    }

  p_ctx->signaled = OMX_FALSE;
  tiz_mutex_unlock (&p_ctx->mutex);

  return OMX_ErrorNone;
}

/* Callback function to signal a client when a wait for resource has ended */
void
check_tizcastclient_comp1_wait_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE,
             "check_tizcastclient_comp1_wait_complete : rid [%u]", rid);
  fail_if (0);
}

/* Callback function to signal the client when a resource needs to be
   freed */
void
check_tizcastclient_comp1_preemption_req (OMX_U32 rid, OMX_PTR ap_data)
{
  check_common_context_t *p_ctx = NULL;
  cc_ctx_t *pp_ctx = NULL;
  assert (ap_data);
  pp_ctx = (cc_ctx_t *) ap_data;
  p_ctx = *pp_ctx;

  TIZ_LOG (TIZ_PRIORITY_TRACE,
             "check_tizcastclient_comp1_preemption_req : rid [%u]", rid);

  p_ctx->rid = rid;
  _ctx_signal (pp_ctx);
}

void
check_tizcastclient_comp1_preemption_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE,
             "check_tizcastclient_comp1_preemption_complete : rid [%u]", rid);
  fail_if (0);
}

/* Callback function to signal a client when a wait for resource has ended */
void
check_tizcastclient_comp2_wait_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  check_common_context_t *p_ctx = NULL;
  cc_ctx_t *pp_ctx = NULL;
  assert (ap_data);
  pp_ctx = (cc_ctx_t *) ap_data;
  p_ctx = *pp_ctx;

  TIZ_LOG (TIZ_PRIORITY_TRACE,
             "check_tizcastclient_comp2_wait_complete : rid [%u]", rid);

  p_ctx->rid = rid;
  _ctx_signal (pp_ctx);
}

/* Callback function to signal the client when a resource needs to be
   freed */
void
check_tizcastclient_comp2_preemption_req (OMX_U32 rid, OMX_PTR ap_data)
{
  check_common_context_t *p_ctx = NULL;
  cc_ctx_t *pp_ctx = NULL;
  assert (ap_data);
  pp_ctx = (cc_ctx_t *) ap_data;
  p_ctx = *pp_ctx;

  TIZ_LOG (TIZ_PRIORITY_TRACE,
             "check_tizcastclient_comp2_preemption_req : rid [%u]", rid);

  p_ctx->rid = rid;
  _ctx_signal (pp_ctx);
}

void
check_tizcastclient_comp2_preemption_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  check_common_context_t *p_ctx = NULL;
  cc_ctx_t *pp_ctx = NULL;
  assert (ap_data);
  pp_ctx = (cc_ctx_t *) ap_data;
  p_ctx = *pp_ctx;

  TIZ_LOG (TIZ_PRIORITY_TRACE,
             "check_tizcastclient_comp2_preemption_complete : rid [%u]", rid);

  p_ctx->rid = rid;
  _ctx_signal (pp_ctx);
}

pid_t
check_tizcastclient_find_proc (const char *name)
{
  DIR *dir;
  struct dirent *ent;
  char *endptr;
  char buf[512];

  if (!(dir = opendir ("/proc")))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Could not open /proc");
      return -1;
    }

  while ((ent = readdir (dir)) != NULL)
    {
      /* if endptr is not a null character, the directory is not
       * entirely numeric, so ignore it */
      long lpid = strtol (ent->d_name, &endptr, 10);
      if (*endptr != '\0')
        {
          continue;
        }

      /* try to open the cmdline file */
      snprintf (buf, sizeof (buf), "/proc/%ld/cmdline", lpid);
      FILE *fp = fopen (buf, "r");

      if (fp)
        {
          if (fgets (buf, sizeof (buf), fp) != NULL)
            {
              /* check the first token in the file, the program name */
              char *first = strtok (buf, " ");
              TIZ_LOG (TIZ_PRIORITY_TRACE, "buf [%s]", buf);
              if (first && strstr (first, name))
                {
                  fclose (fp);
                  closedir (dir);
                  TIZ_LOG (TIZ_PRIORITY_TRACE, "Found process [%s] --> PID [%d]",
                             name, lpid);
                  return (pid_t) lpid;
                }
            }
          fclose (fp);
        }
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Process [%s] NOT FOUND", name);

  closedir (dir);
  return -1;
}

START_TEST (test_client_acquire_and_release)
{
  tiz_cast_error_t error = TIZ_CAST_SUCCESS;
  int rc, daemon_existed = 1;
  tiz_cast_t p_rm;;
  pid_t pid;
  OMX_UUIDTYPE uuid_omx;
  OMX_PRIORITYMGMTTYPE primgmt;
  tiz_cast_client_callbacks_t cbacks;

  /* Check if a CAST daemon is running already */
  if ((pid = check_tizcastclient_find_proc ("tizcastd"))
      || (pid = check_tizcastclient_find_proc ("lt-tizcastd")))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "CAST process [PID %d] FOUND", pid);
    }

  if (-1 == pid)
    {
      /* Start the rm daemon */
      pid = fork ();
      fail_if (pid == -1);
      daemon_existed = 0;
    }

  if (pid)
    {

      sleep (1);

      /* Generate a uuid */
      tiz_uuid_generate (&uuid_omx);

      primgmt.nSize = sizeof (OMX_PRIORITYMGMTTYPE);
      primgmt.nVersion.nVersion = OMX_VERSION;
      primgmt.nGroupPriority = COMPONENT1_PRIORITY;
      primgmt.nGroupID = COMPONENT1_GROUP_ID;

      cbacks.pf_waitend = &check_tizcastclient_comp1_wait_complete;
      cbacks.pf_preempt = &check_tizcastclient_comp1_preemption_req;
      cbacks.pf_preempt_end = &check_tizcastclient_comp1_preemption_complete;

      TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_cast_client_init");
      error =
        tiz_cast_client_init (&p_rm, COMPONENT1_NAME,
                          (const OMX_UUIDTYPE *) &uuid_omx, &primgmt, &cbacks,
                          NULL);
      fail_if (error != TIZ_CAST_SUCCESS);

      /* error = tiz_cast_client_version(&p_rm); */
      error = tiz_cast_client_acquire (&p_rm, TIZ_CAST_RESOURCE_DUMMY, 1);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_cast_client_acquire returned [%d]", error);
      fail_if (error != TIZ_CAST_SUCCESS);

      error = tiz_cast_client_release (&p_rm, TIZ_CAST_RESOURCE_DUMMY, 1);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_cast_client_release returned [%d]", error);
      fail_if (error != TIZ_CAST_SUCCESS);

      error = tiz_cast_client_destroy (&p_rm);
      fail_if (error != TIZ_CAST_SUCCESS);

      if (!daemon_existed)
        {
          error = kill (pid, SIGTERM);
          fail_if (error == -1);
        }

      /* Check db */
      fail_if (!dump_rmdb ("test_client_acquire_and_release.after.dump"));

      rc =
        system
        ("cmp -s /tmp/test_client_acquire_and_release.before.dump /tmp/test_client_acquire_and_release.after.dump");

      TIZ_LOG (TIZ_PRIORITY_TRACE, "DB comparison check [%s]",
                 (rc == 0 ? "SUCCESS" : "FAILED"));
      fail_if (rc != 0);

    }
  else
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Starting the CAST daemon");
      const char *arg0 = "";
      error = execlp (pg_rmd_path, arg0, (char *) NULL);
      fail_if (error == -1);
    }
}

Suite *
rmclient_suite (void)
{
  TCase *tc_client;
  Suite *s = suite_create ("libtizcastclient");

  putenv(TIZ_PLATFORM_RC_FILE_ENV);

  /* test case */
  tc_client = tcase_create ("CAST client");
  tcase_add_unchecked_fixture (tc_client, setup, teardown);
  tcase_set_timeout (tc_client, CAST_CLIENT_TEST_TIMEOUT);
  tcase_add_test (tc_client, test_client_acquire_and_release);
  suite_add_tcase (s, tc_client);

  return s;
}

int
main (void)
{
  int number_failed;
  SRunner *sr = srunner_create (rmclient_suite ());

  tiz_log_init();

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Tizonia OpenMAX IL - Chromecast client unit tests");

  /* Enable extra debugging info for D-BUSXX */
  /* setenv ("DBUSXX_VERBOSE", "1", 0); */

  srunner_run_all (sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  tiz_log_deinit ();

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
