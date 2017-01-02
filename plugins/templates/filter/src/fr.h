/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   fr.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - File Reader component constants
 *
 *
 */
#ifndef FR_H
#define FR_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

#define ARATELIA_FILE_READER_DEFAULT_ROLE             "file_reader.binary"
#define ARATELIA_FILE_READER_COMPONENT_NAME           "OMX.Aratelia.file_reader.binary"
/* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_FILE_READER_INPUT_PORT_INDEX         0
#define ARATELIA_FILE_READER_OUTPUT_PORT_INDEX        1
#define ARATELIA_FILE_READER_PORT_MIN_BUF_COUNT       2
#define ARATELIA_FILE_READER_PORT_MIN_INPUT_BUF_SIZE  8192
#define ARATELIA_FILE_READER_PORT_MIN_OUTPUT_BUF_SIZE 8192
#define ARATELIA_FILE_READER_PORT_NONCONTIGUOUS       OMX_FALSE
#define ARATELIA_FILE_READER_PORT_ALIGNMENT           0
#define ARATELIA_FILE_READER_PORT_SUPPLIERPREF        OMX_BufferSupplyInput

#ifdef __cplusplus
}
#endif

#endif                          /* FR_H */
