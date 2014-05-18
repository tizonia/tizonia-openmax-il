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
 * @file   tizgraphutil.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph utilities
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <boost/foreach.hpp>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <tizplatform.h>

#include "tizomxutil.hpp"
#include "tizgraphutil.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.utils"
#endif

namespace graph = tiz::graph;

namespace  // Unnamed namespace
{

  struct transition_to
  {
    transition_to (const OMX_STATETYPE to_state, const OMX_U32 useconds = 0)
      : to_state_ (to_state), delay_ (useconds), error_ (OMX_ErrorNone)
    {
    }
    void operator()(const OMX_HANDLETYPE &handle)
    {
      if (OMX_ErrorNone == error_)
        error_ = OMX_SendCommand (handle, OMX_CommandStateSet, to_state_, NULL);
      tiz_sleep (delay_);
    }
    const OMX_STATETYPE to_state_;
    OMX_U32 delay_;
    OMX_ERRORTYPE error_;
  };

  struct transition_verify
  {
    transition_verify (const OMX_STATETYPE to_state)
      : to_state_ (to_state),
        error_ (OMX_ErrorNone),
        transition_verified_ (true)
    {
    }
    void operator()(const OMX_HANDLETYPE &handle)
    {
      if (OMX_ErrorNone == error_ && transition_verified_)
      {
        OMX_STATETYPE state;
        error_ = OMX_GetState (handle, &state);
        if (to_state_ != state)
        {
          transition_verified_ = false;
        }
      }
    }
    const OMX_STATETYPE to_state_;
    OMX_ERRORTYPE error_;
    bool transition_verified_;
  };
}

OMX_ERRORTYPE
graph::util::verify_comp_list (const omx_comp_name_lst_t &comp_list)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  std::vector< std::string > components;

  if (OMX_ErrorNoMore == (error = tiz::omxutil::list_comps (components)))
  {
    BOOST_FOREACH (std::string comp, comp_list)
    {
      bool found = false;
      BOOST_FOREACH (std::string component, components)
      {
        if (comp.compare (component) == 0)
        {
          found = true;
          break;
        }
      }

      if (!found)
      {
        error = OMX_ErrorComponentNotFound;
        break;
      }
    }
  }

  if (error == OMX_ErrorNoMore)
  {
    error = OMX_ErrorNone;
  }

  return error;
}

OMX_ERRORTYPE
graph::util::verify_role (const std::string &comp, const std::string &comp_role)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  std::vector< std::string > roles;

  if (OMX_ErrorNoMore == (error = tiz::omxutil::roles_of_comp (
                              (OMX_STRING)comp.c_str (), roles)))
  {
    bool found = false;
    BOOST_FOREACH (std::string role, roles)
    {
      if (comp_role.compare (role) == 0)
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      error = OMX_ErrorComponentNotFound;
    }
  }

  if (error == OMX_ErrorNoMore)
  {
    error = OMX_ErrorNone;
  }

  return error;
}

OMX_ERRORTYPE
graph::util::verify_role_list (const omx_comp_name_lst_t &comp_list,
                               const omx_comp_role_lst_t &role_list)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  std::vector< std::string > roles;
  int role_lst_size = role_list.size ();

  assert (comp_list.size () == role_lst_size);

  for (int i = 0; i < role_lst_size; i++)
  {
    if (OMX_ErrorNone != (error = verify_role (comp_list[i], role_list[i])))
    {
      break;
    }
  }

  return error;
}

OMX_ERRORTYPE
graph::util::instantiate_component (const std::string &comp_name,
                                    const int graph_position,
                                    OMX_PTR ap_app_data,
                                    OMX_CALLBACKTYPE *ap_callbacks,
                                    omx_comp_handle_lst_t &hdl_list,
                                    omx_hdl2name_map_t &h2n_map)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = NULL;
  assert (graph_position < hdl_list.size ());

  if (OMX_ErrorNone
      == (error = OMX_GetHandle (&p_hdl, (OMX_STRING)comp_name.c_str (),
                                 ap_app_data, ap_callbacks)))
  {
    hdl_list[graph_position] = p_hdl;
    h2n_map[p_hdl] = comp_name;
  }

  return error;
}

