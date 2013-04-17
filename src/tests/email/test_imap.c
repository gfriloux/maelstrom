#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <time.h>

#include <Ecore.h>
#include "Email.h"

#define IMAP_TEST_MBOX_NAME "EMAILTESTIMAPNAMEHAHA"

#define IMAP_TEST_MESSAGE "Hello Joe, do you think we can meet part 3:30 tomorrow?"
#define IMAP_TEST_MESSAGE_FROM "foobar@Blurdybloop.COM"
#define IMAP_TEST_MESSAGE_FROM_NAME "Fred Foobar"
#define IMAP_TEST_MESSAGE_TO "mooch@owatagu.siam.edu"
#define IMAP_TEST_MESSAGE_SUBJECT "afternoon meeting"

/* full message text

"Date: Mon, 7 Feb 1994 21:52:25 -0800 (PST)\r\n" \
"From: Fred Foobar <foobar@Blurdybloop.COM>\r\n" \
"Subject: afternoon meeting\r\n" \
"To: mooch@owatagu.siam.edu\r\n" \
"Message-Id: <B27397-0100000@Blurdybloop.COM>\r\n" \
"MIME-Version: 1.0\r\n" \
"Content-Type: TEXT/PLAIN; CHARSET=US-ASCII\r\n" \
"\r\n" \
"Hello Joe, do you think we can meet part 3:30 tomorrow?\r\n" \
"\r\n"

*/
char *getpass_x(const char *prompt);

static void
mail_quit(Email_Operation *op, Email_Operation_Status success)
{
   printf("QUIT: %s - %s\n", success ? "SUCCESS!" : "FAIL!", email_operation_status_message_get(op));
   ecore_main_loop_quit();
}

static const char *const MAIL_FLAGS[] =
{
   "ANSWERED",
   "DELETED",
   "DRAFT",
   "FLAGGED",
   "RECENT",
   "SEEN",
   "*",
};

static void
mail_flags(Email_Imap4_Mail_Flag flags)
{
   unsigned int x;
   for (x = 0; x < EMAIL_IMAP4_MAIL_FLAG_ITERATE; x++)
     if (flags & (1 << x))
       printf(" %s", MAIL_FLAGS[x]);
}

static Eina_Bool
messageparams_cb(const Eina_Hash *h EINA_UNUSED, const char *key, Eina_Inarray *val, void *d EINA_UNUSED)
{
   Eina_Stringshare *s;

   printf("\tPARAM: %s -", key);
   EINA_INARRAY_FOREACH(val, s)
     printf(" %s", s);
   fputc('\n', stdout);
   return EINA_TRUE;
}

