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
 * @file   tizapi.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Tizonia OpenMAX IL - Main OMX IL api base class
 * 
 * 
 */

#ifndef TIZAPI_H
#define TIZAPI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Core.h"
#include "OMX_Types.h"

#include "tizobject.h"

/* factory_new(tizapi, ...); */
  extern const void *tizapi;
  extern const void *tizapi_class;
  void init_tizapi (void);


    OMX_ERRORTYPE
    tizapi_GetComponentVersion (const void *ap_obj,
                                OMX_HANDLETYPE ap_hdl,
                                OMX_STRING ap_comp_name,
                                OMX_VERSIONTYPE * ap_comp_ver,
                                OMX_VERSIONTYPE * ap_spec_ver,
                                OMX_UUIDTYPE * ap_comp_uuid);

    OMX_ERRORTYPE
    tizapi_SendCommand (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_COMMANDTYPE a_cmd,
                        OMX_U32 a_param1, OMX_PTR ap_cmd_data);

    OMX_ERRORTYPE
    tizapi_GetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct);

    OMX_ERRORTYPE
    tizapi_SetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct);

    OMX_ERRORTYPE
    tizapi_GetConfig (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct);

    OMX_ERRORTYPE
    tizapi_SetConfig (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct);

    OMX_ERRORTYPE
    tizapi_GetExtensionIndex (const void *ap_obj,
                              OMX_HANDLETYPE ap_hdl,
                              OMX_STRING a_param_name,
                              OMX_INDEXTYPE * ap_index_type);

    OMX_ERRORTYPE
    tizapi_GetState (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state);

    OMX_ERRORTYPE
    tizapi_ComponentTunnelRequest (const void *ap_obj,
                                   OMX_HANDLETYPE ap_hdl,
                                   OMX_U32 a_port,
                                   OMX_HANDLETYPE ap_tunn_comp,
                                   OMX_U32 a_tunn_port,
                                   OMX_TUNNELSETUPTYPE * ap_tunn_setup);

    OMX_ERRORTYPE
    tizapi_UseBuffer (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                      OMX_U32 a_port_index,
                      OMX_PTR ap_app_private, OMX_U32 a_size_bytes,
                      OMX_U8 * ap_buf);

    OMX_ERRORTYPE
    tizapi_AllocateBuffer (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_BUFFERHEADERTYPE ** app_hdr,
                           OMX_U32 a_pid, OMX_PTR ap_apppriv, OMX_U32 a_size);

    OMX_ERRORTYPE
    tizapi_FreeBuffer (const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl,
                       OMX_U32 a_port_index, OMX_BUFFERHEADERTYPE * ap_buf);

    OMX_ERRORTYPE
    tizapi_EmptyThisBuffer (const void *ap_obj,
                            OMX_HANDLETYPE ap_hdl,
                            OMX_BUFFERHEADERTYPE * ap_buf);

    OMX_ERRORTYPE
    tizapi_FillThisBuffer (const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_BUFFERHEADERTYPE * ap_buf);

    OMX_ERRORTYPE
    tizapi_SetCallbacks (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_CALLBACKTYPE * ap_callbacks,
                         OMX_PTR ap_app_data);

    OMX_ERRORTYPE
    tizapi_ComponentDeInit (const void *ap_obj, OMX_HANDLETYPE ap_hdl);

    OMX_ERRORTYPE
    tizapi_UseEGLImage (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                        OMX_U32 a_port_index, OMX_PTR ap_app_private,
                        void *eglImage);

    OMX_ERRORTYPE
    tizapi_ComponentRoleEnum (const void *ap_obj,
                              OMX_HANDLETYPE ap_hdl,
                              OMX_U8 * a_role, OMX_U32 a_index);


#ifdef __cplusplus
}
#endif

#endif                          /* TIZAPI_H */
