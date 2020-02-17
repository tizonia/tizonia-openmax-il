/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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
 * @file   tizobjsys.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Object lifetime management - implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <tizplatform.h>

#include "tizobjsys.h"

#include "tizobject.h"
#include "tizobject_decls.h"
#include "tizapi.h"
#include "tizservant.h"
#include "tizprc.h"
#include "tizfilterprc.h"
#include "tizkernel.h"
#include "tizfsm.h"
#include "tizstate.h"
#include "tizloaded.h"
#include "tizidletoloaded.h"
#include "tizidle.h"
#include "tizidletoloaded.h"
#include "tizidletoexecuting.h"
#include "tizpause.h"
#include "tizpausetoidle.h"
#include "tizwaitforresources.h"
#include "tizexecuting.h"
#include "tizexecutingtoidle.h"
#include "tizport.h"
#include "tizconfigport.h"
#include "tizaudioport.h"
#include "tizpcmport.h"
#include "tizmp2port.h"
#include "tizmp3port.h"
#include "tizaacport.h"
#include "tizvorbisport.h"
#include "tizopusport.h"
#include "tizflacport.h"
#include "tizvideoport.h"
#include "tizimageport.h"
#include "tizotherport.h"
#include "tizvp8port.h"
#include "tizavcport.h"
#include "tizivrport.h"
#include "tizbinaryport.h"
#include "tizdemuxerport.h"
#include "tizmuxerport.h"
#include "tizmp4port.h"
#include "tizoggport.h"
#include "tizuricfgport.h"
#include "tizdemuxercfgport.h"
#include "tizwebmport.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.objsys"
#endif

struct tiz_os
{
  tiz_map_t * p_map;
  OMX_HANDLETYPE p_hdl;
  tiz_soa_t * p_soa;
};

typedef enum tiz_os_type tiz_os_type_t;
enum tiz_os_type
{
  ETIZClass,
  ETIZObject,
  ETIZApi_class,
  ETIZApi,
  ETIZSrv_class,
  ETIZSrv,
  ETIZPrc_class,
  ETIZPrc,
  ETIZFilterprc_class,
  ETIZFilterprc,
  ETIZKrn_class,
  ETIZKrn,
  ETIZFsm_class,
  ETIZFsm,
  ETIZState_class,
  ETIZState,
  ETIZLoaded_class,
  ETIZLoaded,
  ETIZLoadedtoidle_class,
  ETIZLoadedtoidle,
  ETIZIdle_class,
  ETIZIdle,
  ETIZIdletoloaded_class,
  ETIZIdletoloaded,
  ETIZIdletoexecuting_class,
  ETIZIdletoexecuting,
  ETIZPause_class,
  ETIZPause,
  ETIZPausetoidle_class,
  ETIZPausetoidle,
  ETIZWaitforresources_class,
  ETIZWaitforresources,
  ETIZExecuting_class,
  ETIZExecuting,
  ETIZExecutingtoidle_class,
  ETIZExecutingtoidle,
  ETIZPort_class,
  ETIZPort,
  ETIZConfigport_class,
  ETIZConfigport, /* TIZ_OS_BASE_TYPE_END */
  ETIZAudioport_class,
  ETIZAudioport,
  ETIZPcmport_class,
  ETIZPcmport,
  ETIZMp2port_class,
  ETIZMp2port,
  ETIZMp3port_class,
  ETIZMp3port,
  ETIZAacport_class,
  ETIZAacport,
  ETIZVorbisport_class,
  ETIZVorbisport,
  ETIZOpusport_class,
  ETIZOpusport,
  ETIZFlacport_class,
  ETIZFlacport,
  ETIZVideoport_class,
  ETIZVideoport,
  ETIZImageport_class,
  ETIZImageport,
  ETIZOtherport_class,
  ETIZOtherport,
  ETIZVp8port_class,
  ETIZVp8port,
  ETIZAvcport_class,
  ETIZAvcport,
  ETIZIvrport_class,
  ETIZIvrport,
  ETIZBinaryport_class,
  ETIZBinaryport,
  ETIZDemuxerport_class,
  ETIZDemuxerport,
  ETIZMuxerport_class,
  ETIZMuxerport,
  ETIZWebmport_class,
  ETIZWebmport,
  ETIZOggport_class,
  ETIZOggport,
  ETIZUricfgport_class,
  ETIZUricfgport,
  ETIZDemuxercfgport_class,
  ETIZDemuxercfgport,
  ETIZMp4port_class,
  ETIZMp4port,
};

