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

#define TIZ_OGG_DEMUXER_READ_BLOCKSIZE 16384
#define TIZ_OGG_DEMUXER_DEFAULT_BUFFER_UTILISATION .75
/* Forward declarations */
static OMX_ERRORTYPE oggdmux_prc_deallocate_resources (void *);

static inline void
close_file (oggdmux_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  if (NULL != ap_prc->p_file_)
    {
      fclose (ap_prc->p_file_);
      ap_prc->p_file_ = NULL;
    }
}

static inline void
delete_oggz (oggdmux_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  tiz_mem_free (ap_prc->p_oggz_);
  ap_prc->p_oggz_ = NULL;
  tiz_mem_free (ap_prc->p_tracks_);
  ap_prc->p_tracks_ = NULL;
}

static inline void
delete_uri (oggdmux_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  tiz_mem_free (ap_prc->p_uri_param_);
  ap_prc->p_uri_param_ = NULL;
}

static OMX_ERRORTYPE
obtain_uri (oggdmux_prc_t *ap_prc)
{
  void *p_krn = tiz_get_krn (tiz_api_get_hdl (ap_prc));
  assert (NULL != ap_prc);
  assert (NULL == ap_prc->p_uri_param_);

  ap_prc->p_uri_param_ = tiz_mem_calloc
    (1, sizeof (OMX_PARAM_CONTENTURITYPE) + OMX_MAX_STRINGNAME_SIZE);

  if (NULL == ap_prc->p_uri_param_)
    {
      TIZ_LOGN (TIZ_ERROR, tiz_api_get_hdl (ap_prc),
                "Error allocating memory for the content uri struct");
      return OMX_ErrorInsufficientResources;
    }

  ap_prc->p_uri_param_->nSize = sizeof (OMX_PARAM_CONTENTURITYPE)
    + OMX_MAX_STRINGNAME_SIZE - 1;
  ap_prc->p_uri_param_->nVersion.nVersion = OMX_VERSION;

  tiz_check_omx_err
    (tiz_api_GetParameter (p_krn, tiz_api_get_hdl (ap_prc),
                           OMX_IndexParamContentURI, ap_prc->p_uri_param_));

  TIZ_LOGN (TIZ_NOTICE, tiz_api_get_hdl (ap_prc), "URI [%s]",
            ap_prc->p_uri_param_->contentURI);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
alloc_temp_data_stores (oggdmux_prc_t *ap_prc)
{
  void *p_krn = tiz_get_krn (tiz_api_get_hdl (ap_prc));
  OMX_PARAM_PORTDEFINITIONTYPE port_def;

  assert (NULL != ap_prc);

  port_def.nSize             = (OMX_U32) sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  port_def.nVersion.nVersion = OMX_VERSION;
  port_def.nPortIndex        = ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX;

  tiz_check_omx_err
    (tiz_api_GetParameter (p_krn, tiz_api_get_hdl (ap_prc),
                           OMX_IndexParamPortDefinition, &port_def));

  assert (ap_prc->p_aud_store_ == NULL);
  tiz_check_null_ret_oom ((ap_prc->p_aud_store_ = tiz_mem_alloc (port_def.nBufferSize)));
  ap_prc->aud_store_size_ = port_def.nBufferSize;

  port_def.nPortIndex = ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX;
  tiz_check_omx_err
    (tiz_api_GetParameter (p_krn, tiz_api_get_hdl (ap_prc),
                           OMX_IndexParamPortDefinition, &port_def));

  assert (ap_prc->p_vid_store_ == NULL);
  tiz_check_null_ret_oom ((ap_prc->p_vid_store_ = tiz_mem_alloc (port_def.nBufferSize)));
  ap_prc->vid_store_size_ = port_def.nBufferSize;

  return OMX_ErrorNone;
}

static inline void
dealloc_temp_data_stores (oggdmux_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  tiz_mem_free (ap_prc->p_aud_store_);
  tiz_mem_free (ap_prc->p_vid_store_);
  ap_prc->p_aud_store_      = NULL;
  ap_prc->p_vid_store_      = NULL;
  ap_prc->aud_store_offset_ = 0;
  ap_prc->vid_store_offset_ = 0;
}

static void
store_packet (oggdmux_prc_t *ap_prc, const OMX_U32 a_pid,
              const ogg_packet * ap_ogg_packet)
{
  OMX_U8 *p_store = NULL;
  OMX_U32 *p_offset = NULL;

  assert (NULL != ap_prc);
  assert (NULL != ap_ogg_packet);
  assert (a_pid <= ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);

  p_store  = a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
    ? ap_prc->p_aud_store_ : ap_prc->p_vid_store_;
  p_offset = a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
    ? &ap_prc->aud_store_offset_ : &ap_prc->vid_store_offset_;

  assert (NULL != p_store);
  assert (NULL != p_offset);
  
  memcpy (p_store + *p_offset, ap_ogg_packet->packet, ap_ogg_packet->bytes);
  *p_offset += ap_ogg_packet->bytes;
}

static void
dump_into_omx_buffer (oggdmux_prc_t *ap_prc,
                      const OMX_U32  a_pid,
                      const ogg_packet * ap_ogg_packet,
                      OMX_BUFFERHEADERTYPE *ap_hdr)
{
  OMX_U8  *p_store  = NULL;
  OMX_U32 *p_offset = NULL;

  assert (NULL != ap_prc);
  assert (NULL != ap_ogg_packet);
  assert (a_pid <= ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);
  assert (NULL != ap_hdr);

  p_store  = a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
    ? ap_prc->p_aud_store_ : ap_prc->p_vid_store_;
  p_offset = a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
    ? &ap_prc->aud_store_offset_ : &ap_prc->vid_store_offset_;

  assert (NULL != p_store);
  assert (NULL != p_offset);

  if (0 != *p_offset)
    {
      memcpy (ap_hdr->pBuffer + ap_hdr->nFilledLen, p_store, *p_offset);
      ap_hdr->nFilledLen += *p_offset;
      *p_offset = 0;
    }
  memcpy (ap_hdr->pBuffer + ap_hdr->nFilledLen,
          ap_ogg_packet->packet, ap_ogg_packet->bytes);
  ap_hdr->nFilledLen += ap_ogg_packet->bytes;
}

static bool
enough_room_in_buffer (oggdmux_prc_t *ap_prc,
                       const OMX_BUFFERHEADERTYPE *ap_hdr,
                       const OMX_U32 a_pid)
{
  OMX_U32 *p_store_size = NULL;

  assert (NULL != ap_prc);
  assert (a_pid <= ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);
  assert (NULL != ap_hdr);

  p_store_size = a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
    ? &(ap_prc->aud_store_size_) : &(ap_prc->vid_store_size_);

  assert (NULL != p_store_size);
  return (ap_hdr->nFilledLen
          < (*p_store_size * TIZ_OGG_DEMUXER_DEFAULT_BUFFER_UTILISATION));
}

static OMX_BUFFERHEADERTYPE *
buffer_needed (oggdmux_prc_t *p_obj, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  assert (NULL != p_obj);
  assert (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
          || a_pid == ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);

  pp_hdr = (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
            ? &(p_obj->p_aud_hdr_) : &(p_obj->p_vid_hdr_));
  assert (NULL != pp_hdr);

  if (false == p_obj->port_disabled_)
    {
      if (NULL != *pp_hdr)
        {
          TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_obj),
                    "HEADER [%p] exists...nFilledLen [%d] pid [%d]",
                    *pp_hdr, (*pp_hdr)->nFilledLen, a_pid);
          return *pp_hdr;
        }
      else
        {
          tiz_pd_set_t ports;
          void *p_krn = NULL;

          p_krn = tiz_get_krn (tiz_api_get_hdl (p_obj));

          TIZ_PD_ZERO (&ports);
          if (OMX_ErrorNone == tiz_krn_select (p_krn, 2, &ports))
            {
              if (TIZ_PD_ISSET (a_pid, &ports))
                {
                  if (OMX_ErrorNone == tiz_krn_claim_buffer
                      (p_krn, a_pid, 0, pp_hdr))
                    {
                      TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_obj),
                                "Claimed audio HEADER [%p]...nFilledLen [%d] "
                                "a_pid [%d]", *pp_hdr, (*pp_hdr)->nFilledLen, a_pid);
                      return *pp_hdr;
                    }
                }
            }
        }
    }

  p_obj->awaiting_buffers_ = true;
  return NULL;
}

