/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * Tizonia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   mp3dprc.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Mp3 Decoder processor class
 *
 *
 */

#ifndef MP3DPRC_H
#define MP3DPRC_H

#ifdef __cplusplus
extern "C" {
#endif

void *
mp3d_prc_class_init (void * ap_tos, void * ap_hdl);
void *
mp3d_prc_init (void * ap_tos, void * ap_hdl);

#ifdef __cplusplus
}
#endif

#endif /* MP3DPRC_H */
