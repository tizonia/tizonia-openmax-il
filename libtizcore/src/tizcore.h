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
 * @file   tizcore.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Aratelia OpenMAX IL - IL Core declarations
 *
 *
 */

#ifndef TIZCORE_H
#define TIZCORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Core.h"
#include "OMX_Types.h"

typedef struct role_list_item role_list_item_t;

typedef role_list_item_t *role_list_t;

struct role_list_item
{
  OMX_U8 role[OMX_MAX_STRINGNAME_SIZE];
  role_list_item_t *p_next;
};

typedef enum tizcore_state tizcore_state_t;

enum tizcore_state
{

  ETIZCoreStateStopped = 0,
  ETIZCoreStateStarting,
  ETIZCoreStateStarted
};

typedef enum tizcore_msg_class tizcore_msg_class_t;

enum tizcore_msg_class
{
  ETIZCoreMsgInit = 0,
  ETIZCoreMsgDeinit,
  ETIZCoreMsgComponentNameEnum,
  ETIZCoreMsgGetHandle,
  ETIZCoreMsgFreeHandle,
  ETIZCoreMsgSetupTunnel,
  ETIZCoreMsgTeardownTunnel,
  ETIZCoreMsgComponentOfRoleEnum,
  ETIZCoreMsgRoleOfComponentEnum,
  ETIZCoreMsgGetCoreInterface,
  ETIZCoreMsgFreeCoreInterface
};

typedef struct tizcore_msg tizcore_msg_t;

typedef void *tizcore_msg_data_t;

struct tizcore_msg
{
  tizcore_msg_class_t class;
  tizcore_msg_data_t p_data;
};

typedef struct tizcore_msg_gethandle tizcore_msg_gethandle_t;

struct tizcore_msg_gethandle
{
  OMX_HANDLETYPE *pp_hdl;
  OMX_STRING p_comp_name;
  OMX_PTR p_app_data;
  OMX_CALLBACKTYPE *p_callbacks;
};

typedef struct tizcore_msg_freehandle tizcore_msg_freehandle_t;

struct tizcore_msg_freehandle
{
  OMX_HANDLETYPE p_hdl;
};

typedef struct tizcore_msg_compnameenum tizcore_msg_compnameenum_t;

struct tizcore_msg_compnameenum
{
  OMX_STRING p_comp_name;
  OMX_U32 namelen;
  OMX_U32 index;
};

typedef struct tizcore_msg_compofroleenum tizcore_msg_compofroleenum_t;

struct tizcore_msg_compofroleenum
{
  OMX_STRING p_comp_name;
  OMX_STRING p_role;
  OMX_U32 index;
};

/* Use here the same structure being used for comp of role enum API */
typedef struct tizcore_msg_compofroleenum tizcore_msg_roleofcompenum_t;

#ifdef __cplusplus
}
#endif

#endif /* TIZCORE_H */
