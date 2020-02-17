/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   oggmuxfltprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Ogg muxer filter processor declarations
 *
 *
 */

#ifndef OGGDMUXFLTPRC_DECLS_H
#define OGGDMUXFLTPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <oggz/oggz.h>

#include <OMX_Core.h>

#include <tizplatform.h>

#include <tizfilterprc.h>
#include <tizfilterprc_decls.h>

typedef struct oggmuxflt_prc oggmuxflt_prc_t;
struct oggmuxflt_prc
{
  /* Object */
  const tiz_filter_prc_t _;
  OGGZ * p_oggz_;
  long oggz_audio_serialno_;
  long oggz_video_serialno_;
  long oggz_audio_granulepos_;
  long oggz_video_granulepos_;
  long oggz_audio_packetno_;
  long oggz_video_packetno_;
};

typedef struct oggmuxflt_prc_class oggmuxflt_prc_class_t;
struct oggmuxflt_prc_class
{
  /* Class */
  const tiz_filter_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* OGGDMUXFLTPRC_DECLS_H */
