# Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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


# NOTE: 'docs' folders is not currently being processed by the top Makefile.
# The Doxygen/Sphinx documentation can still be manually generated, whenever is needed.

# NOTE: 'tplay' folder is not currently being processed by the top Makefile.
# This is to disable tplay compilation when building from top. This is because
# tplay's FSMs require a very recent version of Boost.msm (currently >= 1.54).

# SUBDIRS = config include libtizplatform rm libtizcore libtizonia plugins tplay docs
SUBDIRS = config include libtizplatform rm libtizcore libtizonia plugins


# NOTE: A cscope target is included in automake since api 1.12

#CLEANFILES = cscope.in.out cscope.out cscope.po.out

#.PHONY: cscope
#cscope:
#	cscope -b -q -R -Iinclude/omxil-1.2 -slibtizonia -slibtizcore -slibtizplatform -srm -splugins -stplay;

# '-nut’ = '--no-tabs’
#    Use spaces instead of tabs.
# '-sc’ = '--start-left-side-of-comments’
#    Put the '*’ character at the left of comments.
# '-pmt’ = '--preserve-mtime’
#    Preserve access and modification times on output files.
#  '-l79' = '--line-lengthn'
#    Set maximum line length for non-comment lines to 79.


# NOTE: The indent target is disabled for now. Currently experimenting with clang-format for C++.

# .PHONY: indent
# indent:
# 	find . -path ./include -prune \
# 	-o -path "*tests*" -prune \
# 	-o -path ./docs -prune \
# 	-o -iname "config.h" -prune \
# 	-o -iname "utarray.h" -prune \
# 	-o -iname "*.[ch]" \
# 	-exec indent --gnu-style -pmt -sc -nut -l79 "{}" \;

