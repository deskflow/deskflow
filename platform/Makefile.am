## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ..

# FIXME -- add CUnixPlatform.cpp as an unbuilt source
noinst_LIBRARIES = libplatform.a
libplatform_a_SOURCES = 	\
	CPlatform.cpp			\
	CXWindowsClipboard.cpp	\
	CXWindowsScreen.cpp		\
	CXWindowsUtil.cpp		\
	CPlatform.h				\
	CUnixPlatform.h			\
	CXWindowsClipboard.h	\
	CXWindowsScreen.h		\
	CXWindowsUtil.h			\
	IPlatform.h				\
	$(NULL)
INCLUDES =					\
	-I$(DEPTH)/base 		\
	-I$(DEPTH)/mt	 		\
	-I$(DEPTH)/synergy 		\
	$(NULL)
