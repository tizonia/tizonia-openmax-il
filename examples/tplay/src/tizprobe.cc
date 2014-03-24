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

#include <string>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/dict.h>
#include <libavdevice/avdevice.h>
}

#include "tizprobe.h"

static AVDictionary *format_opts;
static AVInputFormat *iformat = NULL;

namespace  // unnamed
{
  void dump_stream_info_to_string (AVDictionary *m, std::string &stream_title,
                                   std::string &stream_genre)
  {
    AVDictionaryEntry *tag = NULL;
    std::string artist, title, album, genre;

    while ((tag = av_dict_get (m, "", tag, AV_DICT_IGNORE_SUFFIX)))
    {
      if (0 == strcmp ("artist", tag->key))
      {
        artist.assign (tag->value);
        boost::trim (artist);
      }
      else if (0 == strcmp ("title", tag->key))
      {
        title.assign (tag->value);
        boost::trim (title);
      }
      if (0 == strcmp ("album", tag->key))
      {
        album.assign (tag->value);
        boost::trim (album);
      }
      if (0 == strcmp ("genre", tag->key))
      {
        genre.assign (tag->value);
        boost::trim (genre);
      }
    }

    stream_title.assign (artist);
    if (!album.empty ())
    {
      stream_title.append (" - ");
      stream_title.append (album);
    }

    if (!title.empty ())
    {
      stream_title.append (" - ");
      stream_title.append (title);
    }
    stream_genre.assign (genre);
  }

  void close_input_file (AVFormatContext **ctx_ptr)
  {
    int i = 0;
    AVFormatContext *fmt_ctx = *ctx_ptr;

    /* close decoder for each stream */
    for (i = 0; i < fmt_ctx->nb_streams; i++)
    {
      AVStream *stream = fmt_ctx->streams[i];
      avcodec_close (stream->codec);
    }
    avformat_close_input (ctx_ptr);
  }

  int open_input_file (AVFormatContext **fmt_ctx_ptr,
                       const std::string &filename, std::string &stream_title,
                       std::string &stream_genre, const bool quiet)
  {
    int err = 0;
    AVFormatContext *fmt_ctx = NULL;
    AVDictionaryEntry *t = NULL;

    if ((err = avformat_open_input (&fmt_ctx, filename.c_str (), iformat,
                                    &format_opts)) < 0)
    {
      return err;
    }

    if ((t = av_dict_get (format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX)))
    {
      close_input_file (&fmt_ctx);
      return AVERROR_OPTION_NOT_FOUND;
    }

    /* fill the streams in the format context */
    if ((err = avformat_find_stream_info (fmt_ctx, NULL)) < 0)
    {
      close_input_file (&fmt_ctx);
      return err;
    }

    if (!quiet)
    {

      dump_stream_info_to_string (fmt_ctx->metadata, stream_title,
                                  stream_genre);
      if (stream_title.empty ())
      {
        stream_title.assign (filename.c_str ());
      }
      boost::replace_all (stream_title, "_", " ");
      av_dump_format (fmt_ctx, 0, filename.c_str (), 0);
    }

    *fmt_ctx_ptr = fmt_ctx;
    return 0;
  }
}

