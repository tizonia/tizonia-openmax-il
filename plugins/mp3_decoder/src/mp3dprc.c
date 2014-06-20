/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Tizonia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   mp3dprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Mp3 Decoder processor class
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <tizplatform.h>

#include <tizkernel.h>

#include "mp3d.h"
#include "mp3dprc.h"
#include "mp3dprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.mp3_decoder.prc"
#endif

static OMX_ERRORTYPE
release_headers (const void *ap_obj, OMX_U32 a_pid)
{
  mp3d_prc_t *p_obj = (mp3d_prc_t *) ap_obj;

  assert (NULL != ap_obj);

  if (OMX_ALL == a_pid || ARATELIA_MP3_DECODER_INPUT_PORT_INDEX == a_pid)
    {
      if (NULL != p_obj->p_inhdr_)
        {
          tiz_check_omx_err
            (tiz_krn_release_buffer
         (tiz_get_krn (handleOf (ap_obj)),
          ARATELIA_MP3_DECODER_INPUT_PORT_INDEX, p_obj->p_inhdr_));
          p_obj->p_inhdr_ = NULL;
          p_obj->remaining_ = 0;
        }
    }

  if (OMX_ALL == a_pid || ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX == a_pid)
    {
      if (NULL != p_obj->p_outhdr_)
        {
          TIZ_TRACE (handleOf (p_obj),
                     "Releasing output HEADER [%p] nFilledLen [%d]",
                     p_obj->p_outhdr_, p_obj->p_outhdr_->nFilledLen);
          tiz_check_omx_err
            (tiz_krn_release_buffer
             (tiz_get_krn (handleOf (ap_obj)),
              ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX, p_obj->p_outhdr_));
          p_obj->p_outhdr_ = NULL;
        }
    }
  return OMX_ErrorNone;
}

static void
print_frame_info (struct mad_header *Header)
{
  const char *Layer, *Mode, *Emphasis;

  /* Convert the layer number to it's printed representation. */
  switch (Header->layer)
    {
    case MAD_LAYER_I:
      Layer = "I";
      break;
    case MAD_LAYER_II:
      Layer = "II";
      break;
    case MAD_LAYER_III:
      Layer = "III";
      break;
    default:
      Layer = "(unexpected layer value)";
      break;
    }

  /* Convert the audio mode to it's printed representation. */
  switch (Header->mode)
    {
    case MAD_MODE_SINGLE_CHANNEL:
      Mode = "single channel";
      break;
    case MAD_MODE_DUAL_CHANNEL:
      Mode = "dual channel";
      break;
    case MAD_MODE_JOINT_STEREO:
      Mode = "joint (MS/intensity) stereo";
      break;
    case MAD_MODE_STEREO:
      Mode = "normal LR stereo";
      break;
    default:
      Mode = "(unexpected mode value)";
      break;
    }

  /* Convert the emphasis to it's printed representation. Note that
   * the MAD_EMPHASIS_RESERVED enumeration value appeared in libmad
   * version 0.15.0b.
   */
  switch (Header->emphasis)
    {
    case MAD_EMPHASIS_NONE:
      Emphasis = "no";
      break;
    case MAD_EMPHASIS_50_15_US:
      Emphasis = "50/15 us";
      break;
    case MAD_EMPHASIS_CCITT_J_17:
      Emphasis = "CCITT J.17";
      break;
#if (MAD_VERSION_MAJOR>=1) ||                           \
  ((MAD_VERSION_MAJOR==0) && (MAD_VERSION_MINOR>=15))
    case MAD_EMPHASIS_RESERVED:
      Emphasis = "reserved(!)";
      break;
#endif
    default:
      Emphasis = "(unexpected emphasis value)";
      break;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "%lu kb/s audio MPEG layer %s stream %s CRC, "
           "%s with %s emphasis at %d Hz sample rate\n",
           Header->bitrate, Layer,
           Header->flags & MAD_FLAG_PROTECTION ? "with" : "without",
           Mode, Emphasis, Header->samplerate);
}

