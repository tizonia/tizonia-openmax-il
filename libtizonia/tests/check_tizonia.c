/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   check_tizonia.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia IL Common Unit Tests
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
#include <sys/time.h>
#include <check.h>
#include <sys/types.h>
#include <signal.h>
#include <assert.h>
#include <limits.h>

#include <OMX_Component.h>
#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include "tizscheduler.h"
#include "tizfsm.h"
#include "tizkernel.h"

#include "check_tizonia.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.check"
#endif

char *pg_rmd_path;
pid_t g_rmd_pid;

#define COMPONENT_NAME "OMX.Aratelia.tizonia.test_component"
#define COMPONENT_ROLE1 "tizonia_test_component.role1"
#define COMPONENT_ROLE2 "tizonia_test_component.role2"
#define COMPONENT_DEFAULT_ROLE "default"

#define INFINITE_WAIT 0xffffffff
/* duration of event timeout in msec when we expect event to be set */
#define TIMEOUT_EXPECTING_SUCCESS 1000
/* duration of event timeout in msec when we don't expect event to be set */
#define TIMEOUT_EXPECTING_FAILURE 2000

typedef void *cc_ctx_t;
typedef struct check_common_context check_common_context_t;
struct check_common_context
{
  OMX_BOOL signaled;
  tiz_mutex_t mutex;
  tiz_cond_t cond;
  volatile OMX_STATETYPE state;
  OMX_ERRORTYPE error;
  OMX_U32 port;
  OMX_BUFFERHEADERTYPE *p_hdr;
};

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

  p_ctx->state = OMX_StateMax;
  p_ctx->error = OMX_ErrorMax;
  p_ctx->port = OMX_ALL;
  p_ctx->p_hdr = NULL;

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
  p_ctx->state = OMX_StateMax;
  p_ctx->error = OMX_ErrorMax;
  p_ctx->port = OMX_ALL;
  p_ctx->p_hdr = NULL;

  tiz_mutex_unlock (&p_ctx->mutex);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
check_EventHandler (OMX_HANDLETYPE ap_hdl,
                    OMX_PTR ap_app_data,
                    OMX_EVENTTYPE eEvent,
                    OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
  check_common_context_t *p_ctx = NULL;
  cc_ctx_t *pp_ctx = NULL;
  assert (ap_app_data);
  pp_ctx = (cc_ctx_t *) ap_app_data;
  p_ctx = *pp_ctx;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Component Event [%s]", tiz_evt_to_str (eEvent));

  if (OMX_EventCmdComplete == eEvent)
    {
      switch ((OMX_COMMANDTYPE) (nData1))
        {
        case OMX_CommandStateSet:
          {
            p_ctx->state = (OMX_STATETYPE) (nData2);
            p_ctx->error = (OMX_ERRORTYPE) (pEventData);
            TIZ_LOG (TIZ_PRIORITY_TRACE, "Component transitioned to [%s] - pEventData [%s]",
                     tiz_fsm_state_to_str ((tiz_fsm_state_id_t) (nData2)),
                     tiz_err_to_str (p_ctx->error));
            _ctx_signal (pp_ctx);
            break;
          }

        case OMX_CommandPortDisable:
          {
            TIZ_LOG (TIZ_PRIORITY_TRACE, "Port  [%d] transitioned to DISABLED",
                     nData2);
            p_ctx->port = (OMX_STATETYPE) (nData2);
            p_ctx->error = (OMX_ERRORTYPE) (pEventData);
            _ctx_signal (pp_ctx);
          }
          break;

        case OMX_CommandPortEnable:
          {
            TIZ_LOG (TIZ_PRIORITY_TRACE, "Port  [%d] transitioned to ENABLED",
                     nData2);
            p_ctx->port = (OMX_STATETYPE) (nData2);
            p_ctx->error = (OMX_ERRORTYPE) (pEventData);
            _ctx_signal (pp_ctx);
          }
          break;

        default:
          {
            TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] received!!",
                     tiz_cmd_to_str (nData1));

            assert (0);
          }

        };
    }
  else if (OMX_EventError == eEvent)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Component reported error [%s] - Test FAILED !!!.",
               tiz_err_to_str ((OMX_ERRORTYPE) (nData1)));
      fail ();
    }
  else
    {
      fail ();
    }

  return OMX_ErrorNone;

}

OMX_ERRORTYPE check_EmptyBufferDone
  (OMX_HANDLETYPE ap_hdl,
   OMX_PTR ap_app_data, OMX_BUFFERHEADERTYPE * ap_buf)
{
  check_common_context_t *p_ctx = NULL;
  cc_ctx_t *pp_ctx = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "EmptyBufferDone: BUFFER [%p]", ap_buf);

  assert (ap_app_data);
  assert (ap_buf);
  pp_ctx = (cc_ctx_t *) ap_app_data;
  p_ctx = *pp_ctx;

  p_ctx->p_hdr = ap_buf;
  _ctx_signal (pp_ctx);

  return OMX_ErrorNone;

}

OMX_ERRORTYPE check_FillBufferDone
  (OMX_HANDLETYPE ap_hdl,
   OMX_PTR ap_app_data, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNone;
}


static OMX_CALLBACKTYPE _check_cbacks = {
  check_EventHandler,
  check_EmptyBufferDone,
  check_FillBufferDone
};

