DEPTH=.
COMMONPREF = root
include Makecommon

#
# subdirectories
#
SUBDIRS = 				\
	base				\
	mt				\
	io				\
	http				\
	net				\
	synergy 			\
	platform			\
	client				\
	server				\
	$(NULL)

#
# targets
#

default: $(COMMONPREF)_force
	$(SUBDIRS_MAKERULE)

all targets: default

clean:
	$(RMR) $(LIBDIR)
	$(SUBDIRS_MAKERULE)

clobber:
	$(RMR) $(LIBDIR)
	$(SUBDIRS_MAKERULE)
