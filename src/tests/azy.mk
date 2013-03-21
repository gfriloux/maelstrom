AZY_TEST_CPPFLAGS= \
@AZY_CFLAGS@ \
@MYSQL_CFLAGS@ \
-I$(top_srcdir) \
-I$(top_srcdir)/src/include/azy

include src/tests/azy/http-simple/Makefile.mk
include src/tests/azy/identi.ca/Makefile.mk
include src/tests/azy/rss/Makefile.mk
include src/tests/azy/unit/Makefile.mk

TYPES_S = \
src/tests/azy/type1_Common.c \
src/tests/azy/type1_Common.h \
src/tests/azy/type1_Common_Azy.c \
src/tests/azy/type1_Common_Azy.h \
src/tests/azy/type2_Common.c \
src/tests/azy/type2_Common.h \
src/tests/azy/type2_Common_Azy.c \
src/tests/azy/type2_Common_Azy.h

COMMON_S = \
src/tests/azy/T_Common.c \
src/tests/azy/T_Common.h \
src/tests/azy/T_Common_Azy.c \
src/tests/azy/T_Common_Azy.h

if HAVE_MYSQL
MYSQL_SERVER_S = \
src/tests/azy/T_SQL.azy_server.c \
src/tests/azy/T_SQL.azy_server.h
else
MYSQL_SERVER_S =
endif

SERVER_S = \
$(MYSQL_SERVER_S) \
src/tests/azy/T_Test1.azy_server.c \
src/tests/azy/T_Test1.azy_server.h \
src/tests/azy/T_Test2.azy_server.c \
src/tests/azy/T_Test2.azy_server.h

if HAVE_MYSQL
MYSQL_CLIENT_S = \
src/tests/azy/T_SQL.azy_client.c \
src/tests/azy/T_SQL.azy_client.h
else
MYSQL_CLIENT_S = 
endif

CLIENT_S = \
$(MYSQL_CLIENT_S) \
src/tests/azy/T_Test1.azy_client.c \
src/tests/azy/T_Test1.azy_client.h \
src/tests/azy/T_Test2.azy_client.c \
src/tests/azy/T_Test2.azy_client.h

GENERATED_S = \
$(CLIENT_S) \
$(SERVER_S) \
$(COMMON_S) \
$(TYPES_S)

INTERMEDIATE_S += $(GENERATED_S)

CLEANFILES += $(GENERATED_S)

check_PROGRAMS += \
src/tests/azy/client \
src/tests/azy/stress_client \
src/tests/azy/server

check_LTLIBRARIES += src/tests/azy/libclient.la

src_tests_azy_libclient_la_SOURCES = \
$(COMMON_S) \
$(CLIENT_S)

src_tests_azy_libclient_la_CPPFLAGS = $(AZY_TEST_CPPFLAGS)

src_tests_azy_libclient_la_LIBADD = \
@MYSQL_LIBS@ \
@AZY_LIBS@ \
src/lib/libmaelstrom.la

nodist_src_tests_azy_client_SOURCES = \
src/tests/azy/client.c

src_tests_azy_client_CPPFLAGS = $(AZY_TEST_CPPFLAGS)

src_tests_azy_client_LDADD = \
@MYSQL_LIBS@ \
@AZY_LIBS@ \
src/lib/libmaelstrom.la \
src/tests/azy/libclient.la

nodist_src_tests_azy_stress_client_SOURCES = \
src/tests/azy/stress_client.c

src_tests_azy_stress_client_CPPFLAGS = $(AZY_TEST_CPPFLAGS)

src_tests_azy_stress_client_LDADD = \
@MYSQL_LIBS@ \
@AZY_LIBS@ \
src/lib/libmaelstrom.la \
src/tests/azy/libclient.la

nodist_src_tests_azy_server_SOURCES = \
$(COMMON_S) \
$(SERVER_S) \
src/tests/azy/server.c

src_tests_azy_server_CPPFLAGS = $(AZY_TEST_CPPFLAGS)

src_tests_azy_server_LDADD = \
@MYSQL_LIBS@ \
@AZY_LIBS@ \
src/lib/libmaelstrom.la

$(GENERATED_S): src/tests/azy/test.azy src/bin/azy_parser
	src/bin/azy_parser -H -p -o src/tests/azy $(top_srcdir)/src/tests/azy/test.azy
	src/bin/azy_parser -H -p -o src/tests/azy $(top_srcdir)/src/tests/azy/type1.azy
	src/bin/azy_parser -H -p -o src/tests/azy $(top_srcdir)/src/tests/azy/type2.azy
