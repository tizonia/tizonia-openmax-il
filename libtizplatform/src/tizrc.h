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
 * @file   tizrc.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform  - Functions to retrieve data from config file
 *
 *
 */

#ifndef TIZRC_H
#define TIZRC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup rcfile Configuration file parsing utilities
 * @ingroup Tizonia-Platform
 */

#include <OMX_Core.h>
#include <OMX_Types.h>

#define TIZ_RCFILE_PLUGINS_DATA_SECTION "plugins-data"

/**
 * Returns a value string from a give section using the value's key
 *
 * @ingroup rcfile
 *
 * @param section String indicating the section where the key-value list pair
 * is to be found.
 *
 * @param key A search key in the specified section.
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
 * @param section String indicating the section where the key-value list pair
 * is to be found.
 *
 * @param key A search key in the specified section.
 *
 * @param length The length of the returned list.
 *
 * @return An array of NULL-terminated strings or NULL if the specified key
 *cannot be found. The array should be freed by the caller.
 */
char **tiz_rcfile_get_value_list (const char *section, const char *key,
                                  unsigned long *length);

/**
 * Returns an integer less than, equal to, or greater than zero if the
 * section-key-value triad provided is respectively, not found, found and
 * matching, or found and no matching.
 *
 * @ingroup rcfile
 *
 * @param section String indicating the section where the key-value pair is
 * to be found.
 *
 * @param key A search key in the specified section.
 *
 * @param value The value to be matched against.
 *
 * @return Returns an integer less than (key or section not found), equal to
 * (section and key found, value matched), or greater than zero (section and
 * key found, value unmatched).
 */
int tiz_rcfile_compare_value (const char *section, const char *key,
                              const char *value);

/**
 * Returns 0 if the status of the configuration file is such that a tizonia
 * configuration file has been found at one of the expected locations and the
 * file can be opened and read by the current user.
 *
 * @ingroup rcfile
 *
 * @return On success, zero is returned. On error, -1 is returned.
 */
int tiz_rcfile_status (void);

#ifdef __cplusplus
}
#endif

#endif /* TIZRC_H */
