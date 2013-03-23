/*
 * Copyright 2010, 2011, 2012 Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Ecore.h>

#include "identica_Common_Azy.h"

static Eina_Error
ret_(Azy_Client *cli EINA_UNUSED, int type EINA_UNUSED, Azy_Event_Client_Transfer_Complete *cse)
{
   Eina_List *l, *r;
   identica_Ident *ret;
   Azy_Content *content = cse->content;

   if (azy_content_error_is_set(content))
     {
        printf("Error encountered: %s\n", azy_content_error_message_get(content));
        return azy_content_error_code_get(content);
     }

   r = azy_content_return_get(content, NULL);
 //  printf("Success? %s!\n", ret ? "YES" : "NO");

   EINA_LIST_FOREACH(r, l, ret)
     identica_Ident_print(NULL, 0, ret);
   return AZY_ERROR_NONE;
}

static Eina_Bool
download_status(void *data EINA_UNUSED, int type EINA_UNUSED, Azy_Event_Client_Transfer_Progress *ev)
{
   int total = -1;

   if (ev->net)
     total = azy_net_content_length_get(ev->net);
   if (total > 0)
     printf("%zu bytes (%i total) transferred for id %u\n", ev->current, total, ev->id);
   else
     printf("%zu bytes transferred for id %u\n", ev->current, ev->id);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
disconnected(void *data EINA_UNUSED, int type EINA_UNUSED, void *data2 EINA_UNUSED)
{
   printf("%s:%s:%d\n", __FILE__, __PRETTY_FUNCTION__, __LINE__);
   ecore_main_loop_quit();
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
connected(void *data EINA_UNUSED, int type EINA_UNUSED, Azy_Client *ev)
{
   Azy_Client_Call_Id id;

   id = azy_client_blank(ev, AZY_NET_TYPE_GET, NULL, (Azy_Content_Cb)azy_value_to_Array_identica_Ident, NULL);
   if (!id) ecore_main_loop_quit();
   azy_client_callback_free_set(ev, id, (Ecore_Cb)Array_identica_Ident_free);

   return ECORE_CALLBACK_RENEW;
}

int
main(void)
{
   Azy_Client *cli;
   eina_init();
   ecore_init();
   azy_init();
   eina_log_domain_level_set("azy", EINA_LOG_LEVEL_DBG);
   eina_log_domain_level_set("ecore_con", EINA_LOG_LEVEL_DBG);

   cli = azy_client_new();

   EINA_SAFETY_ON_NULL_RETURN_VAL(cli, 1);

   EINA_SAFETY_ON_TRUE_RETURN_VAL(!azy_client_host_set(cli, "identi.ca", 443), 1);

   azy_client_secure_set(cli, EINA_TRUE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!azy_client_connect(cli), 1);

   azy_net_uri_set(azy_client_net_get(cli), "/api/statuses/public_timeline.json");
   azy_net_protocol_set(azy_client_net_get(cli), AZY_NET_PROTOCOL_HTTP_1_0);

   ecore_event_handler_add(AZY_EVENT_CLIENT_CONNECTED, (Ecore_Event_Handler_Cb)connected, NULL);
   ecore_event_handler_add(AZY_EVENT_CLIENT_TRANSFER_COMPLETE, (Ecore_Event_Handler_Cb)ret_, NULL);
   ecore_event_handler_add(AZY_EVENT_CLIENT_DISCONNECTED, (Ecore_Event_Handler_Cb)disconnected, NULL);
   ecore_event_handler_add(AZY_EVENT_CLIENT_TRANSFER_PROGRESS, (Ecore_Event_Handler_Cb)download_status, NULL);

   ecore_main_loop_begin();

   azy_client_free(cli);

   azy_shutdown();
   ecore_shutdown();
   eina_shutdown();
   return 0;
}

