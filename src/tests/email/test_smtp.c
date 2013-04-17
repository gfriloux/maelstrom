#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Ecore.h>
#include "Email.h"

char *getpass_x(const char *prompt);

static Eina_Bool
_send(Email_Operation *op EINA_UNUSED, Email_Message *msg, Eina_Bool success)
{
   Email *e;

   e = email_message_email_get(msg);
   printf("Send %s!\n", success ? "successful" : "failed");
   email_quit(e, NULL, NULL);
   return EINA_TRUE;
}

static Eina_Bool
disc(void *d EINA_UNUSED, int type EINA_UNUSED, Email *e EINA_UNUSED)
{
   printf("Disconnected\n");
   ecore_main_loop_quit();
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
con(void *d EINA_UNUSED, int type EINA_UNUSED, Email *e)
{
   Email_Message *msg;
   Email_Contact *ec;
   char buf[1024];

   printf("Connected\n");
   msg = email_message_new();
   email_message_subject_set(msg, "sup dawg");
   printf("Send message to:\n");
   scanf("%1023s", buf);
   ec = email_contact_new(buf);
   email_message_contact_add(msg, ec, EMAIL_MESSAGE_CONTACT_TYPE_TO);
   email_message_content_set(msg, email_message_part_text_new("test message!", sizeof("test message!") - 1));
   email_message_send(e, msg, _send, NULL);
   email_contact_free(ec);
   return ECORE_CALLBACK_RENEW;
}

int
main(int argc, char *argv[])
{
   Email *e;
   char *pass;

   if (argc < 4)
     {
        fprintf(stderr, "Usage: %s username server send_domain\n", argv[0]);
        exit(1);
     }

   email_init();
   eina_log_domain_level_set("email", EINA_LOG_LEVEL_DBG);
   eina_log_domain_level_set("ecore_con", EINA_LOG_LEVEL_DBG);
   pass = getpass_x("Password: ");
   e = email_new(argv[1], pass, NULL);
   email_smtp_set(e, argv[3]);
   ecore_event_handler_add(EMAIL_EVENT_CONNECTED, (Ecore_Event_Handler_Cb)con, NULL);
   ecore_event_handler_add(EMAIL_EVENT_DISCONNECTED, (Ecore_Event_Handler_Cb)disc, NULL);
   email_connect(e, argv[2], EINA_FALSE);
   ecore_main_loop_begin();

   return 0;
}
