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
 * @file   mp4dmuxfltprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - MP4 Demuxer processor declarations
 *
 *
 */

#ifndef MP4DMUXFLTPRC_DECLS_H
#define MP4DMUXFLTPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <mp4v2/mp4v2.h>

#include <OMX_Core.h>

#include <tizplatform.h>

#include <tizfilterprc.h>
#include <tizfilterprc_decls.h>


typedef struct mp4dmuxflt_prc mp4dmuxflt_prc_t;
struct mp4dmuxflt_prc
{
  /* Object */
  const tiz_filter_prc_t _;
  MP4FileHandle mp4v2_hdl_;
  bool mp4v2_inited_;
  uint64_t mp4v2_duration_;
  int mp4v2_failed_init_count_;
  tiz_buffer_t * p_mp4_store_;
  tiz_buffer_t * p_aud_store_;
  tiz_buffer_t * p_vid_store_;
  tiz_vector_t * p_aud_header_lengths_;
  tiz_vector_t * p_vid_header_lengths_;
  bool audio_metadata_delivered_;
  bool video_metadata_delivered_;
  bool audio_auto_detect_on_;
  OMX_S32 audio_coding_type_;
  bool video_auto_detect_on_;
  OMX_S32 video_coding_type_;
};

typedef struct mp4dmuxflt_prc_class mp4dmuxflt_prc_class_t;
struct mp4dmuxflt_prc_class
{
  /* Class */
  const tiz_filter_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* MP4DMUXFLTPRC_DECLS_H */
