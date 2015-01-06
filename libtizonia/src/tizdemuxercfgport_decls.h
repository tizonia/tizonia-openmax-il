/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizdemuxercfgport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Demuxer config port class decls
 *
 *
 */

#ifndef TIZDEMUXERCFGPORT_DECLS_H
#define TIZDEMUXERCFGPORT_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tizuricfgport_decls.h"
#include "OMX_Types.h"

  typedef struct tiz_demuxercfgport tiz_demuxercfgport_t;
  struct tiz_demuxercfgport
  {
    /* Object */
    const tiz_uricfgport_t _;
  };

  typedef struct tiz_demuxercfgport_class tiz_demuxercfgport_class_t;
  struct tiz_demuxercfgport_class
  {
    /* Class */
    const tiz_uricfgport_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* TIZDEMUXERCFGPORT_DECLS_H */
