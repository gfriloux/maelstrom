check_PROGRAMS += src/tests/azy/azy_rss

src_tests_azy_azy_rss_SOURCES = src/tests/azy/rss/rss.c

src_tests_azy_azy_rss_CPPFLAGS = $(AZY_TEST_CPPFLAGS)

src_tests_azy_azy_rss_LDADD = \
  @AZY_LIBS@ \
  $(top_builddir)/src/lib/libmaelstrom.la
