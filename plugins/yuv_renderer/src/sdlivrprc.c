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
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   sdlivrprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - SDL Image/Video Renderer processor class
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizservant_decls.h>

#include "sdlivr.h"
#include "sdlivrprc.h"
#include "sdlivrprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.yuv_renderer.prc"
#endif

/* forward declarations */
static OMX_ERRORTYPE
sdlivr_prc_deallocate_resources (void * ap_obj);

static OMX_ERRORTYPE
sdlivr_prc_render_buffer (const sdlivr_prc_t * ap_prc,
                          OMX_BUFFERHEADERTYPE * p_hdr)
{
  assert (ap_prc);

  if (ap_prc->p_overlay)
    {
      const OMX_VIDEO_PORTDEFINITIONTYPE * p_vpd = &(ap_prc->port_def_);

      /* AVPicture pict; */
      SDL_Rect rect;
      uint8_t * y;
      uint8_t * u;
      uint8_t * v;
      unsigned int bytes;
      int pitch0, pitch1;

      if (p_vpd->nStride == 0)
        {
          /* align pitch on 16-pixel boundary. */
          pitch0 = (p_vpd->nFrameWidth + 15) & ~15;
        }
      else
        {
          pitch0 = p_vpd->nStride;
        }
      pitch1 = pitch0 / 2;

      /* hard-coded to be YUV420 plannar */
      y = p_hdr->pBuffer;
      u = y + pitch0 * p_vpd->nFrameHeight;
      v = u + pitch1 * p_vpd->nFrameHeight / 2;

      SDL_LockYUVOverlay (ap_prc->p_overlay);

      if (ap_prc->p_overlay->pitches[0] != pitch0
          || ap_prc->p_overlay->pitches[1] != pitch1
          || ap_prc->p_overlay->pitches[2] != pitch1)
        {
          int hh;
          uint8_t * y2;
          uint8_t * u2;
          uint8_t * v2;

          y2 = ap_prc->p_overlay->pixels[0];
          u2 = ap_prc->p_overlay->pixels[2];
          v2 = ap_prc->p_overlay->pixels[1];

          for (hh = 0; hh < p_vpd->nFrameHeight; hh++)
            {
              memcpy (y2, y, ap_prc->p_overlay->pitches[0]);
              y2 += ap_prc->p_overlay->pitches[0];
              y += pitch0;
            }
          for (hh = 0; hh < p_vpd->nFrameHeight / 2; hh++)
            {
              memcpy (u2, u, ap_prc->p_overlay->pitches[2]);
              u2 += ap_prc->p_overlay->pitches[2];
              u += pitch1;
            }
          for (hh = 0; hh < p_vpd->nFrameHeight / 2; hh++)
            {
              memcpy (v2, v, ap_prc->p_overlay->pitches[1]);
              v2 += ap_prc->p_overlay->pitches[1];
              v += pitch1;
            }
        }
      else
        {
          bytes = pitch0 * p_vpd->nFrameHeight;
          memcpy (ap_prc->p_overlay->pixels[0], y, bytes);

          bytes = pitch1 * p_vpd->nFrameHeight / 2;
          memcpy (ap_prc->p_overlay->pixels[2], u, bytes);

          bytes = pitch1 * p_vpd->nFrameHeight / 2;
          memcpy (ap_prc->p_overlay->pixels[1], v, bytes);
        }

      SDL_UnlockYUVOverlay (ap_prc->p_overlay);

      rect.x = 0;
      rect.y = 0;
      rect.w = p_vpd->nFrameWidth;
      rect.h = p_vpd->nFrameHeight;
      SDL_DisplayYUVOverlay (ap_prc->p_overlay, &rect);
    }

  p_hdr->nFilledLen = 0;

  return OMX_ErrorNone;
}

/*
 * sdlivrprc
 */

