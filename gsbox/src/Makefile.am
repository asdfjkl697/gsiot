## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\"

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = gsbox

gsbox_SOURCES = \
	main.cc \
	boardhandler.h \
	client.cpp \
	client.h \
	clientshandler.h \
	discoveryhandler.h \
	hardware.cpp \
	hardware.h \
	iotask.h \
	Makefile.am \
	netpacket.cpp \
	netpacket.h \
	obsolete.cpp \
	obsolete.h \
	packet.cpp \
	packet.h \
	radiohandler.h \
	radionetwork.cpp \
	radionetwork.h \
	radiopacket.cpp \
	radiopacket.h \
	radiosynchandler.cpp \
	radiosynchandler.h \
	radiotimerhandler.h \
	rs232.c \
	rs232.h \
	sockettask.h \
	task.h \
	taskmanager.cpp \
	taskmanager.h \
	tcpserver.cpp \
	tcpserver.h \
	tcpservertask.h \
	timerhandler.h \
	typedef.h \
	udpserver.cpp \
	udpserver.h

gsbox_LDFLAGS = 

gsbox_LDADD =  \
	-levent \
	-lcrypt \
	-lxml2


