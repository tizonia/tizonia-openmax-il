/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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

#include <assert.h>
#include <string.h>

#include <tizplatform.h>

#include "tizconfigport.h"
#include "tizconfigport_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.configport"
#endif

static void
clear_metadata_lst (tiz_configport_t * ap_obj)
{
  OMX_PTR * pp_metadata_item = NULL;
  assert (ap_obj);
  while (tiz_vector_length (ap_obj->p_metadata_lst_) > 0)
    {
      pp_metadata_item = tiz_vector_back (ap_obj->p_metadata_lst_);
      assert (pp_metadata_item);
      tiz_mem_free (*pp_metadata_item);
      tiz_vector_pop_back (ap_obj->p_metadata_lst_);
    }
}

static OMX_ERRORTYPE
store_metadata (tiz_configport_t * ap_obj,
                const OMX_CONFIG_METADATAITEMTYPE * ap_meta)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_obj);
  assert (ap_meta);

  {
    const OMX_U32 count = tiz_vector_length (ap_obj->p_metadata_lst_);
    assert (count == ap_obj->metadata_count_.nMetadataItemCount);
    tiz_check_omx (tiz_vector_push_back (ap_obj->p_metadata_lst_, &ap_meta));
    ap_obj->metadata_count_.nMetadataItemCount++;
    TIZ_TRACE (handleOf (ap_obj), "storing metadata [%d] [%s] - count [%u]...",
               ap_obj->metadata_count_.nMetadataItemCount, ap_meta->nKey,
               count);
  }
  return rc;
}

/*
 * tizconfigport class
 */

static void *
configport_ctor (void * ap_obj, va_list * app)
{
  tiz_configport_t * p_obj
    = super_ctor (typeOf (ap_obj, "tizconfigport"), ap_obj, app);
  tiz_port_t * p_base = ap_obj;
  size_t str_len = 0;

  /* Make an internal copy of the component name */
  strncpy (p_obj->comp_name_, va_arg (*app, char *),
           OMX_MAX_STRINGNAME_SIZE - 1);
  str_len = strnlen (p_obj->comp_name_, OMX_MAX_STRINGNAME_SIZE - 1);
  p_obj->comp_name_[str_len] = '\0';

  TIZ_TRACE (handleOf (ap_obj), "comp_name_ [%s]...", p_obj->comp_name_);

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

  /* Generate the uuid */
  tiz_uuid_generate (&p_obj->uuid_);

  p_obj->metadata_count_.nSize = sizeof (OMX_CONFIG_METADATAITEMCOUNTTYPE);
  p_obj->metadata_count_.nVersion.nVersion = OMX_VERSION;
  p_obj->metadata_count_.eScopeMode = OMX_MetadataScopeAllLevels;
  p_obj->metadata_count_.nScopeSpecifier = 0;
  p_obj->metadata_count_.nMetadataItemCount = 0;

  tiz_check_omx_ret_null (
    tiz_vector_init (&(p_obj->p_metadata_lst_), sizeof (OMX_PTR)));

  /* OMX_TIZONIA_PLAYLISTSKIPTYPE */
  p_obj->playlist_skip_.nSize = sizeof (OMX_TIZONIA_PLAYLISTSKIPTYPE);
  p_obj->playlist_skip_.nVersion.nVersion = OMX_VERSION;
  p_obj->playlist_skip_.nValue = 0;

  /* Clear the indexes added by the base port class. They are of no interest
     here and won't be handled in this class.  */
  tiz_vector_clear (p_base->p_indexes_);

  /* Register the indexes we are interested in */
  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_IndexParamDisableResourceConcealment));
  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_IndexParamSuspensionPolicy));
  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_IndexParamPriorityMgmt));
  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_IndexConfigPriorityMgmt));
  tiz_check_omx_ret_null (tiz_port_register_index (
    p_obj, OMX_IndexConfigMetadataItemCount)); /* read-only */
  tiz_check_omx_ret_null (tiz_port_register_index (
    p_obj, OMX_IndexConfigMetadataItem)); /* read-only */
  tiz_check_omx_ret_null (
    tiz_port_register_index (p_obj, OMX_TizoniaIndexConfigPlaylistSkip));

  return p_obj;
}

static void *
configport_dtor (void * ap_obj)
{
  tiz_configport_t * p_obj = ap_obj;
  clear_metadata_lst (p_obj);
  tiz_vector_destroy (p_obj->p_metadata_lst_);
  p_obj->p_metadata_lst_ = NULL;
  return super_dtor (typeOf (ap_obj, "tizconfigport"), ap_obj);
}

/*
 * from tiz_api
 */