tiz::probe::probe (const std::string &uri, const bool quiet)
  : uri_ (uri),
    quiet_ (quiet),
    domain_ (OMX_PortDomainMax),
    audio_coding_type_ (OMX_AUDIO_CodingUnused),
    video_coding_type_ (OMX_VIDEO_CodingUnused),
    pcmtype_ (),
    mp3type_ (),
    opustype_ (),
    flactype_ (),
    vorbistype_ (),
    vp8type_ (),
    meta_file_ (uri.c_str ()),
    stream_title_ (),
    stream_genre_ ()
{
  // Defaults are the same as in the standard pcm renderer
  pcmtype_.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
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
  mp3type_.nSize = sizeof(OMX_AUDIO_PARAM_MP3TYPE);
  mp3type_.nVersion.nVersion = OMX_VERSION;
  mp3type_.nPortIndex = 0;
  mp3type_.nChannels = 2;
  mp3type_.nBitRate = 0;
  mp3type_.nSampleRate = 48000;
  mp3type_.nAudioBandWidth = 0;
  mp3type_.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  mp3type_.eFormat = OMX_AUDIO_MP3StreamFormatMP2Layer3;

  // Defaults for the opus decoder
  opustype_.nSize = sizeof(OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE);
  opustype_.nVersion.nVersion = OMX_VERSION;
  opustype_.nPortIndex = 0;
  opustype_.nChannels = 2;
  opustype_.nBitRate = 256;
  opustype_.nSampleRate = 48000;
  opustype_.nFrameDuration = 2.5;
  opustype_.nEncoderComplexity = 0;
  opustype_.bPacketLossResilience = OMX_FALSE;
  opustype_.bForwardErrorCorrection = OMX_FALSE;
  opustype_.bDtx = OMX_FALSE;
  opustype_.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  opustype_.eFormat = OMX_AUDIO_OPUSStreamFormatVBR;

  // Defaults for the flac decoder
  flactype_.nSize = sizeof(OMX_TIZONIA_AUDIO_PARAM_FLACTYPE);
  flactype_.nVersion.nVersion = OMX_VERSION;
  flactype_.nPortIndex = 0;
  flactype_.nChannels = 2;
  flactype_.nBitsPerSample = 16;
  flactype_.nSampleRate = 48000;
  flactype_.nCompressionLevel = 5;
  flactype_.nBlockSize = 0;
  flactype_.nTotalSamplesEstimate = 0;
  flactype_.eChannelMode = OMX_AUDIO_ChannelModeStereo;

  // Defaults for the vorbis decoder
  vorbistype_.nSize = sizeof(OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE);
  vorbistype_.nVersion.nVersion = OMX_VERSION;
  vorbistype_.nPortIndex = 0;
  vorbistype_.nPortIndex = 0;
  vorbistype_.nChannels = 2;
  vorbistype_.nBitRate = 0;
  vorbistype_.nMinBitRate = 0;
  vorbistype_.nMaxBitRate = 0;
  vorbistype_.nSampleRate = 48000;
  vorbistype_.nAudioBandWidth = 0;
  vorbistype_.nQuality = 5;
  vorbistype_.bManaged = OMX_FALSE;
  vorbistype_.bDownmix = OMX_FALSE;

  av_register_all ();
  av_log_set_level (AV_LOG_QUIET);
}

std::string tiz::probe::get_uri () const
{
  return uri_;
}

OMX_PORTDOMAINTYPE
tiz::probe::get_omx_domain ()
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }

  return domain_;
}

OMX_AUDIO_CODINGTYPE
tiz::probe::get_audio_coding_type ()
{
  if (OMX_AUDIO_CodingUnused == audio_coding_type_)
  {
    (void)probe_file ();
  }
  return audio_coding_type_;
}

OMX_VIDEO_CODINGTYPE
tiz::probe::get_video_coding_type ()
{
  if (OMX_VIDEO_CodingUnused == video_coding_type_)
  {
    (void)probe_file ();
  }
  return video_coding_type_;
}

int tiz::probe::probe_file ()
{
  int ret = 0;
  AVFormatContext *fmt_ctx = NULL;
  AVStream *st = NULL;
  AVCodecContext *cc = NULL;
  CodecID codec_id = CODEC_ID_PROBE;
  std::string extension (boost::filesystem::path (uri_).extension ().string ());

  // For now, for opus files simply rely on the file extension.
  if (extension.compare (".opus") == 0)
  {
    set_opus_codec_info ();
  }
  else
  {
    if (0 == (ret = open_input_file (&fmt_ctx, uri_, stream_title_,
                                     stream_genre_, quiet_)))
    {
      if (NULL == (st = fmt_ctx->streams[0]))
      {
        close_input_file (&fmt_ctx);
        return 1;
      }

      if (NULL == (cc = st->codec))
      {
        close_input_file (&fmt_ctx);
        return 1;
      }

      codec_id = cc->codec_id;

      if (codec_id == CODEC_ID_MP3)
      {
        set_mp3_codec_info (cc);
      }
      else if (codec_id == CODEC_ID_FLAC)
      {
        set_flac_codec_info (cc);
      }
      else if (codec_id == CODEC_ID_VORBIS)
      {
        set_vorbis_codec_info (cc);
      }
      else if (codec_id == CODEC_ID_VP8)
      {
        domain_ = OMX_PortDomainVideo;
        video_coding_type_ = OMX_VIDEO_CodingVP8;
      }
      close_input_file (&fmt_ctx);
    }
    else
    {
      // Unknown format
      return 1;
    }
  }
  return 0;
}

void tiz::probe::set_mp3_codec_info (const AVCodecContext *cc)
{
  assert (NULL != cc);

  domain_ = OMX_PortDomainAudio;
  audio_coding_type_ = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingMP3);
  mp3type_.nSampleRate = cc->sample_rate;
  pcmtype_.nSamplingRate = cc->sample_rate;
  mp3type_.nBitRate = cc->bit_rate;
  mp3type_.nChannels = cc->channels;
  pcmtype_.nChannels = cc->channels;

  if (1 == pcmtype_.nChannels)
  {
    pcmtype_.bInterleaved = OMX_FALSE;
  }

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

