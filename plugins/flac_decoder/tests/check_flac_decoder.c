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
 * @file   check_flac_decoder.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Flac Decoder unit tests
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <check.h>
#include <limits.h>

#include "OMX_Component.h"
#include "OMX_Types.h"
#include "OMX_TizoniaExt.h"

#include "tizplatform.h"
#include "tizfsm.h"
#include "tizkernel.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.flac_decoder.check"
#endif

char *pg_rmd_path;
pid_t g_rmd_pid;

static char OGG_DMUX_COMPONENT_NAME[] = "OMX.Aratelia.container_demuxer.ogg";
static char FLAC_DEC_COMPONENT_NAME[] = "OMX.Aratelia.audio_decoder.flac";
static char PCM_RND_COMPONENT_NAME[] = "OMX.Aratelia.audio_renderer.alsa.pcm";

/* TODO: Move these two to the rc file */
#define RATE_FILE1 44100
#define RATE_FILE2 44100

#define FLAC_DECODER_TEST_TIMEOUT 360
#define INFINITE_WAIT 0xffffffff
/* duration of event timeout in msec when we expect event to be set */
#define TIMEOUT_EXPECTING_SUCCESS 1000
/* duration of event timeout in msec when we expect buffer to be consumed */
#define TIMEOUT_EXPECTING_SUCCESS_BUFFER_TRANSFER 5000
/* duration of event timeout in msec when we don't expect event to be set */
#define TIMEOUT_EXPECTING_FAILURE 2000
#define TIMEOUT_EXPECTING_STREAM_TRANSFER 60000000

typedef void *cc_ctx_t;

static const char *pg_files[] = {
  NULL,
  NULL
};

static const OMX_U32 pg_rates[] = {
  RATE_FILE1,
  RATE_FILE2
};

static const char *pg_cnames[] = {
  OGG_DMUX_COMPONENT_NAME, FLAC_DEC_COMPONENT_NAME, PCM_RND_COMPONENT_NAME
};

static OMX_HANDLETYPE pg_hdls[] = {
  NULL, NULL, NULL
};

static cc_ctx_t pg_ctxs[] = {
  NULL, NULL, NULL
};

#define MAX_EVENTS 4
static OMX_EVENTTYPE pg_events[] = {
  OMX_EventCmdComplete,
  OMX_EventPortSettingsChanged,
  OMX_EventBufferFlag,
  OMX_EventVendorStartUnused    /* This will be used for EmptyBufferDone
                                   events */
};