static OMX_ERRORTYPE
configport_GetComponentVersion (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                                OMX_STRING ap_comp_name,
                                OMX_VERSIONTYPE * ap_comp_version,
                                OMX_VERSIONTYPE * ap_spec_version,
                                OMX_UUIDTYPE * ap_comp_uuid)
{
  const tiz_configport_t * p_obj = ap_obj;
  size_t str_len = 0;

  TIZ_TRACE (ap_hdl, "GetComponentVersion...");

  strncpy (ap_comp_name, p_obj->comp_name_, OMX_MAX_STRINGNAME_SIZE);
  str_len = strnlen (ap_comp_name, OMX_MAX_STRINGNAME_SIZE - 1);
  ap_comp_name[str_len] = '\0';

  *ap_comp_version = p_obj->comp_ver_;
  ap_spec_version->nVersion = OMX_VERSION;

  if (ap_comp_uuid)
    {
      tiz_uuid_copy (ap_comp_uuid, &p_obj->uuid_);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
configport_GetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_configport_t * p_obj = ap_obj;

  TIZ_TRACE (ap_hdl, "GetParameter [%s]...", tiz_idx_to_str (a_index));

  assert (p_obj);

  switch (a_index)
    {
      case OMX_IndexParamDisableResourceConcealment:
        {
          OMX_RESOURCECONCEALMENTTYPE * p_cr = ap_struct;
          *p_cr = p_obj->param_rc_;
        }
        break;

      case OMX_IndexParamSuspensionPolicy:
        {
          OMX_PARAM_SUSPENSIONPOLICYTYPE * p_sp = ap_struct;
          *p_sp = p_obj->param_sp_;
        }
        break;

      case OMX_IndexParamPriorityMgmt:
        {
          OMX_PRIORITYMGMTTYPE * p_pm = ap_struct;
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
configport_SetParameter (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_configport_t * p_obj = (tiz_configport_t *) ap_obj;

  TIZ_TRACE (ap_hdl, "SetParameter [%s]...", tiz_idx_to_str (a_index));

  assert (p_obj);

  switch (a_index)
    {
      case OMX_IndexParamDisableResourceConcealment:
        {

          const OMX_RESOURCECONCEALMENTTYPE * p_conceal
            = (OMX_RESOURCECONCEALMENTTYPE *) ap_struct;

          p_obj->param_rc_ = *p_conceal;
        }
        break;

      case OMX_IndexParamSuspensionPolicy:
        {

          const OMX_PARAM_SUSPENSIONPOLICYTYPE * p_policy
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

          const OMX_PRIORITYMGMTTYPE * p_prio
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
configport_GetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const tiz_configport_t * p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "GetConfig [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
      case OMX_IndexConfigPriorityMgmt:
        {
          OMX_PRIORITYMGMTTYPE * p_pm = ap_struct;
          *p_pm = p_obj->config_pm_;
        }
        break;

      case OMX_IndexConfigMetadataItemCount:
        {
          OMX_CONFIG_METADATAITEMCOUNTTYPE * p_meta_count = ap_struct;
          *p_meta_count = p_obj->metadata_count_;
        }
        break;

      case OMX_IndexConfigMetadataItem:
        {
          OMX_CONFIG_METADATAITEMTYPE * p_meta = ap_struct;
          assert (tiz_vector_length (p_obj->p_metadata_lst_)
                  == p_obj->metadata_count_.nMetadataItemCount);
          if (p_meta->nMetadataItemIndex
              >= p_obj->metadata_count_.nMetadataItemCount)
            {
              rc = OMX_ErrorNoMore;
            }
          else
            {
              OMX_PTR * pp_value = NULL;
              OMX_CONFIG_METADATAITEMTYPE * p_value = NULL;
              pp_value = tiz_vector_at (p_obj->p_metadata_lst_,
                                        p_meta->nMetadataItemIndex);
              assert (pp_value && *pp_value);
              p_value = (OMX_CONFIG_METADATAITEMTYPE *) *pp_value;
              strncpy ((char *) p_meta->nKey, (char *) p_value->nKey, 128);
              p_meta->nKeySizeUsed = strnlen ((char *) p_meta->nKey, 128);
              strncpy ((char *) p_meta->nValue, (char *) p_value->nValue,
                       p_meta->nValueMaxSize);
              p_meta->nValueSizeUsed
                = strnlen ((char *) p_meta->nValue, p_meta->nValueMaxSize);
              TIZ_TRACE (handleOf (ap_obj), "key at [%d] = [%s]...",
                         p_meta->nMetadataItemIndex, p_value->nKey);
            }
        }
        break;

      default:
        {
          if (OMX_TizoniaIndexConfigPlaylistSkip == a_index)
            {
              OMX_TIZONIA_PLAYLISTSKIPTYPE * p_playlist_skip = ap_struct;
              *p_playlist_skip = p_obj->playlist_skip_;
            }
          else
            {
              TIZ_ERROR (ap_hdl, "[OMX_ErrorUnsupportedIndex] : [0x%08x]...",
                         a_index);
              rc = OMX_ErrorUnsupportedIndex;
            }
        }
        break;
    };

  return rc;
}

static OMX_ERRORTYPE
configport_SetConfig (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  tiz_configport_t * p_obj = (tiz_configport_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_TRACE (ap_hdl, "SetConfig [%s]...", tiz_idx_to_str (a_index));

  switch (a_index)
    {
      case OMX_IndexConfigPriorityMgmt:
        {
          const OMX_PRIORITYMGMTTYPE * p_prio
            = (OMX_PRIORITYMGMTTYPE *) ap_struct;
          p_obj->config_pm_ = *p_prio;
        }
        break;

      case OMX_IndexConfigMetadataItemCount:
      case OMX_IndexConfigMetadataItem:
        {
          /* These are read-only indexes. Simply ignore them. */
          TIZ_NOTICE (ap_hdl, "Ignoring read-only index [%s] ",
                      tiz_idx_to_str (a_index));
        }
        break;

      default:
        {
          if (OMX_TizoniaIndexConfigPlaylistSkip == a_index)
            {
              const OMX_TIZONIA_PLAYLISTSKIPTYPE * p_playlist_skip
                = (OMX_TIZONIA_PLAYLISTSKIPTYPE *) ap_struct;
              p_obj->playlist_skip_ = *p_playlist_skip;
            }
          else
            {
              TIZ_ERROR (ap_hdl, "[OMX_ErrorUnsupportedIndex] : [0x%08x]...",
                         a_index);
              rc = OMX_ErrorUnsupportedIndex;
            }
        }
        break;
    };

  return rc;
}

static OMX_ERRORTYPE
configport_GetExtensionIndex (const void * ap_obj, OMX_HANDLETYPE ap_hdl,
                              OMX_STRING ap_param_name,
                              OMX_INDEXTYPE * ap_index_type)
{
  TIZ_TRACE (ap_hdl, "GetExtensionIndex [%s]...", ap_param_name);
  /* No extensions here. */
  return OMX_ErrorUnsupportedIndex;
}

static void
configport_clear_metadata (void * ap_obj)
{
  tiz_configport_t * p_obj = ap_obj;
  assert (p_obj);
  clear_metadata_lst (ap_obj);
  p_obj->metadata_count_.nMetadataItemCount = 0;
}

void
tiz_configport_clear_metadata (void * ap_obj)
{
  const tiz_configport_class_t * class = classOf (ap_obj);
  assert (class->clear_metadata);
  return class->clear_metadata (ap_obj);
}

static OMX_ERRORTYPE
configport_store_metadata (void * ap_obj,
                           const OMX_CONFIG_METADATAITEMTYPE * ap_meta_item)
{
  return store_metadata (ap_obj, ap_meta_item);
}

OMX_ERRORTYPE
tiz_configport_store_metadata (void * ap_obj,
                               const OMX_CONFIG_METADATAITEMTYPE * ap_meta_item)
{
  const tiz_configport_class_t * class = classOf (ap_obj);
  assert (class->store_metadata);
  return class->store_metadata (ap_obj, ap_meta_item);
}

/*
 * tizconfigport_class
 */

static void *
configport_class_ctor (void * ap_obj, va_list * app)
{
  tiz_configport_class_t * p_obj
    = super_ctor (typeOf (ap_obj, "tizconfigport_class"), ap_obj, app);
  typedef void (*voidf) ();
  voidf selector = NULL;
  va_list ap;
  va_copy (ap, *app);

  /* NOTE: Start ignoring splint warnings in this section of code */
  /*@ignore@*/
  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) tiz_configport_clear_metadata)
        {
          *(voidf *) &p_obj->clear_metadata = method;
        }
      else if (selector == (voidf) tiz_configport_store_metadata)
        {
          *(voidf *) &p_obj->store_metadata = method;
        }
    }
  /*@end@*/
  /* NOTE: Stop ignoring splint warnings in this section  */

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

void *
tiz_configport_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizconfigport_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizport), "tizconfigport_class", classOf (tizport),
     sizeof (tiz_configport_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, configport_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return tizconfigport_class;
}

void *
tiz_configport_init (void * ap_tos, void * ap_hdl)
{
  void * tizport = tiz_get_type (ap_hdl, "tizport");
  void * tizconfigport_class = tiz_get_type (ap_hdl, "tizconfigport_class");
  TIZ_LOG_CLASS (tizconfigport_class);
  void * tizconfigport = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (tizconfigport_class, "tizconfigport", tizport, sizeof (tiz_configport_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, configport_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, configport_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetComponentVersion, configport_GetComponentVersion,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetParameter, configport_GetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetParameter, configport_SetParameter,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetConfig, configport_GetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_SetConfig, configport_SetConfig,
     /* TIZ_CLASS_COMMENT: */
     tiz_api_GetExtensionIndex, configport_GetExtensionIndex,
     /* TIZ_CLASS_COMMENT: */
     tiz_configport_clear_metadata, configport_clear_metadata,
     /* TIZ_CLASS_COMMENT: */
     tiz_configport_store_metadata, configport_store_metadata,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return tizconfigport;
}
