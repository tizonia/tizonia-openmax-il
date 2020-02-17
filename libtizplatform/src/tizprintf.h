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
 * @file   tizlog.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - printf macros
 *
 *
 */

#ifndef TIZPRINTF_H
#define TIZPRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

#define TIZ_PRINTF_RED(format, args...) \
  tiz_printf (KRED, NULL, -1, NULL, format, ##args);

#define TIZ_PRINTF_GRN(format, args...) \
  tiz_printf (KGRN, NULL, -1, NULL, format, ##args);

#define TIZ_PRINTF_YEL(format, args...) \
  tiz_printf (KYEL, NULL, -1, NULL, format, ##args);

#define TIZ_PRINTF_BLU(format, args...) \
  tiz_printf (KBLU, NULL, -1, NULL, format, ##args);

#define TIZ_PRINTF_MAG(format, args...) \
  tiz_printf (KMAG, NULL, -1, NULL, format, ##args);

#define TIZ_PRINTF_CYN(format, args...) \
  tiz_printf (KCYN, NULL, -1, NULL, format, ##args);

#define TIZ_PRINTF_WHT(format, args...) \
  tiz_printf (KWHT, NULL, -1, NULL, format, ##args);


#define TIZ_COLOR_01 1
#define TIZ_COLOR_02 2
#define TIZ_COLOR_03 3
#define TIZ_COLOR_04 4
#define TIZ_COLOR_05 5
#define TIZ_COLOR_06 6
#define TIZ_COLOR_07 7
#define TIZ_COLOR_08 8
#define TIZ_COLOR_09 9
#define TIZ_COLOR_10 10
#define TIZ_COLOR_11 11
#define TIZ_COLOR_12 12
#define TIZ_COLOR_13 13
#define TIZ_COLOR_14 14
#define TIZ_COLOR_15 15
#define TIZ_COLOR_16 16

#define TIZ_PRINTF_C01(format, args...) \
  tiz_printf_c (TIZ_COLOR_01, format, ##args);

#define TIZ_PRINTF_C02(format, args...) \
  tiz_printf_c (TIZ_COLOR_02, format, ##args);

#define TIZ_PRINTF_C03(format, args...) \
  tiz_printf_c (TIZ_COLOR_03, format, ##args);

#define TIZ_PRINTF_C04(format, args...) \
  tiz_printf_c (TIZ_COLOR_04, format, ##args);

#define TIZ_PRINTF_C05(format, args...) \
  tiz_printf_c (TIZ_COLOR_05, format, ##args);

#define TIZ_PRINTF_C06(format, args...) \
  tiz_printf_c (TIZ_COLOR_06, format, ##args);

#define TIZ_PRINTF_C07(format, args...) \
  tiz_printf_c (TIZ_COLOR_07, format, ##args);

#define TIZ_PRINTF_C08(format, args...) \
  tiz_printf_c (TIZ_COLOR_08, format, ##args);

#define TIZ_PRINTF_C09(format, args...) \
  tiz_printf_c (TIZ_COLOR_09, format, ##args);

#define TIZ_PRINTF_C10(format, args...) \
  tiz_printf_c (TIZ_COLOR_10, format, ##args);

#define TIZ_PRINTF_C11(format, args...) \
  tiz_printf_c (TIZ_COLOR_11, format, ##args);

#define TIZ_PRINTF_C12(format, args...)                 \
  tiz_printf_c (TIZ_COLOR_12, format, ##args);

#define TIZ_PRINTF_C13(format, args...) \
  tiz_printf_c (TIZ_COLOR_13, format, ##args);

#define TIZ_PRINTF_C14(format, args...) \
  tiz_printf_c (TIZ_COLOR_14, format, ##args);

#define TIZ_PRINTF_C15(format, args...) \
  tiz_printf_c (TIZ_COLOR_15, format, ##args);

#define TIZ_PRINTF_C16(format, args...)                 \
  tiz_printf_c (TIZ_COLOR_16, format, ##args);


#ifdef NDEBUG

#define TIZ_PRINTF_DBG_RED(format, args...)
#define TIZ_PRINTF_DBG_GRN(format, args...)
#define TIZ_PRINTF_DBG_YEL(format, args...)
#define TIZ_PRINTF_DBG_BLU(format, args...)
#define TIZ_PRINTF_DBG_MAG(format, args...)
#define TIZ_PRINTF_DBG_CYN(format, args...)
#define TIZ_PRINTF_DBG_WHT(format, args...)

#else

#define TIZ_PRINTF_DBG_RED(format, args...) \
  tiz_printf (KRED, __FILE__, __LINE__, __FUNCTION__, format, ##args);

#define TIZ_PRINTF_DBG_GRN(format, args...) \
  tiz_printf (KGRN, __FILE__, __LINE__, __FUNCTION__, format, ##args);

#define TIZ_PRINTF_DBG_YEL(format, args...) \
  tiz_printf (KYEL, __FILE__, __LINE__, __FUNCTION__, format, ##args);

#define TIZ_PRINTF_DBG_BLU(format, args...) \
  tiz_printf (KBLU, __FILE__, __LINE__, __FUNCTION__, format, ##args);

#define TIZ_PRINTF_DBG_MAG(format, args...) \
  tiz_printf (KMAG, __FILE__, __LINE__, __FUNCTION__, format, ##args);

#define TIZ_PRINTF_DBG_CYN(format, args...) \
  tiz_printf (KCYN, __FILE__, __LINE__, __FUNCTION__, format, ##args);

#define TIZ_PRINTF_DBG_WHT(format, args...) \
  tiz_printf (KWHT, __FILE__, __LINE__, __FUNCTION__, format, ##args);

#endif

  void
  tiz_printf (const char * ap_color, const char * ap_file, int a_line,
              const char * ap_func, const char * ap_format, ...);

  void
  tiz_printf_c (int a_kc, const char * ap_format, ...);

#ifdef __cplusplus
}
#endif

#endif /* TIZPRINTF_H */
