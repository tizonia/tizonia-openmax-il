/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
 * @file   tizosallog.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL OSAL - Logging
 *
 *
 */

#ifndef TIZOSALLOG_H
#define TIZOSALLOG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "log4c.h"

#ifndef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "root"
#endif

  /* #define WITHOUT_LOG4C 1 */

#define TIZ_LOG(priority,format,args...)      \
  tiz_log(__FILE__,                           \
            __LINE__,                           \
            __FUNCTION__,                       \
            TIZ_LOG_CATEGORY_NAME,            \
            priority,                           \
            NULL,                                              \
            NULL,                                               \
            format,                             \
            ##args);

#define TIZ_LOG_CNAME(priority,cname,cbuf,format,args...)     \
  tiz_log(__FILE__,                                           \
            __LINE__,                                           \
            __FUNCTION__,                                       \
            TIZ_LOG_CATEGORY_NAME,                            \
            priority,                                           \
            cname,                                              \
            cbuf,                                               \
            format,                                             \
            ##args);

#ifndef WITHOUT_LOG4C
#define TIZ_LOG_ERROR  LOG4C_PRIORITY_ERROR
#define TIZ_LOG_WARN   LOG4C_PRIORITY_WARN
#define TIZ_LOG_NOTICE LOG4C_PRIORITY_NOTICE
#define TIZ_LOG_DEBUG  LOG4C_PRIORITY_DEBUG
#define TIZ_LOG_TRACE  LOG4C_PRIORITY_TRACE
#else
#define TIZ_LOG_ERROR  1
#define TIZ_LOG_WARN   2
#define TIZ_LOG_NOTICE 3
#define TIZ_LOG_DEBUG  4
#define TIZ_LOG_TRACE  5
#endif

  int tiz_log_init ();

  int tiz_log_deinit ();

  int tiz_log_setappender (const char *catName, const char *appName);

  void tiz_log (const char *__p_file,
                int __line,
                const char *__p_func,
                const char *__p_cat_name, int __priority,
                /*@null@ */ const char *__p_cname,
                /*@null@ */ char *__p_cbuf,
                const char *__p_format, ...);

#ifdef __cplusplus
}
#endif

#endif                          /* TIZOSALLOG_H */
