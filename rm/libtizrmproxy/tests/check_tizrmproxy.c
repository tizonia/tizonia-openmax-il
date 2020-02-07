/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @brief  Tizonia OpenMAX IL - RM client unit tests
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

#include "tizrmproxy_c.h"
#include "tizrmtypes.h"

#include "check_tizrmproxy.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.rm.proxy.check"
#endif

#define RMPROXY_TEST_TIMEOUT 15

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
  tiz_rm_t *pp_rm;
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
      TIZ_LOG(TIZ_PRIORITY_TRACE, "RM daemon [%s] ...", pg_rmd_path);
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
check_tizrmproxy_comp1_wait_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE,
             "check_tizrmproxy_comp1_wait_complete : rid [%u]", rid);
  fail_if (0);
}

/* Callback function to signal the client when a resource needs to be
   freed */
void
check_tizrmproxy_comp1_preemption_req (OMX_U32 rid, OMX_PTR ap_data)
{
  check_common_context_t *p_ctx = NULL;
  cc_ctx_t *pp_ctx = NULL;
  assert (ap_data);
  pp_ctx = (cc_ctx_t *) ap_data;
  p_ctx = *pp_ctx;

  TIZ_LOG (TIZ_PRIORITY_TRACE,
             "check_tizrmproxy_comp1_preemption_req : rid [%u]", rid);

  p_ctx->rid = rid;
  _ctx_signal (pp_ctx);
}

void
check_tizrmproxy_comp1_preemption_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE,
             "check_tizrmproxy_comp1_preemption_complete : rid [%u]", rid);
  fail_if (0);
}

/* Callback function to signal a client when a wait for resource has ended */
void
check_tizrmproxy_comp2_wait_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  check_common_context_t *p_ctx = NULL;
  cc_ctx_t *pp_ctx = NULL;
  assert (ap_data);
  pp_ctx = (cc_ctx_t *) ap_data;
  p_ctx = *pp_ctx;

  TIZ_LOG (TIZ_PRIORITY_TRACE,
             "check_tizrmproxy_comp2_wait_complete : rid [%u]", rid);

  p_ctx->rid = rid;
  _ctx_signal (pp_ctx);
}

/* Callback function to signal the client when a resource needs to be
   freed */
void
check_tizrmproxy_comp2_preemption_req (OMX_U32 rid, OMX_PTR ap_data)
{
  check_common_context_t *p_ctx = NULL;
  cc_ctx_t *pp_ctx = NULL;
  assert (ap_data);
  pp_ctx = (cc_ctx_t *) ap_data;
  p_ctx = *pp_ctx;

  TIZ_LOG (TIZ_PRIORITY_TRACE,
             "check_tizrmproxy_comp2_preemption_req : rid [%u]", rid);

  p_ctx->rid = rid;
  _ctx_signal (pp_ctx);
}

void
check_tizrmproxy_comp2_preemption_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  check_common_context_t *p_ctx = NULL;
  cc_ctx_t *pp_ctx = NULL;
  assert (ap_data);
  pp_ctx = (cc_ctx_t *) ap_data;
  p_ctx = *pp_ctx;

  TIZ_LOG (TIZ_PRIORITY_TRACE,
             "check_tizrmproxy_comp2_preemption_complete : rid [%u]", rid);

  p_ctx->rid = rid;
  _ctx_signal (pp_ctx);
}

pid_t
check_tizrmproxy_find_proc (const char *name)
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