static void *
sdlivr_prc_ctor (void * ap_obj, va_list * app)
{
  sdlivr_prc_t * p_prc = super_ctor (typeOf (ap_obj, "sdlivrprc"), ap_obj, app);
  assert (p_prc);
  tiz_mem_set (&(p_prc->port_def_), 0, sizeof (OMX_VIDEO_PORTDEFINITIONTYPE));
  p_prc->p_surface = NULL;
  p_prc->p_overlay = NULL;
  p_prc->port_disabled_ = false;
  return p_prc;
}

static void *
sdlivr_prc_dtor (void * ap_obj)
{
  (void) sdlivr_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "sdlivrprc"), ap_obj);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
sdlivr_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  if (-1 == SDL_Init (SDL_INIT_VIDEO))
    {
      rc = OMX_ErrorInsufficientResources;
      TIZ_ERROR (handleOf (ap_obj), "[%s] : while initializing SDL [%s]",
                 tiz_err_to_str (rc), SDL_GetError ());
    }
  return rc;
}

static OMX_ERRORTYPE
sdlivr_prc_deallocate_resources (void * ap_obj)
{
  sdlivr_prc_t * p_prc = ap_obj;
  assert (p_prc);
  if (p_prc->p_overlay)
    {
      SDL_FreeYUVOverlay (p_prc->p_overlay);
      p_prc->p_overlay = NULL;
    }
  /* This frees p_prc->p_surface */
  SDL_Quit ();
  p_prc->p_surface = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sdlivr_prc_prepare_to_transfer (void * ap_obj, OMX_U32 a_pid)
{
  sdlivr_prc_t * p_prc = ap_obj;
  OMX_PARAM_PORTDEFINITIONTYPE portdef;
  TIZ_INIT_OMX_PORT_STRUCT (portdef, ARATELIA_YUV_RENDERER_PORT_INDEX);

  assert (p_prc);

  /* Retrieve port def from port */
  tiz_check_omx (tiz_api_GetParameter (tiz_get_krn (handleOf (p_prc)),
                                       handleOf (p_prc),
                                       OMX_IndexParamPortDefinition, &portdef));

  p_prc->port_def_ = portdef.format.video;

  TIZ_TRACE (
    handleOf (p_prc),
    "nFrameWidth = [%u] nFrameHeight = [%u] "
    "nStride = [%d] nSliceHeight = [%u] nBitrate = [%u] "
    "xFramerate = [%u] eCompressionFormat = [%0x] eColorFormat = [%0x]",
    p_prc->port_def_.nFrameWidth, p_prc->port_def_.nFrameHeight,
    p_prc->port_def_.nStride, p_prc->port_def_.nSliceHeight,
    p_prc->port_def_.nBitrate, p_prc->port_def_.xFramerate,
    p_prc->port_def_.eCompressionFormat, p_prc->port_def_.eColorFormat);

  SDL_WM_SetCaption ("Tizonia YUV renderer", "YUV");

  p_prc->p_surface = SDL_SetVideoMode (
    p_prc->port_def_.nFrameWidth, p_prc->port_def_.nFrameHeight, 0,
    SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_RESIZABLE);

  assert (!p_prc->p_overlay);
  p_prc->p_overlay = SDL_CreateYUVOverlay (p_prc->port_def_.nFrameWidth,
                                           p_prc->port_def_.nFrameHeight,
                                           SDL_YV12_OVERLAY, p_prc->p_surface);

  return p_prc->p_overlay ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
}

static OMX_ERRORTYPE
sdlivr_prc_transfer_and_process (void * ap_obj, OMX_U32 a_pid)
{
  sdlivr_prc_t * p_prc = ap_obj;
  assert (p_prc);
  if (!p_prc->p_overlay)
    {
      p_prc->p_surface = SDL_SetVideoMode (
        p_prc->port_def_.nFrameWidth, p_prc->port_def_.nFrameHeight, 0,
        SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_RESIZABLE);

      p_prc->p_overlay = SDL_CreateYUVOverlay (
        p_prc->port_def_.nFrameWidth, p_prc->port_def_.nFrameHeight,
        SDL_YV12_OVERLAY, p_prc->p_surface);
    }
  return p_prc->p_overlay ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
}

static OMX_ERRORTYPE
sdlivr_prc_stop_and_return (void * ap_obj)
{
  sdlivr_prc_t * p_prc = ap_obj;
  assert (p_prc);
  SDL_FreeYUVOverlay (p_prc->p_overlay);
  p_prc->p_overlay = NULL;
  return OMX_ErrorNone;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
sdlivr_prc_buffers_ready (const void * ap_obj)
{
  sdlivr_prc_t * p_prc = (sdlivr_prc_t *) ap_obj;
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  void * p_krn = tiz_get_krn (handleOf (ap_obj));

  assert (p_prc);

  if (!p_prc->port_disabled_)
    {
      tiz_check_omx (tiz_krn_claim_buffer (
        p_krn, ARATELIA_YUV_RENDERER_PORT_INDEX, 0, &p_hdr));

      while (!p_prc->port_disabled_ && p_hdr)
        {
          if (p_hdr)
            {
              tiz_check_omx (sdlivr_prc_render_buffer (ap_obj, p_hdr));
              if (p_hdr->nFlags & OMX_BUFFERFLAG_EOS)
                {
                  TIZ_TRACE (handleOf (ap_obj),
                             "OMX_BUFFERFLAG_EOS in HEADER [%p]", p_hdr);
                  tiz_srv_issue_event ((OMX_PTR) ap_obj, OMX_EventBufferFlag, 0,
                                       p_hdr->nFlags, NULL);
                }
              tiz_check_omx (tiz_krn_release_buffer (
                p_krn, ARATELIA_YUV_RENDERER_PORT_INDEX, p_hdr));
              tiz_check_omx (tiz_krn_claim_buffer (
                p_krn, ARATELIA_YUV_RENDERER_PORT_INDEX, 0, &p_hdr));
            }
        }
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sdlivr_prc_port_disable (const void * ap_obj, OMX_U32 a_pid)
{
  sdlivr_prc_t * p_prc = (sdlivr_prc_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (OMX_ALL == a_pid || ARATELIA_YUV_RENDERER_PORT_INDEX == a_pid)
    {
      p_prc->port_disabled_ = true;
      rc = sdlivr_prc_deallocate_resources (p_prc);
    }
  return rc;
}

static OMX_ERRORTYPE
sdlivr_prc_port_enable (const void * ap_obj, OMX_U32 a_pid)
{
  sdlivr_prc_t * p_prc = (sdlivr_prc_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (OMX_ALL == a_pid || ARATELIA_YUV_RENDERER_PORT_INDEX == a_pid)
    {
      if (p_prc->port_disabled_)
        {
          p_prc->port_disabled_ = false;
          tiz_check_omx (sdlivr_prc_allocate_resources (p_prc, OMX_ALL));
          tiz_check_omx (sdlivr_prc_prepare_to_transfer (p_prc, OMX_ALL));
        }
    }
  return rc;
}

/*
 * sdlivr_prc_class
 */

static void *
sdlivr_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "sdlivrprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
sdlivr_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * sdlivrprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "sdlivrprc_class", classOf (tizprc),
     sizeof (sdlivr_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, sdlivr_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return sdlivrprc_class;
}

void *
sdlivr_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * sdlivrprc_class = tiz_get_type (ap_hdl, "sdlivrprc_class");
  TIZ_LOG_CLASS (sdlivrprc_class);
  void * sdlivrprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (sdlivrprc_class, "sdlivrprc", tizprc, sizeof (sdlivr_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, sdlivr_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, sdlivr_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, sdlivr_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, sdlivr_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, sdlivr_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, sdlivr_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, sdlivr_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, sdlivr_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, sdlivr_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, sdlivr_prc_port_enable,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return sdlivrprc;
}
