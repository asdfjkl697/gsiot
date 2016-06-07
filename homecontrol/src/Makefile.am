## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\"

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = homecontrol

homecontrol_SOURCES = \
	main.c \
	rs232.c \
	rs232.h \
	packet.c \
	packet.h \
	udpbroadcast.c \
	taskmanager.c \
	udpbroadcast.h \
	typedef.h \
	tcpserver.c \
	tcpserver.h \
	hardware.c \
	hardware.h \
	client.c \
	obsolete.c \
	radionetwork.c \
	taskmanager.h

homecontrol_LDADD =  \
	-levent \
	-lcrypt \
	-lxml2

