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
 * @file   check_tizcore.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia IL Core Unit Tests
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <check.h>
#include <sys/types.h>
#include <signal.h>
#include <limits.h>

#include "tizosal.h"


#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.ilcore.check"
#endif

#define TIZ_CORE_TEST_COMPONENT_NAME "OMX.Aratelia.ilcore.test_component"
#define TIZ_CORE_TEST_COMPONENT_ROLE "default"
#define AUDIO_RENDERER "OMX.Aratelia.audio_renderer.pcm"
#define FILE_READER "OMX.Aratelia.file_reader.binary"

char *pg_rmd_path;
pid_t g_rmd_pid;

static bool
refresh_rm_db (void)
{
  bool rv = false;
  const char *p_rmdb_path = NULL;
  const char *p_sqlite_path = NULL;
  const char *p_init_path = NULL;
  const char *p_rmd_path = NULL;

  p_rmdb_path = tiz_rcfile_get_value("resource-management", "rmdb");
  p_sqlite_path = tiz_rcfile_get_value("resource-management",
                                       "rmdb.sqlite_script");
  p_init_path = tiz_rcfile_get_value("resource-management",
                                     "rmdb.init_script");

  p_rmd_path = tiz_rcfile_get_value("resource-management", "rmd.path");

  if (!p_rmdb_path || !p_sqlite_path || !p_init_path || !p_rmd_path)

    {
      TIZ_LOG(TIZ_PRIORITY_TRACE, "Test data not available...");
    }
  else
    {
      pg_rmd_path = strndup (p_rmd_path, PATH_MAX);

      TIZ_LOG(TIZ_PRIORITY_TRACE, "RM daemon [%s] ...", pg_rmd_path);

      /* Re-fresh the rm db */
      size_t total_len = strlen (p_init_path)
        + strlen (p_sqlite_path)
        + strlen (p_rmdb_path) + 4;
      char *p_cmd = tiz_mem_calloc (1, total_len);
      if (p_cmd)
        {
          snprintf(p_cmd, total_len -1, "%s %s %s",
                  p_init_path, p_sqlite_path, p_rmdb_path);
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
    }

  return rv;
}

static void
setup (void)
{
  int error = 0;

  fail_if (!refresh_rm_db());

  /* Start the rm daemon */
  g_rmd_pid = fork ();
  fail_if (g_rmd_pid == -1);

  if (g_rmd_pid)
    {
      sleep (1);
    }
  else
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Starting the RM Daemon");
      const char *arg0 = "";
      error = execlp (pg_rmd_path, arg0, (char *) NULL);
      fail_if (error == -1);
    }

}

static void
teardown (void)
{
  int error = 0;

  if (g_rmd_pid)
    {
      error = kill (g_rmd_pid, SIGTERM);
      fail_if (error == -1);
    }
  tiz_mem_free (pg_rmd_path);
}

START_TEST (test_ilcore_init_and_deinit)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;

  error = OMX_Init ();
  fail_if (error != OMX_ErrorNone);

  error = OMX_Deinit ();
  fail_if (error != OMX_ErrorNone);
}

END_TEST
START_TEST (test_ilcore_init_and_deinit_get_hdl_free_hdl)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = NULL;
  OMX_U32 appData;
  OMX_CALLBACKTYPE callBacks;

  error = OMX_Init ();
  fail_if (error != OMX_ErrorNone);

  error = OMX_GetHandle (&p_hdl,
                         TIZ_CORE_TEST_COMPONENT_NAME,
                         (OMX_PTR *) (&appData), &callBacks);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_GetHandle error [%s]", tiz_err_to_str (error));
  fail_if (error != OMX_ErrorNone);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%p]", p_hdl);

  error = OMX_FreeHandle (p_hdl);
  fail_if (error != OMX_ErrorNone);

  error = OMX_Deinit ();
  fail_if (error != OMX_ErrorNone);
}

