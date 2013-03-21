IDENTICA_COMMON_S = \
src/tests/azy/identi.ca/identica_Common_Azy.c \
src/tests/azy/identi.ca/identica_Common_Azy.h \
src/tests/azy/identi.ca/identica_Common.h \
src/tests/azy/identi.ca/identica_Common.c

EXTRA_DIST += src/tests/azy/identi.ca/identi.ca.azy

CLEANFILES += $(IDENTICA_COMMON_S)

check_PROGRAMS += src/tests/azy/identi_ca

src_tests_azy_identi_ca_CPPFLAGS = $(AZY_TEST_CPPFLAGS)

src_tests_azy_identi_ca_SOURCES = \
$(IDENTICA_COMMON_S) \
src/tests/azy/identi.ca/identi_ca.c

src_tests_azy_identi_ca_LDADD = \
@AZY_LIBS@ \
$(top_builddir)/src/lib/libmaelstrom.la

$(IDENTICA_COMMON_S): src/tests/azy/identi.ca/identi.ca.azy src/bin/azy_parser
	src/bin/azy_parser -eHn -m common-impl,common-headers -o $(top_builddir)/src/tests/azy/identi.ca $(top_srcdir)/src/tests/azy/identi.ca/identi.ca.azy

INTERMEDIATE_S += $(IDENTICA_COMMON_S)
