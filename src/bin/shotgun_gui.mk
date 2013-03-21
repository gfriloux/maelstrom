bin_PROGRAMS += src/bin/shotgun_gui

src_bin_shotgun_gui_CPPFLAGS = \
$(BIN_CPPFLAGS) \
-I$(top_srcdir)/src/include/shotgun \
-I$(top_srcdir)/src/include/azy \
@SHOTGUN_CFLAGS@ \
@AZY_CFLAGS@ \
@SHOTGUN_GUI_CFLAGS@ \
@edbus_CFLAGS@ \
@enotify_CFLAGS@ \
-DDATA_DIR=\"$(datadir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/shotgun\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_SRC_DIR=\"$(top_srcdir)\"

src_bin_shotgun_gui_LDADD = \
@SHOTGUN_LIBS@ \
@AZY_LIBS@ \
@SHOTGUN_GUI_LIBS@ \
@edbus_LIBS@ \
@enotify_LIBS@ \
-lm \
$(top_builddir)/src/lib/libmaelstrom.la

src_bin_shotgun_gui_SOURCES = \
src/bin/shotgun/azy.c \
src/bin/shotgun/chat.c \
src/bin/shotgun/chat_image.c \
src/bin/shotgun/contact.c \
src/bin/shotgun/contact_list.c \
src/bin/shotgun/dbus.c \
src/bin/shotgun/eet.c \
src/bin/shotgun/events.c \
src/bin/shotgun/getpass_x.c \
src/bin/shotgun/login.c \
src/bin/shotgun/logging.c \
src/bin/shotgun/main.c \
src/bin/shotgun/settings.c \
src/bin/shotgun/ui.c \
src/bin/shotgun/ui.h \
src/bin/shotgun/util.c