#define TIZ_OS_BASE_TYPE_END ETIZConfigport

static const tiz_os_type_init_f tiz_os_type_to_fnt_tbl[] = {
  tiz_class_init,
  tiz_object_init,
  tiz_api_class_init,
  tiz_api_init,
  tiz_srv_class_init,
  tiz_srv_init,
  tiz_prc_class_init,
  tiz_prc_init,
  tiz_filter_prc_class_init,
  tiz_filter_prc_init,
  tiz_krn_class_init,
  tiz_krn_init,
  tiz_fsm_class_init,
  tiz_fsm_init,
  tiz_state_class_init,
  tiz_state_init,
  tiz_loaded_class_init,
  tiz_loaded_init,
  tiz_loadedtoidle_class_init,
  tiz_loadedtoidle_init,
  tiz_idle_class_init,
  tiz_idle_init,
  tiz_idletoloaded_class_init,
  tiz_idletoloaded_init,
  tiz_idletoexecuting_class_init,
  tiz_idletoexecuting_init,
  tiz_pause_class_init,
  tiz_pause_init,
  tiz_pausetoidle_class_init,
  tiz_pausetoidle_init,
  tiz_waitforresources_class_init,
  tiz_waitforresources_init,
  tiz_executing_class_init,
  tiz_executing_init,
  tiz_executingtoidle_class_init,
  tiz_executingtoidle_init,
  tiz_port_class_init,
  tiz_port_init,
  tiz_configport_class_init,
  tiz_configport_init,
  tiz_audioport_class_init,
  tiz_audioport_init,
  tiz_pcmport_class_init,
  tiz_pcmport_init,
  tiz_mp2port_class_init,
  tiz_mp2port_init,
  tiz_mp3port_class_init,
  tiz_mp3port_init,
  tiz_aacport_class_init,
  tiz_aacport_init,
  tiz_vorbisport_class_init,
  tiz_vorbisport_init,
  tiz_opusport_class_init,
  tiz_opusport_init,
  tiz_flacport_class_init,
  tiz_flacport_init,
  tiz_videoport_class_init,
  tiz_videoport_init,
  tiz_imageport_class_init,
  tiz_imageport_init,
  tiz_otherport_class_init,
  tiz_otherport_init,
  tiz_vp8port_class_init,
  tiz_vp8port_init,
  tiz_avcport_class_init,
  tiz_avcport_init,
  tiz_ivrport_class_init,
  tiz_ivrport_init,
  tiz_binaryport_class_init,
  tiz_binaryport_init,
  tiz_demuxerport_class_init,
  tiz_demuxerport_init,
  tiz_muxerport_class_init,
  tiz_muxerport_init,
  tiz_webmport_class_init,
  tiz_webmport_init,
  tiz_oggport_class_init,
  tiz_oggport_init,
  tiz_uricfgport_class_init,
  tiz_uricfgport_init,
  tiz_demuxercfgport_class_init,
  tiz_demuxercfgport_init,
  tiz_mp4port_class_init,
  tiz_mp4port_init,
};

typedef struct tiz_os_type_str tiz_os_type_str_t;
struct tiz_os_type_str
{
  tiz_os_type_t type;
  OMX_STRING str;
};

