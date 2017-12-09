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

#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

#include "xml2cpp.h"
#include "generate_adaptor.h"
#include "generate_proxy.h"

using namespace std;
using namespace DBus;

//typedef map<string,string> TypeCache;

void usage(const char *argv0)
{
  cerr << endl << "Usage: " << argv0 << " <xmlfile> [ --proxy=<outfile.h> ] [ --adaptor=<outfile.h> ]"
       << endl << endl;
  exit(-1);
}

/*int char_to_atomic_type(char t)
{
	if (strchr("ybnqiuxtdsgavre", t))
		return t;

	return DBUS_TYPE_INVALID;
}*/



/*bool is_atomic_type(const string &type)
{
	return type.length() == 1 && char_to_atomic_type(type[0]) != DBUS_TYPE_INVALID;
}*/


int main(int argc, char **argv)
{
  if (argc < 2)
  {
    usage(argv[0]);
  }

  bool proxy_mode, adaptor_mode;
  char *proxy, *adaptor;

  proxy_mode = false;
  proxy = 0;

  adaptor_mode = false;
  adaptor = 0;

  for (int a = 1; a < argc; ++a)
  {
    if (!strncmp(argv[a], "--proxy=", 8))
    {
      proxy_mode = true;
      proxy = argv[a] + 8;
    }
    else if (!strncmp(argv[a], "--adaptor=", 10))
    {
      adaptor_mode = true;
      adaptor = argv[a] + 10;
    }
  }

  if (!proxy_mode && !adaptor_mode) usage(argv[0]);

  ifstream xmlfile(argv[1]);

  if (xmlfile.bad())
  {
    cerr << "unable to open file " << argv[1] << endl;
    return -1;
  }

  Xml::Document doc;

  try
  {
    xmlfile >> doc;
    //cout << doc.to_xml();
  }
  catch (Xml::Error &e)
  {
    cerr << "error parsing " << argv[1] << ": " << e.what() << endl;
    return -1;
  }

  if (!doc.root)
  {
    cerr << "empty document" << endl;
    return -1;
  }

  if (proxy_mode)   generate_proxy(doc, proxy);
  if (adaptor_mode) generate_adaptor(doc, adaptor);

  return 0;
}