static OMX_ERRORTYPE
check_tizonia_GetParameter (OMX_HANDLETYPE ap_hdl,
                             OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  fail_if(OMX_IndexParamPortDefinition != a_index);

  if (OMX_IndexParamPortDefinition == a_index)
    {
      OMX_PARAM_PORTDEFINITIONTYPE *p_port_def = ap_struct;
      p_port_def->eDir = OMX_DirOutput; /* Pretend this is an output port */
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
check_tizonia_SetParameter(OMX_HANDLETYPE ap_hdl,
                            OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
check_tizonia_SetConfig(OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNone;
}

static void
init_fake_comp(OMX_COMPONENTTYPE *p_hdl)
{
  /* Init the component hdl */
  p_hdl->nVersion.s.nVersionMajor = 1;
  p_hdl->nVersion.s.nVersionMinor = 0;
  p_hdl->nVersion.s.nRevision = 0;
  p_hdl->nVersion.s.nStep = 0;
  p_hdl->pComponentPrivate = NULL;
  p_hdl->GetComponentVersion = NULL;
  p_hdl->SendCommand = NULL;
  p_hdl->GetParameter = check_tizonia_GetParameter;
  p_hdl->SetParameter = check_tizonia_SetParameter;
  p_hdl->GetConfig = NULL;
  p_hdl->SetConfig = check_tizonia_SetConfig;;
  p_hdl->GetExtensionIndex = NULL;
  p_hdl->GetState = NULL;
  p_hdl->ComponentTunnelRequest = NULL;
  p_hdl->UseBuffer = NULL;
  p_hdl->AllocateBuffer = NULL;
  p_hdl->FreeBuffer = NULL;
  p_hdl->EmptyThisBuffer = NULL;
  p_hdl->FillThisBuffer = NULL;
  p_hdl->SetCallbacks = NULL;
  p_hdl->ComponentDeInit = NULL;
  p_hdl->UseEGLImage = NULL;
  p_hdl->ComponentRoleEnum = NULL;

}

/*
 * Unit tests
 */

START_TEST (test_tizonia_getstate)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = 0;
  OMX_STATETYPE state;
  OMX_CALLBACKTYPE callBacks;
  OMX_U32 appData;

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  /* Instantiate the component */
  error = OMX_GetHandle (&p_hdl,
                         COMPONENT_NAME, (OMX_PTR *) (&appData), &callBacks);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%X]", p_hdl);

  error = OMX_GetState (p_hdl, &state);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  error = OMX_FreeHandle (p_hdl);
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  fail_if (OMX_ErrorNone != error);
}
END_TEST

START_TEST (test_tizonia_gethandle_freehandle)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = 0;
  OMX_U32 appData;
  OMX_CALLBACKTYPE callBacks;

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  error = OMX_GetHandle (&p_hdl,
                         COMPONENT_NAME, (OMX_PTR *) (&appData), &callBacks);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%p]", p_hdl);

  error = OMX_FreeHandle (p_hdl);
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  fail_if (OMX_ErrorNone != error);
}
END_TEST

START_TEST (test_tizonia_getparameter)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = 0;
  OMX_U32 appData;
  OMX_CALLBACKTYPE callBacks;
  OMX_INDEXTYPE index = OMX_IndexParamPortDefinition;
  OMX_PARAM_PORTDEFINITIONTYPE port_def;

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  error = OMX_GetHandle (&p_hdl,
                         COMPONENT_NAME, (OMX_PTR *) (&appData), &callBacks);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_getparameter: OMX_GetHandle error [%d]",
             error);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%p]", p_hdl);

  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;

  error = OMX_GetParameter (p_hdl, index, &port_def);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_getparameter: OMX_GetParameter error [%d]",
             error);
  fail_if (OMX_ErrorNone != error);

  error = OMX_FreeHandle (p_hdl);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_getparameter: OMX_FreeHandle error [%d]",
             error);
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_getparameter: OMX_Deinit error [%d]",
             error);
  fail_if (OMX_ErrorNone != error);
}
END_TEST

START_TEST (test_tizonia_roles)
{
  OMX_S8 role [OMX_MAX_STRINGNAME_SIZE];
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = 0;
  OMX_U32 appData;
  OMX_CALLBACKTYPE callBacks;
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  OMX_PARAM_COMPONENTROLETYPE role_type;
  OMX_U32 i = 0;
  OMX_U32 bufferCountActual = 0;

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  error = OMX_GetHandle (&p_hdl,
                         COMPONENT_NAME, (OMX_PTR *) (&appData), &callBacks);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_roles: OMX_GetHandle error [%d]",
             error);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%p]", p_hdl);

  for (i = 0; OMX_ErrorNone == error; ++i)
    {
      error = OMX_RoleOfComponentEnum ((OMX_STRING) role, COMPONENT_NAME, i);
      if (OMX_ErrorNone == error)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_roles: [%s] -> Role [%d] -> [%s]",
                   tiz_err_to_str (error), i, role);
        }
    }

  fail_if (OMX_ErrorNoMore != error);

  /* Check for 2 roles found (i must be equal 3) */
  fail_if (i != 3);

  role_type.nSize = sizeof (OMX_PARAM_COMPONENTROLETYPE);
  role_type.nVersion.nVersion = OMX_VERSION;
  strcpy ((OMX_STRING) role_type.cRole, COMPONENT_ROLE1);

  /* Set role #1 */
  error = OMX_SetParameter (p_hdl, OMX_IndexParamStandardComponentRole, &role_type);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_roles: OMX_SetParameter COMPONENT_ROLE1 [%s]",
           tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

  /* Get port def */
  TIZ_INIT_OMX_PORT_STRUCT (port_def, 0);

  error = OMX_GetParameter (p_hdl, OMX_IndexParamPortDefinition, &port_def);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_getparameter: OMX_GetParameter [%s]",
             tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

  bufferCountActual = port_def.nBufferCountActual;

  /* Modify port def */
  port_def.nBufferCountActual = 7;

  error = OMX_SetParameter (p_hdl, OMX_IndexParamPortDefinition, &port_def);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_getparameter: OMX_SetParameter [%s]",
             tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

  /* Set role #2 */
  strcpy ((OMX_STRING) role_type.cRole, COMPONENT_ROLE2);
  error = OMX_SetParameter (p_hdl, OMX_IndexParamStandardComponentRole, &role_type);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_roles: OMX_SetParameter COMPONENT_ROLE2 [%s]",
           tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

  /* Get port def */
  error = OMX_GetParameter (p_hdl, OMX_IndexParamPortDefinition, &port_def);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_roles: OMX_GetParameter [%s]",
             tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

  /* Fail if nBufferCountActual differs from original */
  fail_if (bufferCountActual != port_def.nBufferCountActual);

  /* Now set the "default" role */
  strcpy ((OMX_STRING) role_type.cRole, COMPONENT_DEFAULT_ROLE);
  error = OMX_SetParameter (p_hdl, OMX_IndexParamStandardComponentRole, &role_type);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_roles: OMX_SetParameter COMPONENT_DEFAULT_ROLE [%s]",
           tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

  error = OMX_FreeHandle (p_hdl);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_roles: OMX_FreeHandle [%s]",
           tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_roles: OMX_Deinit [%s]",
           tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);
}
END_TEST

