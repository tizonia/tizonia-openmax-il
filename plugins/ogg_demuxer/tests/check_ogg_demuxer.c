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
 * @file   check_ogg_demuxer.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Ogg demuxer unit tests
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizplatform.h"
#include "tizutils.h"
#include "tizfsm.h"
#include "tizkernel.h"

#include "OMX_Component.h"
#include "OMX_Types.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.ogg_demuxer.check"
#endif

char *pg_rmd_path;
pid_t g_rmd_pid;

#define COMPONENT_NAME "OMX.Aratelia.container_demuxer.ogg"

#define MAX_BUFFERS_IN_TRANSIT_PER_PORT 1

static const char *pg_files[] = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
};

#define OGG_DEMUXER_TEST_TIMEOUT  30
#define INFINITE_WAIT             0xffffffff
/* duration of event timeout in msec when we expect event to be set */
#define TIMEOUT_EXPECTING_SUCCESS 500
/* duration of event timeout in msec when we don't expect event to be set */
#define TIMEOUT_EXPECTING_FAILURE 2000

typedef void *cc_ctx_t;
typedef struct check_common_context check_common_context_t;
struct check_common_context
{
  OMX_BOOL signaled;
  tiz_mutex_t mutex;
  tiz_cond_t cond;
  OMX_STATETYPE state;
  OMX_BOOL eos;
  tiz_vector_t *p_aud_hdrs;
  tiz_vector_t *p_vid_hdrs;
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
    tiz_mem_calloc (sizeof (check_common_context_t), 1);

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

  p_ctx->eos = OMX_FALSE;

  tiz_check_omx_err
    (tiz_vector_init (&(p_ctx->p_aud_hdrs), sizeof (OMX_BUFFERHEADERTYPE *)));
  tiz_check_omx_err
    (tiz_vector_init (&(p_ctx->p_vid_hdrs), sizeof (OMX_BUFFERHEADERTYPE *)));

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

  tiz_vector_clear (p_ctx->p_aud_hdrs);
  tiz_vector_destroy (p_ctx->p_aud_hdrs);
  tiz_vector_clear (p_ctx->p_vid_hdrs);
  tiz_vector_destroy (p_ctx->p_vid_hdrs);

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

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Component Event [%s]",
             tiz_evt_to_str (eEvent));

  if (OMX_EventCmdComplete == eEvent)
    {
      switch ((OMX_COMMANDTYPE) (nData1))
        {
        case OMX_CommandStateSet:
          {
            TIZ_LOG (TIZ_PRIORITY_TRACE, "Component transitioned to [%s]",
                       tiz_state_to_str ((OMX_STATETYPE) (nData2)));
            p_ctx->state = (OMX_STATETYPE) (nData2);
            _ctx_signal (pp_ctx);
            break;
          }

        case OMX_CommandPortDisable:
          {
            TIZ_LOG (TIZ_PRIORITY_TRACE, "Port [%d] is now DISABLED", nData2);
            _ctx_signal (pp_ctx);
          }
          break;

        case OMX_CommandPortEnable:
        default:
          {
            assert (0);
          }

        };
    }

  if (OMX_EventBufferFlag == eEvent)
    {
      if (nData2 & OMX_BUFFERFLAG_EOS)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "Received EOS from [%s] port[%i]",
                     COMPONENT_NAME, nData1);
        }
      else
        {
          fail_if (0);
        }
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE check_EmptyBufferDone
  (OMX_HANDLETYPE ap_hdl,
   OMX_PTR ap_app_data, OMX_BUFFERHEADERTYPE * ap_buf)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "EmptyBufferDone: BUFFER [%p]", ap_buf);
  fail_if (0);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE check_FillBufferDone
  (OMX_HANDLETYPE ap_hdl,
   OMX_PTR ap_app_data, OMX_BUFFERHEADERTYPE * ap_buf)
{
  check_common_context_t *p_ctx = NULL;
  cc_ctx_t *pp_ctx = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE,
           "FillBufferDone: BUFFER [%p] pid [%d] "
           "nFilledLen [%d] nFlags [%X]",
           ap_buf, ap_buf->nOutputPortIndex,
           ap_buf->nFilledLen,
           ap_buf->nFlags);

  assert (NULL != ap_app_data);
  assert (NULL != ap_buf);
  pp_ctx = (cc_ctx_t *) ap_app_data;
  p_ctx = *pp_ctx;

  assert (ap_buf->nOutputPortIndex    == 0
          || ap_buf->nOutputPortIndex == 1);

  (void) tiz_mutex_lock (&p_ctx->mutex);

  if (ap_buf->nOutputPortIndex == 0) /* Audio port */
    {
      tiz_vector_push_back (p_ctx->p_aud_hdrs, &ap_buf);
    }
  else
    {
      tiz_vector_push_back (p_ctx->p_vid_hdrs, &ap_buf);
    }

  tiz_mutex_unlock (&p_ctx->mutex);

  _ctx_signal (pp_ctx);

  return OMX_ErrorNone;
}


