/*
 * Copyright (c) 2011 The Khronos Group Inc. 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions: 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software. 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 *
 */

/*
 *  OMX_RoleNames.h - OpenMax IL version 1.2.0
 *  The OMX_RoleNames header file contains the standard role names as defined
 *  strings.
 */

#ifndef OMX_RoleNames_h
#define OMX_RoleNames_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Audio decoder class */
#define OMX_ROLE_AUDIO_DECODER_AAC      "audio_decoder.aac"
#define OMX_ROLE_AUDIO_DECODER_AMRNB    "audio_decoder.amrnb"
#define OMX_ROLE_AUDIO_DECODER_AMRWB    "audio_decoder.amrwb"
#define OMX_ROLS_AUDIO_DEXODER_AMRPLUS  "audio_decoder.amrwb+"
#define OMX_ROLE_AUDIO_DECODER_MP3      "audio_decoder.mp3"
#define OMX_ROLE_AUDIO_DECODER_RA       "audio_decoder.ra"
#define OMX_ROLE_AUDIO_DECODER_WMA      "audio_decoder.wma"

/* Audio encoder class */
#define OMX_ROLE_AUDIO_ENCODER_AAC      "audio_encoder.aac"
#define OMX_ROLE_AUDIO_ENCODER_AMRNB    "audio_encoder.amrnb"
#define OMX_ROLE_AUDIO_ENCODER_AMRWB    "audio_encoder.amrwb"
#define OMX_ROLS_AUDIO_ENCODER_AMRPLUS  "audio_encoder.amrwb+"
#define OMX_ROLE_AUDIO_ENCODER_MP3      "audio_encoder.mp3"

/* Audio mixer class */
#define OMX_ROLE_AUDIO_MIXER_PCM "audio_mixer.pcm"

/* Audio reader class */
#define OMX_ROLE_AUDIO_READER_BINARY "audio_reader.binary"

/* Audio renderer class */
#define OMX_ROLE_AUDIO_RENDERER_PCM "audio_renderer.pcm"

/* Audio writer class */
#define OMX_ROLE_AUDIO_WRITER_BINARY "audio_writer.binary"

/* Audio capturer class */
#define OMX_ROLE_AUDIO_CAPTURER_PCM "audio_capturer.pcm"

/* Audio processor class */
#define OMX_ROLE_AUDIO_PROCESSOR_PCM_STEREO_WIDENING_LOUDSPEAKERS "audio_processor.pcm.stereo_widening_loudspeakers"
#define OMX_ROLE_AUDIO_PROCESSOR_PCM_STEREO_WIDENING_HEADPHONES   "audio_processor.pcm.stereo_widening_headphones"
#define OMX_ROLE_AUDIO_PROCESSOR_PCM_REVERBERATION                "audio_processor.pcm.reverberation"
#define OMX_ROLE_AUDIO_PROCESSOR_PCM_CHORUS                       "audio_processor.pcm.chorus"
#define OMX_ROLE_AUDIO_PROCESSOR_PCM_EQUALIZER                    "audio_processor.pcm.equalizer"

/* 3D audio mixer class */
#define OMX_ROLE_AUDIO_3D_MIXER_PCM_HEADPHONES   "audio_3D_mixer.pcm.headphones"
#define OMX_ROLE_AUDIO_3D_MIXER_PCM_LOUDSPEAKERS "audio_3D_mixer.pcm.loudspeakers"

/* Image decoder class */
#define OMX_ROLE_IMAGE_DECODER_JPEG "image_decoder.JPEG"

/* Image encoder class */
#define OMX_ROLE_IMAGE_ENCODER_JPEG "image_encoder.JPEG"

/* Image reader class */
#define OMX_ROLE_IMAGE_READER_BINARY "image_reader.binary"

/* Image writer class */
#define OMX_ROLE_IMAGE_WRITER_BINARY "image_writer.binary"

/* Video decoder class */
#define OMX_ROLE_VIDEO_DECODER_H263  "video_decoder.h263"
#define OMX_ROLE_VIDEO_DECODER_AVC   "video_decoder.avc"
#define OMX_ROLE_VIDEO_DECODER_MPEG4 "video_decoder.mpeg4"
#define OMX_ROLE_VIDEO_DECODER_RV    "video_decoder.rv"
#define OMX_ROLE_VIDEO_DECODER_WMV   "video_decoder.wmv"
#define OMX_ROLE_VIDEO_DECODER_VC1   "video_decoder.vc1"

/* Video encoder class */
#define OMX_ROLE_VIDEO_ENCODER_H263  "video_encoder.h263"
#define OMX_ROLE_VIDEO_ENCODER_AVC   "video_encoder.avc"
#define OMX_ROLE_VIDEO_ENCODER_MPEG4 "video_encoder.mpeg4"

/* Video reader class */
#define OMX_ROLE_VIDEO_READER_BINARY "video_reader.binary"

/* Video scheduler class */
#define OMX_ROLE_VIDEO_SCHEDULER_BINARY "video_scheduler.binary"

/* Video writer class */
#define OMX_ROLE_VIDEO_WRITER_BINARY "video_writer.binary"

/* Camera class */
#define OMX_ROLE_CAMERA_YUV "camera.yuv"

/* Clock class */
#define OMX_ROLE_CLOCK_BINARY "clock.binary"

/* Container demuxer class */
#define OMX_ROLE_CONTAINER_DEMUXER_3GP  "container_demuxer_3gp"
#define OMX_ROLE_CONTAINER_DEMUXER_ASF  "container_demuxer_asf"
#define OMX_ROLE_CONTAINER_DEMUXER_REAL "container_demuxer_real"

/* Container muxer class */
#define OMX_ROLE_CONTAINER_MUXER_3GP  "container_muxer_3gp"

/* Image/video processor class */
#define OMX_ROLE_IV_PROCESSOR_YUV "iv_processor.yuv"

/* Image/video rendered class */
#define OMX_ROLE_IV_RENDERER_YUV_OVERLAY "iv_renderer.yuv.overlay"
#define OMX_ROLE_IV_RENDERER_YUV_BLTER   "iv_renderer.yuv.blter"
#define OMX_ROLE_IV_RENDERER_RGB_OVERLAY "iv_renderer.rgb.overlay"
#define OMX_ROLE_IV_RENDERER_RGB_BLTER   "iv_renderer.rgb.blter"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
