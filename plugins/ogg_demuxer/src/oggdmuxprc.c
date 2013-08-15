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
 * @brief  Tizonia OpenMAX IL - File Reader processor class implementation
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

/* Forward declarations */
static OMX_ERRORTYPE oggdmux_prc_deallocate_resources (void *);

static inline void
delete_file (oggdmux_prc_t *ap_prc)
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
  OMX_ERRORTYPE rc = OMX_ErrorNone;
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

  if (OMX_ErrorNone != (rc = tiz_api_GetParameter
                        (p_krn, tiz_api_get_hdl (ap_prc),
                         OMX_IndexParamContentURI, ap_prc->p_uri_param_)))
    {
      TIZ_LOGN (TIZ_ERROR, tiz_api_get_hdl (ap_prc),
                "[%s] : Error retrieving URI param from port",
                tiz_err_to_str (rc));
      return rc;
    }

  TIZ_LOGN (TIZ_NOTICE, tiz_api_get_hdl (ap_prc), "URI [%s]",
            ap_prc->p_uri_param_->contentURI);

  return OMX_ErrorNone;
}

static int
read_audio_packet (OGGZ * oggz, oggz_packet * zp, long serialno, void * user_data)
{
  ogg_packet * op = &zp->op;
  oggdmux_prc_t *p_prc = user_data;
  assert (NULL != p_prc);

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

  return 0;
}

static int
read_video_packet (OGGZ * oggz, oggz_packet * zp, long serialno, void * user_data)
{
  ogg_packet * op = &zp->op;
  oggdmux_prc_t *p_prc = user_data;
  assert (NULL != p_prc);

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

  return 0;
}

static size_t
io_read (void * user_handle, void * buf, size_t n)
{
  FILE * f = (FILE *)user_handle;
  return read (fileno(f), buf, n);
}

static int
io_seek (void * user_handle, long offset, int whence)
{
  FILE * f = (FILE *)user_handle;
  return (fseek (f, offset, whence));
}

static long
io_tell (void * user_handle)
{
  FILE * f = (FILE *)user_handle;
  return ftell (f);
}

static int
read_page_first_pass (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  oggdmux_prc_t *p_prc = user_data;
  assert (NULL != p_prc);
  oggz_table_insert (p_prc->p_tracks_, serialno,
                     &read_page_first_pass); /* NULL makes it barf, needs
                                                something */
  if (ogg_page_bos ((ogg_page *)og))
    {
      return 0;
    }
  else
    {
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
}

static OMX_ERRORTYPE
transform_buffer (const void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * oggdmuxprc
 */

static void *
oggdmux_prc_ctor (void *ap_obj, va_list * app)
{
  oggdmux_prc_t *p_prc = super_ctor (oggdmuxprc, ap_obj, app);
  p_prc->p_file_       = NULL;
  p_prc->p_uri_param_  = NULL;
  p_prc->p_oggz_       = NULL;
  p_prc->p_tracks_     = NULL;
  p_prc->pouthdr_      = NULL;
  p_prc->eos_          = false;
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

  oggz_io_set_read (p_prc->p_oggz_, io_read, p_prc->p_file_);
  oggz_io_set_seek (p_prc->p_oggz_, io_seek, p_prc->p_file_);
  oggz_io_set_tell (p_prc->p_oggz_, io_tell, p_prc->p_file_);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggdmux_prc_deallocate_resources (void *ap_obj)
{
  oggdmux_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  delete_file (p_prc);
  delete_oggz (p_prc);
  return OMX_ErrorNone;
}

static void
print_codec_name (oggdmux_prc_t *ap_prc, long serialno)
{
  assert (NULL != ap_prc);
  OggzStreamContent content = oggz_stream_get_content(ap_prc->p_oggz_, serialno);
  const char *codec_name = oggz_content_type (content);

  if (!codec_name)
    {
      codec_name = "unknown";
    }
  TIZ_LOGN (TIZ_TRACE, tiz_api_get_hdl (ap_prc), "Found codec [%s]", codec_name);
}

static void
set_read_callbacks (oggdmux_prc_t *ap_prc)
{
  long serialno = 0;
  int n = 0;
  int i = 0;
  assert (NULL != ap_prc);

  oggz_table_size (ap_prc->p_tracks_);
  for (i = 0; i < n; i++)
    {
      oggz_table_nth (ap_prc->p_tracks_, i, &serialno);
      print_codec_name (ap_prc, serialno);
      if (0 == i)
        {
          oggz_set_read_callback (ap_prc->p_oggz_, -1, read_audio_packet, ap_prc);
        }
      if (1 == i)
        {
          oggz_set_read_callback (ap_prc->p_oggz_, -1, read_video_packet, ap_prc);
        }
    }
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
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
oggdmux_prc_buffers_ready (const void *ap_obj)
{
  return transform_buffer (ap_obj);
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
          tiz_srv_stop_and_return, oggdmux_prc_stop_and_return, 0));
    }
  return OMX_ErrorNone;
}
