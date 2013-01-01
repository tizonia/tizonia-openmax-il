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


#ifndef TIZOBJECT_H
#define TIZOBJECT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

/* factory_new(Object); */
extern const void *Object;

void *factory_new (const void *class, ...);
void factory_delete (void *p_obj);

const void *classOf (const void *p_obj);
size_t sizeOf (const void *p_obj);
const char * nameOf (const void *p_obj);

void *ctor (void *p_obj, va_list * app);
void *dtor (void *p_obj);

/* factory_new(Class, "name", super, size sel, meth, ... 0); */
extern const void *Class;

/* class' superclass */
const void *super (const void *p_obj);

#ifdef __cplusplus
}
#endif

#endif /* TIZOBJECT_H */