typedef struct check_common_context check_common_context_t;
struct check_common_context
{
  tiz_mutex_t mutex;
  tiz_cond_t cond;
  OMX_STATETYPE state;
  OMX_BUFFERHEADERTYPE *p_hdr;
  OMX_U32 flags;
  OMX_U32 port;
  OMX_U32 index;
  OMX_BOOL signaled[MAX_EVENTS];         /* We'll be waiting for MAX_EVENTS
                                            different events */
  OMX_EVENTTYPE event[MAX_EVENTS];
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

static const char *
hdl2cname(OMX_HANDLETYPE hdl)
{
  int i;
  for (i = 0; i < sizeof(pg_hdls); i++)
    {
      if (hdl == pg_hdls[i])
        {
          return pg_cnames[i];
        }
    }
  assert(0);
  return NULL;
}

static const char *
ctx2cname(cc_ctx_t ctx)
{
  int i;
  for (i = 0; i < sizeof(pg_ctxs); i++)
    {
      if (ctx == pg_ctxs[i])
        {
          return pg_cnames[i];
        }
    }
  assert(0);
  return NULL;
}

static const int
event2signal(OMX_EVENTTYPE event)
{
  int i;
  for (i = 0; i < sizeof(pg_events); i++)
    {
      if (event == pg_events[i])
        {
          return i;
        }
    }
  assert(0);
  return -1;
}

static OMX_ERRORTYPE
_ctx_init (cc_ctx_t * app_ctx)
{
  int i;
  check_common_context_t *p_ctx =
    tiz_mem_calloc (1, sizeof (check_common_context_t));

  if (!p_ctx)
    {
      return OMX_ErrorInsufficientResources;
    }

  for (i=0 ; i < MAX_EVENTS ; i++)
    {
      p_ctx->signaled[i] = OMX_FALSE;
      p_ctx->event[i] = OMX_EventMax;
    }

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
  p_ctx->p_hdr = NULL;
  p_ctx->flags = 0;
  p_ctx->port  = 0;
  p_ctx->index = 0;

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
  p_ctx->cond = NULL;
  tiz_mutex_unlock (&p_ctx->mutex);
  tiz_mutex_destroy (&p_ctx->mutex);
  p_ctx->mutex = NULL;

  tiz_mem_free (p_ctx);

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
_ctx_signal (cc_ctx_t * app_ctx, OMX_EVENTTYPE event)
{
  check_common_context_t *p_ctx = NULL;
  assert (app_ctx);
  assert (-1 != event2signal(event));
  p_ctx = * app_ctx;

  if (tiz_mutex_lock (&p_ctx->mutex))
    {
      return OMX_ErrorBadParameter;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Context [%s] has been signalled [%s]",
             ctx2cname(p_ctx), tiz_evt_to_str(event));

  p_ctx->signaled[event2signal(event)] = OMX_TRUE;
  p_ctx->event[event2signal(event)] = event;

  tiz_cond_signal (&p_ctx->cond);
  tiz_mutex_unlock (&p_ctx->mutex);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
_ctx_wait (cc_ctx_t * app_ctx, OMX_EVENTTYPE event,
           OMX_U32 a_millis, OMX_BOOL * ap_has_timedout)
{
  OMX_ERRORTYPE retcode = OMX_ErrorNone;
  check_common_context_t *p_ctx = NULL;

  assert (app_ctx);
  assert (-1 != event2signal(event));

  p_ctx = * app_ctx;

  * ap_has_timedout = OMX_FALSE;

  if (tiz_mutex_lock (&p_ctx->mutex))
    {
      return OMX_ErrorBadParameter;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Waiting for [%s] a_millis [%u] signaled [%s]",
             ctx2cname(p_ctx), a_millis,
             p_ctx->signaled[event2signal(event)] ? "OMX_TRUE" : "OMX_FALSE");

  if (0 == a_millis)
    {
      if (!p_ctx->signaled[event2signal(event)])
        {
          * ap_has_timedout = OMX_TRUE;
        }
    }

  else if (INFINITE_WAIT == a_millis)
    {
      while (!p_ctx->signaled[event2signal(event)])
        {
          tiz_cond_wait (&p_ctx->cond, &p_ctx->mutex);
        }
    }

  else
    {
      while (!p_ctx->signaled[event2signal(event)])
        {
          retcode = tiz_cond_timedwait (&p_ctx->cond,
                                          &p_ctx->mutex, a_millis);

          /* TODO: Change this to OMX_ErrorTimeout */
          if (retcode == OMX_ErrorUndefined 
              && !p_ctx->signaled[event2signal(event)])
            {
              TIZ_LOG (TIZ_PRIORITY_TRACE, "Waiting for [%s] - "
                         "timeout occurred", ctx2cname(p_ctx));
              * ap_has_timedout = OMX_TRUE;
              break;
            }
        }
    }

  tiz_mutex_unlock (&p_ctx->mutex);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
_ctx_reset (cc_ctx_t * app_ctx, OMX_EVENTTYPE event)
{
  check_common_context_t *p_ctx = NULL;
  assert (app_ctx);
  assert (-1 != event2signal(event));
  p_ctx = * app_ctx;

  if (tiz_mutex_lock (&p_ctx->mutex))
    {
      return OMX_ErrorBadParameter;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Resetting [%s] event [%s] ",
             ctx2cname(p_ctx), tiz_evt_to_str(event));

  p_ctx->signaled[event2signal(event)] = OMX_FALSE;
  p_ctx->event[event2signal(event)] = OMX_StateMax;

  if (OMX_EventCmdComplete == event)
    {
      p_ctx->state = OMX_StateMax;
    }

  if (OMX_EventVendorStartUnused == event)
    {
      p_ctx->p_hdr = NULL;
    }

  if (OMX_EventBufferFlag == event)
    {
      p_ctx->flags = 0;
    }

  if (OMX_EventPortSettingsChanged == event)
    {
      p_ctx->port  = 0;
      p_ctx->index = 0;
    }

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
  const char *p_cname = NULL;

  fail_if(!(p_cname = hdl2cname(ap_hdl)));

  if (OMX_EventCmdComplete == eEvent)
    {
      switch ((OMX_COMMANDTYPE) (nData1))
        {
        case OMX_CommandStateSet:
          {
            TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_CommandStateSet : "
                       "Component transitioned to [%s]",
                       p_cname,
                       tiz_state_to_str ((OMX_STATETYPE) (nData2)));
            p_ctx->state = (OMX_STATETYPE) (nData2);
            _ctx_signal (pp_ctx, OMX_EventCmdComplete);
            break;
          }

        case OMX_CommandPortDisable:
          {
            TIZ_LOG (TIZ_PRIORITY_TRACE, "Port [%d] is now DISABLED", nData2);
            _ctx_signal (pp_ctx, OMX_EventCmdComplete);
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
                     p_cname, nData1);
          p_ctx->flags = nData2;
          _ctx_signal (pp_ctx, OMX_EventBufferFlag);
        }
      else
        {
          fail_if (0);
        }
    }

  if (OMX_EventPortSettingsChanged == eEvent)
    {
      p_ctx->port = nData1;
      p_ctx->index = nData2;
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Received OMX_EventPortSettingsChanged "
               "from [%s] port[%i] index [%s]",
               p_cname, nData1, tiz_idx_to_str (nData2));

      _ctx_signal (pp_ctx, OMX_EventPortSettingsChanged);
    }

  return OMX_ErrorNone;

}

OMX_ERRORTYPE check_EmptyBufferDone
  (OMX_HANDLETYPE ap_hdl,
   OMX_PTR ap_app_data, OMX_BUFFERHEADERTYPE * ap_buf)
{
  check_common_context_t *p_ctx = NULL;
  cc_ctx_t *pp_ctx = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "EmptyBufferDone from [%s]: BUFFER [%p]",
             hdl2cname(ap_hdl), ap_buf);

  assert (ap_app_data);
  assert (ap_buf);
  pp_ctx = (cc_ctx_t *) ap_app_data;
  p_ctx = *pp_ctx;

  p_ctx->p_hdr = ap_buf;
  _ctx_signal (pp_ctx, OMX_EventVendorStartUnused);

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

static bool
init_test_data()
{
  bool rv = false;
  const char *p_testfile1 = NULL;
  const char *p_testfile2 = NULL;

  p_testfile1 = tiz_rcfile_get_value("plugins-data",
                                     "OMX.Aratelia.audio_decoder.flac.testfile1_uri");

  p_testfile2 = tiz_rcfile_get_value("plugins-data",
                                     "OMX.Aratelia.audio_decoder.flac.testfile2_uri");

  if (!p_testfile1 || !p_testfile2)

    {
      TIZ_LOG(TIZ_PRIORITY_TRACE, "Test data not available...");
    }
  else
    {
      pg_files[0] = p_testfile1; pg_files[1] = p_testfile2;
      TIZ_LOG(TIZ_PRIORITY_TRACE, "Test data available [%s]", pg_files[0]);
      TIZ_LOG(TIZ_PRIORITY_TRACE, "Test data available [%s]", pg_files[1]);
      rv = true;
    }

  return rv;
}

/*
 * Unit tests
 */

START_TEST (test_flac_playback)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_oggdmux = NULL, p_flacdec = NULL, p_pcmrnd = NULL;
  OMX_STATETYPE state = OMX_StateMax;
  cc_ctx_t dmux_ctx, dec_ctx, rend_ctx;
  check_common_context_t *p_dmux_ctx = NULL, *p_dec_ctx = NULL, *p_rend_ctx = NULL;
  OMX_BOOL timedout = OMX_FALSE;
  OMX_PARAM_PORTDEFINITIONTYPE dmux_port_def0, dec_port_def0, dec_port_def1, rend_port_def;
  OMX_TIZONIA_AUDIO_PARAM_FLACTYPE dec_flac_type;
  OMX_AUDIO_PARAM_PCMMODETYPE dec_pcm_mode, rend_pcm_mode;
  OMX_PARAM_BUFFERSUPPLIERTYPE supplier;
  OMX_PARAM_CONTENTURITYPE *p_uri_param = NULL;

  fail_if (!init_test_data());

  error = _ctx_init (&dmux_ctx);
  fail_if (OMX_ErrorNone != error);

  error = _ctx_init (&dec_ctx);
  fail_if (OMX_ErrorNone != error);

  error = _ctx_init (&rend_ctx);
  fail_if (OMX_ErrorNone != error);

  p_dmux_ctx = (check_common_context_t *) (dmux_ctx);
  p_dec_ctx = (check_common_context_t *) (dec_ctx);
  p_rend_ctx = (check_common_context_t *) (rend_ctx);

  pg_ctxs[0] = p_dmux_ctx;
  pg_ctxs[1] = p_dec_ctx;
  pg_ctxs[2] = p_rend_ctx;

  error = OMX_Init ();
  fail_if (OMX_ErrorNone != error);

  /* --------------------------- */
  /* Instantiate the ogg demuxer */
  /* --------------------------- */
  error = OMX_GetHandle (&p_oggdmux, OGG_DMUX_COMPONENT_NAME, (OMX_PTR *) (&dmux_ctx),
                         &_check_cbacks);
  fail_if (OMX_ErrorNone != error);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] hdl [%p]", OGG_DMUX_COMPONENT_NAME, p_oggdmux);
  pg_hdls[0] = p_oggdmux;

