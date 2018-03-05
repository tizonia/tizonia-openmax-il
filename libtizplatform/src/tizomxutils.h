/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   tizomxutils.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia Platform - OpenMAX IL types utility functions
 *
 *
 */

#ifndef TIZOMXUTILS_H
#define TIZOMXUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Audio.h>
#include <OMX_Core.h>

#include "tizmem.h"

#define TIZ_INIT_OMX_STRUCT(struct_storage)                      \
  tiz_mem_set (&(struct_storage), 0x0, sizeof (struct_storage)); \
  (struct_storage).nSize = sizeof (struct_storage);              \
  (struct_storage).nVersion.nVersion = OMX_VERSION;

#define TIZ_INIT_OMX_PORT_STRUCT(struct_storage, port_id)        \
  tiz_mem_set (&(struct_storage), 0x0, sizeof (struct_storage)); \
  (struct_storage).nSize = sizeof (struct_storage);              \
  (struct_storage).nVersion.nVersion = OMX_VERSION;              \
  (struct_storage).nPortIndex = port_id;

#define TIZ_OMX_BUF_PTR(hdr) (hdr->pBuffer + hdr->nOffset)

#define TIZ_OMX_BUF_FILL_LEN(hdr) (hdr->nFilledLen)

#define TIZ_OMX_BUF_ALLOC_LEN(hdr) (hdr->nAllocLen - hdr->nOffset)

#define TIZ_OMX_BUF_AVAIL(hdr) (hdr->nAllocLen - hdr->nOffset - hdr->nFilledLen)

/*@observer@ */ OMX_STRING
tiz_cmd_to_str (OMX_COMMANDTYPE a_cmd);
/*@observer@ */ OMX_STRING
tiz_state_to_str (OMX_STATETYPE a_id);
/*@observer@ */ OMX_STRING
tiz_evt_to_str (OMX_EVENTTYPE a_evt);
/*@observer@ */ OMX_STRING
tiz_err_to_str (OMX_ERRORTYPE a_err);
/*@observer@ */ OMX_STRING
tiz_dir_to_str (OMX_DIRTYPE a_dir);
/*@observer@ */ OMX_STRING
tiz_domain_to_str (OMX_PORTDOMAINTYPE a_pd);
/*@observer@ */ OMX_STRING
tiz_idx_to_str (OMX_INDEXTYPE a_idx);
/*@observer@ */ OMX_STRING
tiz_audio_coding_to_str (OMX_AUDIO_CODINGTYPE a_cod);

void
tiz_util_reset_eos_flag (OMX_BUFFERHEADERTYPE * p_hdr);

void
tiz_util_set_eos_flag (OMX_BUFFERHEADERTYPE * p_hdr);

#ifdef __cplusplus
}
#endif

#endif /* TIZOMXUTILS_H */
