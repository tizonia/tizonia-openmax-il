/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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

#ifdef __cplusplus
}
#endif

#endif /* TIZPRINTF_H */
