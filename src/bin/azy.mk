REC = $(RE2C)

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

azy_parser = src/bin/azy_parser$(EXEEXT_FOR_BUILD)

bin_PROGRAMS += azy_parser

azy_parser_SOURCES = \
src/bin/azy/azy.h \
src/bin/azy/azy.c \
src/bin/azy/azy_parser_main.c \
src/bin/azy/azy_parser_lib.h \
src/bin/azy/azy_parser_lib.c \
src/bin/azy_parser.y

azy_parser_CPPFLAGS = \
$(BIN_CPPFLAGS) \
$(AZY_CFLAGS) \
-I$(top_srcdir)/src/bin/azy

azy_parser_LDADD = \
$(AZY_LIBS) \
$(top_builddir)/src/lib/libmaelstrom.la

#lemon is uncontrollably loud and will never be fixed. stfu lemon!
src/bin/lemon$(EXEEXT_FOR_BUILD):
	$(CC_FOR_BUILD) -o $@ -w $(CFLAGS_FOR_BUILD) $(LDFLAGS_FOR_BUILD) src/bin/azy/lemon.c

src/bin/azy_parser.y: src/bin/azy/azy_parser.yre src/bin/lemon$(EXEEXT_FOR_BUILD)
	$(REC) $(RE2C_OPTS) $<

src/bin/azy_parser.c: src/bin/azy_parser.y src/bin/lemon$(EXEEXT_FOR_BUILD)
	cp -f $(top_srcdir)/src/bin/azy/lempar.c . && src/bin/lemon$(EXEEXT_FOR_BUILD) -q $<
