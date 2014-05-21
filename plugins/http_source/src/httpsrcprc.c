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
 * @file   httpsrcprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - HTTP streaming client processor
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "httpsrc.h"
#include "httpsrcprc.h"
#include "httpsrcprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_source.prc"
#endif

/* These macros assume the existence of an "ap_prc" local variable */
#define bail_on_curl_error(expr)                                              \
  do                                                                          \
    {                                                                         \
      CURLcode curl_error = CURLE_OK;                                         \
      if (CURLE_OK != (curl_error = (expr)))                                  \
        {                                                                     \
          OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;                  \
          TIZ_ERROR (handleOf (ap_prc), "[%s] : error while using curl (%s)", \
                     tiz_err_to_str (rc), curl_easy_strerror (curl_error));   \
          goto end;                                                           \
        }                                                                     \
    }                                                                         \
  while (0)

#define bail_on_oom(expr)                                                    \
  do                                                                         \
    {                                                                        \
      if (NULL == (expr))                                                    \
        {                                                                    \
          OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;                 \
          TIZ_ERROR (handleOf (ap_prc), "[OMX_ErrorInsufficientResources]"); \
          goto end;                                                          \
        }                                                                    \
    }                                                                        \
  while (0)

/* This function gets called by libcurl as soon as it has received header
   data. The header callback will be called once for each header and only
   complete header lines are passed on to the callback. Parsing headers is very
   easy using this. The size of the data pointed to by ptr is size multiplied
   with nmemb. Do not assume that the header line is zero terminated! The
   pointer named userdata is the one you set with the CURLOPT_WRITEHEADER
   option. The callback function must return the number of bytes actually taken
   care of. If that amount differs from the amount passed to your function,
   it'll signal an error to the library. This will abort the transfer and
   return CURL_WRITE_ERROR. */
static size_t curl_header_cback (void *ptr, size_t size, size_t nmemb,
                                 void *userdata)
{
  httpsrc_prc_t *p_prc = userdata;
  assert (NULL != p_prc);
  return size;
}

/* This function gets called by libcurl as soon as there is data received that
   needs to be saved. The size of the data pointed to by ptr is size multiplied
   with nmemb, it will not be zero terminated. Return the number of bytes
   actually taken care of. If that amount differs from the amount passed to
   your function, it'll signal an error to the library. This will abort the
   transfer and return CURLE_WRITE_ERROR.  */
static size_t curl_write_cback (void *ptr, size_t size, size_t nmemb,
                                void *userdata)
{
  httpsrc_prc_t *p_prc = userdata;
  assert (NULL != p_prc);
  if (size == 0)
    return 0;

  return size;
}

#ifdef _DEBUG
/* Pass a pointer to a function that matches the following prototype: int
   curl_debug_callback (CURL *, curl_infotype, char *, size_t, void *);
   CURLOPT_DEBUGFUNCTION replaces the standard debug function used when
   CURLOPT_VERBOSE is in effect. This callback receives debug information, as
   specified with the curl_infotype argument. This function must return 0. The
   data pointed to by the char * passed to this function WILL NOT be zero
   terminated, but will be exactly of the size as told by the size_t
   argument.  */
static size_t curl_debug_cback (CURL *, curl_infotype type, char *buf,
                                size_t nbytes, void *userdata)
{
  httpsrc_prc_t *p_prc = userdata;
  if (CURLINFO_TEXT == type || CURLINFO_HEADER_IN || CURLINFO_HEADER_OUT)
    {
      assert (NULL != p_prc);
      char *p_info = tiz_mem_calloc (1, nbytes + 1);
      memcpy (p_info, buf, nbytes);
      TIZ_TRACE (handleOf (p_prc), "libcurl  : [%s]");
      tiz_mem_free (p_info);
    }
  return 0;
}
#endif

static void destroy_curl_resources (httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  curl_slist_free_all (ap_prc->p_http_ok_aliases_);
  ap_prc->p_http_ok_aliases_ = NULL;
  curl_slist_free_all (ap_prc->p_http_headers_);
  ap_prc->p_http_headers_ = NULL;
  curl_easy_cleanup (ap_prc->p_curl_);
  ap_prc->p_curl_ = NULL;
}

static OMX_ERRORTYPE init_curl_resources (httpsrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;

  assert (NULL == ap_prc->p_curl_);

  TIZ_DEBUG (handleOf (ap_prc), "%s", curl_version ());

  tiz_check_null_ret_oom ((ap_prc->p_curl_ = curl_easy_init ()));

  /* Tell libcurl that should accept ICY OK headers*/
  bail_on_oom ((ap_prc->p_http_ok_aliases_ = curl_slist_append (
                    ap_prc->p_http_ok_aliases_, "ICY 200 OK")));

  /* associate the processor with the curl handle */
  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_PRIVATE, ap_prc));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_USERAGENT,
                                        ARATELIA_HTTP_SOURCE_COMPONENT_NAME));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_HEADERFUNCTION,
                                        curl_header_cback));
  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_WRITEHEADER, ap_prc));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_WRITEFUNCTION,
                                        curl_write_cback));
  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_WRITEDATA, ap_prc));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_HTTP200ALIASES,
                                        ap_prc->p_http_ok_aliases_));
  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_FOLLOWLOCATION, 1));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_NETRC, 1));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_MAXREDIRS, 5));
  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_FAILONERROR, 1)); /* true */
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_ERRORBUFFER,
                                        ap_prc->curl_err));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_NOPROGRESS,
                                        1)); /* shut the progress meter */
  /*   curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1l); */
  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_CONNECTTIMEOUT, 10));

  /*   if (proxy != nullptr) */
  /*     curl_easy_setopt(ap_prc->p_curl_, CURLOPT_PROXY, proxy); */

  /*   if (proxy_port > 0) */
  /*     curl_easy_setopt(ap_prc->p_curl_, CURLOPT_PROXYPORT, (long)proxy_port);
   */

  /*   if (proxy_user != nullptr && proxy_password != nullptr) { */
  /*     char proxy_auth_str[1024]; */
  /*     snprintf(proxy_auth_str, sizeof(proxy_auth_str), */
  /*              "%s:%s", */
  /*              proxy_user, proxy_password); */
  /*     curl_easy_setopt(ap_prc->p_curl_, CURLOPT_PROXYUSERPWD,
   * proxy_auth_str);
   */
  /*   } */

  /*   CURLcode code = curl_easy_setopt(ap_prc->p_curl_, CURLOPT_URL,
   * base.uri.c_str()); */
  /*   if (code != CURLE_OK) { */
  /*     error.Format(curl_domain, code, */
  /*                  "curl_easy_setopt() failed: %s", */
  /*                  curl_easy_strerror(code)); */
  /*     return false; */
  /*   } */

  bail_on_oom ((ap_prc->p_http_headers_ = curl_slist_append (
                    ap_prc->p_http_headers_, "Icy-Metadata: 1")));

  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_HTTPHEADER,
                                        ap_prc->p_http_headers_));

