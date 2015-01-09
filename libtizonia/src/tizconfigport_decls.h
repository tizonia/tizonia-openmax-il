/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizconfigport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - configport class declarations
 *
 *
 */

#ifndef TIZCONFIGPORT_DECLS_H
#define TIZCONFIGPORT_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include "tizport_decls.h"

  typedef struct tiz_configport tiz_configport_t;
  struct tiz_configport
  {
    /* Object */
    const tiz_port_t _;
    char comp_name_[OMX_MAX_STRINGNAME_SIZE];
    OMX_VERSIONTYPE comp_ver_;
    OMX_UUIDTYPE uuid_;
    OMX_RESOURCECONCEALMENTTYPE param_rc_;
    OMX_PARAM_SUSPENSIONPOLICYTYPE param_sp_;
    OMX_PRIORITYMGMTTYPE config_pm_;
    OMX_CONFIG_METADATAITEMCOUNTTYPE metadata_count_;
    tiz_vector_t *p_metadata_lst_;
    OMX_TIZONIA_PLAYLISTSKIPTYPE playlist_skip_;
};

  typedef struct tiz_configport_class tiz_configport_class_t;
  struct tiz_configport_class
  {
    /* Class */
    const tiz_port_class_t _;
    void (*clear_metadata)(void *ap_obj);
    OMX_ERRORTYPE (*store_metadata)(void *ap_obj, const OMX_CONFIG_METADATAITEMTYPE *ap_meta_item);
  };

#ifdef __cplusplus
}
#endif

#endif                          /* TIZCONFIGPORT_DECLS_H */
