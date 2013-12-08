/* Copyright (c) 2002-2007 Jean-Marc Valin
   Copyright (c) 2008 CSIRO
   Copyright (c) 2007-2013 Xiph.Org Foundation
   File: opusdec.c

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizutils.h"

#include <math.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.opus_decoder.prc"
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "opusutils.h"
#include <string.h>
#include <stdio.h>

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                           ((buf[base+2]<<16)&0xff0000)| \
                           ((buf[base+1]<<8)&0xff00)| \
                           (buf[base]&0xff))

/* Header contents:
  - "OpusHead" (64 bits)
  - version number (8 bits)
  - Channels C (8 bits)
  - Pre-skip (16 bits)
  - Sampling rate (32 bits)
  - Gain in dB (16 bits, S7.8)
  - Mapping (8 bits, 0=single stream (mono/stereo) 1=Vorbis mapping,
             2..254: reserved, 255: multistream with no mapping)

  - if (mapping != 0)
     - N = totel number of streams (8 bits)
     - M = number of paired streams (8 bits)
     - C times channel origin
          - if (C<2*M)
             - stream = byte/2
             - if (byte&0x1 == 0)
                 - left
               else
                 - right
          - else
             - stream = byte-M
*/


typedef struct {
   int version;
   int channels; /* Number of channels: 1..255 */
   int preskip;
   uint32_t input_sample_rate;
   int gain; /* in dB S7.8 should be zero whenever possible */
   int channel_mapping;
   /* The rest is only used if channel_mapping != 0 */
   int nb_streams;
   int nb_coupled;
   unsigned char stream_map[255];
} OpusHeader;

typedef struct {
   unsigned char *data;
   int maxlen;
   int pos;
} Packet;

typedef struct {
   const unsigned char *data;
   int maxlen;
   int pos;
} ROPacket;

static int read_uint32(ROPacket *p, uint32_t *val)
{
   if (p->pos>p->maxlen-4)
      return 0;
   *val =  (uint32_t)p->data[p->pos  ];
   *val |= (uint32_t)p->data[p->pos+1]<< 8;
   *val |= (uint32_t)p->data[p->pos+2]<<16;
   *val |= (uint32_t)p->data[p->pos+3]<<24;
   p->pos += 4;
   return 1;
}

static int read_uint16(ROPacket *p, uint16_t *val)
{
   if (p->pos>p->maxlen-2)
      return 0;
   *val =  (uint16_t)p->data[p->pos  ];
   *val |= (uint16_t)p->data[p->pos+1]<<8;
   p->pos += 2;
   return 1;
}

static int read_chars(ROPacket *p, unsigned char *str, int nb_chars)
{
   int i;
   if (p->pos>p->maxlen-nb_chars)
      return 0;
   for (i=0;i<nb_chars;i++)
      str[i] = p->data[p->pos++];
   return 1;
}

static int
opus_header_parse(const unsigned char *packet, int len, OpusHeader *h)
{
   int i;
   char str[9];
   ROPacket p;
   unsigned char ch;
   uint16_t shortval;

   p.data = packet;
   p.maxlen = len;
   p.pos = 0;
   str[8] = 0;
   if (len<19)return 0;
   read_chars(&p, (unsigned char*)str, 8);
   if (memcmp(str, "OpusHead", 8)!=0)
      return 0;

   if (!read_chars(&p, &ch, 1))
      return 0;
   h->version = ch;
   if((h->version&240) != 0) /* Only major version 0 supported. */
      return 0;

   if (!read_chars(&p, &ch, 1))
      return 0;
   h->channels = ch;
   if (h->channels == 0)
      return 0;

   if (!read_uint16(&p, &shortval))
      return 0;
   h->preskip = shortval;

   if (!read_uint32(&p, &h->input_sample_rate))
      return 0;

   if (!read_uint16(&p, &shortval))
      return 0;
   h->gain = (short)shortval;

   if (!read_chars(&p, &ch, 1))
      return 0;
   h->channel_mapping = ch;

   if (h->channel_mapping != 0)
   {
      if (!read_chars(&p, &ch, 1))
         return 0;

      if (ch<1)
         return 0;
      h->nb_streams = ch;

      if (!read_chars(&p, &ch, 1))
         return 0;

      if (ch>h->nb_streams || (ch+h->nb_streams)>255)
         return 0;
      h->nb_coupled = ch;

      /* Multi-stream support */
      for (i=0;i<h->channels;i++)
      {
         if (!read_chars(&p, &h->stream_map[i], 1))
            return 0;
         if (h->stream_map[i]>(h->nb_streams+h->nb_coupled) && h->stream_map[i]!=255)
            return 0;
      }
   } else {
      if(h->channels>2)
         return 0;
      h->nb_streams = 1;
      h->nb_coupled = h->channels>1;
      h->stream_map[0]=0;
      h->stream_map[1]=1;
   }
   /*For version 0/1 we know there won't be any more data
     so reject any that have data past the end.*/
   if ((h->version==0 || h->version==1) && p.pos != len)
      return 0;
   return 1;
}