static void
buffer_filled (oggdmux_prc_t *p_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  OMX_BUFFERHEADERTYPE  *p_hdr  = NULL;

  assert (NULL != p_prc);
  assert (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
          || a_pid == ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);

  pp_hdr = (a_pid == ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX
            ? &(p_prc->p_aud_hdr_) : &(p_prc->p_vid_hdr_));
  assert (NULL != pp_hdr && NULL != *pp_hdr);

  p_hdr = *pp_hdr;

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc), "Releasing HEADER [%p] "
            "nFilledLen [%d] pid [%d]", p_hdr, p_hdr->nFilledLen, a_pid);

  p_hdr->nOffset = 0;

  if ((p_hdr->nFlags & OMX_BUFFERFLAG_EOS) != 0)
    {
      TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc),
      "OMX_BUFFERFLAG_EOS in HEADER [%p]", p_hdr);
      p_prc->eos_ = false;
      tiz_srv_issue_event ((OMX_PTR) p_prc,
                           OMX_EventBufferFlag, a_pid, p_hdr->nFlags, NULL);
    }

  tiz_krn_release_buffer (tiz_get_krn (tiz_api_get_hdl (p_prc)), a_pid, p_hdr);
  *pp_hdr = NULL;
}

static int
read_packet (OGGZ * oggz, oggz_packet * zp, long serialno,
             void * user_data, const OMX_U32 a_pid)
{
  ogg_packet *   op    = &zp->op;
  oggdmux_prc_t *p_prc = user_data;
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  assert (NULL != p_prc);

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc),
            "%010lu: bytes [%d]", serialno, op->bytes);

  if (NULL == (p_hdr = buffer_needed (p_prc, a_pid)))
    {
      /* Stop until we have some OMX buffers available */
      TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc),
                "%010lu: no OMX buffers available", serialno);
      /* Temporarily store the data until an omx buffer is ready */
      store_packet (p_prc, a_pid, op);
      return OGGZ_STOP_OK;
    }