  /* --------------------------- */
  /* Instantiate the flac decoder */
  /* --------------------------- */
  error = OMX_GetHandle (&p_flacdec, FLAC_DEC_COMPONENT_NAME, (OMX_PTR *) (&dec_ctx),
                         &_check_cbacks);
  fail_if (OMX_ErrorNone != error);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] hdl [%p]", FLAC_DEC_COMPONENT_NAME, p_flacdec);
  pg_hdls[1] = p_flacdec;

  /* ---------------------------- */
  /* Instantiate the pcm renderer */
  /* ---------------------------- */
  error = OMX_GetHandle (&p_pcmrnd, PCM_RND_COMPONENT_NAME, (OMX_PTR *) (&rend_ctx),
                         &_check_cbacks);
  fail_if (OMX_ErrorNone != error);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] hdl [%p]", PCM_RND_COMPONENT_NAME, p_pcmrnd);
  pg_hdls[2] = p_pcmrnd;;

  /* ---------------------------------------------- */
  /* Obtain the port def from the demuxer's port #0 */
  /* ---------------------------------------------- */
  dmux_port_def0.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  dmux_port_def0.nVersion.nVersion = OMX_VERSION;
  dmux_port_def0.nPortIndex = 0;
  error = OMX_GetParameter (p_oggdmux, OMX_IndexParamPortDefinition, &dmux_port_def0);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] port #0 nBufferSize [%d]",
             OGG_DMUX_COMPONENT_NAME, dmux_port_def0.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] port #0 nBufferCountActual [%d]",
             OGG_DMUX_COMPONENT_NAME, dmux_port_def0.nBufferCountActual);

  /* ---------------------------------------------- */
  /* Obtain the port def from the decoder's port #0 */
  /* ---------------------------------------------- */
  dec_port_def0.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  dec_port_def0.nVersion.nVersion = OMX_VERSION;
  dec_port_def0.nPortIndex = 0;
  error = OMX_GetParameter (p_flacdec, OMX_IndexParamPortDefinition, &dec_port_def0);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] port #0 nBufferSize [%d]",
             FLAC_DEC_COMPONENT_NAME, dec_port_def0.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] port #0 nBufferCountActual [%d]",
             FLAC_DEC_COMPONENT_NAME, dec_port_def0.nBufferCountActual);

  /* --------------------------------------------------------- */
  /* Set the same nBufferCountActual value for dmux and decoder */
  /* --------------------------------------------------------- */
  dmux_port_def0.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  dmux_port_def0.nVersion.nVersion = OMX_VERSION;
  dmux_port_def0.nBufferCountActual = dec_port_def0.nBufferCountActual;
  error = OMX_SetParameter (p_oggdmux, OMX_IndexParamPortDefinition, &dmux_port_def0);
  fail_if (OMX_ErrorNone != error);

  /* ---------------------------------------------- */
  /* Obtain the port def from the decoder's port #1 */
  /* ---------------------------------------------- */
  dec_port_def1.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  dec_port_def1.nVersion.nVersion = OMX_VERSION;
  dec_port_def1.nPortIndex = 1;
  error = OMX_GetParameter (p_flacdec, OMX_IndexParamPortDefinition, &dec_port_def1);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] port #1 nBufferSize [%d]",
             FLAC_DEC_COMPONENT_NAME, dec_port_def1.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] port #1 nBufferCountActual [%d]",
             FLAC_DEC_COMPONENT_NAME, dec_port_def1.nBufferCountActual);

  /* ----------------------------------------------- */
  /* Obtain the port def from the renderer's port #0 */
  /* ----------------------------------------------- */
  rend_port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  rend_port_def.nVersion.nVersion = OMX_VERSION;
  rend_port_def.nPortIndex = 0;
  error = OMX_GetParameter (p_pcmrnd, OMX_IndexParamPortDefinition,
                            &rend_port_def);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] nBufferSize [%d]",
             PCM_RND_COMPONENT_NAME, rend_port_def.nBufferSize);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] nBufferCountActual [%d]",
             PCM_RND_COMPONENT_NAME, rend_port_def.nBufferCountActual);

  /* -------------------------------------------------- */
  /* Obtain the flac settings from the decoder's port #0 */
  /* -------------------------------------------------- */
  dec_flac_type.nSize = sizeof (OMX_TIZONIA_AUDIO_PARAM_FLACTYPE);
  dec_flac_type.nVersion.nVersion = OMX_VERSION;
  dec_flac_type.nPortIndex = 0;
  error = OMX_GetParameter (p_flacdec, OMX_TizoniaIndexParamAudioFlac, &dec_flac_type);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------------------------------- */
  /* Obtain the pcm settings from the decoders's port #1 */
  /* --------------------------------------------------- */
  dec_pcm_mode.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  dec_pcm_mode.nVersion.nVersion = OMX_VERSION;
  dec_pcm_mode.nPortIndex = 1;
  error = OMX_GetParameter (p_flacdec, OMX_IndexParamAudioPcm, &dec_pcm_mode);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------------------------------- */
  /* Obtain the pcm settings from the renderer's port #0 */
  /* --------------------------------------------------- */
  rend_pcm_mode.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  rend_pcm_mode.nVersion.nVersion = OMX_VERSION;
  rend_pcm_mode.nPortIndex = 0;
  error = OMX_GetParameter (p_pcmrnd, OMX_IndexParamAudioPcm, &rend_pcm_mode);
  fail_if (OMX_ErrorNone != error);

  /* ----------------------------------------------------- */
  /* Attempt to set the sampling rate on decoder's port #1 */
  /* ----------------------------------------------------- */
  /* NOTE : This must fail as the port #1 is configured as slave */
  dec_pcm_mode.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  dec_pcm_mode.nVersion.nVersion = OMX_VERSION;
  dec_pcm_mode.nPortIndex = 1;
  dec_pcm_mode.nChannels = 2;
  dec_pcm_mode.nBitPerSample = 16;
  dec_pcm_mode.nSamplingRate = pg_rates[_i];
  error = OMX_SetParameter (p_flacdec, OMX_IndexParamAudioPcm, &dec_pcm_mode);
  fail_if (OMX_ErrorBadParameter != error);

  /* ------------------------------------------ */
  /* Set the flac settings on decoder's port #0  */
  /* ------------------------------------------ */
  error = _ctx_reset (&dec_ctx, OMX_EventPortSettingsChanged);
  dec_flac_type.nSize = sizeof (OMX_TIZONIA_AUDIO_PARAM_FLACTYPE);
  dec_flac_type.nVersion.nVersion = OMX_VERSION;
  dec_flac_type.nPortIndex = 0;
  dec_flac_type.nChannels = 2;
  dec_flac_type.nBitsPerSample = 16;
  dec_flac_type.nSampleRate = pg_rates[_i];
  dec_flac_type.nCompressionLevel = 5;
  dec_flac_type.nBlockSize = 0;
  dec_flac_type.nTotalSamplesEstimate = 0;
  dec_flac_type.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  error = OMX_SetParameter (p_flacdec, OMX_TizoniaIndexParamAudioFlac, &dec_flac_type);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] : OMX_SetParameter(port #0, "
           "OMX_TizoniaIndexParamAudioFlac) = [%s]", FLAC_DEC_COMPONENT_NAME,
           tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

  /* If the sampling rate of the input stream is different from the default
     pcm sampling rate of the output port (i.e. 48000hz), then an event
     shall be received from the output port */

  if (48000 != pg_rates[_i])
    {
      /* ------------------------------------------------------- */
      /* Await port settings change event on decoders's port #1  */
      /* ------------------------------------------------------- */

      error = _ctx_wait (&dec_ctx, OMX_EventPortSettingsChanged,
                         TIMEOUT_EXPECTING_SUCCESS, &timedout);
      fail_if (OMX_ErrorNone != error);
      fail_if (OMX_TRUE == timedout);
      fail_if (1 != p_dec_ctx->port);
      fail_if (OMX_IndexParamAudioPcm != p_dec_ctx->index);
    }

  /* -------------------------------------------------- */
  /* Verify the new sampling rate on decoder's port #1 */
  /* -------------------------------------------------- */
  error = OMX_GetParameter (p_flacdec, OMX_IndexParamAudioPcm, &dec_pcm_mode);
  fail_if (dec_pcm_mode.nSamplingRate != pg_rates[_i]);

  /* ------------------------------------------ */
  /* Set the pcm settings on renderer's port #0 */
  /* ------------------------------------------ */
  rend_pcm_mode.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  rend_pcm_mode.nVersion.nVersion = OMX_VERSION;
  rend_pcm_mode.nPortIndex = 0;
  rend_pcm_mode.nChannels = 2;
  rend_pcm_mode.eNumData = OMX_NumericalDataUnsigned;
  rend_pcm_mode.eEndian = OMX_EndianLittle;
  rend_pcm_mode.bInterleaved = OMX_TRUE;
  rend_pcm_mode.nBitPerSample = 16;
  rend_pcm_mode.nSamplingRate = pg_rates[_i];
  rend_pcm_mode.ePCMMode = OMX_AUDIO_PCMModeMULaw;
  rend_pcm_mode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  rend_pcm_mode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
  error = OMX_SetParameter (p_pcmrnd, OMX_IndexParamAudioPcm, &rend_pcm_mode);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------------------------ */
  /* Set supplier settings on decoder's port #0 */
  /* ------------------------------------------ */
  supplier.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
  supplier.nVersion.nVersion = OMX_VERSION;
  supplier.nPortIndex = 0;
  supplier.eBufferSupplier = OMX_BufferSupplyInput;
  error = OMX_SetParameter (p_flacdec, OMX_IndexParamCompBufferSupplier,
                            &supplier);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_BufferSupplyInput [%s]",
             FLAC_DEC_COMPONENT_NAME, tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* ------------------------------------------ */
  /* Set supplier settings on demuxers's port #0 */
  /* ------------------------------------------ */
  supplier.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
  supplier.nVersion.nVersion = OMX_VERSION;
  supplier.nPortIndex = 0;
  supplier.eBufferSupplier = OMX_BufferSupplyInput;
  error = OMX_SetParameter (p_oggdmux, OMX_IndexParamCompBufferSupplier,
                            &supplier);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_BufferSupplyInput [%s]",
             OGG_DMUX_COMPONENT_NAME, tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* ------------------------------------------ */
  /* Set supplier settings on renderer's port #0 */
  /* ------------------------------------------ */
  supplier.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
  supplier.nVersion.nVersion = OMX_VERSION;
  supplier.nPortIndex = 0;
  supplier.eBufferSupplier = OMX_BufferSupplyInput;
  error = OMX_SetParameter (p_pcmrnd, OMX_IndexParamCompBufferSupplier,
                            &supplier);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_BufferSupplyInput [%s]",
             PCM_RND_COMPONENT_NAME, tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* ------------------------------------------ */
  /* Set supplier settings on decoder's port #1 */
  /* ------------------------------------------ */
  supplier.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
  supplier.nVersion.nVersion = OMX_VERSION;
  supplier.nPortIndex = 1;
  supplier.eBufferSupplier = OMX_BufferSupplyInput;
  error = OMX_SetParameter (p_flacdec, OMX_IndexParamCompBufferSupplier,
                            &supplier);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_BufferSupplyInput [%s]",
             FLAC_DEC_COMPONENT_NAME, tiz_err_to_str(error));
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
  error = OMX_GetParameter (p_oggdmux, OMX_IndexParamContentURI,
                            p_uri_param);
  fail_if (OMX_ErrorNone != error);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Retrieved URI [%s]", p_uri_param->contentURI);

  /* ----------------*/
  /* Set the new URI */
  /* ----------------*/
  strcpy ((char*)p_uri_param->contentURI, pg_files[0]);
  p_uri_param->contentURI[strlen (pg_files[0])] = '\0';
  error = OMX_SetParameter (p_oggdmux, OMX_IndexParamContentURI, p_uri_param);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_SetParameter(OMX_IndexParamContentURI, "
           "URI [%s]) = [%s]", p_uri_param->contentURI, tiz_err_to_str (error));
  fail_if (OMX_ErrorNone != error);

  /* --------------------------------- */
  /* Create Tunnel Demuxer <-> Decoder */
  /* --------------------------------- */
  error = OMX_SetupTunnel(p_oggdmux, 0, p_flacdec, 0);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_SetupTunnel [%s]",
           tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* ----------------------------------- */
  /* Create Tunnel Decoder <-> Renderer*/
  /* ----------------------------------- */
  error = OMX_SetupTunnel(p_flacdec, 1, p_pcmrnd, 0);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_SetupTunnel [%s]",
           tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* ----------------------------------------- */
  /* Disable video output port (port index 1)  */
  /* ----------------------------------------- */
  error = _ctx_reset (&dmux_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_oggdmux, OMX_CommandPortDisable, 1, NULL);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------- */
  /* Await port disable callback */
  /* --------------------------- */
  error = _ctx_wait (&dmux_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);

  /* ------------------------------------- */
  /* Initiate demuxer's transition to IDLE */
  /* ------------------------------------- */
  error = _ctx_reset (&dmux_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_oggdmux, OMX_CommandStateSet,
                           OMX_StateIdle, NULL);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_StateIdle [%s]",
             OGG_DMUX_COMPONENT_NAME, tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* ------------------------------------- */
  /* Initiate decoder's transition to IDLE */
  /* ------------------------------------- */
  error = _ctx_reset (&dec_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_flacdec, OMX_CommandStateSet,
                           OMX_StateIdle, NULL);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_StateIdle [%s]",
             FLAC_DEC_COMPONENT_NAME, tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* -------------------------------------- */
  /* Initiate renderer's transition to IDLE */
  /* -------------------------------------- */
  error = _ctx_reset (&rend_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_pcmrnd, OMX_CommandStateSet,
                           OMX_StateIdle, NULL);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] OMX_StateIdle [%s]",
             PCM_RND_COMPONENT_NAME, tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  /* -------------------------------------------------- */
  /* Await demuxer's transition callback OMX_StateIdle */
  /* -------------------------------------------------- */
  error = _ctx_wait (&dmux_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_dmux_ctx->state [%s]",
             tiz_state_to_str (p_dmux_ctx->state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateIdle != p_dmux_ctx->state);

  /* ----------------------------------------- */
  /* Check demuxer's state transition success */
  /* ----------------------------------------- */
  error = OMX_GetState (p_oggdmux, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] state [%s]",
             OGG_DMUX_COMPONENT_NAME, tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateIdle != state);

  /* -------------------------------------------------- */
  /* Await renderer's transition callback OMX_StateIdle */
  /* -------------------------------------------------- */
  error = _ctx_wait (&rend_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_rend_ctx->state [%s]",
           tiz_state_to_str (p_rend_ctx->state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateIdle != p_rend_ctx->state);

  /* ----------------------------------------- */
  /* Check renderer's state transition success */
  /* ----------------------------------------- */
  error = OMX_GetState (p_pcmrnd, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] state [%s]",
             PCM_RND_COMPONENT_NAME, tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateIdle != state);

  /* ------------------------------------------------- */
  /* Await decoder's transition callback OMX_StateIdle */
  /* ------------------------------------------------- */
  error = _ctx_wait (&dec_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_dec_ctx->state [%s]",
             tiz_state_to_str (p_dec_ctx->state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateIdle != p_dec_ctx->state);

  /* ----------------------------------------- */
  /* Check decoder's state transition success */
  /* ----------------------------------------- */
  error = OMX_GetState (p_flacdec, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] state [%s]",
             FLAC_DEC_COMPONENT_NAME, tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateIdle != state);

  /* ------------------------------------ */
  /* Initiate demuxer's transition to EXE */
  /* ------------------------------------ */
  error = _ctx_reset (&dmux_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_oggdmux, OMX_CommandStateSet,
                           OMX_StateExecuting, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------------------ */
  /* Initiate decoder's transition to EXE */
  /* ------------------------------------ */
  error = _ctx_reset (&dec_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_flacdec, OMX_CommandStateSet,
                           OMX_StateExecuting, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------------------ */
  /* Initiate renderer's transition to EXE */
  /* ------------------------------------ */
  error = _ctx_reset (&rend_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_pcmrnd, OMX_CommandStateSet,
                           OMX_StateExecuting, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------------------------------------ */
  /* Await demuxer's transition callback OMX_StateExecuting */
  /* ------------------------------------------------------ */
  error = _ctx_wait (&dmux_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_dmux_ctx->state [%s]",
             tiz_state_to_str (p_dmux_ctx->state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateExecuting != p_dmux_ctx->state);

  /* ----------------------------------------- */
  /* Check demuxer's state transition success */
  /* ----------------------------------------- */
  state = OMX_StateMax;
  error = OMX_GetState (p_oggdmux, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] state [%s]",
             OGG_DMUX_COMPONENT_NAME, tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateExecuting != state);

  /* ------------------------------------------------------ */
  /* Await decoder's transition callback OMX_StateExecuting */
  /* ------------------------------------------------------ */
  error = _ctx_wait (&dec_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_dec_ctx->state [%s]",
             tiz_state_to_str (p_dec_ctx->state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateExecuting != p_dec_ctx->state);

  /* ----------------------------------------- */
  /* Check decoder's state transition success */
  /* ----------------------------------------- */
  state = OMX_StateMax;
  error = OMX_GetState (p_flacdec, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] state [%s]",
             FLAC_DEC_COMPONENT_NAME, tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateExecuting != state);

  /* ------------------------------------------------------- */
  /* Await renderer's transition callback OMX_StateExecuting */
  /* ------------------------------------------------------- */
  error = _ctx_wait (&rend_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_rend_ctx->state [%s]",
             tiz_state_to_str (p_rend_ctx->state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateExecuting != p_rend_ctx->state);

  /* ----------------------------------------- */
  /* Check renderer's state transition success */
  /* ----------------------------------------- */
  error = OMX_GetState (p_pcmrnd, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] state [%s] [%s]",
           PCM_RND_COMPONENT_NAME, tiz_state_to_str (state),
           tiz_err_to_str (error));
  fflush (stdout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateExecuting != state);

  /* -------------------------------------- */
  /* Wait for EOS flag from flac decoder     */
  /* -------------------------------------- */
  if (!p_dec_ctx->flags)
    {
      error = _ctx_wait (&dec_ctx, OMX_EventBufferFlag,
                         TIMEOUT_EXPECTING_STREAM_TRANSFER,
                         &timedout);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_dec_ctx->flags [%X]",
                 p_dec_ctx->flags);
      fail_if (OMX_ErrorNone != error);
      fail_if (!(p_dec_ctx->flags & OMX_BUFFERFLAG_EOS));
      if (!(p_dec_ctx->flags & OMX_BUFFERFLAG_EOS))
        {
          fail_if (OMX_TRUE == timedout);
        }

    }

  /* -------------------------------------- */
  /* Wait for EOS flag from pcm renderer    */
  /* -------------------------------------- */
  if (!p_rend_ctx->flags)
    {
      error = _ctx_wait (&rend_ctx, OMX_EventBufferFlag,
                         TIMEOUT_EXPECTING_SUCCESS, &timedout);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "p_rend_ctx->flags [%X]",
                 p_rend_ctx->flags);
      fail_if (OMX_ErrorNone != error);
      fail_if (!(p_rend_ctx->flags & OMX_BUFFERFLAG_EOS));
      if (!(p_rend_ctx->flags & OMX_BUFFERFLAG_EOS))
        {
          fail_if (OMX_TRUE == timedout);
        }
    }

  /* ------------------------------------- */
  /* Initiate demuxer's transition to IDLE */
  /* ------------------------------------- */
  error = _ctx_reset (&dmux_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_oggdmux, OMX_CommandStateSet,
                           OMX_StateIdle, NULL);
  fail_if (OMX_ErrorNone != error);

  /* -------------------------------------- */
  /* Initiate renderer's transition to IDLE */
  /* -------------------------------------- */
  error = _ctx_reset (&rend_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_pcmrnd, OMX_CommandStateSet,
                           OMX_StateIdle, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------------------- */
  /* Initiate decoder's transition to IDLE */
  /* ------------------------------------- */
  error = _ctx_reset (&dec_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_flacdec, OMX_CommandStateSet,
                           OMX_StateIdle, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------------------------- */
  /* Await demuxer's transition callback to IDLE */
  /* ------------------------------------------- */
  error = _ctx_wait (&dmux_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_dmux_ctx->state [%s]",
             tiz_state_to_str (p_dmux_ctx->state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateIdle != p_dmux_ctx->state);

  /* ----------------------------------------- */
  /* Check demuxer's state transition success  */
  /* ----------------------------------------- */
  error = OMX_GetState (p_oggdmux, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] state [%s]",
             OGG_DMUX_COMPONENT_NAME, tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateIdle != state);

  /* ------------------------------------------- */
  /* Await decoder's transition callback to IDLE */
  /* ------------------------------------------- */
  error = _ctx_wait (&dec_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_dec_ctx->state [%s]",
             tiz_state_to_str (p_dec_ctx->state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateIdle != p_dec_ctx->state);

  /* ----------------------------------------- */
  /* Check decoder's state transition success */
  /* ----------------------------------------- */
  error = OMX_GetState (p_flacdec, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] state [%s]",
             FLAC_DEC_COMPONENT_NAME, tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateIdle != state);

  /* -------------------------------------------- */
  /* Await renderer's transition callback to IDLE */
  /* -------------------------------------------- */
  error = _ctx_wait (&rend_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_rend_ctx->state [%s]",
             tiz_state_to_str (p_rend_ctx->state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  fail_if (OMX_StateIdle != p_rend_ctx->state);

  /* ----------------------------------------- */
  /* Check renderer's state transition success */
  /* ----------------------------------------- */
  error = OMX_GetState (p_pcmrnd, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] state [%s]",
             PCM_RND_COMPONENT_NAME, tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateIdle != state);

  /* --------------------------------------- */
  /* Initiate demuxer's transition to LOADED */
  /* --------------------------------------- */
  error = _ctx_reset (&dmux_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_oggdmux, OMX_CommandStateSet,
                           OMX_StateLoaded, NULL);
  fail_if (OMX_ErrorNone != error);

  /* --------------------------------------- */
  /* Initiate decoder's transition to LOADED */
  /* --------------------------------------- */
  error = _ctx_reset (&dec_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_flacdec, OMX_CommandStateSet,
                           OMX_StateLoaded, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ---------------------------------------- */
  /* Initiate renderer's transition to LOADED */
  /* ---------------------------------------- */
  error = _ctx_reset (&rend_ctx, OMX_EventCmdComplete);
  error = OMX_SendCommand (p_pcmrnd, OMX_CommandStateSet,
                           OMX_StateLoaded, NULL);
  fail_if (OMX_ErrorNone != error);

  /* ------------------------------------ */
  /* Await demuxer's transition callback */
  /* ------------------------------------ */
  error = _ctx_wait (&dmux_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_dmux_ctx->state [%s]",
             tiz_state_to_str (p_dmux_ctx->state));
  fail_if (OMX_StateLoaded != p_dmux_ctx->state);

  /* ----------------------------------------- */
  /* Check demuxer's state transition success */
  /* ----------------------------------------- */
  error = OMX_GetState (p_oggdmux, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] state [%s]",
           OGG_DMUX_COMPONENT_NAME, tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* ------------------------------------ */
  /* Await decoder's transition callback */
  /* ------------------------------------ */
  error = _ctx_wait (&dec_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_dec_ctx->state [%s]",
             tiz_state_to_str (p_dec_ctx->state));
  fail_if (OMX_StateLoaded != p_dec_ctx->state);

  /* ----------------------------------------- */
  /* Check decoder's state transition success */
  /* ----------------------------------------- */
  error = OMX_GetState (p_flacdec, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] state [%s]",
             FLAC_DEC_COMPONENT_NAME, tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* ------------------------------------ */
  /* Await renderer's transition callback */
  /* ------------------------------------ */
  error = _ctx_wait (&rend_ctx, OMX_EventCmdComplete,
                     TIMEOUT_EXPECTING_SUCCESS, &timedout);
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_TRUE == timedout);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "p_rend_ctx->state [%s]",
             tiz_state_to_str (p_rend_ctx->state));
  fail_if (OMX_StateLoaded != p_rend_ctx->state);

  /* ----------------------------------------- */
  /* Check renderer's state transition success */
  /* ----------------------------------------- */
  error = OMX_GetState (p_pcmrnd, &state);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] state [%s]",
             PCM_RND_COMPONENT_NAME, tiz_state_to_str (state));
  fail_if (OMX_ErrorNone != error);
  fail_if (OMX_StateLoaded != state);

  /* ---------------- */
  /* Teardown tunnels */
  /* ---------------- */
  error = OMX_TeardownTunnel(p_oggdmux, 0, p_flacdec, 0);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_TeardownTunnel [%s]",
           tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);
  error = OMX_TeardownTunnel(p_flacdec, 1, p_pcmrnd, 0);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_TeardownTunnel [%s]",
           tiz_err_to_str(error));
  fail_if (OMX_ErrorNone != error);

  tiz_mem_free (p_uri_param);

  /* ------------------ */
  /* Destroy components */
  /* ------------------ */
  error = OMX_FreeHandle (p_oggdmux);
  fail_if (OMX_ErrorNone != error);
  error = OMX_FreeHandle (p_flacdec);
  fail_if (OMX_ErrorNone != error);
  error = OMX_FreeHandle (p_pcmrnd);
  fail_if (OMX_ErrorNone != error);

  error = OMX_Deinit ();
  fail_if (OMX_ErrorNone != error);

  _ctx_destroy (&dmux_ctx);
  _ctx_destroy (&dec_ctx);
  _ctx_destroy (&rend_ctx);
}
END_TEST

Suite * flacdec_suite (void)
{
  TCase *tc_md;
  Suite *s = suite_create ("libtizflacd");

  /* test case */
  tc_md = tcase_create ("FLAC Playback");
  tcase_add_unchecked_fixture (tc_md, setup, teardown);
  tcase_set_timeout (tc_md, FLAC_DECODER_TEST_TIMEOUT);
  tcase_add_loop_test (tc_md, test_flac_playback, 0, 1);
  suite_add_tcase (s, tc_md);

  return s;
}

int
main (void)
{
  int number_failed;
  SRunner *sr = srunner_create (flacdec_suite ());

  tiz_log_init();

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Tizonia OpenMAX IL - Flac Decoder unit tests");

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
