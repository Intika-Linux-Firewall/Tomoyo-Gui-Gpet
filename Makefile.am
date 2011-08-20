## Process this file with automake to produce Makefile.in
## Created by Anjuta

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src po

gpetdocdir = ${prefix}/share/doc/gpet
gpetdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	ChangeLog.ja\
	INSTALL\
	NEWS\
	gpetrc.sample


INTLTOOL_FILES = intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

EXTRA_DIST = $(gpetdoc_DATA) \
	$(INTLTOOL_FILES)

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache


# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(gpetdocdir)
