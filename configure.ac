#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([pi2mqtt], [0.01], [onichola@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/ds18b20pi.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_SEARCH_LIBS([MQTTClient_subscribe], [paho-mqtt3c])
AC_SEARCH_LIBS([cfg_parse], [confuse])
AC_SEARCH_LIBS([wiringPiSetup], [wiringPi])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([locale.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([utime.h])
AC_CHECK_HEADERS([MQTTClient.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([memset strerror strstr])

AC_CONFIG_FILES([Makefile
		 src/Makefile])
AC_OUTPUT
