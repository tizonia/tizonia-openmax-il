AM_CPPFLAGS = \
	@DBUS_CFLAGS@ \
	@EXPAT_CFLAGS@ \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-Wall

libtizdbus_cxx_la = $(top_builddir)/src/libtizdbus-c++.la

bin_PROGRAMS = tizonia-dbusxx-xml2cpp tizonia-dbusxx-introspect

tizonia_dbusxx_xml2cpp_SOURCES = \
	xml.h \
	xml.cpp \
	xml2cpp.h \
	xml2cpp.cpp \
	generate_adaptor.cpp \
	generate_adaptor.h \
	generate_proxy.cpp \
	generate_proxy.h \
	generator_utils.cpp \
	generator_utils.h

tizonia_dbusxx_xml2cpp_LDADD = \
	@EXPAT_LIBS@

tizonia_dbusxx_introspect_SOURCES = \
	introspect.h \
	introspect.cpp

tizonia_dbusxx_introspect_LDADD = \
	$(libtizdbus_cxx_la) \
	-lpthread
