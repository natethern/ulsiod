##########################
# Makefile.am for ULSIOD #
##########################

# Time-stamp: <2010-01-11 11:12:46 jcgs>

SUBDIRS = .

AM_CPPFLAGS = -Wall -pg -ffast-math -DSIOD_LIB_DEFAULT='"$(datadir)/siod"'

###############################################################################
# SOURCE FILES
###############################################################################

lib_LTLIBRARIES = libulsiod.la

ulsioddir = $(includedir)/siod
ulsiod_HEADERS = src/ulsiod.h src/siodp.h src/md5.h
libulsiod_la_SOURCES = $(ulsiod_HEADERS) src/slib.c src/sliba.c src/slibu.c src/trace.c src/md5.c
libulsiod_la_LIBADD = -lm -lc -ldl -lcrypt

bin_PROGRAMS = siod
siod_SOURCES = src/siod.c
siod_LDADD = -lulsiod # -lm -lc -ldl -lcrypt

# Makefile.am ends here
