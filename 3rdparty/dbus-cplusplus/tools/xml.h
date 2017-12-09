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


#ifndef __DBUSXX_XML_H
#define __DBUSXX_XML_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <exception>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

namespace DBus
{

namespace Xml
{

class Error : public std::exception
{
public:

  Error(const char *error, int line, int column);

  ~Error() throw()
  {}

  const char *what() const throw()
  {
    return _error.c_str();
  }

private:

  std::string _error;
};

class Node;

class Nodes : public std::vector<Node *>
{
public:

  Nodes operator[](const std::string &key);

  Nodes select(const std::string &attr, const std::string &value);
};

class Node
{
public:

  typedef std::map<std::string, std::string> Attributes;

  typedef std::vector<Node> Children;

  std::string name;
  std::string cdata;
  Children children;

  Node(std::string &n, Attributes &a)
    : name(n), _attrs(a)
  {}

  Node(const char *n, const char **a = NULL);

  Nodes operator[](const std::string &key);

  std::string get(const std::string &attribute);

  void set(const std::string &attribute, std::string value);

  std::string to_xml() const;

  Node &add(Node child)
  {
    children.push_back(child);
    return children.back();
  }

private:

  void _raw_xml(std::string &xml, int &depth) const;

  Attributes _attrs;
};

class Document
{
public:

  struct Expat;

  Node *root;

  Document();

  Document(const std::string &xml);

  ~Document();

  void from_xml(const std::string &xml);

  std::string to_xml() const;

private:

  int _depth;
};

} /* namespace Xml */

} /* namespace DBus */

std::istream &operator >> (std::istream &, DBus::Xml::Document &);
std::ostream &operator << (std::ostream &, DBus::Xml::Document &);

#endif//__DBUSXX_XML_H
