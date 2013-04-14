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

#include <assert.h>
#include <limits.h>
#include <string.h>

#include "tizkernel.h"
#include "tizscheduler.h"

#include "sdlivrprc.h"
#include "sdlivrprc_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.yuv_renderer.prc"
#endif

static OMX_ERRORTYPE
sdlivr_proc_render_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
{
  const struct sdlivrprc *p_obj = ap_obj;
  (void) p_obj;

/*   TIZ_LOG (TIZ_TRACE, */
/*            "Rendering HEADER [%p]...nFilledLen[%d] !!!", p_hdr, */
/*            p_hdr->nFilledLen); */

  if (p_obj->p_overlay)
    {
      //AVPicture pict;
      SDL_Rect rect;
      uint8_t *y;
      uint8_t *u;
      uint8_t *v;
      unsigned int bytes;
      int pitch0, pitch1;

      //align pitch on 16-pixel boundary.
      pitch0 = (p_obj->vportdef_.nFrameWidth + 15) & ~15;
      pitch1 = pitch0 / 2;

      //FIXME: hard-coded to be YUV420 plannar
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
  struct sdlivrprc *p_obj = super_ctor (sdlivrprc, ap_obj, app);
  TIZ_LOG (TIZ_TRACE, "Constructing sdlivrprc...[%p]", p_obj);

  p_obj->pinhdr_ = 0;
  p_obj->pouthdr_ = 0;
  p_obj->eos_ = false;

  return p_obj;
}

static void *
sdlivr_proc_dtor (void *ap_obj)
{
  struct sdlivrprc *p_obj = ap_obj;
  TIZ_LOG (TIZ_TRACE, "Destructing sdlivrprc...[%p]", p_obj);
  return super_dtor (sdlivrprc, ap_obj);
}

/*
 * from tiz_servant class
 */

static OMX_ERRORTYPE
sdlivr_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  struct sdlivrprc *p_obj = ap_obj;
  const tiz_servant_t *p_parent = ap_obj;
  assert (ap_obj);

  (void) p_parent;
  (void) p_obj;

  if (-1 == SDL_Init (SDL_INIT_VIDEO))
    {
      TIZ_LOG_CNAME (TIZ_ERROR, TIZ_CNAME (p_parent->p_hdl_),
                     TIZ_CBUF (p_parent->p_hdl_),
                     "Error while initializing SDL [%s]", SDL_GetError ());
      return OMX_ErrorInsufficientResources;
    }

  TIZ_LOG_CNAME (TIZ_TRACE,
                 TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "Resource allocation complete..." "pid = [%d]", a_pid);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sdlivr_proc_deallocate_resources (void *ap_obj)
{
  struct sdlivrprc *p_obj = ap_obj;
  const tiz_servant_t *p_parent = ap_obj;
  assert (ap_obj);

  (void) p_parent;
  (void) p_obj;

  SDL_Quit ();

  TIZ_LOG_CNAME (TIZ_TRACE,
                 TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "Resource deallocation complete...");

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
sdlivr_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  struct sdlivrprc *p_obj = ap_obj;
  const tiz_servant_t *p_parent = ap_obj;
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;
  void *p_krn = tiz_get_krn (p_parent->p_hdl_);
  OMX_PARAM_PORTDEFINITIONTYPE portdef;

  TIZ_LOG (TIZ_TRACE, "pid [%d]", a_pid);

  assert (ap_obj);

  /* Retrieve port def from port */
  portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  portdef.nVersion.nVersion = OMX_VERSION;
  portdef.nPortIndex = 0;       /* port index */
  if (OMX_ErrorNone != (ret_val = tiz_api_GetParameter
                        (p_krn,
                         p_parent->p_hdl_,
                         OMX_IndexParamPortDefinition, &portdef)))
    {
      TIZ_LOG (TIZ_TRACE, "Error retrieving the port definition");
      return ret_val;
    }

  p_obj->vportdef_ = portdef.format.video;

  TIZ_LOG (TIZ_TRACE, "nFrameWidth = [%d] nFrameHeight = [%d] ",
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

  TIZ_LOG_CNAME (TIZ_TRACE,
                 TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
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
  struct sdlivrprc *p_obj = ap_obj;

  assert (NULL != p_obj);

  SDL_FreeYUVOverlay (p_obj->p_overlay);

  return OMX_ErrorNone;
}

/*
 * from tiz_proc class
 */

static OMX_ERRORTYPE
sdlivr_proc_buffers_ready (const void *ap_obj)
{
  const tiz_servant_t *p_parent = ap_obj;
  tiz_pd_set_t ports;
  void *p_krn = tiz_get_krn (p_parent->p_hdl_);
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  TIZ_PD_ZERO (&ports);

  tiz_check_omx_err (tiz_kernel_select (p_krn, 1, &ports));

  if (TIZ_PD_ISSET (0, &ports))
    {
      tiz_check_omx_err (tiz_kernel_claim_buffer (p_krn, 0, 0, &p_hdr));
      tiz_check_omx_err (sdlivr_proc_render_buffer (ap_obj, p_hdr));
      if (p_hdr->nFlags & OMX_BUFFERFLAG_EOS)
        {
          TIZ_LOG (TIZ_TRACE, "OMX_BUFFERFLAG_EOS in HEADER [%p]", p_hdr);
          tiz_servant_issue_event ((OMX_PTR) ap_obj,
                                  OMX_EventBufferFlag,
                                  0, p_hdr->nFlags, NULL);
        }
      tiz_kernel_relinquish_buffer (p_krn, 0, p_hdr);
    }

  return OMX_ErrorNone;
}

/*
 * initialization
 */

const void *sdlivrprc;

void
init_sdlivrprc (void)
{

  if (!sdlivrprc)
    {
      tiz_proc_init ();
      sdlivrprc =
        factory_new
        (tizproc_class,
         "sdlivrprc",
         tizproc,
         sizeof (struct sdlivrprc),
         ctor, sdlivr_proc_ctor,
         dtor, sdlivr_proc_dtor,
         tiz_proc_buffers_ready, sdlivr_proc_buffers_ready,
         tiz_servant_allocate_resources, sdlivr_proc_allocate_resources,
         tiz_servant_deallocate_resources, sdlivr_proc_deallocate_resources,
         tiz_servant_prepare_to_transfer, sdlivr_proc_prepare_to_transfer,
         tiz_servant_transfer_and_process, sdlivr_proc_transfer_and_process,
         tiz_servant_stop_and_return, sdlivr_proc_stop_and_return, 0);
    }
}
