## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src 

homecontroldocdir = ${prefix}/doc/homecontrol
homecontroldoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS


EXTRA_DIST = $(homecontroldoc_DATA)


# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(homecontroldocdir)


AM_CFLAGS = -I/home/billue/source/libevent-2.0.21-stable/include 
