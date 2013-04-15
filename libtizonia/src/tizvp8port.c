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
 * @file   tizvp8port.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  vp8port class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizvp8port.h"
#include "tizvp8port_decls.h"

#include "tizutils.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.vp8port"
#endif

/*
 * tizvp8port class
 */

static void *
vp8port_ctor (void *ap_obj, va_list * app)
{
  struct tizvp8port *p_obj = super_ctor (tizvp8port, ap_obj, app);
  tiz_port_t *p_base = ap_obj;
  OMX_VIDEO_PARAM_VP8TYPE *p_vp8type = NULL;
  OMX_VIDEO_VP8LEVELTYPE *p_levels = NULL;
  OMX_VIDEO_PARAM_BITRATETYPE *p_pbrtype = NULL;
  OMX_U32 i = 0;

  assert (NULL != app);

  tiz_port_register_index (p_obj, OMX_IndexParamVideoVp8);
  tiz_port_register_index (p_obj, OMX_IndexParamVideoProfileLevelCurrent);
  tiz_port_register_index (p_obj,
                          OMX_IndexParamVideoProfileLevelQuerySupported);

  /* Register additional indexes used when the port is instantiated as an
   * output port during encoding */
  if (OMX_DirOutput == p_base->portdef_.eDir)
    {
      tiz_port_register_index (p_obj, OMX_IndexParamVideoBitrate);
      tiz_port_register_index (p_obj, OMX_IndexConfigVideoBitrate);
      tiz_port_register_index (p_obj, OMX_IndexConfigVideoFramerate);
    }

  /* Initialize the OMX_VIDEO_PARAM_VP8TYPE structure */
  if (NULL != (p_vp8type = va_arg (*app, OMX_VIDEO_PARAM_VP8TYPE *)))
    {
      p_obj->vp8type_ = *p_vp8type;
    }

  /* Supported VP8 codec levels can be passed to this class. */
  tiz_vector_init (&(p_obj->p_levels_), sizeof (OMX_VIDEO_VP8LEVELTYPE));

  if (NULL != (p_levels = va_arg (*app, OMX_VIDEO_VP8LEVELTYPE *)))
    {
      while (OMX_VIDEO_VP8LevelMax != p_levels[i])
        {
          TIZ_LOG (TIZ_TRACE, "p_levels[%u] = [%d]...", i, p_levels[i]);
          tiz_vector_push_back (p_obj->p_levels_, &p_levels[i++]);
        }
    }

  /* This is based on the defaults defined in the standard for the VP8 decoder
   * component */
  p_obj->pltype_.nSize = sizeof (OMX_VIDEO_PARAM_PROFILELEVELTYPE);
  p_obj->pltype_.nVersion.nVersion = OMX_VERSION;
  p_obj->pltype_.nPortIndex = p_base->portdef_.nPortIndex;
  p_obj->pltype_.eProfile = OMX_VIDEO_VP8ProfileMain;
  p_obj->pltype_.eLevel = p_levels ? p_levels[0] : OMX_VIDEO_VP8Level_Version0;
  p_obj->pltype_.nIndex = 0;
  p_obj->pltype_.eCodecType = 0;        /* Not applicable */

  /* Init the OMX_VIDEO_PARAM_BITRATETYPE structure, if provided */
  if (NULL != (p_pbrtype = va_arg (*app, OMX_VIDEO_PARAM_BITRATETYPE *)))
    {
      p_obj->pbrtype_ = *p_pbrtype;

      /* Init OMX_VIDEO_CONFIG_BITRATETYPE with the same value
       * provided in OMX_VIDEO_PARAM_BITRATETYPE */
      p_obj->cbrtype_.nSize = p_pbrtype->nSize;
      p_obj->cbrtype_.nVersion.nVersion = p_pbrtype->nVersion.nVersion;
      p_obj->cbrtype_.nPortIndex = p_base->portdef_.nPortIndex;;
      p_obj->cbrtype_.nEncodeBitrate = p_pbrtype->nTargetBitrate;
    }

  /* Init the OMX_CONFIG_FRAMERATETYPE structure */
  /* This is also based on the defaults defined in the standard for the VP8
   * decoder component */
  p_obj->frtype_.nSize = sizeof (OMX_CONFIG_FRAMERATETYPE);
  p_obj->frtype_.nVersion.nVersion = OMX_VERSION;
  p_obj->frtype_.nPortIndex = p_base->portdef_.nPortIndex;
  p_obj->frtype_.xEncodeFramerate = 15;

  return p_obj;
}

