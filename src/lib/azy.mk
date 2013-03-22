src_lib_libmaelstrom_la_LIBADD += $(AZY_LIBS)

src_lib_libmaelstrom_la_CPPFLAGS += \
$(AZY_CFLAGS) \
-I$(top_srcdir)/src/include/azy

azy_source += \
src/lib/azy/azy_lib.c \
src/lib/azy/azy_events.c \
src/lib/azy/azy_client_events.c \
src/lib/azy/azy_server_events.c \
src/lib/azy/azy_value.c \
src/lib/azy/azy_client.c \
src/lib/azy/azy_content.c \
src/lib/azy/azy_content_json.c \
src/lib/azy/azy_content_xml.cpp \
src/lib/azy/azy_server.c \
src/lib/azy/azy_server_module.c \
src/lib/azy/azy_net.c \
src/lib/azy/azy_net_cookie.c \
src/lib/azy/azy_rss.c \
src/lib/azy/azy_rss_item.c \
src/lib/azy/azy_utils.c

extra_source += src/lib/extras/cJSON.c

