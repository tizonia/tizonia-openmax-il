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
 * @file   tizplatform_internal.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - libtizplatform internal declarations
 *
 * @internal
 *
 */

#ifndef TIZINT_H
#define TIZINT_H

/**
 * Value struct used in the Tizonia Platform config file data structure
 *
 * @private
 */
typedef struct value value_t;
struct value
{
  char * p_value;
  value_t * p_next;
};

/**
 * Key-value pair structure used in the Tizonia Platform config file data
 * structure
 *
 * @private
 */
typedef struct keyval keyval_t;
struct keyval
{
  char * p_key;
  value_t * p_value_list;
  value_t * p_value_iter;
  int valcount;
  keyval_t * p_next;
};

/**
 * Handle to the Tizonia Platform config file data structure
 *
 * @private
 */
typedef struct tiz_rcfile tiz_rcfile_t;
struct tiz_rcfile
{
  keyval_t * p_keyvals;
  int count;
};

/**
 * Initialise the Tizonia config file data structure
 *
 * @private
 *
 * @param rcfile A handle to the Tizonia config file data structure
 *
 * @return OMX_ErrorNone on success. OMX_ErrorInsuficientResources otherwise.
 *
 */
OMX_ERRORTYPE
tiz_rcfile_init (tiz_rcfile_t ** rcfile);

/**
 * Deinitialise the resources allocated for the Tizonia config file data
 * structure
 *
 * @private
 *
 * @param rcfile The handle to the Tizonia config file data structure
 */
void
tiz_rcfile_destroy (tiz_rcfile_t * rcfile);

/**
 * Retrieve the config file handle from the event loop thread
 *
 * @private
 */
tiz_rcfile_t *
tiz_rcfile_get_handle (void);

#endif /* TIZINT_H */
