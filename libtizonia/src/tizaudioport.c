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
 * @file   tizaudioport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - audioport class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizaudioport.h"
#include "tizaudioport_decls.h"
#include "tizutils.h"

#include "tizosal.h"

#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.audioport"
#endif

/*
 * tizaudioport class
 */

static void *
audioport_ctor (void *ap_obj, va_list * app)
{
  tiz_audioport_t *p_obj = super_ctor (tizaudioport, ap_obj, app);
  OMX_AUDIO_CODINGTYPE *p_encodings = NULL;

  assert (NULL != p_obj);

  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexParamAudioPortFormat));

  tiz_check_omx_err_ret_null
    (tiz_vector_init (&(p_obj->p_encodings_), sizeof (OMX_AUDIO_CODINGTYPE)));

  /* Initialize the OMX_AUDIO_PARAM_PORTFORMATTYPE structure */
  p_obj->port_format_.nSize
    = (OMX_U32) sizeof (OMX_AUDIO_PARAM_PORTFORMATTYPE);
  p_obj->port_format_.nVersion.nVersion = (OMX_U32) OMX_VERSION;
  p_obj->port_format_.nIndex            = 0;

  if (NULL != (p_encodings = va_arg (*app, OMX_AUDIO_CODINGTYPE *)))
    {
      OMX_U32 i = 0;
      while (OMX_AUDIO_CodingMax != p_encodings[i])
        {
          TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_obj),
                    "p_encodings[%u] = [%d]...", i, p_encodings[i]);
          tiz_check_omx_err_ret_null
            (tiz_vector_push_back (p_obj->p_encodings_, &p_encodings[i++]));
        }
    }

  p_obj->port_format_.eEncoding = p_encodings
    ? p_encodings[0] : OMX_AUDIO_CodingUnused;

  return p_obj;
}

static void *
audioport_dtor (void *ap_obj)
{
  tiz_audioport_t *p_obj = ap_obj;
  assert (NULL != p_obj);
  tiz_vector_clear (p_obj->p_encodings_);
  tiz_vector_destroy (p_obj->p_encodings_);
  return super_dtor (tizaudioport, ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
audioport_GetParameter (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_audioport_t *p_obj = ap_obj;

  TIZ_LOGN (TIZ_TRACE, "GetParameter [%s]...", ap_hdl, tiz_idx_to_str (a_index));
  assert (NULL != ap_obj);

  switch (a_index)
    {
    case OMX_IndexParamAudioPortFormat:
      {
        OMX_AUDIO_PARAM_PORTFORMATTYPE *p_pft = ap_struct;
        OMX_AUDIO_CODINGTYPE *p_encoding = NULL;

        if ((OMX_S32) p_pft->nIndex >= tiz_vector_length (p_obj->p_encodings_))
          {
            return OMX_ErrorNoMore;
          }

        p_encoding = tiz_vector_at (p_obj->p_encodings_, (OMX_S32) p_pft->nIndex);
        assert (NULL != p_encoding);
        p_pft->eEncoding = *p_encoding;
        TIZ_LOGN (TIZ_TRACE, ap_hdl, "Encoding [0x%08x]...", *p_encoding);
      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_GetParameter (tizaudioport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
audioport_SetParameter (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_audioport_t *p_obj = (tiz_audioport_t *) ap_obj;

  TIZ_LOGN (TIZ_TRACE, ap_hdl, "SetParameter [%s]...",
            tiz_idx_to_str (a_index));
  assert (NULL != p_obj);

  switch (a_index)
    {
    case OMX_IndexParamAudioPortFormat:
      {

        const OMX_AUDIO_PARAM_PORTFORMATTYPE *p_audio_format
          = (OMX_AUDIO_PARAM_PORTFORMATTYPE *) ap_struct;
        OMX_AUDIO_CODINGTYPE encoding = p_audio_format->eEncoding;

        if (encoding >= OMX_AUDIO_CodingMax)
          {
            TIZ_LOGN (TIZ_TRACE, ap_hdl, "OMX_ErrorBadParameter "
                     "(Bad encoding [0x%08x]...)", encoding);
            return OMX_ErrorBadParameter;
          }

        if (!tiz_vector_find (p_obj->p_encodings_, &encoding))
          {
            TIZ_LOGN (TIZ_TRACE, ap_hdl, "OMX_ErrorUnsupportedSetting "
                     "(Encoding not supported [0x%08x]...)", encoding);
            return OMX_ErrorUnsupportedSetting;
          }

        p_obj->port_format_.eEncoding = encoding;

        TIZ_LOGN (TIZ_TRACE, ap_hdl, "Set new audio encoding "
                 "[0x%08x]...", encoding);

      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_SetParameter (tizaudioport,
                                   ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return OMX_ErrorNone;

}

/*
 * tizaudioport_class
 */

static void *
audioport_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (tizaudioport_class, ap_obj, app);
}

/*
 * initialization
 */

const void *tizaudioport, *tizaudioport_class;

OMX_ERRORTYPE
tiz_audioport_init (void)
{
  if (!tizaudioport_class)
    {
      tiz_check_omx_err_ret_oom (tiz_port_init ());
      tiz_check_null_ret_oom
        (tizaudioport_class = factory_new (tizport_class,
                                           "tizaudioport_class",
                                           tizport_class,
                                           sizeof (tiz_audioport_class_t),
                                           ctor, audioport_class_ctor, 0));

    }

  if (!tizaudioport)
    {
      tiz_check_omx_err_ret_oom (tiz_port_init ());
      tiz_check_null_ret_oom
        (tizaudioport =
         factory_new
         (tizaudioport_class,
          "tizaudioport",
          tizport,
          sizeof (tiz_audioport_t),
          ctor, audioport_ctor,
          dtor, audioport_dtor,
          tiz_api_GetParameter, audioport_GetParameter,
          tiz_api_SetParameter, audioport_SetParameter, 0));
    }
  return OMX_ErrorNone;
}