END_TEST
START_TEST (test_ilcore_setup_tunnel_tear_down_tunnel)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl_out = NULL;
  OMX_HANDLETYPE p_hdl_in = NULL;
  OMX_U32 appData;
  OMX_CALLBACKTYPE callBacks;

  error = OMX_Init ();
  fail_if (error != OMX_ErrorNone);

  error = OMX_GetHandle (&p_hdl_in,
                         AUDIO_RENDERER, (OMX_PTR *) (&appData), &callBacks);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_GetHandle [%s] error [%s]",
           AUDIO_RENDERER, tiz_err_to_str (error));
  fail_if (error != OMX_ErrorNone);

  error = OMX_GetHandle (&p_hdl_out,
                         FILE_READER, (OMX_PTR *) (&appData), &callBacks);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_GetHandle [%s] error [%s]",
           FILE_READER, tiz_err_to_str (error));
  fail_if (error != OMX_ErrorNone);

  /* ------------------------------------- */
  /* Create Tunnel Reader:0 <-> Renderer:0 */
  /* ------------------------------------- */
  error = OMX_SetupTunnel(p_hdl_out, 0, p_hdl_in, 0);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_SetupTunnel [%s]", tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* ---------------------------------------- */
  /* Tear down Tunnel Reader:0 <-> Renderer:0 */
  /* ---------------------------------------- */
  error = OMX_TeardownTunnel(p_hdl_out, 0, p_hdl_in, 0);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_TeardownTunnel [%s]", tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  error = OMX_FreeHandle (p_hdl_out);
  fail_if (error != OMX_ErrorNone);

  error = OMX_FreeHandle (p_hdl_in);
  fail_if (error != OMX_ErrorNone);

  error = OMX_Deinit ();
  fail_if (error != OMX_ErrorNone);
}

END_TEST
START_TEST (test_ilcore_comp_of_role_enum)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_U32 index = 0;
  OMX_S8 comp_name[OMX_MAX_STRINGNAME_SIZE];

  error = OMX_Init ();
  fail_if (error != OMX_ErrorNone);

  do
    {
      error = OMX_ComponentOfRoleEnum ((OMX_STRING) comp_name,
                                       TIZ_CORE_TEST_COMPONENT_ROLE, index++);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] : component [%s] error [%s]",
               TIZ_CORE_TEST_COMPONENT_ROLE, comp_name, tiz_err_to_str (error));
    } while (OMX_ErrorNone == error);

  fail_if (OMX_ErrorNoMore != error);
  fail_if (index != 2);

  error = OMX_Deinit ();
  fail_if (error != OMX_ErrorNone);
}

END_TEST
START_TEST (test_ilcore_role_of_comp_enum)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_U32 index = 0;
  OMX_S8 role[OMX_MAX_STRINGNAME_SIZE];

  error = OMX_Init ();
  fail_if (error != OMX_ErrorNone);

  do
    {
      error = OMX_RoleOfComponentEnum ((OMX_STRING) role,
                                       TIZ_CORE_TEST_COMPONENT_NAME, index++);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] : Role [%s] error [%s]",
               TIZ_CORE_TEST_COMPONENT_NAME, role, tiz_err_to_str (error));
    } while (OMX_ErrorNone == error);

  fail_if (OMX_ErrorNoMore != error);

  error = OMX_Deinit ();
  fail_if (error != OMX_ErrorNone);
}

END_TEST Suite * tizcore_suite (void)
{
  TCase *tc_ilcore;
  Suite *s = suite_create ("libtizcore");

  /* IL Core API test case */
  tc_ilcore = tcase_create ("ilcore");
  tcase_add_unchecked_fixture (tc_ilcore, setup, teardown);
  tcase_add_test (tc_ilcore, test_ilcore_init_and_deinit);
  tcase_add_test (tc_ilcore,
                  test_ilcore_init_and_deinit_get_hdl_free_hdl);
  tcase_add_test (tc_ilcore, test_ilcore_setup_tunnel_tear_down_tunnel);
  tcase_add_test (tc_ilcore, test_ilcore_comp_of_role_enum);
  tcase_add_test (tc_ilcore, test_ilcore_role_of_comp_enum);

  /* TODO: Negative case for OMX_ErrorPortsNotConnected error */

  suite_add_tcase (s, tc_ilcore);

  return s;
}

int
main (void)
{
  int number_failed;
  SRunner *sr = srunner_create (tizcore_suite ());

  tiz_log_init();

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Tizonia OpenMAX IL - IL core unit tests");

  /* srunner_set_fork_status (sr, CK_NOFORK); */

  srunner_run_all (sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  tiz_log_deinit ();

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