START_TEST (test_tizonia_preannouncements_extension)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = 0;
  OMX_U32 appData;
  OMX_CALLBACKTYPE callBacks;
  OMX_INDEXTYPE ext_index = OMX_IndexComponentStartUnused;
  OMX_TIZONIA_PARAM_BUFFER_PREANNOUNCEMENTSMODETYPE pamode;

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  error = OMX_GetHandle (&p_hdl,
                         COMPONENT_NAME, (OMX_PTR *) (&appData), &callBacks);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_tizonia_preannouncements_extension: "
           "OMX_GetHandle [%s]", tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

  error = OMX_GetExtensionIndex (p_hdl, OMX_TIZONIA_INDEX_PARAM_BUFFER_PREANNOUNCEMENTSMODE,
                                 &ext_index);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_GetExtensionIndex error  [%s] index [%s]",
           tiz_err_to_str (error), tiz_idx_to_str (ext_index));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TizoniaIndexParamBufferPreAnnouncementsMode != ext_index);

  /* Get the preannouncementsmode struct and check that pre-announcements are
     enabled */
  pamode.nSize = sizeof (OMX_TIZONIA_PARAM_BUFFER_PREANNOUNCEMENTSMODETYPE);
  pamode.nVersion.nVersion = OMX_VERSION;
  pamode.nPortIndex = 0;
  error = OMX_GetParameter (p_hdl, ext_index, &pamode);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE != pamode.bEnabled);

  /* Set pre-announcements to FALSE */
  pamode.bEnabled = OMX_FALSE;
  error = OMX_SetParameter (p_hdl, ext_index, &pamode);

  fail_if (OMX_ErrorNone != error);

  /* Get the preannouncementsmode struct and check that pre-announcements are
     now disabled */
  error = OMX_GetParameter (p_hdl, ext_index, &pamode);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_FALSE != pamode.bEnabled);

  error = OMX_FreeHandle (p_hdl);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_FreeHandle [%s]",
           tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_Deinit [%s]",
           tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

}
END_TEST

START_TEST (test_tizonia_move_to_exe_and_transfer_with_allocbuffer)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = 0;
  OMX_COMMANDTYPE cmd = OMX_CommandStateSet;
  OMX_STATETYPE state = OMX_StateIdle;
  cc_ctx_t ctx;
  check_common_context_t *p_ctx = NULL;
  OMX_BOOL timedout = OMX_FALSE;
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  OMX_INDEXTYPE index = OMX_IndexParamPortDefinition;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_U32 i;

  error = _ctx_init (&ctx);
  fail_if (OMX_ErrorNone != error);

  p_ctx = (check_common_context_t *) (ctx);

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  /* Instantiate the component */
  error = OMX_GetHandle (&p_hdl, COMPONENT_NAME, (OMX_PTR *) (&ctx),
                         &_check_cbacks);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%X]", p_hdl);

  /* Obtain the port def params for port #0 */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  error = OMX_GetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferSize [%d]", port_def.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferCountActual [%d]",
             port_def.nBufferCountActual);

  /* Initiate transition to IDLE */
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* Allocate buffers */
  for (i = 0; i < port_def.nBufferCountActual; ++i)
    {
      error = OMX_AllocateBuffer (p_hdl, &p_hdr, 0,  /* input port */
                                  0, port_def.nBufferSize);
      fail_if (OMX_ErrorNone != error);
    }

  /* Await transition callback */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_ctx->state [%s]",
             tiz_fsm_state_to_str (p_ctx->state));
  fail_if (OMX_StateIdle != p_ctx->state);

  /* Check state transition success */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateIdle != state);

  /* Initiate transition to EXE */
  error = _ctx_reset (&ctx);
  state = OMX_StateExecuting;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* Await transition callback */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_ctx->state [%s]",
             tiz_fsm_state_to_str (p_ctx->state));
  fail_if (OMX_StateExecuting != p_ctx->state);

  /* Transfer buffer */
  error = _ctx_reset (&ctx);
  p_hdr->nFilledLen = p_hdr->nAllocLen;
  error = OMX_EmptyThisBuffer (p_hdl, p_hdr);
  fail_if (OMX_ErrorNone != error);

  /* Await BufferDone callback */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (p_ctx->p_hdr != p_hdr);

  /* Initiate transition to IDLE */
  error = _ctx_reset (&ctx);
  state = OMX_StateIdle;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* Await transition callback */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_ctx->state [%s]",
             tiz_fsm_state_to_str (p_ctx->state));
  fail_if (OMX_StateIdle != p_ctx->state);

  /* Initiate transition to LOADED */
  error = _ctx_reset (&ctx);
  state = OMX_StateLoaded;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* Deallocate buffers */
  fail_if (OMX_ErrorNone != error);
  for (i = 0; i < port_def.nBufferCountActual; ++i)
    {
      error = OMX_FreeBuffer (p_hdl, 0,      /* input port */
                              p_hdr);
      fail_if (OMX_ErrorNone != error);
    }

  /* Await transition callback */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateLoaded != p_ctx->state);

  /* Check state transition success */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  error = OMX_FreeHandle (p_hdl);
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  fail_if (OMX_ErrorNone != error);

  _ctx_destroy(&ctx);
}
END_TEST

