BIN_CPPFLAGS = \
$(SHOTGUN_CFLAGS) \
-I$(top_srcdir) \
-I$(top_srcdir)/src/bin \
-I$(top_builddir)/src/bin \
-I$(top_srcdir)/src/include/extras

EXTRA_DIST += \
src/bin/azy/lempar.c \
src/bin/azy/lemon.c \
src/bin/azy/azy_parser.yre

AM_YFLAGS = -d

if BUILD_AZY
include src/bin/azy.mk
endif

if BUILD_SHOTGUN_GUI
include src/bin/shotgun_gui.mk
endif
