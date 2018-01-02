# Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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

dist_bashcompletion_data = tizonia
dist_zshcompletion_data  = _tizonia

if ENABLE_BASH_COMPLETION
dist_bashcompletion_DATA = $(dist_bashcompletion_data)
endif
if ENABLE_ZSH_COMPLETION
dist_zshcompletion_DATA = $(dist_zshcompletion_data)
endif

shell_scripts = tizonia-remote

bin_SCRIPTS = $(shell_scripts)
EXTRA_DIST = tizonia-remote.in tizonia.desktop.in
desktop_DATA = tizonia.desktop
dist_icons256_DATA = icons/256x256/tizonia.png
CLEANFILES = $(bin_SCRIPTS) $(desktop_DATA)

bashcompletiondir=@bashcompletiondir@
zshcompletiondir=@zshcompletiondir@
desktopdir = $(datadir)/applications
iconsdatadir = $(datadir)/icons/hicolor
icons256dir = $(iconsdatadir)/256x256/apps

DISTCHECK_CONFIGURE_FLAGS = \
	--with-bashcompletiondir=$$dc_install_base/$(bashcompletiondir) \
	--with-zshcompletiondir=$$dc_install_base/$(zshcompletiondir)

do_subst = sed -e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]bindir[@],$(bindir),g' \
	-e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
	-e 's,[@]VERSION[@],$(VERSION),g'

tizonia.desktop: tizonia.desktop.in  Makefile
	$(do_subst) < $(srcdir)/tizonia.desktop.in > tizonia.desktop

tizonia-remote: tizonia-remote.in Makefile
	$(do_subst) < $(srcdir)/tizonia-remote.in > tizonia-remote
	chmod +x tizonia-remote

all-local: tizonia-remote tizonia.desktop

clean-local: clean-local-tools
distclean-local: clean-local-tools
.PHONY: clean-local-tools
clean-local-tools:
	-rm -f tizonia-remote tizonia.desktop

