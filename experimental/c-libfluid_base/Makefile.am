AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CPPFLAGS = $(libevent_core_CFLAGS) $(libevent_pthreads_CFLAGS)
if TLS
AM_CPPFLAGS += $(libevent_openssl_CFLAGS)
endif

EXTRA_DIST = README.md

lib_LTLIBRARIES = libfluid_base.la
libfluid_base_la_SOURCES = fluid/base/evloop.h fluid/base/evloop.c\
							fluid/base/ofp_buffer.h\
						   fluid/base/base_of_conn.h fluid/base/base_of_conn.c\
						   fluid/base/base_of_client.h fluid/base/base_of_client.c\
						   fluid/of_conn.h fluid/of_conn.c \
						   fluid/of_settings.h fluid/of_settings.c\
						   fluid/base/of.h\
							fluid/of_client.h fluid/of_client.c
libfluid_base_la_LIBADD = $(libevent_core_LIBS) $(libevent_pthreads_LIBS)
if TLS
libfluid_base_la_LIBADD += $(libevent_openssl_LIBS)
endif

libfluid_base_includedir = $(includedir)
nobase_libfluid_base_include_HEADERS = fluid/base/evloop.h\
										fluid/base/ofp_buffer.h\
										fluid/base/vector.h\
							   		   fluid/base/base_of_conn.h\
							   		   fluid/base/base_of_client.h\
							   		   fluid/of_conn.h\
							   		   fluid/of_settings.h\
							   		   fluid/of_client.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libfluid_base.pc