OMX_ERRORTYPE
graph::util::instantiate_comp_list (const omx_comp_name_lst_t &comp_list,
                                    omx_comp_handle_lst_t &hdl_list,
                                    omx_hdl2name_map_t &h2n_map,
                                    OMX_PTR ap_app_data,
                                    OMX_CALLBACKTYPE *ap_callbacks)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int position = 0;
  bool graph_instantiated = true;

  assert (comp_list.size () == hdl_list.size ());
  assert (h2n_map.empty ());

  BOOST_FOREACH (std::string comp, comp_list)
  {
    if (OMX_ErrorNone
        != (error = instantiate_component (comp, position++, ap_app_data,
                                           ap_callbacks, hdl_list, h2n_map)))
    {
      graph_instantiated = false;
      break;
    }
  }

  if (!graph_instantiated)
  {
    destroy_list (hdl_list);
    hdl_list.clear ();
  }

  return error;
}

void graph::util::destroy_list (omx_comp_handle_lst_t &hdl_list)
{
  int handle_lst_size = hdl_list.size ();

  for (int i = 0; i < handle_lst_size; i++)
  {
    if (hdl_list[i] != NULL)
    {
      OMX_FreeHandle (hdl_list[i]);
      hdl_list[i] = NULL;
    }
  }
}

// TODO: Replace magic numbers in this function
OMX_ERRORTYPE
graph::util::setup_tunnels (const omx_comp_handle_lst_t &hdl_list)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  const int handle_lst_size = hdl_list.size ();

  for (int i = 0; i < handle_lst_size - 1 && OMX_ErrorNone == error; i++)
  {
    error = OMX_SetupTunnel (hdl_list[i], i == 0 ? 0 : 1, hdl_list[i + 1], 0);
  }

  return error;
}

// TODO: Replace magic numbers in this function
OMX_ERRORTYPE
graph::util::tear_down_tunnels (const omx_comp_handle_lst_t &hdl_list)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  const int handle_lst_size = hdl_list.size ();

  for (int i = 0; i < handle_lst_size - 1 && OMX_ErrorNone == error; i++)
  {
    error
        = OMX_TeardownTunnel (hdl_list[i], i == 0 ? 0 : 1, hdl_list[i + 1], 0);
  }

  return error;
}

// TODO: Replace magic numbers in this function
OMX_ERRORTYPE
graph::util::setup_suppliers (const omx_comp_handle_lst_t &hdl_list)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_PARAM_BUFFERSUPPLIERTYPE supplier;
  const int handle_lst_size = hdl_list.size ();

  TIZ_INIT_OMX_PORT_STRUCT (supplier, 0);
  supplier.eBufferSupplier = OMX_BufferSupplyInput;

  for (int i = 0; i < handle_lst_size - 1 && OMX_ErrorNone == error; i++)
  {
    supplier.nPortIndex = i == 0 ? 0 : 1;
    error = OMX_SetParameter (hdl_list[i], OMX_IndexParamCompBufferSupplier,
                              &supplier);
    if (OMX_ErrorNone == error)
    {
      supplier.nPortIndex = 0;
      error = OMX_SetParameter (hdl_list[i + 1],
                                OMX_IndexParamCompBufferSupplier, &supplier);
    }
  }
  return error;
}

OMX_ERRORTYPE
graph::util::transition_one (const omx_comp_handle_lst_t &hdl_list,
                             const int handle_id, const OMX_STATETYPE to)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;

  assert (handle_id < hdl_list.size ());

  struct transition_to transition_component (to);

  transition_component (hdl_list[handle_id]);
  error = transition_component.error_;

  return error;
}