#if 0
  if (got_an_eos)
    {
      printf ("[%010lu]\t%ld bytes\tgranulepos %ld\n", serialno, op->bytes,
              (long)op->granulepos);
    }
#endif

  if (op->b_o_s)
  {
    TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc),
              "%010lu: [%" PRId64 "] BOS %8s\n",
              serialno, op->granulepos, op->packet);
  }

  if (op->e_o_s)
  {
    p_prc->eos_ = true;
    TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc),
              "%010lu: [%" PRId64 "] EOS\n",
              serialno, op->granulepos);
  }

  dump_into_omx_buffer (p_prc, a_pid, op, p_hdr);

  if (p_prc->eos_)
    {
      p_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
    }

  if (p_prc->eos_ || !enough_room_in_buffer (p_prc, p_hdr, a_pid))
    {
      buffer_filled (p_prc, a_pid);

      if (NULL == (p_hdr = buffer_needed (p_prc, a_pid)))
        {
          TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc),
                    "%010lu: Stop until we get more OMX buffers", serialno);
          return OGGZ_STOP_OK;
        }
    }

  return OGGZ_CONTINUE;
}

static int
read_audio_packet (OGGZ * oggz, oggz_packet * zp, long serialno, void * user_data)
{
  return read_packet (oggz, zp, serialno, user_data,
                      ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX);
}

static int
read_video_packet (OGGZ * oggz, oggz_packet * zp, long serialno, void * user_data)
{

  return read_packet (oggz, zp, serialno, user_data,
                      ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX);
}

static OMX_ERRORTYPE
release_buffers (oggdmux_prc_t *ap_prc)
{
  assert (NULL != ap_prc);

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_prc), "release_buffers");

  if (NULL != ap_prc->p_aud_hdr_)
    {
      void *p_krn = tiz_get_krn (tiz_api_get_hdl (ap_prc));
      tiz_check_omx_err
        (tiz_krn_release_buffer (p_krn,
                                 ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX,
                                 ap_prc->p_aud_hdr_));
      ap_prc->p_aud_hdr_ = NULL;
    }

  if (NULL != ap_prc->p_vid_hdr_)
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
do_flush (oggdmux_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_prc), "do_flush");
  (void) oggz_purge (ap_prc->p_oggz_);
  ap_prc->aud_store_offset_ = 0;
  ap_prc->vid_store_offset_ = 0;
  /* Release any buffers held  */
  return release_buffers (ap_prc);
}