static signed short
mad_fixed_to_sshort (mad_fixed_t fixed)
{
  /* A fixed point number is formed of the following bit pattern:
   *
   * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
   * MSB                          LSB
   * S ==> Sign (0 is positive, 1 is negative)
   * W ==> Whole part bits
   * F ==> Fractional part bits
   *
   * This pattern contains MAD_F_FRACBITS fractional bits, one
   * should alway use this macro when working on the bits of a fixed
   * point number. It is not guaranteed to be constant over the
   * different platforms supported by libmad.
   *
   * The signed short value is formed, after clipping, by the least
   * significant whole part bit, followed by the 15 most significant
   * fractional part bits. Warning: this is a quick and dirty way to
   * compute the 16-bit number, madplay includes much better
   * algorithms.
   */

  /* Clipping */
  if (fixed >= MAD_F_ONE)
    {
      return (SHRT_MAX);
    }

  if (fixed <= -MAD_F_ONE)
    {
      return (-SHRT_MAX);
    }

  /* Conversion. */
  fixed = fixed >> (MAD_F_FRACBITS - 15);

  return (signed short) fixed;
}

static size_t
read_from_omx_buffer (const mp3d_prc_t *ap_prc, void *ap_dst,
                      size_t bytes, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  size_t to_read = bytes;

  assert (NULL != ap_dst);
  assert (NULL != ap_hdr);

  if (bytes > 0)
    {
      if (ap_hdr->nFilledLen < bytes)
        {
          to_read = ap_hdr->nFilledLen;
        }

      if (to_read > 0)
        {
          memcpy (ap_dst, ap_hdr->pBuffer + ap_hdr->nOffset, to_read);
          TIZ_TRACE (handleOf (ap_prc),
                     "bytes [%d] to_read [%d] nOffset [%d] nFilledLen [%d] nAllocLen [%d]", bytes, to_read,
                     ap_hdr->nOffset, ap_hdr->nFilledLen, ap_hdr->nAllocLen);
        }

      ap_hdr->nFilledLen -= to_read;
      ap_hdr->nOffset += to_read;
    }
  return to_read;
}

static OMX_ERRORTYPE update_pcm_mode (mp3d_prc_t *ap_prc, const OMX_U32 a_samplerate,
                                      const OMX_U32 a_channels)
{
  assert (NULL != ap_prc);
  if (a_samplerate  != ap_prc->pcmmode_.nSamplingRate
      || a_channels != ap_prc->pcmmode_.nChannels)
    {
      TIZ_DEBUG (handleOf (ap_prc),
                 "Updating pcm mode : old samplerate [%d] new samplerate [%d]",
                 ap_prc->pcmmode_.nSamplingRate, a_samplerate);
      TIZ_DEBUG (handleOf (ap_prc),
                 "Updating pcm mode : old channels [%d] new channels [%d]",
                 ap_prc->pcmmode_.nChannels, a_channels);
      ap_prc->pcmmode_.nSamplingRate = a_samplerate;
      ap_prc->pcmmode_.nChannels     = a_channels;
      tiz_check_omx_err (tiz_krn_SetParameter_internal
                         (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamAudioPcm, &(ap_prc->pcmmode_)));
      tiz_srv_issue_event ((OMX_PTR)ap_prc, OMX_EventPortSettingsChanged,
                           ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX,
                           OMX_IndexParamAudioPcm, /* the index of the
                                                      struct that has
                                                      been modififed */
                           NULL);
    }
  return OMX_ErrorNone;
}

