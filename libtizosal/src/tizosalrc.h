/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   tizosalrc.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL  - Functions to retrieve data from config file
 *
 *
 */

#ifndef TIZOSALRC_H
#define TIZOSALRC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup rcfile Configuration file parsing utilities
 * @ingroup Tizonia-OSAL
 */

#include <OMX_Core.h>
#include <OMX_Types.h>

#define TIZ_RCFILE_PLUGINS_DATA_SECTION "plugins-data"

/**
 * Returns a value string from a give section using the value's key
 *
 * @ingroup rcfile
 *
 * @param section String indicating the section where the key-value pair is
 *located
 * @param key The key to use to search for the value
 *
 * @return A newly allocated string or NULL if the specified key cannot be
 *found.
 */
const char *tiz_rcfile_get_value (const char *section, const char *key);

/**
 * Returns a value string from a give section using the value's key
 *
 * @ingroup rcfile
 *
 * @param section String indicating the section where the key-value pair is
 *located
 * @param key The key to use to search for the value
 * @param length Return location for the number of returned strings
 *
 * @return An array of NULL-terminated strings or NULL if the specified key
 *cannot
 * be found. The array should be freed by the caller.
 */
char **tiz_rcfile_get_value_list (const char *section, const char *key,
                                  unsigned long *length);

#ifdef __cplusplus
}
#endif

#endif /* TIZOSALRC_H */