static size_t
io_read (void * user_handle, void * buf, size_t n)
{
  oggdmux_prc_t *p_prc = user_handle;
  FILE * f = NULL;
  ssize_t  bytes_read = 0;

  assert (NULL != p_prc);
  f = p_prc->p_file_;

  bytes_read = read (fileno(f), buf, n);
  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc),
            "bytes_read [%d] buf [%p] n [%d]", bytes_read, buf , n);
  if(0 == bytes_read)
    {
      p_prc->eos_ = true;
    }
  return bytes_read;
}

static int
io_seek (void * user_handle, long offset, int whence)
{
  oggdmux_prc_t *p_prc = user_handle;
  FILE * f = NULL;
  assert (NULL != p_prc);
  f = p_prc->p_file_;
  return (fseek (f, offset, whence));
}

static long
io_tell (void * user_handle)
{
  oggdmux_prc_t *p_prc = user_handle;
  FILE * f = NULL;
  assert (NULL != p_prc);
  f = p_prc->p_file_;
  return ftell (f);
}

static int
read_page (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  return 0;
}

static int
read_page_first_pass (OGGZ * oggz, const ogg_page * og,
                      long serialno, void * user_data)
{
  oggdmux_prc_t *p_prc = user_data;
  assert (NULL != p_prc);

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc), "serialno = [%d]", serialno);

  if (ogg_page_bos ((ogg_page *)og))
    {
      return 0;
    }
  else
    {
      oggz_table_insert (p_prc->p_tracks_, serialno,
                         &read_page_first_pass); /* NULL makes it barf, needs
                                                    something */
      return OGGZ_STOP_OK;
    }
}

static void
obtain_tracks (oggdmux_prc_t *ap_prc)
{
  oggdmux_prc_t *p_prc = ap_prc;
  long n = 0;
  assert (NULL != p_prc);
  oggz_seek (p_prc->p_oggz_, 0, SEEK_SET);
  oggz_set_read_page (p_prc->p_oggz_, -1, read_page_first_pass, p_prc);
  while ((n = oggz_read (p_prc->p_oggz_, TIZ_OGG_DEMUXER_READ_BLOCKSIZE)) > 0);
  oggz_seek (p_prc->p_oggz_, 0, SEEK_SET);
  oggz_set_read_page (p_prc->p_oggz_, -1, read_page, p_prc);
}

static void
print_codec_name (oggdmux_prc_t *ap_prc, long serialno)
{
  assert (NULL != ap_prc);
  OggzStreamContent content = oggz_stream_get_content(ap_prc->p_oggz_,
                                                      serialno);
  const char *codec_name = oggz_content_type (content);

  if (!codec_name)
    {
      codec_name = "unknown";
    }
  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_prc), "Found codec [%s]",
            codec_name);
}

static void
set_read_callbacks (oggdmux_prc_t *ap_prc)
{
  long serialno = 0;
  int n = 0;
  int i = 0;
  assert (NULL != ap_prc);

  n = oggz_table_size (ap_prc->p_tracks_);
  for (i = 0; i < n; i++)
    {
      oggz_table_nth (ap_prc->p_tracks_, i, &serialno);
      print_codec_name (ap_prc, serialno);
      if (0 == i)
        {
          TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_prc),
                    "Set read_audio_packet callback");
          oggz_set_read_callback (ap_prc->p_oggz_, -1,
                                  read_audio_packet, ap_prc);
        }
      if (1 == i)
        {
          TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_prc),
                    "Set read_video_packet callback");
          oggz_set_read_callback (ap_prc->p_oggz_, -1,
                                  read_video_packet, ap_prc);
        }
    }
}

