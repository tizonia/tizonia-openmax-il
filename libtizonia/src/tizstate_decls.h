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

#ifndef TIZSTATE_DECLS_H
#define TIZSTATE_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tizapi_decls.h"

typedef struct tizstate tizstate_t;

struct tizstate
{
  const struct tizapi _;
  void *ip_fsm;
  OMX_U32 i_servants_count;
};

OMX_ERRORTYPE
tizstate_super_trans_complete (const void *a_class, const void *ap_obj,
                               OMX_PTR ap_servant, OMX_STATETYPE a_new_state);

struct tizstate_class
{
  /* Class */
  const struct tizapi_class _;
    OMX_ERRORTYPE (*state_set) (const void *p_obj,
                                OMX_HANDLETYPE ap_hdl,
                                OMX_COMMANDTYPE a_cmd,
                                OMX_U32 a_param1, OMX_PTR ap_cmd_data);
    OMX_ERRORTYPE (*flush) (const void *p_obj,
                            OMX_HANDLETYPE ap_hdl,
                            OMX_COMMANDTYPE a_cmd,
                            OMX_U32 a_param1, OMX_PTR ap_cmd_data);
    OMX_ERRORTYPE (*disable) (const void *p_obj,
                              OMX_HANDLETYPE ap_hdl,
                              OMX_COMMANDTYPE a_cmd,
                              OMX_U32 a_param1, OMX_PTR ap_cmd_data);
    OMX_ERRORTYPE (*enable) (const void *p_obj,
                             OMX_HANDLETYPE ap_hdl,
                             OMX_COMMANDTYPE a_cmd,
                             OMX_U32 a_param1, OMX_PTR ap_cmd_data);
    OMX_ERRORTYPE (*mark) (const void *p_obj,
                           OMX_HANDLETYPE ap_hdl,
                           OMX_COMMANDTYPE a_cmd,
                           OMX_U32 a_param1, OMX_PTR ap_cmd_data);
    OMX_ERRORTYPE (*trans_complete) (const void *p_obj,
                                     OMX_PTR ap_servant,
                                     OMX_STATETYPE a_new_state);
};

typedef struct tizloaded tizloaded_t;

struct tizloaded
{
  const tizstate_t _;

};

typedef struct tizloadedtoidle tizloadedtoidle_t;

struct tizloadedtoidle
{
  const tizloaded_t _;
};

typedef struct tizwaitforresources tizwaitforresources_t;

struct tizwaitforresources
{
  const tizstate_t _;
};

typedef struct tizidle tizidle_t;

struct tizidle
{
  const tizstate_t _;
};

typedef struct tizidletoloaded tizidletoloaded_t;

struct tizidletoloaded
{
  const tizidle_t _;
};

typedef struct tizexecuting tizexecuting_t;

struct tizexecuting
{
  const tizstate_t _;
};

typedef struct tizexecutingtoidle tizexecutingtoidle_t;

struct tizexecutingtoidle
{
  const tizexecuting_t _;
};

typedef struct tizpause tizpause_t;

struct tizpause
{
  const tizstate_t _;
};

typedef struct tizpausetoidle tizpausetoidle_t;

struct tizpausetoidle
{
  const tizpause_t _;
};

#ifdef __cplusplus
}
#endif

#endif /* TIZSTATE_DECLS_H */
