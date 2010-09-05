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
    AC_TRY_LINK([#include <sys/poll.h>],
            	[struct pollfd ufds[] = { 0, POLLIN, 0 }; poll(ufds, 1, 10);],
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
