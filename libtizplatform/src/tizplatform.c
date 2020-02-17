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
 * @file   tizplatform.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia Platform - Initialization and finalization functions
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* #include "tizlog.h" */
/* #include "tizev.h" */

/* TODO */

void __attribute__ ((constructor)) tiz_platform_load (void)
{
  /*   tiz_log_init (); */
  /*   (void) tiz_event_loop_init (); */
}

void __attribute__ ((destructor)) tiz_platform_unload (void)
{
  /*   (void) tiz_event_loop_destroy (); */
  /*   tiz_log_deinit (); */
}