static OMX_ERRORTYPE
demux_file (oggdmux_prc_t *ap_prc)
{
  int run_status = 0;
  assert (NULL != ap_prc);
  run_status     = oggz_run (ap_prc->p_oggz_);

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_prc), "run_status [%d]", run_status);

  if (ap_prc->eos_)
    {
      buffer_needed (ap_prc, 0);
      if (ap_prc->p_aud_hdr_)
        {
          ap_prc->p_aud_hdr_->nFlags |= OMX_BUFFERFLAG_EOS;
          buffer_filled (ap_prc, 0);
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
  oggdmux_prc_t *p_prc     = super_ctor (oggdmuxprc, ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_file_           = NULL;
  p_prc->p_uri_param_      = NULL;
  p_prc->p_oggz_           = NULL;
  p_prc->p_tracks_         = NULL;
  p_prc->p_aud_hdr_        = NULL;
  p_prc->p_vid_hdr_        = NULL;
  p_prc->eos_              = false;
  p_prc->awaiting_buffers_ = true;
  p_prc->port_disabled_    = false;
  p_prc->p_aud_store_      = NULL;
  p_prc->p_vid_store_      = NULL;
  p_prc->aud_store_offset_ = 0;
  p_prc->vid_store_offset_ = 0;
  p_prc->aud_store_size_   = 0;
  p_prc->vid_store_size_   = 0;
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

  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc),
            "Allocating resources");

  tiz_check_omx_err (alloc_temp_data_stores (p_prc));
  
  if ((p_prc->p_file_
       = fopen ((const char *) p_prc->p_uri_param_->contentURI, "r")) == 0)
    {
      TIZ_LOGN (TIZ_ERROR, tiz_api_get_hdl (p_prc),
                "Error opening file from URI (%s)", strerror (errno));
      return OMX_ErrorInsufficientResources;
    }

  if (NULL == (p_prc->p_oggz_ = oggz_new (OGGZ_READ|OGGZ_AUTO)))
    {
      TIZ_LOGN (TIZ_ERROR, tiz_api_get_hdl (p_prc),
                "Cannot create a new oggz object (%s)", strerror (errno));
      return OMX_ErrorInsufficientResources;
    }

  if (NULL == (p_prc->p_tracks_ = oggz_table_new ()))
    {
      TIZ_LOGN (TIZ_ERROR, tiz_api_get_hdl (p_prc),
                "Cannot create a new oggz object");
      return OMX_ErrorInsufficientResources;
    }

  oggz_io_set_read (p_prc->p_oggz_, io_read, p_prc);
  oggz_io_set_seek (p_prc->p_oggz_, io_seek, p_prc);
  oggz_io_set_tell (p_prc->p_oggz_, io_tell, p_prc);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggdmux_prc_deallocate_resources (void *ap_obj)
{
  oggdmux_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc),
            "Deallocating resources");
  close_file (p_prc);
  delete_oggz (p_prc);
  dealloc_temp_data_stores (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggdmux_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  oggdmux_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  obtain_tracks (p_prc);
  set_read_callbacks (p_prc);
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
  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc),
            "stop_and_return");
  return do_flush (p_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
oggdmux_prc_buffers_ready (const void *ap_obj)
{
  oggdmux_prc_t *p_prc = (oggdmux_prc_t *) ap_obj;
  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc),
            "Received buffer ready notification - "
            "awaiting_buffers [%s] eos [%s]",
            p_prc->awaiting_buffers_ ? "YES" : "NO",
            p_prc->eos_ ? "YES" : "NO");
  if (p_prc->awaiting_buffers_ && !p_prc->eos_)
    {
      p_prc->awaiting_buffers_ = false;
      return demux_file (p_prc);
    }
  else if (p_prc->awaiting_buffers_  && p_prc->eos_)
    {
      OMX_BUFFERHEADERTYPE *p_hdr = buffer_needed (p_prc, 0);
      p_prc->awaiting_buffers_ = false;
      if (p_prc->p_aud_hdr_)
        {
          p_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
          buffer_filled (p_prc, 0);
        }
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggdmux_prc_port_flush (const void *ap_obj, OMX_U32 TIZ_UNUSED(a_pid))
{
  oggdmux_prc_t *p_prc = (oggdmux_prc_t *) ap_obj;
  assert (NULL != p_prc);
  return do_flush (p_prc);
}

static OMX_ERRORTYPE
oggdmux_prc_port_disable (const void *ap_obj, OMX_U32 TIZ_UNUSED(a_pid))
{
  oggdmux_prc_t *p_prc = (oggdmux_prc_t *) ap_obj;
  assert (NULL != p_prc);
  /* Release any buffers held  */
  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (p_prc), "port_disable");
  return release_buffers ((oggdmux_prc_t *) ap_obj);
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
          tiz_prc_port_enable, oggdmux_prc_port_enable,
          0));
    }
  return OMX_ErrorNone;
}
