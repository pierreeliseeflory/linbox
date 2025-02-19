dnl Check for NTL
dnl Bradford Hovinen, 2001-06-13
dnl Modified by Pascal Giorgi, 2003-12-03
dnl Inspired by gnome-bonobo-check.m4 by Miguel de Icaza, 99-04-12
dnl Stolen from Chris Lahey       99-2-5
dnl stolen from Manish Singh again
dnl stolen back from Frank Belew
dnl stolen from Manish Singh
dnl Shamelessly stolen from Owen Taylor
dnl Copyright (c) the LinBox group
dnl This file is part of LinBox

 dnl ========LICENCE========
 dnl This file is part of the library LinBox.
 dnl 
 dnl LinBox is free software: you can redistribute it and/or modify
 dnl it under the terms of the  GNU Lesser General Public
 dnl License as published by the Free Software Foundation; either
 dnl version 2.1 of the License, or (at your option) any later version.
 dnl 
 dnl This library is distributed in the hope that it will be useful,
 dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
 dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 dnl Lesser General Public License for more details.
 dnl 
 dnl You should have received a copy of the GNU Lesser General Public
 dnl License along with this library; if not, write to the Free Software
 dnl Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 dnl ========LICENCE========
 dnl


dnl LB_CHECK_NTL ([MINIMUM-VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl
dnl Test for Victor Shoup's NTL (Number Theory Library) and define
dnl NTL_CFLAGS and NTL_LIBS

AC_DEFUN([LB_CHECK_NTL],
[

AC_ARG_WITH(ntl,
[AC_HELP_STRING([--with-ntl=<path>|yes|no], [Use NTL library. If argument is no, you do not have
                            the library installed on your machine (set as
			    default). If argument is yes or <empty> that means
			    the library is reachable with the standard search
			    path (/usr or /usr/local). Otherwise you give the
			    <path> to the directory which contain the library.
	     ])],
	     [if test "$withval" = yes ; then
			NTL_HOME_PATH="${DEFAULT_CHECKING_PATH}"
	      elif test "$withval" != no ; then
			NTL_HOME_PATH="$withval ${DEFAULT_CHECKING_PATH}"
	     fi],
	     [NTL_HOME_PATH=${DEFAULT_CHECKING_PATH}])

min_ntl_version=ifelse([$1], ,5.0,$1)


dnl Check for existence
BACKUP_CXXFLAGS=${CXXFLAGS}
BACKUP_LIBS=${LIBS}

if test -n "$NTL_HOME_PATH"; then
AC_MSG_CHECKING(for NTL >= $min_ntl_version)
fi

for NTL_HOME in ${NTL_HOME_PATH}
 do
if test -r "$NTL_HOME/include/NTL/ZZ.h"; then

	if test "x$NTL_HOME" != "x/usr" -a "x$NTL_HOME" != "x/usr/local"; then
		NTL_CFLAGS="-I${NTL_HOME}/include -pthread"
		NTL_LIBS="-L${NTL_HOME}/lib -lntl"
	else
		NTL_CFLAGS="-pthread"
		NTL_LIBS="-lntl"
	fi
	CXXFLAGS="${BACKUP_CXXFLAGS} ${NTL_CFLAGS} ${FFLAS_FFPACK_CFLAGS}"
	LIBS="${BACKUP_LIBS} ${NTL_LIBS} ${FFLAS_FFPACK_LIBS}"

	AC_TRY_LINK(
	[#include <NTL/ZZ.h>],
	[NTL::ZZ a;],
	[
	AC_TRY_RUN(
	[#include <NTL/version.h>
	int main () { if (NTL_MAJOR_VERSION < 5) return -1; else return 0; }
	],[
	ntl_found="yes"
	break
	],[
	ntl_problem="$problem $NTL_HOME"
	unset NTL_CFLAGS
	unset NTL_LIBS
	],[
	ntl_found="yes"
	ntl_cross="yes"
	break
	])
	],
	[
	ntl_found="no"
	ntl_checked="$checked $NTL_HOME"
	unset NTL_CFLAGS
	unset NTL_LIBS
	])
else
	ntl_found="no"
fi
done

if test "x$ntl_found" = "xyes" ; then
	AC_SUBST(NTL_CFLAGS)
	AC_SUBST(NTL_LIBS)
	AC_DEFINE(HAVE_NTL,1,[Define if NTL is installed])
	HAVE_NTL=yes
	if test "x$ntl_cross" != "xyes"; then
		AC_MSG_RESULT(found)
	else
		AC_MSG_RESULT(unknown)
		echo "WARNING: You appear to be cross compiling, so there is no way to determine"
		echo "whether your NTL version is new enough. I am assuming it is."
	fi
	ifelse([$2], , :, [$2])
elif test -n "$ntl_problem"; then
	AC_MSG_RESULT(problem)
	echo "Sorry, your NTL version is too old. Disabling."
	ifelse([$3], , :, [$3])
elif test   "x$ntl_found" = "xno";  then
	AC_MSG_RESULT(not found)
	if test "x$NTL_HOME" != "x/usr" -a "x$NTL_HOME" != "x/usr/local" ; then
		AC_MSG_WARN(NTL >= $min_ntl_version was not found. LinBox also requires the NTL namespace to be enabled.  Please make sure NTL is compiled correctly.)
	fi
	ifelse([$3], , :, [$3])
fi



AM_CONDITIONAL(LINBOX_HAVE_NTL, test "x$HAVE_NTL" = "x$HAVE_NTL")
AC_MSG_WARN(LINBOX_HAVE_NTL : $LINBOX_HAVE_NTL)

CXXFLAGS=${BACKUP_CXXFLAGS}
LIBS=${BACKUP_LIBS}
#unset LD_LIBRARY_PATH

])

