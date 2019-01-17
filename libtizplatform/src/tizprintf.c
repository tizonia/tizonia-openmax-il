/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   tizlog.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - tiz_printf implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <alloca.h>

#include "tizprintf.h"

void
tiz_printf (const char * ap_color, const char * ap_file, int a_line,
            const char * ap_func, const char * ap_format, ...)
{
  size_t size = 256;
  char * buffer = alloca (size);
  va_list va;
  va_start (va, ap_format);
  vsnprintf (buffer, size, ap_format, va);
  va_end (va);
  if (ap_file && ap_func)
    {
      const char * p_env = getenv ("TIZONIA_DEBUG_COLOR_TRACE");
      if (p_env && strncmp (p_env, "1", 2) == 0)
        {
          fprintf (stderr, "%s[%s:%s:%d] --- %s%s\n", ap_color, ap_file,
                   ap_func, a_line, buffer, KNRM);
        }
    }
  else
    {
      fprintf (stderr, "%s%s%s", ap_color, buffer, KNRM);
    }
}
