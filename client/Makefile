DEPTH=..
include $(DEPTH)/Makecommon

#
# target file
#
TARGET = synergy

#
# source files
#
LCXXINCS =				\
	-I$(DEPTH)/base 		\
	-I$(DEPTH)/mt	 		\
	-I$(DEPTH)/io	 		\
	-I$(DEPTH)/net	 		\
	-I$(DEPTH)/synergy		\
	-I$(DEPTH)/platform 		\
	$(NULL)
CXXFILES = 				\
	CXWindowsSecondaryScreen.cpp	\
	CClient.cpp			\
	client.cpp			\
	$(NULL)

#
# libraries we depend on
#
DEPLIBS =				\
	$(LIBDIR)/libplatform.a		\
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

targets: $(BINTARGET)

$(BINTARGET): $(OBJECTS) $(DEPLIBS)
	if test ! -d $(BINDIR); then $(MKDIR) $(BINDIR); fi
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