static int
synthesize_samples (const void *ap_obj, int next_sample)
{
  mp3d_prc_t *p_prc = (mp3d_prc_t *) ap_obj;
  const unsigned char *p_bufend = p_prc->p_outhdr_->pBuffer
    + p_prc->p_outhdr_->nAllocLen;
  unsigned char *p_output =
    p_prc->p_outhdr_->pBuffer + p_prc->p_outhdr_->nFilledLen;
  bool buffer_full = (p_bufend == p_output);
  int i;

  for (i = next_sample; i < p_prc->synth_.pcm.length && !buffer_full; i++)
    {
      signed short sample;

      /* Left channel */
      sample = mad_fixed_to_sshort (p_prc->synth_.pcm.samples[0][i]);
      *(p_output++) = sample >> 8;
      *(p_output++) = sample & 0xff;

      /* Right channel. If the decoded stream is monophonic then
       * the right output channel is the same as the left one.
       */
      if (MAD_NCHANNELS (&p_prc->frame_.header) == 2)
        {
          sample = mad_fixed_to_sshort (p_prc->synth_.pcm.samples[1][i]);
        }
      *(p_output++) = sample >> 8;
      *(p_output++) = sample & 0xff;

      p_prc->p_outhdr_->nFilledLen += 4;

      if (p_prc->frame_.header.samplerate != p_prc->pcmmode_.nSamplingRate
          || MAD_NCHANNELS (&p_prc->frame_.header) != p_prc->pcmmode_.nChannels)
        {
          (void)update_pcm_mode (p_prc, p_prc->frame_.header.samplerate,
                                 MAD_NCHANNELS (&p_prc->frame_.header));
        }

      /* release the output buffer if it is full. */
      if (p_output == p_bufend)
        {
          p_output    = p_prc->p_outhdr_->pBuffer;
          TIZ_TRACE (handleOf (p_prc),
                    "Releasing output HEADER [%p] nFilledLen [%d]",
                    p_prc->p_outhdr_, p_prc->p_outhdr_->nFilledLen);
          p_prc->p_outhdr_->nFilledLen = p_prc->p_outhdr_->nAllocLen;
          tiz_krn_release_buffer (tiz_get_krn (handleOf (p_prc)),
                                  ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX,
                                  p_prc->p_outhdr_);
          p_prc->p_outhdr_ = NULL;
          buffer_full = true;
        }
    }

  /* Return the sample index if there are more samples to process */
  if (i < p_prc->synth_.pcm.length)
    {
      return i;
    }

  /* Otherwise return 0 */

  return 0;

}

