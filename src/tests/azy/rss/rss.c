/*
 * Copyright 2010, 2011, 2012 Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Ecore.h>
#include <Azy.h>

static Eina_Bool
ret_(Azy_Client *cli EINA_UNUSED, int type EINA_UNUSED, Azy_Event_Client_Transfer_Complete *cse)
{
   Azy_Rss *ret;
   Eina_Bool rss;
   Azy_Content *content = cse->content;

   if (azy_content_error_is_set(content))
     {
        printf("Error encountered: %s\n", azy_content_error_message_get(content));
        return ECORE_CALLBACK_RENEW;
     }

   ret = azy_content_return_get(content, &rss);
 //  printf("Success? %s!\n", ret ? "YES" : "NO");

   if (rss) azy_rss_print("> ", 0, ret);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
download_status(void *data EINA_UNUSED, int type EINA_UNUSED, Azy_Event_Client_Transfer_Progress *ev)
{
   int total = -1;

   if (ev->net)
     total = azy_net_content_length_get(ev->net);
   if (total > 0)
     printf("%zu bytes (%i total) transferred for id %u\n", ev->size, total, ev->id);
   else
     printf("%zu bytes transferred for id %u\n", ev->size, ev->id);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
disconnected(void *data EINA_UNUSED, int type EINA_UNUSED, Azy_Client *ev)
{
   printf("%s:%s:%d\n", __FILE__, __PRETTY_FUNCTION__, __LINE__);
   if (!azy_client_redirect(ev))
     ecore_main_loop_quit();
   return ECORE_CALLBACK_RENEW;
}

int
main(void)
{
   Azy_Client *cli;
   Azy_Client_Call_Id id;

   eina_init();
   ecore_init();
   azy_init();
   eina_log_domain_level_set("azy", EINA_LOG_LEVEL_DBG);
   eina_log_domain_level_set("ecore_con", EINA_LOG_LEVEL_DBG);

   ecore_event_handler_add(AZY_EVENT_CLIENT_TRANSFER_COMPLETE, (Ecore_Event_Handler_Cb)ret_, NULL);
   ecore_event_handler_add(AZY_EVENT_CLIENT_DISCONNECTED, (Ecore_Event_Handler_Cb)disconnected, NULL);
   ecore_event_handler_add(AZY_EVENT_CLIENT_TRANSFER_PROGRESS, (Ecore_Event_Handler_Cb)download_status, NULL);

   cli = azy_client_util_connect("http://www.avweb.com/topnews.xml");
   EINA_SAFETY_ON_NULL_RETURN_VAL(cli, 1);
   id = azy_client_blank(cli, AZY_NET_TYPE_GET, NULL, NULL, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!id, ECORE_CALLBACK_CANCEL);
   azy_client_callback_free_set(cli, id, (Ecore_Cb)azy_rss_free);

   ecore_main_loop_begin();

   azy_client_free(cli);

   cli = azy_client_util_connect("http://git.enlightenment.org/core/efl.git/atom/?h=master");
   EINA_SAFETY_ON_NULL_RETURN_VAL(cli, 1);
   id = azy_client_blank(cli, AZY_NET_TYPE_GET, NULL, NULL, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!id, ECORE_CALLBACK_CANCEL);
   azy_client_callback_free_set(cli, id, (Ecore_Cb)azy_rss_free);

   ecore_main_loop_begin();
   azy_client_free(cli);

   azy_shutdown();
   ecore_shutdown();
   eina_shutdown();
   return 0;
}

