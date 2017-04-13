#noinst_LIBRARIES += vendor/json/libjson.a

noinst_LTLIBRARIES += libpatricia.la

lib_LTLIBRARIES = libpatricia.la
libpatricia_la_LDFLAGS=-module -avoid-version -shared

libpatricia_la_SOURCES = vendor/patricia/patricia.h \
                        vendor/patricia/patricia.c

