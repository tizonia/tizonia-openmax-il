/**
 * Copyright (C) 2017 Julien Isorce
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

#include <assert.h>
#include <string.h>
#include <limits.h>

#include <tizplatform.h>
#include <tizkernel.h>

#include "vp8d.h"
#include "vp8dinport.h"
#include "vp8dinport_decls.h"
#include "vp8dprc_decls.h"

/*
 * vp8dinport class
 */

static void * vp8d_inport_ctor (void * ap_obj, va_list * app)
{
   return super_ctor (typeOf (ap_obj, "vp8dinport"), ap_obj, app);
}

static void * vp8d_inport_dtor (void * ap_obj)
{
   return super_dtor (typeOf (ap_obj, "vp8dinport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
vp8d_inport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                          OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  OMX_ERRORTYPE err = OMX_ErrorNone;

  assert (ap_obj);
  assert (ap_hdl);
  assert (ap_struct);

  if (a_index == OMX_IndexParamPortDefinition) {
    vp8d_prc_t * p_prc = tiz_get_prc (ap_hdl);
    OMX_VIDEO_PORTDEFINITIONTYPE * p_def = &(p_prc->port_def_.format.video);
    OMX_PARAM_PORTDEFINITIONTYPE * i_def = (OMX_PARAM_PORTDEFINITIONTYPE *) ap_struct;

    /* Make changes only if there is a resolution change */
    if ((p_def->nFrameWidth == i_def->format.video.nFrameWidth) &&
        (p_def->nFrameHeight == i_def->format.video.nFrameHeight) &&
        (p_def->xFramerate == i_def->format.video.xFramerate) &&
        (p_def->eCompressionFormat == i_def->format.video.eCompressionFormat))
      return err;

    /* Set some default values if not set */
    if (i_def->format.video.nStride == 0)
      i_def->format.video.nStride = i_def->format.video.nFrameWidth;
    if (i_def->format.video.nSliceHeight == 0)
      i_def->format.video.nSliceHeight = i_def->format.video.nFrameHeight;

    err = super_SetParameter (typeOf (ap_obj, "vp8dinport"), ap_obj,
                              ap_hdl, a_index, ap_struct);
    if (err == OMX_ErrorNone) {
      tiz_port_t * p_obj = (tiz_port_t *) ap_obj;

      /* Get a locally copy of port def. Useful for the early return above */
      tiz_check_omx (tiz_api_GetParameter (tiz_get_krn (handleOf (p_prc)),
                                           handleOf (p_prc),
                                           OMX_IndexParamPortDefinition,
                                           &(p_prc->port_def_)));

      /* Set desired buffer size that will be used when allocating input buffers */
      p_obj->portdef_.nBufferSize = p_def->nFrameWidth * p_def->nFrameHeight * 2;
    }
  }

  return err;
}

/*
 * vp8dinport_class
 */

static void *
vp8d_inport_class_ctor (void * ap_obj, va_list * app)
{
   /* NOTE: Class methods might be added in the future. None for now. */
   return super_ctor (typeOf (ap_obj, "vp8dinport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
vp8d_inport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizvp8port = tiz_get_type(ap_hdl, "tizvp8port");
  void * vp8dinport_class
    = factory_new(classOf(tizvp8port), "vp8dinport_class",
                  classOf(tizvp8port), sizeof(vp8d_inport_class_t),
                  ap_tos, ap_hdl, ctor, vp8d_inport_class_ctor, 0);
  return vp8dinport_class;
}

void *
vp8d_inport_init (void * ap_tos, void * ap_hdl)
{
  void * tizvp8port = tiz_get_type (ap_hdl, "tizvp8port");
  void * vp8dinport_class = tiz_get_type (ap_hdl, "vp8dinport_class");
  void * vp8dinport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (vp8dinport_class, "vp8dinport", tizvp8port,
    sizeof (vp8d_inport_t),
    /* TIZ_CLASS_COMMENT: class constructor */
    ap_tos, ap_hdl,
    /* TIZ_CLASS_COMMENT: class constructor */
    ctor, vp8d_inport_ctor,
    /* TIZ_CLASS_COMMENT: class destructor */
    dtor, vp8d_inport_dtor,
    /* TIZ_CLASS_COMMENT: */
    tiz_api_SetParameter, vp8d_inport_SetParameter,
    /* TIZ_CLASS_COMMENT: stop value*/
    0);

   return vp8dinport;
}
