NULL=
DEPTH=..\..

NSIS="D:\Program Files\NSIS\makensis"

DOCS =						\
	ChangeLog				\
	$(NULL)

default: dosifydocs installer

installer:
	$(NSIS) synergy.nsi

dosifydocs: dosify.exe
	.\dosify.exe $(DEPTH) . $(DOCS)

dosify.exe: dosify.c
	$(CC) /nologo /Yd /Zi /MLd /Fe$@ $**