static tiz_os_type_str_t tiz_os_type_to_str_tbl[] = {
  {ETIZClass, "tizclass"},
  {ETIZObject, "tizobject"},
  {ETIZApi_class, "tizapi_class"},
  {ETIZApi, "tizapi"},
  {ETIZSrv_class, "tizsrv_class"},
  {ETIZSrv, "tizsrv"},
  {ETIZPrc_class, "tizprc_class"},
  {ETIZPrc, "tizprc"},
  {ETIZFilterprc_class, "tizfilterprc_class"},
  {ETIZFilterprc, "tizfilterprc"},
  {ETIZKrn_class, "tizkrn_class"},
  {ETIZKrn, "tizkrn"},
  {ETIZFsm_class, "tizfsm_class"},
  {ETIZFsm, "tizfsm"},
  {ETIZState_class, "tizstate_class"},
  {ETIZState, "tizstate"},
  {ETIZLoaded_class, "tizloaded_class"},
  {ETIZLoaded, "tizloaded"},
  {ETIZLoadedtoidle_class, "tizloadedtoidle_class"},
  {ETIZLoadedtoidle, "tizloadedtoidle"},
  {ETIZIdle_class, "tizidle_class"},
  {ETIZIdle, "tizidle"},
  {ETIZIdletoloaded_class, "tizidletoloaded_class"},
  {ETIZIdletoloaded, "tizidletoloaded"},
  {ETIZIdletoexecuting_class, "tizidletoexecuting_class"},
  {ETIZIdletoexecuting, "tizidletoexecuting"},
  {ETIZPause_class, "tizpause_class"},
  {ETIZPause, "tizpause"},
  {ETIZPausetoidle_class, "tizpausetoidle_class"},
  {ETIZPausetoidle, "tizpausetoidle"},
  {ETIZWaitforresources_class, "tizwaitforresources_class"},
  {ETIZWaitforresources, "tizwaitforresources"},
  {ETIZExecuting_class, "tizexecuting_class"},
  {ETIZExecuting, "tizexecuting"},
  {ETIZExecutingtoidle_class, "tizexecutingtoidle_class"},
  {ETIZExecutingtoidle, "tizexecutingtoidle"},
  {ETIZPort_class, "tizport_class"},
  {ETIZPort, "tizport"},
  {ETIZConfigport_class, "tizconfigport_class"},
  {ETIZConfigport, "tizconfigport"},
  {ETIZAudioport_class, "tizaudioport_class"},
  {ETIZAudioport, "tizaudioport"},
  {ETIZPcmport_class, "tizpcmport_class"},
  {ETIZPcmport, "tizpcmport"},
  {ETIZMp2port_class, "tizmp2port_class"},
  {ETIZMp2port, "tizmp2port"},
  {ETIZMp3port_class, "tizmp3port_class"},
  {ETIZMp3port, "tizmp3port"},
  {ETIZAacport_class, "tizaacport_class"},
  {ETIZAacport, "tizaacport"},
  {ETIZVorbisport_class, "tizvorbisport_class"},
  {ETIZVorbisport, "tizvorbisport"},
  {ETIZOpusport_class, "tizopusport_class"},
  {ETIZOpusport, "tizopusport"},
  {ETIZFlacport_class, "tizflacport_class"},
  {ETIZFlacport, "tizflacport"},
  {ETIZVideoport_class, "tizvideoport_class"},
  {ETIZVideoport, "tizvideoport"},
  {ETIZImageport_class, "tizimageport_class"},
  {ETIZImageport, "tizimageport"},
  {ETIZOtherport_class, "tizotherport_class"},
  {ETIZOtherport, "tizotherport"},
  {ETIZVp8port_class, "tizvp8port_class"},
  {ETIZVp8port, "tizvp8port"},
  {ETIZAvcport_class, "tizavcport_class"},
  {ETIZAvcport, "tizavcport"},
  {ETIZIvrport_class, "tizivrport_class"},
  {ETIZIvrport, "tizivrport"},
  {ETIZBinaryport_class, "tizbinaryport_class"},
  {ETIZBinaryport, "tizbinaryport"},
  {ETIZDemuxerport_class, "tizdemuxerport_class"},
  {ETIZDemuxerport, "tizdemuxerport"},
  {ETIZMuxerport_class, "tizmuxerport_class"},
  {ETIZMuxerport, "tizmuxerport"},
  {ETIZWebmport_class, "tizwebmport_class"},
  {ETIZWebmport, "tizwebmport"},
  {ETIZOggport_class, "tizoggport_class"},
  {ETIZOggport, "tizoggport"},
  {ETIZUricfgport_class, "tizuricfgport_class"},
  {ETIZUricfgport, "tizuricfgport"},
  {ETIZDemuxercfgport_class, "tizdemuxercfgport_class"},
  {ETIZDemuxercfgport, "tizdemuxercfgport"},
  {ETIZMp4port_class, "tizmp4port_class"},
  {ETIZMp4port, "tizmp4port"},
};