#ifdef _DEBUG
  curl_easy_setopt (ap_prc->p_curl_, CURLOPT_VERBOSE, 1);
  curl_easy_setopt (ap_prc->p_curl_, CURLOPT_DEBUGDATA, ap_prc);
  curl_easy_setopt (ap_prc->p_curl_, CURLOPT_DEBUGFUNCTION, curl_debug_cback);
#endif

  /* all ok */
  rc = OMX_ErrorNone;

end:

  if (OMX_ErrorNone != rc)
    {
      /* Clean-up */
      destroy_curl_resources (ap_prc);
    }

  return rc;
}

static OMX_ERRORTYPE release_buffers (httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);

  if (ap_prc->p_outhdr_)
    {
      void *p_krn = tiz_get_krn (handleOf (ap_prc));
      tiz_check_omx_err (tiz_krn_release_buffer (p_krn, 0, ap_prc->p_outhdr_));
      ap_prc->p_outhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE do_flush (httpsrc_prc_t *ap_prc)
{
  return release_buffers (ap_prc);
}

/*
 * httpsrcprc
 */

static void *httpsrc_prc_ctor (void *ap_obj, va_list *app)
{
  httpsrc_prc_t *p_obj
      = super_ctor (typeOf (ap_obj, "httpsrcprc"), ap_obj, app);
  p_obj->eos_ = false;
  return p_obj;
}

static void *httpsrc_prc_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "httpsrcprc"), ap_obj);
}

static OMX_ERRORTYPE httpsrc_prc_read_buffer (const void *ap_obj,
                                              OMX_BUFFERHEADERTYPE *p_hdr)
{
  return OMX_ErrorNone;
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE httpsrc_prc_allocate_resources (void *ap_obj,
                                                     OMX_U32 a_pid)
{
  httpsrc_prc_t *p_prc = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_prc);

  tiz_check_omx_err (init_curl_resources (p_prc));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_prepare_to_transfer (void *ap_obj,
                                                      OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_transfer_and_process (void *ap_obj,
                                                       OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE httpsrc_prc_buffers_ready (const void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_io_ready (void *ap_obj,
                                           tiz_event_io_t *ap_ev_io, int a_fd,
                                           int a_events)
{
  TIZ_TRACE (handleOf (ap_obj), "Received io event on fd [%d]", a_fd);
  stop_io_watcher (ap_obj);
  return render_pcm_data (ap_obj);
}

static OMX_ERRORTYPE httpsrc_prc_pause (const void *ap_obj)
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *)ap_obj;
  assert (NULL != p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_resume (const void *ap_obj)
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *)ap_obj;
  assert (NULL != p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_port_flush (const void *ap_obj,
                                             OMX_U32 TIZ_UNUSED (a_pid))
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *)ap_obj;
  assert (NULL != p_prc);
  return do_flush (p_prc);
}

static OMX_ERRORTYPE httpsrc_prc_port_disable (const void *ap_obj,
                                               OMX_U32 TIZ_UNUSED (a_pid))
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *)ap_obj;
  assert (NULL != p_prc);
  /* Release any buffers held  */
  return release_buffers ((httpsrc_prc_t *)ap_obj);
}

static OMX_ERRORTYPE httpsrc_prc_port_enable (const void *ap_obj,
                                              OMX_U32 TIZ_UNUSED (a_pid))
{
  /* TODO */
  return OMX_ErrorNone;
}

/*
 * httpsrc_prc_class
 */

static void *httpsrc_prc_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "httpsrcprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *httpsrc_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *httpsrcprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizprc), "httpsrcprc_class", classOf (tizprc),
       sizeof(httpsrc_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, httpsrc_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return httpsrcprc_class;
}

void *httpsrc_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *httpsrcprc_class = tiz_get_type (ap_hdl, "httpsrcprc_class");
  TIZ_LOG_CLASS (httpsrcprc_class);
  void *httpsrcprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (httpsrcprc_class, "httpsrcprc", tizprc, sizeof(httpsrc_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, httpsrc_prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, httpsrc_prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, httpsrc_prc_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, httpsrc_prc_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, httpsrc_prc_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, httpsrc_prc_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, httpsrc_prc_stop_and_return,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, httpsrc_prc_buffers_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_io_ready, httpsrc_prc_io_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_pause, httpsrc_prc_pause,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_resume, httpsrc_prc_resume,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_flush, httpsrc_prc_port_flush,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_disable, httpsrc_prc_port_disable,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_enable, httpsrc_prc_port_enable,
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return httpsrcprc;
}
