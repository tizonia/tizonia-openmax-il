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

#include <assert.h>

#include <tizplatform.h>

#include "tizutils.h"
#include "tizaudioport.h"
#include "tizaudioport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.audioport"
#endif

static inline OMX_ERRORTYPE
update_audio_coding_type (void *ap_obj,
                          OMX_AUDIO_CODINGTYPE a_encoding)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tiz_audioport_t *p_obj = ap_obj;
  tiz_port_t *p_base = ap_obj;

  assert (NULL != ap_obj);

  if (a_encoding >= OMX_AUDIO_CodingMax)
    {
      TIZ_ERROR (handleOf (ap_obj), "[OMX_ErrorBadParameter] : "
                 "(Bad encoding [0x%08x]...)", a_encoding);
      rc = OMX_ErrorBadParameter;
      goto end;
    }

  if (!tiz_vector_find (p_obj->p_encodings_, &a_encoding))
    {
      TIZ_ERROR (handleOf (ap_obj), "[OMX_ErrorUnsupportedSetting] : "
                 "(Encoding not supported [0x%08x]...)", a_encoding);
      rc = OMX_ErrorUnsupportedSetting;
      goto end;
    }

  /* All well */

  /* Update this port's OMX_AUDIO_PARAM_PORTFORMATTYPE structure */
  p_obj->port_format_.eEncoding = a_encoding;

  /* Now update the base class' OMX_PARAM_PORTDEFINITIONTYPE */
  p_base->portdef_.format.audio.eEncoding = a_encoding;

 end:

  return rc;
}

/*
 * tizaudioport class
 */

static void *
audioport_ctor (void *ap_obj, va_list * app)
{
  tiz_audioport_t *p_obj = super_ctor (typeOf (ap_obj, "tizaudioport"), ap_obj, app);
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
          TIZ_TRACE (handleOf (ap_obj),
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
  return super_dtor (typeOf (ap_obj, "tizaudioport"), ap_obj);
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

  TIZ_TRACE (ap_hdl, "PORT [%d] GetParameter [%s]...",
            tiz_port_index (ap_obj), tiz_idx_to_str (a_index));
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
        TIZ_TRACE (ap_hdl, "Encoding [0x%08x]...", *p_encoding);
      }
      break;

    default:
      {
        /* Try the parent's indexes */
        return super_GetParameter (typeOf (ap_obj, "tizaudioport"),
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
  OMX_ERRORTYPE    rc    = OMX_ErrorNone;

  TIZ_TRACE (ap_hdl, "PORT [%d] SetParameter [%s]...",
            tiz_port_index (ap_obj), tiz_idx_to_str (a_index));
  assert (NULL != p_obj);

  switch (a_index)
    {
    case OMX_IndexParamAudioPortFormat:
      {

        const OMX_AUDIO_PARAM_PORTFORMATTYPE *p_audio_format
          = (OMX_AUDIO_PARAM_PORTFORMATTYPE *) ap_struct;
        OMX_AUDIO_CODINGTYPE encoding = p_audio_format->eEncoding;
        TIZ_TRACE (ap_hdl, "Setting new audio encoding "
                 "[0x%08x]...", encoding);
        rc = update_audio_coding_type (p_obj, encoding);
      }
      break;

    default:
      {
        /* Try the parent's indexes */
        rc = super_SetParameter (typeOf (ap_obj, "tizaudioport"),
                                 ap_obj, ap_hdl, a_index, ap_struct);
      }
    };

  return rc;
}

static OMX_ERRORTYPE
audioport_set_portdef_format (void *ap_obj,
                              const OMX_PARAM_PORTDEFINITIONTYPE * ap_pdef)
{
  tiz_audioport_t *p_obj = ap_obj;
  tiz_port_t *p_base = ap_obj;

  assert (NULL != p_obj);
  assert (NULL != ap_pdef);

  p_base->portdef_.format.audio.pNativeRender = ap_pdef->format.audio.pNativeRender;
  p_base->portdef_.format.audio.bFlagErrorConcealment = ap_pdef->format.audio.bFlagErrorConcealment;

  TIZ_TRACE (handleOf (ap_obj), "PORT [%d] audio.eEncoding [%d]",
             tiz_port_index (ap_obj), ap_pdef->format.audio.eEncoding);

  return update_audio_coding_type (p_obj, ap_pdef->format.audio.eEncoding);
}

/*
 * tizaudioport_class
 */

static void *
audioport_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizaudioport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_audioport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizaudioport_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizport), "tizaudioport_class", classOf (tizport), sizeof (tiz_audioport_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, audioport_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizaudioport_class;
}

void *
tiz_audioport_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizaudioport_class = tiz_get_type (ap_hdl, "tizaudioport_class");
  void * tizaudioport =
    factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizaudioport_class, "tizaudioport", tizport, sizeof (tiz_audioport_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, audioport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, audioport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, audioport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, audioport_SetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_port_set_portdef_format, audioport_set_portdef_format,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizaudioport;
}
