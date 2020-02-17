/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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
 * @file   tizutils.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Utilities
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizutils.h"

typedef struct tiz_state_str tiz_state_str_t;

struct tiz_state_str
{
  tiz_fsm_state_id_t state;
  OMX_STRING str;
};

tiz_state_str_t tiz_fsm_state_to_str_tbl[]
  = {{EStateReserved_0x00000000, "OMX_StateReserved_0x00000000"},
     {EStateLoaded, "OMX_StateLoaded"},
     {EStateIdle, "OMX_StateIdle"},
     {EStateExecuting, "OMX_StateExecuting"},
     {EStatePause, "OMX_StatePause"},
     {EStateWaitForResources, "OMX_StateWaitForResources"},
     {ESubStateLoadedToIdle, "ESubStateLoadedToIdle"},
     {ESubStateIdleToLoaded, "ESubStateIdleToLoaded"},
     {ESubStateIdleToExecuting, "ESubStateIdleToExecuting"},
     {ESubStateExecutingToIdle, "ESubStateExecutingToIdle"},
     {ESubStatePauseToIdle, "ESubStatePauseToIdle"},
     {EStateMax, "EStateMax"}};

void
tiz_clear_header (OMX_BUFFERHEADERTYPE * ap_hdr)
{
  assert (ap_hdr);

  ap_hdr->nFilledLen = 0;
  ap_hdr->nOffset = 0;
  ap_hdr->hMarkTargetComponent = 0;
  ap_hdr->pMarkData = 0;
  ap_hdr->nTickCount = 0;
  ap_hdr->nTimeStamp = 0;
  ap_hdr->nFlags = 0;
}

const OMX_STRING
tiz_fsm_state_to_str (tiz_fsm_state_id_t a_id)
{
  const OMX_S32 count
    = sizeof (tiz_fsm_state_to_str_tbl) / sizeof (tiz_state_str_t);
  OMX_S32 i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_fsm_state_to_str_tbl[i].state == a_id)
        {
          return tiz_fsm_state_to_str_tbl[i].str;
        }
    }

  if (OMX_StateMax == (OMX_STATETYPE) a_id)
    {
      return "OMX_StateMax";
    }

  return "Unknown OpenMAX IL state";
}
