src_lib_libmaelstrom_la_LIBADD += @EMAIL_LIBS@

src_lib_libmaelstrom_la_CPPFLAGS += \
@EMAIL_CFLAGS@ \
-I$(top_srcdir)/src/include/email

email_source += \
src/lib/email/attachment.c \
src/lib/email/auth.c \
src/lib/email/contact.c \
src/lib/email/email.c \
src/lib/email/email_utils.c \
src/lib/email/imap_funcs.c \
src/lib/email/imap_handlers.c \
src/lib/email/imap_login.c \
src/lib/email/message.c \
src/lib/email/pop_handlers.c \
src/lib/email/pop_list.c \
src/lib/email/pop_login.c \
src/lib/email/pop_retr.c \
src/lib/email/smtp_handlers.c \
src/lib/email/smtp_login.c