/*Process an Opus header and setup the opus decoder based on it.
  It takes several pointers for header values which are needed
  elsewhere in the code.*/
OpusMSDecoder *
process_opus_header(OMX_HANDLETYPE ap_hdl, OMX_U8 * ap_ogg_data, const OMX_U32 nbytes,
                    opus_int32 *rate, int *mapping_family, int *channels, int *preskip, float *gain,
                    float manual_gain, int *streams, int quiet)
{
   int err;
   OpusMSDecoder *st;
   OpusHeader header;

   if (opus_header_parse(ap_ogg_data, nbytes, &header)==0)
   {
      TIZ_ERROR(ap_hdl, "Cannot parse header\n");
      return NULL;
   }

   *mapping_family = header.channel_mapping;
   *channels       = header.channels;

   if(!*rate)*rate=header.input_sample_rate;
   /*If the rate is unspecified we decode to 48000*/
   if(*rate==0)*rate=48000;
   if(*rate<8000||*rate>192000){
     TIZ_ERROR(ap_hdl, "Warning: Crazy input_rate %d, decoding to 48000 instead.\n",*rate);
     *rate=48000;
   }

   *preskip = header.preskip;
   st = opus_multistream_decoder_create(48000, header.channels, header.nb_streams, header.nb_coupled, header.stream_map, &err);
   if(err != OPUS_OK){
     TIZ_ERROR(ap_hdl, "Cannot create encoder: %s\n", opus_strerror(err));
     return NULL;
   }
   if (!st)
   {
      TIZ_ERROR (ap_hdl, "Decoder initialization failed: %s\n", opus_strerror(err));
      return NULL;
   }

   *streams=header.nb_streams;

   if(header.gain!=0 || manual_gain!=0)
   {
      /*Gain API added in a newer libopus version, if we don't have it
        we apply the gain ourselves. We also add in a user provided
        manual gain at the same time.*/
      int gainadj = (int)(manual_gain*256.)+header.gain;
#ifdef OPUS_SET_GAIN
      err=opus_multistream_decoder_ctl(st,OPUS_SET_GAIN(gainadj));
      if(err==OPUS_UNIMPLEMENTED)
      {
#endif
         *gain = pow(10., gainadj/5120.);
#ifdef OPUS_SET_GAIN
      } else if (err!=OPUS_OK)
      {
         TIZ_ERROR (ap_hdl, "Error setting gain: %s\n", opus_strerror(err));
         return NULL;
      }
#endif
   }

   if (!quiet)
   {
      TIZ_TRACE(ap_hdl, "Decoding to %d Hz (%d channel%s)", *rate,
        *channels, *channels>1?"s":"");
      if(header.version!=1)TIZ_ERROR(ap_hdl, ", Header v%d",header.version);
      TIZ_ERROR(ap_hdl, "\n");
      if (header.gain!=0)TIZ_TRACE(ap_hdl, "Playback gain: %f dB\n", header.gain/256.);
      if (manual_gain!=0)TIZ_TRACE(ap_hdl, "Manual gain: %f dB\n", manual_gain);
   }

   return st;
}

void
process_opus_comments(OMX_HANDLETYPE ap_hdl, char *comments, int length)
{
   char *c=comments;
   int len, i, nb_fields;/* , err=0; */

   if (length<(8+4+4))
   {
      TIZ_ERROR (ap_hdl, "Invalid/corrupted comments\n");
      return;
   }
   if (strncmp(c, "OpusTags", 8) != 0)
   {
      TIZ_ERROR (ap_hdl, "Invalid/corrupted comments\n");
      return;
   }
   c += 8;
   /*    fprintf (stderr, "Encoded with "); */
   len=readint(c, 0);
   c+=4;
   if (len < 0 || len>(length-16))
   {
      TIZ_ERROR (ap_hdl, "Invalid/corrupted comments\n");
      return;
   }
   /*    err&=fwrite(c, 1, len, stderr)!=(unsigned)len; */
   TIZ_TRACE (ap_hdl, "Encoded with %s", c);
   c+=len;
   /*    fprintf (stderr, "\n"); */
   /*The -16 check above makes sure we can read this.*/
   nb_fields=readint(c, 0);
   c+=4;
   length-=16+len;
   if (nb_fields < 0 || nb_fields>(length>>2))
   {
      TIZ_ERROR (ap_hdl, "Invalid/corrupted comments\n");
      return;
   }
   for (i=0;i<nb_fields;i++)
   {
      if (length<4)
      {
         TIZ_ERROR (ap_hdl, "Invalid/corrupted comments\n");
         return;
      }
      len=readint(c, 0);
      c+=4;
      length-=4;
      if (len < 0 || len>length)
      {
         TIZ_ERROR (ap_hdl, "Invalid/corrupted comments\n");
         return;
      }
      /*       err &= fwrite(c, 1, len, stderr)!=(unsigned)len; */
      TIZ_TRACE (ap_hdl, "%s", c);
      c+=len;
      length-=len;
      /*       fprintf (stderr, "\n"); */
   }
}
