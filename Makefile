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
	net				\
	synergy 			\
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

#
#
# test
#
#

#
# source files
#
LCXXINCS =				\
	-I$(DEPTH)/base 		\
	-I$(DEPTH)/mt	 		\
	-I$(DEPTH)/io	 		\
	-I$(DEPTH)/net	 		\
	-I$(DEPTH)/synergy 		\
	$(NULL)
CXXFILES = test.cpp

#
# libraries we depend on
#
DEPLIBS =				\
	$(LIBDIR)/libsynergy.a		\
	$(LIBDIR)/libnet.a		\
	$(LIBDIR)/libio.a		\
	$(LIBDIR)/libmt.a		\
	$(LIBDIR)/libbase.a		\
	$(NULL)
LLDLIBS  =				\
	$(DEPLIBS)			\
	-lpthread			\
	$(NULL)

test: $(OBJECTS) $(DEPLIBS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