static /*@null@ */ void *
os_calloc (/*@null@ */ tiz_soa_t * p_soa, size_t a_size)
{
  return p_soa ? tiz_soa_calloc (p_soa, a_size) : tiz_mem_calloc (1, a_size);
}

static inline void
os_free (tiz_soa_t * p_soa, void * ap_addr)
{
  p_soa ? tiz_soa_free (p_soa, ap_addr) : tiz_mem_free (ap_addr);
}

static char *
os_strndup (tiz_soa_t * p_soa, const char * s, size_t n)
{
  char * result;
  size_t len = strlen (s);

  assert (p_soa);

  if (n < len)
    {
      len = n;
    }

  if (NULL == (result = (char *) os_calloc (p_soa, len + 1)))
    {
      return NULL;
    }

  result[len] = '\0';
  return (char *) memcpy (result, s, len);
}

static OMX_S32
os_map_compare_func (OMX_PTR ap_key1, OMX_PTR ap_key2)
{
  return strncmp ((const char *) ap_key1, (const char *) ap_key2,
                  OMX_MAX_STRINGNAME_SIZE);
}

static void
os_map_free_func (OMX_PTR ap_key, OMX_PTR ap_value)
{
  /* Don't bother deleting the keys here as they are allocated from the chunk
     of memory provided by the small object allocator, which gets released when
     the component is deinited. */
  tiz_mem_free (ap_value);
}

#ifdef _DEBUG
static OMX_S32
print_function (OMX_PTR ap_key, OMX_PTR ap_value, OMX_PTR ap_arg)
{
  const char * name = ap_key;
  tiz_os_t * p_os = ap_arg;
  TIZ_TRACE (p_os->p_hdl, "type [%s]->[%p]", name, ap_value);
  return 0;
}
#endif

static void
print_types (const tiz_os_t * ap_os)
{
#ifdef _DEBUG
  assert (ap_os);
  assert (ap_os->p_map);
  tiz_map_for_each (ap_os->p_map, print_function, (tiz_os_t *) ap_os);
#endif
}

static OMX_ERRORTYPE
os_register_type (tiz_os_t * ap_os, const tiz_os_type_init_f a_type_init_f,
                  const char * a_type_name, const OMX_S32 a_type_id)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  void * p_obj = NULL;

  assert (ap_os);
  assert (ap_os->p_map);
  assert (a_type_init_f);
  assert (a_type_name);
  assert (strnlen (a_type_name, OMX_MAX_STRINGNAME_SIZE)
          < OMX_MAX_STRINGNAME_SIZE);
  assert (a_type_id >= 0);

  /* Call the type init function */
  p_obj = a_type_init_f (ap_os, ap_os->p_hdl);

  if (p_obj)
    {
      /* Register the class or object type */
      TIZ_TRACE (ap_os->p_hdl,
                 "Registering type #[%d] : [%s] -> [%p] "
                 "nameOf [%s]",
                 a_type_id, a_type_name, p_obj, nameOf (p_obj));
      rc = tiz_map_insert (ap_os->p_map, os_strndup (ap_os->p_soa, a_type_name,
                                                     OMX_MAX_STRINGNAME_SIZE),
                           p_obj, (OMX_U32 *) (&a_type_id));
    }

  /*   print_types (ap_os); */

  return rc;
}

static OMX_ERRORTYPE
register_base_types (tiz_os_t * ap_os)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const OMX_S32 count
    = sizeof (tiz_os_type_to_str_tbl) / sizeof (tiz_os_type_str_t);
  OMX_S32 type_id = 0;
  OMX_S32 last_element = 0;

  assert (ap_os);
  assert (count >= TIZ_OS_BASE_TYPE_END);
  last_element = TIZ_OS_BASE_TYPE_END;

  for (type_id = 0; type_id <= last_element && OMX_ErrorNone == rc; ++type_id)
    {
      TIZ_TRACE (ap_os->p_hdl, "Registering type [%s]...",
                 tiz_os_type_to_str_tbl[type_id].str);
      rc = os_register_type (ap_os, tiz_os_type_to_fnt_tbl[type_id],
                             tiz_os_type_to_str_tbl[type_id].str, type_id);
    }
  return rc;
}