START_TEST (test_tizonia_command_cancellation_loaded_to_idle_no_buffers)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = 0;
  OMX_COMMANDTYPE cmd = OMX_CommandStateSet;
  OMX_STATETYPE state = OMX_StateIdle;
  cc_ctx_t ctx;
  check_common_context_t *p_ctx = NULL;
  OMX_BOOL timedout = OMX_FALSE;
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  OMX_PARAM_BUFFERSUPPLIERTYPE supplier;
  OMX_INDEXTYPE index = OMX_IndexParamPortDefinition;
  OMX_COMPONENTTYPE fake_comp;
  OMX_TUNNELSETUPTYPE tsetup = { 0, OMX_BufferSupplyUnspecified };

  init_fake_comp(&fake_comp);

  error = _ctx_init (&ctx);
  fail_if (OMX_ErrorNone != error);

  p_ctx = (check_common_context_t *) (ctx);

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Instantiate the component */
  /* ------------------------- */
  error = OMX_GetHandle (&p_hdl, COMPONENT_NAME, (OMX_PTR *) (&ctx),
                         &_check_cbacks);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%X]", p_hdl);

  /* -------------------------------------- */
  /* Obtain the port def params for port #0 */
  /* -------------------------------------- */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  error = OMX_GetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferSize [%d]", port_def.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferCountActual [%d]",
             port_def.nBufferCountActual);

  /* ----------------------------------------*/
  /* Set supplier settings to "non-supplier" */
  /* ----------------------------------------*/
  supplier.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
  supplier.nVersion.nVersion = OMX_VERSION;
  supplier.nPortIndex = 0;
  supplier.eBufferSupplier = OMX_BufferSupplyOutput;
  error = OMX_SetParameter (p_hdl, OMX_IndexParamCompBufferSupplier,
                            &supplier);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_BufferSupplyInput [%s]",
             COMPONENT_NAME, tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* -------------------- */
  /* Create a fake tunnel */
  /* -------------------- */
  error = ((OMX_COMPONENTTYPE*)p_hdl)->ComponentTunnelRequest (p_hdl,
                                                                  0, /* port index */
                                                                  &fake_comp,
                                                                  0, /* whatever */
                                                                  &tsetup);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------- */
  /* Initiate transition to IDLE */
  /* --------------------------- */
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------- */
  /* Check state is still LOADED */
  /* --------------------------- */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* -------------------------------------------------------------- */
  /* Cancel transition to IDLE by commanding a transition to LOADED */
  /* -------------------------------------------------------------- */
  error = _ctx_reset (&ctx);
  state = OMX_StateLoaded;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateLoaded != p_ctx->state);
  fail_if (OMX_ErrorCommandCanceled != p_ctx->error);

  /* ------------------------------ */
  /* Check state transition success */
  /* ------------------------------ */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* ---------------------- */
  /* Free handle and deinit */
  /* ---------------------- */
  error = OMX_FreeHandle (p_hdl);
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  fail_if (OMX_ErrorNone != error);

  _ctx_destroy(&ctx);
}
END_TEST

START_TEST (test_tizonia_command_cancellation_loaded_to_idle_with_tunneled_supplied_buffers)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = 0;
  OMX_COMMANDTYPE cmd = OMX_CommandStateSet;
  OMX_STATETYPE state = OMX_StateIdle;
  cc_ctx_t ctx;
  check_common_context_t *p_ctx = NULL;
  OMX_BOOL timedout = OMX_FALSE;
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  OMX_PARAM_BUFFERSUPPLIERTYPE supplier;
  OMX_INDEXTYPE index = OMX_IndexParamPortDefinition;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_U32 i;
  OMX_U8 *p_buf = NULL;
  OMX_COMPONENTTYPE fake_comp;
  OMX_TUNNELSETUPTYPE tsetup = { 0, OMX_BufferSupplyUnspecified };

  init_fake_comp(&fake_comp);

  error = _ctx_init (&ctx);
  fail_if (OMX_ErrorNone != error);

  p_ctx = (check_common_context_t *) (ctx);

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Instantiate the component */
  /* ------------------------- */
  error = OMX_GetHandle (&p_hdl, COMPONENT_NAME, (OMX_PTR *) (&ctx),
                         &_check_cbacks);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%X]", p_hdl);

  /* -------------------------------------- */
  /* Obtain the port def params for port #0 */
  /* -------------------------------------- */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  error = OMX_GetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferSize [%d]", port_def.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferCountActual [%d]",
             port_def.nBufferCountActual);

  /* ------------------------------------ */
  /* Increase the buffer count on port #0 */
  /* ------------------------------------ */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  port_def.nBufferCountActual = port_def.nBufferCountMin + 1;
  error = OMX_SetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferCountActual [%d]",
             port_def.nBufferCountActual);

  /* ----------------------------------------*/
  /* Set supplier settings to "non-supplier" */
  /* ----------------------------------------*/
  supplier.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
  supplier.nVersion.nVersion = OMX_VERSION;
  supplier.nPortIndex = 0;
  supplier.eBufferSupplier = OMX_BufferSupplyOutput;
  error = OMX_SetParameter (p_hdl, OMX_IndexParamCompBufferSupplier,
                            &supplier);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_BufferSupplyInput [%s]",
             COMPONENT_NAME, tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* -------------------- */
  /* Create a fake tunnel */
  /* -------------------- */
  error = ((OMX_COMPONENTTYPE*)p_hdl)->
    ComponentTunnelRequest (p_hdl,
                            0, /* port index */
                            &fake_comp,
                            0, /* whatever */
                            &tsetup);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------- */
  /* Initiate transition to IDLE */
  /* --------------------------- */
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------- */
  /* Check state is still LOADED */
  /* --------------------------- */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* ------------------------------------ */
  /* Allocate nBufferCountActual - 1 buffers */
  /* ------------------------------------ */
  p_buf = tiz_mem_alloc (port_def.nBufferSize * sizeof (OMX_U8));
  for (i = 0; i < port_def.nBufferCountActual - 1; ++i)
    {
      error = OMX_UseBuffer (p_hdl, &p_hdr, 0,       /* input port */
                             0, port_def.nBufferSize, p_buf);
      fail_if (OMX_ErrorNone != error);
    }

  /* -------------------------------------------------------------- */
  /* Cancel transition to IDLE by commanding a transition to LOADED */
  /* -------------------------------------------------------------- */
  /* NOTE: This must fail with error OMX_ErrorIncorrectStateOperation */
  error = _ctx_reset (&ctx);
  state = OMX_StateLoaded;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorIncorrectStateOperation != error);

  /* ---------------------- */
  /* Now deallocate buffers */
  /* ---------------------- */
  for (i = 0; i < port_def.nBufferCountActual - 1; ++i)
    {
      error = OMX_FreeBuffer (p_hdl, 0,      /* input port */
                              p_hdr);
      fail_if (OMX_ErrorNone != error);
    }

  /* -------------------------------------------------------------- */
  /* Cancel transition to IDLE by commanding a transition to LOADED */
  /* -------------------------------------------------------------- */
  /* NOTE: This must succeed now */
  error = _ctx_reset (&ctx);
  state = OMX_StateLoaded;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateLoaded != p_ctx->state);
  fail_if (OMX_ErrorCommandCanceled != p_ctx->error);

  /* ------------------------------ */
  /* Check state transition success */
  /* ------------------------------ */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* ---------------------- */
  /* Free handle and deinit */
  /* ---------------------- */
  error = OMX_FreeHandle (p_hdl);
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  fail_if (OMX_ErrorNone != error);

  _ctx_destroy(&ctx);
}
END_TEST

