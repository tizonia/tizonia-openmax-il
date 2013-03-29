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
 * @file   tizport.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - port class
 *
 *
 */

#ifndef TIZPORT_H
#define TIZPORT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <limits.h>
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Types.h"

#include "tizapi.h"
#include "tizscheduler.h"
#include "tizosal.h"

#define TIZ_PORT_CONFIG_PORT_INDEX ULONG_MAX

  typedef enum tiz_port_flag_ids tiz_port_flag_ids_t;

  enum tiz_port_flag_ids
    {
      EFlagEnabled = 0,
      EFlagBeingEnabled,
      EFlagBeingDisabled,
      EFlagPopulated,
      EFlagTunneled,
      EFlagBufferSupplier,
      EFlagBufferAllocator,
      EFlagFlushInProgress,
      EFlagMax
    };

  typedef struct tiz_port_options tiz_port_options_t;

  struct tiz_port_options
  {
    /* Port domain (audio, video, image, other...) */
    OMX_PORTDOMAINTYPE domain;
    /* Input or output port */
    OMX_DIRTYPE dir;
    /* Minimum number of buffers required by this port */
    OMX_U32 min_buf_count;
    /* Minimum size, in bytes, of buffers used for this port */
    OMX_U32 min_buf_size;
    /* Buffers contiguous requirement (true or false) */
    OMX_BOOL contiguous;
    /* Buffer aligment requirement */
    OMX_U32 alignment;
    /* Port supplier preference used during tunneling */
    OMX_BUFFERSUPPLIERTYPE buf_supplier;
    /* Memory allocation and deallocation hooks */
    tiz_port_alloc_hooks_t mem_hooks;
    /* Port index of the port that acts as a master or slave of this port. Use -1
     * for none. */
    OMX_U32 mos_port;
  };


  /* factory_new(tizport, ...); */
  extern const void *tizport;
  extern const void *tiz_port_class;

  OMX_BOOL tiz_port_check_flags (const void *ap_obj, OMX_U32 a_nflags, ...);

#define TIZ_PORT_IS_CONFIG_PORT(_p)                      \
  (TIZ_PORT_CONFIG_PORT_INDEX == tiz_port_index(_p))

#define TIZ_PORT_IS_ENABLED(_p) tiz_port_check_flags(_p,1,EFlagEnabled)

#define TIZ_PORT_IS_DISABLED(_p) !tiz_port_check_flags(_p,1,EFlagEnabled)

#define TIZ_PORT_IS_BEING_ENABLED(_p)            \
  tiz_port_check_flags(_p,1,EFlagBeingEnabled)

#define TIZ_PORT_IS_BEING_DISABLED(_p)           \
  tiz_port_check_flags(_p,1,EFlagBeingDisabled)

#define TIZ_PORT_IS_POPULATED(_p)                \
  tiz_port_check_flags(_p,1,EFlagPopulated)

#define TIZ_PORT_IS_TUNNELED(_p)                 \
  tiz_port_check_flags(_p,1,EFlagTunneled)

#define TIZ_PORT_IS_SUPPLIER(_p)                 \
  tiz_port_check_flags(_p,1,EFlagBufferSupplier)

#define TIZ_PORT_IS_ALLOCATOR(_p)                        \
  tiz_port_check_flags(_p,1,EFlagBufferAllocator)

#define TIZ_PORT_IS_POPULATED_AND_ENABLED(_p)            \
  tiz_port_check_flags(_p,2,EFlagPopulated,EFlagEnabled)

#define TIZ_PORT_IS_TUNNELED_AND_SUPPLIER(_p)                    \
  tiz_port_check_flags(_p,2,EFlagTunneled,EFlagBufferSupplier)

#define TIZ_PORT_IS_ENABLED_TUNNELED_AND_SUPPLIER(_p)                    \
  tiz_port_check_flags(_p,3,EFlagEnabled,EFlagTunneled,EFlagBufferSupplier)

#define TIZ_PORT_IS_ENABLED_TUNNELED_AND_NON_SUPPLIER(_p)                \
  tiz_port_check_flags(_p,2,EFlagEnabled,EFlagTunneled)                  \
  &&                                                                    \
  !tiz_port_check_flags(_p,1,EFlagBufferSupplier)

#define TIZ_PORT_IS_ENABLED_TUNNELED_SUPPLIER_AND_NOT_POPULATED(_p)      \
  tiz_port_check_flags(_p,3,EFlagEnabled,EFlagTunneled,EFlagBufferSupplier) \
  &&                                                                    \
  !tiz_port_check_flags(_p,1,EFlagPopulated)

#define TIZ_PORT_IS_POPULATED_AND_GOING_TO_ENABLED(_p)           \
  tiz_port_check_flags(_p,2,EFlagPopulated,EFlagBeingEnabled)

#define TIZ_PORT_IS_BEING_FLUSHED(_p)                    \
  tiz_port_check_flags(_p,1,EFlagFlushInProgress)

  void tiz_port_set_flags (const void *ap_obj, OMX_U32 a_nflags, ...);

#define TIZ_PORT_SET_GOING_TO_ENABLED(_p)        \
  tiz_port_set_flags(_p,1,EFlagBeingEnabled)

#define TIZ_PORT_SET_GOING_TO_DISABLED(_p)       \
  tiz_port_set_flags(_p,1,EFlagBeingDisabled)

  void tiz_port_clear_flags (const void *ap_obj, OMX_U32 a_nflags, ...);

