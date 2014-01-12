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

if HAVE_DOXYGEN
# directory = $(top_srcdir)/docs/man/man3/
# 
# dist_man_MANS = $(directory)/man_page_1.3 $(directory)/man_page_2.3
# $(directory)/man_page_1.3: doxyfile.stamp
# $(directory)/man_page_2.3: doxyfile.stamp

doxyfile.stamp:
	$(DOXYGEN) Doxyfile
	echo Timestamp > doxyfile.stamp

DISTCLEANFILES = Doxyfile doxyfile.stamp

all-local: doxyfile.stamp

clean-local: clean-local-doxygen
distclean-local: clean-local-doxygen
.PHONY: clean-local-doxygen
clean-local-doxygen:
	-rm -rf $(top_builddir)/docs/html
#	rm -rf $(top_builddir)/docs/man
endif
