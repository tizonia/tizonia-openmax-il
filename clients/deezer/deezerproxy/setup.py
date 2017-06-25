#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
#
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from setuptools import setup, find_packages
import re
import sys

# Only 2.6-2.7 are supported.
if not ((2, 6, 0) <= sys.version_info[:3] < (2, 8)):
    sys.stderr.write('deezerproxy does not officially support this Python version.\n')
    # try to continue anyway

VERSIONFILE = '_version.py'

version_line = open(VERSIONFILE).read()
version_re = r"^__version__ = ['\"]([^'\"]*)['\"]"
match = re.search(version_re, version_line, re.M)
if match:
    version = match.group(1)
else:
    raise RuntimeError("Could not find version in '%s'" % VERSIONFILE)

setup(
    name                 = 'tizdeezerproxy',
    version              = version,
    author               = 'Juan A. Rubio',
    author_email         = 'juan.rubio@aratelia.com',
    url                  = 'http://www.tizonia.org',
    py_modules           = ['tizdeezerproxy', 'tizdeezermodels', 'tizmopidydeezer', 'tizmopidydeezerutils'],
    scripts              = [],
    license              = "Apache License, Version 2.0",
    description          = 'Tizonia Deezer proxy',
    install_requires     = [
#        'deezer >= 0.4.1',
    ],
    classifiers          = [
        "Development Status :: 2 - Pre-Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: OS Independent",
        'Programming Language :: Python',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
        'Topic :: Internet :: WWW/HTTP',
        'Topic :: Multimedia :: Sound/Audio',
        'Topic :: Software Development :: Libraries :: Python Modules',
    ],
    zip_safe             = False,
    include_package_data = True
)

