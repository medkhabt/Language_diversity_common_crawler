dnl Check for a type x; define HAVE_TYPE_x when the type x is defined
dnl in the standard include files.

# AC_HAVE_TYPE(TYPE, [INCLUDES = DEFAULT-INCLUDES])
# ------------------------------------------------------------
# Check whether the type TYPE is supported by the system, maybe via the
# the provided includes.
#
# The most obvious way to check for a TYPE is just to compile a variable
# definition:
#
#   TYPE my_var;
#
# Unfortunately this does not work for const qualified types in C++,
# where you need an initializer.  So you think of
#
#   TYPE my_var = (TYPE) 0;
#
# Unfortunately, again, this is not valid for some C++ classes.
#
# Then you look for another scheme.  For instance you think of declaring
# a function which uses a parameter of type TYPE:
#
#   int foo (TYPE param);
#
# but of course you soon realize this does not make it with K&R
# compilers.  And by no ways you want to
#
#   int foo (param)
#     TYPE param
#   { ; }
#
# since this time it's C++ who is not happy.
#
# Don't even think of the return type of a function, since K&R cries
# there too.  So you start thinking of declaring a *pointer* to this TYPE:
#
#   TYPE *p;
#
# but you know fairly well that this is legal in C for aggregates which
# are unknown (TYPE = struct does-not-exist).
#
# Then you think of using sizeof to make sure the TYPE is really
# defined:
#
#   sizeof (TYPE);
#
# But this succeeds if TYPE is a variable: you get the size of the
# variable's type!!!
#
# This time you tell yourself the last two options *together* will make
# it.  And indeed this is the solution invented by Alexandre Oliva.
#
# Also note that we use
#
#   if (sizeof (TYPE))
#
# to `read' sizeof (to avoid warnings), while not depending on its type
# (not necessarily size_t etc.).  Equally, instead of defining an unused
# variable, we just use a cast to avoid warnings from the compiler.
# Suggested by Paul Eggert.
#
# Now, the next issue is that C++ disallows defining types inside casts
# and inside `sizeof()', but we would like to allow unnamed structs, for
# use inside AC_CHECK_SIZEOF, for example.  So we create a typedef of the
# new type.  Note that this does not obviate the need for the other
# constructs in general.
AC_DEFUN([AC_HAVE_TYPE],
[
AS_VAR_PUSHDEF([ac_Type], [ac_cv_type_$1])dnl
AC_CACHE_CHECK([for $1], [ac_Type],
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT([$2])
typedef $1 ac__type_new_;],
[if ((ac__type_new_ *) 0)
  return 0;
if (sizeof (ac__type_new_))
  return 0;])],
       [AS_VAR_SET([ac_Type], [yes])],
       [AS_VAR_SET([ac_Type], [no])])])
AS_IF([test AS_VAR_GET([ac_Type]) = yes],
  AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_TYPE_]$1), 1,
    [Define to 1 if the system has the type `]$1['.]),
  ,)[]dnl
AS_VAR_POPDEF([ac_Type])dnl
])


