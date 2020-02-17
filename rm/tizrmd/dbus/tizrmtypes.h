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

/**
 * @file   tizrmtypes.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Resource Manager Daemon - Types and constants
 *
 *
 */

#ifndef TIZRMTYPES_H
#define TIZRMTYPES_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

  typedef enum tiz_rm_error_t
  {
    TIZ_RM_SUCCESS = 0,
    TIZ_RM_MISUSE,
    TIZ_RM_OOM,
    TIZ_RM_COMPONENT_NOT_PROVISIONED,
    TIZ_RM_RESOURCE_NOT_PROVISIONED,
    TIZ_RM_NOT_ENOUGH_RESOURCE_PROVISIONED,
    TIZ_RM_NOT_ENOUGH_RESOURCE_AVAILABLE,
    TIZ_RM_NOT_ENOUGH_RESOURCE_ACQUIRED,
    TIZ_RM_PREEMPTION_IN_PROGRESS,
    TIZ_RM_DATABASE_OPEN_ERROR,
    TIZ_RM_DATABASE_INIT_ERROR,
    TIZ_RM_DATABASE_ACCESS_ERROR,
    TIZ_RM_DATABASE_CLOSE_ERROR,
    TIZ_RM_DATABASE_ERROR,
    TIZ_RM_WAIT_COMPLETE,
    TIZ_RM_DBUS,
    TIZ_RM_UNKNOWN,
    TIZ_RM_ERROR_MAX
  } tiz_rm_error_t;

  typedef enum tiz_rm_resource_id_t
  {
    TIZ_RM_RESOURCE_DUMMY = 0,
    TIZ_RM_RESOURCE_ALSA_SINK,
    TIZ_RM_RESOURCE_FILESYSTEM,
    TIZ_RM_RESOURCE_MAX
  } tiz_rm_resource_id_t;

#ifdef __cplusplus
}
#endif

#endif                          // TIZRMTYPES_H