START_TEST (test_proxy_acquire_and_release)
{
  tiz_rm_error_t error = TIZ_RM_SUCCESS;
  int rc, daemon_existed = 1;
  tiz_rm_t p_rm;;
  pid_t pid;
  OMX_UUIDTYPE uuid_omx;
  OMX_PRIORITYMGMTTYPE primgmt;
  tiz_rm_proxy_callbacks_t cbacks;

  /* Init RM database */
  fail_if (!refresh_rm_db ());
  rc = system ("./updatedb.sh db_acquire_and_release.sql3");

  /* Dump its initial contents */
  fail_if (!dump_rmdb ("test_proxy_acquire_and_release.before.dump"));

  /* Check if an RM daemon is running already */
  if ((pid = check_tizrmproxy_find_proc ("tizrmd"))
      || (pid = check_tizrmproxy_find_proc ("lt-tizrmd")))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "RM Process [PID %d] FOUND", pid);
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

      cbacks.pf_waitend = &check_tizrmproxy_comp1_wait_complete;
      cbacks.pf_preempt = &check_tizrmproxy_comp1_preemption_req;
      cbacks.pf_preempt_end = &check_tizrmproxy_comp1_preemption_complete;

      TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_init");
      error =
        tiz_rm_proxy_init (&p_rm, COMPONENT1_NAME,
                          (const OMX_UUIDTYPE *) &uuid_omx, &primgmt, &cbacks,
                          NULL);
      fail_if (error != TIZ_RM_SUCCESS);

      /* error = tiz_rm_proxy_version(&p_rm); */
      error = tiz_rm_proxy_acquire (&p_rm, TIZ_RM_RESOURCE_DUMMY, 1);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_acquire returned [%d]", error);
      fail_if (error != TIZ_RM_SUCCESS);

      error = tiz_rm_proxy_release (&p_rm, TIZ_RM_RESOURCE_DUMMY, 1);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_release returned [%d]", error);
      fail_if (error != TIZ_RM_SUCCESS);

      error = tiz_rm_proxy_destroy (&p_rm);
      fail_if (error != TIZ_RM_SUCCESS);

      if (!daemon_existed)
        {
          error = kill (pid, SIGTERM);
          fail_if (error == -1);
        }

      /* Check db */
      fail_if (!dump_rmdb ("test_proxy_acquire_and_release.after.dump"));

      rc =
        system
        ("cmp -s /tmp/test_proxy_acquire_and_release.before.dump /tmp/test_proxy_acquire_and_release.after.dump");

      TIZ_LOG (TIZ_PRIORITY_TRACE, "DB comparison check [%s]",
                 (rc == 0 ? "SUCCESS" : "FAILED"));
      fail_if (rc != 0);

    }
  else
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Starting the RM Daemon");
      const char *arg0 = "";
      error = execlp (pg_rmd_path, arg0, (char *) NULL);
      fail_if (error == -1);
    }
}

END_TEST
START_TEST (test_proxy_acquire_and_destroy_no_release)
{
  tiz_rm_error_t error = TIZ_RM_SUCCESS;
  int rc, daemon_existed = 1;
  tiz_rm_t p_rm;;
  pid_t pid;
  OMX_UUIDTYPE uuid_omx;
  OMX_PRIORITYMGMTTYPE primgmt;
  tiz_rm_proxy_callbacks_t cbacks;

  /* Init the RM database */
  /*   fail_if (!refresh_rm_db ()); */
  rc = system ("./updatedb.sh db_acquire_and_release.sql3");

  /* Dump its initial contents */
  fail_if (!dump_rmdb ("test_proxy_acquire_and_destroy_no_release.before.dump"));

  /* Check if an RM daemon is running already */
  if ((pid = check_tizrmproxy_find_proc ("tizrmd"))
      || (pid = check_tizrmproxy_find_proc ("lt-tizrmd")))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "RM Process [PID %d] FOUND", pid);
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

      cbacks.pf_waitend = &check_tizrmproxy_comp1_wait_complete;
      cbacks.pf_preempt = &check_tizrmproxy_comp1_preemption_req;
      cbacks.pf_preempt_end = &check_tizrmproxy_comp1_preemption_complete;

      TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_init");
      error =
        tiz_rm_proxy_init (&p_rm, COMPONENT1_NAME,
                          (const OMX_UUIDTYPE *) &uuid_omx, &primgmt, &cbacks,
                          NULL);
      fail_if (error != TIZ_RM_SUCCESS);

      /* error = tiz_rm_proxy_version(&p_rm); */
      error = tiz_rm_proxy_acquire (&p_rm, TIZ_RM_RESOURCE_DUMMY, 1);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_acquire returned [%d]", error);
      fail_if (error != TIZ_RM_SUCCESS);

      error = tiz_rm_proxy_destroy (&p_rm);
      fail_if (error != TIZ_RM_SUCCESS);

      if (!daemon_existed)
        {
          error = kill (pid, SIGTERM);
          fail_if (error == -1);
        }

      /* Check db */
      fail_if (!dump_rmdb ("test_proxy_acquire_and_destroy_no_release.after.dump"));

      rc =
        system
        ("cmp -s /tmp/test_proxy_acquire_and_destroy_no_release.before.dump /tmp/test_proxy_acquire_and_destroy_no_release.after.dump");

      TIZ_LOG (TIZ_PRIORITY_TRACE, "DB comparison check [%s]",
                 (rc == 0 ? "SUCCESS" : "FAILED"));
      fail_if (rc != 0);

    }
  else
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Starting the RM Daemon");
      const char *arg0 = "";
      error = execlp (pg_rmd_path, arg0, (char *) NULL);
      fail_if (error == -1);
    }
}

