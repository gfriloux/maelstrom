src_lib_libmaelstrom_la_LIBADD += $(SHOTGUN_LIBS)

src_lib_libmaelstrom_la_CPPFLAGS += \
$(SHOTGUN_CFLAGS) \
-I$(top_srcdir)/src/include/shotgun

shotgun_source += \
src/lib/shotgun/iq.c \
src/lib/shotgun/login.c \
src/lib/shotgun/messaging.c \
src/lib/shotgun/presence.c \
src/lib/shotgun/shotgun.c \
src/lib/shotgun/shotgun_utils.c \
src/lib/shotgun/xml.cpp

extra_source += \
src/lib/extras/md5.c \
src/lib/extras/sha1.c \
src/lib/extras/sha1wrap.c