OMX_ERRORTYPE
tiz_os_init (tiz_os_t ** app_os, const OMX_HANDLETYPE ap_hdl,
             tiz_soa_t * ap_soa)
{
  tiz_os_t * p_os = NULL;

  assert (app_os);
  assert (ap_hdl);

  TIZ_TRACE (ap_hdl, "Init");

  if (NULL == (p_os = (tiz_os_t *) os_calloc (ap_soa, sizeof (tiz_os_t))))
    {
      return OMX_ErrorInsufficientResources;
    }

  assert (p_os);

  if (OMX_ErrorNone != tiz_map_init (&(p_os->p_map), os_map_compare_func,
                                     os_map_free_func, NULL))
    {
      os_free (ap_soa, p_os);
      p_os = NULL;
      return OMX_ErrorInsufficientResources;
    }

  p_os->p_hdl = ap_hdl;
  p_os->p_soa = ap_soa;

  *app_os = p_os;

  return OMX_ErrorNone;
}

void
tiz_os_destroy (tiz_os_t * ap_os)
{
  if (ap_os)
    {
      while (!tiz_map_empty (ap_os->p_map))
        {
          tiz_map_erase_at (ap_os->p_map, 0);
        };
      tiz_map_destroy (ap_os->p_map);
      os_free (ap_os->p_soa, ap_os);
    }
}

OMX_ERRORTYPE
tiz_os_register_type (tiz_os_t * ap_os, const tiz_os_type_init_f a_type_init_f,
                      const OMX_STRING a_type_name)
{
  assert (ap_os);
  return os_register_type (ap_os, a_type_init_f, a_type_name,
                           tiz_map_size (ap_os->p_map));
}

OMX_ERRORTYPE
tiz_os_register_base_types (tiz_os_t * ap_os)
{
  assert (ap_os);
  return register_base_types (ap_os);
}

static OMX_ERRORTYPE
register_additional_type (tiz_os_t * ap_os, const char * a_type_name)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const OMX_S32 count
    = sizeof (tiz_os_type_to_str_tbl) / sizeof (tiz_os_type_str_t);
  OMX_S32 type_id = TIZ_OS_BASE_TYPE_END + 1;

  assert (ap_os);

  for (; type_id < count && OMX_ErrorNone == rc; ++type_id)
    {
      if (0 == strncmp (a_type_name, tiz_os_type_to_str_tbl[type_id].str,
                        OMX_MAX_STRINGNAME_SIZE))
        {
          TIZ_TRACE (ap_os->p_hdl, "Registering additional type [%s]...",
                     a_type_name);
          rc = os_register_type (ap_os, tiz_os_type_to_fnt_tbl[type_id],
                                 a_type_name, type_id);
          break;
        }
    }
  return rc;
}

void *
tiz_os_get_type (const tiz_os_t * ap_os, const char * a_type_name)
{
  void * res = NULL;
  assert (ap_os);
  assert (ap_os->p_map);
  assert (a_type_name);
  res = tiz_map_find (ap_os->p_map, (OMX_PTR) a_type_name);
  TIZ_TRACE (ap_os->p_hdl, "Get type [%s]->[%p] - total types [%d]",
             a_type_name, res, tiz_map_size (ap_os->p_map));
  if (!res)
    {
      if (OMX_ErrorNone
          == register_additional_type ((tiz_os_t *) ap_os, a_type_name))
        {
          print_types (ap_os);
          res = tiz_map_find (ap_os->p_map, (OMX_PTR) a_type_name);
        }
    }
  assert (res);
  return res;
}

void *
tiz_os_calloc (const tiz_os_t * ap_os, size_t a_size)
{
  assert (ap_os);
  assert (ap_os->p_soa);
  return os_calloc (ap_os->p_soa, a_size);
}

void
tiz_os_free (const tiz_os_t * ap_os, void * ap_addr)
{
  assert (ap_os);
  assert (ap_os->p_soa);
  os_free (ap_os->p_soa, ap_addr);
}
