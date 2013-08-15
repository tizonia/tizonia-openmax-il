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
 * @file   frprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - File Reader processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "frprc.h"
#include "frprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizosal.h"

#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.file_reader.prc"
#endif

/*
 * frprc
 */

static void *
fr_prc_ctor (void *ap_obj, va_list * app)
{
  fr_prc_t *p_obj = super_ctor (frprc, ap_obj, app);
  p_obj->eos_ = false;
  return p_obj;
}

static void *
fr_prc_dtor (void *ap_obj)
{
  return super_dtor (frprc, ap_obj);
}

static OMX_ERRORTYPE
fr_prc_read_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
{
  return OMX_ErrorNone;
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
fr_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
fr_prc_buffers_ready (const void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * initialization
 */

const void *frprc;

void
init_frprc (void)
{
  if (!frprc)
    {
      tiz_prc_init ();
      frprc =
        factory_new
        (tizprc_class,
         "frprc",
         tizprc,
         sizeof (fr_prc_t),
         ctor, fr_prc_ctor,
         dtor, fr_prc_dtor,
         tiz_prc_buffers_ready, fr_prc_buffers_ready,
         tiz_srv_allocate_resources, fr_prc_allocate_resources,
         tiz_srv_deallocate_resources, fr_prc_deallocate_resources,
         tiz_srv_prepare_to_transfer, fr_prc_prepare_to_transfer,
         tiz_srv_transfer_and_process, fr_prc_transfer_and_process,
         tiz_srv_stop_and_return, fr_prc_stop_and_return, 0);
    }
}
