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

  typedef enum tizrm_error_t
  {
    TIZRM_SUCCESS = 0,
    TIZRM_MISUSE,
    TIZRM_OOM,
    TIZRM_COMPONENT_NOT_PROVISIONED,
    TIZRM_RESOURCE_NOT_PROVISIONED,
    TIZRM_NOT_ENOUGH_RESOURCE_PROVISIONED,
    TIZRM_NOT_ENOUGH_RESOURCE_AVAILABLE,
    TIZRM_NOT_ENOUGH_RESOURCE_ACQUIRED,
    TIZRM_PREEMPTION_IN_PROGRESS,
    TIZRM_DATABASE_OPEN_ERROR,
    TIZRM_DATABASE_INIT_ERROR,
    TIZRM_DATABASE_ACCESS_ERROR,
    TIZRM_DATABASE_CLOSE_ERROR,
    TIZRM_DATABASE_ERROR,
    TIZRM_WAIT_COMPLETE,
    TIZRM_DBUS,
    TIZRM_UNKNOWN,
    TIZRM_ERROR_MAX
  } tizrm_error_t;

  typedef enum tizrm_resource_id_t
  {
    TIZRM_RESOURCE_DUMMY = 0,
    TIZRM_RESOURCE_ALSA_SINK,
    TIZRM_RESOURCE_FILESYSTEM,
    TIZRM_RESOURCE_MAX
  } tizrm_resource_id_t;

#ifdef __cplusplus
}
#endif

#endif                          // TIZRMTYPES_H
