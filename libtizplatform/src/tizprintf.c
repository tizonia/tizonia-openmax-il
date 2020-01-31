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
#include <errno.h>
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
#include "tizrc.h"

#define COLOR_THEMES "color-themes"
#define ACTIVE_THEME "active-theme"
#define C01 "C01"
#define C02 "C02"
#define C03 "C03"
#define C04 "C04"
#define C05 "C05"
#define C06 "C06"
#define C07 "C07"
#define C08 "C08"
#define C09 "C09"
#define C10 "C10"
#define C11 "C11"
#define C12 "C12"
#define C13 "C13"
#define C14 "C14"
#define C15 "C15"
#define C16 "C16"

static char * gp_def_C01 = "31;49;22";
static char * gp_def_C02 = "32;49;22";
static char * gp_def_C03 = "33;49;22";
static char * gp_def_C04 = "34;49;22";
static char * gp_def_C05 = "35;49;22";
static char * gp_def_C06 = "36;49;22";
static char * gp_def_C07 = "37;49;22";
static char * gp_def_C08 = "91;49;22";
static char * gp_def_C09 = "92;49;22";
static char * gp_def_C10 = "93;49;22";
static char * gp_def_C11 = "94;49;22";
static char * gp_def_C12 = "95;49;22";
static char * gp_def_C13 = "36";
static char * gp_def_C14 = "37";
static char * gp_def_C15 = "41";
static char * gp_def_C16 = "46";

void
tiz_printf (const char * ap_color, const char * ap_file, int a_line,
            const char * ap_func, const char * ap_format, ...)
{
  size_t size = 256;
  char * p_buffer = alloca (size);
  va_list va;
  va_start (va, ap_format);
  vsnprintf (p_buffer, size, ap_format, va);
  va_end (va);
  if (ap_file && ap_func)
    {
      const char * p_env = getenv ("TIZONIA_DEBUG_COLOR_TRACE");
      if (p_env && strncmp (p_env, "1", 2) == 0)
        {
          fprintf (stderr, "%s[%s:%s:%d] --- %s%s\n", ap_color, ap_file,
                   ap_func, a_line, p_buffer, KNRM);
        }
    }
  else
    {
      fprintf (stdout, "%s%s%s", ap_color, p_buffer, KNRM);
    }
}

static int
replacechar (char * ap_str, char a_orig, char a_rep)
{
  char * ix = ap_str;
  int n = 0;
  while ((ix = strchr (ix, a_orig)) != NULL)
    {
      *ix++ = a_rep;
      n++;
    }
  return n;
}

void
tiz_printf_c (int a_kc, const char * ap_format, ...)
{

#define CASE_TIZ_COLOR_(COLOR_ENUM, COLOR, DEFAULT)                         \
  case COLOR_ENUM:                                                          \
    {                                                                       \
      if (p_active_theme)                                                   \
        {                                                                   \
          (void) strncat (color_name, COLOR, OMX_MAX_STRINGNAME_SIZE - 2);  \
          p = (char *) tiz_rcfile_get_value (COLOR_THEMES, color_name);     \
        }                                                                   \
      if (!p)                                                               \
        {                                                                   \
          p_ansi_color = DEFAULT;                                           \
        }                                                                   \
    }                                                                       \
    break

  char color_name[OMX_MAX_STRINGNAME_SIZE];
  char * p = NULL;
  char * p_ansi_color = NULL;
  const char * p_active_theme
    = tiz_rcfile_get_value (COLOR_THEMES, ACTIVE_THEME);
  if (p_active_theme)
    {
      (void) strcpy (color_name, p_active_theme);
      (void) strcat (color_name, ".");
    }

  switch (a_kc)
    {
      CASE_TIZ_COLOR_ (TIZ_COLOR_01, C01, gp_def_C01);
      CASE_TIZ_COLOR_ (TIZ_COLOR_02, C02, gp_def_C02);
      CASE_TIZ_COLOR_ (TIZ_COLOR_03, C03, gp_def_C03);
      CASE_TIZ_COLOR_ (TIZ_COLOR_04, C04, gp_def_C04);
      CASE_TIZ_COLOR_ (TIZ_COLOR_05, C05, gp_def_C05);
      CASE_TIZ_COLOR_ (TIZ_COLOR_06, C06, gp_def_C06);
      CASE_TIZ_COLOR_ (TIZ_COLOR_07, C07, gp_def_C07);
      CASE_TIZ_COLOR_ (TIZ_COLOR_08, C08, gp_def_C08);
      CASE_TIZ_COLOR_ (TIZ_COLOR_09, C09, gp_def_C09);
      CASE_TIZ_COLOR_ (TIZ_COLOR_10, C10, gp_def_C10);
      CASE_TIZ_COLOR_ (TIZ_COLOR_11, C11, gp_def_C11);
      CASE_TIZ_COLOR_ (TIZ_COLOR_12, C12, gp_def_C12);
      CASE_TIZ_COLOR_ (TIZ_COLOR_13, C13, gp_def_C13);
      CASE_TIZ_COLOR_ (TIZ_COLOR_14, C14, gp_def_C14);
      CASE_TIZ_COLOR_ (TIZ_COLOR_15, C15, gp_def_C15);
      CASE_TIZ_COLOR_ (TIZ_COLOR_16, C16, gp_def_C16);
      default:
        {
          assert (0);
        };
    };

  if (p)
    {
      (void) replacechar (p, ',', ';');
      p_ansi_color = p;
    }

  if (p_ansi_color)
    {
      size_t size = OMX_MAX_STRINGNAME_SIZE;
      char * p_buffer = alloca (size);
      va_list va;
      va_start (va, ap_format);
      vsnprintf (p_buffer, size, ap_format, va);
      va_end (va);
      fprintf (stdout, "\033[%sm%s%s\n", p_ansi_color, p_buffer, KNRM);
    }
}
