## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..
VDEPTH = ./$(VPATH)/$(DEPTH)

EXTRA_DIST =				\
	mt.dsp					\
	$(NULL)

MAINTAINERCLEANFILES =		\
	Makefile.in				\
	$(NULL)

noinst_LIBRARIES = libmt.a
libmt_a_SOURCES = 			\
	CLock.cpp				\
	CMutex.cpp				\
	CCondVar.cpp			\
	CThread.cpp				\
	CThreadRep.cpp			\
	CTimerThread.cpp		\
	CCondVar.h				\
	CLock.h					\
	CMutex.h				\
	CThread.h				\
	CThreadRep.h			\
	CTimerThread.h			\
	XThread.h				\
	$(NULL)
INCLUDES =					\
	-I$(VDEPTH)/lib/base 	\
	$(NULL)
