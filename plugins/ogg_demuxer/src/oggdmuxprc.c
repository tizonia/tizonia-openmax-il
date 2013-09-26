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
 * @file   oggdmuxprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Ogg demuxer processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "oggdmuxprc.h"
#include "oggdmuxprc_decls.h"

#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizosal.h"

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.ogg_demuxer.prc"
#endif

#define TIZ_OGG_DEMUXER_INITIAL_READ_BLOCKSIZE 16384
#define TIZ_OGG_DEMUXER_DEFAULT_READ_BLOCKSIZE 2048

#define TIZ_OGG_DEMUXER_DEFAULT_BUFFER_UTILISATION .75

#define ALL_OGG_STREAMS -1

/* Forward declarations */
static OMX_ERRORTYPE oggdmux_prc_deallocate_resources (void *);

static inline void
close_file (/*@special@*/ oggdmux_prc_t * ap_prc)
  /*@releases ap_prc->p_file_ @*/
  /*@ensures isnull ap_prc->p_file_@*/
{
  assert (NULL != ap_prc);
  if (NULL != ap_prc->p_file_)
    {
      (void) fclose (ap_prc->p_file_);
      ap_prc->p_file_ = NULL;
    }
}

static inline void
delete_oggz (/*@special@*/ oggdmux_prc_t * ap_prc)
  /*@releases ap_prc->p_oggz_, ap_prc->p_tracks_ @*/
  /*@ensures isnull ap_prc->p_oggz_, ap_prc->p_tracks_ @*/
{
  assert (NULL != ap_prc);
  oggz_close (ap_prc->p_oggz_);
  ap_prc->p_oggz_ = NULL;
  oggz_table_delete (ap_prc->p_tracks_);
  ap_prc->p_tracks_ = NULL;
}

static inline void
delete_uri (/*@special@*/ oggdmux_prc_t * ap_prc)
  /*@releases ap_prc->p_uri_param_ @*/
  /*@ensures isnull ap_prc->p_uri_param_ @*/
{
  assert (NULL != ap_prc);
  tiz_mem_free (ap_prc->p_uri_param_);
  ap_prc->p_uri_param_ = NULL;
}

static inline void
dealloc_temp_data_stores (/*@special@*/ oggdmux_prc_t * ap_prc)
  /*@releases ap_prc->p_aud_store_, ap_prc->p_vid_store_ @*/
  /*@ensures isnull ap_prc->p_aud_store_, ap_prc->p_vid_store_ @*/
{
  assert (NULL != ap_prc);
  tiz_mem_free (ap_prc->p_aud_store_);
  tiz_mem_free (ap_prc->p_vid_store_);
  ap_prc->p_aud_store_ = NULL;
  ap_prc->p_vid_store_ = NULL;
  ap_prc->aud_store_offset_ = 0;
  ap_prc->vid_store_offset_ = 0;
}

