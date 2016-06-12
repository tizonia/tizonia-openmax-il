doc_DATA = doxygen-build.stamp

EXTRA_DIST = Doxyfile.in

if HAVE_DOXYGEN
doxygen-build.stamp: Doxyfile ../include/nestegg/nestegg.h
	doxygen
	touch doxygen-build.stamp
else
doxygen-build.stamp:
	echo "*** Warning: Doxygen not found; documentation will not be built."
	touch doxygen-build.stamp
endif

dist_docdir = $(distdir)/libnestegg

dist-hook:
	if test -d html; then \
	  mkdir $(dist_docdir); \
	  echo -n "copying built documenation..."; \
	  cp -rp html $(dist_docdir)/html; \
	  echo "OK"; \
	fi


install-data-local: doxygen-build.stamp
	$(mkinstalldirs) $(DESTDIR)$(docdir)
	if test -d html; then \
	  cp -rp html $(DESTDIR)$(docdir)/html; \
	fi

uninstall-local:
	rm -rf $(DESTDIR)$(docdir)

clean-local:
	if test -d html; then rm -rf html; fi
	if test -f doxygen-build.stamp; then rm -f doxygen-build.stamp; fi

