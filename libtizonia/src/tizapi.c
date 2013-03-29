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
 * @file   tizapi.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Main OMX IL api base class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizapi.h"
#include "tizapi_decls.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.api"
#endif


/*
 * tizapi
 */

static void *
api_ctor (void *ap_obj, va_list * app)
{
  struct tizapi *p_obj = super_ctor (tizapi, ap_obj, app);
  return p_obj;
}

static void *
api_dtor (void *ap_obj)
{
  return super_dtor (tizapi, ap_obj);
}

static OMX_ERRORTYPE
api_GetComponentVersion (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_STRING ap_comp_name,
                         OMX_VERSIONTYPE * ap_comp_ver,
                         OMX_VERSIONTYPE * ap_spec_ver,
                         OMX_UUIDTYPE * ap_comp_uuid)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_GetComponentVersion (const void *ap_obj,
                            OMX_HANDLETYPE ap_hdl,
                            OMX_STRING ap_comp_name,
                            OMX_VERSIONTYPE * ap_comp_ver,
                            OMX_VERSIONTYPE * ap_spec_ver,
                            OMX_UUIDTYPE * ap_comp_uuid)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->GetComponentVersion);
  return class->GetComponentVersion (ap_obj,
                                     ap_hdl,
                                     ap_comp_name,
                                     ap_comp_ver, ap_spec_ver, ap_comp_uuid);
}

OMX_ERRORTYPE
super_GetComponentVersion (const void *a_class,
                           const void *ap_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_STRING ap_comp_name,
                           OMX_VERSIONTYPE * ap_comp_ver,
                           OMX_VERSIONTYPE * ap_spec_ver,
                           OMX_UUIDTYPE * ap_comp_uuid)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->GetComponentVersion);
  return superclass->GetComponentVersion (ap_obj,
                                          ap_hdl,
                                          ap_comp_name,
                                          ap_comp_ver,
                                          ap_spec_ver, ap_comp_uuid);
}

static OMX_ERRORTYPE
api_SendCommand (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_SendCommand (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                    OMX_PTR ap_cmd_data)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->SendCommand);
  return class->SendCommand (ap_obj, ap_hdl, a_cmd, a_param1, ap_cmd_data);
}

OMX_ERRORTYPE
super_SendCommand (const void *a_class,
                   const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1,
                   OMX_PTR ap_cmd_data)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->SendCommand);
  return superclass->SendCommand (ap_obj, ap_hdl, a_cmd, a_param1,
                                  ap_cmd_data);
}

static OMX_ERRORTYPE
api_GetParameter (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_GetParameter (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl,
                     OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->GetParameter);
  return class->GetParameter (ap_obj, ap_hdl, a_index, ap_struct);
}

OMX_ERRORTYPE
super_GetParameter (const void *a_class,
                    const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->GetParameter);
  return superclass->GetParameter (ap_obj, ap_hdl, a_index, ap_struct);
}


static OMX_ERRORTYPE
api_SetParameter (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_SetParameter (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl,
                     OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->SetParameter);
  return class->SetParameter (ap_obj, ap_hdl, a_index, ap_struct);
}

OMX_ERRORTYPE
super_SetParameter (const void *a_class,
                    const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->SetParameter);
  return superclass->SetParameter (ap_obj, ap_hdl, a_index, ap_struct);
}


static OMX_ERRORTYPE
api_GetConfig (const void *ap_obj,
               OMX_HANDLETYPE ap_hdl, OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_GetConfig (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->GetConfig);
  return class->GetConfig (ap_obj, ap_hdl, a_index, ap_struct);
}

OMX_ERRORTYPE
super_GetConfig (const void *a_class,
                 const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->GetConfig);
  return superclass->GetConfig (ap_obj, ap_hdl, a_index, ap_struct);
}