static void *
vp8port_dtor (void *ap_obj)
{
  struct tizvp8port *p_obj = ap_obj;
  tiz_vector_clear (p_obj->p_levels_);
  tiz_vector_destroy (p_obj->p_levels_);
  return super_dtor (tizvp8port, ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
vp8port_GetParameter (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tizvp8port *p_obj = ap_obj;
  const tiz_port_t *p_base = ap_obj;

  TIZ_LOG (TIZ_TRACE, "GetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamVideoVp8:
      {
        OMX_VIDEO_PARAM_VP8TYPE *p_vp8type
          = (OMX_VIDEO_PARAM_VP8TYPE *) ap_struct;
        *p_vp8type = p_obj->vp8type_;
      }
      break;

    case OMX_IndexParamVideoBitrate:
      {
        OMX_VIDEO_PARAM_BITRATETYPE *p_pbrtype
          = (OMX_VIDEO_PARAM_BITRATETYPE *) ap_struct;

        /* This index only applies when this is an output port */
        if (OMX_DirOutput != p_base->portdef_.eDir)
          {
            TIZ_LOG (TIZ_ERROR, "OMX_ErrorUnsupportedIndex [%s]",
                     tiz_idx_to_str (a_index));
            return OMX_ErrorUnsupportedIndex;
          }

        *p_pbrtype = p_obj->pbrtype_;
      }
      break;

    case OMX_IndexParamVideoProfileLevelCurrent:
      {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *p_pltype
          = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) ap_struct;
        *p_pltype = p_obj->pltype_;
      }
      break;

    case OMX_IndexParamVideoProfileLevelQuerySupported:
      {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *p_pltype
          = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) ap_struct;
        OMX_U32 index = p_pltype->nIndex;

        if (index >= tiz_vector_length (p_obj->p_levels_))
          {
            return OMX_ErrorNoMore;
          }
        else
          {
            OMX_VIDEO_VP8LEVELTYPE *p_level = NULL;
            *p_pltype = p_obj->pltype_;
            p_pltype->nIndex = index;
            p_level = tiz_vector_at (p_obj->p_levels_, index);
            assert (p_level && *p_level);
            p_pltype->eLevel = *p_level;
            TIZ_LOG (TIZ_TRACE, "Level [0x%08x]...", *p_level);
          }
      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_GetParameter (tizvp8port,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8port_SetParameter (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  struct tizvp8port *p_obj = (struct tizvp8port *) ap_obj;

  TIZ_LOG (TIZ_TRACE, "SetParameter [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamVideoVp8:
      {
        /* This is a read-only index when used on an input port and read-write
         * on an output port. Just ignore when read-only. */
        const tiz_port_t *p_base = ap_obj;
        if (OMX_DirOutput == p_base->portdef_.eDir)
          {
            const OMX_VIDEO_PARAM_VP8TYPE *p_vp8type
              = (OMX_VIDEO_PARAM_VP8TYPE *) ap_struct;
            const OMX_VIDEO_VP8PROFILETYPE profile = p_vp8type->eProfile;
            const OMX_VIDEO_VP8LEVELTYPE level = p_vp8type->eLevel;

            if (profile > OMX_VIDEO_VP8ProfileMain)
              {
                TIZ_LOG (TIZ_TRACE, "OMX_ErrorBadParameter "
                         "(Bad profile [0x%08x]...)", profile);
                return OMX_ErrorBadParameter;
              }

            p_obj->vp8type_.eProfile = profile;

            if (level > OMX_VIDEO_VP8Level_Version3)
              {
                TIZ_LOG (TIZ_TRACE, "OMX_ErrorBadParameter "
                         "(Bad level [0x%08x]...)", level);
                return OMX_ErrorBadParameter;
              }

            p_obj->vp8type_.eLevel = level;

            if (3 < p_vp8type->nDCTPartitions)
              {
                TIZ_LOG (TIZ_TRACE, "OMX_ErrorBadParameter "
                         "(Bad DCT Partition [0x%08x]...)",
                         p_vp8type->nDCTPartitions);
                return OMX_ErrorBadParameter;
              }

            p_obj->vp8type_.nDCTPartitions = p_vp8type->nDCTPartitions;
            p_obj->vp8type_.bErrorResilientMode =
              p_vp8type->bErrorResilientMode;
          }
      }
      break;

    case OMX_IndexParamVideoBitrate:
      {
        const OMX_VIDEO_PARAM_BITRATETYPE *p_pbrtype
          = (OMX_VIDEO_PARAM_BITRATETYPE *) ap_struct;
        /* This index is only supported when used on an output port. */
        const tiz_port_t *p_base = ap_obj;
        if (OMX_DirOutput != p_base->portdef_.eDir)
          {
            TIZ_LOG (TIZ_ERROR, "OMX_ErrorUnsupportedIndex [%s]",
                     tiz_idx_to_str (a_index));
            return OMX_ErrorUnsupportedIndex;
          }

        p_obj->pbrtype_.eControlRate = p_pbrtype->eControlRate;
        p_obj->pbrtype_.nTargetBitrate = p_pbrtype->nTargetBitrate;
      }
      break;

    case OMX_IndexParamVideoProfileLevelCurrent:
      {
        const OMX_VIDEO_PARAM_PROFILELEVELTYPE *p_pltype
          = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) ap_struct;
        /* This index is read-write only when used on an output port. */
        const tiz_port_t *p_base = ap_obj;

        if (OMX_DirOutput == p_base->portdef_.eDir)
          {
            /* TODO: What to do with the profile and level values in
             * OMX_VIDEO_PARAM_VP8TYPE  */
            p_obj->pltype_ = *p_pltype;
          }
      }
      break;

    case OMX_IndexParamVideoProfileLevelQuerySupported:
      {
        /* This is a read-only index for both input and output ports. Simply
         * ignore it here.  */
      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_SetParameter (tizvp8port,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8port_GetConfig (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tizvp8port *p_obj = ap_obj;

  TIZ_LOG (TIZ_TRACE, "GetConfig [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexConfigVideoBitrate:
      {
        OMX_VIDEO_CONFIG_BITRATETYPE *p_cbrtype
          = (OMX_VIDEO_CONFIG_BITRATETYPE *) ap_struct;
        /* This index is only supported when used on an output port. */
        const tiz_port_t *p_base = ap_obj;
        if (OMX_DirOutput != p_base->portdef_.eDir)
          {
            TIZ_LOG (TIZ_ERROR, "OMX_ErrorUnsupportedIndex [%s]",
                     tiz_idx_to_str (a_index));
            return OMX_ErrorUnsupportedIndex;
          }

        *p_cbrtype = p_obj->cbrtype_;
      }
      break;

    case OMX_IndexConfigVideoFramerate:
      {
        OMX_CONFIG_FRAMERATETYPE *p_frtype
          = (OMX_CONFIG_FRAMERATETYPE *) ap_struct;
        /* This index is only supported when used on an output port. */
        const tiz_port_t *p_base = ap_obj;
        if (OMX_DirOutput != p_base->portdef_.eDir)
          {
            TIZ_LOG (TIZ_ERROR, "OMX_ErrorUnsupportedIndex [%s]",
                     tiz_idx_to_str (a_index));
            return OMX_ErrorUnsupportedIndex;
          }

        *p_frtype = p_obj->frtype_;
      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_GetConfig (tizvp8port,
                                ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8port_SetConfig (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  struct tizvp8port *p_obj = (struct tizvp8port *) ap_obj;

  TIZ_LOG (TIZ_TRACE, "SetConfig [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexConfigVideoBitrate:
      {
        const OMX_VIDEO_CONFIG_BITRATETYPE *p_cbrtype
          = (OMX_VIDEO_CONFIG_BITRATETYPE *) ap_struct;
        /* This index is only supported when used on an output port. */
        const tiz_port_t *p_base = ap_obj;
        if (OMX_DirOutput != p_base->portdef_.eDir)
          {
            TIZ_LOG (TIZ_ERROR, "OMX_ErrorUnsupportedIndex [%s]",
                     tiz_idx_to_str (a_index));
            return OMX_ErrorUnsupportedIndex;
          }

        p_obj->cbrtype_.nEncodeBitrate = p_cbrtype->nEncodeBitrate;
      }
      break;

    case OMX_IndexConfigVideoFramerate:
      {
        const OMX_CONFIG_FRAMERATETYPE *p_frtype
          = (OMX_CONFIG_FRAMERATETYPE *) ap_struct;
        /* This index is only supported when used on an output port. */
        const tiz_port_t *p_base = ap_obj;
        if (OMX_DirOutput != p_base->portdef_.eDir)
          {
            TIZ_LOG (TIZ_ERROR, "OMX_ErrorUnsupportedIndex [%s]",
                     tiz_idx_to_str (a_index));
            return OMX_ErrorUnsupportedIndex;
          }

        p_obj->frtype_.xEncodeFramerate = p_frtype->xEncodeFramerate;
      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_SetConfig (tizvp8port,
                                ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
  vp8port_set_portdef_format
  (void *ap_obj, const OMX_PARAM_PORTDEFINITIONTYPE * ap_pdef)
{
  /* TODO */
  return OMX_ErrorNone;
}

static OMX_BOOL
vp8port_check_tunnel_compat (const void *ap_obj,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  tiz_port_t *p_obj = (tiz_port_t *) ap_obj;

  assert (ap_this_def);
  assert (ap_other_def);

  if (ap_other_def->eDomain != ap_this_def->eDomain)
    {
      TIZ_LOG (TIZ_TRACE,
               "port [%d] check_tunnel_compat : "
               "Video domain not found, instead found domain [%d]",
               p_obj->pid_, ap_other_def->eDomain);
      return OMX_FALSE;
    }

  if (ap_other_def->format.video.eCompressionFormat != OMX_VIDEO_CodingUnused)
    {
      if (ap_other_def->format.video.eCompressionFormat != OMX_VIDEO_CodingVP8)
        {
          TIZ_LOG (TIZ_TRACE, "port [%d] check_tunnel_compat : "
                   "VP8 encoding not found, instead found encoding [%d]",
                   p_obj->pid_, ap_other_def->format.video.eCompressionFormat);
          return OMX_FALSE;
        }
    }

  TIZ_LOG (TIZ_TRACE, "port [%d] check_tunnel_compat [OK]", p_obj->pid_);

  return OMX_TRUE;
}

/*
 * tizvp8port_class
 */

static void *
vp8port_class_ctor (void *ap_obj, va_list * app)
{
  struct tizvp8port_class *p_obj = super_ctor (tizvp8port_class, ap_obj, app);
  typedef void (*voidf) ();
  voidf selector;
  va_list ap;
  va_copy (ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      /* voidf method = va_arg (ap, voidf); */
      /*          if (selector == (voidf) tiz_servant_tick) */
      /*             { */
      /*                *(voidf*) & p_obj->tick = method; */
      /*             } */
      /*          else if (selector == (voidf) tiz_servant_enqueue) */
      /*             { */
      /*                *(voidf*) & p_obj->enqueue = method; */
      /*             } */

    }

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

const void *tizvp8port, *tizvp8port_class;

void
init_tizvp8port (void)
{

  if (!tizvp8port_class)
    {
      init_tizvideoport ();
      tizvp8port_class = factory_new (tizvideoport_class,
                                      "tizvp8port_class",
                                      tizvideoport_class,
                                      sizeof (struct tizvp8port_class),
                                      ctor, vp8port_class_ctor, 0);

    }

  if (!tizvp8port)
    {
      init_tizvideoport ();
      tizvp8port =
        factory_new
        (tizvp8port_class,
         "tizvp8port",
         tizvideoport,
         sizeof (struct tizvp8port),
         ctor, vp8port_ctor,
         dtor, vp8port_dtor,
         tiz_api_GetParameter, vp8port_GetParameter,
         tiz_api_SetParameter, vp8port_SetParameter,
         tiz_api_GetConfig, vp8port_GetConfig,
         tiz_api_SetConfig, vp8port_SetConfig,
         tiz_port_set_portdef_format, vp8port_set_portdef_format,
         tiz_port_check_tunnel_compat, vp8port_check_tunnel_compat, 0);
    }

}
