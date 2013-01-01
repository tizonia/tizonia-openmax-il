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
 * @file   tizapi_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Tizonia OpenMAX IL - Main OMX IL api base class
 * 
 * 
 */

#ifndef TIZAPI_DECLS_H
#define TIZAPI_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tizobject_decls.h"

struct tizapi
{
  /* Object */
  const struct Object _;

};

struct tizapi_class
{
  /* Class */
  const struct Class _;
    OMX_ERRORTYPE (*GetComponentVersion) (const void *ap_obj,
                                          OMX_HANDLETYPE ap_hdl,
                                          OMX_STRING ap_comp_name,
                                          OMX_VERSIONTYPE * ap_comp_ver,
                                          OMX_VERSIONTYPE * ap_spec_ver,
                                          OMX_UUIDTYPE * ap_comp_uuid);

    OMX_ERRORTYPE (*SendCommand) (const void *ap_obj,
                                  OMX_HANDLETYPE ap_hdl,
                                  OMX_COMMANDTYPE a_cmd,
                                  OMX_U32 a_param1, OMX_PTR ap_cmd_data);

    OMX_ERRORTYPE (*GetParameter) (const void *ap_obj,
                                   OMX_HANDLETYPE ap_hdl,
                                   OMX_INDEXTYPE a_index,
                                   OMX_PTR ap_struct);

    OMX_ERRORTYPE (*SetParameter) (const void *ap_obj,
                                   OMX_HANDLETYPE ap_hdl,
                                   OMX_INDEXTYPE a_index,
                                   OMX_PTR ap_struct);

    OMX_ERRORTYPE (*GetConfig) (const void *ap_obj,
                                OMX_HANDLETYPE ap_hdl,
                                OMX_INDEXTYPE a_index,
                                OMX_PTR ap_struct);

    OMX_ERRORTYPE (*SetConfig) (const void *ap_obj,
                                OMX_HANDLETYPE ap_hdl,
                                OMX_INDEXTYPE a_index,
                                OMX_PTR ap_struct);

    OMX_ERRORTYPE (*GetExtensionIndex) (const void *ap_obj,
                                        OMX_HANDLETYPE ap_hdl,
                                        OMX_STRING a_param_name,
                                        OMX_INDEXTYPE * ap_index_type);

    OMX_ERRORTYPE (*GetState) (const void *ap_obj,
                               OMX_HANDLETYPE ap_hdl,
                               OMX_STATETYPE * ap_state);

    OMX_ERRORTYPE (*ComponentTunnelRequest) (const void *ap_obj,
                                             OMX_HANDLETYPE ap_hdl,
                                             OMX_U32 a_port,
                                             OMX_HANDLETYPE ap_tunn_comp,
                                             OMX_U32 a_tunn_port,
                                             OMX_TUNNELSETUPTYPE *
                                             ap_tunn_setup);

    OMX_ERRORTYPE (*UseBuffer) (const void *ap_obj,
                                OMX_HANDLETYPE ap_hdl,
                                OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                                OMX_U32 a_port_index,
                                OMX_PTR ap_app_private,
                                OMX_U32 a_size_bytes, OMX_U8 * ap_buf);

    OMX_ERRORTYPE (*AllocateBuffer) (const void *ap_obj,
                                     OMX_HANDLETYPE ap_hdl,
                                     OMX_BUFFERHEADERTYPE ** pap_buf,
                                     OMX_U32 a_port_index,
                                     OMX_PTR ap_app_private, OMX_U32 a_size_bytes);

    OMX_ERRORTYPE (*FreeBuffer) (const void *ap_obj,
                                 OMX_HANDLETYPE ap_hdl,
                                 OMX_U32 a_port_index,
                                 OMX_BUFFERHEADERTYPE * ap_buf);

    OMX_ERRORTYPE (*EmptyThisBuffer) (const void *ap_obj,
                                      OMX_HANDLETYPE ap_hdl,
                                      OMX_BUFFERHEADERTYPE * ap_buf);

    OMX_ERRORTYPE (*FillThisBuffer) (const void *ap_obj,
                                     OMX_HANDLETYPE ap_hdl,
                                     OMX_BUFFERHEADERTYPE * ap_buf);

