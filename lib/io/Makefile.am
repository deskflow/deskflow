## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..

EXTRA_DIST =				\
	io.dsp					\
	$(NULL)

noinst_LIBRARIES = libio.a
libio_a_SOURCES = 				\
	CBufferedInputStream.cpp	\
	CBufferedOutputStream.cpp	\
	CInputStreamFilter.cpp		\
	COutputStreamFilter.cpp		\
	CStreamBuffer.cpp			\
	XIO.cpp						\
	CBufferedInputStream.h		\
	CBufferedOutputStream.h		\
	CInputStreamFilter.h		\
	COutputStreamFilter.h		\
	CStreamBuffer.h				\
	IInputStream.h				\
	IOutputStream.h				\
	IStreamFilterFactory.h		\
	XIO.h						\
	$(NULL)
INCLUDES =						\
	-I$(DEPTH)/lib/base 		\
	-I$(DEPTH)/lib/mt	 		\
	$(NULL)