static OMX_ERRORTYPE
obtain_uri (oggdmux_prc_t * ap_prc)
{
  void *p_krn = tiz_get_krn (tiz_api_get_hdl (ap_prc));
  assert (NULL != ap_prc);
  assert (NULL == ap_prc->p_uri_param_);

  if (NULL == (ap_prc->p_uri_param_ = tiz_mem_calloc
               (1, sizeof (OMX_PARAM_CONTENTURITYPE) + OMX_MAX_STRINGNAME_SIZE)))
    {
      TIZ_ERROR (tiz_api_get_hdl (ap_prc),
                "Error allocating memory for the content uri struct");
      return OMX_ErrorInsufficientResources;
    }

  ap_prc->p_uri_param_->nSize = sizeof (OMX_PARAM_CONTENTURITYPE)
    + OMX_MAX_STRINGNAME_SIZE - 1;
  ap_prc->p_uri_param_->nVersion.nVersion = OMX_VERSION;

  tiz_check_omx_err
    (tiz_api_GetParameter (p_krn, tiz_api_get_hdl (ap_prc),
                           OMX_IndexParamContentURI, ap_prc->p_uri_param_));

  TIZ_NOTICE (tiz_api_get_hdl (ap_prc), "URI [%s]",
            ap_prc->p_uri_param_->contentURI);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
alloc_temp_data_stores (oggdmux_prc_t * ap_prc)
{
  void *p_krn = tiz_get_krn (tiz_api_get_hdl (ap_prc));
  OMX_PARAM_PORTDEFINITIONTYPE port_def;

  assert (NULL != ap_prc);

  port_def.nSize = (OMX_U32) sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex = ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX;

  tiz_check_omx_err
    (tiz_api_GetParameter (p_krn, tiz_api_get_hdl (ap_prc),
                           OMX_IndexParamPortDefinition, &port_def));

  assert (ap_prc->p_aud_store_ == NULL);
  tiz_check_null_ret_oom ((ap_prc->p_aud_store_ =
                           tiz_mem_alloc (port_def.nBufferSize * 3)));
  ap_prc->aud_buf_size_ = port_def.nBufferSize;

  port_def.nPortIndex = ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX;
  tiz_check_omx_err
    (tiz_api_GetParameter (p_krn, tiz_api_get_hdl (ap_prc),
                           OMX_IndexParamPortDefinition, &port_def));

  assert (ap_prc->p_vid_store_ == NULL);
  tiz_check_null_ret_oom ((ap_prc->p_vid_store_ =
                           tiz_mem_alloc (port_def.nBufferSize * 3)));
  ap_prc->vid_buf_size_ = port_def.nBufferSize;

  return OMX_ErrorNone;
}

static void
store_data (oggdmux_prc_t * ap_prc, const OMX_U32 a_pid,
            const OMX_U8 *ap_data, OMX_U32 a_nbytes)
{
  OMX_U8 *p_store = NULL;
  OMX_U32 *p_offset = NULL;

  assert (NULL != ap_prc);
  assert (a_pid <= ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);
  assert (NULL != ap_data);

  p_store = a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
    ? ap_prc->p_aud_store_ : ap_prc->p_vid_store_;
  p_offset = a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
    ? &ap_prc->aud_store_offset_ : &ap_prc->vid_store_offset_;

  assert (NULL != p_store);
  assert (NULL != p_offset);

  memcpy (p_store + *p_offset, ap_data, a_nbytes);
  *p_offset += a_nbytes;
}

static void
dump_temp_store (oggdmux_prc_t * ap_prc,
                 const OMX_U32 a_pid,
                 OMX_BUFFERHEADERTYPE * ap_hdr)
{
  OMX_U8  *p_store       = NULL;
  OMX_U32 *p_offset      = NULL;
  OMX_U32  bytes_to_copy = 0;

  assert (NULL != ap_prc);
  assert (a_pid <= ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);
  assert (NULL != ap_hdr);

  p_store = a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
    ? ap_prc->p_aud_store_ : ap_prc->p_vid_store_;
  p_offset = a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
    ? &ap_prc->aud_store_offset_ : &ap_prc->vid_store_offset_;

  assert (NULL != p_store);
  assert (NULL != p_offset);

  TIZ_ERROR (tiz_api_get_hdl (ap_prc),
            "offset [%d] pid [%d] nFilledLen [%d]"
            "nAllocLen [%d]",
            *p_offset, a_pid, ap_hdr->nFilledLen, ap_hdr->nAllocLen);

  if (0 != *p_offset)
    {
      bytes_to_copy = ap_hdr->nAllocLen - ap_hdr->nFilledLen;
      bytes_to_copy = MIN (*p_offset, bytes_to_copy);
      if (bytes_to_copy > 0)
        {
          memcpy (ap_hdr->pBuffer + ap_hdr->nFilledLen, p_store, bytes_to_copy);
          ap_hdr->nFilledLen += bytes_to_copy;
          *p_offset          -= bytes_to_copy;
          if (*p_offset > 0)
            {
              memmove (p_store, p_store + bytes_to_copy, *p_offset);
            }
        }
    }
}

static void
dump_ogg_packet (oggdmux_prc_t * ap_prc,
                 const OMX_U32 a_pid,
                 const ogg_packet * ap_ogg_packet,
                 OMX_BUFFERHEADERTYPE * ap_hdr)
{
  OMX_U32  bytes_to_copy = 0;

  assert (NULL != ap_prc);
  assert (a_pid <= ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);
  assert (NULL != ap_ogg_packet);
  assert (NULL != ap_hdr);

  bytes_to_copy = ap_hdr->nAllocLen - ap_hdr->nFilledLen;
  bytes_to_copy = MIN (ap_ogg_packet->bytes, bytes_to_copy);
  if (bytes_to_copy > 0)
    {
      memcpy (ap_hdr->pBuffer + ap_hdr->nFilledLen,
              ap_ogg_packet->packet, bytes_to_copy);
      ap_hdr->nFilledLen += bytes_to_copy;

      if (ap_ogg_packet->bytes > bytes_to_copy)
        {
          store_data (ap_prc, a_pid, ap_ogg_packet->packet + bytes_to_copy,
                      ap_ogg_packet->bytes - bytes_to_copy);
        }
    }
}

static inline bool
is_audio_content (const OggzStreamContent content)
{
  switch (content)
    {
    case OGGZ_CONTENT_VORBIS:
    case OGGZ_CONTENT_SPEEX:
    case OGGZ_CONTENT_PCM:
    case OGGZ_CONTENT_FLAC0:
    case OGGZ_CONTENT_FLAC:
    case OGGZ_CONTENT_CELT:
    case OGGZ_CONTENT_OPUS:
      return true;
    default:
      return false;
    };
}

static inline bool
is_video_content (const OggzStreamContent content)
{
  switch (content)
    {
    case OGGZ_CONTENT_THEORA:
    case OGGZ_CONTENT_VP8:
      return true;
    default:
      return false;
    };
}

static bool
enough_room_in_buffer (oggdmux_prc_t * ap_prc,
                       const OMX_BUFFERHEADERTYPE * ap_hdr,
                       const OMX_U32 a_pid)
{
  OMX_U32 *p_buf_size = NULL;
  bool rc = false;
  assert (NULL != ap_prc);
  assert (NULL != ap_hdr);
  assert (a_pid <= ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);

  p_buf_size = a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
    ? &(ap_prc->aud_buf_size_) : &(ap_prc->vid_buf_size_);

  assert (NULL != p_buf_size);
/*   return (ap_hdr->nFilledLen */
/*           < (*p_buf_size * TIZ_OGG_DEMUXER_DEFAULT_BUFFER_UTILISATION)); */

  rc = (ap_hdr->nFilledLen
        < (*p_buf_size * TIZ_OGG_DEMUXER_DEFAULT_BUFFER_UTILISATION));
  TIZ_TRACE (tiz_api_get_hdl (ap_prc),
            "buf_size [%d] filled len [%d] rc [%s]", *p_buf_size,
            ap_hdr->nFilledLen, rc ? "TRUE" : "FALSE");
  return rc;
}

static OMX_BUFFERHEADERTYPE *
buffer_needed (oggdmux_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  bool *p_port_disabled = NULL;
  assert (NULL != ap_prc);
  assert (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
          || a_pid == ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);

  pp_hdr = (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
            ? &(ap_prc->p_aud_hdr_) : &(ap_prc->p_vid_hdr_));
  assert (NULL != pp_hdr);

  p_port_disabled = (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
                     ? &(ap_prc->aud_port_disabled_)
                     : &(ap_prc->vid_port_disabled_));
  assert (NULL != p_port_disabled);

  if (false == *p_port_disabled)
    {
      if (NULL != *pp_hdr)
        {
          TIZ_TRACE (tiz_api_get_hdl (ap_prc),
                    "HEADER [%p] exists...nFilledLen [%d] pid [%d]",
                    *pp_hdr, (*pp_hdr)->nFilledLen, a_pid);
          return *pp_hdr;
        }
      else
        {
          tiz_pd_set_t ports;
          void *p_krn = NULL;

          p_krn = tiz_get_krn (tiz_api_get_hdl (ap_prc));

          TIZ_PD_ZERO (&ports);
          if (OMX_ErrorNone == tiz_krn_select (p_krn, 2, &ports))
            {
              if (TIZ_PD_ISSET (a_pid, &ports))
                {
                  if (OMX_ErrorNone == tiz_krn_claim_buffer
                      (p_krn, a_pid, 0, pp_hdr))
                    {
                      TIZ_TRACE (tiz_api_get_hdl (ap_prc),
                                "Claimed HEADER [%p]...nFilledLen [%d] "
                                "a_pid [%d]", *pp_hdr, (*pp_hdr)->nFilledLen,
                                a_pid);
                      return *pp_hdr;
                    }
                }
            }
        }
      ap_prc->awaiting_buffers_ = true;
    }

  return NULL;
}

static void
buffer_filled (oggdmux_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  assert (NULL != ap_prc);
  assert (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
          || a_pid == ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);

  pp_hdr = (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
            ? &(ap_prc->p_aud_hdr_) : &(ap_prc->p_vid_hdr_));
  assert (NULL != pp_hdr && NULL != *pp_hdr);

  p_hdr = *pp_hdr;

  TIZ_TRACE (tiz_api_get_hdl (ap_prc), "Releasing HEADER [%p] "
            "nFilledLen [%d] pid [%d]", p_hdr, p_hdr->nFilledLen, a_pid);

  p_hdr->nOffset = 0;

  if ((p_hdr->nFlags & OMX_BUFFERFLAG_EOS) != 0)
    {
      bool *p_eos = (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
                     ? &(ap_prc->aud_eos_) : &(ap_prc->vid_eos_));

      TIZ_TRACE (tiz_api_get_hdl (ap_prc),
                "OMX_BUFFERFLAG_EOS in HEADER [%p]", p_hdr);

      /* Reset the internal flag */
      *p_eos = false;
      tiz_srv_issue_event ((OMX_PTR) ap_prc,
                           OMX_EventBufferFlag, a_pid, p_hdr->nFlags, NULL);
    }

  /* TODO: Check for OOM error and issue Error Event */
  (void) tiz_krn_release_buffer
    (tiz_get_krn (tiz_api_get_hdl (ap_prc)), a_pid, p_hdr);
  *pp_hdr = NULL;
}

static int
read_packet (OGGZ * ap_oggz, oggz_packet * ap_zp, long serialno,
             void *ap_user_data, const OMX_U32 a_pid)
{
  ogg_packet *p_op = NULL;
  oggdmux_prc_t *p_prc = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  bool *p_eos = NULL;

  assert (NULL != ap_oggz);
  assert (NULL != ap_zp);
  assert (NULL != ap_user_data);
  assert (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
          || a_pid == ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);

  p_prc = ap_user_data;
  p_op  = &ap_zp->op;
  p_eos = (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
           ? &(p_prc->aud_eos_) : &(p_prc->vid_eos_));

  TIZ_TRACE (tiz_api_get_hdl (p_prc),
            "%010lu: bytes [%d]", serialno, p_op->bytes);

  if (NULL == (p_hdr = buffer_needed (p_prc, a_pid)))
    {
      /* Stop until we have some OMX buffers available */
      TIZ_TRACE (tiz_api_get_hdl (p_prc),
                "%010lu: no OMX buffers available", serialno);
      /* Temporarily store the data until an omx buffer is ready */
      store_data (p_prc, a_pid, p_op->packet, p_op->bytes);
      return OGGZ_STOP_OK;
    }

#if 0
  if (got_an_eos)
    {
      printf ("[%010lu]\t%ld bytes\tgranulepos %ld\n", serialno, p_op->bytes,
              (long) p_op->granulepos);
    }
#endif

  if (p_op->b_o_s > 0)
    {
#ifndef S_SPLINT_S
      TIZ_TRACE (tiz_api_get_hdl (p_prc),
                "%010lu: [%" PRId64 "] BOS %8s\n",
                serialno, p_op->granulepos, p_op->packet);
#endif
    }

  if (oggz_get_eos (ap_oggz, serialno) == 1)
    {
      *p_eos = true;
#ifndef S_SPLINT_S
      TIZ_TRACE (tiz_api_get_hdl (p_prc),
                "%010lu: [%" PRId64 "] EOS\n", serialno, p_op->granulepos);
#endif
    }

  if (*p_eos)
    {
      TIZ_TRACE (tiz_api_get_hdl (p_prc),
                "%010lu: Adding EOS flag\n", serialno);
      p_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
    }

  
  if (*p_eos || !enough_room_in_buffer (p_prc, p_hdr, a_pid))
    {
      buffer_filled (p_prc, a_pid);

      if (NULL == (p_hdr = buffer_needed (p_prc, a_pid)))
        {
          TIZ_TRACE (tiz_api_get_hdl (p_prc),
                    "%010lu: Stop until we get more OMX buffers", serialno);
          return OGGZ_STOP_OK;
        }
    }

  dump_temp_store (p_prc, a_pid, p_hdr);

  if (*p_eos || !enough_room_in_buffer (p_prc, p_hdr, a_pid))
    {
      buffer_filled (p_prc, a_pid);

      if (NULL == (p_hdr = buffer_needed (p_prc, a_pid)))
        {
          TIZ_TRACE (tiz_api_get_hdl (p_prc),
                    "%010lu: Stop until we get more OMX buffers", serialno);
          return OGGZ_STOP_OK;
        }
    }

  dump_ogg_packet (p_prc, a_pid, p_op, p_hdr);

  if (*p_eos || !enough_room_in_buffer (p_prc, p_hdr, a_pid))
    {
      buffer_filled (p_prc, a_pid);

      if (NULL == (p_hdr = buffer_needed (p_prc, a_pid)))
        {
          TIZ_TRACE (tiz_api_get_hdl (p_prc),
                    "%010lu: Stop until we get more OMX buffers", serialno);
          return OGGZ_STOP_OK;
        }
    }

  return OGGZ_CONTINUE;
}

static int
read_audio_packet (OGGZ * ap_oggz, oggz_packet * ap_zp, long serialno,
                   void *ap_user_data)
{
  oggdmux_prc_t *p_prc = ap_user_data;
  int rc = OGGZ_CONTINUE;
  assert (NULL != ap_user_data);

  if (!p_prc->aud_port_disabled_
      && is_audio_content (oggz_stream_get_content (p_prc->p_oggz_, serialno)))
    {
      TIZ_TRACE (tiz_api_get_hdl (p_prc),
                "Called read_audio_packet callback");
      rc = read_packet (ap_oggz, ap_zp, serialno, ap_user_data,
                        ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX);
    }
  return rc;
}

static int
read_video_packet (OGGZ * ap_oggz, oggz_packet * ap_zp, long serialno,
                   void *ap_user_data)
{
  oggdmux_prc_t *p_prc = ap_user_data;
  int rc = OGGZ_CONTINUE;
  assert (NULL != ap_user_data);

  if (!p_prc->vid_port_disabled_
      && is_video_content (oggz_stream_get_content (p_prc->p_oggz_, serialno)))
    {
      TIZ_TRACE (tiz_api_get_hdl (p_prc),
                "Called read_video_packet callback");
      rc = read_packet (ap_oggz, ap_zp, serialno, ap_user_data,
                        ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);
    }
  return rc;
}

static OMX_ERRORTYPE
release_buffers (oggdmux_prc_t * ap_prc, const OMX_U32 a_pid)
{
  assert (NULL != ap_prc);

  TIZ_TRACE (tiz_api_get_hdl (ap_prc), "pid [%d]", a_pid);

  if ((a_pid    == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
       || a_pid == OMX_ALL)
      && (NULL != ap_prc->p_aud_hdr_))
    {
      void *p_krn = tiz_get_krn (tiz_api_get_hdl (ap_prc));
      tiz_check_omx_err
        (tiz_krn_release_buffer (p_krn,
                                 ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX,
                                 ap_prc->p_aud_hdr_));
      ap_prc->p_aud_hdr_ = NULL;
    }

  if ((a_pid    == ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX
       || a_pid == OMX_ALL)
      && (NULL != ap_prc->p_vid_hdr_))
    {
      void *p_krn = tiz_get_krn (tiz_api_get_hdl (ap_prc));
      tiz_check_omx_err
        (tiz_krn_release_buffer (p_krn,
                                 ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX,
                                 ap_prc->p_vid_hdr_));
      ap_prc->p_vid_hdr_ = NULL;
    }

  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE
do_flush (oggdmux_prc_t * ap_prc)
{
  assert (NULL != ap_prc);
  TIZ_TRACE (tiz_api_get_hdl (ap_prc), "do_flush");
  (void) oggz_purge (ap_prc->p_oggz_);
  ap_prc->aud_store_offset_ = 0;
  ap_prc->vid_store_offset_ = 0;
  /* Release any buffers held  */
  return release_buffers (ap_prc, OMX_ALL);
}

static size_t
io_read (void *ap_user_handle, void *ap_buf, size_t n)
{
  oggdmux_prc_t *p_prc = ap_user_handle;
  FILE *f = NULL;
  ssize_t bytes_read = 0;

  assert (NULL != p_prc);
  f = p_prc->p_file_;

  bytes_read = read (fileno (f), ap_buf, n);
  TIZ_TRACE (tiz_api_get_hdl (p_prc),
            "bytes_read [%d] buf [%p] n [%d]", bytes_read, ap_buf, n);
  if (0 == bytes_read)
    {
      p_prc->aud_eos_ = true;
      p_prc->vid_eos_ = true;
    }
  return bytes_read;
}

static int
io_seek (void *ap_user_handle, long offset, int whence)
{
  oggdmux_prc_t *p_prc = ap_user_handle;
  FILE *f = NULL;
  assert (NULL != p_prc);
  f = p_prc->p_file_;
  return (fseek (f, offset, whence));
}

static long
io_tell (void *ap_user_handle)
{
  oggdmux_prc_t *p_prc = ap_user_handle;
  FILE *f = NULL;
  assert (NULL != p_prc);
  f = p_prc->p_file_;
  return ftell (f);
}

static int
read_page_normal (OGGZ * ap_oggz, const ogg_page * ap_og,
                  long a_serialno, void *ap_user_data)
{
  return OGGZ_CONTINUE;
}

static int
read_page_first_pass (OGGZ * ap_oggz, const ogg_page * ap_og,
                      long a_serialno, void *ap_user_data)
{
  int rc = OGGZ_CONTINUE;
  oggdmux_prc_t *p_prc = ap_user_data;
  assert (NULL != p_prc);
  assert (NULL != ap_oggz);

  if (oggz_get_bos (ap_oggz, a_serialno) > 0)
    {
      TIZ_TRACE (tiz_api_get_hdl (p_prc),
                "serialno = [%d]", a_serialno);
      if (NULL == oggz_table_insert (p_prc->p_tracks_,
                                     a_serialno,
                                     &read_page_first_pass))  /* NULL makes it barf, needs
                                                               * something */
        {
          TIZ_ERROR (tiz_api_get_hdl (p_prc), "serialno = [%d] - "
                    "Could not insert serialno in oggz table", a_serialno);
          rc = OGGZ_STOP_ERR;
        }
    }

  if (oggz_get_bos (ap_oggz, ALL_OGG_STREAMS) == 0)
    {
      TIZ_TRACE (tiz_api_get_hdl (p_prc),
                "Number of tracks [%d]",
                oggz_get_numtracks (ap_oggz));
      return OGGZ_STOP_OK;
    }

  return rc;
}

static inline OMX_ERRORTYPE
seek_to_byte_offset (oggdmux_prc_t * ap_prc, const oggz_off_t a_offset)
{
  assert (NULL != ap_prc);
  if (oggz_seek (ap_prc->p_oggz_, a_offset, SEEK_SET) == -1)
    {
      TIZ_ERROR (tiz_api_get_hdl (ap_prc),
                "[OMX_ErrorInsufficientResources] : "
                "Could not seek to [%d] offset", a_offset);
      return OMX_ErrorInsufficientResources;
    }
  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE
set_read_page_callback (oggdmux_prc_t * ap_prc,
                        OggzReadPage ap_read_cback)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_read_cback);
  if (oggz_set_read_page (ap_prc->p_oggz_, ALL_OGG_STREAMS,
                          ap_read_cback, ap_prc) < 0)
    {
      TIZ_ERROR (tiz_api_get_hdl (ap_prc),
                "[OMX_ErrorInsufficientResources] : "
                "Could not set read page callback.");
      return OMX_ErrorInsufficientResources;
    }
  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE
set_read_packet_callback (oggdmux_prc_t * ap_prc, long serialno,
                          OggzReadPacket ap_read_cback)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_read_cback);
  if (oggz_set_read_callback (ap_prc->p_oggz_, serialno,
                              ap_read_cback, ap_prc) < 0)
    {
      TIZ_ERROR (tiz_api_get_hdl (ap_prc),
                "[OMX_ErrorInsufficientResources] : "
                "Could not set read packet callback.");
      return OMX_ErrorInsufficientResources;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
obtain_tracks (oggdmux_prc_t * ap_prc)
{
  oggdmux_prc_t *p_prc = ap_prc;
  long n = 0;
  assert (NULL != p_prc);

  /* Seek to beginning of file and set the first pass callback that will help
     with the discovery of the codecs*/
  tiz_check_omx_err (seek_to_byte_offset (p_prc, 0));
  tiz_check_omx_err (set_read_page_callback (p_prc, read_page_first_pass));

  while ((n = oggz_read (p_prc->p_oggz_,
                         TIZ_OGG_DEMUXER_INITIAL_READ_BLOCKSIZE)) > 0);

  /* Seek to beginning of file and set the normal callback (no-op function) */
  tiz_check_omx_err (seek_to_byte_offset (p_prc, 0));
  tiz_check_omx_err (set_read_page_callback (p_prc, read_page_normal));

  return OMX_ErrorNone;
}

static void
print_codec_name (oggdmux_prc_t * ap_prc, long serialno)
{
  if (NULL != ap_prc)
    {
      OggzStreamContent content = oggz_stream_get_content (ap_prc->p_oggz_,
                                                           serialno);
      const char *p_codec_name = oggz_content_type (content);
      TIZ_TRACE (tiz_api_get_hdl (ap_prc), "%010lu: codec [%s]",
                serialno,
                p_codec_name != NULL ? p_codec_name : "Unknown");
    }
}

static OMX_ERRORTYPE
set_read_packet_callbacks (oggdmux_prc_t * ap_prc)
{
  long serialno = 0;
  int n = 0;
  int i = 0;
  void *p_nth_data = NULL;

  assert (NULL != ap_prc);

  n = oggz_table_size (ap_prc->p_tracks_);
  TIZ_TRACE (tiz_api_get_hdl (ap_prc),
            "oggz table size [%d]", n);
  for (i = 0; i < n; i++)
    {
      OggzStreamContent content = OGGZ_CONTENT_UNKNOWN;
      p_nth_data = oggz_table_nth (ap_prc->p_tracks_, i, &serialno);
      assert (NULL != p_nth_data);
      print_codec_name (ap_prc, serialno);
      content = oggz_stream_get_content (ap_prc->p_oggz_, serialno);
      if (is_audio_content (content))
        {
          TIZ_TRACE (tiz_api_get_hdl (ap_prc),
                    "Set read_audio_packet callback");
          tiz_check_omx_err
            (set_read_packet_callback (ap_prc, serialno, read_audio_packet));
        }

      if (is_video_content (content))
        {
          TIZ_TRACE (tiz_api_get_hdl (ap_prc),
                    "Set read_video_packet callback");
          tiz_check_omx_err
            (set_read_packet_callback (ap_prc, serialno, read_video_packet));
        }
    }

  if (oggz_run_set_blocksize (ap_prc->p_oggz_,
                              TIZ_OGG_DEMUXER_DEFAULT_READ_BLOCKSIZE) != 0)
    {
      TIZ_ERROR (tiz_api_get_hdl (ap_prc),
                "[OMX_ErrorInsufficientResources] : "
                "Could not set the oggz block size.");
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
demux_file (oggdmux_prc_t * ap_prc)
{
  int run_status = 0;
  assert (NULL != ap_prc);
  run_status = oggz_run (ap_prc->p_oggz_);
  TIZ_TRACE (tiz_api_get_hdl (ap_prc), "run_status [%d]",
            run_status);

  if (ap_prc->aud_eos_)
    {
      OMX_BUFFERHEADERTYPE *p_hdr
        = buffer_needed (ap_prc, ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX);
      if (NULL != ap_prc->p_aud_hdr_)
        {
          TIZ_TRACE (tiz_api_get_hdl (ap_prc),
                    "Audio: Adding EOS flag\n");
          p_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
          buffer_filled (ap_prc, ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX);
        }
    }

  if (ap_prc->vid_eos_)
    {
      OMX_BUFFERHEADERTYPE *p_hdr
        = buffer_needed (ap_prc, ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);
      if (NULL != ap_prc->p_vid_hdr_)
        {
          TIZ_TRACE (tiz_api_get_hdl (ap_prc),
                    "Video: Adding EOS flag\n");
          p_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
          buffer_filled (ap_prc, ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);
        }
    }
  return OMX_ErrorNone;
}

/*
 * oggdmuxprc
 */

static void *
oggdmux_prc_ctor (void *ap_obj, va_list * app)
{
  oggdmux_prc_t *p_prc      = super_ctor (oggdmuxprc, ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_file_            = NULL;
  p_prc->p_uri_param_       = NULL;
  p_prc->p_oggz_            = NULL;
  p_prc->p_tracks_          = NULL;
  p_prc->p_aud_hdr_         = NULL;
  p_prc->p_vid_hdr_         = NULL;
  p_prc->awaiting_buffers_  = true;
  p_prc->p_aud_store_       = NULL;
  p_prc->p_vid_store_       = NULL;
  p_prc->aud_store_offset_  = 0;
  p_prc->vid_store_offset_  = 0;
  p_prc->aud_buf_size_      = 0;
  p_prc->vid_buf_size_      = 0;
  p_prc->aud_eos_           = false;
  p_prc->vid_eos_           = false;
  p_prc->aud_port_disabled_ = false;
  p_prc->vid_port_disabled_ = false;
  return p_prc;
}

static void *
oggdmux_prc_dtor (void *ap_obj)
{
  (void) oggdmux_prc_deallocate_resources (ap_obj);
  return super_dtor (oggdmuxprc, ap_obj);
}

/*
 * from tizsrv class
 */
static OMX_ERRORTYPE
oggdmux_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  oggdmux_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  assert (NULL == p_prc->p_oggz_);
  assert (NULL == p_prc->p_uri_param_);

  tiz_check_omx_err (obtain_uri (p_prc));

  TIZ_TRACE (tiz_api_get_hdl (p_prc), "Allocating resources");

  tiz_check_omx_err (alloc_temp_data_stores (p_prc));

  if ((p_prc->p_file_
       = fopen ((const char *) p_prc->p_uri_param_->contentURI, "r")) == 0)
    {
      TIZ_ERROR (tiz_api_get_hdl (p_prc),
                "Error opening file from URI (%s)", strerror (errno));
      return OMX_ErrorInsufficientResources;
    }

  if (NULL == (p_prc->p_oggz_ = oggz_new (OGGZ_READ | OGGZ_AUTO)))
    {
      TIZ_ERROR (tiz_api_get_hdl (p_prc),
                "Cannot create a new oggz object (%s)", strerror (errno));
      return OMX_ErrorInsufficientResources;
    }

  if (NULL == (p_prc->p_tracks_ = oggz_table_new ()))
    {
      TIZ_ERROR (tiz_api_get_hdl (p_prc),
                "Cannot create a new oggz object");
      return OMX_ErrorInsufficientResources;
    }

  if (oggz_io_set_read (p_prc->p_oggz_, io_read, p_prc) != 0)
    {
      TIZ_ERROR (tiz_api_get_hdl (p_prc),
                "[OMX_ErrorInsufficientResources] : "
                "Cannot set the oggz io read callback");
      return OMX_ErrorInsufficientResources;
    }

  if (oggz_io_set_seek (p_prc->p_oggz_, io_seek, p_prc) != 0)
    {
      TIZ_ERROR (tiz_api_get_hdl (p_prc),
                "[OMX_ErrorInsufficientResources] : "
                "Cannot set the oggz io seek callback");
      return OMX_ErrorInsufficientResources;
    }

  if (oggz_io_set_tell (p_prc->p_oggz_, io_tell, p_prc) != 0)
    {
      TIZ_ERROR (tiz_api_get_hdl (p_prc),
                "[OMX_ErrorInsufficientResources] : "
                "Cannot set the oggz io tell callback");
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggdmux_prc_deallocate_resources (void *ap_obj)
{
  oggdmux_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  TIZ_TRACE (tiz_api_get_hdl (p_prc), "Deallocating resources");
  close_file (p_prc);
  delete_oggz (p_prc);
  delete_uri (p_prc);
  dealloc_temp_data_stores (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggdmux_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  oggdmux_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  tiz_check_omx_err (obtain_tracks (p_prc));
  tiz_check_omx_err (set_read_packet_callbacks (p_prc));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggdmux_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggdmux_prc_stop_and_return (void *ap_obj)
{
  oggdmux_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  TIZ_TRACE (tiz_api_get_hdl (p_prc), "stop_and_return");
  return do_flush (p_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
oggdmux_prc_buffers_ready (const void *ap_obj)
{
  oggdmux_prc_t *p_prc = (oggdmux_prc_t *) ap_obj;
  TIZ_TRACE (tiz_api_get_hdl (p_prc),
            "Received buffer ready notification - "
            "awaiting_buffers [%s] aud eos [%s]"
            "vid eos [%s]",
            p_prc->awaiting_buffers_ ? "YES" : "NO",
            p_prc->aud_eos_ ? "YES" : "NO",
            p_prc->vid_eos_ ? "YES" : "NO");
  if (p_prc->awaiting_buffers_
      && (!p_prc->aud_eos_ || !p_prc->vid_eos_))
    {
      p_prc->awaiting_buffers_ = false;
      return demux_file (p_prc);
    }
  else if (p_prc->awaiting_buffers_ && p_prc->aud_eos_)
    {
      OMX_BUFFERHEADERTYPE *p_hdr
        = buffer_needed (p_prc, ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX);
      p_prc->awaiting_buffers_ = false;
      if (NULL != p_prc->p_aud_hdr_)
        {
          TIZ_TRACE (tiz_api_get_hdl (p_prc),
                    "Audio: Adding EOS flag\n");
          p_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
          buffer_filled (p_prc, ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX);
        }
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggdmux_prc_port_flush (const void *ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  oggdmux_prc_t *p_prc = (oggdmux_prc_t *) ap_obj;
  assert (NULL != p_prc);
  return do_flush (p_prc);
}

static OMX_ERRORTYPE
oggdmux_prc_port_disable (const void *ap_obj, OMX_U32 a_pid)
{
  oggdmux_prc_t *p_prc = (oggdmux_prc_t *) ap_obj;
  bool *p_port_disabled = NULL;
  assert (NULL != p_prc);
  assert (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
          || a_pid == ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);

  p_port_disabled  = (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
                     ? &(p_prc->aud_port_disabled_)
                     : &(p_prc->vid_port_disabled_));
  assert (NULL != p_port_disabled);
  *p_port_disabled = true;

  /* Release any buffers held  */
  TIZ_TRACE (tiz_api_get_hdl (p_prc), "port_disable");
  return release_buffers (p_prc, a_pid);
}

static OMX_ERRORTYPE
oggdmux_prc_port_enable (const void *ap_obj, OMX_U32 a_pid)
{
  /* TODO */
  return OMX_ErrorNone;
}

/*
 * initialization
 */

const void *oggdmuxprc;

OMX_ERRORTYPE
oggdmux_prc_init (void)
{
  if (!oggdmuxprc)
    {
      tiz_check_omx_err_ret_oom (tiz_prc_init ());
      tiz_check_null_ret_oom
        (oggdmuxprc =
         factory_new
         (tizprc_class,
          "oggdmuxprc",
          tizprc,
          sizeof (oggdmux_prc_t),
          ctor, oggdmux_prc_ctor,
          dtor, oggdmux_prc_dtor,
          tiz_prc_buffers_ready, oggdmux_prc_buffers_ready,
          tiz_srv_allocate_resources, oggdmux_prc_allocate_resources,
          tiz_srv_deallocate_resources, oggdmux_prc_deallocate_resources,
          tiz_srv_prepare_to_transfer, oggdmux_prc_prepare_to_transfer,
          tiz_srv_transfer_and_process, oggdmux_prc_transfer_and_process,
          tiz_srv_stop_and_return, oggdmux_prc_stop_and_return,
          tiz_prc_port_flush, oggdmux_prc_port_flush,
          tiz_prc_port_disable, oggdmux_prc_port_disable,
          tiz_prc_port_enable, oggdmux_prc_port_enable, 0));
    }
  return OMX_ErrorNone;
}
