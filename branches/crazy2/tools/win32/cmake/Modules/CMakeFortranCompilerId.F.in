      PROGRAM CMakeFortranCompilerId
#if 0
! Identify the compiler
#endif
#if defined(__INTEL_COMPILER) || defined(__ICC)
        PRINT *, 'INFO:compiler[Intel]'
#elif defined(__SUNPRO_F90) || defined(__SUNPRO_F95)
        PRINT *, 'INFO:compiler[SunPro]'
#elif defined(__G95__)
        PRINT *, 'INFO:compiler[G95]'
#elif defined(__GNUC__)
        PRINT *, 'INFO:compiler[GNU]'
#elif defined(__IBMC__)
# if defined(__COMPILER_VER__)
        PRINT *, 'INFO:compiler[zOS]'
# elif __IBMC__ >= 800
        PRINT *, 'INFO:compiler[XL]'
# else
        PRINT *, 'INFO:compiler[VisualAge]'
# endif
#elif defined(__PGI)
        PRINT *, 'INFO:compiler[PGI]'
#elif defined(_SGI_COMPILER_VERSION) || defined(_COMPILER_VERSION)
        PRINT *, 'INFO:compiler[MIPSpro]'
#       if 0
!       This compiler is either not known or is too old to define an
!       identification macro.  Try to identify the platform and guess that
!       it is the native compiler.
#       endif
#elif defined(_AIX) || defined(__AIX) || defined(__AIX__) || defined(__aix) || defined(__aix__)
        PRINT *, 'INFO:compiler[VisualAge]'
#elif defined(__sgi) || defined(__sgi__) || defined(_SGI)
        PRINT *, 'INFO:compiler[MIPSpro]'
#elif defined(__hpux) || defined(__hpux__)
        PRINT *, 'INFO:compiler[HP]'
#elif 1
#       if 0
!       The above 'elif 1' instead of 'else' is to work around a bug in the
!       SGI preprocessor which produces both the __sgi and else blocks.
#       endif
        PRINT *, 'INFO:compiler[]'
#endif

#if 0
! Identify the platform
#endif
#if defined(__linux) || defined(__linux__) || defined(linux)
        PRINT *, 'INFO:platform[Linux]'
#elif defined(__CYGWIN__)
        PRINT *, 'INFO:platform[Cygwin]'
#elif defined(__MINGW32__)
        PRINT *, 'INFO:platform[MinGW]'
#elif defined(__APPLE__)
        PRINT *, 'INFO:platform[Darwin]'
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
        PRINT *, 'INFO:platform[Windows]'
#elif defined(__FreeBSD__) || defined(__FreeBSD)
        PRINT *, 'INFO:platform[FreeBSD]'
#elif defined(__NetBSD__) || defined(__NetBSD)
        PRINT *, 'INFO:platform[NetBSD]'
#elif defined(__OpenBSD__) || defined(__OPENBSD)
        PRINT *, 'INFO:platform[OpenBSD]'
#elif defined(__sun) || defined(sun)
        PRINT *, 'INFO:platform[SunOS]'
#elif defined(_AIX) || defined(__AIX) || defined(__AIX__) || defined(__aix) || defined(__aix__)
        PRINT *, 'INFO:platform[AIX]'
#elif defined(__sgi) || defined(__sgi__) || defined(_SGI)
        PRINT *, 'INFO:platform[IRIX]'
#elif defined(__hpux) || defined(__hpux__)
        PRINT *, 'INFO:platform[HP-UX]'
#elif defined(__HAIKU) || defined(__HAIKU__) || defined(_HAIKU)
        PRINT *, 'INFO:platform[Haiku]'
#       if 0
!       Haiku also defines __BEOS__ so we must
!       put it prior to the check for __BEOS__
#       endif
#elif defined(__BeOS) || defined(__BEOS__) || defined(_BEOS)
        PRINT *, 'INFO:platform[BeOS]'
#elif defined(__QNX__) || defined(__QNXNTO__)
        PRINT *, 'INFO:platform[QNX]'
#elif defined(__tru64) || defined(_tru64) || defined(__TRU64__)
        PRINT *, 'INFO:platform[Tru64]'
#elif defined(__riscos) || defined(__riscos__)
        PRINT *, 'INFO:platform[RISCos]'
#elif defined(__sinix) || defined(__sinix__) || defined(__SINIX__)
        PRINT *, 'INFO:platform[SINIX]'
#elif defined(__UNIX_SV__)
        PRINT *, 'INFO:platform[UNIX_SV]'
#elif defined(__bsdos__)
        PRINT *, 'INFO:platform[BSDOS]'
#elif defined(_MPRAS) || defined(MPRAS)
        PRINT *, 'INFO:platform[MP-RAS]'
#elif defined(__osf) || defined(__osf__)
        PRINT *, 'INFO:platform[OSF1]'
#elif defined(_SCO_SV) || defined(SCO_SV) || defined(sco_sv)
        PRINT *, 'INFO:platform[SCO_SV]'
#elif defined(__ultrix) || defined(__ultrix__) || defined(_ULTRIX)
        PRINT *, 'INFO:platform[ULTRIX]'
#elif defined(__XENIX__) || defined(_XENIX) || defined(XENIX)
        PRINT *, 'INFO:platform[Xenix]'
#elif 1
#       if 0
!       The above 'elif 1' instead of 'else' is to work around a bug in the
!       SGI preprocessor which produces both the __sgi and else blocks.
#       endif
        PRINT *, 'INFO:platform[]'
#endif
      END
