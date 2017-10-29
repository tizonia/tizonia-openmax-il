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
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = tizonia-remote.in

bashcompletiondir=@bashcompletiondir@
zshcompletiondir=@zshcompletiondir@

DISTCHECK_CONFIGURE_FLAGS = \
	--with-bashcompletiondir=$$dc_install_base/$(bashcompletiondir) \
	--with-zshcompletiondir=$$dc_install_base/$(zshcompletiondir)

do_subst = sed -e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
	-e 's,[@]VERSION[@],$(VERSION),g'

tizonia-remote: tizonia-remote.in Makefile
	$(do_subst) < $(srcdir)/tizonia-remote.in > tizonia-remote
	chmod +x tizonia-remote

all-local: tizonia-remote

clean-local: clean-local-tools
distclean-local: clean-local-tools
.PHONY: clean-local-tools
clean-local-tools:
	-rm -f tizonia-remote

