/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @brief  Tizonia OpenMAX IL - SDL Video Renderer processor class
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

#include "sdlivr.h"
#include "sdlivrprc.h"
#include "sdlivrprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.yuv_renderer.prc"
#endif

static OMX_ERRORTYPE
sdlivr_proc_render_buffer (const sdlivr_prc_t *ap_prc, OMX_BUFFERHEADERTYPE * p_hdr)
{
  assert (NULL != ap_prc);

  if (ap_prc->p_overlay)
    {
      /* AVPicture pict; */
      SDL_Rect rect;
      uint8_t *y;
      uint8_t *u;
      uint8_t *v;
      unsigned int bytes;
      int pitch0, pitch1;

      /* align pitch on 16-pixel boundary. */
      pitch0 = (ap_prc->vportdef_.nFrameWidth + 15) & ~15;
      pitch1 = pitch0 / 2;

      /* hard-coded to be YUV420 plannar */
      y = p_hdr->pBuffer;
      u = y + pitch0 * ap_prc->vportdef_.nFrameHeight;
      v = u + pitch1 * ap_prc->vportdef_.nFrameHeight / 2;

      SDL_LockYUVOverlay (ap_prc->p_overlay);

      if (ap_prc->p_overlay->pitches[0] != pitch0
          || ap_prc->p_overlay->pitches[1] != pitch1
          || ap_prc->p_overlay->pitches[2] != pitch1)
        {
          int hh;
          uint8_t *y2;
          uint8_t *u2;
          uint8_t *v2;

          y2 = ap_prc->p_overlay->pixels[0];
          u2 = ap_prc->p_overlay->pixels[2];
          v2 = ap_prc->p_overlay->pixels[1];

          for (hh = 0; hh < ap_prc->vportdef_.nFrameHeight; hh++)
            {
              memcpy (y2, y, ap_prc->p_overlay->pitches[0]);
              y2 += ap_prc->p_overlay->pitches[0];
              y += pitch0;
            }
          for (hh = 0; hh < ap_prc->vportdef_.nFrameHeight / 2; hh++)
            {
              memcpy (u2, u, ap_prc->p_overlay->pitches[2]);
              u2 += ap_prc->p_overlay->pitches[2];
              u += pitch1;
            }
          for (hh = 0; hh < ap_prc->vportdef_.nFrameHeight / 2; hh++)
            {
              memcpy (v2, v, ap_prc->p_overlay->pitches[1]);
              v2 += ap_prc->p_overlay->pitches[1];
              v += pitch1;
            }
        }
      else
        {
          bytes = pitch0 * ap_prc->vportdef_.nFrameHeight;
          memcpy (ap_prc->p_overlay->pixels[0], y, bytes);

          bytes = pitch1 * ap_prc->vportdef_.nFrameHeight / 2;
          memcpy (ap_prc->p_overlay->pixels[2], u, bytes);

          bytes = pitch1 * ap_prc->vportdef_.nFrameHeight / 2;
          memcpy (ap_prc->p_overlay->pixels[1], v, bytes);
        }

      SDL_UnlockYUVOverlay (ap_prc->p_overlay);

      rect.x = 0;
      rect.y = 0;
      rect.w = ap_prc->vportdef_.nFrameWidth;
      rect.h = ap_prc->vportdef_.nFrameHeight;
      SDL_DisplayYUVOverlay (ap_prc->p_overlay, &rect);
    }

  p_hdr->nFilledLen = 0;

  return OMX_ErrorNone;
}


/*
 * sdlivrprc
 */

static void *
sdlivr_proc_ctor (void *ap_obj, va_list * app)
{
  sdlivr_prc_t *p_prc = super_ctor (typeOf (ap_obj, "sdlivrprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->pinhdr_ = NULL;
  p_prc->pouthdr_ = NULL;
  p_prc->eos_ = false;
  return p_prc;
}

static void *
sdlivr_proc_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "sdlivrprc"), ap_obj);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
sdlivr_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  if (-1 == SDL_Init (SDL_INIT_VIDEO))
    {
      rc =  OMX_ErrorInsufficientResources;
      TIZ_ERROR (handleOf (ap_obj),
                 "[%s] : while initializing SDL [%s]", tiz_err_to_str (rc),
                 SDL_GetError ());
    }
  return rc;
}