static OMX_ERRORTYPE
api_SetConfig (const void *ap_obj,
               OMX_HANDLETYPE ap_hdl, OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_SetConfig (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->SetConfig);
  return class->SetConfig (ap_obj, ap_hdl, a_index, ap_struct);
}

OMX_ERRORTYPE
super_SetConfig (const void *a_class,
                 const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->SetConfig);
  return superclass->SetConfig (ap_obj, ap_hdl, a_index, ap_struct);
}

static OMX_ERRORTYPE
api_GetExtensionIndex (const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl,
                       OMX_STRING a_param_name, OMX_INDEXTYPE * ap_index_type)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_GetExtensionIndex (const void *ap_obj,
                          OMX_HANDLETYPE ap_hdl,
                          OMX_STRING a_param_name,
                          OMX_INDEXTYPE * ap_index_type)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->GetExtensionIndex);
  return class->GetExtensionIndex (ap_obj,
                                   ap_hdl, a_param_name, ap_index_type);
}

OMX_ERRORTYPE
super_GetExtensionIndex (const void *a_class,
                         const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_STRING a_param_name,
                         OMX_INDEXTYPE * ap_index_type)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->GetExtensionIndex);
  return superclass->GetExtensionIndex (ap_obj,
                                        ap_hdl, a_param_name, ap_index_type);
}

static OMX_ERRORTYPE
api_GetState (const void *ap_obj,
              OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_GetState (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->GetState);
  return class->GetState (ap_obj, ap_hdl, ap_state);
}

OMX_ERRORTYPE
super_GetState (const void *a_class,
                const void *ap_obj,
                OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->GetState);
  return superclass->GetState (ap_obj, ap_hdl, ap_state);
}

static OMX_ERRORTYPE
api_ComponentTunnelRequest (const void *ap_obj,
                            OMX_HANDLETYPE ap_hdl,
                            OMX_U32 a_port,
                            OMX_HANDLETYPE ap_tunn_comp,
                            OMX_U32 a_tunn_port,
                            OMX_TUNNELSETUPTYPE * ap_tunn_setup)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_ComponentTunnelRequest (const void *ap_obj,
                               OMX_HANDLETYPE ap_hdl,
                               OMX_U32 a_port,
                               OMX_HANDLETYPE ap_tunn_comp,
                               OMX_U32 a_tunn_port,
                               OMX_TUNNELSETUPTYPE * ap_tunn_setup)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->ComponentTunnelRequest);
  return class->ComponentTunnelRequest (ap_obj,
                                        ap_hdl,
                                        a_port,
                                        ap_tunn_comp,
                                        a_tunn_port, ap_tunn_setup);
}

OMX_ERRORTYPE
super_ComponentTunnelRequest (const void *a_class,
                              const void *ap_obj,
                              OMX_HANDLETYPE ap_hdl,
                              OMX_U32 a_port,
                              OMX_HANDLETYPE ap_tunn_comp,
                              OMX_U32 a_tunn_port,
                              OMX_TUNNELSETUPTYPE * ap_tunn_setup)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->ComponentTunnelRequest);
  return superclass->ComponentTunnelRequest (ap_obj,
                                             ap_hdl,
                                             a_port,
                                             ap_tunn_comp,
                                             a_tunn_port, ap_tunn_setup);
}

static OMX_ERRORTYPE
api_UseBuffer (const void *ap_obj,
               OMX_HANDLETYPE ap_hdl,
               OMX_BUFFERHEADERTYPE ** app_hdr,
               OMX_U32 a_pid,
               OMX_PTR ap_apppriv, OMX_U32 a_size, OMX_U8 * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_UseBuffer (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_BUFFERHEADERTYPE ** app_hdr,
                  OMX_U32 a_pid,
                  OMX_PTR ap_apppriv, OMX_U32 a_size, OMX_U8 * ap_buf)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->UseBuffer);
  return class->UseBuffer (ap_obj,
                           ap_hdl, app_hdr, a_pid, ap_apppriv, a_size, ap_buf);
}

