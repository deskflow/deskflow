## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..

# FIXME -- add CUnixPlatform.cpp as an unbuilt source
noinst_LIBRARIES = libplatform.a
libplatform_a_SOURCES = 					\
	CPlatform.cpp							\
	CXWindowsClipboard.cpp					\
	CXWindowsClipboardTextConverter.cpp		\
	CXWindowsClipboardUCS2Converter.cpp		\
	CXWindowsClipboardUTF8Converter.cpp		\
	CXWindowsScreen.cpp						\
	CXWindowsScreenSaver.cpp				\
	CXWindowsUtil.cpp						\
	CPlatform.h								\
	CUnixPlatform.h							\
	CXWindowsClipboard.h					\
	CXWindowsClipboardTextConverter.h		\
	CXWindowsClipboardUCS2Converter.h		\
	CXWindowsClipboardUTF8Converter.h		\
	CXWindowsScreen.h						\
	CXWindowsScreenSaver.h					\
	CXWindowsUtil.h							\
	IPlatform.h								\
	$(NULL)
INCLUDES =									\
	-I$(DEPTH)/lib/base 					\
	-I$(DEPTH)/lib/mt	 					\
	-I$(DEPTH)/lib/synergy 					\
	$(NULL)
