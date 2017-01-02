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
 * @file   sdlivrprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - SDL Video Renderer processor class decls
 *
 *
 */

#ifndef SDLIVRPRC_DECLS_H
#define SDLIVRPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <SDL.h>

#include <tizprc_decls.h>

  typedef struct sdlivr_prc sdlivr_prc_t;
  struct sdlivr_prc
  {
    /* Object */
    const tiz_prc_t _;
    OMX_BUFFERHEADERTYPE *pinhdr_;
    OMX_BUFFERHEADERTYPE *pouthdr_;
    OMX_VIDEO_PORTDEFINITIONTYPE vportdef_;
    SDL_Surface *p_surface;
    SDL_Overlay *p_overlay;
    bool eos_;
  };

  typedef struct sdlivr_prc_class sdlivr_prc_class_t;
  struct sdlivr_prc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* SDLIVRPRC_DECLS_H */
