DEPTH=..
include $(DEPTH)/Makecommon

#
# target file
#
TARGET = client

#
# source files
#
LCXXINCS =				\
	-I$(DEPTH)/base 		\
	-I$(DEPTH)/mt	 		\
	-I$(DEPTH)/io	 		\
	-I$(DEPTH)/net	 		\
	-I$(DEPTH)/synergy		\
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

targets: $(TARGET)

$(TARGET): $(OBJECTS) $(DEPLIBS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

