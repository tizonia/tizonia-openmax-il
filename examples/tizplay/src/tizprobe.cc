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
 * @file   tizprobe.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  File probing utility
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizprobe.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/dict.h>
#include <libavdevice/avdevice.h>
}

static AVDictionary *format_opts;
static AVInputFormat *iformat = NULL;

static int
open_input_file (AVFormatContext ** fmt_ctx_ptr, const char *filename)
{
  int err, i;
  AVFormatContext *fmt_ctx = NULL;
  AVDictionaryEntry *t;

  if ((err = avformat_open_input (&fmt_ctx, filename,
                                  iformat, &format_opts)) < 0)
    {
      return err;
    }

  if ((t = av_dict_get (format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX)))
    {
      av_log (NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
      return AVERROR_OPTION_NOT_FOUND;
    }

  /* fill the streams in the format context */
  if ((err = avformat_find_stream_info (fmt_ctx, NULL)) < 0)
    {
      return err;
    }

  av_dump_format (fmt_ctx, 0, filename, 0);

  *fmt_ctx_ptr = fmt_ctx;
  return 0;
}

static void
close_input_file (AVFormatContext ** ctx_ptr)
{
  int i;
  AVFormatContext *fmt_ctx = *ctx_ptr;

  /* close decoder for each stream */
  for (i = 0; i < fmt_ctx->nb_streams; i++)
    {
      AVStream *stream = fmt_ctx->streams[i];
      avcodec_close (stream->codec);
    }
  avformat_close_input (ctx_ptr);
}

tizprobe::tizprobe (const std::string & uri):
uri_ (uri),
domain_ (OMX_PortDomainMax),
audio_coding_type_ (OMX_AUDIO_CodingUnused),
video_coding_type_ (OMX_VIDEO_CodingUnused),
pcmtype_ (),
mp3type_ (),
vp8type_ ()
{
  // Defaults are the same as in the standard pcm renderer
  pcmtype_.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmtype_.nVersion.nVersion = OMX_VERSION;
  pcmtype_.nPortIndex = 0;
  pcmtype_.nChannels = 2;
  pcmtype_.eNumData = OMX_NumericalDataSigned;
  pcmtype_.eEndian = OMX_EndianBig;
  pcmtype_.bInterleaved = OMX_TRUE;
  pcmtype_.nBitPerSample = 16;
  pcmtype_.nSamplingRate = 48000;
  pcmtype_.ePCMMode = OMX_AUDIO_PCMModeLinear;
  pcmtype_.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  pcmtype_.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

  // Defaults are the same as in the standard mp3 decoder
  mp3type_.nSize = sizeof (OMX_AUDIO_PARAM_MP3TYPE);
  mp3type_.nVersion.nVersion = OMX_VERSION;
  mp3type_.nPortIndex = 0;
  mp3type_.nChannels = 2;
  mp3type_.nBitRate = 0;
  mp3type_.nSampleRate = 48000;
  mp3type_.nAudioBandWidth = 0;
  mp3type_.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  mp3type_.eFormat = OMX_AUDIO_MP3StreamFormatMP2Layer3;

  av_register_all ();

}

OMX_PORTDOMAINTYPE tizprobe::get_omx_domain ()
{
  if (OMX_PortDomainMax == domain_)
    {
      (void) probe_file ();
    }

  return domain_;
}

int
tizprobe::probe_file ()
{
  int ret = 0;
  AVFormatContext *fmt_ctx = NULL;
  AVStream *st = NULL;
  AVCodecContext *cc = NULL;
  CodecID codec_id = CODEC_ID_PROBE;

  if ((ret = open_input_file (&fmt_ctx, uri_.c_str ())))
    {
      return ret;
    }

  if (NULL == (st = fmt_ctx->streams[0]))
    {
      return 1;
    }

  if (NULL == (cc = st->codec))
    {
      return 1;
    }

  codec_id = cc->codec_id;

  if (codec_id == CODEC_ID_MP3)
    {
      domain_ = OMX_PortDomainAudio;
      audio_coding_type_ = OMX_AUDIO_CodingMP3;
      mp3type_.nSampleRate = cc->sample_rate;
      pcmtype_.nSamplingRate = cc->sample_rate;
      mp3type_.nChannels = cc->channels;
      pcmtype_.nChannels = cc->channels;
      if (AV_SAMPLE_FMT_U8 == cc->sample_fmt)
        {
          pcmtype_.eNumData = OMX_NumericalDataUnsigned;
          pcmtype_.nBitPerSample = 8;
        }
      else if (AV_SAMPLE_FMT_S16 == cc->sample_fmt)
        {
          pcmtype_.eNumData = OMX_NumericalDataSigned;
          pcmtype_.nBitPerSample = 16;
        }
      else if (AV_SAMPLE_FMT_S32 == cc->sample_fmt)
        {
          pcmtype_.eNumData = OMX_NumericalDataSigned;
          pcmtype_.nBitPerSample = 32;
        }
      else
        {
          pcmtype_.eNumData = OMX_NumericalDataSigned;
          pcmtype_.nBitPerSample = 16;
        }
    }
  else if (codec_id == CODEC_ID_VP8)
    {
      domain_ = OMX_PortDomainVideo;
      video_coding_type_ = OMX_VIDEO_CodingVP8;
    }

  close_input_file (&fmt_ctx);

  return 0;
}

void
tizprobe::get_pcm_codec_info (OMX_AUDIO_PARAM_PCMMODETYPE & pcmtype)
{
  if (OMX_PortDomainMax == domain_)
    {
      (void) probe_file ();
    }

  pcmtype = pcmtype_;
  pcmtype.eChannelMapping[0] = pcmtype_.eChannelMapping[0];
  pcmtype.eChannelMapping[1] = pcmtype_.eChannelMapping[1];

  return;
}

void
tizprobe::get_mp3_codec_info (OMX_AUDIO_PARAM_MP3TYPE & mp3type)
{
  if (OMX_PortDomainMax == domain_)
    {
      (void) probe_file ();
    }
  mp3type = mp3type_;
  return;
}

void
tizprobe::get_vp8_codec_info (OMX_VIDEO_PARAM_VP8TYPE & vp8type)
{
  if (OMX_PortDomainMax == domain_)
    {
      (void) probe_file ();
    }
  vp8type = vp8type_;
  return;
}
