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
 * @file   cc_prc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL Chromecast renderer - base processor class
 * implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <tizplatform.h>

#include <tizport.h>
#include <tizport_decls.h>
#include <tizport-macros.h>
#include <tizkernel.h>

#include "cc_prc.h"
#include "cc_prc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer.prc""
#endif

/* Forward declaration */

/*
 * filterprc
 */

static void *
cc_prc_ctor (void * ap_obj, va_list * app)
{
  cc_prc_t * p_prc = super_ctor (typeOf (ap_obj, "cc_prc"), ap_obj, app);
  assert (p_prc);
  return p_prc;
}

static void *
cc_prc_dtor (void * ap_obj)
{
  cc_prc_t * p_prc = ap_obj;
  assert (p_prc);
  return super_dtor (typeOf (ap_obj, "cc_prc"), ap_obj);
}

/*
 * cc_prc_class
 */

static void *
cc_prc_class_ctor (void * ap_obj, va_list * app)
{
  cc_prc_class_t * p_obj
    = super_ctor (typeOf (ap_obj, "cc_prc_class"), ap_obj, app);
  typedef void (*voidf) ();
  voidf selector = NULL;
  va_list ap;
  va_copy (ap, *app);

  /* NOTE: Start ignoring splint warnings in this section of code */
  /*@ignore@*/
  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      (void)method;
    }
  /*@end@*/
  /* NOTE: Stop ignoring splint warnings in this section  */

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

void *
cc_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * cc_prc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "cc_prc_class", classOf (tizprc),
     sizeof (cc_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return cc_prc_class;
}

void *
cc_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * cc_prc_class = tiz_get_type (ap_hdl, "cc_prc_class");
  TIZ_LOG_CLASS (cc_prc_class);
  void * filterprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_prc_class, "cc_prc", tizprc, sizeof (cc_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, cc_prc_dtor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return filterprc;
}
