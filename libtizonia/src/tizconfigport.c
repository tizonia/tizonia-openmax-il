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
 * @file   tizconfigport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - configport class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizconfigport.h"
#include "tizconfigport_decls.h"
#include "tizport.h"
#include "tizosal.h"

#include <assert.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.configport"
#endif

/*
 * tizconfigport class
 */

static void *
configport_ctor (void *ap_obj, va_list * app)
{
  tiz_configport_t *p_obj = super_ctor (typeOf (ap_obj, "tizconfigport"), ap_obj, app);
  tiz_port_t *p_base = ap_obj;
  size_t str_len = 0;
  
  /* Make an internal copy of the component name */
  strncpy (p_obj->comp_name_, va_arg (*app, char *), OMX_MAX_STRINGNAME_SIZE - 1);
  str_len = strnlen (p_obj->comp_name_, OMX_MAX_STRINGNAME_SIZE - 1);
  p_obj->comp_name_[str_len] = '\0';

  TIZ_TRACE (tiz_api_get_hdl (ap_obj),
            "comp_name_ [%s]...", p_obj->comp_name_);

  /* Component version */
  p_obj->comp_ver_ = va_arg (*app, OMX_VERSIONTYPE);

  /* Init the OMX IL structs */

  /* TODO: One day, these should probably be exposed via constructor arguments */

  /* OMX_RESOURCECONCEALMENTTYPE */
  p_obj->param_rc_.nSize = sizeof (OMX_RESOURCECONCEALMENTTYPE);
  p_obj->param_rc_.nVersion.nVersion = OMX_VERSION;
  p_obj->param_rc_.bResourceConcealmentForbidden = OMX_TRUE;

  /* OMX_PARAM_SUSPENSIONPOLICYTYPE */
  p_obj->param_sp_.nSize = sizeof (OMX_PARAM_SUSPENSIONPOLICYTYPE);
  p_obj->param_sp_.nVersion.nVersion = OMX_VERSION;
  p_obj->param_sp_.ePolicy = OMX_SuspensionDisabled;

  /* OMX_PRIORITYMGMTTYPE */
  p_obj->config_pm_.nSize = sizeof (OMX_PRIORITYMGMTTYPE);
  p_obj->config_pm_.nVersion.nVersion = OMX_VERSION;
  p_obj->config_pm_.nGroupPriority = 0;
  p_obj->config_pm_.nGroupID = 0;

  /* This is a bit ugly... */
  /* ... we clear the indexes added by the base port class */
  tiz_vector_clear (p_base->p_indexes_);

  /* Now register the indexes we are interested in */
  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexParamDisableResourceConcealment));
  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexParamSuspensionPolicy));
  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexParamPriorityMgmt));
  tiz_check_omx_err_ret_null
    (tiz_port_register_index (p_obj, OMX_IndexConfigPriorityMgmt));

  /* Generate the uuid */
  tiz_uuid_generate (&p_obj->uuid_);

  return p_obj;
}

