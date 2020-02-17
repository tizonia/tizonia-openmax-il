/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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

#ifndef TIZGRAPHUTIL_INL
#define TIZGRAPHUTIL_INL

namespace tiz
{
  namespace graph
  {
    template < typename ParamT, OMX_INDEXTYPE ParamIndex >
    OMX_ERRORTYPE util::normalize_tunnel_settings (
        const omx_comp_handle_lst_t &hdl_list, const int tunnel_id,
        const OMX_U32 out_port_id, const OMX_U32 in_port_id)
    {
      const int handle_lst_size = hdl_list.size ();
      assert (tunnel_id < handle_lst_size - 1);

      ParamT omx_struct;
      TIZ_INIT_OMX_PORT_STRUCT (omx_struct, out_port_id);
      // Retrieve the settings from the output port...
      tiz_check_omx (
          OMX_GetParameter (hdl_list[tunnel_id], ParamIndex, &omx_struct));
      omx_struct.nPortIndex = in_port_id;
      // ... and apply them on the input port...
      return OMX_SetParameter (hdl_list[tunnel_id + 1], ParamIndex,
                               &omx_struct);
    }

    template < typename ParamT >
    OMX_ERRORTYPE util::get_channels_and_rate_from_audio_port (
        const OMX_HANDLETYPE handle, const OMX_U32 port_id,
        const OMX_INDEXTYPE param_index, OMX_U32 &channels,
        OMX_U32 &sampling_rate)

    {
      // Retrieve the current settings from the port
      ParamT param_type;
      TIZ_INIT_OMX_PORT_STRUCT (param_type, port_id);
      tiz_check_omx (OMX_GetParameter (handle, param_index, &param_type));
      channels = param_type.nChannels;
      sampling_rate = param_type.nSampleRate;
      return OMX_ErrorNone;
    }

    /** Same as previous function but to be used in structures that have a
     * nSamplingRate member instead of nSampleRate one. */
    template < typename ParamT >
    OMX_ERRORTYPE util::get_channels_and_rate_from_audio_port_v2 (
        const OMX_HANDLETYPE handle, const OMX_U32 port_id,
        const OMX_INDEXTYPE param_index, OMX_U32 &channels,
        OMX_U32 &sampling_rate)

    {
      // Retrieve the current settings from the port
      ParamT param_type;
      TIZ_INIT_OMX_PORT_STRUCT (param_type, port_id);
      tiz_check_omx (OMX_GetParameter (handle, param_index, &param_type));
      channels = param_type.nChannels;
      sampling_rate = param_type.nSamplingRate;
      return OMX_ErrorNone;
    }

    template < typename ParamT >
    OMX_ERRORTYPE util::set_channels_and_rate_on_audio_port (
        const OMX_HANDLETYPE handle, const OMX_U32 port_id,
        const OMX_INDEXTYPE param_index, const OMX_U32 channels,
        const OMX_U32 sampling_rate)

    {
      // Retrieve the current settings from the port
      ParamT param_type;
      TIZ_INIT_OMX_PORT_STRUCT (param_type, port_id);
      tiz_check_omx (OMX_GetParameter (handle, param_index, &param_type));
      param_type.nChannels = channels;
      param_type.nSampleRate = sampling_rate;
      tiz_check_omx (OMX_SetParameter (handle, param_index, &param_type));
      return OMX_ErrorNone;
    }

  }  // namespace graph
}  // namespace tiz

#include "tizgraphutil.inl"

#endif  // TIZGRAPHUTIL_INL
