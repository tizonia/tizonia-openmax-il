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
 * @file   inprocrndprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - ZMQ inproc socket writer class declarations
 *
 *
 */

#ifndef INPROCRNDPRC_DECLS_H
#define INPROCRNDPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#include <zmq.h>
#include <zmq_utils.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>

  typedef struct inprocrnd_prc inprocrnd_prc_t;
  struct inprocrnd_prc
  {
    /* Object */
    const tiz_prc_t _;
    OMX_BUFFERHEADERTYPE *p_inhdr_;
    bool port_disabled_;
    bool paused_;
    bool stopped_;
    void * p_zmq_ctx_;
    void * p_zmq_sock_;
    int zmq_fd_;
    bool eos_;
  };

  typedef struct inprocrnd_prc_class inprocrnd_prc_class_t;
  struct inprocrnd_prc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* INPROCRNDPRC_DECLS_H */
