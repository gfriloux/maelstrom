bin_PROGRAMS += src/bin/shotgun_gui

src_bin_shotgun_gui_CPPFLAGS = \
$(BIN_CPPFLAGS) \
-I$(top_srcdir)/src/include/shotgun \
-I$(top_srcdir)/src/include/azy \
@SHOTGUN_CFLAGS@ \
@AZY_CFLAGS@ \
@SHOTGUN_GUI_CFLAGS@ \
@ELDBUS_CFLAGS@ \
-DDATA_DIR=\"$(datadir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/shotgun\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_SRC_DIR=\"$(top_srcdir)\"

src_bin_shotgun_gui_LDADD = \
@SHOTGUN_LIBS@ \
@AZY_LIBS@ \
@SHOTGUN_GUI_LIBS@ \
@ELDBUS_LIBS@ \
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

EDJE_FLAGS = -id $(top_srcdir)/src/bin/shotgun/theme

shotgun_gui_filesdir = $(datadir)/shotgun
shotgun_gui_files_DATA = src/bin/shotgun/default.edj

images = \
src/bin/shotgun/theme/arrows_both.png \
src/bin/shotgun/theme/arrows_pending_left.png \
src/bin/shotgun/theme/arrows_pending_right.png \
src/bin/shotgun/theme/arrows_rejected.png \
src/bin/shotgun/theme/dialog_ok.png \
src/bin/shotgun/theme/logout.png \
src/bin/shotgun/theme/settings.png \
src/bin/shotgun/theme/status.png \
src/bin/shotgun/theme/useradd.png \
src/bin/shotgun/theme/userdel.png \
src/bin/shotgun/theme/useroffline.png \
src/bin/shotgun/theme/userunknown.png \
src/bin/shotgun/theme/x.png

EXTRA_DIST += \
src/bin/shotgun/theme/default.edc \
$(images)

src/bin/shotgun/default.edj: $(images) src/bin/shotgun/theme/default.edc
	@edje_cc@ $(EDJE_FLAGS) \
	$(top_srcdir)/src/bin/shotgun/theme/default.edc \
	$(top_builddir)/src/bin/shotgun/default.edj
