/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   tiztcproc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - test component processor class declarations
 *
 *
 */

#ifndef TIZTCPROC_DECLS_H
#define TIZTCPROC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tiztcproc.h"
#include "tizprc_decls.h"

  typedef struct tiz_tcprc tiz_tcprc_t;
  struct tiz_tcprc
  {
    /* Object */
    const tiz_prc_t _;

  };

  typedef struct tiz_tcprc_class tiz_tcprc_class_t;
  struct tiz_tcprc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* TIZTCPROC_DECLS_H */
