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
 * @file   tizlimits.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - OS-related limits utils
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <errno.h>

long
tiz_pathname_max (const char * file)
{
  bool call_fail = false;
  long path_max = 0;
  long name_max = 0;
  long pathname_max = -1;

  assert (file);

  errno = 0;
  path_max = pathconf (file, _PC_PATH_MAX);
  name_max = pathconf (file, _PC_NAME_MAX);

  if (-1 == path_max)
    {
      if (errno == 0)
        {
          /* Call succeeded, the limit is indeterminate */
          path_max = PATH_MAX;
        }
      else
        {
          /* Call failed */
          call_fail = true;
        }
    }

  if (-1 == name_max)
    {
      if (errno == 0)
        {
          /* Call succeeded, the limit is indeterminate */
          name_max = NAME_MAX;
        }
      else
        {
          /* Call failed */
          call_fail = true;
        }
    }

  if (!call_fail)
    {
      pathname_max = path_max + name_max;
    }

  assert (call_fail ? -1 == pathname_max : pathname_max > 0);
  return pathname_max;
}