static void *
configport_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tizconfigport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
configport_GetComponentVersion (const void *ap_obj,
                                OMX_HANDLETYPE ap_hdl,
                                OMX_STRING ap_comp_name,
                                OMX_VERSIONTYPE * ap_comp_version,
                                OMX_VERSIONTYPE * ap_spec_version,
                                OMX_UUIDTYPE * ap_comp_uuid)
{
  const tiz_configport_t *p_obj = ap_obj;

  TIZ_TRACE (ap_hdl, "GetComponentVersion...");

  strcpy (ap_comp_name, p_obj->comp_name_);
  *ap_comp_version = p_obj->comp_ver_;
  ap_spec_version->nVersion = OMX_VERSION;

  if (ap_comp_uuid)
    {
      tiz_uuid_copy (ap_comp_uuid, &p_obj->uuid_);
    }

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
configport_GetParameter (const void *ap_obj,
                         OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_configport_t *p_obj = ap_obj;

  TIZ_TRACE (ap_hdl, "GetParameter [%s]...", tiz_idx_to_str (a_index));

  assert (NULL != p_obj);

  switch (a_index)
    {
    case OMX_IndexParamDisableResourceConcealment:
      {
        OMX_RESOURCECONCEALMENTTYPE *p_cr = ap_struct;
        *p_cr = p_obj->param_rc_;
      }
      break;

    case OMX_IndexParamSuspensionPolicy:
      {
        OMX_PARAM_SUSPENSIONPOLICYTYPE *p_sp = ap_struct;
        *p_sp = p_obj->param_sp_;
      }
      break;

    case OMX_IndexParamPriorityMgmt:
      {
        OMX_PRIORITYMGMTTYPE *p_pm = ap_struct;
        *p_pm = p_obj->config_pm_;
      }
      break;

    default:
      {
        TIZ_ERROR (ap_hdl, "[OMX_ErrorUnsupportedIndex] : [0x%08x]...",
                  a_index);
        return OMX_ErrorUnsupportedIndex;
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
configport_SetParameter (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_configport_t *p_obj = (tiz_configport_t *) ap_obj;

  TIZ_TRACE (ap_hdl, "SetParameter [%s]...", tiz_idx_to_str (a_index));

  assert (NULL != p_obj);

  switch (a_index)
    {
    case OMX_IndexParamDisableResourceConcealment:
      {

        const OMX_RESOURCECONCEALMENTTYPE *p_conceal
          = (OMX_RESOURCECONCEALMENTTYPE *) ap_struct;

        p_obj->param_rc_ = *p_conceal;

      }
      break;

    case OMX_IndexParamSuspensionPolicy:
      {

        const OMX_PARAM_SUSPENSIONPOLICYTYPE *p_policy
          = (OMX_PARAM_SUSPENSIONPOLICYTYPE *) ap_struct;

        if (p_policy->ePolicy > OMX_SuspensionPolicyMax)
          {
            return OMX_ErrorBadParameter;
          }

        p_obj->param_sp_ = *p_policy;

      }
      break;

    case OMX_IndexParamPriorityMgmt:
      {

        const OMX_PRIORITYMGMTTYPE *p_prio
          = (OMX_PRIORITYMGMTTYPE *) ap_struct;

        p_obj->config_pm_ = *p_prio;

      }
      break;

    default:
      {
        TIZ_ERROR (ap_hdl, "[OMX_ErrorUnsupportedIndex] : [0x%08x]...",
                  a_index);
        return OMX_ErrorUnsupportedIndex;
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
configport_GetConfig (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_configport_t *p_obj = ap_obj;

  TIZ_TRACE (ap_hdl, "GetConfig [%s]...", tiz_idx_to_str (a_index));

  assert (NULL != p_obj);

  switch (a_index)
    {

    case OMX_IndexConfigPriorityMgmt:
      {
        OMX_PRIORITYMGMTTYPE *p_pm = ap_struct;
        *p_pm = p_obj->config_pm_;
      }
      break;

    default:
      {
        TIZ_ERROR (ap_hdl, "[OMX_ErrorUnsupportedIndex] : [0x%08x]...",
                 a_index);
        return OMX_ErrorUnsupportedIndex;
      }
    };

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
configport_SetConfig (const void *ap_obj,
                      OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_configport_t *p_obj = (tiz_configport_t *) ap_obj;

  TIZ_TRACE (ap_hdl, "SetConfig [%s]...", tiz_idx_to_str (a_index));

  assert (NULL != p_obj);

  switch (a_index)
    {

    case OMX_IndexConfigPriorityMgmt:
      {

        const OMX_PRIORITYMGMTTYPE *p_prio
          = (OMX_PRIORITYMGMTTYPE *) ap_struct;

        p_obj->config_pm_ = *p_prio;

      }
      break;

    default:
      {
        TIZ_ERROR (ap_hdl, "[OMX_ErrorUnsupportedIndex] : [0x%08x]...",
                 a_index);
        return OMX_ErrorUnsupportedIndex;
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
configport_GetExtensionIndex (const void *ap_obj,
                              OMX_HANDLETYPE ap_hdl,
                              OMX_STRING ap_param_name,
                              OMX_INDEXTYPE * ap_index_type)
{
  TIZ_TRACE (ap_hdl, "GetExtensionIndex [%s]...", ap_param_name);
  /* No extensions here. */
  return OMX_ErrorUnsupportedIndex;
}

/*
 * tizconfigport_class
 */

static void *
configport_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tizconfigport_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_configport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizconfigport_class = factory_new (classOf (tizport),
                                            "tizconfigport_class",
                                            classOf (tizport),
                                            sizeof (tiz_configport_class_t),
                                            ap_tos, ap_hdl,
                                            ctor, configport_class_ctor, 0);
  return tizconfigport_class;
}

void *
tiz_configport_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizconfigport_class = tiz_get_type (ap_hdl, "tizconfigport_class");
  TIZ_LOG_CLASS (tizconfigport_class);
  void * tizconfigport =
    factory_new
    (tizconfigport_class,
     "tizconfigport",
     tizport,
     sizeof (tiz_configport_t),
     ap_tos, ap_hdl,
     ctor, configport_ctor,
     dtor, configport_dtor,
     tiz_api_GetComponentVersion, configport_GetComponentVersion,
     tiz_api_GetParameter, configport_GetParameter,
     tiz_api_SetParameter, configport_SetParameter,
     tiz_api_GetConfig, configport_GetConfig,
     tiz_api_SetConfig, configport_SetConfig,
     tiz_api_GetExtensionIndex, configport_GetExtensionIndex, 0);

  return tizconfigport;
}