START_TEST (test_tizonia_command_cancellation_loaded_to_idle_no_buffers_port_disabled_unblocks_transition)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = 0;
  OMX_COMMANDTYPE cmd = OMX_CommandStateSet;
  OMX_STATETYPE state = OMX_StateIdle;
  cc_ctx_t ctx;
  check_common_context_t *p_ctx = NULL;
  OMX_BOOL timedout = OMX_FALSE;
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  OMX_PARAM_BUFFERSUPPLIERTYPE supplier;
  OMX_INDEXTYPE index = OMX_IndexParamPortDefinition;
  OMX_COMPONENTTYPE fake_comp;
  OMX_TUNNELSETUPTYPE tsetup = { 0, OMX_BufferSupplyUnspecified };
  int loop = 0;

  init_fake_comp(&fake_comp);

  error = _ctx_init (&ctx);
  fail_if (OMX_ErrorNone != error);

  p_ctx = (check_common_context_t *) (ctx);

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Instantiate the component */
  /* ------------------------- */
  error = OMX_GetHandle (&p_hdl, COMPONENT_NAME, (OMX_PTR *) (&ctx),
                         &_check_cbacks);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%X]", p_hdl);

  /* -------------------------------------- */
  /* Obtain the port def params for port #0 */
  /* -------------------------------------- */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  error = OMX_GetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferSize [%d]", port_def.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferCountActual [%d]",
             port_def.nBufferCountActual);

  /* ------------------------------------ */
  /* Increase the buffer count on port #0 */
  /* ------------------------------------ */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  port_def.nBufferCountActual = port_def.nBufferCountMin + 1;
  error = OMX_SetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferCountActual [%d]",
             port_def.nBufferCountActual);

  /* ----------------------------------------*/
  /* Set supplier settings to "non-supplier" */
  /* ----------------------------------------*/
  supplier.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
  supplier.nVersion.nVersion = OMX_VERSION;
  supplier.nPortIndex = 0;
  supplier.eBufferSupplier = OMX_BufferSupplyOutput;
  error = OMX_SetParameter (p_hdl, OMX_IndexParamCompBufferSupplier,
                            &supplier);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_BufferSupplyInput [%s]",
             COMPONENT_NAME, tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* -------------------- */
  /* Create a fake tunnel */
  /* -------------------- */
  error = ((OMX_COMPONENTTYPE*)p_hdl)->ComponentTunnelRequest (p_hdl,
                                                                  0, /* port index */
                                                                  &fake_comp,
                                                                  0, /* whatever */
                                                                  &tsetup);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------- */
  /* Initiate transition to IDLE */
  /* --------------------------- */
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------- */
  /* Check state is still LOADED */
  /* --------------------------- */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* ------------------------------------------------------------ */
  /* Unblock transition to IDLE by commanding the port to disable */
  /* ------------------------------------------------------------ */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandPortDisable;
  error = OMX_SendCommand (p_hdl, cmd, 0, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ---------------------------- */
  /* Await port disabled callback */
  /* ---------------------------- */
  /* NOTE: This callback must occur before the Idle transition callback */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (0 != p_ctx->port);
  fail_if (OMX_ErrorNone != p_ctx->error);

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  /* NOTE: Since we need to receive two consecutive callbacks, we poll for the
     second instead of waiting */
  while (OMX_StateIdle != p_ctx->state && ++loop < 5)
    {
      tiz_sleep(10000);
    }
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_StateIdle != p_ctx->state);
  fail_if (OMX_ErrorNone != p_ctx->error);

  /* ------------------------------ */
  /* Check state transition success */
  /* ------------------------------ */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateIdle != state);

  /* ---------------- */
  /* Unload component */
  /* ---------------- */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandStateSet;
  state = OMX_StateLoaded;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateLoaded != p_ctx->state);
  fail_if (OMX_ErrorNone != p_ctx->error);

  /* ------------------------------ */
  /* Check state transition success */
  /* ------------------------------ */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* ---------------------- */
  /* Free handle and deinit */
  /* ---------------------- */
  error = OMX_FreeHandle (p_hdl);
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  fail_if (OMX_ErrorNone != error);

  _ctx_destroy(&ctx);
}
END_TEST

