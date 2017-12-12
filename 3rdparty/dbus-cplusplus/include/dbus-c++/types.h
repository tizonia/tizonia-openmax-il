/*
 *
 *  D-Bus++ - C++ bindings for D-Bus
 *
 *  Copyright (C) 2005-2007  Paolo Durante <shackan@gmail.com>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef __DBUSXX_TYPES_H
#define __DBUSXX_TYPES_H

#include <stdint.h>
#include <string>
#include <vector>
#include <map>

#include "api.h"
#include "util.h"
#include "message.h"
#include "error.h"

namespace Tiz { namespace DBus
{

struct DXXAPI Path : public std::string
{
  Path() {}
  Path(const std::string &s) : std::string(s) {}
  Path(const char *c) : std::string(c) {}
  Path &operator = (std::string &s)
  {
    std::string::operator = (s);
    return *this;
  }
};

struct DXXAPI Signature : public std::string
{
  Signature() {}
  Signature(const std::string &s) : std::string(s) {}
  Signature(const char *c) : std::string(c) {}
  Signature &operator = (std::string &s)
  {
    std::string::operator = (s);
    return *this;
  }
};

struct DXXAPI Invalid {};

class DXXAPI Variant
{
public:

  Variant();

  Variant(MessageIter &it);

  Variant &operator = (const Variant &v);

  const Signature signature() const;

  void clear();

  MessageIter reader() const
  {
    return _msg.reader();
  }

  MessageIter writer()
  {
    return _msg.writer();
  }

  template <typename T>
  operator T() const;

private:

  Message _msg;
};

template <
typename T1,
         typename T2 = Invalid,
         typename T3 = Invalid,
         typename T4 = Invalid,
         typename T5 = Invalid,
         typename T6 = Invalid,
         typename T7 = Invalid,
         typename T8 = Invalid,
         typename T9 = Invalid,
         typename T10 = Invalid,
         typename T11 = Invalid,
         typename T12 = Invalid,
         typename T13 = Invalid,
         typename T14 = Invalid,
         typename T15 = Invalid,
         typename T16 = Invalid // nobody needs more than 16
         >
struct Struct
{
  T1 _1;
  T2 _2;
  T3 _3;
  T4 _4;
  T5 _5;
  T6 _6;
  T7 _7;
  T8 _8;
  T9 _9;
  T10 _10;
  T11 _11;
  T12 _12;
  T13 _13;
  T14 _14;
  T15 _15;
  T16 _16;
};

template<typename K, typename V>
inline bool dict_has_key(const std::map<K, V>& map, const K &key)
{
  return map.find(key) != map.end();
}

template <typename T>
struct type
{
  static std::string sig()
  {
    throw ErrorInvalidArgs("unknown type");
    return "";
  }
};

template <> struct type<Variant>
{
  static std::string sig()
  {
    return "v";
  }
};
template <> struct type<uint8_t>
{
  static std::string sig()
  {
    return "y";
  }
};
template <> struct type<bool>
{
  static std::string sig()
  {
    return "b";
  }
};
template <> struct type<int16_t>
{
  static std::string sig()
  {
    return "n";
  }
};
template <> struct type<uint16_t>
{
  static std::string sig()
  {
    return "q";
  }
};
template <> struct type<int32_t>
{
  static std::string sig()
  {
    return "i";
  }
};
template <> struct type<uint32_t>
{
  static std::string sig()
  {
    return "u";
  }
};
template <> struct type<int64_t>
{
  static std::string sig()
  {
    return "x";
  }
};
template <> struct type<uint64_t>
{
  static std::string sig()
  {
    return "t";
  }
};
template <> struct type<double>
{
  static std::string sig()
  {
    return "d";
  }
};
template <> struct type<std::string>
{
  static std::string sig()
  {
    return "s";
  }
};
template <> struct type<Path>
{
  static std::string sig()
  {
    return "o";
  }
};
template <> struct type<Signature>
{
  static std::string sig()
  {
    return "g";
  }
};
template <> struct type<Invalid>
{
  static std::string sig()
  {
    return "";
  }
};

template <typename E>
struct type< std::vector<E> >
{
  static std::string sig()
  {
    return "a" + type<E>::sig();
  }
};

template <typename K, typename V>
struct type< std::map<K, V> >
{
  static std::string sig()
  {
    return "a{" + type<K>::sig() + type<V>::sig() + "}";
  }
};

template <
typename T1,
         typename T2,
         typename T3,
         typename T4,
         typename T5,
         typename T6,
         typename T7,
         typename T8,
         typename T9,
         typename T10,
         typename T11,
         typename T12,
         typename T13,
         typename T14,
         typename T15,
         typename T16 // nobody needs more than 16
         >
struct type< Struct<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16> >
{
  static std::string sig()
  {
    return "("
           + type<T1>::sig()
           + type<T2>::sig()
           + type<T3>::sig()
           + type<T4>::sig()
           + type<T5>::sig()
           + type<T6>::sig()
           + type<T7>::sig()
           + type<T8>::sig()
           + type<T9>::sig()
           + type<T10>::sig()
           + type<T11>::sig()
           + type<T12>::sig()
           + type<T13>::sig()
           + type<T14>::sig()
           + type<T15>::sig()
           + type<T16>::sig()
           + ")";
  }
};

extern DXXAPI DBus::MessageIter &operator << (DBus::MessageIter &iter, const DBus::Variant &val);

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const DBus::Invalid &)
{
  return iter;
}

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const uint8_t &val)
{
  iter.append_byte(val);
  return iter;
}

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const bool &val)
{
  iter.append_bool(val);
  return iter;
}

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const int16_t &val)
{
  iter.append_int16(val);
  return iter;
}

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const uint16_t &val)
{
  iter.append_uint16(val);
  return iter;
}

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const int32_t &val)
{
  iter.append_int32(val);
  return iter;
}

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const uint32_t &val)
{
  iter.append_uint32(val);
  return iter;
}

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const int64_t &val)
{
  iter.append_int64(val);
  return iter;
}

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const uint64_t &val)
{
  iter.append_uint64(val);
  return iter;
}

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const double &val)
{
  iter.append_double(val);
  return iter;
}

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const std::string &val)
{
  iter.append_string(val.c_str());
  return iter;
}

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const DBus::Path &val)
{
  iter.append_path(val.c_str());
  return iter;
}

inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const DBus::Signature &val)
{
  iter.append_signature(val.c_str());
  return iter;
}

template<typename E>
inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const std::vector<E>& val)
{
  const std::string sig = DBus::type<E>::sig();
  DBus::MessageIter ait = iter.new_array(sig.c_str());

  typename std::vector<E>::const_iterator vit;
  for (vit = val.begin(); vit != val.end(); ++vit)
  {
    ait << *vit;
  }

  iter.close_container(ait);
  return iter;
}

template<>
inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const std::vector<uint8_t>& val)
{
  DBus::MessageIter ait = iter.new_array("y");
  ait.append_array('y', &val.front(), val.size());
  iter.close_container(ait);
  return iter;
}

template<typename K, typename V>
inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const std::map<K, V>& val)
{
  const std::string sig = "{" + DBus::type<K>::sig() + DBus::type<V>::sig() + "}";
  DBus::MessageIter ait = iter.new_array(sig.c_str());

  typename std::map<K, V>::const_iterator mit;
  for (mit = val.begin(); mit != val.end(); ++mit)
  {
    DBus::MessageIter eit = ait.new_dict_entry();

    eit << mit->first << mit->second;

    ait.close_container(eit);
  }

  iter.close_container(ait);
  return iter;
}

template <
typename T1,
         typename T2,
         typename T3,
         typename T4,
         typename T5,
         typename T6,
         typename T7,
         typename T8,
         typename T9,
         typename T10,
         typename T11,
         typename T12,
         typename T13,
         typename T14,
         typename T15,
         typename T16
         >
inline DBus::MessageIter &operator << (DBus::MessageIter &iter, const DBus::Struct<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16>& val)
{
  DBus::MessageIter sit = iter.new_struct();

  sit << val._1 << val._2 << val._3 << val._4
      << val._5 << val._6 << val._7 << val._8
      << val._9 << val._10 << val._11 << val._12
      << val._13 << val._14 << val._15 << val._16;

  iter.close_container(sit);

  return iter;
}

extern DXXAPI DBus::MessageIter &operator << (DBus::MessageIter &iter, const DBus::Variant &val);

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, DBus::Invalid &)
{
  return iter;
}

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, uint8_t &val)
{
  val = iter.get_byte();
  return ++iter;
}

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, bool &val)
{
  val = iter.get_bool();
  return ++iter;
}

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, int16_t &val)
{
  val = iter.get_int16();
  return ++iter;
}

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, uint16_t &val)
{
  val = iter.get_uint16();
  return ++iter;
}

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, int32_t &val)
{
  val = iter.get_int32();
  return ++iter;
}

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, uint32_t &val)
{
  val = iter.get_uint32();
  return ++iter;
}

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, int64_t &val)
{
  val = iter.get_int64();
  return ++iter;
}

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, uint64_t &val)
{
  val = iter.get_uint64();
  return ++iter;
}

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, double &val)
{
  val = iter.get_double();
  return ++iter;
}

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, std::string &val)
{
  val = iter.get_string();
  return ++iter;
}

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, DBus::Path &val)
{
  val = iter.get_path();
  return ++iter;
}

inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, DBus::Signature &val)
{
  val = iter.get_signature();
  return ++iter;
}

extern DXXAPI DBus::MessageIter &operator >> (DBus::MessageIter &iter, DBus::Variant &val);

template<typename E>
inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, std::vector<E>& val)
{
  if (!iter.is_array())
    throw DBus::ErrorInvalidArgs("array expected");

  DBus::MessageIter ait = iter.recurse();

  while (!ait.at_end())
  {
    E elem;

    ait >> elem;

    val.push_back(elem);
  }
  return ++iter;
}

template<>
inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, std::vector<uint8_t>& val)
{
  if (!iter.is_array())
    throw DBus::ErrorInvalidArgs("array expected");

  if (iter.array_type() != 'y')
    throw DBus::ErrorInvalidArgs("byte-array expected");

  DBus::MessageIter ait = iter.recurse();

  uint8_t *array;
  size_t length = ait.get_array(&array);

  val.insert(val.end(), array, array + length);

  return ++iter;
}

template<typename K, typename V>
inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, std::map<K, V>& val)
{
  if (!iter.is_dict())
    throw DBus::ErrorInvalidArgs("dictionary value expected");

  DBus::MessageIter mit = iter.recurse();

  while (!mit.at_end())
  {
    K key;
    V value;

    DBus::MessageIter eit = mit.recurse();

    eit >> key >> value;

    val[key] = value;

    ++mit;
  }

  return ++iter;
}

template <
typename T1,
         typename T2,
         typename T3,
         typename T4,
         typename T5,
         typename T6,
         typename T7,
         typename T8,
         typename T9,
         typename T10,
         typename T11,
         typename T12,
         typename T13,
         typename T14,
         typename T15,
         typename T16
         >
inline DBus::MessageIter &operator >> (DBus::MessageIter &iter, DBus::Struct<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16>& val)
{
  DBus::MessageIter sit = iter.recurse();

  sit >> val._1 >> val._2 >> val._3 >> val._4
      >> val._5 >> val._6 >> val._7 >> val._8
      >> val._9 >> val._10 >> val._11 >> val._12
      >> val._13 >> val._14 >> val._15 >> val._16;

  return ++iter;
}

template <typename T>
inline DBus::Variant::operator T() const
{
  T cast;
  DBus::MessageIter ri = _msg.reader();
  ri >> cast;
  return cast;
}

} /* namespace DBus */ } /* namespace Tiz */

#endif//__DBUSXX_TYPES_H

