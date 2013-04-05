#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Ecore.h>
#include "Email.h"

char *getpass_x(const char *prompt);

static void
mail_quit(Email_Operation *op EINA_UNUSED)
{
   ecore_main_loop_quit();
}

static const char *const MBOX_FLAGS[] =
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
mail_flags(Email_Imap_Mailbox_Attribute flags)
{
   unsigned int x;
   for (x = 0; x < 7; x++)
     if (flags & (1 << x))
       printf(" %s", MBOX_FLAGS[x]);
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
mail_rights(Email_Imap_Mailbox_Rights flags)
{
   unsigned int x;
   for (x = 0; x < 12; x++)
     if (flags & (1 << x))
       printf(" %s", MBOX_RIGHTS[x]);
}

static Eina_Bool
mail_select(Email_Operation *op, Email_Imap4_Mailbox_Info *info)
{
   const char *acc = "READ-WRITE";

   if (info->access == EMAIL_IMAP_MAILBOX_ACCESS_READONLY)
     acc = "READ-ONLY";
   printf("SELECT INBOX: %s\n", acc);

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

   if (info->uidvalidity) printf("\tUIDVALIDITY: %llu\n", info->uidvalidity);
   if (info->uidnext) printf("\tUIDNEXT: %llu\n", info->uidnext);
   email_quit(email_operation_email_get(op), mail_quit, NULL);
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
mail_list_attrs(Email_Imap_Mailbox_Attribute flags)
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

   EINA_LIST_FOREACH(list, l, it)
     {
        printf("%s: (", it->name);
        mail_list_attrs(it->attributes);
        printf(" )\n");
     }
   email_imap4_select(email_operation_email_get(op), "INBOX", mail_select, NULL);
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
   email_connect(e, argv[2], EINA_TRUE);
   ecore_main_loop_begin();

   return 0;
}
