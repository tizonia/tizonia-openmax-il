# Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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

# NOTE: 'player' folder is not currently being processed by the top Makefile.
# This is to disable tizonia's player compilation when building from top. Still
# it can be manually configured and built.

# SUBDIRS = config include libtizplatform rm libtizcore libtizonia plugins tizonia docs

if ENABLE_PLAYER
SUBDIRS= 3rdparty config include clients libtizplatform cast rm libtizcore libtizonia plugins player
else
SUBDIRS= 3rdparty include clients libtizplatform cast rm libtizcore libtizonia plugins config
endif
