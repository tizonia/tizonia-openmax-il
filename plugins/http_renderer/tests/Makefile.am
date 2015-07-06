# Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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

TESTS = check_http_renderer

BUILT_SOURCES = check_http_renderer.h

EXTRA_DIST = \
	tizonia.conf \
	tizonia.conf.in \
	check_http_renderer.h.in \
	check_http_renderer.h

CLEANFILES = check_http_renderer.h tizonia.conf

check_PROGRAMS = check_http_renderer

check_http_renderer_SOURCES = check_http_renderer.c

check_http_renderer_CFLAGS = \
	@TIZILHEADERS_CFLAGS@ \
	@TIZPLATFORM_CFLAGS@ \
	@TIZONIA_CFLAGS@ \
	@CHECK_CFLAGS@

check_http_renderer_LDADD = \
	@TIZPLATFORM_LIBS@ \
	@TIZONIA_LIBS@ \
	@TIZCORE_LIBS@ \
	@CHECK_LIBS@

do_subst = sed -e 's,[@]abs_top_builddir[@],$(abs_top_builddir),g' \
	-e 's,[@]localstatedir[@],$(localstatedir),g' \
	-e 's,[@]bindir[@],$(bindir),g' \
	-e 's,[@]libdir[@],$(libdir),g' \
	-e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]VERSION[@],$(VERSION),g'

check_http_renderer.h: check_http_renderer.h.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

tizonia.conf: tizonia.conf.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@

all-local: tizonia.conf

clean-local: clean-local-check-http-renderer
distclean-local: clean-local-check-http-renderer
.PHONY: clean-local-check-http-renderer
clean-local-check-http-renderer:
	-rm -f core tizrm.db
