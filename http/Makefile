DEPTH=..
include $(DEPTH)/Makecommon

#
# target file
#
TARGET = http

#
# source files
#
LCXXINCS =				\
	-I$(DEPTH)/base 		\
	-I$(DEPTH)/mt 			\
	-I$(DEPTH)/io 			\
	$(NULL)
CXXFILES = 				\
	XHTTP.cpp			\
	CHTTPProtocol.cpp		\
	$(NULL)

targets: $(LIBTARGET)

$(LIBTARGET): $(OBJECTS) $(DEPLIBS)
	if test ! -d $(LIBDIR); then $(MKDIR) $(LIBDIR); fi
	$(ARF) $(LIBTARGET) $(OBJECTS)
