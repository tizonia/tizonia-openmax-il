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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizobject.h"
#include "tizobject_decls.h"
#include "tizobjsys.h"
#include "tizutils.h"
#include "tizplatform.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.object"
#endif

/*
 * Object
 */

static void *
object_ctor (void * ap_obj, va_list * app)
{
  return ap_obj;
}

static void *
object_dtor (void * ap_obj)
{
  return ap_obj;
}

const void *
classOf (const void * ap_obj)
{
  const tiz_object_t * p_obj = ap_obj;
  assert (p_obj && p_obj->class);
  return p_obj->class;
}

size_t
sizeOf (const void * ap_obj)
{
  const tiz_class_t * class = classOf (ap_obj);
  return class->size;
}

const char *
nameOf (const void * ap_obj)
{
  const tiz_class_t * class = classOf (ap_obj);
  return class->name;
}

const OMX_HANDLETYPE
handleOf (const void * ap_obj)
{
  const tiz_class_t * class = classOf (ap_obj);
  return (OMX_HANDLETYPE) class->hdl;
}

const void *
typeOf (const void * ap_obj, const char * ap_type_name)
{
  const tiz_class_t * p_class = classOf (ap_obj);
  return tiz_os_get_type (p_class->tos, ap_type_name);
}

void
print_class (const void * ap_class, const char * file, int line,
             const char * func)
{
  const tiz_class_t * p_class = ap_class;
  assert (p_class);
  tiz_log (file, line, func, TIZ_LOG_CATEGORY_NAME, TIZ_PRIORITY_TRACE,
           TIZ_CNAME (p_class->hdl), TIZ_CBUF (p_class->hdl),
           "[%p] - name [%s] - super [%p] - super name [%s] - "
           "size [%d] - tos [%p] - hdl [%p]",
           p_class, p_class->name, p_class->super, p_class->super->name,
           p_class->size, p_class->tos, p_class->hdl);
}

/*
 * Class
 */

#ifdef HAVE_FUNC_ATTRIBUTE_NO_SANITIZE_ADDRESS
ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif
static void *
class_ctor (void * ap_obj, va_list * app)
{
  tiz_class_t * p_obj = ap_obj;
  const size_t offset = offsetof (tiz_class_t, ctor);

  p_obj->name = va_arg (*app, char *);
  p_obj->super = va_arg (*app, tiz_class_t *);
  p_obj->size = va_arg (*app, size_t);
  p_obj->tos = va_arg (*app, void *);
  p_obj->hdl = va_arg (*app, void *);

  assert (p_obj->super);

  memcpy ((char *) p_obj + offset, (char *) p_obj->super + offset,
          sizeOf (p_obj->super) - offset);
  {
    typedef void (*voidf) (); /* generic function pointer */
    voidf selector = NULL;
    va_list ap;
    va_copy (ap, *app);

    while ((selector = va_arg (ap, voidf)))
      {
        voidf method = va_arg (ap, voidf);

        if (selector == (voidf) ctor)
          *(voidf *) &p_obj->ctor = method;
        else if (selector == (voidf) dtor)
          *(voidf *) &p_obj->dtor = method;
      }

    va_end (ap);
    return p_obj;
  }
}

static void *
class_dtor (void * ap_obj)
{
  return NULL;
}

const void *
super (const void * ap_obj)
{
  const tiz_class_t * p_obj = ap_obj;
  assert (p_obj && p_obj->super);
  return p_obj->super;
}

/*
 * initialization
 */

/*
 * object management and selectors
 */

void *
factory_new (const void * ap_class, ...)
{
  const tiz_class_t * p_class = ap_class;
  tiz_object_t * p_obj = NULL;
  va_list ap;

  assert (p_class && p_class->size > 0);
  if ((p_obj = tiz_mem_calloc (1, p_class->size)))
    {
      p_obj->class = p_class;
      va_start (ap, ap_class);
      p_obj = ctor (p_obj, &ap);
      va_end (ap);
    }
  return p_obj;
}

void
factory_delete (void * ap_obj)
{
  if (ap_obj)
    {
      tiz_mem_free (dtor (ap_obj));
    }
}

void *
ctor (void * ap_obj, va_list * app)
{
  const tiz_class_t * p_class = classOf (ap_obj);
  /*   TIZ_LOG_CLASS (p_class); */
  assert (p_class->ctor);
  return p_class->ctor (ap_obj, app);
}

void *
super_ctor (const void * ap_class, void * ap_obj, va_list * app)
{
  const tiz_class_t * p_super = super (ap_class);
  assert (ap_obj && p_super->ctor);
  /*   TIZ_LOG_CLASS (p_super); */
  return p_super->ctor (ap_obj, app);
}

void *
dtor (void * ap_obj)
{
  tiz_class_t * p_class = (tiz_class_t *) classOf (ap_obj);
  assert (p_class && p_class->dtor);
  return p_class->dtor (ap_obj);
}

void *
super_dtor (const void * ap_class, void * ap_obj)
{
  const tiz_class_t * p_super = super (ap_class);
  assert (ap_obj && p_super->dtor);
  return p_super->dtor (ap_obj);
}

static const tiz_class_t templates[] = {{{templates + 1},
                                         "tizobject",
                                         templates,
                                         sizeof (tiz_object_t),
                                         NULL,
                                         NULL,
                                         object_ctor,
                                         object_dtor},
                                        {{templates + 1},
                                         "tizclass",
                                         templates,
                                         sizeof (tiz_class_t),
                                         NULL,
                                         NULL,
                                         class_ctor,
                                         class_dtor}};

void *
tiz_class_init (void * ap_tos, void * ap_hdl)
{
  tiz_class_t * tizclass = tiz_mem_calloc (1, sizeof (tiz_class_t));
  memcpy ((char *) tizclass, (char *) (templates + 1), sizeof (tiz_class_t));
  memcpy ((char *) tizclass, (char *) &tizclass, sizeof (tiz_class_t *));
  tizclass->tos = ap_tos;
  tizclass->hdl = ap_hdl;
  TIZ_TRACE (ap_hdl, "class name [%s]->[%p]", tizclass->name, tizclass);
  return tizclass;
}

void *
tiz_object_init (void * ap_tos, void * ap_hdl)
{
  tiz_class_t * tizclass = tiz_get_type (ap_hdl, "tizclass");
  TIZ_LOG_CLASS (tizclass);
  tiz_class_t * tizobject = tiz_mem_calloc (1, sizeof (tiz_class_t));
  const size_t super_offset = offsetof (tiz_class_t, super);
  memcpy ((char *) tizobject, (char *) templates, sizeof (tiz_class_t));
  memcpy ((char *) tizobject, (char *) &tizclass, sizeof (tiz_class_t *));
  memcpy ((char *) tizobject + super_offset, (char *) &tizobject,
          sizeof (tiz_class_t *));
  memcpy ((char *) tizclass + super_offset, (char *) &tizobject,
          sizeof (tiz_class_t *));
  TIZ_TRACE (ap_hdl, "object name [%s]->[%p]", tizobject->name, tizobject);
  return tizobject;
}