static OMX_ERRORTYPE
decode_buffer (const void *ap_obj)
{
  mp3d_prc_t *p_obj = (mp3d_prc_t *) ap_obj;
  unsigned char *p_guardzone = NULL;
  int status = 0;

  assert (NULL != p_obj->p_outhdr_);

  if (NULL != p_obj->p_inhdr_)
    {
      if ((p_obj->p_inhdr_->nFlags & OMX_BUFFERFLAG_EOS) != 0)
        {
          p_obj->eos_ = true;
        }
    }

  /* Check if there is any remaining PCM data from a previous run of the
   * decoding loop that needs to be synthesised */
  if ((0 != p_obj->next_synth_sample_ &&
       p_obj->next_synth_sample_ < p_obj->synth_.pcm.length)
      && (p_obj->p_outhdr_->nFilledLen < p_obj->p_outhdr_->nAllocLen))
    {
      p_obj->next_synth_sample_
        = synthesize_samples (p_obj, p_obj->next_synth_sample_);
    }

  while (NULL != p_obj->p_outhdr_)
    {

      /* The input bucket must be filled if it becomes empty or if
       * it's the first execution of the loop.
       */

      if ((NULL == p_obj->stream_.buffer
           || MAD_ERROR_BUFLEN == p_obj->stream_.error)
          && p_obj->p_inhdr_ != NULL)
        {
          size_t read_size = 0;
          unsigned char *p_read_start = NULL;
          p_obj->remaining_ = 0;

          if (p_obj->stream_.next_frame != NULL)
            {
              p_obj->remaining_ = p_obj->stream_.bufend
                - p_obj->stream_.next_frame;
              memmove (p_obj->in_buff_, p_obj->stream_.next_frame,
                       p_obj->remaining_);
              p_read_start = p_obj->in_buff_ + p_obj->remaining_;
              read_size = INPUT_BUFFER_SIZE - p_obj->remaining_;
            }
          else
            {
              read_size = INPUT_BUFFER_SIZE;
              p_read_start = p_obj->in_buff_;
              p_obj->remaining_ = 0;
            }

          /* Fill-in the buffer. If an error occurs print a message
           * and leave the decoding loop. If the end of stream is
           * reached we also leave the loop but the return status is
           * left untouched.
           */
          read_size = read_from_omx_buffer (p_obj, p_read_start, read_size,
                                            p_obj->p_inhdr_);
          if (read_size == 0)
            {
              if ((p_obj->p_inhdr_->nFlags & OMX_BUFFERFLAG_EOS) != 0)
                {
                  TIZ_TRACE (handleOf (p_obj),
                            "end of input stream");
                  status = 2;
                }
              else
                {
                  TIZ_TRACE (handleOf (p_obj),
                            "read_size <= 0");
                  status = 1;
                }
              break;
            }


          /* TODO */
          /*       if(BstdFileEofP (BstdFile)) */
          /*         { */
          /*           p_guardzone = p_read_start + read_size; */
          /*           memset (p_guardzone, 0, MAD_BUFFER_GUARD); */
          /*           read_size += MAD_BUFFER_GUARD; */
          /*         } */

          /* Pipe the new buffer content to libmad's stream decoder
           * facility.
           */
          mad_stream_buffer (&p_obj->stream_, p_obj->in_buff_, read_size
                             + p_obj->remaining_);
          p_obj->stream_.error = 0;
        }

      if (mad_frame_decode (&p_obj->frame_, &p_obj->stream_) == -1)
        {
          if (MAD_RECOVERABLE (p_obj->stream_.error))
            {
              if (p_obj->stream_.error != MAD_ERROR_LOSTSYNC
                  || p_obj->stream_.this_frame != p_guardzone)
                {
                  TIZ_TRACE (handleOf (p_obj),
                            "recoverable frame level error (%s)",
                            mad_stream_errorstr (&p_obj->stream_));
                }
              continue;
            }
          else
            {
              if (p_obj->stream_.error == MAD_ERROR_BUFLEN)
                {
                  if (!p_obj->p_inhdr_)
                    {
                      TIZ_TRACE (handleOf (p_obj),
                                "p_obj->stream_.error==MAD_ERROR_BUFLEN "
                                "p_obj->p_inhdr_=[NULL]");
                      break;
                    }
                  else
                    {
                      TIZ_TRACE (handleOf (p_obj),
                                "p_obj->stream_.error==MAD_ERROR_BUFLEN "
                                "p_obj->p_inhdr_=[%p] nFilledLen [%d]",
                                p_obj->p_inhdr_, p_obj->p_inhdr_->nFilledLen);
                      continue;
                    }
                }
              else
                {
                  TIZ_TRACE (handleOf (p_obj),
                            "unrecoverable frame level error (%s).",
                            mad_stream_errorstr (&p_obj->stream_));
                  status = 2;
                  break;
                }
            }
        }

      /* The characteristics of the stream's first frame is printed
       * on stderr. The first frame is representative of the entire
       * stream.
       */
      if (0 == p_obj->frame_count_)
        {
          print_frame_info (&p_obj->frame_.header);
        }

      p_obj->frame_count_++;
      mad_timer_add (&p_obj->timer_, p_obj->frame_.header.duration);

      /* Once decoded the frame is synthesized to PCM samples. No errors
       * are reported by mad_synth_frame();
       */
      mad_synth_frame (&p_obj->synth_, &p_obj->frame_);

      p_obj->next_synth_sample_
        = synthesize_samples (p_obj, p_obj->next_synth_sample_);

    }

  (void) status;
  /* TODO */
/*   if (p_obj->p_outhdr_ != NULL */
/*       && p_obj->p_outhdr_->nFilledLen != 0 */
/*       && status == 2) */
/*     { */
/*       const tiz_srv_t *p_parent = ap_obj; */
/*       void *p_krn = tiz_get_krn (p_parent->p_hdl_); */

/*       p_obj->eos_ = true; */
/*       p_obj->p_outhdr_->nFlags |= OMX_BUFFERFLAG_EOS; */
/*       TIZ_LOG (TIZ_PRIORITY_TRACE, handleOf (p_obj), */
/*                        "Releasing output buffer [%p] ..." */
/*                        "nFilledLen = [%d] OMX_BUFFERFLAG_EOS", */
/*                        p_obj->p_outhdr_, p_obj->p_outhdr_->nFilledLen); */
/*       tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_obj)), 1, p_obj->p_outhdr_); */
/*       /\* p_obj->p_outhdr_ = NULL; *\/ */
/*     } */

  return OMX_ErrorNone;
}