static OMX_ERRORTYPE
sdlivr_proc_deallocate_resources (void *ap_obj)
{
  SDL_Quit ();
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sdlivr_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  sdlivr_prc_t *p_prc = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PARAM_PORTDEFINITIONTYPE portdef;
  TIZ_INIT_OMX_PORT_STRUCT (portdef, ARATELIA_YUV_RENDERER_PORT_INDEX);

  TIZ_TRACE (handleOf (p_prc), "pid [%d]", a_pid);

  assert (NULL != p_prc);

  /* Retrieve port def from port */
  if (OMX_ErrorNone != (rc = tiz_api_GetParameter
                        (tiz_get_krn (handleOf (p_prc)),
                         handleOf (p_prc),
                         OMX_IndexParamPortDefinition, &portdef)))
    {
      TIZ_ERROR (handleOf (p_prc),
                 "[%s] : retrieving the port definition", tiz_err_to_str (rc));
      return rc;
    }

  p_prc->vportdef_ = portdef.format.video;

  TIZ_TRACE (handleOf (p_prc),
            "nFrameWidth = [%d] nFrameHeight = [%d] ",
            /*            "nStride = [%d] nSliceHeight = [%d] nBitrate = [%d] " */
            /*            "xFramerate = [%s] eCompressionFormat = [%d] eColorFormat = [%d]", */
            p_prc->vportdef_.nFrameWidth, p_prc->vportdef_.nFrameHeight);
  /*            p_prc->vportdef_.nStride, */
  /*            p_prc->vportdef_.nSliceHeight, */
  /*            p_prc->vportdef_.nBitrate, */
  /*            p_prc->vportdef_.xFramerate, */
  /*            p_prc->vportdef_.eCompressionFormat, */
  /*            p_prc->vportdef_.eColorFormat); */

  SDL_WM_SetCaption ("Tizonia OpenMAX IL YUV renderer", "YUV");


  p_prc->p_surface = SDL_SetVideoMode
    (p_prc->vportdef_.nFrameWidth, p_prc->vportdef_.nFrameHeight, 0,
     SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_RESIZABLE);

  p_prc->p_overlay = SDL_CreateYUVOverlay
    (p_prc->vportdef_.nFrameWidth, p_prc->vportdef_.nFrameHeight,
     SDL_YV12_OVERLAY, p_prc->p_surface);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sdlivr_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sdlivr_proc_stop_and_return (void *ap_obj)
{
  sdlivr_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  SDL_FreeYUVOverlay (p_prc->p_overlay);
  return OMX_ErrorNone;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
sdlivr_proc_buffers_ready (const void *ap_obj)
{
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  if (OMX_ErrorNone == tiz_krn_claim_buffer
      (tiz_get_krn (handleOf (ap_obj)), 0, 0, &p_hdr))
    {
      if (NULL != p_hdr)
        {
          tiz_check_omx_err (sdlivr_proc_render_buffer (ap_obj, p_hdr));
          if (p_hdr->nFlags & OMX_BUFFERFLAG_EOS)
            {
              TIZ_TRACE (handleOf (ap_obj),
                         "OMX_BUFFERFLAG_EOS in HEADER [%p]", p_hdr);
              tiz_srv_issue_event ((OMX_PTR) ap_obj,
                                   OMX_EventBufferFlag,
                                   0, p_hdr->nFlags, NULL);
            }
          tiz_check_omx_err
            (tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_obj)), 0, p_hdr));
        }
    }

  return OMX_ErrorNone;
}


/*
 * sdlivr_prc_class
 */

static void *
sdlivr_prc_class_ctor (void *ap_obj, va_list * app)
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
    (classOf (tizprc), "sdlivrprc_class", classOf (tizprc), sizeof (sdlivr_prc_class_t),
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
     ctor, sdlivr_proc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, sdlivr_proc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, sdlivr_proc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, sdlivr_proc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, sdlivr_proc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, sdlivr_proc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, sdlivr_proc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, sdlivr_proc_stop_and_return,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return sdlivrprc;
}
