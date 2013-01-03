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

sqlite_scripts = initrmdb.sql3
shell_scripts = tizrm_dumpdb.sh tizrm_gendb.sh
sqlite_databases = tizrm.db

EXTRA_DIST = $(sqlite_scripts) $(shell_scripts) $(sqlite_databases)

tizrmdbdir = $(datadir)/$(PACKAGE)
tizrmdb_DATA = $(sqlite_scripts) $(shell_scripts) $(sqlite_databases)

tizrm.db: $(sqlite_scripts)
	./tizrm_gendb.sh

all-local: tizrm.db

clean-local: clean-local-tizrmdb
distclean-local: clean-local-tizrmdb
.PHONY: clean-local-tizrmdb
clean-local-tizrmdb:
	-rm -f tizrm.db

