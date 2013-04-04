#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Ecore.h>
#include "Email.h"

char *getpass_x(const char *prompt);

static void
mail_quit(Email *e EINA_UNUSED)
{
   ecore_main_loop_quit();
}

static void
mail_select(Email_Operation *op, Eina_Bool success)
{
   printf("SELECT INBOX: %s\n", success ? "SUCCESS!" : "FAIL!");
}

static const char *const MBOX_FLAGS[] =
{
   [0] = "HASCHILDREN",
   [1] = "HASNOCHILDREN",
   [2] = "MARKED",
   [3] = "NOINFERIORS",
   [4] = "NOSELECT",
   [5] = "UNMARKED",
};

static void
mail_list_flags(Email_Imap_Mailbox_Flag flags)
{
   unsigned int x;
   for (x = 0; x <= 5; x++)
     if (flags & (1 << x))
       printf(" %s", MBOX_FLAGS[x]);
}

static Eina_Bool
mail_list(Email_Operation *op, Eina_List *list)
{
   Email_List_Item_Imap4 *it;
   const Eina_List *l;

   EINA_LIST_FOREACH(list, l, it)
     {
        printf("%s: (", it->name);
        mail_list_flags(it->flags);
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
