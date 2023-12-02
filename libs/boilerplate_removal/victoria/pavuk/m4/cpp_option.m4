#
# test specific cc/gcc/... compiler & linker commandline options.
#
# see: http://www.opensubscriber.com/message/automake@gnu.org/1456940.html
#

AC_DEFUN([AC_MY_CPP_OPTION],
[AC_MSG_CHECKING([for $2])
AC_CACHE_VAL(epos_cv_cpp_opt_$1,
[cat > conftest.$ac_ext <<EOF
#include <stdio.h>
int main(int argc, char **argv)
{
argc=argc; argv=argv; return 0;
}
EOF
epos_cv_cpp_opt_$1=""
for opt in $3; do
    if test -z "${epos_cv_cpp_opt_$1}"; then
        if $CC -o conftest$ac_exeext $CFLAGS $CPPFLAGS $LDFLAGS conftest.$ac_ext 2>conftest2 1>&5; then
            if test -f conftest2; then
                msg=`cat conftest2`
                if test -z "${msg}"; then
                  epos_cv_cpp_opt_$1=${opt}
                fi
            else
                epos_cv_cpp_opt_$1=${opt}
            fi
        fi
    fi
done
if test -z "${epos_cv_cpp_opt_$1}"; then
        epos_cv_cpp_opt_$1="unknown"
fi
rm -f conftest conftest2])
if test "${epos_cv_cpp_opt_$1}" = "unknown"; then
        AC_MSG_RESULT("unknown")
        $1=""
else
        AC_MSG_RESULT(${epos_cv_cpp_opt_$1})
        $1=${epos_cv_cpp_opt_$1}
fi])