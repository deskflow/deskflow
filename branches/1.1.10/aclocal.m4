# aclocal.m4 generated automatically by aclocal 1.5

# Copyright 1996, 1997, 1998, 1999, 2000, 2001
# Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

dnl synergy -- mouse and keyboard sharing utility
dnl Copyright (C) 2002 Chris Schoeneman
dnl 
dnl This package is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU General Public License
dnl found in the file COPYING that should have accompanied this file.
dnl 
dnl This package is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.

AC_DEFUN([ACX_CHECK_SOCKLEN_T], [
	AC_MSG_CHECKING([for socklen_t])
	AC_TRY_COMPILE([
		#include <unistd.h>
		#include <sys/socket.h>
		],
		[socklen_t len;],[acx_socklen_t_ok=yes],[acx_socklen_t_ok=no])
	AC_MSG_RESULT($acx_socklen_t_ok)
	if test x"$acx_socklen_t_ok" = xyes; then
		ifelse([$1],,AC_DEFINE(HAVE_SOCKLEN_T,1,[Define if your compiler defines socklen_t.]),[$1])
		:
	else
		acx_socklen_t_ok=no
		$2
	fi
])dnl ACX_CHECK_SOCKLEN_T

AC_DEFUN([ACX_CHECK_CXX], [
	AC_MSG_CHECKING([if g++ defines correct C++ macro])
	AC_TRY_COMPILE(, [
		#if defined(_LANGUAGE_C) && !defined(_LANGUAGE_C_PLUS_PLUS)
		#error wrong macro
		#endif],[acx_cxx_macro_ok=yes],[acx_cxx_macro_ok=no])
	AC_MSG_RESULT($acx_cxx_macro_ok)
	if test x"$acx_cxx_macro_ok" = xyes; then
		SYNERGY_CXXFLAGS=""
	else
		SYNERGY_CXXFLAGS="-U_LANGUAGE_C -D_LANGUAGE_C_PLUS_PLUS"
	fi
])dnl ACX_CHECK_CXX

AC_DEFUN([ACX_CHECK_CXX_BOOL], [
	AC_MSG_CHECKING([for bool support])
	AC_TRY_COMPILE(, [bool t = true, f = false;],
		[acx_cxx_bool_ok=yes],[acx_cxx_bool_ok=no])
	AC_MSG_RESULT($acx_cxx_bool_ok)
	if test x"$acx_cxx_bool_ok" = xyes; then
		ifelse([$1],,AC_DEFINE(HAVE_CXX_BOOL,1,[Define if your compiler has bool support.]),[$1])
		:
	else
		acx_cxx_bool_ok=no
		$2
	fi
])dnl ACX_CHECK_CXX_BOOL

AC_DEFUN([ACX_CHECK_CXX_EXCEPTIONS], [
	AC_MSG_CHECKING([for exception support])
	AC_TRY_COMPILE(, [try{throw int(4);}catch(int){throw;}catch(...){}],
		[acx_cxx_exception_ok=yes],[acx_cxx_exception_ok=no])
	AC_MSG_RESULT($acx_cxx_exception_ok)
	if test x"$acx_cxx_exception_ok" = xyes; then
		ifelse([$1],,AC_DEFINE(HAVE_CXX_EXCEPTIONS,1,[Define if your compiler has exceptions support.]),[$1])
		:
	else
		acx_cxx_exception_ok=no
		$2
	fi
])dnl ACX_CHECK_CXX_EXCEPTIONS

AC_DEFUN([ACX_CHECK_CXX_CASTS], [
	AC_MSG_CHECKING([for C++ cast support])
	AC_TRY_COMPILE(, [const char* f="a";const_cast<char*>(f);
		reinterpret_cast<const int*>(f);static_cast<int>(4.5);],
		[acx_cxx_cast_ok=yes],[acx_cxx_cast_ok=no])
	AC_MSG_RESULT($acx_cxx_cast_ok)
	if test x"$acx_cxx_cast_ok" = xyes; then
		ifelse([$1],,AC_DEFINE(HAVE_CXX_CASTS,1,[Define if your compiler has C++ cast support.]),[$1])
		:
	else
		acx_cxx_cast_ok=no
		$2
	fi
])dnl ACX_CHECK_CXX_CASTS

AC_DEFUN([ACX_CHECK_CXX_MUTABLE], [
	AC_MSG_CHECKING([for mutable support])
	AC_TRY_COMPILE(, [struct A{mutable int b;void f() const {b=0;}};
		A a;a.f();],[acx_cxx_mutable_ok=yes],[acx_cxx_mutable_ok=no])
	AC_MSG_RESULT($acx_cxx_mutable_ok)
	if test x"$acx_cxx_mutable_ok" = xyes; then
		ifelse([$1],,AC_DEFINE(HAVE_CXX_MUTABLE,1,[Define if your compiler has mutable support.]),[$1])
		:
	else
		acx_cxx_mutable_ok=no
		$2
	fi
])dnl ACX_CHECK_CXX_MUTABLE

AC_DEFUN([ACX_CHECK_CXX_STDLIB], [
	AC_MSG_CHECKING([for C++ standard library])
	AC_TRY_LINK([#include <set>], [std::set<int> a; a.insert(3);],
		[acx_cxx_stdlib_ok=yes],[acx_cxx_stdlib_ok=no])
	AC_MSG_RESULT($acx_cxx_stdlib_ok)
	if test x"$acx_cxx_stdlib_ok" = xyes; then
		ifelse([$1],,AC_DEFINE(HAVE_CXX_STDLIB,1,[Define if your compiler has standard C++ library support.]),[$1])
		:
	else
		acx_cxx_stdlib_ok=no
		$2
	fi
])dnl ACX_CHECK_CXX_STDLIB

AC_DEFUN([ACX_CHECK_GETPWUID_R], [
	AC_MSG_CHECKING([for working getpwuid_r])
	AC_TRY_LINK([#include <pwd.h>],
		[char buffer[4096]; struct passwd pwd, *pwdp;
		getpwuid_r(0, &pwd, buffer, sizeof(buffer), &pwdp);],
		acx_getpwuid_r_ok=yes, acx_getpwuid_r_ok=no)
	AC_MSG_RESULT($acx_getpwuid_r_ok)
	if test x"$acx_getpwuid_r_ok" = xyes; then
		ifelse([$1],,AC_DEFINE(HAVE_GETPWUID_R,1,[Define if you have a working \`getpwuid_r\' function.]),[$1])
		:
	else
		acx_getpwuid_r_ok=no
		$2
	fi
])dnl ACX_CHECK_GETPWUID_R

AC_DEFUN([ACX_CHECK_POLL], [
    AC_MSG_CHECKING([for poll])
    AC_TRY_LINK([#include <poll.h>],
				[#if defined(_POLL_EMUL_H_)
				#error emulated poll
				#endif
            	struct pollfd ufds[] = { 0, POLLIN, 0 }; poll(ufds, 1, 10);],
            	acx_poll_ok=yes, acx_poll_ok=no)
	AC_MSG_RESULT($acx_poll_ok)
	if test x"$acx_poll_ok" = xyes; then
		ifelse([$1],,AC_DEFINE(HAVE_POLL,1,[Define if you have the \`poll\' function.]),[$1])
		:
	else
		acx_poll_ok=no
		$2
	fi
])dnl ACX_CHECK_POLL

dnl See if we need extra libraries for nanosleep
AC_DEFUN([ACX_CHECK_NANOSLEEP], [
	acx_nanosleep_ok=no
	acx_nanosleep_list=""

	dnl check if user has set NANOSLEEP_LIBS
	save_user_NANOSLEEP_LIBS="$NANOSLEEP_LIBS"
	if test x"$NANOSLEEP_LIBS" != x; then
		acx_nanosleep_list=user
	fi

	dnl check various libraries (including no extra libraries) for
	dnl nanosleep.  `none' should appear first.
	acx_nanosleep_list="none $acx_nanosleep_list rt"
	for flag in $acx_nanosleep_list; do
        case $flag in
            none)
            AC_MSG_CHECKING([for nanosleep])
            NANOSLEEP_LIBS=""
            ;;

            user)
            AC_MSG_CHECKING([for nanosleep in $save_user_NANOSLEEP_LIBS])
			NANOSLEEP_LIBS="$save_user_NANOSLEEP_LIBS"
            ;;

            *)
            AC_MSG_CHECKING([for nanosleep in -l$flag])
            NANOSLEEP_LIBS="-l$flag"
            ;;
        esac

    	save_LIBS="$LIBS"
    	LIBS="$NANOSLEEP_LIBS $LIBS"
    	AC_TRY_LINK([#include <time.h>],
            		[struct timespec t = { 1, 1000 }; nanosleep(&t, NULL);],
            		acx_nanosleep_ok=yes, acx_nanosleep_ok=no)
		LIBS="$save_LIBS"
        AC_MSG_RESULT($acx_nanosleep_ok)
        if test x"$acx_nanosleep_ok" = xyes; then
            break;
        fi
        NANOSLEEP_LIBS=""
	done

	AC_SUBST(NANOSLEEP_LIBS)

	# execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
	if test x"$acx_nanosleep_ok" = xyes; then
        	ifelse([$1],,AC_DEFINE(HAVE_NANOSLEEP,1,[Define if you have the \`nanosleep\' function.]),[$1])
        	:
	else
        	acx_nanosleep_ok=no
        	$2
	fi
])dnl ACX_CHECK_NANOSLEEP

dnl See if we need extra libraries for inet_aton
AC_DEFUN([ACX_CHECK_INET_ATON], [
	acx_inet_aton_ok=no
	acx_inet_aton_list=""

	dnl check if user has set INET_ATON_LIBS
	save_user_INET_ATON_LIBS="$INET_ATON_LIBS"
	if test x"$INET_ATON_LIBS" != x; then
		acx_inet_aton_list=user
	fi

	dnl check various libraries (including no extra libraries) for
	dnl inet_aton.  `none' should appear first.
	acx_inet_aton_list="none $acx_inet_aton_list resolv"
	for flag in $acx_inet_aton_list; do
        case $flag in
            none)
            AC_MSG_CHECKING([for inet_aton])
            INET_ATON_LIBS=""
            ;;

            user)
            AC_MSG_CHECKING([for inet_aton in $save_user_INET_ATON_LIBS])
			INET_ATON_LIBS="$save_user_INET_ATON_LIBS"
            ;;

            *)
            AC_MSG_CHECKING([for inet_aton in -l$flag])
            INET_ATON_LIBS="-l$flag"
            ;;
        esac

    	save_LIBS="$LIBS"
    	LIBS="$INET_ATON_LIBS $LIBS"
    	AC_TRY_LINK([#include <sys/types.h>
					#include <sys/socket.h>
					#include <netinet/in.h>
					#include <arpa/inet.h>],
            		[struct in_addr addr; inet_aton("foo.bar", &addr);],
            		acx_inet_aton_ok=yes, acx_inet_aton_ok=no)
		LIBS="$save_LIBS"
        AC_MSG_RESULT($acx_inet_aton_ok)
        if test x"$acx_inet_aton_ok" = xyes; then
        	AC_DEFINE(HAVE_INET_ATON,1,[Define if you have the \`inet_aton\' function.])
            break;
        fi
        INET_ATON_LIBS=""
	done

	AC_SUBST(INET_ATON_LIBS)
])dnl ACX_CHECK_INET_ATON

dnl The following macros are from http://www.gnu.org/software/ac-archive/
dnl which distributes them under the following license:
dnl
dnl Every Autoconf macro presented on this web site is free software; you can
dnl redistribute it and/or modify it under the terms of the GNU General
dnl Public License as published by the Free Software Foundation; either
dnl version 2, or (at your option) any later version.
dnl 
dnl They are distributed in the hope that they will be useful, but WITHOUT
dnl ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
dnl FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
dnl more details. (You should have received a copy of the GNU General Public
dnl License along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place -- Suite 330, Boston, MA 02111-1307,
dnl USA.)
dnl 
dnl As a special exception, the Free Software Foundation gives unlimited
dnl permission to copy, distribute and modify the configure scripts that are
dnl the output of Autoconf. You need not follow the terms of the GNU General
dnl Public License when using or distributing such scripts, even though
dnl portions of the text of Autoconf appear in them. The GNU General Public
dnl License (GPL) does govern all other use of the material that constitutes
dnl the Autoconf program.
dnl 
dnl Certain portions of the Autoconf source text are designed to be copied
dnl (in certain cases, depending on the input) into the output of Autoconf.
dnl We call these the "data" portions. The rest of the Autoconf source text
dnl consists of comments plus executable code that decides which of the data
dnl portions to output in any given case. We call these comments and
dnl executable code the "non-data" portions. Autoconf never copies any of the
dnl non-data portions into its output.
dnl 
dnl This special exception to the GPL applies to versions of Autoconf
dnl released by the Free Software Foundation. When you make and distribute a
dnl modified version of Autoconf, you may extend this special exception to
dnl the GPL to apply to your modified version as well, *unless* your modified
dnl version has the potential to copy into its output some of the text that
dnl was the non-data portion of the version that you started with. (In other
dnl words, unless your change moves or copies text from the non-data portions
dnl to the data portions.) If your modification has such potential, you must
dnl delete any notice of this special exception to the GPL from your modified
dnl version

AC_DEFUN([ACX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
acx_pthread_ok=no

# We used to check for pthread.h first, but this fails if pthread.h
# requires special compiler flags (e.g. on True64 or Sequent).
# It gets checked for in the link test anyway.

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
        save_CXXFLAGS="$CXXFLAGS"
        CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CXXFLAGS=$PTHREAD_CFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
        AC_MSG_RESULT($acx_pthread_ok)
        if test x"$acx_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
        fi
        LIBS="$save_LIBS"
        CXXFLAGS="$save_CXXFLAGS"
fi

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all.

acx_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
# pthread: Linux, etcetera
# --thread-safe: KAI C++

case "${host_cpu}-${host_os}" in
        *solaris*)

        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthread or
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:

        acx_pthread_flags="-pthread -pthreads pthread -mt $acx_pthread_flags"
        ;;
esac

if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                ;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CXXFLAGS="$CXXFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>],
                    [pthread_t th; pthread_join(th, 0);
                     pthread_attr_init(0); pthread_cleanup_push(0, 0);
                     pthread_create(0,0,0,0); pthread_cleanup_pop(0); ],
                    [acx_pthread_ok=yes])

        LIBS="$save_LIBS"
        CXXFLAGS="$save_CXXFLAGS"

        AC_MSG_RESULT($acx_pthread_ok)
        if test "x$acx_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
done
fi

# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CXXFLAGS="$CXXFLAGS"
        CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"

        # Detect AIX lossage: threads are created detached by default
        # and the JOINABLE attribute has a nonstandard name (UNDETACHED).
        AC_MSG_CHECKING([for joinable pthread attribute])
        AC_TRY_LINK([#include <pthread.h>],
                    [int attr=PTHREAD_CREATE_JOINABLE;],
                    ok=PTHREAD_CREATE_JOINABLE, ok=unknown)
        if test x"$ok" = xunknown; then
                AC_TRY_LINK([#include <pthread.h>],
                            [int attr=PTHREAD_CREATE_UNDETACHED;],
                            ok=PTHREAD_CREATE_UNDETACHED, ok=unknown)
        fi
        if test x"$ok" != xPTHREAD_CREATE_JOINABLE; then
                AC_DEFINE(PTHREAD_CREATE_JOINABLE, $ok,
                          [Define to the necessary symbol if this constant
                           uses a non-standard name on your system.])
        fi
        AC_MSG_RESULT(${ok})
        if test x"$ok" = xunknown; then
                AC_MSG_WARN([we do not know how to create joinable pthreads])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
                *-aix* | *-freebsd*) flag="-D_THREAD_SAFE";;
                alpha*-osf*)         flag="-D_REENTRANT";;
                *solaris*)           flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
                PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

		# Detect POSIX sigwait()
        AC_MSG_CHECKING([for POSIX sigwait])
        AC_TRY_LINK([#include <pthread.h>
					#include <signal.h>],
                    [sigset_t sigset; int signal; sigwait(&sigset, &signal);],
                    ok=yes, ok=unknown)
        if test x"$ok" = xunknown; then
        		save_CXXFLAGS2="$CXXFLAGS"
		        CXXFLAGS="$CXXFLAGS -D_POSIX_PTHREAD_SEMANTICS"
                AC_TRY_LINK([#include <pthread.h>
					#include <signal.h>],
                    [sigset_t sigset; int signal; sigwait(&sigset, &signal);],
                    ok=-D_POSIX_PTHREAD_SEMANTICS, ok=no)
        		CXXFLAGS="$save_CXXFLAGS2"
        fi
        AC_MSG_RESULT(${ok})
        if test x"$ok" != xno; then
        	AC_DEFINE(HAVE_POSIX_SIGWAIT,1,[Define if you have a POSIX \`sigwait\' function.])
        	if test x"$ok" != xyes; then
                PTHREAD_CFLAGS="$ok $PTHREAD_CFLAGS"
			fi
        fi

		# Detect pthread signal functions
        AC_MSG_CHECKING([for pthread signal functions])
        AC_TRY_LINK([#include <pthread.h>
					#include <signal.h>],
                    [pthread_kill(pthread_self(), SIGTERM);],
                    ok=yes, ok=no)
        AC_MSG_RESULT(${ok})
        if test x"$ok" = xyes; then
        	AC_DEFINE(HAVE_PTHREAD_SIGNAL,1,[Define if you have \`pthread_sigmask\' and \`pthread_kill\' functions.])
        fi

        LIBS="$save_LIBS"
        CXXFLAGS="$save_CXXFLAGS"

        # More AIX lossage: must compile with cc_r
        AC_CHECK_PROG(PTHREAD_CC, cc_r, cc_r, ${CC})
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        acx_pthread_ok=no
        $2
fi
])dnl ACX_PTHREAD

dnl enable maximum compiler warnings.  must ignore unknown pragmas to
dnl build on solaris.
dnl we only know how to do this for g++
AC_DEFUN([ACX_CXX_WARNINGS], [
	AC_MSG_CHECKING([for C++ compiler warning flags])
	if test "$GXX" = "yes"; then
		acx_cxx_warnings="-Wall -Wno-unknown-pragmas"
	fi
	if test -n "$acx_cxx_warnings"; then
		CXXFLAGS="$CXXFLAGS $acx_cxx_warnings"
	else
		acx_cxx_warnings="unknown"
	fi
	AC_MSG_RESULT($acx_cxx_warnings)
])dnl ACX_CXX_WARNINGS

dnl enable compiler warnings are errors
dnl we only know how to do this for g++
AC_DEFUN([ACX_CXX_WARNINGS_ARE_ERRORS], [
	AC_MSG_CHECKING([for C++ compiler warning are errors flags])
	if test "$GXX" = "yes"; then
		acx_cxx_warnings_are_errors="-Werror"
	fi
	if test -n "$acx_cxx_warnings_are_errors"; then
		CXXFLAGS="$CXXFLAGS $acx_cxx_warnings_are_errors"
	else
		acx_cxx_warnings_are_errors="unknown"
	fi
	AC_MSG_RESULT($acx_cxx_warnings_are_errors)
])dnl ACX_CXX_WARNINGS_ARE_ERRORS

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 5

# There are a few dirty hacks below to avoid letting `AC_PROG_CC' be
# written in clear, in which case automake, when reading aclocal.m4,
# will think it sees a *use*, and therefore will trigger all it's
# C support machinery.  Also note that it means that autoscan, seeing
# CC etc. in the Makefile, will ask for an AC_PROG_CC use...


# We require 2.13 because we rely on SHELL being computed by configure.
AC_PREREQ([2.13])

# AC_PROVIDE_IFELSE(MACRO-NAME, IF-PROVIDED, IF-NOT-PROVIDED)
# -----------------------------------------------------------
# If MACRO-NAME is provided do IF-PROVIDED, else IF-NOT-PROVIDED.
# The purpose of this macro is to provide the user with a means to
# check macros which are provided without letting her know how the
# information is coded.
# If this macro is not defined by Autoconf, define it here.
ifdef([AC_PROVIDE_IFELSE],
      [],
      [define([AC_PROVIDE_IFELSE],
              [ifdef([AC_PROVIDE_$1],
                     [$2], [$3])])])


# AM_INIT_AUTOMAKE(PACKAGE,VERSION, [NO-DEFINE])
# ----------------------------------------------
AC_DEFUN([AM_INIT_AUTOMAKE],
[AC_REQUIRE([AC_PROG_INSTALL])dnl
# test to see if srcdir already configured
if test "`CDPATH=:; cd $srcdir && pwd`" != "`pwd`" &&
   test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run \"make distclean\" there first])
fi

# Define the identity of the package.
PACKAGE=$1
AC_SUBST(PACKAGE)dnl
VERSION=$2
AC_SUBST(VERSION)dnl
ifelse([$3],,
[AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package])])

# Autoconf 2.50 wants to disallow AM_ names.  We explicitly allow
# the ones we care about.
ifdef([m4_pattern_allow],
      [m4_pattern_allow([^AM_[A-Z]+FLAGS])])dnl

# Autoconf 2.50 always computes EXEEXT.  However we need to be
# compatible with 2.13, for now.  So we always define EXEEXT, but we
# don't compute it.
AC_SUBST(EXEEXT)
# Similar for OBJEXT -- only we only use OBJEXT if the user actually
# requests that it be used.  This is a bit dumb.
: ${OBJEXT=o}
AC_SUBST(OBJEXT)

# Some tools Automake needs.
AC_REQUIRE([AM_SANITY_CHECK])dnl
AC_REQUIRE([AC_ARG_PROGRAM])dnl
AM_MISSING_PROG(ACLOCAL, aclocal)
AM_MISSING_PROG(AUTOCONF, autoconf)
AM_MISSING_PROG(AUTOMAKE, automake)
AM_MISSING_PROG(AUTOHEADER, autoheader)
AM_MISSING_PROG(MAKEINFO, makeinfo)
AM_MISSING_PROG(AMTAR, tar)
AM_PROG_INSTALL_SH
AM_PROG_INSTALL_STRIP
# We need awk for the "check" target.  The system "awk" is bad on
# some platforms.
AC_REQUIRE([AC_PROG_AWK])dnl
AC_REQUIRE([AC_PROG_MAKE_SET])dnl
AC_REQUIRE([AM_DEP_TRACK])dnl
AC_REQUIRE([AM_SET_DEPDIR])dnl
AC_PROVIDE_IFELSE([AC_PROG_][CC],
                  [_AM_DEPENDENCIES(CC)],
                  [define([AC_PROG_][CC],
                          defn([AC_PROG_][CC])[_AM_DEPENDENCIES(CC)])])dnl
AC_PROVIDE_IFELSE([AC_PROG_][CXX],
                  [_AM_DEPENDENCIES(CXX)],
                  [define([AC_PROG_][CXX],
                          defn([AC_PROG_][CXX])[_AM_DEPENDENCIES(CXX)])])dnl
])

#
# Check to make sure that the build environment is sane.
#

# serial 3

# AM_SANITY_CHECK
# ---------------
AC_DEFUN([AM_SANITY_CHECK],
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftest.file
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftest.file 2> /dev/null`
   if test "$[*]" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftest.file`
   fi
   rm -f conftest.file
   if test "$[*]" != "X $srcdir/configure conftest.file" \
      && test "$[*]" != "X conftest.file $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "$[2]" = conftest.file
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
AC_MSG_RESULT(yes)])


# serial 2

# AM_MISSING_PROG(NAME, PROGRAM)
# ------------------------------
AC_DEFUN([AM_MISSING_PROG],
[AC_REQUIRE([AM_MISSING_HAS_RUN])
$1=${$1-"${am_missing_run}$2"}
AC_SUBST($1)])


# AM_MISSING_HAS_RUN
# ------------------
# Define MISSING if not defined so far and test if it supports --run.
# If it does, set am_missing_run to use it, otherwise, to nothing.
AC_DEFUN([AM_MISSING_HAS_RUN],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
test x"${MISSING+set}" = xset || MISSING="\${SHELL} $am_aux_dir/missing"
# Use eval to expand $SHELL
if eval "$MISSING --run true"; then
  am_missing_run="$MISSING --run "
else
  am_missing_run=
  am_backtick='`'
  AC_MSG_WARN([${am_backtick}missing' script is too old or missing])
fi
])

# AM_AUX_DIR_EXPAND

# For projects using AC_CONFIG_AUX_DIR([foo]), Autoconf sets
# $ac_aux_dir to `$srcdir/foo'.  In other projects, it is set to
# `$srcdir', `$srcdir/..', or `$srcdir/../..'.
#
# Of course, Automake must honor this variable whenever it calls a
# tool from the auxiliary directory.  The problem is that $srcdir (and
# therefore $ac_aux_dir as well) can be either absolute or relative,
# depending on how configure is run.  This is pretty annoying, since
# it makes $ac_aux_dir quite unusable in subdirectories: in the top
# source directory, any form will work fine, but in subdirectories a
# relative path needs to be adjusted first.
#
# $ac_aux_dir/missing
#    fails when called from a subdirectory if $ac_aux_dir is relative
# $top_srcdir/$ac_aux_dir/missing
#    fails if $ac_aux_dir is absolute,
#    fails when called from a subdirectory in a VPATH build with
#          a relative $ac_aux_dir
#
# The reason of the latter failure is that $top_srcdir and $ac_aux_dir
# are both prefixed by $srcdir.  In an in-source build this is usually
# harmless because $srcdir is `.', but things will broke when you
# start a VPATH build or use an absolute $srcdir.
#
# So we could use something similar to $top_srcdir/$ac_aux_dir/missing,
# iff we strip the leading $srcdir from $ac_aux_dir.  That would be:
#   am_aux_dir='\$(top_srcdir)/'`expr "$ac_aux_dir" : "$srcdir//*\(.*\)"`
# and then we would define $MISSING as
#   MISSING="\${SHELL} $am_aux_dir/missing"
# This will work as long as MISSING is not called from configure, because
# unfortunately $(top_srcdir) has no meaning in configure.
# However there are other variables, like CC, which are often used in
# configure, and could therefore not use this "fixed" $ac_aux_dir.
#
# Another solution, used here, is to always expand $ac_aux_dir to an
# absolute PATH.  The drawback is that using absolute paths prevent a
# configured tree to be moved without reconfiguration.

AC_DEFUN([AM_AUX_DIR_EXPAND], [
# expand $ac_aux_dir to an absolute path
am_aux_dir=`CDPATH=:; cd $ac_aux_dir && pwd`
])

# AM_PROG_INSTALL_SH
# ------------------
# Define $install_sh.
AC_DEFUN([AM_PROG_INSTALL_SH],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
install_sh=${install_sh-"$am_aux_dir/install-sh"}
AC_SUBST(install_sh)])

# One issue with vendor `install' (even GNU) is that you can't
# specify the program used to strip binaries.  This is especially
# annoying in cross-compiling environments, where the build's strip
# is unlikely to handle the host's binaries.
# Fortunately install-sh will honor a STRIPPROG variable, so we
# always use install-sh in `make install-strip', and initialize
# STRIPPROG with the value of the STRIP variable (set by the user).
AC_DEFUN([AM_PROG_INSTALL_STRIP],
[AC_REQUIRE([AM_PROG_INSTALL_SH])dnl
INSTALL_STRIP_PROGRAM="\${SHELL} \$(install_sh) -c -s"
AC_SUBST([INSTALL_STRIP_PROGRAM])])

# serial 4						-*- Autoconf -*-



# There are a few dirty hacks below to avoid letting `AC_PROG_CC' be
# written in clear, in which case automake, when reading aclocal.m4,
# will think it sees a *use*, and therefore will trigger all it's
# C support machinery.  Also note that it means that autoscan, seeing
# CC etc. in the Makefile, will ask for an AC_PROG_CC use...



# _AM_DEPENDENCIES(NAME)
# ---------------------
# See how the compiler implements dependency checking.
# NAME is "CC", "CXX" or "OBJC".
# We try a few techniques and use that to set a single cache variable.
#
# We don't AC_REQUIRE the corresponding AC_PROG_CC since the latter was
# modified to invoke _AM_DEPENDENCIES(CC); we would have a circular
# dependency, and given that the user is not expected to run this macro,
# just rely on AC_PROG_CC.
AC_DEFUN([_AM_DEPENDENCIES],
[AC_REQUIRE([AM_SET_DEPDIR])dnl
AC_REQUIRE([AM_OUTPUT_DEPENDENCY_COMMANDS])dnl
AC_REQUIRE([AM_MAKE_INCLUDE])dnl
AC_REQUIRE([AM_DEP_TRACK])dnl

ifelse([$1], CC,   [depcc="$CC"   am_compiler_list=],
       [$1], CXX,  [depcc="$CXX"  am_compiler_list=],
       [$1], OBJC, [depcc="$OBJC" am_compiler_list='gcc3 gcc']
       [$1], GCJ,  [depcc="$GCJ"  am_compiler_list='gcc3 gcc'],
                   [depcc="$$1"   am_compiler_list=])

AC_CACHE_CHECK([dependency style of $depcc],
               [am_cv_$1_dependencies_compiler_type],
[if test -z "$AMDEP_TRUE" && test -f "$am_depcomp"; then
  # We make a subdir and do the tests there.  Otherwise we can end up
  # making bogus files that we don't know about and never remove.  For
  # instance it was reported that on HP-UX the gcc test will end up
  # making a dummy file named `D' -- because `-MD' means `put the output
  # in D'.
  mkdir conftest.dir
  # Copy depcomp to subdir because otherwise we won't find it if we're
  # using a relative directory.
  cp "$am_depcomp" conftest.dir
  cd conftest.dir

  am_cv_$1_dependencies_compiler_type=none
  if test "$am_compiler_list" = ""; then
     am_compiler_list=`sed -n ['s/^#*\([a-zA-Z0-9]*\))$/\1/p'] < ./depcomp`
  fi
  for depmode in $am_compiler_list; do
    # We need to recreate these files for each test, as the compiler may
    # overwrite some of them when testing with obscure command lines.
    # This happens at least with the AIX C compiler.
    echo '#include "conftest.h"' > conftest.c
    echo 'int i;' > conftest.h
    echo "${am__include} ${am__quote}conftest.Po${am__quote}" > confmf

    case $depmode in
    nosideeffect)
      # after this tag, mechanisms are not by side-effect, so they'll
      # only be used when explicitly requested
      if test "x$enable_dependency_tracking" = xyes; then
	continue
      else
	break
      fi
      ;;
    none) break ;;
    esac
    # We check with `-c' and `-o' for the sake of the "dashmstdout"
    # mode.  It turns out that the SunPro C++ compiler does not properly
    # handle `-M -o', and we need to detect this.
    if depmode=$depmode \
       source=conftest.c object=conftest.o \
       depfile=conftest.Po tmpdepfile=conftest.TPo \
       $SHELL ./depcomp $depcc -c conftest.c -o conftest.o >/dev/null 2>&1 &&
       grep conftest.h conftest.Po > /dev/null 2>&1 &&
       ${MAKE-make} -s -f confmf > /dev/null 2>&1; then
      am_cv_$1_dependencies_compiler_type=$depmode
      break
    fi
  done

  cd ..
  rm -rf conftest.dir
else
  am_cv_$1_dependencies_compiler_type=none
fi
])
$1DEPMODE="depmode=$am_cv_$1_dependencies_compiler_type"
AC_SUBST([$1DEPMODE])
])


# AM_SET_DEPDIR
# -------------
# Choose a directory name for dependency files.
# This macro is AC_REQUIREd in _AM_DEPENDENCIES
AC_DEFUN([AM_SET_DEPDIR],
[rm -f .deps 2>/dev/null
mkdir .deps 2>/dev/null
if test -d .deps; then
  DEPDIR=.deps
else
  # MS-DOS does not allow filenames that begin with a dot.
  DEPDIR=_deps
fi
rmdir .deps 2>/dev/null
AC_SUBST(DEPDIR)
])


# AM_DEP_TRACK
# ------------
AC_DEFUN([AM_DEP_TRACK],
[AC_ARG_ENABLE(dependency-tracking,
[  --disable-dependency-tracking Speeds up one-time builds
  --enable-dependency-tracking  Do not reject slow dependency extractors])
if test "x$enable_dependency_tracking" != xno; then
  am_depcomp="$ac_aux_dir/depcomp"
  AMDEPBACKSLASH='\'
fi
AM_CONDITIONAL([AMDEP], [test "x$enable_dependency_tracking" != xno])
pushdef([subst], defn([AC_SUBST]))
subst(AMDEPBACKSLASH)
popdef([subst])
])

# Generate code to set up dependency tracking.
# This macro should only be invoked once -- use via AC_REQUIRE.
# Usage:
# AM_OUTPUT_DEPENDENCY_COMMANDS

#
# This code is only required when automatic dependency tracking
# is enabled.  FIXME.  This creates each `.P' file that we will
# need in order to bootstrap the dependency handling code.
AC_DEFUN([AM_OUTPUT_DEPENDENCY_COMMANDS],[
AC_OUTPUT_COMMANDS([
test x"$AMDEP_TRUE" != x"" ||
for mf in $CONFIG_FILES; do
  case "$mf" in
  Makefile) dirpart=.;;
  */Makefile) dirpart=`echo "$mf" | sed -e 's|/[^/]*$||'`;;
  *) continue;;
  esac
  grep '^DEP_FILES *= *[^ #]' < "$mf" > /dev/null || continue
  # Extract the definition of DEP_FILES from the Makefile without
  # running `make'.
  DEPDIR=`sed -n -e '/^DEPDIR = / s///p' < "$mf"`
  test -z "$DEPDIR" && continue
  # When using ansi2knr, U may be empty or an underscore; expand it
  U=`sed -n -e '/^U = / s///p' < "$mf"`
  test -d "$dirpart/$DEPDIR" || mkdir "$dirpart/$DEPDIR"
  # We invoke sed twice because it is the simplest approach to
  # changing $(DEPDIR) to its actual value in the expansion.
  for file in `sed -n -e '
    /^DEP_FILES = .*\\\\$/ {
      s/^DEP_FILES = //
      :loop
	s/\\\\$//
	p
	n
	/\\\\$/ b loop
      p
    }
    /^DEP_FILES = / s/^DEP_FILES = //p' < "$mf" | \
       sed -e 's/\$(DEPDIR)/'"$DEPDIR"'/g' -e 's/\$U/'"$U"'/g'`; do
    # Make sure the directory exists.
    test -f "$dirpart/$file" && continue
    fdir=`echo "$file" | sed -e 's|/[^/]*$||'`
    $ac_aux_dir/mkinstalldirs "$dirpart/$fdir" > /dev/null 2>&1
    # echo "creating $dirpart/$file"
    echo '# dummy' > "$dirpart/$file"
  done
done
], [AMDEP_TRUE="$AMDEP_TRUE"
ac_aux_dir="$ac_aux_dir"])])

# AM_MAKE_INCLUDE()
# -----------------
# Check to see how make treats includes.
AC_DEFUN([AM_MAKE_INCLUDE],
[am_make=${MAKE-make}
cat > confinc << 'END'
doit:
	@echo done
END
# If we don't find an include directive, just comment out the code.
AC_MSG_CHECKING([for style of include used by $am_make])
am__include='#'
am__quote=
_am_result=none
# First try GNU make style include.
echo "include confinc" > confmf
# We grep out `Entering directory' and `Leaving directory'
# messages which can occur if `w' ends up in MAKEFLAGS.
# In particular we don't look at `^make:' because GNU make might
# be invoked under some other name (usually "gmake"), in which
# case it prints its new name instead of `make'.
if test "`$am_make -s -f confmf 2> /dev/null | fgrep -v 'ing directory'`" = "done"; then
   am__include=include
   am__quote=
   _am_result=GNU
fi
# Now try BSD make style include.
if test "$am__include" = "#"; then
   echo '.include "confinc"' > confmf
   if test "`$am_make -s -f confmf 2> /dev/null`" = "done"; then
      am__include=.include
      am__quote='"'
      _am_result=BSD
   fi
fi
AC_SUBST(am__include)
AC_SUBST(am__quote)
AC_MSG_RESULT($_am_result)
rm -f confinc confmf
])

# serial 3

# AM_CONDITIONAL(NAME, SHELL-CONDITION)
# -------------------------------------
# Define a conditional.
#
# FIXME: Once using 2.50, use this:
# m4_match([$1], [^TRUE\|FALSE$], [AC_FATAL([$0: invalid condition: $1])])dnl
AC_DEFUN([AM_CONDITIONAL],
[ifelse([$1], [TRUE],
        [errprint(__file__:__line__: [$0: invalid condition: $1
])dnl
m4exit(1)])dnl
ifelse([$1], [FALSE],
       [errprint(__file__:__line__: [$0: invalid condition: $1
])dnl
m4exit(1)])dnl
AC_SUBST([$1_TRUE])
AC_SUBST([$1_FALSE])
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi])

# Like AC_CONFIG_HEADER, but automatically create stamp file.

# serial 3

# When config.status generates a header, we must update the stamp-h file.
# This file resides in the same directory as the config header
# that is generated.  We must strip everything past the first ":",
# and everything past the last "/".

AC_PREREQ([2.12])

AC_DEFUN([AM_CONFIG_HEADER],
[ifdef([AC_FOREACH],dnl
	 [dnl init our file count if it isn't already
	 m4_ifndef([_AM_Config_Header_Index], m4_define([_AM_Config_Header_Index], [0]))
	 dnl prepare to store our destination file list for use in config.status
	 AC_FOREACH([_AM_File], [$1],
		    [m4_pushdef([_AM_Dest], m4_patsubst(_AM_File, [:.*]))
		    m4_define([_AM_Config_Header_Index], m4_incr(_AM_Config_Header_Index))
		    dnl and add it to the list of files AC keeps track of, along
		    dnl with our hook
		    AC_CONFIG_HEADERS(_AM_File,
dnl COMMANDS, [, INIT-CMDS]
[# update the timestamp
echo timestamp >"AS_ESCAPE(_AM_DIRNAME(]_AM_Dest[))/stamp-h]_AM_Config_Header_Index["
][$2]m4_ifval([$3], [, [$3]]))dnl AC_CONFIG_HEADERS
		    m4_popdef([_AM_Dest])])],dnl
[AC_CONFIG_HEADER([$1])
  AC_OUTPUT_COMMANDS(
   ifelse(patsubst([$1], [[^ ]], []),
	  [],
	  [test -z "$CONFIG_HEADERS" || echo timestamp >dnl
	   patsubst([$1], [^\([^:]*/\)?.*], [\1])stamp-h]),dnl
[am_indx=1
for am_file in $1; do
  case " \$CONFIG_HEADERS " in
  *" \$am_file "*)
    am_dir=\`echo \$am_file |sed 's%:.*%%;s%[^/]*\$%%'\`
    if test -n "\$am_dir"; then
      am_tmpdir=\`echo \$am_dir |sed 's%^\(/*\).*\$%\1%'\`
      for am_subdir in \`echo \$am_dir |sed 's%/% %'\`; do
        am_tmpdir=\$am_tmpdir\$am_subdir/
        if test ! -d \$am_tmpdir; then
          mkdir \$am_tmpdir
        fi
      done
    fi
    echo timestamp > "\$am_dir"stamp-h\$am_indx
    ;;
  esac
  am_indx=\`expr \$am_indx + 1\`
done])
])]) # AM_CONFIG_HEADER

# _AM_DIRNAME(PATH)
# -----------------
# Like AS_DIRNAME, only do it during macro expansion
AC_DEFUN([_AM_DIRNAME],
       [m4_if(m4_regexp([$1], [^.*[^/]//*[^/][^/]*/*$]), -1,
	      m4_if(m4_regexp([$1], [^//\([^/]\|$\)]), -1,
		    m4_if(m4_regexp([$1], [^/.*]), -1,
			  [.],
			  m4_patsubst([$1], [^\(/\).*], [\1])),
		    m4_patsubst([$1], [^\(//\)\([^/].*\|$\)], [\1])),
	      m4_patsubst([$1], [^\(.*[^/]\)//*[^/][^/]*/*$], [\1]))[]dnl
]) # _AM_DIRNAME