START_TEST (test_tizonia_command_cancellation_loaded_to_idle_with_buffers_port_disabled_cant_unblock_transition)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = 0;
  OMX_COMMANDTYPE cmd = OMX_CommandStateSet;
  OMX_STATETYPE state = OMX_StateIdle;
  cc_ctx_t ctx;
  check_common_context_t *p_ctx = NULL;
  OMX_BOOL timedout = OMX_FALSE;
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  OMX_PARAM_BUFFERSUPPLIERTYPE supplier;
  OMX_INDEXTYPE index = OMX_IndexParamPortDefinition;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_U32 i;
  OMX_U8 *p_buf = NULL;
  OMX_COMPONENTTYPE fake_comp;
  OMX_TUNNELSETUPTYPE tsetup = { 0, OMX_BufferSupplyUnspecified };
  int loop = 0;

  init_fake_comp(&fake_comp);

  error = _ctx_init (&ctx);
  fail_if (OMX_ErrorNone != error);

  p_ctx = (check_common_context_t *) (ctx);

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Instantiate the component */
  /* ------------------------- */
  error = OMX_GetHandle (&p_hdl, COMPONENT_NAME, (OMX_PTR *) (&ctx),
                         &_check_cbacks);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%X]", p_hdl);

  /* -------------------------------------- */
  /* Obtain the port def params for port #0 */
  /* -------------------------------------- */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  error = OMX_GetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferSize [%d]", port_def.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferCountActual [%d]",
             port_def.nBufferCountActual);

  /* ------------------------------------ */
  /* Increase the buffer count on port #0 */
  /* ------------------------------------ */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  port_def.nBufferCountActual = port_def.nBufferCountMin + 1;
  error = OMX_SetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferCountActual [%d]",
             port_def.nBufferCountActual);

  /* ----------------------------------------*/
  /* Set supplier settings to "non-supplier" */
  /* ----------------------------------------*/
  supplier.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
  supplier.nVersion.nVersion = OMX_VERSION;
  supplier.nPortIndex = 0;
  supplier.eBufferSupplier = OMX_BufferSupplyOutput;
  error = OMX_SetParameter (p_hdl, OMX_IndexParamCompBufferSupplier,
                            &supplier);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_BufferSupplyInput [%s]",
             COMPONENT_NAME, tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* -------------------- */
  /* Create a fake tunnel */
  /* -------------------- */
  error = ((OMX_COMPONENTTYPE*)p_hdl)->ComponentTunnelRequest (p_hdl,
                                                                  0, /* port index */
                                                                  &fake_comp,
                                                                  0, /* whatever */
                                                                  &tsetup);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------- */
  /* Initiate transition to IDLE */
  /* --------------------------- */
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------- */
  /* Check state is still LOADED */
  /* --------------------------- */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* ------------------------------------ */
  /* Allocate nBufferCountActual - 1 buffers */
  /* ------------------------------------ */
  p_buf = tiz_mem_alloc (port_def.nBufferSize * sizeof (OMX_U8));
  for (i = 0; i < port_def.nBufferCountActual - 1; ++i)
    {
      error = OMX_UseBuffer (p_hdl, &p_hdr, 0,       /* input port */
                             0, port_def.nBufferSize, p_buf);
      fail_if (OMX_ErrorNone != error);
    }

  /* ------------------------------------------------------------------- */
  /* Try to unblock transition to IDLE by commanding the port to disable */
  /* ------------------------------------------------------------------- */
  /* NOTE: This must fail with error OMX_ErrorIncorrectStateOperation */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandPortDisable;
  error = OMX_SendCommand (p_hdl, cmd, 0, NULL);
  fail_if (OMX_ErrorIncorrectStateOperation != error);

  /* ---------------------- */
  /* Now deallocate buffers */
  /* ---------------------- */
  for (i = 0; i < port_def.nBufferCountActual - 1; ++i)
    {
      error = OMX_FreeBuffer (p_hdl, 0,      /* input port */
                              p_hdr);
      fail_if (OMX_ErrorNone != error);
    }

  /* ------------------------------------------------------------------- */
  /* Try to unblock transition to IDLE by commanding the port to disable */
  /* ------------------------------------------------------------------- */
  /* NOTE: This must succeed now */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandPortDisable;
  error = OMX_SendCommand (p_hdl, cmd, 0, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ---------------------------- */
  /* Await port disabled callback */
  /* ---------------------------- */
  /* NOTE: This callback must occur before the Idle transition callback */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (0 != p_ctx->port);
  fail_if (OMX_ErrorNone != p_ctx->error);

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  /* NOTE: Since we need to receive two consecutive callbacks, we poll for the
     second instead of waiting */
  while (OMX_StateIdle != p_ctx->state && ++loop < 5)
    {
      tiz_sleep(10000);
    }
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_StateIdle != p_ctx->state);
  fail_if (OMX_ErrorNone != p_ctx->error);

  /* ------------------------------ */
  /* Check state transition success */
  /* ------------------------------ */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateIdle != state);

  /* ---------------- */
  /* Unload component */
  /* ---------------- */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandStateSet;
  state = OMX_StateLoaded;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (p_ctx->state));
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateLoaded != p_ctx->state);
  fail_if (OMX_ErrorNone != p_ctx->error);

  /* ------------------------------ */
  /* Check state transition success */
  /* ------------------------------ */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* ---------------------- */
  /* Free handle and deinit */
  /* ---------------------- */
  error = OMX_FreeHandle (p_hdl);
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  fail_if (OMX_ErrorNone != error);

  _ctx_destroy(&ctx);
}
END_TEST