OMX_ERRORTYPE
graph::util::transition_all (const omx_comp_handle_lst_t &hdl_list,
                             const OMX_STATETYPE to, const OMX_STATETYPE from)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;

  if ((to == OMX_StateIdle && from == OMX_StateLoaded)
      || (to == OMX_StateExecuting && from == OMX_StateIdle))
  {
    // Suppliers first, hence back to front order
    error = (std::for_each (hdl_list.rbegin (), hdl_list.rend (),
                            transition_to (to))).error_;

    // NOTE: Leave this commented code here - for testing purposes
    // Non-suppliers first, hence front to back order
    //       error = (std::for_each(hdl_list.begin(), hdl_list.end(),
    //                              transition_to(to))).error_;
  }
  else
  {
    // Non-suppliers first, hence front to back order
    error = (std::for_each (hdl_list.begin (), hdl_list.end (),
                            transition_to (to))).error_;

    // NOTE: Leave this commented code here - for testing purposes
    // Suppliers first, hence back to front order
    //       error = (std::for_each(hdl_list.rbegin(), hdl_list.rend(),
    //                              transition_to(to))).error_;
  }

  TIZ_LOG (TIZ_PRIORITY_DEBUG, "to [%s] from [%s] error [%s]",
           tiz_state_to_str (to), tiz_state_to_str (from),
           tiz_err_to_str (error));

  return error;
}

bool graph::util::verify_transition_all (const omx_comp_handle_lst_t &hdl_list,
                                         const OMX_STATETYPE to)
{
  return (std::for_each (hdl_list.begin (), hdl_list.end (),
                         transition_verify (to))).transition_verified_;
}

bool graph::util::verify_transition_one (const OMX_HANDLETYPE handle,
                                         const OMX_STATETYPE to)
{
  OMX_STATETYPE state = OMX_StateMax;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  bool transition_verified = false;

  error = OMX_GetState (handle, &state);
  if (OMX_ErrorNone == error && to == state)
  {
    transition_verified = true;
  }

  return transition_verified;
}

OMX_ERRORTYPE
graph::util::apply_volume (const OMX_HANDLETYPE handle, const OMX_U32 pid,
                           const int step)
{
#define VOL_STEP 5
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  bool new_volume = false;
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  TIZ_INIT_OMX_PORT_STRUCT (volume, pid);
  tiz_check_omx_err (
      OMX_GetConfig (handle, OMX_IndexConfigAudioVolume, &volume));
  if (volume.sVolume.nValue <= (volume.sVolume.nMax - VOL_STEP) && step > 0)
  {
    volume.sVolume.nValue += VOL_STEP;
    new_volume = true;
  }
  else if (volume.sVolume.nValue >= (volume.sVolume.nMin + VOL_STEP) && step
                                                                        < 0)
  {
    volume.sVolume.nValue -= VOL_STEP;
    new_volume = true;
  }
  if (new_volume)
  {
    tiz_check_omx_err (
        OMX_SetConfig (handle, OMX_IndexConfigAudioVolume, &volume));
  }
  return rc;
}

OMX_ERRORTYPE
graph::util::apply_mute (const OMX_HANDLETYPE handle, const OMX_U32 pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_AUDIO_CONFIG_MUTETYPE mute;
  TIZ_INIT_OMX_PORT_STRUCT (mute, pid);
  tiz_check_omx_err (OMX_GetConfig (handle, OMX_IndexConfigAudioMute, &mute));
  mute.bMute = (mute.bMute == OMX_FALSE
                    ? OMX_TRUE
                    : (mute.bMute == OMX_TRUE ? OMX_FALSE : OMX_TRUE));
  tiz_check_omx_err (OMX_SetConfig (handle, OMX_IndexConfigAudioMute, &mute));
  return rc;
}

OMX_ERRORTYPE
graph::util::disable_port (const OMX_HANDLETYPE handle, const OMX_U32 port_id)
{
  return OMX_SendCommand (handle, OMX_CommandPortDisable, port_id, NULL);
}

