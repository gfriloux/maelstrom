AM_CFLAGS = \
  -I$(top_srcdir) \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/include/extras \
  $(CC_PIC)
AM_CXXFLAGS = $(AM_CFLAGS) $(CXX_PIC)
LIBADD = -lm

noinst_LTLIBRARIES = \
libcJSON.la \
libb64.la \
libpugixml.la \
libstrmatch.la

libpugixml_la_SOURCES = pugixml.cpp
libpugixml_la_LIBADD =

libcJSON_la_SOURCES = cJSON.c

libb64_la_SOURCES = cencode.c cdecode.c

libstrmatch_la_CFLAGS = $(AM_CFLAGS) @SSE_CFLAGS@
libstrmatch_la_SOURCES = \
strmatch/Sbf.c \
strmatch/Sbom.c \
strmatch/Smp.c \
strmatch/Skmp.c \
strmatch/Sbm.c \
strmatch/Sbmh.c \
strmatch/Sbmhs.c \
strmatch/Ssmith.c \
strmatch/Sshiftor.c \
strmatch/Sshiftand.c \
strmatch/Skr.c \
strmatch/Sbyh.c \
strmatch/Sskip.c \
strmatch/Skmpskip.c \
strmatch/dsor.c \
strmatch/strstrsse.c \
strmatch/strstrsse42.c \
strmatch/strstrsse42a.c \
strmatch/lstrstr.c \
strmatch/report.c

MAINTAINERCLEANFILES = Makefile.in
