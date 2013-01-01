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


#ifndef TIZOBJECT_DECLS_H
#define TIZOBJECT_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

struct Object
{
  /* object's description */
  const struct Class *class;
};

struct Class
{
  /* class' description */
  const struct Object _;
  /* class' name */
  const char *name;
  /* class' super class */
  const struct Class *super;
  /* class' object's size */
  size_t size;
  void *(*ctor) (void *p_obj, va_list * app);
  void *(*dtor) (void *p_obj);
};

void *super_ctor (const void *class, void *p_obj, va_list * app);
void *super_dtor (const void *class, void *p_obj);

#ifdef __cplusplus
}
#endif

#endif /* TIZOBJECT_DECLS_H */
