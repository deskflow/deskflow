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

SUBDIRS =							\
	lib								\
	cmd								\
	doc								\
	dist							\
	$(NULL)

EXTRA_DIST =						\
	all.dsp							\
	synergy.dsw						\
	synergy.xcode/project.pbxproj	\
	examples/synergy.conf			\
	$(NULL)

MAINTAINERCLEANFILES =				\
	Makefile.in						\
	aclocal.m4						\
	config.h						\
	config.h.in						\
	config.log						\
	config.status					\
	configure						\
	stamp-h.in						\
	stamp-h1						\
	$(NULL)

PKG_FILES =							\
	ChangeLog						\
	README							\
	cmd/synergyc/synergyc			\
	cmd/synergys/synergys			\
	examples/synergy.conf			\
	$(NULL)
PKG_DOC_FILES =						\
	doc/PORTING						\
	doc/*.html						\
	doc/*.css						\
	$(NULL)
PKG_PROG_FILES =					\
	synergyc						\
	synergys						\
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

# build binary package.  owner/group of packaged files will be
# owner/group of user running make.
PKGTOPDIR=/var/tmp/@PACKAGE@-@VERSION@
dist-pkg: all
	rm -rf $(PKGTOPDIR)
	mkdir $(PKGTOPDIR)
	mkdir $(PKGTOPDIR)/@PACKAGE@-@VERSION@
	mkdir $(PKGTOPDIR)/@PACKAGE@-@VERSION@/doc
	cp $(PKG_FILES) $(PKGTOPDIR)/@PACKAGE@-@VERSION@
	cp $(PKG_DOC_FILES) $(PKGTOPDIR)/@PACKAGE@-@VERSION@/doc
	(cd $(PKGTOPDIR)/@PACKAGE@-@VERSION@; \
		chmod 644 *; \
		chmod 755 doc $(PKG_PROG_FILES); \
		strip $(PKG_PROG_FILES) )
	type=`uname -s -m | tr '[A-Z] ' '[a-z].'`; \
	(cd $(PKGTOPDIR); tar cf - @PACKAGE@-@VERSION@ | \
		gzip - ) > @PACKAGE@-@VERSION@-1.$${type}.tar.gz && \
	rm -rf $(PKGTOPDIR)
