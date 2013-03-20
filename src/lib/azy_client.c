/*
 * Copyright 2010 Mike Blumenkrantz <mike@zentific.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <regex.h>
#include <ctype.h>

#include "Azy.h"
#include "azy_private.h"

static unsigned int __azy_client_send_id = 0;
static char _init = 0;

int AZY_CLIENT_DISCONNECTED;
int AZY_CLIENT_CONNECTED;
int AZY_CLIENT_RETURN;
int AZY_CLIENT_ERROR;

void *
azy_client_data_get(Azy_Client *client)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, NULL);
   return client->data;
}

void
azy_client_data_set(Azy_Client *client, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(client);
   client->data = data;
}

Azy_Client *
azy_client_new(void)
{
   Azy_Client *client;

   azy_init();

   if (!(client = calloc(sizeof(Azy_Client), 1)))
     return NULL;

   if (EINA_UNLIKELY(!_init))
     {
        AZY_CLIENT_DISCONNECTED = ecore_event_type_new();
        AZY_CLIENT_CONNECTED = ecore_event_type_new();
        AZY_CLIENT_RETURN = ecore_event_type_new();
        AZY_CLIENT_ERROR = ecore_event_type_new();
        _init = 1;
     }

   client->add = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD, (Ecore_Event_Handler_Cb)_azy_client_handler_add, client);
   client->del = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, (Ecore_Event_Handler_Cb)_azy_client_handler_del, client);

   return client;
}

Eina_Bool
azy_client_host_set(Azy_Client *client,
                     const char  *host,
                     int          port)
{
   if ((!client) || (!host) || (port < 1) || (port > 65535))
     return EINA_FALSE;

   if (client->host)
     eina_stringshare_del(client->host);
   client->host = eina_stringshare_add(host);
   client->port = port;
   
   return EINA_TRUE;
}

const char *
azy_client_hostname_get(Azy_Client *client)
{
   if (!client) return NULL;

   return client->host;
}

Eina_Bool
azy_client_hostname_set(Azy_Client *client, const char *hostname)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(hostname, EINA_FALSE);

   client->host = eina_stringshare_add(hostname);
   return EINA_TRUE;
}

int
azy_client_port_get(Azy_Client *client)
{
   if (!client) return -1;

   return client->port;
}

Eina_Bool
azy_client_port_set(Azy_Client *client, int port)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, EINA_FALSE);
   if (port < 1)
     return EINA_FALSE;

   client->port = port;
   return EINA_TRUE;
}

Eina_Bool
azy_client_connect(Azy_Client *client,
                    Eina_Bool    secure)
{
   Ecore_Con_Server *svr;
   int flags = ECORE_CON_REMOTE_NODELAY;

   EINA_SAFETY_ON_NULL_RETURN_VAL(client, EINA_FALSE);
   if ((client->connected) || (!client->host) || (!client->port))
     return EINA_FALSE;

   client->secure = secure;

   if (secure)
     flags |= ECORE_CON_USE_MIXED;

   if (!(svr = ecore_con_server_connect(flags, client->host, client->port, NULL)))
     return EINA_FALSE;

   client->net = azy_net_new(svr);

   return EINA_TRUE;
}

Azy_Net *
azy_client_net_get(Azy_Client *client)
{
   if (!client)
     return NULL;

   return client->net;
}

Eina_Bool
azy_client_connected_get(Azy_Client *client)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, EINA_FALSE);
   return client->connected;
}

void
azy_client_close(Azy_Client *client)
{
   DBG("(client=%p)", client);

   if (!client)
     return;

   if (!client->connected)
     return;

   ecore_con_server_del(client->net->conn);

   azy_net_free(client->net);
   client->net = NULL;

   client->connected = EINA_FALSE;
}

Eina_Bool
azy_client_callback_set(Azy_Client *client,
                         unsigned int id,
                         Azy_Client_Return_Cb callback)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(id < 1, EINA_FALSE);

   if (!client->callbacks)
     client->callbacks = eina_hash_int32_new(NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(client->callbacks, EINA_FALSE);

   return eina_hash_add(client->callbacks, &id, callback);
}

Eina_Bool
azy_client_callback_free_set(Azy_Client *client,
                              unsigned int id,
                              void (*callback)(void*))
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(id < 1, EINA_FALSE);

   if (!client->free_callbacks)
     client->free_callbacks = eina_hash_int32_new(NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(client->free_callbacks, EINA_FALSE);

   return eina_hash_add(client->free_callbacks, &id, callback);
}

unsigned int
azy_client_call(Azy_Client       *client,
                 Azy_Content      *content,
                 Azy_Net_Transport transport,
                 Azy_Content_Cb    cb)
{
   Eina_Strbuf *msg;
   Azy_Client_Handler_Data *handler_data;

   DBG("(client=%p, net=%p, content=%p)", client, client->net, content);

   EINA_SAFETY_ON_NULL_RETURN_VAL(client, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(content, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(content->method, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, 0);

   INFO("New method call: '%s'", content->method);

   if (!client->connected)
     {
        ERR("Can't perform RPC on closed connection.");
        return 0;
     }

   ++__azy_client_send_id;
   if (__azy_client_send_id == 0)
     ++__azy_client_send_id;

   content->id = __azy_client_send_id;

   azy_net_transport_set(client->net, transport);
   if (!azy_content_serialize_request(content, transport))
     return 0;
   azy_net_type_set(client->net, AZY_NET_TYPE_POST);
   if (!azy_net_uri_get(client->net))
     {
        WARN("URI currently set to NULL, defaulting to \"/\"");
        azy_net_uri_set(client->net, "/");
     }

   azy_net_message_length_set(client->net, content->length);
   msg = azy_net_header_create(client->net);
   EINA_SAFETY_ON_NULL_GOTO(msg, error);

#ifdef ISCOMFITOR
   char buf[64];
   snprintf(buf, sizeof(buf), "\nSENDING >>>>>>>>>>>>>>>>>>>>>>>>\n%%.%is%%.%llis\n>>>>>>>>>>>>>>>>>>>>>>>>",
            eina_strbuf_length_get(msg), content->length);
   DBG(buf, eina_strbuf_string_get(msg), content->buffer);
#endif

   EINA_SAFETY_ON_TRUE_GOTO(!ecore_con_server_send(client->net->conn, eina_strbuf_string_get(msg), eina_strbuf_length_get(msg)), error);
   INFO("Send [1/2] complete! %i bytes queued for sending.", eina_strbuf_length_get(msg));
   eina_strbuf_free(msg);
   msg = NULL;

   EINA_SAFETY_ON_TRUE_GOTO(!ecore_con_server_send(client->net->conn, content->buffer, content->length), error);
   INFO("Send [2/2] complete! %lli bytes queued for sending.", content->length);
   ecore_con_server_flush(client->net->conn);
 
   if (!(handler_data = calloc(sizeof(Azy_Client_Handler_Data), 1)))
     return 0;
   handler_data->client = client;
   handler_data->method = eina_stringshare_add(content->method);
   handler_data->callback = cb;

   handler_data->id = __azy_client_send_id;
   client->conns = eina_list_append(client->conns, handler_data);
   if ((!handler_data->data) && (client->conns->data == handler_data))
     /* only set data handler if it's the current call */
     handler_data->data = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, (Ecore_Event_Handler_Cb)_azy_client_handler_data, handler_data);


   DBG("(client=%p, net=%p, content=%p, handler_data=%p)", client, client->net, content, handler_data);
   return __azy_client_send_id;
