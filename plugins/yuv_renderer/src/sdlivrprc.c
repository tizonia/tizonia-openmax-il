/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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

#include "sdlivrprc.h"
#include "sdlivrprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"

#include "tizosal.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.yuv_renderer.prc"
#endif

static OMX_ERRORTYPE
sdlivr_proc_render_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
{
  const sdlivr_prc_t *p_obj = ap_obj;

  if (p_obj->p_overlay)
    {
      /* AVPicture pict; */
      SDL_Rect rect;
      uint8_t *y;
      uint8_t *u;
      uint8_t *v;
      unsigned int bytes;
      int pitch0, pitch1;

      /* align pitch on 16-pixel boundary. */
      pitch0 = (p_obj->vportdef_.nFrameWidth + 15) & ~15;
      pitch1 = pitch0 / 2;

      /* hard-coded to be YUV420 plannar */
      y = p_hdr->pBuffer;
      u = y + pitch0 * p_obj->vportdef_.nFrameHeight;
      v = u + pitch1 * p_obj->vportdef_.nFrameHeight / 2;

      SDL_LockYUVOverlay (p_obj->p_overlay);

      if (p_obj->p_overlay->pitches[0] != pitch0
          || p_obj->p_overlay->pitches[1] != pitch1
          || p_obj->p_overlay->pitches[2] != pitch1)
        {
          int hh;
          uint8_t *y2;
          uint8_t *u2;
          uint8_t *v2;

          y2 = p_obj->p_overlay->pixels[0];
          u2 = p_obj->p_overlay->pixels[2];
          v2 = p_obj->p_overlay->pixels[1];

          for (hh = 0; hh < p_obj->vportdef_.nFrameHeight; hh++)
            {
              memcpy (y2, y, p_obj->p_overlay->pitches[0]);
              y2 += p_obj->p_overlay->pitches[0];
              y += pitch0;
            }
          for (hh = 0; hh < p_obj->vportdef_.nFrameHeight / 2; hh++)
            {
              memcpy (u2, u, p_obj->p_overlay->pitches[2]);
              u2 += p_obj->p_overlay->pitches[2];
              u += pitch1;
            }
          for (hh = 0; hh < p_obj->vportdef_.nFrameHeight / 2; hh++)
            {
              memcpy (v2, v, p_obj->p_overlay->pitches[1]);
              v2 += p_obj->p_overlay->pitches[1];
              v += pitch1;
            }
        }
      else
        {
          bytes = pitch0 * p_obj->vportdef_.nFrameHeight;
          memcpy (p_obj->p_overlay->pixels[0], y, bytes);

          bytes = pitch1 * p_obj->vportdef_.nFrameHeight / 2;
          memcpy (p_obj->p_overlay->pixels[2], u, bytes);

          bytes = pitch1 * p_obj->vportdef_.nFrameHeight / 2;
          memcpy (p_obj->p_overlay->pixels[1], v, bytes);
        }

      SDL_UnlockYUVOverlay (p_obj->p_overlay);

      rect.x = 0;
      rect.y = 0;
      rect.w = p_obj->vportdef_.nFrameWidth;
      rect.h = p_obj->vportdef_.nFrameHeight;
      SDL_DisplayYUVOverlay (p_obj->p_overlay, &rect);
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
  sdlivr_prc_t *p_obj = super_ctor (typeOf (ap_obj, "sdlivrprc"), ap_obj, app);
  p_obj->pinhdr_ = 0;
  p_obj->pouthdr_ = 0;
  p_obj->eos_ = false;
  return p_obj;
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
  sdlivr_prc_t *p_obj = ap_obj;
  assert (ap_obj);
  (void) p_obj;

  if (-1 == SDL_Init (SDL_INIT_VIDEO))
    {
      TIZ_ERROR (handleOf (ap_obj),
                "Error while initializing SDL [%s]", SDL_GetError ());
      return OMX_ErrorInsufficientResources;
    }
  TIZ_TRACE (handleOf (ap_obj),
                 "Resource allocation complete..." "pid = [%d]", a_pid);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sdlivr_proc_deallocate_resources (void *ap_obj)
{
  sdlivr_prc_t *p_obj = ap_obj;
  assert (ap_obj);
  (void) p_obj;
  SDL_Quit ();
  TIZ_TRACE (handleOf (ap_obj),
            "Resource deallocation complete...");
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sdlivr_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  sdlivr_prc_t *p_obj = ap_obj;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;
  void *p_krn = tiz_get_krn (handleOf (ap_obj));
  OMX_PARAM_PORTDEFINITIONTYPE portdef;

  TIZ_TRACE (handleOf (ap_obj), "pid [%d]", a_pid);

  assert (ap_obj);

  /* Retrieve port def from port */
  portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  portdef.nVersion.nVersion = OMX_VERSION;
  portdef.nPortIndex = 0;       /* port index */
  if (OMX_ErrorNone != (ret_val = tiz_api_GetParameter
                        (p_krn,
                         handleOf (ap_obj),
                         OMX_IndexParamPortDefinition, &portdef)))
    {
      TIZ_TRACE (handleOf (ap_obj),
                "Error retrieving the port definition");
      return ret_val;
    }

  p_obj->vportdef_ = portdef.format.video;

  TIZ_TRACE (handleOf (ap_obj),
            "nFrameWidth = [%d] nFrameHeight = [%d] ",
            /*            "nStride = [%d] nSliceHeight = [%d] nBitrate = [%d] " */
            /*            "xFramerate = [%s] eCompressionFormat = [%d] eColorFormat = [%d]", */
            p_obj->vportdef_.nFrameWidth, p_obj->vportdef_.nFrameHeight);
  /*            p_obj->vportdef_.nStride, */
  /*            p_obj->vportdef_.nSliceHeight, */
  /*            p_obj->vportdef_.nBitrate, */
  /*            p_obj->vportdef_.xFramerate, */
  /*            p_obj->vportdef_.eCompressionFormat, */
  /*            p_obj->vportdef_.eColorFormat); */

  SDL_WM_SetCaption ("Tizonia OpenMAX IL YUV renderer", "YUV");


  p_obj->p_surface = SDL_SetVideoMode
    (p_obj->vportdef_.nFrameWidth, p_obj->vportdef_.nFrameHeight, 0,
     SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_RESIZABLE);

  p_obj->p_overlay = SDL_CreateYUVOverlay
    (p_obj->vportdef_.nFrameWidth, p_obj->vportdef_.nFrameHeight,
     SDL_YV12_OVERLAY, p_obj->p_surface);
  TIZ_TRACE (handleOf (ap_obj),
            "Transfering buffers...pid [%d]", a_pid);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sdlivr_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  assert (ap_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sdlivr_proc_stop_and_return (void *ap_obj)
{
  sdlivr_prc_t *p_obj = ap_obj;

  assert (NULL != p_obj);

  SDL_FreeYUVOverlay (p_obj->p_overlay);

  return OMX_ErrorNone;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
sdlivr_proc_buffers_ready (const void *ap_obj)
{
  tiz_pd_set_t ports;
  void *p_krn = tiz_get_krn (handleOf (ap_obj));
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  TIZ_PD_ZERO (&ports);

  tiz_check_omx_err (tiz_krn_select (p_krn, 1, &ports));

  if (TIZ_PD_ISSET (0, &ports))
    {
      tiz_check_omx_err (tiz_krn_claim_buffer (p_krn, 0, 0, &p_hdr));
      tiz_check_omx_err (sdlivr_proc_render_buffer (ap_obj, p_hdr));
      if (p_hdr->nFlags & OMX_BUFFERFLAG_EOS)
        {
          TIZ_TRACE (handleOf (ap_obj),
                    "OMX_BUFFERFLAG_EOS in HEADER [%p]", p_hdr);
          tiz_srv_issue_event ((OMX_PTR) ap_obj,
                                  OMX_EventBufferFlag,
                                  0, p_hdr->nFlags, NULL);
        }
      tiz_krn_release_buffer (p_krn, 0, p_hdr);
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
  void * sdlivrprc_class = factory_new (classOf (tizprc),
                                       "sdlivrprc_class",
                                        classOf (tizprc),
                                        sizeof (sdlivr_prc_class_t),
                                        ap_tos, ap_hdl,
                                        ctor, sdlivr_prc_class_ctor, 0);
  return sdlivrprc_class;
}

void *
sdlivr_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * sdlivrprc_class = tiz_get_type (ap_hdl, "sdlivrprc_class");
  TIZ_LOG_CLASS (sdlivrprc_class);
  void * sdlivrprc =
    factory_new
    (sdlivrprc_class,
     "sdlivrprc",
     tizprc,
     sizeof (sdlivr_prc_t),
     ap_tos, ap_hdl,
     ctor, sdlivr_proc_ctor,
     dtor, sdlivr_proc_dtor,
     tiz_prc_buffers_ready, sdlivr_proc_buffers_ready,
     tiz_srv_allocate_resources, sdlivr_proc_allocate_resources,
     tiz_srv_deallocate_resources, sdlivr_proc_deallocate_resources,
     tiz_srv_prepare_to_transfer, sdlivr_proc_prepare_to_transfer,
     tiz_srv_transfer_and_process, sdlivr_proc_transfer_and_process,
     tiz_srv_stop_and_return, sdlivr_proc_stop_and_return, 0);

  return sdlivrprc;
}
