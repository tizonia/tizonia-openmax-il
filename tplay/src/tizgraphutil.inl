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

#ifndef TIZGRAPHUTIL_INL
#define TIZGRAPHUTIL_INL

namespace tiz
{
  namespace graph
  {
      template<typename ParamT>
      OMX_ERRORTYPE util::get_channels_and_rate_from_audio_port (const OMX_HANDLETYPE  handle,
                                                                 const OMX_U32         port_id,
                                                                 const OMX_INDEXTYPE   param_index,
                                                                 OMX_U32              &channels,
                                                                 OMX_U32              &sampling_rate)

      {
        OMX_ERRORTYPE rc = OMX_ErrorNone;
        // Retrieve the current settings from the component port
        ParamT param_type;
        TIZ_INIT_OMX_PORT_STRUCT (param_type, port_id);
        tiz_check_omx_err (OMX_GetParameter (handle, param_index, &param_type));
        channels = param_type.nChannels;
        sampling_rate = param_type.nSampleRate;
        return OMX_ErrorNone;
      }

      template<typename ParamT>
      OMX_ERRORTYPE util::set_channels_and_rate_on_audio_port (const OMX_HANDLETYPE  handle,
                                                               const OMX_U32         port_id,
                                                               const OMX_INDEXTYPE   param_index,
                                                               const OMX_U32         channels,
                                                               const OMX_U32         sampling_rate)

      {
        OMX_ERRORTYPE rc = OMX_ErrorNone;
        // Retrieve the current settings from the component port
        ParamT param_type;
        TIZ_INIT_OMX_PORT_STRUCT (param_type, port_id);
        tiz_check_omx_err (OMX_GetParameter (handle, param_index, &param_type));
        param_type.nChannels = channels;
        param_type.nSampleRate = sampling_rate;
        tiz_check_omx_err (OMX_SetParameter (handle, param_index, &param_type));
        return OMX_ErrorNone;
      }

  }  // namespace graph
}  // namespace tiz

#include "tizgraphutil.inl"

#endif  // TIZGRAPHUTIL_INL