static bool
mp3d_claim_input (mp3d_prc_t *ap_obj)
{
  bool rc = false;
  assert (NULL != ap_obj);

  if (!ap_obj->in_port_disabled_)
    {
      if (OMX_ErrorNone == tiz_krn_claim_buffer
          (tiz_get_krn (handleOf (ap_obj)), 0, 0, &ap_obj->p_inhdr_))
        {
          if (NULL != ap_obj->p_inhdr_)
            {
              TIZ_TRACE (handleOf (ap_obj),
                         "Claimed INPUT HEADER [%p]...nFilledLen [%d]",
                         ap_obj->p_inhdr_, ap_obj->p_inhdr_->nFilledLen);
              rc = true;
            }
        }
    }

  return rc;
}

static bool
mp3d_claim_output (mp3d_prc_t *ap_obj)
{
  bool rc = false;
  assert (NULL != ap_obj);

  if (!ap_obj->out_port_disabled_)
    {
      if (OMX_ErrorNone == tiz_krn_claim_buffer
          (tiz_get_krn (handleOf (ap_obj)), 1, 0, &ap_obj->p_outhdr_))
        {
          if (NULL != ap_obj->p_outhdr_)
            {
              TIZ_TRACE (handleOf (ap_obj),
                         "Claimed OUTPUT HEADER [%p] BUFFER [%p] "
                         "nFilledLen [%d]...", ap_obj->p_outhdr_,
                         ap_obj->p_outhdr_->pBuffer, ap_obj->p_outhdr_->nFilledLen);
              rc = true;
            }
        }
    }
  return rc;
}

/*
 * mp3dprc
 */

static void *
mp3d_proc_ctor (void *ap_obj, va_list * app)
{
  mp3d_prc_t *p_obj = super_ctor (typeOf (ap_obj, "mp3dprc"), ap_obj, app);
  p_obj->remaining_ = 0;
  p_obj->frame_count_ = 0;
  p_obj->p_inhdr_ = 0;
  p_obj->p_outhdr_ = 0;
  p_obj->next_synth_sample_ = 0;
  p_obj->eos_ = false;
  p_obj->in_port_disabled_ = false;
  p_obj->out_port_disabled_ = false;
  return p_obj;
}

