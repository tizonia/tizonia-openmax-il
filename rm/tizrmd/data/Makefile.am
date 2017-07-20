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

sqlite_scripts = tizonia-rm-db-initial.sql3
shell_scripts = tizonia-rm-db-dump.sh tizonia-rm-db-generate.sh
sqlite_databases = tizrm.db

tizrmdbdir = $(datadir)/$(PACKAGE)
tizrmdb_DATA = $(sqlite_scripts) $(sqlite_databases)

bin_SCRIPTS = $(shell_scripts)
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = $(sqlite_scripts) $(sqlite_databases) tizonia-rm-db-dump.sh.in tizonia-rm-db-generate.sh.in

do_subst = sed -e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]VERSION[@],$(VERSION),g'

tizonia-rm-db-dump.sh: tizonia-rm-db-dump.sh.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@
	chmod +x $@

tizonia-rm-db-generate.sh: tizonia-rm-db-generate.sh.in Makefile
	$(do_subst) < $(srcdir)/$@.in > $@
	chmod +x $@

tizrm.db: $(sqlite_scripts) $(shell_scripts)
	sh ./tizonia-rm-db-generate.sh

all-local: tizrm.db

clean-local: clean-local-tizrmdb
distclean-local: clean-local-tizrmdb
.PHONY: clean-local-tizrmdb
clean-local-tizrmdb:
	-rm -f $(sqlite_databases)

