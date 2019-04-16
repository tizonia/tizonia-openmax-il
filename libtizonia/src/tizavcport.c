/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   tizavcport.c
 * @author Gurkirpal Singh
 *
 * @brief  avcport class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizplatform.h>

#include "tizutils.h"
#include "tizavcport.h"
#include "tizavcport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.avcport"
#endif

/*
 * tizavcport class
 */

static void *
avcport_ctor (void * ap_obj, va_list * app)
{
  tiz_avcport_t * p_obj
    = super_ctor (typeOf (ap_obj, "tizavcport"), ap_obj, app);
  tiz_port_t * p_base = ap_obj;
  OMX_VIDEO_PARAM_AVCTYPE * p_avctype = NULL;
  OMX_VIDEO_AVCLEVELTYPE * p_levels = NULL;
  OMX_VIDEO_PARAM_BITRATETYPE * p_pbrtype = NULL;
  OMX_VIDEO_PARAM_QUANTIZATIONTYPE * p_pqtype = NULL;

  assert (app);

  tiz_port_register_index (p_obj, OMX_IndexParamVideoAvc);
  tiz_port_register_index (p_obj,
                           OMX_IndexParamVideoProfileLevelQuerySupported);
  tiz_port_register_index (p_obj, OMX_IndexParamVideoProfileLevelCurrent);
  tiz_port_register_index (p_obj, OMX_IndexParamVideoQuantization);

  /* Register additional indexes used when the port is instantiated as an
   * output port during encoding */
  if (OMX_DirOutput == p_base->portdef_.eDir)
    {
      tiz_port_register_index (p_obj, OMX_IndexParamVideoBitrate);
      tiz_port_register_index (p_obj, OMX_IndexConfigVideoFramerate);
      tiz_port_register_index (p_obj, OMX_IndexConfigVideoBitrate);
    }

  /* Initialize the OMX_VIDEO_PARAM_AVCTYPE structure */
  TIZ_INIT_OMX_PORT_STRUCT(p_obj->avctype_, p_base->portdef_.nPortIndex);
  if ((p_avctype = va_arg (*app, OMX_VIDEO_PARAM_AVCTYPE *)))
    {
      p_obj->avctype_ = *p_avctype;
    }

  /* Supported AVC codec levels can be passed to this class. */
  tiz_vector_init (&(p_obj->p_levels_), sizeof (OMX_VIDEO_AVCLEVELTYPE));

  if ((p_levels = va_arg (*app, OMX_VIDEO_AVCLEVELTYPE *)))
    {
      OMX_U32 i = 0;
      while (OMX_VIDEO_AVCLevelMax != p_levels[i])
        {
          tiz_vector_push_back (p_obj->p_levels_, &p_levels[i++]);
        }
    }

  /* This is based on the defaults defined in the standard for the AVC decoder
   * component */
  p_obj->pltype_.nSize = sizeof (OMX_VIDEO_PARAM_PROFILELEVELTYPE);
  p_obj->pltype_.nVersion.nVersion = OMX_VERSION;
  p_obj->pltype_.nPortIndex = p_base->portdef_.nPortIndex;
  p_obj->pltype_.eProfile = OMX_VIDEO_AVCProfileBaseline;
  p_obj->pltype_.eLevel = p_levels ? p_levels[0] : OMX_VIDEO_AVCLevel1;
  p_obj->pltype_.nIndex = 0;
  p_obj->pltype_.eCodecType = 0; /* Not applicable */

  /* Init the OMX_VIDEO_PARAM_BITRATETYPE structure, if provided */
  TIZ_INIT_OMX_PORT_STRUCT(p_obj->pbrtype_, p_base->portdef_.nPortIndex);
  TIZ_INIT_OMX_PORT_STRUCT(p_obj->cbrtype_, p_base->portdef_.nPortIndex);
  if ((p_pbrtype = va_arg (*app, OMX_VIDEO_PARAM_BITRATETYPE *)))
    {
      p_obj->pbrtype_ = *p_pbrtype;

      /* Init OMX_VIDEO_CONFIG_BITRATETYPE with the same value
       * provided in OMX_VIDEO_PARAM_BITRATETYPE */
      p_obj->cbrtype_.nSize = p_pbrtype->nSize;
      p_obj->cbrtype_.nVersion.nVersion = p_pbrtype->nVersion.nVersion;
      p_obj->cbrtype_.nPortIndex = p_base->portdef_.nPortIndex;
      p_obj->cbrtype_.nEncodeBitrate = p_pbrtype->nTargetBitrate;
    }

  /* Init the OMX_VIDEO_PARAM_QUANTIZATIONTYPE structure, if provided */
  TIZ_INIT_OMX_PORT_STRUCT(p_obj->pqtype_, p_base->portdef_.nPortIndex);
  if ((p_pqtype = va_arg (*app, OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)))
    {
      p_obj->pqtype_ = *p_pqtype;
    }

  /* Init the OMX_CONFIG_FRAMERATETYPE structure */
  /* This is also based on the defaults defined in the standard for the AVC
   * decoder component */
  p_obj->frtype_.nSize = sizeof (OMX_CONFIG_FRAMERATETYPE);
  p_obj->frtype_.nVersion.nVersion = OMX_VERSION;
  p_obj->frtype_.nPortIndex = p_base->portdef_.nPortIndex;
  p_obj->frtype_.xEncodeFramerate = 15;

  return p_obj;
}

