#noinst_LIBRARIES += vendor/json/libjson.a

noinst_LTLIBRARIES += libjson.la

lib_LTLIBRARIES = libjson.la
libjson_la_SOURCES = vendor/json/json.h \
                        vendor/json/json.c

