## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..

EXTRA_DIST =								\
	makehook.dsp							\
	platform.dsp							\
	synrgyhk.dsp							\
	CMSWindowsClipboard.cpp					\
	CMSWindowsClipboardAnyTextConverter.cpp	\
	CMSWindowsClipboardTextConverter.cpp	\
	CMSWindowsClipboardUTF16Converter.cpp	\
	CMSWindowsScreen.cpp					\
	CMSWindowsScreenSaver.cpp				\
	CSynergyHook.cpp						\
	CWin32Platform.cpp						\
	CMSWindowsClipboard.h					\
	CMSWindowsClipboardAnyTextConverter.h	\
	CMSWindowsClipboardTextConverter.h		\
	CMSWindowsClipboardUTF16Converter.h		\
	CMSWindowsScreen.h						\
	CMSWindowsScreenSaver.h					\
	CSynergyHook.h							\
	CWin32Platform.h						\
	IMSWindowsScreenEventHandler.h			\
	$(NULL)

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
EXTRA_libplatform_a_SOURCES =				\
	CUnixPlatform.cpp						\
	$(NULL)
INCLUDES =									\
	-I$(DEPTH)/lib/base 					\
	-I$(DEPTH)/lib/mt	 					\
	-I$(DEPTH)/lib/synergy 					\
	$(NULL)