static void
messageinfo_print(Email_Imap4_Message *im)
{
   printf("MESSAGE:\n");
   if (im->uid) printf("\tUID %u\n", im->uid);
   if (im->internaldate)
     {
        char timebuf[1024];
        struct tm *t;

        t = localtime((time_t*)&im->internaldate);
        strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %T %z (%Z)", t);
        printf("\tINTERNALDATE %s\n", timebuf);
     }
   if (im->flags)
     {
        printf("\tFLAGS:");
        mail_flags(im->flags);
        fputc('\n', stdout);
     }
   if (im->params) eina_hash_foreach(im->params, (Eina_Hash_Foreach)messageparams_cb, NULL);
   if (im->msg)
     {
        Eina_Strbuf *buf;

        buf = email_message_serialize(im->msg);
        printf("%s\n", eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
     }
}

static const char *const MBOX_RIGHTS[] =
{
   "LOOKUP",
   "READ",
   "SEEN",
   "WRITE",
   "INSERT",
   "POST",
   "CREATE",
   "DELETE_MBOX",
   "DELETE_MSG",
   "EXPUNGE",
   "ADMIN",
};

static void
mail_rights(Email_Imap4_Mailbox_Rights flags)
{
   unsigned int x;
   for (x = 0; x < EMAIL_IMAP4_MAILBOX_RIGHT_ITERATE; x++)
     if (flags & (1 << x))
       printf(" %s", MBOX_RIGHTS[x]);
}

static Eina_Bool
mailboxinfo_print(void *d EINA_UNUSED, int type, Email_Imap4_Mailbox_Info *info)
{
   const char *acc = "READ-WRITE";

   if (info->access == EMAIL_IMAP4_MAILBOX_ACCESS_READONLY)
     acc = "READ-ONLY";
   if (type) printf("INBOX INFO: %s\n", acc);

   printf("\tMESSAGES: %u\n", info->exists);
   printf("\tMESSAGES (RECENT): %u\n", info->recent);
   printf("\tMESSAGES (UNSEEN): %u\n", info->unseen);

   printf("\tFLAGS:");
   mail_flags(info->flags);
   fputc('\n', stdout);

   printf("\tPERMANENTFLAGS:");
   mail_flags(info->permanentflags);
   fputc('\n', stdout);

   printf("\tRIGHTS:");
   mail_rights(info->rights);
   fputc('\n', stdout);

   if (info->expunge)
     {
        unsigned int *num;

        printf("\tEXPUNGE:");
        EINA_INARRAY_FOREACH(info->expunge, num)
          printf(" %u", *num);
        fputc('\n', stdout);
     }

   if (info->fetch)
     {
        Email_Imap4_Message *im;

        printf("\tFETCH:");
        EINA_INARRAY_FOREACH(info->fetch, im)
          messageinfo_print(im);
        fputc('\n', stdout);
     }

   if (info->uidvalidity) printf("\tUIDVALIDITY: %llu\n", info->uidvalidity);
   if (info->uidnext) printf("\tUIDNEXT: %llu\n", info->uidnext);
   return ECORE_CALLBACK_RENEW;
}

static void
mbox_delete(Email_Operation *op, Email_Operation_Status status)
{
   char *mbox = email_operation_data_get(op);
   switch (status)
     {
      case EMAIL_OPERATION_STATUS_OK:
        printf("DELETED TEST MAILBOX SUCCESSFULLY!\n");
        email_quit(email_operation_email_get(op), mail_quit, NULL);
        break;
      case EMAIL_OPERATION_STATUS_NO:
        printf("FAILED TO CREATE MAILBOX '%s': %s\n", IMAP_TEST_MBOX_NAME, email_operation_status_message_get(op));
        break;
      case EMAIL_OPERATION_STATUS_BAD:
        printf("IMAP SERVER APPEARS TO BE MECHANICALLY HANDICAPPED!\n");
      default: break;
     }
   free(mbox);
}

static void
mbox_sent(Email_Operation *op, Email_Operation_Status status)
{
   Email_Message *msg = email_operation_data_get(op);
   switch (status)
     {
      case EMAIL_OPERATION_STATUS_OK:
        printf("APPENDED MESSAGE TO MAILBOX SUCCESSFULLY!\n");
        break;
      case EMAIL_OPERATION_STATUS_NO:
        printf("FAILED TO APPEND MESSAGE TO MAILBOX: %s\n", email_operation_status_message_get(op));
        break;
      case EMAIL_OPERATION_STATUS_BAD:
        printf("IMAP SERVER APPEARS TO BE MECHANICALLY HANDICAPPED!\n");
      default: break;
     }
   email_message_free(msg);
}

static void
mbox_create(Email_Operation *op, Email_Operation_Status status)
{
   char *mbox = email_operation_data_get(op);
   switch (status)
     {
      case EMAIL_OPERATION_STATUS_OK:
        printf("CREATED TEST MAILBOX SUCCESSFULLY!\n");
        break;
      case EMAIL_OPERATION_STATUS_NO:
        printf("FAILED TO CREATE MAILBOX '%s': %s\n", mbox, email_operation_status_message_get(op));
        break;
      case EMAIL_OPERATION_STATUS_BAD:
        printf("IMAP SERVER APPEARS TO BE MECHANICALLY HANDICAPPED!\n");
        free(mbox);
      default: break;
     }
}

static void
mail_expunge(Email_Operation *op, Email_Operation_Status status)
{
   switch (status)
     {
      case EMAIL_OPERATION_STATUS_OK:
        printf("EXPUNGED TEST MAILBOX SUCCESSFULLY!\n");
        break;
      case EMAIL_OPERATION_STATUS_NO:
        printf("FAILED TO EXPUNGE MAILBOX '%s': %s\n", email_imap4_mbox_get(email_operation_email_get(op)), email_operation_status_message_get(op));
        break;
      case EMAIL_OPERATION_STATUS_BAD:
        printf("IMAP SERVER APPEARS TO BE MECHANICALLY HANDICAPPED!\n");
      default: break;
     }
}

static Eina_Bool
mail_fetch(Email_Operation *op, Eina_Inarray *fetch)
{
   Email_Imap4_Message *im;

   switch (email_operation_imap4_status_get(op))
     {
      case EMAIL_OPERATION_STATUS_OK:
        if (!fetch) break;
        EINA_INARRAY_FOREACH(fetch, im)
          messageinfo_print(im);
        break;
      case EMAIL_OPERATION_STATUS_NO:
        printf("FAILED TO FETCH TEST MAIL: %s\n", email_operation_status_message_get(op));
        break;
      case EMAIL_OPERATION_STATUS_BAD:
        printf("IMAP SERVER APPEARS TO BE MECHANICALLY HANDICAPPED!\n");
      default: break;
     }
   return EINA_TRUE;
}

static Eina_Bool
mail_select(Email_Operation *op, Email_Imap4_Mailbox_Info *info)
{
   const char *acc = "READ-WRITE";
   Email_Message *msg;
   Email_Contact *ec;
   Email_Message_Part *part;
   char *mbox = email_operation_data_get(op);

   if (!info)
     {
        printf("SELECT FAILED!\n");
        free(mbox);
        ecore_main_loop_quit();
        return ECORE_CALLBACK_RENEW;
     }
   if (info->access == EMAIL_IMAP4_MAILBOX_ACCESS_READONLY)
     acc = "READ-ONLY";
   printf("SELECT (%s) INBOX: %s\n", email_operation_status_message_get(op), acc);
   mailboxinfo_print(NULL, 0, info);
   email_imap4_noop(info->e);
   msg = email_message_new();
   email_message_content_set(msg, email_message_part_text_new(IMAP_TEST_MESSAGE, sizeof(IMAP_TEST_MESSAGE) - 1));
   ec = email_contact_new(IMAP_TEST_MESSAGE_FROM);
   email_contact_name_set(ec, IMAP_TEST_MESSAGE_FROM_NAME);
   email_message_from_add(msg, ec);
   email_contact_free(ec);
   ec = email_contact_new(IMAP_TEST_MESSAGE_TO);
   email_message_contact_add(msg, ec, EMAIL_MESSAGE_CONTACT_TYPE_TO);
   email_contact_free(ec);
   email_message_subject_set(msg, IMAP_TEST_MESSAGE_SUBJECT);
   part = email_message_part_new("testname", "text/plain");
   email_message_part_content_set(part, "herbergerber", sizeof("herbergerber"));
   email_message_part_add(msg, part);
   op = email_imap4_append(info->e, mbox, msg, EMAIL_IMAP4_MAIL_FLAG_DRAFT | EMAIL_IMAP4_MAIL_FLAG_DELETED, mbox_sent, msg);
   email_operation_blocking_set(op); //ensure APPEND completes before next command
   email_operation_blocking_set(email_imap4_noop(info->e));
   op = email_imap4_fetch(info->e, (unsigned int[]){0,0},
     EMAIL_IMAP4_UTIL_FETCH_INFOS(
       EMAIL_IMAP4_UTIL_FETCH_BASIC(FLAGS),
       EMAIL_IMAP4_UTIL_FETCH_BASIC(ENVELOPE),
       EMAIL_IMAP4_UTIL_FETCH_BASIC(UID),
       EMAIL_IMAP4_UTIL_FETCH_BASIC(BODYSTRUCTURE),
       //EMAIL_IMAP4_UTIL_FETCH_BODY_MIME(1, 0, 0, EMAIL_IMAP4_UTIL_BODY_PARTS(1, 1)),
       EMAIL_IMAP4_UTIL_FETCH_BODY_TEXT(1, 0, 0, NULL),
       //EMAIL_IMAP4_UTIL_FETCH_BODY_HEADER(1, 0, 0, NULL),
       EMAIL_IMAP4_UTIL_FETCH_BASIC(INTERNALDATE)
     ),
     mail_fetch, NULL);
   email_operation_blocking_set(op); //ensure FETCH completes before next command
   op = email_imap4_expunge(info->e, mail_expunge, NULL);
   email_operation_blocking_set(op); //ensure EXPUNGE completes before next command
   email_imap4_close(info->e, NULL, NULL);
   email_imap4_delete(info->e, mbox, mbox_delete, mbox);
   return EINA_TRUE;
}

static const char *const MBOX_ATTRS[] =
{
   "HASCHILDREN",
   "HASNOCHILDREN",
   "MARKED",
   "NOINFERIORS",
   "NOSELECT",
   "UNMARKED",
};

static void
mail_list_attrs(Email_Imap4_Mailbox_Attribute flags)
{
   unsigned int x;
   for (x = 0; x < 6; x++)
     if (flags & (1 << x))
       printf(" %s", MBOX_ATTRS[x]);
}

static Eina_Bool
mail_list(Email_Operation *op, Eina_List *list)
{
   Email_List_Item_Imap4 *it;
   const Eina_List *l;
   Email *e = email_operation_email_get(op);
   char buf[1024];
   const Eina_Inarray *namespaces;
   Email_Imap4_Namespace *ns;
   char *mbox;

   printf("LIST (%s)\n", email_operation_status_message_get(op));
   EINA_LIST_FOREACH(list, l, it)
     {
        printf("%s: (", it->name);
        mail_list_attrs(it->attributes);
        printf(" )\n");
     }
   namespaces = email_imap4_namespaces_get(e, EMAIL_IMAP4_NAMESPACE_PERSONAL);
   if (namespaces)
     {
        EINA_INARRAY_FOREACH(namespaces, ns)
          {
             /* ensure we use the correct namespace so our new mbox doesn't get rejected */
             snprintf(buf, sizeof(buf), "%s" IMAP_TEST_MBOX_NAME, ns->prefix);
             break;
          }
     }
   mbox = strdup(namespaces ? buf : IMAP_TEST_MBOX_NAME);
   op = email_imap4_create(e, mbox, mbox_create, mbox);
   email_operation_blocking_set(op);
   email_imap4_select(e, mbox, mail_select, mbox);
   return EINA_TRUE;
}

static Eina_Bool
con(void *d EINA_UNUSED, int type EINA_UNUSED, Email *e)
{
   email_imap4_list(e, NULL, "%", mail_list, NULL);
   return ECORE_CALLBACK_RENEW;
}

int
main(int argc, char *argv[])
{
   Email *e;
   char *pass;

   if (argc < 3)
     {
        fprintf(stderr, "Usage: %s username server\n", argv[0]);
        exit(1);
     }

   email_init();
   eina_log_domain_level_set("email", EINA_LOG_LEVEL_DBG);
   eina_log_domain_level_set("ecore_con", EINA_LOG_LEVEL_DBG);
   pass = getpass_x("Password: ");
   e = email_new(argv[1], pass, NULL);
   email_imap4_set(e);
   ecore_event_handler_add(EMAIL_EVENT_CONNECTED, (Ecore_Event_Handler_Cb)con, NULL);
   ecore_event_handler_add(EMAIL_EVENT_MAILBOX_STATUS, (Ecore_Event_Handler_Cb)mailboxinfo_print, NULL);
   email_connect(e, argv[2], EINA_TRUE);
   ecore_main_loop_begin();

   return 0;
}
