lib_LTLIBRARIES = \
	libtizdbus-c++-1.la

libtizdbus_c___1_la_SOURCES = \
	connection.cpp    \
	connection_p.h    \
	debug.cpp    \
	dispatcher.cpp    \
	dispatcher_p.h    \
	error.cpp    \
	eventloop.cpp    \
	eventloop-integration.cpp    \
	interface.cpp    \
	internalerror.h    \
	introspection.cpp    \
	message.cpp    \
	message_p.h    \
	object.cpp    \
	pendingcall.cpp    \
	pendingcall_p.h    \
	pipe.cpp    \
	property.cpp    \
	server.cpp    \
	server_p.h    \
	types.cpp

libtizdbus_c___1_la_CXXFLAGS = \
	-I$(top_srcdir)/include \
	-Wno-unused-parameter

libtizdbus_c___1_la_LIBADD = \
	$(dbus_LIBS)

AM_CPPFLAGS = \
	$(dbus_CFLAGS)

HEADER_DIR = $(top_srcdir)/include/dbus-c++

libtizdbus_c___1_HEADERS = \
	$(HEADER_DIR)/api.h          \
	$(HEADER_DIR)/connection.h          \
	$(HEADER_DIR)/dbus.h          \
	$(HEADER_DIR)/debug.h          \
	$(HEADER_DIR)/dispatcher.h          \
	$(HEADER_DIR)/error.h          \
	$(HEADER_DIR)/eventloop.h          \
	$(HEADER_DIR)/eventloop-integration.h          \
	$(HEADER_DIR)/interface.h          \
	$(HEADER_DIR)/introspection.h          \
	$(HEADER_DIR)/message.h          \
	$(HEADER_DIR)/object.h          \
	$(HEADER_DIR)/pendingcall.h          \
	$(HEADER_DIR)/pipe.h          \
	$(HEADER_DIR)/property.h          \
	$(HEADER_DIR)/refptr_impl.h          \
	$(HEADER_DIR)/server.h          \
	$(HEADER_DIR)/types.h          \
	$(HEADER_DIR)/util.h

libtizdbus_c___1dir=$(includedir)/tizonia/dbus-c++/

EXTRA_DIST = \
	$(libtizdbus_c___1_HEADERS)

