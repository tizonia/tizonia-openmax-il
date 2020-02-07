/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   tizport-macros.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - port macros
 *
 *
 */

#ifndef TIZPORT_MACROS_H
#define TIZPORT_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tizport.h"

#define TIZ_PORT_IS_CONFIG_PORT(_p) \
  (TIZ_PORT_CONFIG_PORT_INDEX == tiz_port_index (_p))

#define TIZ_PORT_IS_ENABLED(_p) tiz_port_check_flags (_p, 1, EFlagEnabled)

#define TIZ_PORT_IS_DISABLED(_p) !tiz_port_check_flags (_p, 1, EFlagEnabled)

#define TIZ_PORT_IS_BEING_ENABLED(_p) \
  tiz_port_check_flags (_p, 1, EFlagBeingEnabled)

#define TIZ_PORT_IS_BEING_DISABLED(_p) \
  tiz_port_check_flags (_p, 1, EFlagBeingDisabled)

#define TIZ_PORT_IS_POPULATED(_p) tiz_port_check_flags (_p, 1, EFlagPopulated)

#define TIZ_PORT_IS_TUNNELED(_p) tiz_port_check_flags (_p, 1, EFlagTunneled)

#define TIZ_PORT_IS_SUPPLIER(_p) \
  tiz_port_check_flags (_p, 1, EFlagBufferSupplier)

#define TIZ_PORT_IS_ALLOCATOR(_p) \
  tiz_port_check_flags (_p, 1, EFlagBufferAllocator)

#define TIZ_PORT_IS_POPULATED_AND_ENABLED(_p) \
  tiz_port_check_flags (_p, 2, EFlagPopulated, EFlagEnabled)

#define TIZ_PORT_IS_TUNNELED_AND_SUPPLIER(_p) \
  tiz_port_check_flags (_p, 2, EFlagTunneled, EFlagBufferSupplier)

#define TIZ_PORT_IS_ENABLED_TUNNELED_AND_SUPPLIER(_p) \
  tiz_port_check_flags (_p, 3, EFlagEnabled, EFlagTunneled, EFlagBufferSupplier)

#define TIZ_PORT_IS_ENABLED_TUNNELED_AND_NON_SUPPLIER(_p)   \
  tiz_port_check_flags (_p, 2, EFlagEnabled, EFlagTunneled) \
    && !tiz_port_check_flags (_p, 1, EFlagBufferSupplier)

#define TIZ_PORT_IS_ENABLED_TUNNELED_SUPPLIER_AND_NOT_POPULATED(_p) \
  tiz_port_check_flags (_p, 3, EFlagEnabled, EFlagTunneled,         \
                        EFlagBufferSupplier)                        \
    && !tiz_port_check_flags (_p, 1, EFlagPopulated)

#define TIZ_PORT_IS_POPULATED_AND_GOING_TO_ENABLED(_p) \
  tiz_port_check_flags (_p, 2, EFlagPopulated, EFlagBeingEnabled)

#define TIZ_PORT_IS_BEING_FLUSHED(_p) \
  tiz_port_check_flags (_p, 1, EFlagFlushInProgress)

#define TIZ_PORT_SET_GOING_TO_ENABLED(_p) \
  tiz_port_set_flags (_p, 1, EFlagBeingEnabled)

#define TIZ_PORT_SET_GOING_TO_DISABLED(_p) \
  tiz_port_set_flags (_p, 1, EFlagBeingDisabled)

#define TIZ_PORT_SET_DISABLED(_p) \
  tiz_port_clear_flags (_p, 2, EFlagEnabled, EFlagBeingDisabled)

#define TIZ_PORT_SET_ENABLED(_p)                       \
  do                                                   \
    {                                                  \
      tiz_port_set_flags (_p, 1, EFlagEnabled);        \
      tiz_port_clear_flags (_p, 1, EFlagBeingEnabled); \
    }                                                  \
  while (0)

#define TIZ_PORT_SET_FLUSH_IN_PROGRESS(_p) \
  tiz_port_set_flags (_p, 1, EFlagFlushInProgress)

#define TIZ_PORT_CLEAR_FLUSH_IN_PROGRESS(_p) \
  tiz_port_clear_flags (_p, 1, EFlagFlushInProgress)

#define TIZ_PORT_MAY_CALL_USE_BUFFER(_p) \
  tiz_port_check_tunneled_port_status (_p, OMX_PORTSTATUS_ACCEPTUSEBUFFER)

#define TIZ_PORT_MAY_EXCHANGE_BUFFERS(_p) \
  tiz_port_check_tunneled_port_status (_p, OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE)

#define TIZ_PORT_MAY_INITIATE_EXE_TO_IDLE(_p)                    \
  ((tiz_port_check_flags (_p, 2, EFlagEnabled, EFlagTunneled)    \
    && !tiz_port_check_flags (_p, 1, EFlagBufferSupplier)        \
    && tiz_port_check_tunneled_port_status (                     \
         _p, OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN))         \
   || (tiz_port_check_flags (_p, 3, EFlagEnabled, EFlagTunneled, \
                             EFlagBufferSupplier)))

#define TIZ_PORT_INC_CLAIMED_COUNT(_p) tiz_port_update_claimed_count (_p, 1)

#define TIZ_PORT_DEC_CLAIMED_COUNT(_p) tiz_port_update_claimed_count (_p, -1)

#define TIZ_PORT_GET_CLAIMED_COUNT(_p) tiz_port_update_claimed_count (_p, 0)

#ifdef __cplusplus
}
#endif

#endif /* TIZPORT_MACROS_H */