void tiz::probe::set_opus_codec_info ()
{
  domain_ = OMX_PortDomainAudio;
  audio_coding_type_
      = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingOPUS);
  opustype_.nSampleRate = 48000;
  pcmtype_.nSamplingRate = 48000;
  mp3type_.nChannels = 2;
  pcmtype_.nChannels = 2;

  pcmtype_.bInterleaved = OMX_TRUE;
  pcmtype_.eNumData = OMX_NumericalDataSigned;
  pcmtype_.nBitPerSample = 16;
  pcmtype_.eEndian = OMX_EndianLittle;
}

void tiz::probe::set_flac_codec_info (const AVCodecContext *cc)
{
  assert (NULL != cc);

  domain_ = OMX_PortDomainAudio;
  audio_coding_type_
      = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingFLAC);
  flactype_.nSampleRate = cc->sample_rate;
  pcmtype_.nSamplingRate = cc->sample_rate;
  //   flactype_.nBitRate     = cc->bit_rate;
  flactype_.nChannels = cc->channels;
  pcmtype_.nChannels = cc->channels;

  if (1 == pcmtype_.nChannels)
  {
    pcmtype_.bInterleaved = OMX_FALSE;
  }

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

  pcmtype_.eEndian = OMX_EndianLittle;
}

void tiz::probe::set_vorbis_codec_info (const AVCodecContext *cc)
{
  assert (NULL != cc);

  domain_ = OMX_PortDomainAudio;
  audio_coding_type_
      = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingVORBIS);
  vorbistype_.nSampleRate = cc->sample_rate;
  pcmtype_.nSamplingRate = cc->sample_rate;
  vorbistype_.nChannels = cc->channels;
  pcmtype_.nChannels = cc->channels;

  if (1 == pcmtype_.nChannels)
  {
    pcmtype_.bInterleaved = OMX_FALSE;
  }

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
  else if (AV_SAMPLE_FMT_FLT == cc->sample_fmt)
  {
    pcmtype_.eNumData = OMX_NumericalDataSigned;
    pcmtype_.nBitPerSample = 32;
  }
  else
  {
    pcmtype_.eNumData = OMX_NumericalDataSigned;
    pcmtype_.nBitPerSample = 16;
  }

  // This is a hack, not sure why libav says this is a 16 bit format
  pcmtype_.nBitPerSample = 32;
  pcmtype_.eEndian = OMX_EndianLittle;
}

void tiz::probe::get_pcm_codec_info (OMX_AUDIO_PARAM_PCMMODETYPE &pcmtype)
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }

  pcmtype = pcmtype_;
  pcmtype.eChannelMapping[0] = pcmtype_.eChannelMapping[0];
  pcmtype.eChannelMapping[1] = pcmtype_.eChannelMapping[1];

  return;
}

void tiz::probe::get_mp3_codec_info (OMX_AUDIO_PARAM_MP3TYPE &mp3type)
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }
  mp3type = mp3type_;
  return;
}

void tiz::probe::get_opus_codec_info (
    OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE &opustype)
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }
  opustype = opustype_;
  return;
}

void tiz::probe::get_flac_codec_info (
    OMX_TIZONIA_AUDIO_PARAM_FLACTYPE &flactype)
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }
  flactype = flactype_;
  return;
}

void tiz::probe::get_vorbis_codec_info (OMX_AUDIO_PARAM_VORBISTYPE &vorbistype)
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }
  vorbistype = vorbistype_;
  return;
}

void tiz::probe::get_vp8_codec_info (OMX_VIDEO_PARAM_VP8TYPE &vp8type)
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }
  vp8type = vp8type_;
  return;
}

std::string tiz::probe::get_stream_title ()
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }
  if (stream_title_.empty ())
  {
    stream_title_.assign (uri_.c_str ());
    boost::replace_all (stream_title_, "_", " ");
  }
  return stream_title_;
}

std::string tiz::probe::get_stream_genre ()
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }
  return stream_genre_;
}

std::string tiz::probe::retrieve_meta_data_str (
    TagLib::String (TagLib::Tag::*TagFunction)() const) const
{
  assert (NULL != TagFunction);
  if (!meta_file_.isNull () && meta_file_.tag ())
  {
    TagLib::Tag *tag = meta_file_.tag ();
    return (tag->*TagFunction)().stripWhiteSpace ().to8Bit ();
  }
  return std::string ();
}

unsigned int tiz::probe::retrieve_meta_data_uint (
    TagLib::uint (TagLib::Tag::*TagFunction)() const) const
{
  assert (NULL != TagFunction);
  if (!meta_file_.isNull () && meta_file_.tag ())
  {
    TagLib::Tag *tag = meta_file_.tag ();
    return (tag->*TagFunction)();
  }
  return 0;
}

std::string tiz::probe::title () const
{
  return retrieve_meta_data_str (&TagLib::Tag::title);
}

std::string tiz::probe::artist () const
{
  return retrieve_meta_data_str (&TagLib::Tag::artist);
}