#define TIZ_PORT_SET_DISABLED(_p)                                \
  tiz_port_clear_flags(_p,2,EFlagEnabled,EFlagBeingDisabled)

#define TIZ_PORT_SET_ENABLED(_p)                         \
  do {                                                  \
    tiz_port_set_flags(_p,1,EFlagEnabled);               \
    tiz_port_clear_flags(_p,1,EFlagBeingEnabled);        \
  } while(0)

#define TIZ_PORT_SET_FLUSH_IN_PROGRESS(_p)       \
  tiz_port_set_flags(_p,1,EFlagFlushInProgress)

#define TIZ_PORT_CLEAR_FLUSH_IN_PROGRESS(_p)             \
  tiz_port_clear_flags(_p,1,EFlagFlushInProgress)


  OMX_BOOL tiz_port_check_tunneled_port_status (const void *ap_obj,
                                               OMX_U32 a_port_status);

#define TIZ_PORT_MAY_CALL_USE_BUFFER(_p)                                 \
  tiz_port_check_tunneled_port_status(_p,OMX_PORTSTATUS_ACCEPTUSEBUFFER)

#define TIZ_PORT_MAY_EXCHANGE_BUFFERS(_p)                                \
  tiz_port_check_tunneled_port_status(_p,OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE)

#define TIZ_PORT_MAY_INITIATE_EXE_TO_IDLE(_p)                            \
  ( (tiz_port_check_flags(_p,2,EFlagEnabled,EFlagTunneled)               \
     &&                                                                 \
     !tiz_port_check_flags(_p,1,EFlagBufferSupplier)                     \
     &&                                                                 \
     tiz_port_check_tunneled_port_status(_p,OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN)) \
    ||                                                                  \
    (tiz_port_check_flags(_p,3,EFlagEnabled,EFlagTunneled,EFlagBufferSupplier)) )


  OMX_ERRORTYPE
  tiz_port_register_index (const void *ap_obj, OMX_INDEXTYPE a_index);

  OMX_ERRORTYPE tiz_port_find_index (const void *ap_obj,
                                    OMX_INDEXTYPE a_index);

  OMX_U32 tiz_port_index (const void *ap_obj);

  void tiz_port_set_index (void *ap_obj, OMX_U32 a_pid);

  OMX_ERRORTYPE
  tiz_port_set_portdef_format (void *ap_obj,
                              const OMX_PARAM_PORTDEFINITIONTYPE * ap_pdef);

  OMX_S32 tiz_port_buffer_count (const void *ap_obj);

  OMX_DIRTYPE tiz_port_dir (const void *ap_obj);

  OMX_PORTDOMAINTYPE tiz_port_domain (const void *ap_obj);

  OMX_HANDLETYPE tiz_port_get_tunnel_comp (const void *ap_obj);

  /* OMX_BUFFERHEADERTYPE* */
  /* tiz_port_get_hdr(const void *ap_obj, OMX_S32 a_pos); */

  tiz_vector_t *tiz_port_get_hdrs_list (void *ap_obj);

  OMX_ERRORTYPE tiz_port_populate (const void *ap_obj, OMX_HANDLETYPE ap_hdl);

  OMX_ERRORTYPE tiz_port_depopulate (const void *ap_obj);

  OMX_BOOL
  tiz_port_check_tunnel_compat (const void *ap_obj,
                               OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                               OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def);

  OMX_S32 tiz_port_update_claimed_count (void *ap_obj, OMX_S32 a_offset);

#define TIZ_PORT_INC_CLAIMED_COUNT(_p)           \
  tiz_port_update_claimed_count(_p,1)

#define TIZ_PORT_DEC_CLAIMED_COUNT(_p)           \
  tiz_port_update_claimed_count(_p,-1)

#define TIZ_PORT_GET_CLAIMED_COUNT(_p)           \
  tiz_port_update_claimed_count(_p,0)

  OMX_ERRORTYPE tiz_port_store_mark (void *ap_obj,
                                    const OMX_MARKTYPE * ap_mark_info,
                                    OMX_BOOL a_owned);

  OMX_ERRORTYPE tiz_port_mark_buffer (void *ap_obj,
                                     OMX_BUFFERHEADERTYPE * ap_hdr);

  void
  tiz_port_set_alloc_hooks (void *ap_obj,
                           const tiz_port_alloc_hooks_t * ap_new_hooks,
                           tiz_port_alloc_hooks_t * ap_old_hooks);

  OMX_ERRORTYPE
  tiz_port_populate_header (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                           OMX_BUFFERHEADERTYPE * ap_hdr);

  void
  tiz_port_depopulate_header (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                             OMX_BUFFERHEADERTYPE * ap_hdr);

  OMX_BOOL
  tiz_port_is_master_or_slave (const void *ap_obj, OMX_U32 * ap_mos_pid);

  OMX_ERRORTYPE
  tiz_port_apply_slaving_behaviour (void *ap_obj, void *ap_mos_port,
                                   const OMX_INDEXTYPE a_index,
                                   const OMX_PTR ap_struct,
                                   tiz_vector_t * ap_changed_idxs);

  void tiz_port_update_tunneled_status (void *ap_obj, OMX_HANDLETYPE ap_hdl,
                                       OMX_U32 a_port_status);

  void tiz_port_reset_tunneled_port_status_flag (void *ap_obj,
                                                OMX_HANDLETYPE ap_hdl,
                                                OMX_U32 a_port_status_flag);

  void init_tizport (void);

#ifdef __cplusplus
}
#endif

#endif                          /* TIZPORT_H */
