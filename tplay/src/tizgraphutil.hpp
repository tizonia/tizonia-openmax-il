/* -*-Mode: c++; -*- */
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
 * @file   tizgraphutil.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph utilities
 *
 *
 */

#ifndef TIZGRAPHUTIL_HPP
#define TIZGRAPHUTIL_HPP

#include <string>

#include <boost/function.hpp>

#include <OMX_Core.h>
#include <OMX_Types.h>

#include <OMX_TizoniaExt.h>

#include "tizgraphtypes.hpp"

namespace tiz
{
  namespace graph
  {
    class util
    {

    public:
      static OMX_ERRORTYPE verify_comp_list (
          const omx_comp_name_lst_t &comp_list);

      static OMX_ERRORTYPE verify_role (const std::string &comp,
                                        const std::string &role);

      static OMX_ERRORTYPE verify_role_list (
          const omx_comp_name_lst_t &comp_list,
          const omx_comp_role_lst_t &role_list);

      static OMX_ERRORTYPE instantiate_component (
          const std::string &comp_name, const int graph_position,
          OMX_PTR ap_app_data, OMX_CALLBACKTYPE *ap_callbacks,
          omx_comp_handle_lst_t &hdl_list, omx_hdl2name_map_t &h2n_map);

      static OMX_ERRORTYPE instantiate_comp_list (
          const omx_comp_name_lst_t &comp_list, omx_comp_handle_lst_t &hdl_list,
          omx_hdl2name_map_t &h2n_map, OMX_PTR ap_app_data,
          OMX_CALLBACKTYPE *ap_callbacks);

      static void destroy_list (omx_comp_handle_lst_t &hdl_list);

      static OMX_ERRORTYPE setup_suppliers (
          const omx_comp_handle_lst_t &hdl_list);

      static OMX_ERRORTYPE setup_tunnels (
          const omx_comp_handle_lst_t &hdl_list);

      static OMX_ERRORTYPE tear_down_tunnels (
          const omx_comp_handle_lst_t &hdl_list);

      static OMX_ERRORTYPE transition_one (
          const omx_comp_handle_lst_t &hdl_list, const int handle_id,
          const OMX_STATETYPE to);

      static OMX_ERRORTYPE transition_all (
          const omx_comp_handle_lst_t &hdl_list, const OMX_STATETYPE to,
          const OMX_STATETYPE from);

      static bool verify_transition_all (const omx_comp_handle_lst_t &hdl_list,
                                         const OMX_STATETYPE to);

      static bool verify_transition_one (const OMX_HANDLETYPE handle,
                                         const OMX_STATETYPE to);

      static OMX_ERRORTYPE apply_volume (const OMX_HANDLETYPE handle,
                                         const OMX_U32 pid, const int step);

      static OMX_ERRORTYPE apply_mute (const OMX_HANDLETYPE handle,
                                       const OMX_U32 pid);

      static OMX_ERRORTYPE disable_port (const OMX_HANDLETYPE handle,
                                         const OMX_U32 port_id);

      static OMX_ERRORTYPE modify_tunnel (const omx_comp_handle_lst_t &hdl_list,
                                          const int tunnel_id,
                                          const OMX_COMMANDTYPE cmd);

      static OMX_ERRORTYPE disable_tunnel (
          const omx_comp_handle_lst_t &hdl_list, const int tunnel_id);

      static OMX_ERRORTYPE enable_tunnel (const omx_comp_handle_lst_t &hdl_list,
                                          const int tunnel_id);

      template<typename ParamT>
      static OMX_ERRORTYPE get_channels_and_rate_from_audio_port (const OMX_HANDLETYPE handle,
                                                                  const OMX_U32 port_id,
                                                                  const OMX_INDEXTYPE param_index,
                                                                  OMX_U32 &channels,
                                                                  OMX_U32 &sampling_rate);

      template<typename ParamT>
      static OMX_ERRORTYPE set_channels_and_rate_on_audio_port (const OMX_HANDLETYPE handle,
                                                                const OMX_U32 port_id,
                                                                const OMX_INDEXTYPE param_index,
                                                                const OMX_U32 channels,
                                                                const OMX_U32 sampling_rate);

      static OMX_ERRORTYPE set_content_uri (const OMX_HANDLETYPE handle,
                                            const std::string &uri);

      static OMX_ERRORTYPE set_pcm_mode (
          const OMX_HANDLETYPE handle, const OMX_U32 port_id,
          boost::function< void(OMX_AUDIO_PARAM_PCMMODETYPE &pcmmode) > getter);

      static OMX_ERRORTYPE set_mp3_type (
          const OMX_HANDLETYPE handle, const OMX_U32 port_id,
          boost::function< void(OMX_AUDIO_PARAM_MP3TYPE &mp3type) > getter,
          bool &need_port_settings_changed_evt);

      static OMX_ERRORTYPE set_aac_type (
          const OMX_HANDLETYPE handle, const OMX_U32 port_id,
          boost::function< void(OMX_AUDIO_PARAM_AACPROFILETYPE &aactype) > getter,
          bool &need_port_settings_changed_evt);

      static OMX_ERRORTYPE set_flac_type (
          const OMX_HANDLETYPE handle, const OMX_U32 port_id,
          boost::function< void(OMX_TIZONIA_AUDIO_PARAM_FLACTYPE &flactype) >
              getter, bool &need_port_settings_changed_evt);

      static OMX_ERRORTYPE enable_port_format_auto_detection (
          const OMX_HANDLETYPE handle, const OMX_U32 port_id,
          const OMX_PORTDOMAINTYPE domain);

      static void dump_graph_info (const char *ap_coding_type_str,
                                   const char *ap_graph_type_str,
                                   const std::string &uri);

      static bool is_fatal_error (const OMX_ERRORTYPE error);

    };
  }  // namespace graph
}  // namespace tiz

#include "tizgraphutil.inl"

#endif  // TIZGRAPHUTIL_HPP