END_TEST
START_TEST (test_proxy_wait_cancel_wait)
{
  tiz_rm_error_t error = TIZ_RM_SUCCESS;
  int rc, daemon_existed = 1;
  tiz_rm_t p_rm;;
  pid_t pid;
  OMX_UUIDTYPE uuid_omx;
  OMX_PRIORITYMGMTTYPE primgmt;
  tiz_rm_proxy_callbacks_t cbacks;

  /* Check if an RM daemon is running already */
  if ((pid = check_tizrmproxy_find_proc ("tizrmd"))
      || (pid = check_tizrmproxy_find_proc ("lt-tizrmd")))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "RM Process [PID %d] FOUND -- > SKIPPING THIS TEST", pid);
    }
  else
    {
      /* Init the RM database */
      rc = system ("./updatedb.sh db_wait_cancel_wait.before.sql3");

      /* Dump its initial contents */
      fail_if (!dump_rmdb ("test_proxy_wait_cancel_wait.before.dump"));

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

          cbacks.pf_waitend = &check_tizrmproxy_comp1_wait_complete;
          cbacks.pf_preempt = &check_tizrmproxy_comp1_preemption_req;
          cbacks.pf_preempt_end = &check_tizrmproxy_comp1_preemption_complete;

          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_init");
          error =
            tiz_rm_proxy_init (&p_rm, COMPONENT1_NAME,
                              (const OMX_UUIDTYPE *) &uuid_omx, &primgmt, &cbacks,
                              NULL);
          fail_if (error != TIZ_RM_SUCCESS);

          /* error = tiz_rm_proxy_version(&p_rm); */
          error = tiz_rm_proxy_acquire (&p_rm, TIZ_RM_RESOURCE_DUMMY, 1);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_acquire returned [%d]", error);
          fail_if (error != TIZ_RM_NOT_ENOUGH_RESOURCE_AVAILABLE);

          /* Wait for the resource */
          error = tiz_rm_proxy_wait (&p_rm, TIZ_RM_RESOURCE_DUMMY, 1);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_wait returned [%d]", error);
          fail_if (error != TIZ_RM_SUCCESS);

          /* Now cancel the wait */
          error = tiz_rm_proxy_cancel_wait (&p_rm, TIZ_RM_RESOURCE_DUMMY, 1);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_cancel_wait returned [%d]",
                     error);
          fail_if (error != TIZ_RM_SUCCESS);

          error = tiz_rm_proxy_destroy (&p_rm);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_destroy returned [%d]", error);
          fail_if (error != TIZ_RM_SUCCESS);

          if (!daemon_existed)
            {
              error = kill (pid, SIGTERM);
              fail_if (error == -1);
            }

          /* Check db */
          fail_if (!dump_rmdb ("test_proxy_wait_cancel_wait.after.dump"));

          rc =
            system
            ("cmp -s /tmp/test_proxy_wait_cancel_wait.before.dump /tmp/test_proxy_wait_cancel_wait.after.dump");

          TIZ_LOG (TIZ_PRIORITY_TRACE, "DB comparison check [%s]",
                     (rc == 0 ? "SUCCESS" : "FAILED"));
          fail_if (rc != 0);

          /* Restore the db */
          rc = system ("./updatedb.sh db_wait_cancel_wait.after.sql3");

        }
      else
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "Starting the RM Daemon");
          const char *arg0 = "";
          error = execlp (pg_rmd_path, arg0, (char *) NULL);
          fail_if (error == -1);
        }
    }
}

