# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2002 Chris Schoeneman
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file COPYING that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

## Process this file with automake to produce Makefile.in
NULL =
DEPTH = .
VDEPTH = ./$(VPATH)

SUBDIRS =					\
	lib						\
	cmd						\
	dist					\
	$(NULL)

EXTRA_DIST =				\
	BUGS					\
	FAQ						\
	HISTORY					\
	PORTING					\
	TODO					\
	all.dsp					\
	synergy.dsw				\
	doc/doxygen.cfg.in		\
	examples/synergy.conf	\
	$(NULL)

MAINTAINERCLEANFILES =		\
	Makefile.in				\
	aclocal.m4				\
	config.h				\
	config.h.in				\
	config.log				\
	config.status			\
	configure				\
	stamp-h.in				\
	stamp-h1				\
	doc/doxygen.cfg			\
	doc/doxygen/html/*		\
	$(NULL)

# build doxygen documentation
doxygen:
	doxygen doc/doxygen.cfg

# build RPMs
RPMTOPDIR=/var/tmp/@PACKAGE@-@VERSION@
dist-rpm: dist
	rm -rf $(RPMTOPDIR)
	mkdir $(RPMTOPDIR)
	(cd $(RPMTOPDIR); mkdir BUILD SOURCES SPECS SRPMS RPMS)
	cp @PACKAGE@-@VERSION@.tar.gz $(RPMTOPDIR)/SOURCES
	rpm --define '_topdir $(RPMTOPDIR)' -ba dist/rpm/synergy.spec && \
		mv -f $(RPMTOPDIR)/SRPMS/*.rpm . && \
		mv -f $(RPMTOPDIR)/RPMS/*/*.rpm . && \
		rm -rf $(RPMTOPDIR)

# build zip
# FIXME -- have automake generate this rule for us
dist-zip: distdir
	zip -r $(distdir).zip $(distdir)
	-chmod -R a+w $(distdir) >/dev/null 2>&1; rm -rf $(distdir)
