DEPTH=..
include $(DEPTH)/Makecommon

#
# target file
#
TARGET = base

#
# source files
#
LCXXINCS =				\
	$(NULL)
CXXFILES = 				\
	XBase.cpp			\
	CLog.cpp			\
	CFunctionJob.cpp		\
	CStopwatch.cpp			\
	CString.cpp			\
	$(NULL)

targets: $(LIBTARGET)

$(LIBTARGET): $(OBJECTS)
	if test ! -d $(LIBDIR); then $(MKDIR) $(LIBDIR); fi
	$(ARF) $(LIBTARGET) $(OBJECTS)
