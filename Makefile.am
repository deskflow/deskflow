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
	$(NULL)

# build doxygen documentation
doxygen:
	doxygen doc/doxygen.cfg
