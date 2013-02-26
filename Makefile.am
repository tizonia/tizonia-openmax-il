# Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
#
# This file is part of Tizonia
#
# Tizonia is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
# more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.

SUBDIRS = include libtizosal rm libtizcore libtizonia tizcomps examples docs

CLEANFILES = cscope.in.out cscope.out cscope.po.out

.PHONY: cscope
cscope:
	cscope -b -q -R -Iinclude/omxil-1.2 -slibtizonia -slibtizcore -slibtizosal -srm -stizcomps -sexamples;

# '-nut’ = '--no-tabs’
#    Use spaces instead of tabs.
# '-sc’ = '--start-left-side-of-comments’
#    Put the '*’ character at the left of comments.
# '-pmt’ = '--preserve-mtime’
#    Preserve access and modification times on output files.
#  '-l79' = '--line-lengthn'
#    Set maximum line length for non-comment lines to 79.

.PHONY: indent
indent:
	find . -path ./include -prune \
	-o -path "*tests*" -prune \
	-o -path ./docs -prune \
	-o -iname "config.h" -prune \
	-o -iname "utarray.h" -prune \
	-o -iname "*.[ch]" \
	-exec indent --gnu-style -pmt -sc -nut -l79 "{}" \;