OMX_ERRORTYPE
super_UseBuffer (const void *a_class,
                 const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_BUFFERHEADERTYPE ** app_hdr,
                 OMX_U32 a_pid,
                 OMX_PTR ap_apppriv, OMX_U32 a_size, OMX_U8 * ap_buf)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->UseBuffer);
  return superclass->UseBuffer (ap_obj,
                                ap_hdl,
                                app_hdr, a_pid, ap_apppriv, a_size, ap_buf);
}

static OMX_ERRORTYPE
api_AllocateBuffer (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_BUFFERHEADERTYPE ** app_hdr,
                    OMX_U32 a_pid, OMX_PTR ap_apppriv, OMX_U32 a_size)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_AllocateBuffer (const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl,
                       OMX_BUFFERHEADERTYPE ** app_hdr,
                       OMX_U32 a_pid, OMX_PTR ap_apppriv, OMX_U32 a_size)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->AllocateBuffer);
  return class->AllocateBuffer (ap_obj,
                                ap_hdl, app_hdr, a_pid, ap_apppriv, a_size);
}

OMX_ERRORTYPE
super_AllocateBuffer (const void *a_class,
                      const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_BUFFERHEADERTYPE ** app_hdr,
                      OMX_U32 a_pid, OMX_PTR ap_apppriv, OMX_U32 a_size)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->AllocateBuffer);
  return superclass->AllocateBuffer (ap_obj,
                                     ap_hdl,
                                     app_hdr, a_pid, ap_apppriv, a_size);
}

static OMX_ERRORTYPE
api_FreeBuffer (const void *ap_obj,
                OMX_HANDLETYPE ap_hdl,
                OMX_U32 a_port_index, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_FreeBuffer (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_U32 a_port_index, OMX_BUFFERHEADERTYPE * ap_buf)
{
  const struct tiz_api_class *class = classOf (ap_obj);

  assert (class->FreeBuffer);
  return class->FreeBuffer (ap_obj, ap_hdl, a_port_index, ap_buf);
}

OMX_ERRORTYPE
super_FreeBuffer (const void *a_class,
                  const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_U32 a_port_index, OMX_BUFFERHEADERTYPE * ap_buf)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->FreeBuffer);
  return superclass->FreeBuffer (ap_obj, ap_hdl, a_port_index, ap_buf);
}

static OMX_ERRORTYPE
api_EmptyThisBuffer (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_EmptyThisBuffer (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_buf)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->EmptyThisBuffer);
  return class->EmptyThisBuffer (ap_obj, ap_hdl, ap_buf);
}

OMX_ERRORTYPE
super_EmptyThisBuffer (const void *a_class,
                       const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_buf)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->EmptyThisBuffer);
  return superclass->EmptyThisBuffer (ap_obj, ap_hdl, ap_buf);
}


static OMX_ERRORTYPE
api_FillThisBuffer (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_FillThisBuffer (const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_buf)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->FillThisBuffer);
  return class->FillThisBuffer (ap_obj, ap_hdl, ap_buf);
}

OMX_ERRORTYPE
super_FillThisBuffer (const void *a_class,
                      const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_buf)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->FillThisBuffer);
  return superclass->FillThisBuffer (ap_obj, ap_hdl, ap_buf);
}

static OMX_ERRORTYPE
api_SetCallbacks (const void *ap_obj,
                  OMX_HANDLETYPE ap_hdl,
                  OMX_CALLBACKTYPE * ap_callbacks, OMX_PTR ap_app_data)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_SetCallbacks (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl,
                     OMX_CALLBACKTYPE * ap_callbacks, OMX_PTR ap_app_data)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->SetCallbacks);
  return class->SetCallbacks (ap_obj, ap_hdl, ap_callbacks, ap_app_data);
}

