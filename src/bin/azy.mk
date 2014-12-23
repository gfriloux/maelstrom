if NEED_RE2C
NEED_REC = src/bin/re2cbin
noinst_PROGRAMS += src/bin/re2cbin
src_bin_re2cbin_SOURCES = \
src/bin/re2c/code.cpp \
src/bin/re2c/dfa.cpp \
src/bin/re2c/main.cpp \
src/bin/re2c/actions.cpp \
src/bin/re2c/substr.cpp \
src/bin/re2c/translate.cpp \
src/bin/re2c/mbo_getopt.cpp \
src/bin/re2c/scanner.cpp \
src/bin/re2c/basics.h \
src/bin/re2c/dfa.h \
src/bin/re2c/globals.h \
src/bin/re2c/ins.h \
src/bin/re2c/parser.h \
src/bin/re2c/re_parser.ypp \
src/bin/re2c/re.h \
src/bin/re2c/scanner.h \
src/bin/re2c/substr.h \
src/bin/re2c/token.h \
src/bin/re2c/mbo_getopt.h \
src/bin/re2c/code.h \
src/bin/re2c/stream_lc.h \
src/bin/re2c/code_names.h \
src/bin/re2c/re_parser.h

src_bin_re2cbin_CPPFLAGS = \
$(BIN_CPPFLAGS) \
-w \
-I$(top_srcdir)/src/bin/re2c \
-I$(top_builddir)/src/bin/re2c

INTERMEDIATE_S += \
src/bin/re2c/re_parser.cpp \
src/bin/re2c/re_parser.h

DISTCLEANFILES += \
src/bin/re_parser.cpp \
src/bin/re_parser.h

REC = $(top_builddir)/src/bin/re2cbin
src/bin/re2c/scanner.cpp: src/bin/re2c/re_parser.h
else
NEED_REC =
REC = $(RE2C)
endif

DISTCLEANFILES += \
src/bin/azy_parser.c \
src/bin/azy_parser.h \
src/bin/azy_parser.y \
src/bin/y.tab.h \
lempar.c

INTERMEDIATE_S += \
src/bin/azy_parser.c \
src/bin/azy_parser.h \
src/bin/azy_parser.y \
src/bin/y.tab.h

RE2C_OPTS = -bi -o src/bin/azy_parser.y

bin_PROGRAMS += src/bin/azy_parser

src_bin_azy_parser_SOURCES = \
src/bin/azy/azy.h \
src/bin/azy/azy.c \
src/bin/azy/azy_parser_main.c \
src/bin/azy/azy_parser_lib.h \
src/bin/azy/azy_parser_lib.c \
src/bin/azy_parser.y

src_bin_azy_parser_CPPFLAGS = \
$(BIN_CPPFLAGS) \
$(AZY_CFLAGS) \
-I$(top_srcdir)/src/bin/azy

src_bin_azy_parser_LDADD = \
$(AZY_LIBS) \
$(top_builddir)/src/lib/libmaelstrom.la

#lemon is uncontrollably loud and will never be fixed. stfu lemon!
src/bin/lemon$(EXEEXT_FOR_BUILD):
	$(CC_FOR_BUILD) -o $@ -w src/bin/azy/lemon.c

src/bin/azy_parser.y: src/bin/azy/azy_parser.yre $(NEED_REC) src/bin/lemon$(EXEEXT_FOR_BUILD)
	$(REC) $(RE2C_OPTS) $<

src/bin/azy_parser.c: src/bin/azy_parser.y src/bin/lemon$(EXEEXT_FOR_BUILD)
	cp -f $(top_srcdir)/src/bin/azy/lempar.c . && src/bin/lemon$(EXEEXT_FOR_BUILD) -q $<