error:
   if (msg)
     eina_strbuf_free(msg);
   return 0;
}

unsigned int
azy_client_send(Azy_Client   *client,
                 unsigned char *data,
                 int            length)
{
   Eina_Strbuf *msg;
   Azy_Client_Handler_Data *handler_data;

   EINA_SAFETY_ON_NULL_RETURN_VAL(client, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(length < 1, 0);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!ecore_con_server_connected_get(client->net->conn), 0);

   azy_net_message_length_set(client->net, length);
   msg = azy_net_header_create(client->net);
   EINA_SAFETY_ON_NULL_GOTO(msg, error);
#ifdef ISCOMFITOR
   DBG("\nSENDING >>>>>>>>>>>>>>>>>>>>>>>>\n%.*s%.*s\n>>>>>>>>>>>>>>>>>>>>>>>>",
       eina_strbuf_length_get(msg), eina_strbuf_string_get(msg), length, data);
#endif
   EINA_SAFETY_ON_TRUE_GOTO(!ecore_con_server_send(client->net->conn, eina_strbuf_string_get(msg), eina_strbuf_length_get(msg)), error);
   INFO("Send [1/2] complete! %i bytes queued for sending.", eina_strbuf_length_get(msg));
   eina_strbuf_free(msg);
   msg = NULL;

   EINA_SAFETY_ON_TRUE_GOTO(!ecore_con_server_send(client->net->conn, data, length), error);
   INFO("Send [2/2] complete! %i bytes queued for sending.", length);
   ecore_con_server_flush(client->net->conn);

   EINA_SAFETY_ON_TRUE_RETURN_VAL(!(handler_data = calloc(sizeof(Azy_Client_Handler_Data), 1)), 0);

   if (!handler_data->data)
     handler_data->data = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, (Ecore_Event_Handler_Cb)_azy_client_handler_data, handler_data);
   handler_data->client = client;

   ++__azy_client_send_id;
   if (__azy_client_send_id == 0)
     ++__azy_client_send_id;

   handler_data->id = __azy_client_send_id;
   client->conns = eina_list_append(client->conns, handler_data);
   
   return __azy_client_send_id;

error:
   if (msg)
     eina_strbuf_free(msg);
   return 0;
}

void
azy_client_free(Azy_Client *client)
{
   DBG("(client=%p)", client);

   if (!client)
     return;

   azy_client_close(client);
   if (client->host)
     eina_stringshare_del(client->host);
   if (client->session_id)
     eina_stringshare_del(client->session_id);
   if (client->add)
     ecore_event_handler_del(client->add);
   if (client->del)
     ecore_event_handler_del(client->del);
   if (client->callbacks)
     eina_hash_free(client->callbacks);
   if (client->free_callbacks)
     eina_hash_free(client->free_callbacks);
   if (client->conns)
     client->conns = eina_list_free(client->conns);
   free(client);
}
