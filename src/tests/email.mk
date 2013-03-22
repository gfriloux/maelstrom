EMAIL_TEST_CPPFLAGS = \
@EMAIL_CFLAGS@ \
-I$(top_srcdir) \
-I$(top_srcdir)/src/include/email \
-I$(top_srcdir)/src/tests/email

EMAIL_TEST_LDADD = \
$(top_builddir)/src/lib/libmaelstrom.la \
@EMAIL_LIBS@

check_PROGRAMS += \
src/tests/email/test_pop3 \
src/tests/email/test_smtp

src_tests_email_test_pop3_SOURCES = \
src/tests/email/test_pop3.c \
src/tests/email/getpass_x.c

src_tests_email_test_smtp_SOURCES = \
src/tests/email/test_smtp.c \
src/tests/email/getpass_x.c

src_tests_email_test_pop3_CPPFLAGS = $(EMAIL_TEST_CPPFLAGS)
src_tests_email_test_pop3_LDADD = $(EMAIL_TEST_LDADD)
src_tests_email_test_smtp_CPPFLAGS = $(EMAIL_TEST_CPPFLAGS)
src_tests_email_test_smtp_LDADD = $(EMAIL_TEST_LDADD)