OMX_ERRORTYPE
super_SetCallbacks (const void *a_class,
                    const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_CALLBACKTYPE * ap_callbacks, OMX_PTR ap_app_data)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->SetCallbacks);
  return superclass->SetCallbacks (ap_obj, ap_hdl, ap_callbacks, ap_app_data);
}


static OMX_ERRORTYPE
api_ComponentDeInit (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_ComponentDeInit (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  const struct tiz_api_class *class = classOf (ap_obj);
  assert (class->ComponentDeInit);
  return class->ComponentDeInit (ap_obj, ap_hdl);
}

OMX_ERRORTYPE
super_ComponentDeInit (const void *a_class,
                       const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  const struct tiz_api_class *superclass = super (a_class);
  assert (ap_obj && superclass->ComponentDeInit);
  return superclass->ComponentDeInit (ap_obj, ap_hdl);
}


static OMX_ERRORTYPE
api_UseEGLImage (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                 OMX_U32 a_port_index, OMX_PTR ap_app_private, void *eglImage)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_UseEGLImage (const void *ap_obj,
                    OMX_HANDLETYPE ap_hdl,
                    OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                    OMX_U32 a_port_index, OMX_PTR ap_app_private,
                    void *eglImage)
{
  const struct tiz_api_class *class = classOf (ap_obj);

  assert (class->UseEGLImage);
  return class->UseEGLImage (ap_obj,
                             ap_hdl,
                             app_buf_hdr, a_port_index, ap_app_private,
                             eglImage);
}

OMX_ERRORTYPE
super_UseEGLImage (const void *a_class,
                   const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_BUFFERHEADERTYPE ** app_buf_hdr,
                   OMX_U32 a_port_index, OMX_PTR ap_app_private,
                   void *eglImage)
{
  const struct tiz_api_class *superclass = super (a_class);

  assert (ap_obj && superclass->UseEGLImage);
  return superclass->UseEGLImage (ap_obj,
                                  ap_hdl,
                                  app_buf_hdr,
                                  a_port_index, ap_app_private, eglImage);
}

static OMX_ERRORTYPE
api_ComponentRoleEnum (const void *ap_obj,
                       OMX_HANDLETYPE ap_hdl, OMX_U8 * a_role, OMX_U32 a_index)
{
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
tiz_api_ComponentRoleEnum (const void *ap_obj,
                          OMX_HANDLETYPE ap_hdl,
                          OMX_U8 * a_role, OMX_U32 a_index)
{
  const struct tiz_api_class *class = classOf (ap_obj);

  assert (class->ComponentRoleEnum);
  return class->ComponentRoleEnum (ap_obj, ap_hdl, a_role, a_index);
}

OMX_ERRORTYPE
super_ComponentRoleEnum (const void *a_class,
                         const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_U8 * a_role, OMX_U32 a_index)
{
  const struct tiz_api_class *superclass = super (a_class);

  assert (ap_obj && superclass->ComponentRoleEnum);
  return superclass->ComponentRoleEnum (ap_obj, ap_hdl, a_role, a_index);
}

/*
 * tiz_api_class
 */

static void *
api_class_ctor (void *ap_obj, va_list * app)
{
  struct tiz_api_class *p_obj = super_ctor (tiz_api_class, ap_obj, app);
  typedef void (*voidf) ();
  voidf selector;
  va_list ap;
  va_copy (ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) tiz_api_GetComponentVersion)
        {
          *(voidf *) & p_obj->GetComponentVersion = method;
        }
      else if (selector == (voidf) tiz_api_SendCommand)
        {
          *(voidf *) & p_obj->SendCommand = method;
        }
      else if (selector == (voidf) tiz_api_GetParameter)
        {
          *(voidf *) & p_obj->GetParameter = method;
        }
      else if (selector == (voidf) tiz_api_SetParameter)
        {
          *(voidf *) & p_obj->SetParameter = method;
        }
      else if (selector == (voidf) tiz_api_GetConfig)
        {
          *(voidf *) & p_obj->GetConfig = method;
        }
      else if (selector == (voidf) tiz_api_SetConfig)
        {
          *(voidf *) & p_obj->SetConfig = method;
        }
      else if (selector == (voidf) tiz_api_GetExtensionIndex)
        {
          *(voidf *) & p_obj->GetExtensionIndex = method;
        }
      else if (selector == (voidf) tiz_api_GetState)
        {
          *(voidf *) & p_obj->GetState = method;
        }
      else if (selector == (voidf) tiz_api_ComponentTunnelRequest)
        {
          *(voidf *) & p_obj->ComponentTunnelRequest = method;
        }
      else if (selector == (voidf) tiz_api_UseBuffer)
        {
          *(voidf *) & p_obj->UseBuffer = method;
        }
      else if (selector == (voidf) tiz_api_AllocateBuffer)
        {
          *(voidf *) & p_obj->AllocateBuffer = method;
        }
      else if (selector == (voidf) tiz_api_FreeBuffer)
        {
          *(voidf *) & p_obj->FreeBuffer = method;
        }
      else if (selector == (voidf) tiz_api_EmptyThisBuffer)
        {
          *(voidf *) & p_obj->EmptyThisBuffer = method;
        }
      else if (selector == (voidf) tiz_api_FillThisBuffer)
        {
          *(voidf *) & p_obj->FillThisBuffer = method;
        }
      else if (selector == (voidf) tiz_api_SetCallbacks)
        {
          *(voidf *) & p_obj->SetCallbacks = method;
        }
      else if (selector == (voidf) tiz_api_ComponentDeInit)
        {
          *(voidf *) & p_obj->ComponentDeInit = method;
        }
      else if (selector == (voidf) tiz_api_UseEGLImage)
        {
          *(voidf *) & p_obj->UseEGLImage = method;
        }
      else if (selector == (voidf) tiz_api_ComponentRoleEnum)
        {
          *(voidf *) & p_obj->ComponentRoleEnum = method;
        }

    }

  va_end (ap);

  return p_obj;
}

