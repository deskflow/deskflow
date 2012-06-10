
all::
	@subdirs="$(SUBDIRS)"; for d in $$subdirs; do (cd $$d; $(MAKE) $@) || exit 1; done

clean::
	@subdirs="$(SUBDIRS)"; for d in $$subdirs; do (cd $$d; $(MAKE) $@) || exit 1; done

clean::
	rm -f $(program) $(library) *.o

SHELL = @SHELL@
top_srcdir = @top_srcdir@
@SET_MAKE@
CC = @CC@
CFLAGS = @CFLAGS@ $(DIR_CFLAGS)
CCLD = $(CC)
CXX = @CXX@
CXXFLAGS = @CXXFLAGS@
CXXLD = $(CXX)
CPPFLAGS = @CPPFLAGS@
DEFS = @DEFS@
ALL_CPPFLAGS = $(CPPFLAGS) $(DEFS) $(DIR_CPPFLAGS)
LIBS = @LIBS@
LDFLAGS = @LDFLAGS@
RANLIB = @RANLIB@
AR = ar cq

.SUFFIXES:
.SUFFIXES: .cxx .c .o

.c.o:
	$(CC) $(ALL_CPPFLAGS) $(CFLAGS) -c $<

.cxx.o:
	$(CXX) $(ALL_CPPFLAGS) $(CXXFLAGS) -c $<
