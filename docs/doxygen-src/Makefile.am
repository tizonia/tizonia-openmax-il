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

#if HAVE_DOXYGEN
# directory = $(top_srcdir)/docs/man/man3/
#
# dist_man_MANS = $(directory)/man_page_1.3 $(directory)/man_page_2.3
# $(directory)/man_page_1.3: doxyfile.stamp
# $(directory)/man_page_2.3: doxyfile.stamp

EXTRA_DIST = \
	doxyfile \
	doxyfile.in \
	footer.html \
	header.html \
	mainpage.dox \
	plugins.dox \
	resourcemanagement.dox

CLEANFILES = doxyfile doxyfile.stamp

doxyfile.stamp: doxyfile mainpage.dox plugins.dox resourcemanagement.dox footer.html header.html customdoxygen.css doxy-boot.js
	$(DOXYGEN) doxyfile
	cp doxy-boot.js html
	echo Timestamp > doxyfile.stamp

do_subst = sed -e 's,[@]abs_top_srcdir[@],$(abs_top_srcdir),g' \
	-e 's,[@]abs_srcdir[@],$(abs_srcdir),g' \
	-e 's,[@]abs_builddir[@],$(abs_builddir),g' \
	-e 's,[@]VERSION[@],$(VERSION),g'

doxyfile: doxyfile.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

all-local: doxyfile doxyfile.stamp

clean-local: clean-local-doxygen
distclean-local: clean-local-doxygen
.PHONY: clean-local-doxygen
clean-local-doxygen:
	-rm -rf html xml
#endif
