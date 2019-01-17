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
 * @file   inprocrnd.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - ZMQ inproc socket writer constants
 *
 *
 */
#ifndef INPROCRND_H
#define INPROCRND_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

#define ARATELIA_INPROC_WRITER_AUDIO_ROLE         "audio_writer.inproc"
#define ARATELIA_INPROC_WRITER_VIDEO_ROLE         "video_writer.binary"
#define ARATELIA_INPROC_WRITER_IMAGE_ROLE         "image_writer.binary"
#define ARATELIA_INPROC_WRITER_OTHER_ROLE         "other_writer.binary"
#define ARATELIA_INPROC_WRITER_COMPONENT_NAME     "OMX.Aratelia.inproc_writer.binary"
#define ARATELIA_INPROC_WRITER_PORT_INDEX         0 /* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_INPROC_WRITER_PORT_MIN_BUF_COUNT 2
#define ARATELIA_INPROC_WRITER_PORT_MIN_BUF_SIZE  8192
#define ARATELIA_INPROC_WRITER_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_INPROC_WRITER_PORT_ALIGNMENT     0
#define ARATELIA_INPROC_WRITER_PORT_SUPPLIERPREF  OMX_BufferSupplyInput

#ifdef __cplusplus
}
#endif

#endif                          /* INPROCRND_H */