static void *
mp3d_proc_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "mp3dprc"), ap_obj);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
mp3d_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  mp3d_prc_t *p_obj = ap_obj;
  assert (NULL != ap_obj);
  mad_stream_init (&p_obj->stream_);
  mad_frame_init (&p_obj->frame_);
  mad_synth_init (&p_obj->synth_);
  mad_timer_reset (&p_obj->timer_);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3d_proc_deallocate_resources (void *ap_obj)
{
  mp3d_prc_t *p_obj = ap_obj;
  assert (NULL != ap_obj);
  mad_synth_finish (&p_obj->synth_);
  mad_frame_finish (&p_obj->frame_);
  mad_stream_finish (&p_obj->stream_);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3d_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  mp3d_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);

  p_prc->eos_ = false;

  TIZ_INIT_OMX_PORT_STRUCT (p_prc->pcmmode_, ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX);
  tiz_check_omx_err (tiz_api_GetParameter
                     (tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
                      OMX_IndexParamAudioPcm, &(p_prc->pcmmode_)));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3d_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3d_proc_stop_and_return (void *ap_obj)
{
  mp3d_prc_t *p_obj = ap_obj;
  char buffer[80];
  assert (NULL != ap_obj);
  mad_timer_string (p_obj->timer_, buffer, "%lu:%02lu.%03u",
                    MAD_UNITS_MINUTES, MAD_UNITS_MILLISECONDS, 0);
  TIZ_TRACE (handleOf (p_obj),
            "%lu frames decoded (%s).\n", p_obj->frame_count_, buffer);
  return release_headers (p_obj, OMX_ALL);
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
mp3d_proc_buffers_ready (const void *ap_obj)
{
  mp3d_prc_t *p_obj = (mp3d_prc_t *) ap_obj;

  assert (NULL != ap_obj);

  while (true)
    {
      if (!p_obj->p_inhdr_)
        {
          if ((!mp3d_claim_input (p_obj)
               && p_obj->stream_.next_frame == NULL)
              || (!p_obj->p_inhdr_
                  && p_obj->stream_.error == MAD_ERROR_BUFLEN))
            {
              break;
            }
        }

      if (!p_obj->p_outhdr_)
        {
          if (!mp3d_claim_output (p_obj))
            {
              break;
            }
        }

      tiz_check_omx_err (decode_buffer (ap_obj));
      if (p_obj->p_inhdr_ != NULL && (0 == p_obj->p_inhdr_->nFilledLen))
        {
          p_obj->p_inhdr_->nOffset = 0;
          tiz_check_omx_err (tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_obj)),
                                                     ARATELIA_MP3_DECODER_INPUT_PORT_INDEX,
                                                     p_obj->p_inhdr_));
          p_obj->p_inhdr_ = NULL;
        }
    }

  if (p_obj->eos_ && p_obj->p_outhdr_ != NULL)
    {
      /* EOS has been received and all the input data has been consumed
       * already, so its time to propagate the EOS flag */
      TIZ_TRACE (handleOf (p_obj),
                "p_obj->eos OUTPUT HEADER [%p]...", p_obj->p_outhdr_);
      p_obj->p_outhdr_->nFlags |= OMX_BUFFERFLAG_EOS;
      tiz_check_omx_err (tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_obj)),
                                                 ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX,
                                                 p_obj->p_outhdr_));
      p_obj->p_outhdr_ = NULL;
      p_obj->eos_ = false;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3d_proc_port_flush (const void *ap_obj, OMX_U32 a_pid)
{
  mp3d_prc_t *p_obj = (mp3d_prc_t *) ap_obj;
  return release_headers (p_obj, a_pid);
}

static OMX_ERRORTYPE
mp3d_proc_port_disable (const void *ap_obj, OMX_U32 a_pid)
{
  mp3d_prc_t *p_obj = (mp3d_prc_t *) ap_obj;
  assert (NULL != p_obj);
  if (OMX_ALL == a_pid || ARATELIA_MP3_DECODER_INPUT_PORT_INDEX == a_pid)
    {
      p_obj->in_port_disabled_ = true;
    }
  if (OMX_ALL == a_pid || ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX == a_pid)
    {
      p_obj->out_port_disabled_ = true;
    }
  return release_headers (p_obj, a_pid);
}

static OMX_ERRORTYPE
mp3d_proc_port_enable (const void *ap_obj, OMX_U32 a_pid)
{
  mp3d_prc_t *p_obj = (mp3d_prc_t *) ap_obj;
  assert (NULL != p_obj);
  if (OMX_ALL == a_pid || ARATELIA_MP3_DECODER_INPUT_PORT_INDEX == a_pid)
    {
      p_obj->in_port_disabled_ = false;
    }
  if (OMX_ALL == a_pid || ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX == a_pid)
    {
      p_obj->out_port_disabled_ = false;
    }
  return OMX_ErrorNone;
}

/*
 * mp3d_prc_class
 */

static void *
mp3d_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "mp3dprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
mp3d_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * mp3dprc_class =
    factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "mp3dprc_class", classOf (tizprc), sizeof (mp3d_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, mp3d_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value */
     0);
  return mp3dprc_class;
}

void *
mp3d_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * mp3dprc_class = tiz_get_type (ap_hdl, "mp3dprc_class");
  TIZ_LOG_CLASS (mp3dprc_class);
  void * mp3dprc =
    factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (mp3dprc_class, "mp3dprc", tizprc, sizeof (mp3d_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, mp3d_proc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, mp3d_proc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, mp3d_proc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, mp3d_proc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, mp3d_proc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, mp3d_proc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, mp3d_proc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, mp3d_proc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, mp3d_proc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, mp3d_proc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, mp3d_proc_port_enable,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return mp3dprc;
}
