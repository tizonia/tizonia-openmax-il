/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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

#ifndef TIZSTATE_DECLS_H
#define TIZSTATE_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tizapi_decls.h"

/*
   * state
   */
typedef struct tiz_state tiz_state_t;
struct tiz_state
{
  const tiz_api_t _;
  void * p_fsm_;
  OMX_U32 servants_count_;
};

OMX_ERRORTYPE
tiz_state_super_state_set (const void * a_class, const void * p_obj,
                           OMX_HANDLETYPE ap_hdl, OMX_COMMANDTYPE a_cmd,
                           OMX_U32 a_param1, OMX_PTR ap_cmd_data);

OMX_ERRORTYPE
tiz_state_super_trans_complete (const void * a_class, const void * ap_obj,
                                OMX_PTR ap_servant, OMX_STATETYPE a_new_state);

typedef struct tiz_state_class tiz_state_class_t;
struct tiz_state_class
{
  /* Class */
  const tiz_api_class_t _;
  OMX_ERRORTYPE (*state_set)
  (const void * p_obj, OMX_HANDLETYPE ap_hdl, OMX_COMMANDTYPE a_cmd,
   OMX_U32 a_param1, OMX_PTR ap_cmd_data);
  OMX_ERRORTYPE (*flush)
  (const void * p_obj, OMX_HANDLETYPE ap_hdl, OMX_COMMANDTYPE a_cmd,
   OMX_U32 a_param1, OMX_PTR ap_cmd_data);
  OMX_ERRORTYPE (*disable)
  (const void * p_obj, OMX_HANDLETYPE ap_hdl, OMX_COMMANDTYPE a_cmd,
   OMX_U32 a_param1, OMX_PTR ap_cmd_data);
  OMX_ERRORTYPE (*enable)
  (const void * p_obj, OMX_HANDLETYPE ap_hdl, OMX_COMMANDTYPE a_cmd,
   OMX_U32 a_param1, OMX_PTR ap_cmd_data);
  OMX_ERRORTYPE (*mark)
  (const void * p_obj, OMX_HANDLETYPE ap_hdl, OMX_COMMANDTYPE a_cmd,
   OMX_U32 a_param1, OMX_PTR ap_cmd_data);
  OMX_ERRORTYPE (*trans_complete)
  (const void * p_obj, OMX_PTR ap_servant, OMX_STATETYPE a_new_state);
  OMX_ERRORTYPE (*tunneled_ports_status_update) (void * ap_obj);
};

/*
   * loaded
   */
typedef struct tiz_loaded tiz_loaded_t;
struct tiz_loaded
{
  const tiz_state_t _;
};

typedef struct tiz_loaded_class tiz_loaded_class_t;
struct tiz_loaded_class
{
  const tiz_state_class_t _;
};

/*
   * loadedtoidle
   */
typedef struct tiz_loadedtoidle tiz_loadedtoidle_t;
struct tiz_loadedtoidle
{
  const tiz_loaded_t _;
};

typedef struct tiz_loadedtoidle_class tiz_loadedtoidle_class_t;
struct tiz_loadedtoidle_class
{
  const tiz_loaded_class_t _;
};

/*
   * waitforresources
   */
typedef struct tiz_waitforresources tiz_waitforresources_t;
struct tiz_waitforresources
{
  const tiz_state_t _;
};

typedef struct tiz_waitforresources_class tiz_waitforresources_class_t;
struct tiz_waitforresources_class
{
  const tiz_state_class_t _;
};

/*
   * idle
   */
typedef struct tiz_idle tiz_idle_t;
struct tiz_idle
{
  const tiz_state_t _;
};

typedef struct tiz_idle_class tiz_idle_class_t;
struct tiz_idle_class
{
  const tiz_state_class_t _;
};

/*
   * idletoloaded
   */
typedef struct tiz_idletoloaded tiz_idletoloaded_t;
struct tiz_idletoloaded
{
  const tiz_idle_t _;
};

typedef struct tiz_idletoloaded_class tiz_idletoloaded_class_t;
struct tiz_idletoloaded_class
{
  const tiz_idle_class_t _;
};

/*
   * idletoexecuting
   */
typedef struct tiz_idletoexecuting tiz_idletoexecuting_t;
struct tiz_idletoexecuting
{
  const tiz_idle_t _;
};

typedef struct tiz_idletoexecuting_class tiz_idletoexecuting_class_t;
struct tiz_idletoexecuting_class
{
  const tiz_idle_class_t _;
};

/*
   * idletoexecuting
   */
typedef struct tiz_executing tiz_executing_t;
struct tiz_executing
{
  const tiz_state_t _;
};

typedef struct tiz_executing_class tiz_executing_class_t;
struct tiz_executing_class
{
  const tiz_state_class_t _;
};

/*
   * idletoexecuting
   */
typedef struct tiz_executingtoidle tiz_executingtoidle_t;
struct tiz_executingtoidle
{
  const tiz_executing_t _;
};

typedef struct tiz_executingtoidle_class tiz_executingtoidle_class_t;
struct tiz_executingtoidle_class
{
  const tiz_executing_class_t _;
};

/*
   * pause
   */
typedef struct tiz_pause tiz_pause_t;
struct tiz_pause
{
  const tiz_state_t _;
};

typedef struct tiz_pause_class tiz_pause_class_t;
struct tiz_pause_class
{
  const tiz_state_class_t _;
};

/*
   * pausetoidle
   */
typedef struct tiz_pausetoidle tiz_pausetoidle_t;
struct tiz_pausetoidle
{
  const tiz_pause_t _;
};

typedef struct tiz_pausetoidle_class tiz_pausetoidle_class_t;
struct tiz_pausetoidle_class
{
  const tiz_pause_class_t _;
};

#ifdef __cplusplus
}
#endif

#endif /* TIZSTATE_DECLS_H */
