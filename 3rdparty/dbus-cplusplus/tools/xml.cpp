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


#include "xml.h"

#include <expat.h>

std::istream &operator >> (std::istream &in, DBus::Xml::Document &doc)
{
  std::stringbuf xmlbuf;
  in.get(xmlbuf, '\0');
  doc.from_xml(xmlbuf.str());

  return in;
}

std::ostream &operator << (std::ostream &out, const DBus::Xml::Document &doc)
{
  return out << doc.to_xml();
}

using namespace DBus;
using namespace DBus::Xml;

Error::Error(const char *error, int line, int column)
{
  std::ostringstream estream;

  estream << "line " << line << ", column " << column << ": " << error;

  _error = estream.str();
}

Node::Node(const char *n, const char **a)
  : name(n)
{
  if (a)
    for (int i = 0; a[i]; i += 2)
    {
      _attrs[a[i]] = a[i + 1];

      //debug_log("xml:\t%s = %s", a[i], a[i+1]);
    }
}

Nodes Nodes::operator[](const std::string &key)
{
  Nodes result;

  for (iterator i = begin(); i != end(); ++i)
  {
    Nodes part = (**i)[key];

    result.insert(result.end(), part.begin(), part.end());
  }
  return result;
}

Nodes Nodes::select(const std::string &attr, const std::string &value)
{
  Nodes result;

  for (iterator i = begin(); i != end(); ++i)
  {
    if ((*i)->get(attr) == value)
      result.insert(result.end(), *i);
  }
  return result;
}

Nodes Node::operator[](const std::string &key)
{
  Nodes result;

  if (key.length() == 0) return result;

  for (Children::iterator i = children.begin(); i != children.end(); ++i)
  {
    if (i->name == key)
      result.push_back(&(*i));
  }
  return result;
}

std::string Node::get(const std::string &attribute)
{
  if (_attrs.find(attribute) != _attrs.end())
    return _attrs[attribute];
  else
    return "";
}

void Node::set(const std::string &attribute, std::string value)
{
  if (value.length())
    _attrs[attribute] = value;
  else
    _attrs.erase(value);
}

std::string Node::to_xml() const
{
  std::string xml;
  int depth = 0;

  _raw_xml(xml, depth);

  return xml;
}

void Node::_raw_xml(std::string &xml, int &depth) const
{
  xml.append(depth * 2, ' ');
  xml.append("<" + name);

  for (Attributes::const_iterator i = _attrs.begin(); i != _attrs.end(); ++i)
  {
    xml.append(" " + i->first + "=\"" + i->second + "\"");
  }

  if (cdata.length() == 0 && children.size() == 0)
  {
    xml.append("/>\n");
  }
  else
  {
    xml.append(">");

    if (cdata.length())
    {
      xml.append(cdata);
    }

    if (children.size())
    {
      xml.append("\n");
      depth++;

      for (Children::const_iterator i = children.begin(); i != children.end(); ++i)
      {
        i->_raw_xml(xml, depth);
      }

      depth--;
      xml.append(depth * 2, ' ');
    }
    xml.append("</" + name + ">\n");
  }
}

Document::Document()
  : root(0), _depth(0)
{
}

Document::Document(const std::string &xml)
  : root(0), _depth(0)
{
  from_xml(xml);
}

Document::~Document()
{
  delete root;
}

struct Document::Expat
{
  static void start_doctype_decl_handler(
    void *data, const XML_Char *name, const XML_Char *sysid, const XML_Char *pubid, int has_internal_subset
  );
  static void end_doctype_decl_handler(void *data);
  static void start_element_handler(void *data, const XML_Char *name, const XML_Char **atts);
  static void character_data_handler(void *data, const XML_Char *chars, int len);
  static void end_element_handler(void *data, const XML_Char *name);
};

void Document::from_xml(const std::string &xml)
{
  _depth = 0;
  delete root;
  root = 0;

  XML_Parser parser = XML_ParserCreate("UTF-8");

  XML_SetUserData(parser, this);

  XML_SetDoctypeDeclHandler(
    parser,
    Document::Expat::start_doctype_decl_handler,
    Document::Expat::end_doctype_decl_handler
  );

  XML_SetElementHandler(
    parser,
    Document::Expat::start_element_handler,
    Document::Expat::end_element_handler
  );

  XML_SetCharacterDataHandler(
    parser,
    Document::Expat::character_data_handler
  );

  XML_Status status = XML_Parse(parser, xml.c_str(), xml.length(), true);

  if (status == XML_STATUS_ERROR)
  {
    const char *error = XML_ErrorString(XML_GetErrorCode(parser));
    int line = XML_GetCurrentLineNumber(parser);
    int column = XML_GetCurrentColumnNumber(parser);

    XML_ParserFree(parser);

    throw Error(error, line, column);
  }
  else
  {
    XML_ParserFree(parser);
  }
}

std::string Document::to_xml() const
{
  return root->to_xml();
}

void Document::Expat::start_doctype_decl_handler(
  void *data, const XML_Char *name, const XML_Char *sysid, const XML_Char *pubid, int has_internal_subset
)
{
}

void Document::Expat::end_doctype_decl_handler(void *data)
{
}

void Document::Expat::start_element_handler(void *data, const XML_Char *name, const XML_Char **atts)
{
  Document *doc = (Document *)data;

  //debug_log("xml:%d -> %s", doc->_depth, name);

  if (!doc->root)
  {
    doc->root = new Node(name, atts);
  }
  else
  {
    Node::Children *cld = &(doc->root->children);

    for (int i = 1; i < doc->_depth; ++i)
    {
      cld = &(cld->back().children);
    }
    cld->push_back(Node(name, atts));

    //std::cerr << doc->to_xml() << std::endl;
  }
  doc->_depth++;
}

void Document::Expat::character_data_handler(void *data, const XML_Char *chars, int len)
{
  Document *doc = (Document *)data;

  Node *nod = doc->root;

  for (int i = 1; i < doc->_depth; ++i)
  {
    nod = &(nod->children.back());
  }
  int x, y;

  x = 0;
  y = len - 1;

  while (isspace(chars[y]) && y > 0) --y;
  while (isspace(chars[x]) && x < y) ++x;

  nod->cdata = std::string(chars, x, y + 1);
}

void Document::Expat::end_element_handler(void *data, const XML_Char *name)
{
  Document *doc = (Document *)data;

  //debug_log("xml:%d <- %s", doc->_depth, name);

  doc->_depth--;
}

