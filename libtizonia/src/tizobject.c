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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "tizobject.h"
#include "tizobject_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.object"
#endif


/*
 * Object
 */

static void *
Object_ctor (void *ap_obj, va_list * app)
{
  return ap_obj;
}

static void *
Object_dtor (void *ap_obj)
{
  return ap_obj;
}

const void *
classOf (const void *ap_obj)
{
  const struct Object *p_obj = ap_obj;
  assert (p_obj && p_obj->class);
  return p_obj->class;
}

size_t
sizeOf (const void *ap_obj)
{
  const struct Class *class = classOf (ap_obj);
  return class->size;
}

const char *
nameOf (const void *ap_obj)
{
  const struct Class *class = classOf (ap_obj);
  return class->name;
}

/*
 * Class
 */

static void *
Class_ctor (void *ap_obj, va_list * app)
{
  struct Class *p_obj = ap_obj;
  const size_t offset = offsetof (struct Class, ctor);

  p_obj->name = va_arg (*app, char *);
  p_obj->super = va_arg (*app, struct Class *);
  p_obj->size = va_arg (*app, size_t);

  assert (p_obj->super);

  memcpy ((char *) p_obj + offset, (char *) p_obj->super
          + offset, sizeOf (p_obj->super) - offset);
  {
    typedef void (*voidf) ();   /* generic function pointer */
    voidf selector;
    va_list ap;
    va_copy (ap, *app);

    while ((selector = va_arg (ap, voidf)))
      {
        voidf method = va_arg (ap, voidf);

        if (selector == (voidf) ctor)
          *(voidf *) & p_obj->ctor = method;
        else if (selector == (voidf) dtor)
          *(voidf *) & p_obj->dtor = method;
      }

    va_end (ap);
    return p_obj;
  }
}

static void *
Class_dtor (void *ap_obj)
{
  return 0;
}

const void *
super (const void *ap_obj)
{
  const struct Class *p_obj = ap_obj;
  assert (p_obj && p_obj->super);
  return p_obj->super;
}

/*
 * initialization
 */

static const struct Class object[] = {
  {{object + 1},
   "Object", object, sizeof (struct Object),
   Object_ctor, Object_dtor},
  {{object + 1},
   "Class", object, sizeof (struct Class),
   Class_ctor, Class_dtor}
};

const void *Object = object;
const void *Class = object + 1;

/*
 * object management and selectors
 */

void *
factory_new (const void *a_class, ...)
{
  const struct Class *class = a_class;
  struct Object *object = NULL;
  va_list ap;

  assert (class && class->size);
  if (NULL != (object = tiz_mem_calloc (1, class->size)))
  {
    object->class = class;
    va_start (ap, a_class);
    object        = ctor (object, &ap);
    va_end (ap);
  }
  return object;
}

void
factory_delete (void *ap_obj)
{
  if (ap_obj)
    {
      free (dtor (ap_obj));
    }
}

void *
ctor (void *ap_obj, va_list * app)
{
  const struct Class *class = classOf (ap_obj);
  assert (class->ctor);
  return class->ctor (ap_obj, app);
}

void *
super_ctor (const void *a_class, void *ap_obj, va_list * app)
{
  const struct Class *superclass = super (a_class);
  assert (ap_obj && superclass->ctor);
  return superclass->ctor (ap_obj, app);
}

void *
dtor (void *ap_obj)
{
  const struct Class *class = classOf (ap_obj);
  assert (class->dtor);
  return class->dtor (ap_obj);
}

void *
super_dtor (const void *a_class, void *ap_obj)
{
  const struct Class *superclass = super (a_class);
  assert (ap_obj && superclass->dtor);
  return superclass->dtor (ap_obj);
}