// TODO: Replace magic numbers in this function
OMX_ERRORTYPE
graph::util::modify_tunnel (const omx_comp_handle_lst_t &hdl_list,
                            const int tunnel_id, const OMX_COMMANDTYPE cmd)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  const int handle_lst_size = hdl_list.size ();
  assert (tunnel_id < handle_lst_size - 1);

  omx_comp_handle_lst_t tunnel_handles;
  tunnel_handles.push_back (hdl_list[tunnel_id]);
  tunnel_handles.push_back (hdl_list[tunnel_id + 1]);

  std::vector< int > port_ids;
  port_ids.push_back (tunnel_id == 0 ? 0 : 1);
  port_ids.push_back (tunnel_id + 1 == handle_lst_size - 1 ? 0 : 1);

  for (int i = 0; i < 2 && error == OMX_ErrorNone; i++)
  {
    error = OMX_SendCommand (tunnel_handles[i], cmd, port_ids[i], NULL);
  }

  return error;
}

OMX_ERRORTYPE
graph::util::disable_tunnel (const omx_comp_handle_lst_t &hdl_list,
                             const int tunnel_id)
{
  return modify_tunnel (hdl_list, tunnel_id, OMX_CommandPortDisable);
}

OMX_ERRORTYPE
graph::util::enable_tunnel (const omx_comp_handle_lst_t &hdl_list,
                            const int tunnel_id)
{
  return modify_tunnel (hdl_list, tunnel_id, OMX_CommandPortEnable);
}

OMX_ERRORTYPE
graph::util::set_content_uri (const OMX_HANDLETYPE handle,
                              const std::string &uri)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  // Set the URI
  OMX_PARAM_CONTENTURITYPE *p_uritype = NULL;
  const long pathname_max = tiz_pathname_max (uri.c_str ());
  const int uri_len = uri.length ();

  if (NULL == (p_uritype = (OMX_PARAM_CONTENTURITYPE *)tiz_mem_calloc (
                   1, sizeof(OMX_PARAM_CONTENTURITYPE) + uri_len + 1))
      || uri_len > pathname_max)
  {
    rc = OMX_ErrorInsufficientResources;
  }
  else
  {
    p_uritype->nSize = sizeof(OMX_PARAM_CONTENTURITYPE) + uri_len + 1;
    p_uritype->nVersion.nVersion = OMX_VERSION;

    const size_t uri_offset = offsetof (OMX_PARAM_CONTENTURITYPE, contentURI);
    strncpy ((char *)p_uritype + uri_offset, uri.c_str (), uri_len);
    p_uritype->contentURI[uri_len] = '\0';

    rc = OMX_SetParameter (handle, OMX_IndexParamContentURI, p_uritype);

    tiz_mem_free (p_uritype);
    p_uritype = NULL;
  }
  return rc;
}

OMX_ERRORTYPE
graph::util::set_pcm_mode (
    const OMX_HANDLETYPE handle, const OMX_U32 port_id,
    boost::function< void(OMX_AUDIO_PARAM_PCMMODETYPE &pcmmode) > getter)
{
  // Set the pcm settings
  OMX_AUDIO_PARAM_PCMMODETYPE pcmtype;
  TIZ_INIT_OMX_PORT_STRUCT (pcmtype, port_id);
  getter (pcmtype);
  tiz_check_omx_err (
      OMX_SetParameter (handle, OMX_IndexParamAudioPcm, &pcmtype));
}