static OMX_CALLBACKTYPE _check_cbacks = {
  check_EventHandler,
  check_EmptyBufferDone,
  check_FillBufferDone
};

static bool
init_test_data()
{
  bool rv = false;
  const char *p_testfile1 = NULL;
  const char *p_testfile2 = NULL;
  const char *p_testfile3 = NULL;
  const char *p_testfile4 = NULL;
  const char *p_testfile5 = NULL;

  p_testfile1 = tiz_rcfile_get_value("plugins-data",
                                     "OMX.Aratelia.container_demuxer.ogg.default_uri");
  p_testfile2 = tiz_rcfile_get_value("plugins-data",
                                     "OMX.Aratelia.container_demuxer.ogg.audio_uri_original");
  p_testfile3 = tiz_rcfile_get_value("plugins-data",
                                     "OMX.Aratelia.container_demuxer.ogg.audio_uri_demuxed");
  p_testfile4 = tiz_rcfile_get_value("plugins-data",
                                     "OMX.Aratelia.container_demuxer.ogg.video_uri_original");
  p_testfile5 = tiz_rcfile_get_value("plugins-data",
                                     "OMX.Aratelia.container_demuxer.ogg.video_uri_demuxed");

  p_testfile1 = access (p_testfile1, R_OK ) != -1 ? p_testfile1 : NULL;
  p_testfile2 = access (p_testfile2, R_OK ) != -1 ? p_testfile2 : NULL;
  p_testfile4 = access (p_testfile4, R_OK ) != -1 ? p_testfile4 : NULL;

  if (!p_testfile1 || !p_testfile2
      ||!p_testfile3 || !p_testfile4 || !p_testfile5)

    {
      TIZ_LOG(TIZ_PRIORITY_TRACE, "Some of the test streams are not available...");
    }
  else
    {
      pg_files[0] = p_testfile1;
      pg_files[1] = p_testfile2;
      pg_files[2] = p_testfile3;
      pg_files[3] = p_testfile4;
      pg_files[4] = p_testfile5;
      TIZ_LOG(TIZ_PRIORITY_TRACE, "Test streams available [%s]", pg_files[0]);
      rv = true;
    }

  return rv;
}

