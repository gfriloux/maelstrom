#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Ecore.h>
#include "Email.h"

char *getpass_x(const char *prompt);

static int count = 0;

static void
mail_quit(Email_Operation *op EINA_UNUSED)
{
   ecore_main_loop_quit();
}

static void
mail_retr(Email_Operation *op, Eina_Binbuf *buf)
{
   printf("Received message (%zu bytes): \n%*s\n",
     eina_binbuf_length_get(buf), (int)eina_binbuf_length_get(buf),
     (char*)eina_binbuf_string_get(buf));
   if (--count == 0)
     {
        email_pop3_rset(email_operation_email_get(op), NULL, NULL);
        email_quit(email_operation_email_get(op), mail_quit, NULL);
     }
}

static Eina_Bool
mail_list(Email_Operation *op, Eina_List *list)
{
   Email_List_Item_Pop3 *it;
   const Eina_List *l;

   EINA_LIST_FOREACH(list, l, it)
     {
        printf("#%u, %zu octets\n", it->id, it->size);
        email_pop3_retrieve(email_operation_email_get(op), it->id, mail_retr, NULL);
        count++;
     }
   return EINA_TRUE;
}

static void
mail_stat(Email_Operation *op, unsigned int num EINA_UNUSED, size_t size EINA_UNUSED)
{
   email_pop3_list(email_operation_email_get(op), mail_list, NULL);
}

static Eina_Bool
con(void *d EINA_UNUSED, int type EINA_UNUSED, Email *e)
{
   email_pop3_stat(e, mail_stat, NULL);
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
   email_pop3_set(e);
   ecore_event_handler_add(EMAIL_EVENT_CONNECTED, (Ecore_Event_Handler_Cb)con, NULL);
   email_connect(e, argv[2], EINA_TRUE);
   ecore_main_loop_begin();

   return 0;
}
