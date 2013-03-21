check_PROGRAMS += src/tests/azy/unit/t001_content

src_tests_azy_unit_t001_content_CPPFLAGS = $(AZY_TEST_CPPFLAGS)

src_tests_azy_unit_t001_content_SOURCES = \
  src/tests/azy/unit/t001_content.c

src_tests_azy_unit_t001_content_LDADD = \
  @AZY_LIBS@ \
  $(top_builddir)/src/lib/libmaelstrom.la