static OMX_ERRORTYPE
transfer_buffers(cc_ctx_t * app_ctx, OMX_HANDLETYPE ap_hdl,
                 tiz_vector_t *ap_hdrs)
{
  check_common_context_t *p_ctx = NULL;
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int i = 0;
  int nbufs = 0;

  assert (NULL != app_ctx);
  assert (NULL != ap_hdrs);
  assert (NULL != ap_hdl);
  p_ctx = * app_ctx;

  if (tiz_mutex_lock (&p_ctx->mutex))
    {
      return OMX_ErrorBadParameter;
    }

  nbufs = tiz_vector_length (ap_hdrs);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "header list size [%d]", nbufs);

  for (i = 0; i < nbufs && i < MAX_BUFFERS_IN_TRANSIT_PER_PORT; i++)
    {
      pp_hdr = tiz_vector_at (ap_hdrs, i);
      assert (pp_hdr && *pp_hdr);
      p_hdr = *pp_hdr;
      p_hdr->nFilledLen = 0;
      TIZ_LOG (TIZ_PRIORITY_TRACE,
               "FillThisBuffer: BUFFER [%p] pid [%d] "
               "nFilledLen [%d] nFlags [%X]",
               p_hdr, p_hdr->nOutputPortIndex,
               p_hdr->nFilledLen,
               p_hdr->nFlags);
      tiz_vector_erase (ap_hdrs, i, 1);
      error = OMX_FillThisBuffer (ap_hdl, p_hdr);
      fail_if (OMX_ErrorNone != error);
    }

  tiz_mutex_unlock (&p_ctx->mutex);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
write_data_to_file(cc_ctx_t * app_ctx, tiz_vector_t *ap_hdrs,
                   FILE *ap_file, bool *ap_eof, int *ap_bytes_written,
                   const int file_id)
{
  check_common_context_t *p_ctx = NULL;
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  bool eof = false;
  int i = 0;
  int err = 0;
  int nbufs = 0;

  assert (NULL != app_ctx);
  assert (NULL != ap_hdrs);
  assert (NULL != ap_file);
  assert (NULL != ap_eof);
  assert (NULL != ap_bytes_written);
  p_ctx = * app_ctx;

  if (*ap_eof)
    {
      return OMX_ErrorNone;
    }

  if (tiz_mutex_lock (&p_ctx->mutex))
    {
      return OMX_ErrorBadParameter;
    }

  nbufs = tiz_vector_length (ap_hdrs);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "header list size [%d]",
           tiz_vector_length (ap_hdrs));

  for (i = 0; i < nbufs; i++)
    {
      pp_hdr = tiz_vector_at (ap_hdrs, i);
      assert (pp_hdr && *pp_hdr);
      p_hdr = *pp_hdr;

      if (!(*ap_eof) && p_hdr->nFilledLen > 0)
        {
          if (!(err = fwrite (p_hdr->pBuffer, 1, p_hdr->nFilledLen,
                              ap_file)))
            {
              TIZ_LOG (TIZ_PRIORITY_TRACE, "An error occurred while writing to file [%d].",
                       pg_files[file_id]);
              fail_if (0);
            }

          *ap_bytes_written += p_hdr->nFilledLen;
          TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] : bytes written [%d]",
                   pg_files[file_id], (*ap_bytes_written));
          p_hdr->nFilledLen = 0;
        }

      if (p_hdr->nFlags & OMX_BUFFERFLAG_EOS)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "End of file reached on stream [%s].",
                   pg_files[file_id]);
          /* EOF on stream */
          eof = true;
        }
    }

  *ap_eof = eof;
  tiz_mutex_unlock (&p_ctx->mutex);

  return OMX_ErrorNone;
}

static bool
ready_to_stop(cc_ctx_t * app_ctx,
              const OMX_U32 aud_actual, const OMX_U32 vid_actual)
{
  check_common_context_t *p_ctx = NULL;
  bool ready_to_go = false;;
  int aud_count = 0;
  int vid_count = 0;

  assert (NULL != app_ctx);
  p_ctx = * app_ctx;

  if (tiz_mutex_lock (&p_ctx->mutex))
    {
      return OMX_ErrorBadParameter;
    }

  aud_count = tiz_vector_length (p_ctx->p_aud_hdrs);
  vid_count = tiz_vector_length (p_ctx->p_vid_hdrs);

  tiz_mutex_unlock (&p_ctx->mutex);

  if ((aud_actual + vid_actual) == (aud_count + vid_count))
    {
      ready_to_go = true;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "aud_count [%d] vid_count [%d] ready_to_go [%s]",
           aud_count, vid_count, ready_to_go ? "YES" : "NO");

  return ready_to_go;
}

