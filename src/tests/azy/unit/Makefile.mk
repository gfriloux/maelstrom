check_PROGRAMS += src/tests/azy/unit/t001_content \
  src/tests/azy/unit/t002_json

src_tests_azy_unit_t001_content_CPPFLAGS = $(AZY_TEST_CPPFLAGS)

src_tests_azy_unit_t001_content_SOURCES = \
  src/tests/azy/unit/t001_content.c

src_tests_azy_unit_t001_content_LDADD = \
  @AZY_LIBS@ \
  $(top_builddir)/src/lib/libmaelstrom.la

BUILT_SOURCES = .sources_t002_json
src_tests_azy_unit_t002_json_SOURCES = \
  src/tests/azy/unit/azy_content_deserialize_json.c \
  src/tests/azy/unit/t002_json.c \
  src/tests/azy/unit/t002_json.h \
  src/tests/azy/unit/T002_Common_Azy.c \
  src/tests/azy/unit/T002_Common_Azy.h \
  src/tests/azy/unit/T002_Common.c \
  src/tests/azy/unit/T002_Common.h
src_tests_azy_unit_t002_json_CPPFLAGS = $(AZY_TEST_CPPFLAGS)
src_tests_azy_unit_t002_json_LDFLAGS = $(AZY_TEST_LDFLAGS) -lz -lrt -lm -lcheck -lc
src_tests_azy_unit_t002_json_LDADD = \
  @AZY_LIBS@ \
  $(top_builddir)/src/lib/libmaelstrom.la
.sources_t002_json: src/tests/azy/unit/t002_json.azy src/bin/azy_parser
	src/bin/azy_parser -H -p -o src/tests/azy/unit/ $(top_srcdir)/src/tests/azy/unit/t002_json.azy
