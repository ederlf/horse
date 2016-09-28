AC_PREREQ(2.59)
AC_INIT(horse, 0.0.0, eleao@qmul.ac.uk)

AC_CONFIG_SRCDIR(README.md)
AM_INIT_AUTOMAKE([1.10 no-define foreign])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AM_CONDITIONAL(GCC, test "$GCC" = yes)   # let the Makefile know if we're gcc

# Check for C Standard headers
AC_HEADER_STDC

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
