#!/usr/bin/autoconf
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(ulsiod, 1.0, john.sturdy@ul.ie)
AM_INIT_AUTOMAKE([-Wall -Werror])

AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_MAJOR
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h limits.h netdb.h netinet/in.h stdlib.h string.h sys/fs_types.h sys/mount.h sys/socket.h sys/time.h sys/timeb.h unistd.h utime.h utmp.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_REPLACE_FNMATCH
AC_FUNC_FORK
AC_FUNC_GETPGRP
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([alarm endpwent fchdir floor getcwd gethostbyaddr gethostbyname gethostname getpass gettimeofday lchown memchr memmove memset mkdir pow putenv regcomp rmdir select socket sqrt strchr strcspn strdup strerror strncasecmp strrchr strspn strstr tzset utime utmpname])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_MSG_NOTICE([************************************************************])
AC_MSG_NOTICE([* Now run `make` and `make install`.                       *])
AC_MSG_NOTICE([************************************************************])
