/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   tizcasttypes.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Chromecast client daemon - Types and constants
 *
 *
 */

#ifndef TIZCASTTYPES_H
#define TIZCASTTYPES_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

  typedef enum tiz_cast_error_t
  {
    TIZ_CAST_SUCCESS = 0,
    TIZ_CAST_MISUSE,
    TIZ_CAST_URL_LOAD_FAILURE,
    TIZ_CAST_CC_CMD_FAILURE,
    TIZ_CAST_OOM,
    TIZ_CAST_DBUS,
    TIZ_CAST_UNKNOWN,
    TIZ_CAST_ERROR_MAX
  } tiz_cast_error_t;

#ifdef __cplusplus
}
#endif

#endif                          // TIZCASTTYPES_H