static void *
avcport_dtor (void * ap_obj)
{
  tiz_avcport_t * p_obj = ap_obj;
  tiz_vector_clear (p_obj->p_levels_);
  tiz_vector_destroy (p_obj->p_levels_);
  return super_dtor (typeOf (ap_obj, "tizavcport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
avcport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_avcport_t * p_obj = ap_obj;
  const tiz_port_t * p_base = ap_obj;

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));
  assert (p_obj);

  switch (a_index)
    {
      case OMX_IndexParamVideoAvc:
        {
          OMX_VIDEO_PARAM_AVCTYPE * p_avctype
            = (OMX_VIDEO_PARAM_AVCTYPE *) ap_struct;
          *p_avctype = p_obj->avctype_;
        }
        break;

      case OMX_IndexParamVideoBitrate:
        {
          OMX_VIDEO_PARAM_BITRATETYPE * p_pbrtype
            = (OMX_VIDEO_PARAM_BITRATETYPE *) ap_struct;

          /* This index only applies when this is an output port */
          if (OMX_DirOutput != p_base->portdef_.eDir)
            {
              TIZ_ERROR (ap_hdl, "OMX_ErrorUnsupportedIndex [%s]",
                         tiz_idx_to_str (a_index));
              return OMX_ErrorUnsupportedIndex;
            }

          *p_pbrtype = p_obj->pbrtype_;
        }
        break;

      case OMX_IndexParamVideoProfileLevelCurrent:
        {
          OMX_VIDEO_PARAM_PROFILELEVELTYPE * p_pltype
            = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) ap_struct;
          *p_pltype = p_obj->pltype_;
        }
        break;

      case OMX_IndexParamVideoProfileLevelQuerySupported:
        {
          OMX_VIDEO_PARAM_PROFILELEVELTYPE * p_pltype
            = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) ap_struct;
          OMX_U32 index = p_pltype->nIndex;

          if (index >= tiz_vector_length (p_obj->p_levels_))
            {
              return OMX_ErrorNoMore;
            }
          else
            {
              OMX_VIDEO_AVCLEVELTYPE * p_level = NULL;
              *p_pltype = p_obj->pltype_;
              p_pltype->nIndex = index;
              p_level = tiz_vector_at (p_obj->p_levels_, index);
              assert (p_level && *p_level);
              p_pltype->eLevel = *p_level;
              TIZ_TRACE (ap_hdl, "Level [0x%08x]...", *p_level);
            }
        }
        break;

      case OMX_IndexParamVideoQuantization:
        {
          OMX_VIDEO_PARAM_QUANTIZATIONTYPE * p_pqtype
            = (OMX_VIDEO_PARAM_QUANTIZATIONTYPE *) ap_struct;

          *p_pqtype = p_obj->pqtype_;
        }
        break;

      default:
        {
          /* Try the parent's indexes */
          return super_GetParameter (typeOf (ap_obj, "tizavcport"), ap_obj,
                                     ap_hdl, a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
avcport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_avcport_t * p_obj = (tiz_avcport_t *) ap_obj;

  TIZ_TRACE (ap_hdl, "PORT [%d] SetParameter [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));
  assert (p_obj);
  assert (ap_struct);

  switch (a_index)
    {
      case OMX_IndexParamVideoAvc:
        {
          /* This is a read-only index when used on an input port and read-write
         * on an output port. Just ignore when read-only. */
          const tiz_port_t * p_base = ap_obj;
          if (OMX_DirOutput == p_base->portdef_.eDir)
            {
              const OMX_VIDEO_PARAM_AVCTYPE * p_avctype
                = (OMX_VIDEO_PARAM_AVCTYPE *) ap_struct;
              const OMX_VIDEO_AVCPROFILETYPE profile = p_avctype->eProfile;
              const OMX_VIDEO_AVCLEVELTYPE level = p_avctype->eLevel;
              const OMX_VIDEO_AVCLOOPFILTERTYPE loopFilterMode
                = p_avctype->eLoopFilterMode;

              /* Check profile validity */
              if (profile > OMX_VIDEO_AVCProfileHigh444)
                {
                  TIZ_ERROR (ap_hdl,
                             "[OMX_ErrorBadParameter] : "
                             "(Bad profile [0x%08x]...)",
                             profile);
                  return OMX_ErrorBadParameter;
                }

              /* Check level validity */
              if (level > OMX_VIDEO_AVCLevel51)
                {
                  TIZ_ERROR (ap_hdl,
                             "[OMX_ErrorBadParameter] : "
                             "(Bad level [0x%08x]...)",
                             level);
                  return OMX_ErrorBadParameter;
                }

              /* Check filter mode */
              if (loopFilterMode > OMX_VIDEO_AVCLoopFilterDisableSliceBoundary)
                {
                  TIZ_ERROR (ap_hdl,
                             "[OMX_ErrorBadParameter] : "
                             "(Bad loop filter mode [0x%08x]...)",
                             loopFilterMode);
                  return OMX_ErrorBadParameter;
                }

              /* All standard-compliancy checks are done. Now simply copy the
                 received struct (the decoder/encoder processor may want to do
                 additional checks). */
              p_obj->avctype_ = *p_avctype;
            }
          else
            {
              TIZ_NOTICE (ap_hdl, "Ignoring read-only index [%s] ",
                          tiz_idx_to_str (a_index));
            }
        }
        break;

      case OMX_IndexParamVideoBitrate:
        {
          const OMX_VIDEO_PARAM_BITRATETYPE * p_pbrtype
            = (OMX_VIDEO_PARAM_BITRATETYPE *) ap_struct;
          /* This index is only supported when used on an output port. */
          const tiz_port_t * p_base = ap_obj;
          if (OMX_DirOutput != p_base->portdef_.eDir)
            {
              TIZ_ERROR (ap_hdl, "OMX_ErrorUnsupportedIndex [%s]",
                         tiz_idx_to_str (a_index));
              return OMX_ErrorUnsupportedIndex;
            }

          p_obj->pbrtype_.eControlRate = p_pbrtype->eControlRate;
          p_obj->pbrtype_.nTargetBitrate = p_pbrtype->nTargetBitrate;
        }
        break;

      case OMX_IndexParamVideoProfileLevelCurrent:
        {
          const OMX_VIDEO_PARAM_PROFILELEVELTYPE * p_pltype
            = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) ap_struct;
          /* This index is read-write only when used on an output port. */
          const tiz_port_t * p_base = ap_obj;

          if (OMX_DirOutput == p_base->portdef_.eDir)
            {
              /* TODO: What to do with the profile and level values in
             * OMX_VIDEO_PARAM_AVCTYPE  */
              p_obj->pltype_ = *p_pltype;
            }
        }
        break;

      case OMX_IndexParamVideoProfileLevelQuerySupported:
        {
          /* This is a read-only index for both input and output ports. Simply
         * ignore it here.  */
          TIZ_NOTICE (ap_hdl, "Ignoring read-only index [%s] ",
                      tiz_idx_to_str (a_index));
        }
        break;

      case OMX_IndexParamVideoQuantization:
        {
          const OMX_VIDEO_PARAM_QUANTIZATIONTYPE * p_pqtype
            = (OMX_VIDEO_PARAM_QUANTIZATIONTYPE *) ap_struct;

          p_obj->pqtype_.nQpI = p_pqtype->nQpI;
          p_obj->pqtype_.nQpP = p_pqtype->nQpP;
          p_obj->pqtype_.nQpB = p_pqtype->nQpB;
        }
        break;

      default:
        {
          /* Try the parent's indexes */
          return super_SetParameter (typeOf (ap_obj, "tizavcport"), ap_obj,
                                     ap_hdl, a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
avcport_GetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_avcport_t * p_obj = ap_obj;

  TIZ_TRACE (ap_hdl, "PORT [%d] GetConfig [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));
  assert (p_obj);

  switch (a_index)
    {
      case OMX_IndexConfigVideoBitrate:
        {
          OMX_VIDEO_CONFIG_BITRATETYPE * p_cbrtype
            = (OMX_VIDEO_CONFIG_BITRATETYPE *) ap_struct;
          /* This index is only supported when used on an output port. */
          const tiz_port_t * p_base = ap_obj;
          if (OMX_DirOutput != p_base->portdef_.eDir)
            {
              TIZ_ERROR (ap_hdl, "[OMX_ErrorUnsupportedIndex] : [%s]",
                         tiz_idx_to_str (a_index));
              return OMX_ErrorUnsupportedIndex;
            }

          *p_cbrtype = p_obj->cbrtype_;
        }
        break;

      case OMX_IndexConfigVideoFramerate:
        {
          OMX_CONFIG_FRAMERATETYPE * p_frtype
            = (OMX_CONFIG_FRAMERATETYPE *) ap_struct;
          /* This index is only supported when used on an output port. */
          const tiz_port_t * p_base = ap_obj;
          if (OMX_DirOutput != p_base->portdef_.eDir)
            {
              TIZ_ERROR (ap_hdl, "OMX_ErrorUnsupportedIndex [%s]",
                         tiz_idx_to_str (a_index));
              return OMX_ErrorUnsupportedIndex;
            }

          *p_frtype = p_obj->frtype_;
        }
        break;

      default:
        {
          /* Try the parent's indexes */
          return super_GetConfig (typeOf (ap_obj, "tizavcport"), ap_obj, ap_hdl,
                                  a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
avcport_SetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_avcport_t * p_obj = (tiz_avcport_t *) ap_obj;

  TIZ_TRACE (ap_hdl, "PORT [%d] SetConfig [%s]...", tiz_port_index (ap_obj),
             tiz_idx_to_str (a_index));
  assert (p_obj);

  switch (a_index)
    {
      case OMX_IndexConfigVideoBitrate:
        {
          const OMX_VIDEO_CONFIG_BITRATETYPE * p_cbrtype
            = (OMX_VIDEO_CONFIG_BITRATETYPE *) ap_struct;
          /* This index is only supported when used on an output port. */
          const tiz_port_t * p_base = ap_obj;
          if (OMX_DirOutput != p_base->portdef_.eDir)
            {
              TIZ_ERROR (ap_hdl, "[OMX_ErrorUnsupportedIndex] : [%s]",
                         tiz_idx_to_str (a_index));
              return OMX_ErrorUnsupportedIndex;
            }

          p_obj->cbrtype_.nEncodeBitrate = p_cbrtype->nEncodeBitrate;
        }
        break;

      case OMX_IndexConfigVideoFramerate:
        {
          const OMX_CONFIG_FRAMERATETYPE * p_frtype
            = (OMX_CONFIG_FRAMERATETYPE *) ap_struct;
          /* This index is only supported when used on an output port. */
          const tiz_port_t * p_base = ap_obj;
          if (OMX_DirOutput != p_base->portdef_.eDir)
            {
              TIZ_ERROR (ap_hdl, "OMX_ErrorUnsupportedIndex [%s]",
                         tiz_idx_to_str (a_index));
              return OMX_ErrorUnsupportedIndex;
            }

          p_obj->frtype_.xEncodeFramerate = p_frtype->xEncodeFramerate;
        }
        break;

      default:
        {
          /* Try the parent's indexes */
          return super_SetConfig (typeOf (ap_obj, "tizavcport"), ap_obj, ap_hdl,
                                  a_index, ap_struct);
        }
    };

  return OMX_ErrorNone;
}

static bool
avcport_check_tunnel_compat (const void * ap_obj,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t * p_obj = (tiz_port_t *) ap_obj;

  assert (ap_this_def);
  assert (ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_ERROR (
        handleOf (ap_obj),
        "PORT [%d] : Video domain not found, instead found domain [%d]",
        p_obj->pid_, ap_other_def->eDomain);
      return false;
    }

  if (ap_other_def->format.video.eCompressionFormat != OMX_VIDEO_CodingUnused)
    {
      if (ap_other_def->format.video.eCompressionFormat != OMX_VIDEO_CodingAVC)
        {
          TIZ_ERROR (
            handleOf (ap_obj),
            "PORT [%d] : AVC encoding not found, instead found encoding [%d]",
            p_obj->pid_, ap_other_def->format.video.eCompressionFormat);
          return false;
        }
    }

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] check_tunnel_compat [OK]",
             p_obj->pid_);

  return true;
}

/*
 * tizavcport_class
 */

static void *
avcport_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizavcport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_avcport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizvideoport = tiz_get_type (ap_hdl, "tizvideoport");
  void * tizavcport_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizvideoport), "tizavcport_class", classOf (tizvideoport),
     sizeof (tiz_avcport_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, avcport_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizavcport_class;
}

void *
tiz_avcport_init (void * ap_tos, void * ap_hdl)
{
  void * tizvideoport = tiz_get_type (ap_hdl, "tizvideoport");
  void * tizavcport_class = tiz_get_type (ap_hdl, "tizavcport_class");
  TIZ_LOG_CLASS (tizavcport_class);
  void * tizavcport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizavcport_class, "tizavcport", tizvideoport, sizeof (tiz_avcport_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, avcport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, avcport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, avcport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, avcport_SetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetConfig, avcport_GetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetConfig, avcport_SetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_check_tunnel_compat, avcport_check_tunnel_compat,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizavcport;
}