    OMX_ERRORTYPE (*SetCallbacks) (const void *ap_obj,
                                   OMX_HANDLETYPE ap_hdl,
                                   OMX_CALLBACKTYPE * ap_callbacks,
                                   OMX_PTR ap_app_data);

    OMX_ERRORTYPE (*ComponentDeInit) (const void *ap_obj,
                                      OMX_HANDLETYPE ap_hdl);

    OMX_ERRORTYPE (*UseEGLImage) (const void *ap_obj,
                                  OMX_HANDLETYPE ap_hdl,
                                  OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                                  OMX_U32 a_port_index,
                                  OMX_PTR ap_app_private, void *eglImage);

    OMX_ERRORTYPE (*ComponentRoleEnum) (const void *ap_obj,
                                        OMX_HANDLETYPE ap_hdl,
                                        OMX_U8 * a_role, OMX_U32 a_index);

};

OMX_ERRORTYPE
super_GetComponentVersion (const void *class,
                           const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_STRING ap_comp_name,
                           OMX_VERSIONTYPE * ap_comp_ver,
                           OMX_VERSIONTYPE * ap_spec_ver,
                           OMX_UUIDTYPE * ap_comp_uuid);

OMX_ERRORTYPE
super_SendCommand (const void *a_class,
                   const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data);

OMX_ERRORTYPE
super_GetParameter (const void *a_class,
                    const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_INDEXTYPE a_index,
                    OMX_PTR ap_struct);

OMX_ERRORTYPE
super_SetParameter (const void *a_class,
                    const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_INDEXTYPE a_index,
                    OMX_PTR ap_struct);

OMX_ERRORTYPE
super_GetConfig (const void *a_class,
                 const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_INDEXTYPE a_index, OMX_PTR ap_struct);

OMX_ERRORTYPE
super_SetConfig (const void *a_class,
                 const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_INDEXTYPE a_index, OMX_PTR ap_struct);

OMX_ERRORTYPE
super_GetExtensionIndex (const void *a_class,
                         const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_STRING a_param_name,
                         OMX_INDEXTYPE * ap_index_type);

OMX_ERRORTYPE
super_GetState (const void *a_class,
                const void *ap_obj,
                OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state);

OMX_ERRORTYPE
super_ComponentTunnelRequest (const void *a_class,
                              const void *ap_obj,
                              OMX_HANDLETYPE ap_hdl,
                              OMX_U32 a_port,
                              OMX_HANDLETYPE ap_tunn_comp,
                              OMX_U32 a_tunn_port,
                              OMX_TUNNELSETUPTYPE * ap_tunn_setup);

OMX_ERRORTYPE
super_UseBuffer (const void *a_class,
                 const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                 OMX_U32 a_port_index,
                 OMX_PTR ap_app_private, OMX_U32 a_size_bytes, OMX_U8 * ap_buf);

OMX_ERRORTYPE
super_AllocateBuffer (const void *a_class,
                      const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_BUFFERHEADERTYPE ** pap_buf,
                      OMX_U32 a_port_index,
                      OMX_PTR ap_app_private, OMX_U32 a_size_bytes);

OMX_ERRORTYPE
super_FreeBuffer (const void *a_class,
                  const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_U32 a_port_index, OMX_BUFFERHEADERTYPE * ap_buf);

OMX_ERRORTYPE
super_EmptyThisBuffer (const void *a_class,
                       const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl,
                       OMX_BUFFERHEADERTYPE * ap_buf);

OMX_ERRORTYPE
super_FillThisBuffer (const void *a_class,
                      const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_BUFFERHEADERTYPE * ap_buf);

OMX_ERRORTYPE
super_SetCallbacks (const void *a_class,
                    const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_CALLBACKTYPE * ap_callbacks, OMX_PTR ap_app_data);

OMX_ERRORTYPE
super_ComponentDeInit (const void *a_class,
                       const void *ap_obj, OMX_HANDLETYPE ap_hdl);

OMX_ERRORTYPE
super_UseEGLImage (const void *a_class,
                   const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                   OMX_U32 a_port_index, OMX_PTR ap_app_private, void *eglImage);

OMX_ERRORTYPE
super_ComponentRoleEnum (const void *a_class,
                         const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_U8 * a_role, OMX_U32 a_index);




#ifdef __cplusplus
}
#endif

#endif /* TIZAPI_DECLS_H */