END_TEST
START_TEST (test_proxy_busy_resource_management)
{
  tiz_rm_error_t error = TIZ_RM_SUCCESS;
  OMX_ERRORTYPE omx_error = OMX_ErrorNone;
  int rc, daemon_existed = 1;
  tiz_rm_t p_rm1, p_rm2;
  pid_t pid;
  OMX_UUIDTYPE uuid_omx1, uuid_omx2;
  OMX_PRIORITYMGMTTYPE primgmt;
  tiz_rm_proxy_callbacks_t cbacks1, cbacks2;
  cc_ctx_t ctx;
  check_common_context_t *p_ctx = NULL;
  OMX_BOOL timedout = OMX_FALSE;

  /* Init the RM database */
  rc = system ("./updatedb.sh db_busy_resource_management.before.sql3");

  /* Dump its initial contents */
  fail_if (!dump_rmdb ("test_proxy_busy_resource_management.before.dump"));

  /* Check if an RM daemon is running already */
  if ((pid = check_tizrmproxy_find_proc ("tizrmd"))
      || (pid = check_tizrmproxy_find_proc ("lt-tizrmd")))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "RM Process [PID %d] FOUND -- > SKIPPING THIS TEST", pid);
    }
  else
    {
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

          omx_error = _ctx_init (&ctx);
          fail_if (OMX_ErrorNone != omx_error);
          p_ctx = (check_common_context_t *) (ctx);

          /* Generate the uuids */
          tiz_uuid_generate (&uuid_omx1);
          tiz_uuid_generate (&uuid_omx2);

          /* Init the rm hdls */
          primgmt.nSize = sizeof (OMX_PRIORITYMGMTTYPE);
          primgmt.nVersion.nVersion = OMX_VERSION;
          primgmt.nGroupPriority = COMPONENT1_PRIORITY;
          primgmt.nGroupID = COMPONENT1_GROUP_ID;

          cbacks1.pf_waitend = &check_tizrmproxy_comp1_wait_complete;
          cbacks1.pf_preempt = &check_tizrmproxy_comp1_preemption_req;
          cbacks1.pf_preempt_end = &check_tizrmproxy_comp1_preemption_complete;

          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_init : [%s]", COMPONENT1_NAME);
          error =
            tiz_rm_proxy_init (&p_rm1, COMPONENT1_NAME,
                              (const OMX_UUIDTYPE *) &uuid_omx1, &primgmt,
                              &cbacks1, (OMX_PTR *) (&ctx));
          fail_if (error != TIZ_RM_SUCCESS);

          cbacks2.pf_waitend = &check_tizrmproxy_comp2_wait_complete;
          cbacks2.pf_preempt = &check_tizrmproxy_comp2_preemption_req;
          cbacks2.pf_preempt_end = &check_tizrmproxy_comp2_preemption_complete;

          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_init : [%s]", COMPONENT2_NAME);
          error =
            tiz_rm_proxy_init (&p_rm2, COMPONENT2_NAME,
                              (const OMX_UUIDTYPE *) &uuid_omx2, &primgmt,
                              &cbacks2, (OMX_PTR *) (&ctx));
          fail_if (error != TIZ_RM_SUCCESS);

          /* Component1 acquires all the available units (1) of resource TIZ_RM_RESOURCE_DUMMY */
          error = tiz_rm_proxy_acquire (&p_rm1, TIZ_RM_RESOURCE_DUMMY, 1);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_acquire returned (rm1) [%d]",
                     error);
          fail_if (error != TIZ_RM_SUCCESS);

          /* Component2 cannot acquire the resource, because there are not enough
           * units available */
          error = tiz_rm_proxy_acquire (&p_rm2, TIZ_RM_RESOURCE_DUMMY, 1);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_acquire returned (rm2) [%d]",
                     error);
          fail_if (error != TIZ_RM_NOT_ENOUGH_RESOURCE_AVAILABLE);

          /* Component2 signals its intent to wait for the resource */
          error = tiz_rm_proxy_wait (&p_rm2, TIZ_RM_RESOURCE_DUMMY, 1);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_wait returned (rm2) [%d]",
                     error);
          fail_if (error != TIZ_RM_SUCCESS);

          /* Now Component1 releases the resource */
          error = tiz_rm_proxy_release (&p_rm1, TIZ_RM_RESOURCE_DUMMY, 1);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_release returned (rm1) [%d]",
                     error);
          fail_if (error != TIZ_RM_SUCCESS);

          /* and now Component2 receives the resource  */
          /* Check wait complete */
          omx_error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
          fail_if (OMX_ErrorNone != omx_error);
          fail_if (OMX_TRUE == timedout);
          fail_if (TIZ_RM_RESOURCE_DUMMY != p_ctx->rid);

          TIZ_LOG (TIZ_PRIORITY_TRACE, "Wait complete (rm2)");

          /* Component2 releases the resource  */
          error = tiz_rm_proxy_release (&p_rm2, TIZ_RM_RESOURCE_DUMMY, 1);
          fail_if (error != TIZ_RM_SUCCESS);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_release returned (rm2) [%d]",
                     error);

          /* Destroy the rm hdls */
          TIZ_LOG (TIZ_PRIORITY_TRACE, "Destroying rm hdls");
          error = tiz_rm_proxy_destroy (&p_rm1);
          fail_if (error != TIZ_RM_SUCCESS);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_destroy returned (rm1) [%d]",
                     error);

          error = tiz_rm_proxy_destroy (&p_rm2);
          fail_if (error != TIZ_RM_SUCCESS);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_destroy returned (rm2) [%d]",
                     error);

          _ctx_reset(&ctx);
          _ctx_destroy(&ctx);

          if (!daemon_existed)
            {
              error = kill (pid, SIGTERM);
              fail_if (error == -1);
            }

          /* Check db */
          fail_if (!dump_rmdb ("test_proxy_busy_resource_management.after.dump"));

          rc =
            system
            ("cmp -s /tmp/test_proxy_busy_resource_management.before.dump /tmp/test_proxy_busy_resource_management.after.dump");

          TIZ_LOG (TIZ_PRIORITY_TRACE, "DB comparison check [%s]",
                     (rc == 0 ? "SUCCESS" : "FAILED"));
          fail_if (rc != 0);

          /* Restore the RM database */
          rc = system ("./updatedb.sh db_busy_resource_management.before.sql3");

        }
      else
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "Starting the RM Daemon");
          const char *arg0 = "";
          error = execlp (pg_rmd_path, arg0, (char *) NULL);
          fail_if (error == -1);
        }
    }
}