std::string tiz::probe::album () const
{
  return retrieve_meta_data_str (&TagLib::Tag::album);
}

std::string tiz::probe::year () const
{
  return boost::lexical_cast< std::string >(
      retrieve_meta_data_uint (&TagLib::Tag::year));
}

std::string tiz::probe::comment () const
{
  return retrieve_meta_data_str (&TagLib::Tag::comment);
}

std::string tiz::probe::track () const
{
  return boost::lexical_cast< std::string >(
      retrieve_meta_data_uint (&TagLib::Tag::track));
}

std::string tiz::probe::genre () const
{
  return retrieve_meta_data_str (&TagLib::Tag::genre);
}

std::string tiz::probe::stream_length () const
{
  std::string length_str;

  if (!meta_file_.isNull () && meta_file_.audioProperties ())
  {
    TagLib::AudioProperties *properties = meta_file_.audioProperties ();
    int seconds = properties->length () % 60;
    int minutes = (properties->length () - seconds) / 60;
    int hours = 0;
    if (minutes >= 60)
    {
      int total_minutes = minutes;
      minutes = total_minutes % 60;
      hours = (total_minutes - minutes) / 60;
    }

    if (hours > 0)
    {
      length_str.append (boost::lexical_cast< std::string >(hours));
      length_str.append ("h:");
    }

    if (minutes > 0)
    {
      length_str.append (boost::lexical_cast< std::string >(minutes));
      length_str.append ("m:");
    }

    char seconds_str[3];
    sprintf (seconds_str, "%02i", seconds);
    length_str.append (seconds_str);
    length_str.append ("s");
  }

  return length_str;
}

void tiz::probe::dump_pcm_info ()
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }

#define KNRM "\x1B[0m"
#define KYEL "\x1B[33m"
  fprintf (stdout, "   %s%ld Ch, %g KHz, %lu:%s:%s %s\n", KYEL,
           pcmtype_.nChannels, ((float)pcmtype_.nSamplingRate) / 1000,
           pcmtype_.nBitPerSample,
           pcmtype_.eNumData == OMX_NumericalDataSigned ? "s" : "u",
           pcmtype_.eEndian == OMX_EndianBig ? "b" : "l", KNRM);
}

void tiz::probe::dump_mp3_info ()
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }

#define KNRM "\x1B[0m"
#define KYEL "\x1B[33m"
  fprintf (stdout, "   %s%ld Ch, %g KHz, %lu Kbps %s\n", KYEL,
           mp3type_.nChannels, ((float)mp3type_.nSampleRate) / 1000,
           mp3type_.nBitRate / 1000, KNRM);
}

void tiz::probe::dump_mp3_and_pcm_info ()
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }

#define KNRM "\x1B[0m"
#define KYEL "\x1B[33m"
  fprintf (stdout, "   %s%ld Ch, %g KHz, %lu Kbps, %lu:%s:%s %s\n", KYEL,
           mp3type_.nChannels, ((float)mp3type_.nSampleRate) / 1000,
           mp3type_.nBitRate / 1000, pcmtype_.nBitPerSample,
           pcmtype_.eNumData == OMX_NumericalDataSigned ? "s" : "u",
           pcmtype_.eEndian == OMX_EndianBig ? "b" : "l", KNRM);
}

void tiz::probe::dump_stream_metadata ()
{
  if (OMX_PortDomainMax == domain_)
  {
    (void)probe_file ();
  }

  std::string the_title = title ().empty () ? get_stream_title () : title ();
  std::string the_artist = artist ().empty () ? get_stream_genre () : artist ();

#define KNRM "\x1B[0m"
#define KCYN "\x1B[36m"
  fprintf (
      stdout, "   %s%s, %s - [%s, %.2g MiB] %s\n", KCYN, the_title.c_str (),
      the_artist.c_str (), stream_length ().c_str (),
      ((float)boost::filesystem::file_size (uri_.c_str ()) / (1024 * 1024)),
      KNRM);
  if (!album ().empty ())
  {
    fprintf (stdout, "     %sAlbum   : %s%s\n", KCYN, album ().c_str (), KNRM);
  }
  if (!year ().empty () && year ().compare ("0") != 0)
  {
    fprintf (stdout, "     %sYear    : %s%s\n", KCYN, year ().c_str (), KNRM);
  }
  if (!track ().empty () && track ().compare ("0") != 0)
  {
    fprintf (stdout, "     %sTrack   : %s%s\n", KCYN, track ().c_str (), KNRM);
  }
  if (!genre ().empty ())
  {
    fprintf (stdout, "     %sGenre   : %s%s\n", KCYN, genre ().c_str (), KNRM);
  }
  if (!comment ().empty ())
  {
    fprintf (stdout, "     %sComment : %s%s\n", KCYN, comment ().c_str (),
             KNRM);
  }
}
