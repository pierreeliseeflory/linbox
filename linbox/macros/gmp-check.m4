# Check for GMP
# Bradford Hovinen, 2001-06-13
# Inspired by gnome-bonobo-check.m4 by Miguel de Icaza, 99-04-12
# Stolen from Chris Lahey       99-2-5
# stolen from Manish Singh again
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl LB_CHECK_GMP ([MINIMUM-VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl
dnl Test for the GNU Multiprecision library and define GMP_CFLAGS and GMP_LIBS

AC_DEFUN([LB_CHECK_GMP],
[

AC_ARG_WITH(gmp-prefix,[  --with-gmp-prefix=PFX      Prefix where GMP is installed (optional)],
[gmp_prefix="$withval"],[gmp_prefix=""])

min_gmp_version=ifelse([$1], ,3.1.1,$1)
AC_MSG_CHECKING(for GMP >= $min_gmp_version)

if test x$gmp_prefix = x; then
	gmp_prefix=/usr
else 
	LD_LIBRARY_PATH=${gmp_prefix}/lib
	export LD_LIBRARY_PATH
fi

dnl Check for existence

GMP_CFLAGS="-I${gmp_prefix}/include"
GMP_LIBS="-L${gmp_prefix}/lib -lgmp"

BACKUP_CXXFLAGS=${CXXFLAGS}
BACKUP_LIBS=${LIBS}

CXXFLAGS=${GMP_CFLAGS}
LIBS=${GMP_LIBS}

AC_TRY_LINK(
[#include <gmp.h>],
[mpz_t a; mpz_init (a);],
[
AC_TRY_RUN(
[#include <gmp.h>
int main () {  if (__GNU_MP_VERSION < 3) return -1; else return 0; }
],[
AC_MSG_RESULT(found)
AC_SUBST(GMP_CFLAGS)
AC_SUBST(GMP_LIBS)
AC_DEFINE(HAVE_GMP)

ifelse([$2], , :, [$2])
],[
AC_MSG_RESULT(not found)
echo "Sorry, your GMP version is too old. Disabling."

unset GMP_CFLAGS
unset GMP_LIBS

ifelse([$3], , :, [$3])
])
],
[
AC_MSG_RESULT(not found)

unset GMP_CFLAGS
unset GMP_LIBS

ifelse([$3], , :, [$3])
])

CXXFLAGS=${BACKUP_CXXFLAGS}
LIBS=${BACKUP_LIBS}
#unset LD_LIBRARY_PATH

])