END_TEST
START_TEST (test_proxy_resource_preemption)
{
  tiz_rm_error_t error = TIZ_RM_SUCCESS;
  OMX_ERRORTYPE omx_error = OMX_ErrorNone;
  int rc, daemon_existed = 1;
  tiz_rm_t p_rm1, p_rm2;
  pid_t pid;
  OMX_UUIDTYPE uuid_omx1, uuid_omx2;
  OMX_PRIORITYMGMTTYPE primgmt;
  tiz_rm_proxy_callbacks_t cbacks1, cbacks2;
  cc_ctx_t ctx1, ctx2;
  check_common_context_t *p_ctx1 = NULL, *p_ctx2 = NULL;
  OMX_BOOL timedout1 = OMX_FALSE, timedout2 = OMX_FALSE;;

  /* Init the RM database */
  rc = system ("./updatedb.sh db_resource_preemption.before.sql3");

  /* Dump its initial contents */
  fail_if (!dump_rmdb ("test_proxy_resource_preemption.before.dump"));

  /* Check if an RM daemon is running already */
  if ((pid = check_tizrmproxy_find_proc ("tizrmd"))
      || (pid = check_tizrmproxy_find_proc ("lt-tizrmd")))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "RM Process [PID %d] FOUND -- > SKIPPING THIS TEST", pid);
    }
  else
    {
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

          omx_error = _ctx_init (&ctx1);
          fail_if (OMX_ErrorNone != omx_error);
          p_ctx1 = (check_common_context_t *) (ctx1);
          p_ctx1->pp_rm = &p_rm1;

          omx_error = _ctx_init (&ctx2);
          fail_if (OMX_ErrorNone != omx_error);
          p_ctx2 = (check_common_context_t *) (ctx2);
          p_ctx2->pp_rm = &p_rm2;

          /* Generate the uuids */
          tiz_uuid_generate (&uuid_omx1);
          tiz_uuid_generate (&uuid_omx2);

          /* Init the rm hdls */
          primgmt.nSize = sizeof (OMX_PRIORITYMGMTTYPE);
          primgmt.nVersion.nVersion = OMX_VERSION;
          primgmt.nGroupPriority = COMPONENT1_PRIORITY;
          primgmt.nGroupID = COMPONENT1_GROUP_ID;

          cbacks1.pf_waitend = &check_tizrmproxy_comp1_wait_complete;
          cbacks1.pf_preempt = &check_tizrmproxy_comp1_preemption_req;
          cbacks1.pf_preempt_end = &check_tizrmproxy_comp1_preemption_complete;

          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_init : [%s]", COMPONENT1_NAME);
          error =
            tiz_rm_proxy_init (&p_rm1, COMPONENT1_NAME,
                              (const OMX_UUIDTYPE *) &uuid_omx1, &primgmt,
                              &cbacks1, (OMX_PTR *) (&ctx1));
          fail_if (error != TIZ_RM_SUCCESS);

          primgmt.nGroupPriority = COMPONENT2_PRIORITY;
          primgmt.nGroupID = COMPONENT2_GROUP_ID;

          cbacks2.pf_waitend = &check_tizrmproxy_comp2_wait_complete;
          cbacks2.pf_preempt = &check_tizrmproxy_comp2_preemption_req;
          cbacks2.pf_preempt_end = &check_tizrmproxy_comp2_preemption_complete;

          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_init : [%s]", COMPONENT2_NAME);
          error =
            tiz_rm_proxy_init (&p_rm2, COMPONENT2_NAME,
                              (const OMX_UUIDTYPE *) &uuid_omx2, &primgmt,
                              &cbacks2, (OMX_PTR *) (&ctx2));
          fail_if (error != TIZ_RM_SUCCESS);

          /* Component1 acquires all the available units (1) of resource TIZ_RM_RESOURCE_DUMMY */
          error = tiz_rm_proxy_acquire (&p_rm1, TIZ_RM_RESOURCE_DUMMY, 1);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_acquire returned (rm1) [%d]",
                     error);
          fail_if (error != TIZ_RM_SUCCESS);

          /* Component2 requests the same resource. It belongs to a higher priority
           * group and causes the preemption of the resource from Component1 */
          error = tiz_rm_proxy_acquire (&p_rm2, TIZ_RM_RESOURCE_DUMMY, 1);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_acquire returned (rm2) [%d]",
                     error);
          fail_if (error != TIZ_RM_PREEMPTION_IN_PROGRESS);

          TIZ_LOG (TIZ_PRIORITY_TRACE, "Preemption is in progress (rm1) [%d]",
                     error);

          /* Verify preemption req for Component1's resource */
          omx_error = _ctx_wait (&ctx1, TIMEOUT_EXPECTING_SUCCESS, &timedout1);
          fail_if (OMX_ErrorNone != omx_error);
          fail_if (OMX_TRUE == timedout1);
          fail_if (TIZ_RM_RESOURCE_DUMMY != p_ctx1->rid);

          TIZ_LOG (TIZ_PRIORITY_TRACE, "Preemption request verified (rm1) [%d]",
                     error);

          /* Now Component1 releases the resource */
          error = tiz_rm_proxy_preemption_conf (&p_rm1, TIZ_RM_RESOURCE_DUMMY, 1);
          TIZ_LOG (TIZ_PRIORITY_TRACE,
                     "tiz_rm_proxy_preemption_conf returned (rm1) [%d]", error);
          fail_if (error != TIZ_RM_SUCCESS);

          /* Verify preemption completion and Component2's resource ownership */
          omx_error = _ctx_wait (&ctx2, TIMEOUT_EXPECTING_SUCCESS, &timedout2);
          fail_if (OMX_ErrorNone != omx_error);
          fail_if (OMX_TRUE == timedout2);
          fail_if (TIZ_RM_RESOURCE_DUMMY != p_ctx2->rid);

          TIZ_LOG (TIZ_PRIORITY_TRACE, "Preemption completion verified (rm2) [%d]",
                     error);

          /* Now Component2 releases the resource */
          error = tiz_rm_proxy_release (&p_rm2, TIZ_RM_RESOURCE_DUMMY, 1);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_release returned (rm2) [%d]",
                     error);
          fail_if (error != TIZ_RM_SUCCESS);

          /* Destroy the rm hdls */
          TIZ_LOG (TIZ_PRIORITY_TRACE, "Destroying rm hdls");
          error = tiz_rm_proxy_destroy (&p_rm1);
          fail_if (error != TIZ_RM_SUCCESS);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_destroy returned (rm1) [%d]",
                     error);

          error = tiz_rm_proxy_destroy (&p_rm2);
          fail_if (error != TIZ_RM_SUCCESS);
          TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_rm_proxy_destroy returned (rm2) [%d]",
                     error);

          _ctx_reset(&ctx1);
          _ctx_destroy(&ctx1);

          _ctx_reset(&ctx2);
          _ctx_destroy(&ctx2);

          if (!daemon_existed)
            {
              error = kill (pid, SIGTERM);
              fail_if (error == -1);
            }

          /* Check db */
          fail_if (!dump_rmdb ("test_proxy_resource_preemption.after.dump"));

          rc =
            system
            ("cmp -s /tmp/test_proxy_resource_preemption.before.dump /tmp/test_proxy_resource_preemption.after.dump");

          TIZ_LOG (TIZ_PRIORITY_TRACE, "DB comparison check [%s]",
                     (rc == 0 ? "SUCCESS" : "FAILED"));
          fail_if (rc != 0);

          /* Restore the RM database */
          rc = system ("./updatedb.sh db_resource_preemption.after.sql3");

        }
      else
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "Starting the RM Daemon");
          const char *arg0 = "";
          error = execlp (pg_rmd_path, arg0, (char *) NULL);
          fail_if (error == -1);
        }

    }
}
END_TEST

