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
 * @file   inprocrndprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - ZMQ inproc socket writer processor
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "inprocrnd.h"
#include "inprocrndprc.h"
#include "inprocrndprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.inproc_writer.prc"
#endif

#define goto_end_on_zmq_null_pointer(expr, prc, msg)  \
  do                                                  \
    {                                                 \
      if (NULL == (expr))                             \
        {                                             \
          TIZ_ERROR (handleOf (prc), "%s (%s)", msg); \
          goto end;                                   \
        }                                             \
    }                                                 \
  while (0)

#define goto_end_on_zmq_error(expr, prc, msg)         \
  do                                                  \
    {                                                 \
      if (0 != (expr))                                \
        {                                             \
          TIZ_ERROR (handleOf (prc), "%s (%s)", msg); \
          goto end;                                   \
        }                                             \
    }                                                 \
  while (0)

static OMX_BUFFERHEADERTYPE *get_header (inprocrnd_prc_t *ap_prc)
{
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  assert (ap_prc);

  if (!ap_prc->port_disabled_)
    {
      if (!ap_prc->p_inhdr_)
        {
          (void)tiz_krn_claim_buffer (tiz_get_krn (handleOf (ap_prc)),
                                      ARATELIA_INPROC_WRITER_PORT_INDEX, 0,
                                      &ap_prc->p_inhdr_);
          if (ap_prc->p_inhdr_)
            {
              TIZ_TRACE (handleOf (ap_prc),
                         "Claimed HEADER [%p]...nFilledLen [%d]",
                         ap_prc->p_inhdr_, ap_prc->p_inhdr_->nFilledLen);
            }
        }
      p_hdr = ap_prc->p_inhdr_;
    }
  return p_hdr;
}

static bool ready_to_process (inprocrnd_prc_t *ap_prc)
{
  assert (ap_prc);
  TIZ_TRACE (handleOf (ap_prc), "paused [%s] port disabled [%s] stopped [%s]",
             ap_prc->paused_ ? "YES" : "NO",
             ap_prc->port_disabled_ ? "YES" : "NO",
             ap_prc->stopped_ ? "YES" : "NO");
  return (!ap_prc->paused_ && !ap_prc->port_disabled_ && !ap_prc->stopped_
          && get_header (ap_prc));
}

static bool ready_to_write_to_zmq_sock (inprocrnd_prc_t *ap_prc)
{
  int zmq_rc = 0;
  int zevents = 0;
  size_t zevents_len = sizeof(zevents);
  bool sock_ready = false;
  assert (ap_prc);

  if (ap_prc->p_zmq_sock_)
    {
      zmq_rc = zmq_getsockopt (ap_prc->p_zmq_sock_, ZMQ_EVENTS, &zevents,
                               &zevents_len);
      if (!zmq_rc)
        {
          if (zevents & ZMQ_POLLOUT)
            {
              // We can write to the ZeroMQ socket
              sock_ready = true;
            }
        }
    }
  return sock_ready;
}

