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
 * @file   tizosaluuid.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Universally unique identifier utility functions
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <uuid/uuid.h>
#include <string.h>

#include "OMX_Core.h"

#include "tizosaluuid.h"
#include "tizosalmem.h"

void
tiz_uuid_generate (OMX_UUIDTYPE * ap_uuid)
{
  uuid_t uid =
    { '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0'
  };

  assert (NULL != ap_uuid);

  /* Khronos uuid type is 128 bytes length !! */
  (void) tiz_mem_set (ap_uuid, 0, 128);

  uuid_generate (uid);

  /* Khronos uuid type is 128 bytes length.  We only use the 16 first */
  /*   bytes. */
  memcpy (ap_uuid, uid, 16);
}

void
tiz_uuid_copy (OMX_UUIDTYPE * ap_uuid_dst, const OMX_UUIDTYPE * ap_uuid_src)
{
  assert (NULL != ap_uuid_dst);
  assert (NULL != ap_uuid_src);
  assert (ap_uuid_src != (const OMX_UUIDTYPE *) ap_uuid_dst);

  /* Khronos uuid type is 128 bytes length */
  memcpy (ap_uuid_dst, ap_uuid_src, 128);
}

void
tiz_uuid_str (const OMX_U8 * ap_uuid, OMX_STRING ap_str)
{
  uuid_t uid =
    { '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0'
  };

  assert (NULL != ap_uuid);
  assert (NULL != ap_str);

  uuid_clear (uid);

  memcpy (uid, ap_uuid, 16);

  /* Khronos uuid type is 128 bytes length !! */
  (void) tiz_mem_set (ap_str, 0, 128);

  uuid_unparse (uid, ap_str);
}

void
tiz_str_uuid (const char *ap_str, OMX_UUIDTYPE * ap_uuid)
{
  uuid_t uid =
    { '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0'
  };
  int parse_result = 0;

  assert (NULL != ap_str);
  assert (NULL != ap_uuid);

  uuid_clear (uid);

  /* Khronos uuid type is 128 bytes length !! */
  (void) tiz_mem_set (ap_uuid, 0, 128);

  parse_result = uuid_parse (ap_str, uid);
  assert (parse_result == 0);

  memcpy (ap_uuid, uid, 16);
}