Suite *
rmproxy_suite (void)
{
  TCase *tc_proxy;
  Suite *s = suite_create ("libtizrmproxy");

  putenv(TIZ_PLATFORM_RC_FILE_ENV);

  /* test case */
  tc_proxy = tcase_create ("RM proxy");
  tcase_add_unchecked_fixture (tc_proxy, setup, teardown);
  tcase_set_timeout (tc_proxy, RMPROXY_TEST_TIMEOUT);
  tcase_add_test (tc_proxy, test_proxy_acquire_and_release);
  tcase_add_test (tc_proxy, test_proxy_acquire_and_destroy_no_release);
  tcase_add_test (tc_proxy, test_proxy_wait_cancel_wait);
  tcase_add_test (tc_proxy, test_proxy_busy_resource_management);
  tcase_add_test (tc_proxy, test_proxy_resource_preemption);
  suite_add_tcase (s, tc_proxy);

  return s;
}

int
main (void)
{
  int number_failed;
  SRunner *sr = srunner_create (rmproxy_suite ());

  tiz_log_init();

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Tizonia OpenMAX IL - RM client unit tests");

  /* Enable extra debugging info for D-BUSXX */
  /* setenv ("DBUSXX_VERBOSE", "1", 0); */

  srunner_run_all (sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  tiz_log_deinit ();

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
