DEPTH=..
include $(DEPTH)/Makecommon

#
# target file
#
TARGET = mt

#
# source files
#
LCXXINCS =				\
	-I$(DEPTH)/base 		\
	$(NULL)
CXXFILES = 				\
	CLock.cpp			\
	CMutex.cpp			\
	CCondVar.cpp			\
	CThread.cpp			\
	CThreadRep.cpp			\
	CTimerThread.cpp		\
	$(NULL)

targets: $(LIBTARGET)

$(LIBTARGET): $(OBJECTS)
	if test ! -d $(LIBDIR); then $(MKDIR) $(LIBDIR); fi
	$(ARF) $(LIBTARGET) $(OBJECTS)
