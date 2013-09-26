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
 * @file   tizcoretc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL Core - Test Component
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <assert.h>
#include <string.h>

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Types.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.ilcore.test_comp"
#endif

#define TIZ_CORE_TEST_COMPONENT_ROLE "default"
#define TIZ_CORE_TEST_COMPONENT_NAME "OMX.Aratelia.ilcore.test_component"

static OMX_VERSIONTYPE tc_comp_version = { {1, 0, 0, 0} };

static OMX_ERRORTYPE
GetComponentVersion (OMX_HANDLETYPE ap_hdl,
                     OMX_STRING ap_comp_name,
                     OMX_VERSIONTYPE * ap_comp_ver,
                     OMX_VERSIONTYPE * ap_spec_ver,
                     OMX_UUIDTYPE * ap_comp_uuid)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "GetComponentVersion");

  if (!ap_hdl
      || !ap_comp_name || !ap_comp_ver || !ap_spec_ver || !ap_comp_uuid)
    {
      return OMX_ErrorBadParameter;
    }

  strcpy (ap_comp_name, TIZ_CORE_TEST_COMPONENT_NAME);

  *ap_comp_ver = tc_comp_version;

  ap_spec_ver->nVersion = OMX_VERSION;

  if (ap_comp_uuid)
    {
      /* TODO: assign component UUID. */
    }


  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
SendCommand (OMX_HANDLETYPE ap_hdl,
             OMX_COMMANDTYPE a_cmd, OMX_U32 a_param1, OMX_PTR ap_cmd_data)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
GetParameter (OMX_HANDLETYPE ap_hdl, OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
SetParameter (OMX_HANDLETYPE ap_hdl, OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
GetConfig (OMX_HANDLETYPE ap_hdl, OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
SetConfig (OMX_HANDLETYPE ap_hdl, OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
GetExtensionIndex (OMX_HANDLETYPE ap_hdl,
                   OMX_STRING a_param_name, OMX_INDEXTYPE * ap_index_type)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
GetState (OMX_HANDLETYPE ap_hdl, OMX_STATETYPE * ap_state)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ComponentTunnelRequest (OMX_HANDLETYPE ap_hdl,
                        OMX_U32 a_port,
                        OMX_HANDLETYPE ap_tunn_comp,
                        OMX_U32 a_tunn_port,
                        OMX_TUNNELSETUPTYPE * ap_tunn_setup)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
UseBuffer (OMX_HANDLETYPE ap_hdl,
           OMX_BUFFERHEADERTYPE ** app_buf_hdr,
           OMX_U32 a_port_index,
           OMX_PTR ap_app_private, OMX_U32 a_size_bytes, OMX_U8 * ap_buf)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
AllocateBuffer (OMX_HANDLETYPE ap_hdl,
                OMX_BUFFERHEADERTYPE ** pap_buf,
                OMX_U32 a_port_index, OMX_PTR ap_app_private,
                OMX_U32 a_size_bytes)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
FreeBuffer (OMX_HANDLETYPE ap_hdl,
            OMX_U32 a_port_index, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
EmptyThisBuffer (OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
FillThisBuffer (OMX_HANDLETYPE ap_hdl, OMX_BUFFERHEADERTYPE * ap_buf)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
SetCallbacks (OMX_HANDLETYPE ap_hdl,
              OMX_CALLBACKTYPE * ap_callbacks, OMX_PTR ap_app_data)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
ComponentDeInit (OMX_HANDLETYPE ap_hdl)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "ComponentDeInit");
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
UseEGLImage (OMX_HANDLETYPE ap_hdl,
             OMX_BUFFERHEADERTYPE ** app_buf_hdr,
             OMX_U32 a_port_index, OMX_PTR ap_app_private, void *eglImage)
{
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
ComponentRoleEnum (OMX_HANDLETYPE ap_hdl, OMX_U8 * a_role, OMX_U32 a_index)
{

  OMX_ERRORTYPE ret_val = OMX_ErrorNone;
  TIZ_LOG (TIZ_PRIORITY_TRACE, "ComponentRoleEnum : a_index [%d]", a_index);

  if (!a_role)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "ComponentRoleEnum: "
               "NULL a_role pointer found.");
      return OMX_ErrorBadParameter;
    }

  if (0 == a_index)
    {
      strcpy ((char *) a_role, TIZ_CORE_TEST_COMPONENT_ROLE);
      ret_val = OMX_ErrorNone;
    }
  else
    {
      ret_val = OMX_ErrorNoMore;
    }

  return ret_val;

}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{

  OMX_COMPONENTTYPE *p_hdl = (OMX_COMPONENTTYPE *) ap_hdl;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_ComponentInit: "
           "Inititializing the test component's hdl");

  assert (p_hdl);

  /* Fill in the component hdl */
  p_hdl->nVersion.s.nVersionMajor = 1;
  p_hdl->nVersion.s.nVersionMinor = 0;
  p_hdl->nVersion.s.nRevision = 0;
  p_hdl->nVersion.s.nStep = 0;
  p_hdl->pComponentPrivate = 0;
  p_hdl->pApplicationPrivate = 0;
  p_hdl->GetComponentVersion = GetComponentVersion;
  p_hdl->SendCommand = SendCommand;
  p_hdl->GetParameter = GetParameter;
  p_hdl->SetParameter = SetParameter;
  p_hdl->GetConfig = GetConfig;
  p_hdl->SetConfig = SetConfig;
  p_hdl->GetExtensionIndex = GetExtensionIndex;
  p_hdl->GetState = GetState;
  p_hdl->ComponentTunnelRequest = ComponentTunnelRequest;
  p_hdl->UseBuffer = UseBuffer;
  p_hdl->AllocateBuffer = AllocateBuffer;
  p_hdl->FreeBuffer = FreeBuffer;
  p_hdl->EmptyThisBuffer = EmptyThisBuffer;
  p_hdl->FillThisBuffer = FillThisBuffer;
  p_hdl->SetCallbacks = SetCallbacks;
  p_hdl->ComponentDeInit = ComponentDeInit;
  p_hdl->UseEGLImage = UseEGLImage;
  p_hdl->ComponentRoleEnum = ComponentRoleEnum;

  return OMX_ErrorNone;

}