/*
 * initialization
 */

const void *tiz_api_class, *tizapi;

void
init_tizapi (void)
{

  if (!tiz_api_class)
    {
      tiz_api_class = factory_new (Class,
                                  "tiz_api_class",
                                  Class,
                                  sizeof (struct tiz_api_class),
                                  ctor, api_class_ctor, 0);
    }

  if (!tizapi)
    {
      tizapi =
        factory_new
        (tiz_api_class,
         "tizapi",
         Object,
         sizeof (struct tizapi),
         ctor, api_ctor,
         dtor, api_dtor,
         tiz_api_GetComponentVersion, api_GetComponentVersion,
         tiz_api_SendCommand, api_SendCommand,
         tiz_api_GetParameter, api_GetParameter,
         tiz_api_SetParameter, api_SetParameter,
         tiz_api_GetConfig, api_GetConfig,
         tiz_api_SetConfig, api_SetConfig,
         tiz_api_GetExtensionIndex, api_GetExtensionIndex,
         tiz_api_GetState, api_GetState,
         tiz_api_ComponentTunnelRequest, api_ComponentTunnelRequest,
         tiz_api_UseBuffer, api_UseBuffer,
         tiz_api_AllocateBuffer, api_AllocateBuffer,
         tiz_api_FreeBuffer, api_FreeBuffer,
         tiz_api_EmptyThisBuffer, api_EmptyThisBuffer,
         tiz_api_FillThisBuffer, api_FillThisBuffer,
         tiz_api_SetCallbacks, api_SetCallbacks,
         tiz_api_ComponentDeInit, api_ComponentDeInit,
         tiz_api_UseEGLImage, api_UseEGLImage,
         tiz_api_ComponentRoleEnum, api_ComponentRoleEnum, 0);
    }

}
