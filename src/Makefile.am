## Process this file with automake to produce Makefile.in

## Created by Anjuta

#uidir = $(datadir)/gpet/ui
#ui_DATA = gpet.ui

icondir = $(datadir)/gpet/pixmaps
icon_DATA = tomoyo.png

ccsdir = $(srcdir)/usr_sbin

GPET_CFLAGS = -Wno-pointer-sign -Wno-unused-function\
	-D__GPET\
	`pkg-config --cflags gtk+-2.0`\
	`pkg-config --cflags gconf-2.0`

AM_CPPFLAGS = \
	-I$(ccsdir) \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_ICON_DIR=\""$(icondir)"\" \
	$(GPET_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

sbin_PROGRAMS = gpet

ccs_SOURCES = \
	$(ccsdir)/ccstools.c  \
	$(ccsdir)/ccstools.h  \
	$(ccsdir)/editpolicy.c  \
	$(ccsdir)/editpolicy.h  \
	$(ccsdir)/editpolicy_keyword.c \
	$(ccsdir)/editpolicy_offline.c  \
	$(ccsdir)/editpolicy_optimizer.c  \
	$(ccsdir)/editpolicy_color.c

gpet_SOURCES = \
	$(ccs_SOURCES) \
	gpet.c \
	gpet.h \
	menu.c \
	g_undef.h \
	conf.c \
	process.c \
	other.c \
	interface.inc

gpet_LDFLAGS = \
	-Wl,--export-dynamic \
	`pkg-config --libs-only-L gtk+-2.0` \
	`pkg-config --libs-only-l gtk+-2.0` \
	`pkg-config --libs-only-L gconf-2.0` \
	`pkg-config --libs-only-l gconf-2.0`

gpet_LDADD = $(GPET_LIBS)

EXTRA_DIST = $(ui_DATA) \
	$(icon_DATA)

# Remove ui directory on uninstall
uninstall-local:
#	-rm -r $(uidir)
	-rm -r $(datadir)/gpet
