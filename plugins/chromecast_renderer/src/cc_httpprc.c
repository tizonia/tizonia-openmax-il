/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * along with Tizonia.  If not, see <chromecast://www.gnu.org/licenses/>.
 */

/**
 * @file   cc_httpprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Chromecast renderer - http streaming client - processor class
 * implementation,
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "chromecastrnd.h"
#include "cc_httpprc.h"
#include "cc_httpprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer.prc.http"
#endif

static OMX_ERRORTYPE
obtain_url (cc_http_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const long pathname_max = PATH_MAX + NAME_MAX;

  assert (ap_prc);
  assert (!ap_prc->p_content_uri_);

  ap_prc->p_content_uri_
    = tiz_mem_calloc (1, sizeof (OMX_PARAM_CONTENTURITYPE) + pathname_max + 1);

  tiz_check_null_ret_oom (ap_prc->p_content_uri_);

  ap_prc->p_content_uri_->nSize
    = sizeof (OMX_PARAM_CONTENTURITYPE) + pathname_max + 1;
  ap_prc->p_content_uri_->nVersion.nVersion = OMX_VERSION;

  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamContentURI, ap_prc->p_content_uri_));
  TIZ_NOTICE (handleOf (ap_prc), "URI [%s]",
              ap_prc->p_content_uri_->contentURI);
  /* Verify we are getting an http scheme */
  if (strncasecmp ((const char *) ap_prc->p_content_uri_->contentURI, "http://",
                   7)
      && strncasecmp ((const char *) ap_prc->p_content_uri_->contentURI,
                      "https://", 8))
    {
      rc = OMX_ErrorContentURIError;
    }

  return rc;
}

static inline void
delete_url (cc_http_prc_t * ap_prc)
{
  assert (ap_prc);
  tiz_mem_free (ap_prc->p_content_uri_);
  ap_prc->p_content_uri_ = NULL;
}

/*
 * cc_httpprc
 */

static void *
cc_http_prc_ctor (void * ap_obj, va_list * app)
{
  cc_http_prc_t * p_prc
    = super_ctor (typeOf (ap_obj, "cc_httpprc"), ap_obj, app);
  p_prc->p_content_uri_ = NULL;
  return p_prc;
}

static void *
cc_http_prc_dtor (void * ap_obj)
{
  delete_url (ap_obj);
  return super_dtor (typeOf (ap_obj, "cc_httpprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
cc_http_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  cc_http_prc_t * p_prc = ap_obj;
  assert (p_prc);

  tiz_check_omx (tiz_srv_super_allocate_resources (typeOf (p_prc, "cc_httpprc"),
                                                   p_prc, a_pid));

  return obtain_url (ap_obj);
}

static OMX_ERRORTYPE
cc_http_prc_deallocate_resources (void * ap_prc)
{
  delete_url (ap_prc);
  return tiz_srv_super_deallocate_resources (typeOf (ap_prc, "cc_httpprc"),
                                             ap_prc);
}

static const char *
cc_http_prc_get_next_url (const void * p_obj)
{
  cc_http_prc_t * p_prc = (cc_http_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_content_uri_);
  return (const char *) p_prc->p_content_uri_->contentURI;
}

static const char *
cc_http_prc_get_prev_url (const void * p_obj)
{
  cc_http_prc_t * p_prc = (cc_http_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_content_uri_);
  return (const char *) p_prc->p_content_uri_->contentURI;
}

static const char *
cc_http_prc_get_current_stream_album_art_url (const void * p_obj)
{
  /* This is Tizonia's logo */
#define DEFAULT_ART "https://avatars0.githubusercontent.com/u/3161606?v=3&s=400"
  return DEFAULT_ART;
}

static OMX_ERRORTYPE
cc_http_prc_store_stream_metadata (const void * p_obj)
{
  cc_http_prc_t * p_prc = (cc_http_prc_t *) p_obj;
  cc_prc_t * p_cc_prc = (cc_prc_t *) p_obj;
  assert (p_prc);

  TIZ_DEBUG (handleOf (p_prc), "store_stream_metadata");

  /* Station url */
  {
    tiz_check_omx (cc_prc_store_display_title (
      p_cc_prc, "Station", (const char *) p_prc->p_content_uri_->contentURI));
    tiz_check_omx (cc_prc_store_stream_metadata_item (
      p_cc_prc, "Station", (const char *) p_prc->p_content_uri_->contentURI));
  }

  return OMX_ErrorNone;
}

/*
 * cc_http_prc_class
 */

static void *
cc_http_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "cc_httpprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
cc_http_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_httpprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (cc_prc), "cc_httpprc_class", classOf (cc_prc),
     sizeof (cc_http_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_http_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return cc_httpprc_class;
}

void *
cc_http_prc_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_httpprc_class = tiz_get_type (ap_hdl, "cc_httpprc_class");
  TIZ_LOG_CLASS (cc_httpprc_class);
  void * cc_httpprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_httpprc_class, "cc_httpprc", cc_prc, sizeof (cc_http_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_http_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, cc_http_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, cc_http_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, cc_http_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_next_url, cc_http_prc_get_next_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_prev_url, cc_http_prc_get_prev_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_current_stream_album_art_url,
     cc_http_prc_get_current_stream_album_art_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_store_stream_metadata, cc_http_prc_store_stream_metadata,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return cc_httpprc;
}
