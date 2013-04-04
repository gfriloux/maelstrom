#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Ecore.h>
#include "Email.h"

#ifndef __UNUSED__
# define __UNUSED__
#endif

char *getpass_x(const char *prompt);

static int count = 0;

static void
mail_quit(Email *e __UNUSED__)
{
   ecore_main_loop_quit();
}

static void
mail_retr(Email *e, Eina_Binbuf *buf)
{
   printf("Received message (%zu bytes): \n%*s\n",
     eina_binbuf_length_get(buf), (int)eina_binbuf_length_get(buf),
     (char*)eina_binbuf_string_get(buf));
   if (--count == 0)
     {
        email_pop3_rset(e, NULL);
        email_quit(e, (Email_Cb)mail_quit);
     }
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
mail_list(Email *e, Eina_List *list)
{
   Email_List_Item_Imap4 *it;
   const Eina_List *l;

   EINA_LIST_FOREACH(list, l, it)
     {
        printf("%s: (", it->name);
        mail_list_flags(it->flags);
        printf(" )\n");
     }
   return EINA_TRUE;
}

static Eina_Bool
con(void *d __UNUSED__, int type __UNUSED__, Email *e)
{
   email_imap4_list(e, NULL, "%", mail_list);
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
