DEPTH=..
include $(DEPTH)/Makecommon

#
# target file
#
TARGET = io

#
# source files
#
LCXXINCS =				\
	-I$(DEPTH)/base 		\
	-I$(DEPTH)/mt 			\
	$(NULL)
CXXFILES = 				\
	XIO.cpp				\
	CInputStreamFilter.cpp		\
	COutputStreamFilter.cpp		\
	CStreamBuffer.cpp		\
	CBufferedInputStream.cpp	\
	CBufferedOutputStream.cpp	\
	$(NULL)

targets: $(LIBTARGET)

$(LIBTARGET): $(OBJECTS) $(DEPLIBS)
	if test ! -d $(LIBDIR); then $(MKDIR) $(LIBDIR); fi
	$(ARF) $(LIBTARGET) $(OBJECTS)
