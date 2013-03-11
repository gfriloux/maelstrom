AM_CFLAGS= \
  $(AZY_CFLAGS) \
  -I$(top_srcdir) \
  -I$(top_srcdir)/src/include
AM_CXXFLAGS = $(AM_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in

check_PROGRAMS = simple
simple_SOURCES = http-simple.c
simple_LDADD = \
  $(AZY_LIBS) \
  $(top_builddir)/src/lib/libazy.la
