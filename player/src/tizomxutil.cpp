/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   tizomxutil.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL Core wrapper functions
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.omxutil"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "tizplatform.h"

#include "tizomxutil.hpp"

void tiz::omxutil::init ()
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;

  if (OMX_ErrorNone != (ret = OMX_Init ()))
  {
    fprintf (stderr, "FATAL. Could not init OpenMAX IL : %s",
             tiz_err_to_str (ret));
    exit (EXIT_FAILURE);
  }
}

void tiz::omxutil::deinit ()
{
  (void)OMX_Deinit ();
}

OMX_ERRORTYPE
tiz::omxutil::list_comps (std::vector< std::string > &components)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_U32 index = 0;
  char comp_name[OMX_MAX_STRINGNAME_SIZE];

  do
  {
    error = OMX_ComponentNameEnum ((OMX_STRING)comp_name,
                                   OMX_MAX_STRINGNAME_SIZE, index++);
    if (OMX_ErrorNone == error)
    {
      components.push_back (std::string (comp_name));
    }
  } while (OMX_ErrorNone == error);

  return error;
}

OMX_ERRORTYPE
tiz::omxutil::roles_of_comp (const std::string &comp,
                             std::vector< std::string > &roles)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_U32 index = 0;
  char role[OMX_MAX_STRINGNAME_SIZE];

  do
  {
    error = OMX_RoleOfComponentEnum (
        (OMX_STRING)role, const_cast< OMX_STRING >(comp.c_str ()), index++);
    if (OMX_ErrorNone == error)
    {
      roles.push_back (std::string (role));
    }
  } while (OMX_ErrorNone == error);

  return error;
}

OMX_ERRORTYPE
tiz::omxutil::comps_of_role (const std::string &role,
                             std::vector< std::string > &components)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_U32 index = 0;
  char comp_name[OMX_MAX_STRINGNAME_SIZE];

  do
  {
    error = OMX_ComponentOfRoleEnum ((OMX_STRING)comp_name,
                                     const_cast< OMX_STRING >(role.c_str ()),
                                     index++);
    if (OMX_ErrorNone == error)
    {
      components.push_back (std::string (comp_name));
    }
  } while (OMX_ErrorNone == error);

  return error;
}