START_TEST (test_tizonia_command_cancellation_disabled_to_enabled_no_buffers)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = 0;
  OMX_COMMANDTYPE cmd = OMX_CommandStateSet;
  OMX_STATETYPE state = OMX_StateIdle;
  cc_ctx_t ctx;
  check_common_context_t *p_ctx = NULL;
  OMX_BOOL timedout = OMX_FALSE;
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  OMX_PARAM_BUFFERSUPPLIERTYPE supplier;
  OMX_INDEXTYPE index = OMX_IndexParamPortDefinition;
  OMX_COMPONENTTYPE fake_comp;
  OMX_TUNNELSETUPTYPE tsetup = { 0, OMX_BufferSupplyUnspecified };

  init_fake_comp(&fake_comp);

  error = _ctx_init (&ctx);
  fail_if (OMX_ErrorNone != error);

  p_ctx = (check_common_context_t *) (ctx);

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Instantiate the component */
  /* ------------------------- */
  error = OMX_GetHandle (&p_hdl, COMPONENT_NAME, (OMX_PTR *) (&ctx),
                         &_check_cbacks);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%X]", p_hdl);

  /* -------------------------------------- */
  /* Obtain the port def params for port #0 */
  /* -------------------------------------- */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  error = OMX_GetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferSize [%d]", port_def.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferCountActual [%d]",
             port_def.nBufferCountActual);

  /* ----------------------------------------*/
  /* Set supplier settings to "non-supplier" */
  /* ----------------------------------------*/
  supplier.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
  supplier.nVersion.nVersion = OMX_VERSION;
  supplier.nPortIndex = 0;
  supplier.eBufferSupplier = OMX_BufferSupplyOutput;
  error = OMX_SetParameter (p_hdl, OMX_IndexParamCompBufferSupplier,
                            &supplier);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_BufferSupplyInput [%s]",
             COMPONENT_NAME, tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* -------------------- */
  /* Create a fake tunnel */
  /* -------------------- */
  error = ((OMX_COMPONENTTYPE*)p_hdl)->ComponentTunnelRequest (p_hdl,
                                                                  0, /* port index */
                                                                  &fake_comp,
                                                                  0, /* whatever */
                                                                  &tsetup);
  fail_if (OMX_ErrorNone != error);

  /* --------------- */
  /* Disable port #0 */
  /* --------------- */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandPortDisable;
  error = OMX_SendCommand (p_hdl, cmd, 0, NULL);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------- */
  /* Await port disable callback */
  /* --------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (0 != p_ctx->port);
  fail_if (OMX_ErrorNone != p_ctx->error);

  /* ------------------------------ */
  /* Verfify port disabled property */
  /* ------------------------------ */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  index = OMX_IndexParamPortDefinition;
  error = OMX_GetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == port_def.bEnabled);

  /* --------------------------- */
  /* Initiate transition to IDLE */
  /* --------------------------- */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandStateSet;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateIdle != p_ctx->state);
  fail_if (OMX_ErrorNone != p_ctx->error);

  /* ------------------- */
  /* Check state is IDLE */
  /* ------------------- */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateIdle != state);

  /* -------------- */
  /* Enable port #0 */
  /* -------------- */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandPortEnable;
  error = OMX_SendCommand (p_hdl, cmd, 0, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------- */
  /* Await until timeout */
  /* ------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_FAILURE, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE != timedout);

  /* ---------------------------------------------------------- */
  /* Cancel port ENABLED command by issuing a port DISABLED cmd */
  /* ---------------------------------------------------------- */
  error = _ctx_reset (&ctx);
  timedout = OMX_FALSE;
  cmd = OMX_CommandPortDisable;
  error = OMX_SendCommand (p_hdl, cmd, 0, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ----------------------------------- */
  /* Await canceled port enable callback */
  /* ----------------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_ErrorCommandCanceled != p_ctx->error);

  /* ----------------------------- */
  /*  Command transition to LOADED */
  /* ----------------------------- */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandStateSet;
  state = OMX_StateLoaded;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateLoaded != p_ctx->state);
  fail_if (OMX_ErrorNone != p_ctx->error);

  /* ------------------------------ */
  /* Check state transition success */
  /* ------------------------------ */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* ---------------------- */
  /* Free handle and deinit */
  /* ---------------------- */
  error = OMX_FreeHandle (p_hdl);
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  fail_if (OMX_ErrorNone != error);

  _ctx_destroy(&ctx);
}
END_TEST

START_TEST (test_tizonia_command_cancellation_disabled_to_enabled_with_tunneled_supplied_buffers)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = 0;
  OMX_COMMANDTYPE cmd = OMX_CommandStateSet;
  OMX_STATETYPE state = OMX_StateIdle;
  cc_ctx_t ctx;
  check_common_context_t *p_ctx = NULL;
  OMX_BOOL timedout = OMX_FALSE;
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  OMX_PARAM_BUFFERSUPPLIERTYPE supplier;
  OMX_INDEXTYPE index = OMX_IndexParamPortDefinition;
  OMX_COMPONENTTYPE fake_comp;
  OMX_TUNNELSETUPTYPE tsetup = { 0, OMX_BufferSupplyUnspecified };
  OMX_U8 *p_buf = NULL;
  OMX_U32 i;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  init_fake_comp(&fake_comp);

  error = _ctx_init (&ctx);
  fail_if (OMX_ErrorNone != error);

  p_ctx = (check_common_context_t *) (ctx);

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Instantiate the component */
  /* ------------------------- */
  error = OMX_GetHandle (&p_hdl, COMPONENT_NAME, (OMX_PTR *) (&ctx),
                         &_check_cbacks);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%X]", p_hdl);

  /* -------------------------------------- */
  /* Obtain the port def params for port #0 */
  /* -------------------------------------- */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  error = OMX_GetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferSize [%d]", port_def.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferCountActual [%d]",
             port_def.nBufferCountActual);

  /* ------------------------------------ */
  /* Increase the buffer count on port #0 */
  /* ------------------------------------ */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  port_def.nBufferCountActual = port_def.nBufferCountMin + 1;
  error = OMX_SetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "nBufferCountActual [%d]",
             port_def.nBufferCountActual);

  /* ----------------------------------------*/
  /* Set supplier settings to "non-supplier" */
  /* ----------------------------------------*/
  supplier.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
  supplier.nVersion.nVersion = OMX_VERSION;
  supplier.nPortIndex = 0;
  supplier.eBufferSupplier = OMX_BufferSupplyOutput;
  error = OMX_SetParameter (p_hdl, OMX_IndexParamCompBufferSupplier,
                            &supplier);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_BufferSupplyInput [%s]",
             COMPONENT_NAME, tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* -------------------- */
  /* Create a fake tunnel */
  /* -------------------- */
  error = ((OMX_COMPONENTTYPE*)p_hdl)->ComponentTunnelRequest (p_hdl,
                                                                  0, /* port index */
                                                                  &fake_comp,
                                                                  0, /* whatever */
                                                                  &tsetup);
  fail_if (OMX_ErrorNone != error);

  /* --------------- */
  /* Disable port #0 */
  /* --------------- */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandPortDisable;
  error = OMX_SendCommand (p_hdl, cmd, 0, NULL);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------- */
  /* Await port disable callback */
  /* --------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (0 != p_ctx->port);
  fail_if (OMX_ErrorNone != p_ctx->error);

  /* ------------------------------ */
  /* Verify port disabled property */
  /* ------------------------------ */
  port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = 0;
  index = OMX_IndexParamPortDefinition;
  error = OMX_GetParameter (p_hdl, index, &port_def);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == port_def.bEnabled);

  /* --------------------------- */
  /* Initiate transition to IDLE */
  /* --------------------------- */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandStateSet;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateIdle != p_ctx->state);
  fail_if (OMX_ErrorNone != p_ctx->error);

  /* ------------------- */
  /* Check state is IDLE */
  /* ------------------- */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateIdle != state);

  /* -------------- */
  /* Enable port #0 */
  /* -------------- */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandPortEnable;
  error = OMX_SendCommand (p_hdl, cmd, 0, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------- */
  /* Await until timeout */
  /* ------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_FAILURE, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE != timedout);


  /* ------------------------------------ */
  /* Allocate nBufferCountActual - 1 buffers */
  /* ------------------------------------ */
  p_buf = tiz_mem_alloc (port_def.nBufferSize * sizeof (OMX_U8));
  for (i = 0; i < port_def.nBufferCountActual - 1; ++i)
    {
      error = OMX_UseBuffer (p_hdl, &p_hdr, 0,       /* input port */
                             0, port_def.nBufferSize, p_buf);
      fail_if (OMX_ErrorNone != error);
    }

  /* ---------------------------------------------------------- */
  /* Cancel port ENABLED command by issuing a port DISABLED cmd */
  /* ---------------------------------------------------------- */
  /* NOTE: This must fail with error OMX_ErrorIncorrectStateOperation */
  error = _ctx_reset (&ctx);
  timedout = OMX_FALSE;
  cmd = OMX_CommandPortDisable;
  error = OMX_SendCommand (p_hdl, cmd, 0, NULL);
  fail_if (OMX_ErrorIncorrectStateOperation != error);

  /* ---------------------- */
  /* Now deallocate buffers */
  /* ---------------------- */
  for (i = 0; i < port_def.nBufferCountActual - 1; ++i)
    {
      error = OMX_FreeBuffer (p_hdl, 0,      /* input port */
                              p_hdr);
      fail_if (OMX_ErrorNone != error);
    }

  /* ---------------------------------------------------------- */
  /* Cancel port ENABLED command by issuing a port DISABLED cmd */
  /* ---------------------------------------------------------- */
  /* NOTE: This must succeed now */
  error = _ctx_reset (&ctx);
  timedout = OMX_FALSE;
  cmd = OMX_CommandPortDisable;
  error = OMX_SendCommand (p_hdl, cmd, 0, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ----------------------------------- */
  /* Await canceled port enable callback */
  /* ----------------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_ErrorCommandCanceled != p_ctx->error);

  /* ----------------------------- */
  /*  Command transition to LOADED */
  /* ----------------------------- */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandStateSet;
  state = OMX_StateLoaded;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateLoaded != p_ctx->state);
  fail_if (OMX_ErrorNone != p_ctx->error);

  /* ------------------------------ */
  /* Check state transition success */
  /* ------------------------------ */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_fsm_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* ---------------------- */
  /* Free handle and deinit */
  /* ---------------------- */
  error = OMX_FreeHandle (p_hdl);
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  fail_if (OMX_ErrorNone != error);

  _ctx_destroy(&ctx);
}
END_TEST

