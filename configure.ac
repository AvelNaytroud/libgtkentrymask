AC_PREREQ(2.63)
AC_INIT([libgtkentrymask],[0.1],[stephane.raimbault@gmail.com])
AM_INIT_AUTOMAKE([1.11.1 foreign])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_CONFIG_MACRO_DIR([m4])

# Checks for libraries.
AM_PATH_GTK_2_0(2.0.0,,AC_MSG_ERROR(libgtkentrymask needs GTK+ 2.x))

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([langinfo.h locale.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MKTIME
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([localeconv nl_langinfo strchr])

AC_CONFIG_FILES([Makefile
	libgtkentrymask-2.0.pc
	])
AC_OUTPUT