static OMX_ERRORTYPE release_header (inprocrnd_prc_t *ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_inhdr_)
    {
      TIZ_TRACE (handleOf (ap_prc), "Releasing HEADER [%p] emptied",
                 ap_prc->p_inhdr_);
      ap_prc->p_inhdr_->nOffset = 0;
      ap_prc->p_inhdr_->nFilledLen = 0;
      tiz_check_omx (tiz_krn_release_buffer (
          tiz_get_krn (handleOf (ap_prc)), ARATELIA_INPROC_WRITER_PORT_INDEX,
          ap_prc->p_inhdr_));
      ap_prc->p_inhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE buffer_emptied (inprocrnd_prc_t *ap_prc)
{
  assert (ap_prc);
  assert (ap_prc->p_inhdr_);
  assert (ap_prc->p_inhdr_->nFilledLen == 0);

  if ((ap_prc->p_inhdr_->nFlags & OMX_BUFFERFLAG_EOS) != 0)
    {
      TIZ_DEBUG (handleOf (ap_prc), "OMX_BUFFERFLAG_EOS in HEADER [%p]",
                 ap_prc->p_inhdr_);
      tiz_srv_issue_event ((OMX_PTR)ap_prc, OMX_EventBufferFlag, 0,
                           ap_prc->p_inhdr_->nFlags, NULL);
    }

  return release_header (ap_prc);
}

static OMX_ERRORTYPE write_buffer (inprocrnd_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  assert (ap_prc);

  while ((p_hdr = get_header (ap_prc)))
    {
      if (p_hdr->nFilledLen > 0)
        {
          const int bytes_to_write = 0;
          p_hdr->nFilledLen -= bytes_to_write;
          p_hdr->nOffset += bytes_to_write;
        }

      if (0 == p_hdr->nFilledLen)
        {
          rc = buffer_emptied (ap_prc);
          p_hdr = NULL;
        }
    }

  return rc;
}

/*
 * inprocrndprc
 */

static void *inprocrnd_prc_ctor (void *ap_prc, va_list *app)
{
  inprocrnd_prc_t *p_prc
      = super_ctor (typeOf (ap_prc, "inprocrndprc"), ap_prc, app);
  p_prc->port_disabled_ = false;
  p_prc->paused_ = false;
  p_prc->stopped_ = true;
  p_prc->p_zmq_ctx_ = NULL;
  p_prc->p_zmq_sock_ = NULL;
  p_prc->zmq_fd_ = -1;
  p_prc->eos_ = false;
  return p_prc;
}

static void *inprocrnd_prc_dtor (void *ap_prc)
{
  return super_dtor (typeOf (ap_prc, "inprocrndprc"), ap_prc);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE inprocrnd_prc_allocate_resources (void *ap_prc,
                                                       OMX_U32 a_pid)
{
  inprocrnd_prc_t *p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  int zmq_rc = 0;
  assert (p_prc);

  /* Create the zmq context */
  p_prc->p_zmq_ctx_ = zmq_ctx_new ();
  goto_end_on_zmq_null_pointer (p_prc->p_zmq_ctx_, p_prc, zmq_strerror (errno));

  /* No need for io threads (since inproc transport) */
  zmq_ctx_set (p_prc->p_zmq_ctx_, ZMQ_IO_THREADS, 0);

  /* Create the zmq PUB socket */
  p_prc->p_zmq_sock_ = zmq_socket (p_prc->p_zmq_ctx_, ZMQ_PUB);
  goto_end_on_zmq_null_pointer (p_prc->p_zmq_sock_, p_prc,
                                zmq_strerror (errno));

  /* Bind the socket to the inproc address */
  zmq_rc = zmq_bind (p_prc->p_zmq_sock_, "inproc://broadcast");
  goto_end_on_zmq_error (zmq_rc, p_prc, zmq_strerror (errno));

  /* All good */
  rc = OMX_ErrorNone;

end:

  return rc;
}

static OMX_ERRORTYPE inprocrnd_prc_deallocate_resources (void *ap_prc)
{
  inprocrnd_prc_t *p_prc = ap_prc;
  assert (p_prc);
  if (p_prc->p_zmq_sock_)
    {
      zmq_close (p_prc->p_zmq_sock_);
      p_prc->p_zmq_sock_ = NULL;
    }
  if (p_prc->p_zmq_ctx_)
    {
      zmq_ctx_shutdown (p_prc->p_zmq_ctx_);
      p_prc->p_zmq_ctx_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE inprocrnd_prc_prepare_to_transfer (void *ap_prc,
                                                        OMX_U32 a_pid)
{
  inprocrnd_prc_t *p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  int zmq_rc = 0;
  size_t fd_len = 0;
  assert (p_prc);

  fd_len = sizeof(p_prc->zmq_fd_);
  zmq_rc
      = zmq_getsockopt (p_prc->p_zmq_sock_, ZMQ_FD, &p_prc->zmq_fd_, &fd_len);
  goto_end_on_zmq_error (zmq_rc, p_prc, zmq_strerror (errno));

  /* All goood */
  rc = OMX_ErrorNone;

end:

  return rc;
}

static OMX_ERRORTYPE inprocrnd_prc_transfer_and_process (void *ap_prc,
                                                         OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE inprocrnd_prc_stop_and_return (void *ap_prc)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE inprocrnd_prc_io_ready (void *ap_prc,
                                             tiz_event_io_t *ap_ev_io, int a_fd,
                                             int a_events)
{
  inprocrnd_prc_t *p_prc = (inprocrnd_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (ready_to_process (p_prc) && ready_to_write_to_zmq_sock (p_prc))
    {
      rc = write_buffer (p_prc);
    }
  return rc;
}

static OMX_ERRORTYPE inprocrnd_prc_buffers_ready (const void *ap_prc)
{
  inprocrnd_prc_t *p_prc = (inprocrnd_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (ready_to_process (p_prc))
    {
      rc = write_buffer (p_prc);
    }
  return rc;
}

/*
 * inprocrnd_prc_class
 */

static void *inprocrnd_prc_class_ctor (void *ap_prc, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_prc, "inprocrndprc_class"), ap_prc, app);
}

/*
 * initialization
 */

void *inprocrnd_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *inprocrndprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizprc), "inprocrndprc_class", classOf (tizprc),
       sizeof(inprocrnd_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, inprocrnd_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return inprocrndprc_class;
}

void *inprocrnd_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *inprocrndprc_class = tiz_get_type (ap_hdl, "inprocrndprc_class");
  TIZ_LOG_CLASS (inprocrndprc_class);
  void *inprocrndprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (inprocrndprc_class, "inprocrndprc", tizprc, sizeof(inprocrnd_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, inprocrnd_prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, inprocrnd_prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, inprocrnd_prc_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, inprocrnd_prc_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, inprocrnd_prc_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, inprocrnd_prc_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, inprocrnd_prc_stop_and_return,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_io_ready, inprocrnd_prc_io_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, inprocrnd_prc_buffers_ready,
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return inprocrndprc;
}