Suite *
tiz_suite (void)
{
  TCase *tc_tizonia;
  Suite *s = suite_create ("libtizonia");

  putenv(TIZ_PLATFORM_RC_FILE_ENV);

  /* IL Common API test cases */
  tc_tizonia = tcase_create ("tizonia");
  tcase_add_unchecked_fixture (tc_tizonia, setup, teardown);

/*   (void) test_tizonia_getstate; */
/*   (void) test_tizonia_gethandle_freehandle; */
/*   (void) test_tizonia_getparameter; */
/*   (void) test_tizonia_roles; */
/*   (void) test_tizonia_preannouncements_extension; */
/*   (void) test_tizonia_move_to_exe_and_transfer_with_allocbuffer; */
/*   (void) test_tizonia_command_cancellation_loaded_to_idle_no_buffers; */
  (void) test_tizonia_command_cancellation_loaded_to_idle_with_tunneled_supplied_buffers;
/*   (void) test_tizonia_command_cancellation_loaded_to_idle_no_buffers_port_disabled_unblocks_transition; */
  (void) test_tizonia_command_cancellation_loaded_to_idle_with_buffers_port_disabled_cant_unblock_transition;
/*   (void) test_tizonia_command_cancellation_disabled_to_enabled_no_buffers; */
  (void) test_tizonia_command_cancellation_disabled_to_enabled_with_tunneled_supplied_buffers;

  tcase_add_test (tc_tizonia, test_tizonia_getstate);
  tcase_add_test (tc_tizonia, test_tizonia_gethandle_freehandle);
  tcase_add_test (tc_tizonia, test_tizonia_getparameter);
  tcase_add_test (tc_tizonia, test_tizonia_roles);
  tcase_add_test (tc_tizonia, test_tizonia_preannouncements_extension);
  tcase_add_test (tc_tizonia, test_tizonia_pd_set);
  tcase_add_test (tc_tizonia,
                  test_tizonia_move_to_exe_and_transfer_with_allocbuffer);
  tcase_add_test (tc_tizonia,
                  test_tizonia_command_cancellation_loaded_to_idle_no_buffers);
  /* TEST DISABLED */
  /*   tcase_add_test (tc_tizonia, */
  /*                   test_tizonia_command_cancellation_loaded_to_idle_with_tunneled_supplied_buffers); */
  tcase_add_test (tc_tizonia,
                  test_tizonia_command_cancellation_loaded_to_idle_no_buffers_port_disabled_unblocks_transition);
  /* TEST DISABLED */
  /*   tcase_add_test (tc_tizonia, */
  /*                   test_tizonia_command_cancellation_loaded_to_idle_with_buffers_port_disabled_cant_unblock_transition); */
  tcase_add_test (tc_tizonia,
                  test_tizonia_command_cancellation_disabled_to_enabled_no_buffers);
  /* TEST DISABLED */
  /*   tcase_add_test (tc_tizonia, */
  /*                   test_tizonia_command_cancellation_disabled_to_enabled_with_tunneled_supplied_buffers); */

  suite_add_tcase (s, tc_tizonia);

  return s;
}

int
main (void)
{
  int number_failed;
  SRunner *sr = srunner_create (tiz_suite ());

  tiz_log_init();

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Tizonia OpenMAX IL - libtizonia unit tests");

  srunner_run_all (sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  tiz_log_deinit ();

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */
