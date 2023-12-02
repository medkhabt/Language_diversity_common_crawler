dnl overhauled version of the standard macro, which does not help us to
dnl produce properly formatted signal handler functions: when RETSIGTYPE=int,
dnl we'll need a way to add that 'return value;' at the end of our signal
dnl handlers.
dnl
dnl This can now be done by wrapping the 'return value;' statement like this:
dnl
dnl #if !defined(RETSIGTYPE_IS_VOID)
dnl return value;
dnl #endif
dnl

# AC_TYPE_SIGNAL_EX
# -----------------
# Note that identifiers starting with SIG are reserved by ANSI C.
AC_DEFUN([AC_TYPE_SIGNAL_EX],
[AC_CACHE_CHECK([return type of signal handlers], ac_cv_type_signal,
[AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([#include <sys/types.h>
#include <signal.h>
],
     [return *(signal (0, 0)) (0) == 1;])],
       [ac_cv_type_signal=int],
       [ac_cv_type_signal=void])])
AC_DEFINE_UNQUOTED(RETSIGTYPE, $ac_cv_type_signal,
       [Define as the return type of signal handlers
        (`int' or `void').])
if eval "test x$ac_cv_type_signal = xvoid"; then
  AC_DEFINE(RETSIGTYPE_IS_VOID, 1,
      [Define to 1 if sighandler_t has a 'void' return type,
       i.e. when RETSIGTYPE is defined to 'void'.])
fi
])


