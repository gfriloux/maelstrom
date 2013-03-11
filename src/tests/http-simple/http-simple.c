/*
 * Copyright 2010, 2011, 2012 Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Ecore.h>
#include <Azy.h>

static Eina_Binbuf *buf = NULL;

static Eina_Bool
ret_(Azy_Client *cli __UNUSED__, int type __UNUSED__, Azy_Content *content)
{
   Azy_Net *net;
   if (azy_content_error_is_set(content))
     {
        printf("Error encountered: %s\n", azy_content_error_message_get(content));
        return ECORE_CALLBACK_RENEW;
     }

   net = azy_net_buffer_new(eina_binbuf_string_steal(buf), eina_binbuf_length_get(buf), azy_net_transport_get(azy_content_net_get(content)), 1);
   if (!azy_content_deserialize(content, net))
     fprintf(stderr, "FAIL!\n");
   else
     {
        Azy_Rss *rss;

        rss = azy_content_return_get(content);
        azy_rss_print("> ", 0, rss);
        azy_rss_free(rss);
     }
   azy_net_free(net);
   ecore_main_loop_quit();
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
download_status(void *data __UNUSED__, int type __UNUSED__, Azy_Event_Transfer_Progress *ev)
{
   void *buffer;
   size_t len;

   printf("%zu bytes (%i total) transferred for id %u\n", ev->size, azy_net_message_length_get(ev->net), ev->id);
   buffer = azy_net_buffer_steal(ev->net, &len);
   if (!buf) buf = eina_binbuf_manage_new_length(buffer, len);
   else
     {
        eina_binbuf_append_length(buf, buffer, len);
        free(buffer);
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
disconnected(void *data __UNUSED__, int type __UNUSED__, Azy_Client *ev)
{
   printf("%s:%s:%d\n", __FILE__, __PRETTY_FUNCTION__, __LINE__);
   if (!azy_client_redirect(ev))
     ecore_main_loop_quit();
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
connected(void *data __UNUSED__, int type __UNUSED__, Azy_Client *cli)
{
   Azy_Client_Call_Id id;

   if (!azy_client_current(cli))
     {
        id = azy_client_blank(cli, AZY_NET_TYPE_GET, NULL, NULL, NULL);
        EINA_SAFETY_ON_TRUE_RETURN_VAL(!id, ECORE_CALLBACK_RENEW);
     }

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
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!azy_client_host_set(cli, "http://git.enlightenment.org", 80), 1);

   EINA_SAFETY_ON_TRUE_RETURN_VAL(!azy_client_connect(cli, EINA_FALSE), 1);

   azy_net_uri_set(azy_client_net_get(cli), "/core/efl.git/atom/?h=master");

   azy_net_protocol_set(azy_client_net_get(cli), AZY_NET_PROTOCOL_HTTP_1_0);

   ecore_event_handler_add(AZY_CLIENT_CONNECTED, (Ecore_Event_Handler_Cb)connected, NULL);
   ecore_event_handler_add(AZY_CLIENT_TRANSFER_COMPLETE, (Ecore_Event_Handler_Cb)ret_, NULL);
   ecore_event_handler_add(AZY_CLIENT_DISCONNECTED, (Ecore_Event_Handler_Cb)disconnected, NULL);
   ecore_event_handler_add(AZY_EVENT_TRANSFER_PROGRESS, (Ecore_Event_Handler_Cb)download_status, NULL);
   ecore_main_loop_begin();
   azy_client_free(cli);

   eina_binbuf_free(buf);
   azy_shutdown();
   ecore_shutdown();
   eina_shutdown();
   return 0;
}

