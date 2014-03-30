/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   tizutils.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Utilities
 *
 *
 */

#ifndef TIZUTILS_H
#define TIZUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Types.h>
#include <OMX_Core.h>

#include "tizfsm.h"

/** \addtogroup deprecated
 *  @{
 */

typedef OMX_U32 _tiz_pd_mask;

/* Number of port indexes that can fit in an `tiz_pd_set'.  */
#define _TIZ_PD_SETSIZE 64
#define _TIZ_NPDBITS (8 * (int)sizeof(_tiz_pd_mask))
#define _TIZ_PDELT(d) ((d) / _TIZ_NPDBITS)
#define _TIZ_PDMASK(d) ((_tiz_pd_mask)1 << ((d) % _TIZ_NPDBITS))

typedef struct tiz_pd_set tiz_pd_set_t;

struct tiz_pd_set
{
  _tiz_pd_mask a_pds_bits[_TIZ_PD_SETSIZE / _TIZ_NPDBITS];
#define TIZ_PDS_BITS(set) ((set)->a_pds_bits)
};

#define TIZ_PD_ZERO(set)                                                      \
  do                                                                          \
    {                                                                         \
      unsigned int a_i;                                                       \
      tiz_pd_set_t *a_arr = (set);                                            \
      for (a_i = 0;                                                           \
           a_i < (unsigned int)(sizeof(tiz_pd_set_t) / sizeof(_tiz_pd_mask)); \
           ++a_i)                                                             \
        TIZ_PDS_BITS (a_arr)[a_i] = 0;                                        \
    }                                                                         \
  while (0)

#define TIZ_PD_SET(d, set) \
  ((void)(TIZ_PDS_BITS (set)[_TIZ_PDELT (d)] |= _TIZ_PDMASK (d)))
#define TIZ_PD_CLR(d, set) \
  ((void)(TIZ_PDS_BITS (set)[_TIZ_PDELT (d)] &= ~_TIZ_PDMASK (d)))
#define TIZ_PD_ISSET(d, set) \
  ((TIZ_PDS_BITS (set)[_TIZ_PDELT (d)] & _TIZ_PDMASK (d)) != 0)

/** @}*/

/* ---------------------------------- */
/* libtizonia-specific logging macros */
/* ---------------------------------- */
#define TIZ_CNAME(hdl) (((OMX_COMPONENTTYPE *)hdl)->pComponentPrivate)
#define TIZ_CBUF(hdl) \
  (((OMX_COMPONENTTYPE *)hdl)->pComponentPrivate + OMX_MAX_STRINGNAME_SIZE)

#define TIZ_LOGN(priority, hdl, format, args...)                              \
  tiz_log (__FILE__, __LINE__, __FUNCTION__, TIZ_LOG_CATEGORY_NAME, priority, \
           TIZ_CNAME (hdl), TIZ_CBUF (hdl), format, ##args);

#define TIZ_ERROR(hdl, format, args...)                                 \
  tiz_log (__FILE__, __LINE__, __FUNCTION__, TIZ_LOG_CATEGORY_NAME,     \
           TIZ_PRIORITY_ERROR, TIZ_CNAME (hdl), TIZ_CBUF (hdl), format, \
           ##args);

#define TIZ_WARN(hdl, format, args...)                                 \
  tiz_log (__FILE__, __LINE__, __FUNCTION__, TIZ_LOG_CATEGORY_NAME,    \
           TIZ_PRIORITY_WARN, TIZ_CNAME (hdl), TIZ_CBUF (hdl), format, \
           ##args);

#define TIZ_NOTICE(hdl, format, args...)                                 \
  tiz_log (__FILE__, __LINE__, __FUNCTION__, TIZ_LOG_CATEGORY_NAME,      \
           TIZ_PRIORITY_NOTICE, TIZ_CNAME (hdl), TIZ_CBUF (hdl), format, \
           ##args);

#define TIZ_DEBUG(hdl, format, args...)                                 \
  tiz_log (__FILE__, __LINE__, __FUNCTION__, TIZ_LOG_CATEGORY_NAME,     \
           TIZ_PRIORITY_DEBUG, TIZ_CNAME (hdl), TIZ_CBUF (hdl), format, \
           ##args);

#define TIZ_TRACE(hdl, format, args...)                                 \
  tiz_log (__FILE__, __LINE__, __FUNCTION__, TIZ_LOG_CATEGORY_NAME,     \
           TIZ_PRIORITY_TRACE, TIZ_CNAME (hdl), TIZ_CBUF (hdl), format, \
           ##args);

void tiz_clear_header (OMX_BUFFERHEADERTYPE *ap_hdr);

const OMX_STRING tiz_fsm_state_to_str (tiz_fsm_state_id_t a_id);

#ifdef __cplusplus
}
#endif

#endif /* TIZUTILS_H */
