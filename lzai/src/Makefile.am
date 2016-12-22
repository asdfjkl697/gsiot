## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir). \
	$(libcurl_CFLAGS). \
	$(OPENSSL_CFLAGS). \
	$(LIBCRYPTO_CFLAGS). \
	$(libcrypto_CFLAGS)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\"

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = lzai

lzai_SOURCES = \
	main.cc \
	lzal.cpp \
	lzsql.cpp \
	Makefile.am \
	lzal.h \
	lzsql.h

lzai_LDFLAGS = 

lzai_LDADD = \
	-lsqlite3


