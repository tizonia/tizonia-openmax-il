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

#define COLOR_THEME "color-theme"
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

static char * gp_def_c01 = "31;49;22";
static char * gp_def_c02 = "32;49;22";
static char * gp_def_c03 = "33;49;22";
static char * gp_def_c04 = "34;49;22";
static char * gp_def_c05 = "35;49;22";
static char * gp_def_c06 = "36;49;22";
static char * gp_def_c07 = "37;49;22";
static char * gp_def_c08 = "91;49;22";
static char * gp_def_c09 = "92;49;22";
static char * gp_def_c10 = "93;49;22";
static char * gp_def_c11 = "94;49;22";
static char * gp_def_c12 = "95;49;22";
static char * gp_def_c13 = "36";
static char * gp_def_c14 = "37";
static char * gp_def_c15 = "41";
static char * gp_def_c16 = "46";

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
  char * p = NULL;
  char * p_ansi_color = NULL;
  switch (a_kc)
    {
    case TIZ_COLOR_01:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C01);
        if (!p) p_ansi_color = gp_def_c01;
      }
      break;
    case TIZ_COLOR_02:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C02);
        if (!p) p_ansi_color = gp_def_c02;
      }
      break;
    case TIZ_COLOR_03:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C03);
        if (!p) p_ansi_color = gp_def_c03;
      }
      break;
    case TIZ_COLOR_04:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C04);
        if (!p) p_ansi_color = gp_def_c04;
      }
      break;
    case TIZ_COLOR_05:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C05);
        if (!p) p_ansi_color = gp_def_c05;
      }
      break;
    case TIZ_COLOR_06:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C06);
        if (!p) p_ansi_color = gp_def_c06;
      }
      break;
    case TIZ_COLOR_07:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C07);
        if (!p) p_ansi_color = gp_def_c07;
      }
      break;
    case TIZ_COLOR_08:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C08);
        if (!p) p_ansi_color = gp_def_c08;
      }
      break;
    case TIZ_COLOR_09:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C09);
        if (!p) p_ansi_color = gp_def_c09;
      }
      break;
    case TIZ_COLOR_10:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C10);
        if (!p) p_ansi_color = gp_def_c10;
      }
      break;
    case TIZ_COLOR_11:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C11);
        if (!p) p_ansi_color = gp_def_c11;
      }
      break;
    case TIZ_COLOR_12:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C12);
        if (!p) p_ansi_color = gp_def_c12;
      }
      break;
    case TIZ_COLOR_13:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C13);
        if (!p) p_ansi_color = gp_def_c13;
      }
      break;
    case TIZ_COLOR_14:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C14);
        if (!p) p_ansi_color = gp_def_c14;
      }
      break;
    case TIZ_COLOR_15:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C15);
        if (!p) p_ansi_color = gp_def_c15;
      }
      break;
    case TIZ_COLOR_16:
      {
        p = (char *) tiz_rcfile_get_value (COLOR_THEME, C16);
        if (!p) p_ansi_color = gp_def_c16;
      }
      break;
    default:
      {
        assert (0);
      };
    };

  if (p)
    {
      (void)replacechar(p, ',', ';');
      p_ansi_color = p;
    }

  if (p_ansi_color)
    {
      size_t size = 256;
      char * p_buffer = alloca (size);
      va_list va;
      va_start (va, ap_format);
      vsnprintf (p_buffer, size, ap_format, va);
      va_end (va);
      fprintf (stdout, "\033[%sm%s%s\n", p_ansi_color, p_buffer, KNRM);
    }
}
