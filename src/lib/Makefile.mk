LIB_CPPFLAGS = \
-I$(top_srcdir) \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/src/include/extras \
-I$(top_srcdir)/src/include/extras/strmatch

noinst_LTLIBRARIES = src/lib/libstrmatch.la

EXTRA_DIST += \
src/lib/extras/strmatch/automaton.h \
src/lib/extras/strmatch/reference \
src/lib/extras/strmatch/report.h \
src/lib/extras/strmatch/strstrsse.h \
src/lib/extras/strmatch/util.h \
src/lib/extras/strmatch/varshift.h

src_lib_libstrmatch_la_CFLAGS = @SSE_CFLAGS@

src_lib_libstrmatch_la_CPPFLAGS = $(LIB_CPPFLAGS)

src_lib_libstrmatch_la_SOURCES = \
src/lib/extras/strmatch/Sbf.c \
src/lib/extras/strmatch/Sbom.c \
src/lib/extras/strmatch/Smp.c \
src/lib/extras/strmatch/Skmp.c \
src/lib/extras/strmatch/Sbm.c \
src/lib/extras/strmatch/Sbmh.c \
src/lib/extras/strmatch/Sbmhs.c \
src/lib/extras/strmatch/Ssmith.c \
src/lib/extras/strmatch/Sshiftor.c \
src/lib/extras/strmatch/Sshiftand.c \
src/lib/extras/strmatch/Skr.c \
src/lib/extras/strmatch/Sbyh.c \
src/lib/extras/strmatch/Sskip.c \
src/lib/extras/strmatch/Skmpskip.c \
src/lib/extras/strmatch/dsor.c \
src/lib/extras/strmatch/strstrsse.c \
src/lib/extras/strmatch/strstrsse42.c \
src/lib/extras/strmatch/strstrsse42a.c \
src/lib/extras/strmatch/lstrstr.c \
src/lib/extras/strmatch/report.c

lib_LTLIBRARIES = src/lib/libmaelstrom.la
src_lib_libmaelstrom_la_LIBADD = \
-lm \
src/lib/libstrmatch.la

src_lib_libmaelstrom_la_LDFLAGS = -version-info @version_info@ -no-undefined
src_lib_libmaelstrom_la_CPPFLAGS = \
-DEFL_SHOTGUN_BUILD \
$(LIB_CPPFLAGS) \
$(SHOTGUN_CFLAGS)

extra_source = \
src/lib/extras/cencode.c \
src/lib/extras/cdecode.c

if BUILD_SHOTGUN
extra_source += \
src/lib/extras/md5.c \
src/lib/extras/memrchr.c
else
if BUILD_EMAIL
extra_source += \
src/lib/extras/md5.c \
src/lib/extras/memrchr.c
endif
endif

if BUILD_AZY
extra_source += src/lib/extras/pugixml.cpp
else
if BUILD_SHOTGUN
extra_source += src/lib/extras/pugixml.cpp
endif
endif

azy_source =
if BUILD_AZY
include src/lib/azy.mk
endif

shotgun_source =
if BUILD_SHOTGUN
include src/lib/shotgun.mk
endif

email_source =
if BUILD_EMAIL
include src/lib/email.mk
endif

src_lib_libmaelstrom_la_SOURCES = \
$(azy_source) \
$(email_source) \
$(extra_source) \
$(shotgun_source)
