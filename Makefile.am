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
	all.dsp					\
	synergy.dsw				\
	doc/doxygen.cfg			\
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
