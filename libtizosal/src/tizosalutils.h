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
 * @file   tizosalutils.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief Tizonia OpenMAX IL - OpenMAX IL types utility functions
 *
 *
 */

#ifndef TIZOSALUTILS_H
#define TIZOSALUTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Types.h"

  /*@observer@ */ const OMX_STRING tiz_cmd_to_str (OMX_COMMANDTYPE a_cmd);

  /*@observer@ */ const OMX_STRING tiz_state_to_str (OMX_STATETYPE a_id);

  /*@observer@ */ const OMX_STRING tiz_evt_to_str (OMX_EVENTTYPE a_evt);

  /*@observer@ */ const OMX_STRING tiz_err_to_str (OMX_ERRORTYPE a_err);

  /*@observer@ */ const OMX_STRING tiz_dir_to_str (OMX_DIRTYPE a_dir);

  /*@observer@ */ const OMX_STRING tiz_idx_to_str (OMX_INDEXTYPE a_idx);

#define TIZ_UTIL_TEST_ERR(_a)                                   \
  do {                                                          \
    OMX_ERRORTYPE _err = _a;                                    \
    if (OMX_ErrorNone != _err) {                                \
      TIZ_LOG(TIZ_ERROR, "[%s]...", tiz_err_to_str (_err)); \
      return _err;                                              \
    }                                                           \
  } while(0)

#define TIZ_UTIL_TEST_ERR_OOM(_a)                                       \
  do {                                                                  \
    OMX_ERRORTYPE _err = _a;                                            \
    if (OMX_ErrorNone != _err) {                                        \
      TIZ_LOG(TIZ_ERROR, "[OMX_ErrorInsufficientResources] "        \
              "was [%s]...", tiz_err_to_str (_err));                    \
      return OMX_ErrorInsufficientResources;                            \
    }                                                                   \
  } while(0)

#define TIZ_UTIL_TEST_ERR_RET_NULL(_a)                                   \
  do {                                                          \
    OMX_ERRORTYPE _err = _a;                                    \
    if (OMX_ErrorNone != _err) {                                \
      TIZ_LOG(TIZ_ERROR, "[%s]...", tiz_err_to_str (_err)); \
      return NULL;                                              \
    }                                                           \
  } while(0)

#ifdef __cplusplus
}
#endif

#endif                          /* TIZOSALUTILS_H */
