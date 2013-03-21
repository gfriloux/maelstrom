check_PROGRAMS += src/tests/azy/http_simple

src_tests_azy_http_simple_SOURCES = src/tests/azy/http-simple/http-simple.c

src_tests_azy_http_simple_CPPFLAGS = $(AZY_TEST_CPPFLAGS)

src_tests_azy_http_simple_LDADD = \
@AZY_LIBS@ \
src/lib/libmaelstrom.la