OMX_ERRORTYPE
graph::util::set_mp3_type (
    const OMX_HANDLETYPE handle, const OMX_U32 port_id,
    boost::function< void(OMX_AUDIO_PARAM_MP3TYPE &mp3type) > getter,
    bool &need_port_settings_changed_evt)
{
  // Retrieve the current mp3 settings
  OMX_AUDIO_PARAM_MP3TYPE mp3type_orig;
  TIZ_INIT_OMX_PORT_STRUCT (mp3type_orig, port_id);

  tiz_check_omx_err (
      OMX_GetParameter (handle, OMX_IndexParamAudioMp3, &mp3type_orig));

  // Set the new mp3 settings
  OMX_AUDIO_PARAM_MP3TYPE mp3type;
  TIZ_INIT_OMX_PORT_STRUCT (mp3type, port_id);

  getter (mp3type);
  tiz_check_omx_err (
      OMX_SetParameter (handle, OMX_IndexParamAudioMp3, &mp3type));

  // Record whether we need to wait for a port settings change event or not
  // (i.e. the decoder output port implements the "slaving" behaviour)
  need_port_settings_changed_evt
      = ((mp3type_orig.nSampleRate != mp3type.nSampleRate)
         || (mp3type_orig.nChannels != mp3type.nChannels));

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::util::set_aac_type (
    const OMX_HANDLETYPE handle, const OMX_U32 port_id,
    boost::function< void(OMX_AUDIO_PARAM_AACPROFILETYPE &aactype) > getter,
    bool &need_port_settings_changed_evt)
{
  // Retrieve the current aac settings
  OMX_AUDIO_PARAM_AACPROFILETYPE aactype_orig;
  TIZ_INIT_OMX_PORT_STRUCT (aactype_orig, port_id);

  tiz_check_omx_err (
      OMX_GetParameter (handle, OMX_IndexParamAudioAac, &aactype_orig));

  // Set the new aac settings
  OMX_AUDIO_PARAM_AACPROFILETYPE aactype;
  TIZ_INIT_OMX_PORT_STRUCT (aactype, port_id);

  getter (aactype);
  tiz_check_omx_err (
      OMX_SetParameter (handle, OMX_IndexParamAudioAac, &aactype));

  // Record whether we need to wait for a port settings change event or not
  // (i.e. the decoder output port implements the "slaving" behaviour)
  need_port_settings_changed_evt
      = ((aactype_orig.nSampleRate != aactype.nSampleRate)
         || (aactype_orig.nChannels != aactype.nChannels));

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::util::set_flac_type (
    const OMX_HANDLETYPE handle, const OMX_U32 port_id,
    boost::function< void(OMX_TIZONIA_AUDIO_PARAM_FLACTYPE &flactype) > getter,
    bool &need_port_settings_changed_evt)
{
  // Retrieve the current flac settings
  OMX_TIZONIA_AUDIO_PARAM_FLACTYPE flactype_orig;
  TIZ_INIT_OMX_PORT_STRUCT (flactype_orig, port_id);

  tiz_check_omx_err (OMX_GetParameter (
      handle, static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioFlac),
      &flactype_orig));

  OMX_TIZONIA_AUDIO_PARAM_FLACTYPE flactype;
  TIZ_INIT_OMX_PORT_STRUCT (flactype, port_id);

  // Set the new flac settings
  getter (flactype);
  flactype.nPortIndex = 0;
  tiz_check_omx_err (OMX_SetParameter (
      handle, static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioFlac),
      &flactype));

  // Record whether we need to wait for a port settings change event or not
  // (i.e the decoder output port implements the "slaving" behaviour)
  need_port_settings_changed_evt
      = ((flactype_orig.nSampleRate != flactype.nSampleRate)
         || (flactype_orig.nChannels != flactype.nChannels));

  return OMX_ErrorNone;
}

void graph::util::dump_graph_info (const char *ap_coding_type_str,
                                   const char *ap_graph_type_str,
                                   const std::string &uri)
{
#define KNRM "\x1B[0m"
#define KGRN "\x1B[32m"
  fprintf (stdout, "%s[%s] [%s] : '%s'.%s\n", KGRN, ap_coding_type_str,
           ap_graph_type_str, uri.c_str (), KNRM);
}

bool graph::util::is_fatal_error (const OMX_ERRORTYPE error)
{
  bool rc = false;
  switch (error)
  {
    // Some pretty major OMX IL errors
    case OMX_ErrorInsufficientResources:
    case OMX_ErrorUndefined:
    case OMX_ErrorInvalidComponentName:
    case OMX_ErrorComponentNotFound:
    case OMX_ErrorNotImplemented:
    case OMX_ErrorPortsNotCompatible:
    case OMX_ErrorVersionMismatch:
      rc = true;
      break;
    default:
      break;
  };
  return rc;
}