static bool
compare_files(const char *ap_file1, const char *ap_file2)
{
  char *cmp_cmd = NULL;
  bool  rc      = false;
  assert (NULL != ap_file1);
  assert (NULL != ap_file2);

  cmp_cmd = tiz_mem_calloc (1, strlen ("cmp") +
                            strlen (ap_file1) +
                            strlen (ap_file2) + 3);

  sprintf (cmp_cmd, "%s %s %s", "cmp", ap_file1, ap_file2);
  rc = (system (cmp_cmd) == 0);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "File comparison %s : [%s]", cmp_cmd,
           rc ? "OK" : "NOT OK");
  fprintf (stderr, "File comparison %s : [%s]\n", ap_file1,
           rc ? "OK" : "NOT OK");

  tiz_mem_free (cmp_cmd);

  return rc;
}

/*
 * Unit tests
 */

START_TEST (test_ogg_demuxer)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = NULL;
  OMX_COMMANDTYPE cmd = OMX_CommandStateSet;
  OMX_STATETYPE state = OMX_StateIdle;
  cc_ctx_t ctx;
  check_common_context_t *p_ctx = NULL;
  OMX_BOOL timedout = OMX_FALSE;
  OMX_PARAM_PORTDEFINITIONTYPE aud_port_def;
  OMX_PARAM_PORTDEFINITIONTYPE vid_port_def;
  OMX_PARAM_CONTENTURITYPE *p_uri_param = NULL;
  OMX_BUFFERHEADERTYPE **p_aud_hdrlst = NULL;
  OMX_BUFFERHEADERTYPE **p_vid_hdrlst = NULL;
  bool aud_eof = false;
  bool vid_eof = false;
  OMX_U32 i = 0;
  FILE *p_aud_file = NULL;
  FILE *p_vid_file = NULL;
  int aud_bytes_written = 0;
  int vid_bytes_written = 0;
  int idle_trasition_wait_count = 0;
  bool cmp_outcome = false;

  fail_if (!init_test_data());

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

  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_hdl [%p]", p_hdl);

  /* -------------------------------- */
  /* Obtain the port def from port #0 */
  /* -------------------------------- */
  aud_port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  aud_port_def.nVersion.nVersion = OMX_VERSION;
  aud_port_def.nPortIndex = 0;
  error = OMX_GetParameter (p_hdl, OMX_IndexParamPortDefinition, &aud_port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Audio port nBufferSize [%d]", aud_port_def.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Audio port nBufferCountActual [%d]",
             aud_port_def.nBufferCountActual);

  /* -------------------------------- */
  /* Obtain the port def from port #1 */
  /* -------------------------------- */
  vid_port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  vid_port_def.nVersion.nVersion = OMX_VERSION;
  vid_port_def.nPortIndex = 1;
  error = OMX_GetParameter (p_hdl, OMX_IndexParamPortDefinition, &vid_port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Video port nBufferSize [%d]", vid_port_def.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Video port nBufferCountActual [%d]",
             vid_port_def.nBufferCountActual);

  /* --------------------------------------------------------- */
  /* Set the same nBufferCountActual value for audio and video */
  /* --------------------------------------------------------- */
  vid_port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  vid_port_def.nVersion.nVersion = OMX_VERSION;
  vid_port_def.nBufferCountActual = aud_port_def.nBufferCountActual;
  error = OMX_SetParameter (p_hdl, OMX_IndexParamPortDefinition, &vid_port_def);
  fail_if (OMX_ErrorNone != error);

  /* ---------------------- */
  /* Obtain the current URI */
  /* ---------------------- */
  p_uri_param = tiz_mem_calloc
    (1, sizeof (OMX_PARAM_CONTENTURITYPE) + OMX_MAX_STRINGNAME_SIZE);

  fail_if (!p_uri_param);
  p_uri_param->nSize = sizeof (OMX_PARAM_CONTENTURITYPE)
    + OMX_MAX_STRINGNAME_SIZE;
  p_uri_param->nVersion.nVersion = OMX_VERSION;
  error = OMX_GetParameter (p_hdl, OMX_IndexParamContentURI, p_uri_param);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Retrieved URI [%s]", p_uri_param->contentURI);

  /* ----------------*/
  /* Set the new URI */
  /* ----------------*/
  strcpy ((char*)p_uri_param->contentURI, pg_files[0]);
  p_uri_param->contentURI[strlen (pg_files[0])] = '\0';
  error = OMX_SetParameter (p_hdl, OMX_IndexParamContentURI, p_uri_param);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_SetParameter(OMX_IndexParamContentURI, "
           "URI [%s]) = [%s]", p_uri_param->contentURI, tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

  /* ----------------------------------------- */
  /* Disable video output port (port index 1)  */
  /* ----------------------------------------- */
  /*   cmd = OMX_CommandPortDisable; */
  /*   error = OMX_SendCommand (p_hdl, cmd, 1, NULL); */
  /*   fail_if (OMX_ErrorNone != error); */

  /* --------------------------- */
  /* Await port disable callback */
  /* --------------------------- */
  /*   error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout); */
  /*   fail_if (OMX_ErrorNone != error); */
  /*   fail_if (OMX_TRUE == timedout); */

  /* --------------------------- */
  /* Initiate transition to IDLE */
  /* --------------------------- */
  error = _ctx_reset (&ctx);
  cmd = OMX_CommandStateSet;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ---------------------- */
  /* Allocate audio buffers */
  /* ---------------------- */
  p_aud_hdrlst = (OMX_BUFFERHEADERTYPE **)
    tiz_mem_calloc (aud_port_def.nBufferCountActual, sizeof (OMX_BUFFERHEADERTYPE *));

  for (i = 0; i < aud_port_def.nBufferCountActual; ++i)
    {
      error = OMX_AllocateBuffer (p_hdl, &p_aud_hdrlst[i], 0,    /* audio port */
                                  0, aud_port_def.nBufferSize);
      fail_if (OMX_ErrorNone != error);
      fail_if (p_aud_hdrlst[i] == NULL);
      fail_if (aud_port_def.nBufferSize > p_aud_hdrlst[i]->nAllocLen);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_aud_hdrlst[%i] =  [%p]", i, p_aud_hdrlst[i]);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_aud_hdrlst[%d]->nAllocLen [%d]", i,
                 p_aud_hdrlst[i]->nAllocLen);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_aud_hdrlst[%d]->nFilledLen [%d]", i,
                 p_aud_hdrlst[i]->nFilledLen);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_aud_hdrlst[%d]->nOffset [%d]", i,
                 p_aud_hdrlst[i]->nOffset);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_aud_hdrlst[%d]->nOutputPortIndex [%d]", i,
                 p_aud_hdrlst[i]->nOutputPortIndex);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_aud_hdrlst[%d]->nInputPortIndex [%d]", i,
                 p_aud_hdrlst[i]->nInputPortIndex);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_aud_hdrlst[%d]->nFlags [%X]", i,
                 p_aud_hdrlst[i]->nFlags);

    }

  /* ---------------------- */
  /* Allocate video buffers */
  /* ---------------------- */
  p_vid_hdrlst = (OMX_BUFFERHEADERTYPE **)
    tiz_mem_calloc (vid_port_def.nBufferCountActual, sizeof (OMX_BUFFERHEADERTYPE *));

  for (i = 0; i < vid_port_def.nBufferCountActual; ++i)
    {
      error = OMX_AllocateBuffer (p_hdl, &p_vid_hdrlst[i], 1,    /* video port */
                                  0, vid_port_def.nBufferSize);
      fail_if (OMX_ErrorNone != error);
      fail_if (p_vid_hdrlst[i] == NULL);
      fail_if (vid_port_def.nBufferSize > p_vid_hdrlst[i]->nAllocLen);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_vid_hdrlst[%i] =  [%p]", i, p_vid_hdrlst[i]);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_vid_hdrlst[%d]->nAllocLen [%d]", i,
                 p_vid_hdrlst[i]->nAllocLen);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_vid_hdrlst[%d]->nFilledLen [%d]", i,
                 p_vid_hdrlst[i]->nFilledLen);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_vid_hdrlst[%d]->nOffset [%d]", i,
                 p_vid_hdrlst[i]->nOffset);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_vid_hdrlst[%d]->nOutputPortIndex [%d]", i,
                 p_vid_hdrlst[i]->nOutputPortIndex);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_vid_hdrlst[%d]->nInputPortIndex [%d]", i,
                 p_vid_hdrlst[i]->nInputPortIndex);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_vid_hdrlst[%d]->nFlags [%X]", i,
                 p_vid_hdrlst[i]->nFlags);
    }

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_ctx->state [%d] p_ctx->state [%s]", p_ctx->state,
           tiz_state_to_str (p_ctx->state));
  fail_if (OMX_StateIdle != p_ctx->state);

  /* ------------------------------ */
  /* Check state transition success */
  /* ------------------------------ */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateIdle != state);

  /* -------------------------- */
  /* Initiate transition to EXE */
  /* -------------------------- */
  error = _ctx_reset (&ctx);
  state = OMX_StateExecuting;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_ctx->state [%s]",
             tiz_state_to_str (p_ctx->state));
  fail_if (OMX_StateExecuting != p_ctx->state);

  for (i = 0; i < aud_port_def.nBufferCountActual; ++i)
    {
      tiz_vector_push_back (p_ctx->p_aud_hdrs, &(p_aud_hdrlst[i]));
    }

  for (i = 0; i < vid_port_def.nBufferCountActual; ++i)
    {
      tiz_vector_push_back (p_ctx->p_vid_hdrs, &(p_vid_hdrlst[i]));
    }

  /* -------------------- */
  /* buffer transfer loop */
  /* -------------------- */
  fail_if ((p_aud_file = fopen (pg_files[2], "w")) == 0);
  fail_if ((p_vid_file = fopen (pg_files[4], "w")) == 0);

  i = 0;
  while (true)
    {
      error = _ctx_reset (&ctx);
      fail_if (OMX_ErrorNone != error);

      /* ---------------------- */
      /* Transfer audio buffers */
      /* ---------------------- */
      if (!aud_eof)
        {
          error = transfer_buffers (&ctx, p_hdl, p_ctx->p_aud_hdrs);
          fail_if (OMX_ErrorNone != error);
        }

      /* --------------------- */
      /* Transfer video buffer */
      /* --------------------- */
      if (!vid_eof)
        {
          error = transfer_buffers (&ctx, p_hdl, p_ctx->p_vid_hdrs);
          fail_if (OMX_ErrorNone != error);
        }

      error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
      fail_if (OMX_ErrorNone != error);

      /* Write audio data to file */
      write_data_to_file (&ctx, p_ctx->p_aud_hdrs, p_aud_file,
                          &aud_eof, &aud_bytes_written, 2);
      /* Write video data to file */
      write_data_to_file (&ctx, p_ctx->p_vid_hdrs, p_vid_file,
                          &vid_eof, &vid_bytes_written, 4);
      if (aud_eof && vid_eof)
        {
          break;
        }
    }

  fclose (p_aud_file);
  fclose (p_vid_file);

  /* --------------------------- */
  /* Initiate transition to IDLE */
  /* --------------------------- */
  error = _ctx_reset (&ctx);
  state = OMX_StateIdle;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* Wait until all buffers are returned */
  while (true)
    {
      error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
      fail_if (OMX_ErrorNone != error);
      if (ready_to_stop (&ctx,
                         aud_port_def.nBufferCountActual,
                         vid_port_def.nBufferCountActual))
        {
          break;
        }
      error = _ctx_reset (&ctx);
      fail_if (OMX_ErrorNone != error);
    }

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  /* Lets poll for the event, to workaround the race condition that apperas
     with the context wait mechanism */
  idle_trasition_wait_count = 3;
  while (idle_trasition_wait_count--)
    {
      error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
      fail_if (OMX_ErrorNone != error);
      fail_if (OMX_TRUE == timedout);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_ctx->state [%s]",
               tiz_state_to_str (p_ctx->state));
      error = _ctx_reset (&ctx);
      if (OMX_StateIdle == p_ctx->state)
        {
          break;
        }
    }
  fail_if (OMX_StateIdle != p_ctx->state);

  /* ----------------------------- */
  /* Initiate transition to LOADED */
  /* ----------------------------- */
  error = _ctx_reset (&ctx);
  state = OMX_StateLoaded;
  error = OMX_SendCommand (p_hdl, cmd, state, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------ */
  /* Deallocate audio buffers */
  /* ------------------------ */
  fail_if (OMX_ErrorNone != error);
  for (i = 0; i < aud_port_def.nBufferCountActual; ++i)
    {
      error = OMX_FreeBuffer (p_hdl, 0,      /* audio port */
                              p_aud_hdrlst[i]);
      fail_if (OMX_ErrorNone != error);
    }

  /* ------------------------ */
  /* Deallocate video buffers */
  /* ------------------------ */
  fail_if (OMX_ErrorNone != error);
  for (i = 0; i < vid_port_def.nBufferCountActual; ++i)
    {
      error = OMX_FreeBuffer (p_hdl, 1,      /* video port */
                              p_vid_hdrlst[i]);
      fail_if (OMX_ErrorNone != error);
    }

  /* ------------------------- */
  /* Await transition callback */
  /* ------------------------- */
  error = _ctx_wait (&ctx, TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateLoaded != p_ctx->state);

  /* ------------------------------ */
  /* Check state transition success */
  /* ------------------------------ */
  error = OMX_GetState (p_hdl, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "state [%s]", tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  error = OMX_FreeHandle (p_hdl);
  fail_if (OMX_ErrorNone != error);

  cmp_outcome = compare_files (pg_files[1], pg_files[2]);
  cmp_outcome &= compare_files (pg_files[3], pg_files[4]);

  tiz_mem_free (p_aud_hdrlst);
  tiz_mem_free (p_vid_hdrlst);
  tiz_mem_free (p_uri_param);

  error = OMX_Deinit ();
  fail_if (OMX_ErrorNone != error);

  /* TODO: Do not enable this check for NOW */
  /*   fail_if (!cmp_outcome); */

  _ctx_destroy (&ctx);
}
END_TEST

Suite *
od_suite (void)
{
  TCase *tc_oggdmux;
  Suite *s = suite_create ("libtizoggdemux");

  /* test case */
  tc_oggdmux = tcase_create ("Ogg demuxer component");
  tcase_add_unchecked_fixture (tc_oggdmux, setup, teardown);
  tcase_set_timeout (tc_oggdmux, OGG_DEMUXER_TEST_TIMEOUT);
  tcase_add_test (tc_oggdmux, test_ogg_demuxer);
  suite_add_tcase (s, tc_oggdmux);

  return s;
}

int
main (void)
{
  int number_failed;
  SRunner *sr = srunner_create (od_suite ());

  tiz_log_init();

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Tizonia OpenMAX IL - Ogg demuxer component unit tests");

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
