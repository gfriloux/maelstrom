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


const char *error400 = "HTTP/1.1 400 Bad Request\r\n"
                       "Connection: close\r\n"
                       "Content-Type: text/html\r\n\r\n"
                       "<html>"
                       "<head>"
                       "<title>400 Bad Request</title>"
                       "</head>"
                       "<body>"
                       "<h1>Bad Request</h1>"
                       "<p>The server received a request it could not understand.</p>"
                       "</body>"
                       "</html>";
const char *error500 = "HTTP/1.1 500 Internal Server Error\r\n"
                       "Connection: close\r\n\r\n"
                       "Content-Type: text/html\r\n\r\n"
                       "<html>"
                       "<head>"
                       "<title>500 Internal Server Error</title>"
                       "</head>"
                       "<body>"
                       "<h1>Internal Server Error</h1>"
                       "<p>The server encountered an internal error or misconfigurationand was unable to complete your request.</p>"
                       "</body>"
                       "</html>";
const char *error501 = "HTTP/1.1 501 Method Not Implemented\r\n"
                       "Allow: TRACE\r\n"
                       "Connection: close\r\n\r\n"
                       "Content-Type: text/html\r\n\r\n"
                       "<html>"
                       "<head>"
                       "<title>501 Method Not Implemented</title>"
                       "</head>"
                       "<body>"
                       "<h1>Method Not Implemented</h1>"
                       "</body>"
                       "</html>";

static void                       _azy_server_client_new(Azy_Server      *server,
                                                          Ecore_Con_Client *conn);
static void                       _azy_server_module_free(Azy_Server_Module *module,
                                                           Eina_Bool           shutdown);
static Azy_Server_Module        *_azy_server_module_new(Azy_Server_Module_Def *def,
                                                          Azy_Server_Client     *client);
static Azy_Server_Module_Def    *_azy_server_module_def_find(Azy_Server *server,
                                                               const char  *name);
static Azy_Server_Module        *_azy_server_client_module_find(Azy_Server_Client *client,
                                                                  const char         *name);
static Azy_Server_Module_Method *_azy_server_module_method_find(Azy_Server_Module *module,
                                                                  const char         *name);
static Eina_Bool                  _azy_server_client_method_run(Azy_Server_Client *client,
                                                                 Azy_Content       *content);

static void      _azy_server_client_send(Azy_Server_Client *client,
                                          Azy_Content       *content);
static Eina_Bool _azy_server_client_download(Azy_Server_Client *client);
static Eina_Bool _azy_server_client_upload(Azy_Server_Client *client);
static Eina_Bool _azy_server_client_rpc(Azy_Server_Client *client,
                                         Azy_Net_Transport  t);

static Eina_Bool _azy_server_client_handler_request(Azy_Server_Client *client);
static Eina_Bool _azy_server_client_handler_del(Azy_Server_Client         *client,
                                                 int                         type,
                                                 Ecore_Con_Event_Client_Del *ev);
static Eina_Bool _azy_server_client_handler_data(Azy_Server_Client          *client,
                                                  int                          type,
                                                  Ecore_Con_Event_Client_Data *ev);

static void      _azy_server_client_free(Azy_Server_Client *client);


static void
_azy_server_module_free(Azy_Server_Module *module,
                         Eina_Bool           shutdown)
{
   if (!module)
     return;

   if (shutdown && module->def && module->def->shutdown)
     module->def->shutdown(module);

   free(module->data);
   free(module);
}

static Azy_Server_Module *
_azy_server_module_new(Azy_Server_Module_Def *def,
                        Azy_Server_Client     *client)
{
   DBG("(client=%p)", client);
   EINA_SAFETY_ON_NULL_RETURN_VAL(def, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, NULL);

   Azy_Server_Module *s = calloc(sizeof(Azy_Server_Module), 1);

   if (def->data_size > 0)
     s->data = calloc(1, def->data_size);

   s->def = def;
   s->client = client;

   if (s->def->init && !s->def->init(s))
     {
        _azy_server_module_free(s, EINA_FALSE);
        return NULL;
     }

   return s;
}

static Azy_Server_Module_Def *
_azy_server_module_def_find(Azy_Server *server,
                             const char  *name)
{
   Eina_List *l;
   Azy_Server_Module_Def *def;

   DBG("(server=%p)", server);
   EINA_SAFETY_ON_NULL_RETURN_VAL(server, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);

   EINA_LIST_FOREACH(server->module_defs, l, def)
     {
        if (!strcmp(def->name, name))
          return def;
     }

   return NULL;
}

static Azy_Server_Module_Method *
_azy_server_module_method_find(Azy_Server_Module *module,
                                const char         *name)
{
   Eina_List *l;
   Azy_Server_Module_Method *method;

   DBG("(module=%p, name='%s')", module, name);
   EINA_SAFETY_ON_NULL_RETURN_VAL(module, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(module->def, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);

   EINA_LIST_FOREACH(module->def->methods, l, method)
     if (!strcmp(method->name, name))
       {
          INFO("Found method with name: '%s'", name);
          return method;
       }

   INFO("Could not find method with name: '%s'", name);
   return NULL;
}

static Eina_Bool
_azy_server_client_method_run(Azy_Server_Client *client,
                               Azy_Content       *content)
{
   Azy_Server_Module *module = NULL;
   Azy_Server_Module_Method *method;
   Eina_Bool retval = EINA_FALSE;
   const char *module_name;

   DBG("(client=%p, content=%p)", client, content);
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(content, EINA_FALSE);

   INFO("Running RPC for %s", content->method);
   /* get Azy_Server_Module object for current connection and given module name */
   if (!(module_name = azy_content_module_name_get(content, client->net->http.req.http_path)))
     {
        azy_content_error_faultmsg_set(content, -1, "Undefined module name.");
        return EINA_FALSE;
     }

   if (!(module = _azy_server_client_module_find(client, module_name)))
     {
        Azy_Server_Module_Def *def;

        def = _azy_server_module_def_find(client->server, module_name);

        eina_stringshare_del(module_name);
        if (!def)
          {
             azy_content_error_faultmsg_set(content, -1, "Unknown module %s.", module_name);
             return EINA_FALSE;
          }

        if (!(module = _azy_server_module_new(def, client)))
          {
             azy_content_error_faultmsg_set(content, -1, "Module initialization failed.");
             return EINA_FALSE;
          }

        client->modules = eina_list_append(client->modules, module);
     }
   else
     eina_stringshare_del(module_name);

   module->content = content;

   method = _azy_server_module_method_find(module, azy_content_method_get(content));

   if (method)
     {
        if (module->def->pre)
          {
             if (module->def->pre(module, content))
               retval = method->method(module, content);

             else if (!azy_content_retval_get(content) && !azy_content_error_is_set(content))
               azy_content_error_faultmsg_set(content, -1, "Pre-call did not return value or set error.");
          }
        else
          {
             retval = method->method(module, content);
          }
     }
   else if (module->def->fallback)
     {
        if (module->def->fallback(module, content))
          {
             if ((azy_content_retval_get(content)) && (!azy_content_error_is_set(content)))
               azy_content_error_faultmsg_set(content, -1, "Fallback did not return value or set error.");
          }
        else
          azy_content_error_faultmsg_set(content, -1, "Method %s not found in %s module.", azy_content_method_get(content), module->def->name);
     }
   else
     azy_content_error_faultmsg_set(content, -1, "Method %s not found in %s module.", azy_content_method_get(content), module->def->name);

   if (module->def->post)
     module->def->post(module, content);

   module->content = NULL;
   return retval;
}

static void
_azy_server_client_new(Azy_Server      *server,
                        Ecore_Con_Client *conn)
{
   if (!server) return;
   Azy_Server_Client *client = calloc(sizeof(Azy_Server_Client), 1);

   if (!client) return;
   client->net = azy_net_new(conn);
   client->net->server_client = EINA_TRUE;
   client->ip = ecore_con_client_ip_get(conn);
   client->server = server;
   client->last_used = ecore_time_get();
   client->session_id = azy_uuid_new();

   client->del = ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DEL, (Ecore_Event_Handler_Cb)_azy_server_client_handler_del, client);
   client->data = ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DATA, (Ecore_Event_Handler_Cb)_azy_server_client_handler_data, client);
   /* FIXME: is there other data I want to shove into these handlers? */
}

static void
_azy_server_client_free(Azy_Server_Client *client)
{
   Azy_Server_Module *s;
   if (!client)
     return;

   DBG("(client=%p)", client);
   azy_net_free(client->net);
   EINA_LIST_FREE(client->modules, s)
     _azy_server_module_free(s, EINA_TRUE);
   if (client->session_id)
     eina_stringshare_del(client->session_id);

   if (client->del)
     ecore_event_handler_del(client->del);
   if (client->data)
     ecore_event_handler_del(client->data);

   free(client);
}

static Azy_Server_Module *
_azy_server_client_module_find(Azy_Server_Client *client,
                                const char         *name)
{
   Eina_List *l;
   Azy_Server_Module *module;

   DBG("(client=%p)", client);
   EINA_SAFETY_ON_NULL_RETURN_VAL(client, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);

   EINA_LIST_FOREACH(client->modules, l, module)
     if (!strcmp(module->def->name, name))
       {
          INFO("Found module with name: '%s'", name);
          return module;
       }

   INFO("Could not find module with name: '%s'", name);
   return NULL;
}

static Eina_Bool
_azy_server_client_download(Azy_Server_Client *client)
{
   Eina_List *l;
   Azy_Server_Module_Def *def;
   Azy_Content *content;

   DBG("(client=%p)", client);

   /* for each available module type, check if it has download hook */
   EINA_LIST_FOREACH(client->server->module_defs, l, def)
     {
        if (def->download)
          {
             Azy_Server_Module *module = NULL, *existing_module;
             Eina_List *l;

             /* check if module is instantiated */
             EINA_LIST_FOREACH(client->modules, l, existing_module)
               if (existing_module->def == def)
                 module = existing_module;

             /* it's not, instantiate it now */
             if (!module)
               {
                  module = _azy_server_module_new(def, client);
                  client->modules = eina_list_append(client->modules, module);
               }

             if (def->download(module))
               {
                  _azy_server_client_send(client, module->content);
                  return ECORE_CALLBACK_RENEW;
               }
          }
     }

   client->net->http.res.http_code = 501;
   client->net->http.res.http_msg = azy_net_http_msg_get(501);
   azy_net_header_set(client->net, "Content-Type", "text/plain");
   content = azy_content_new(NULL);
   azy_content_buffer_set(content, (unsigned char *)strdup("Download hook is not implemented."), 33);

   _azy_server_client_send(client, content);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_azy_server_client_upload(Azy_Server_Client *client)
{
   Eina_List *l;
   Azy_Server_Module_Def *def;
   Azy_Content *content;

   DBG("(client=%p)", client);

   EINA_LIST_FOREACH(client->server->module_defs, l, def)
     {
        if (def->upload)
          {
             Azy_Server_Module *module = NULL, *existing_module;
             Eina_List *l;

             EINA_LIST_FOREACH(client->modules, l, existing_module)
               if (existing_module->def == def)
                 module = existing_module;

             if (!module)
               {
                  module = _azy_server_module_new(def, client);
                  client->modules = eina_list_append(client->modules, module);
               }

             if (def->upload(module))
               {
                  _azy_server_client_send(client, module->content);
                  return ECORE_CALLBACK_RENEW;
               }
          }
     }

   client->net->http.res.http_code = 501;
   client->net->http.res.http_msg = azy_net_http_msg_get(501);
   azy_net_header_set(client->net, "Content-Type", "text/plain");
   content = azy_content_new(NULL);
   azy_content_buffer_set(content, (unsigned char *)strdup("Upload hook is not implemented."), 31);

   _azy_server_client_send(client, content);
   return ECORE_CALLBACK_RENEW;
}

static void
_azy_server_client_send(Azy_Server_Client *client,
                         Azy_Content       *content)
{
   Eina_Strbuf *header;

   DBG("(client=%p, content=%p)", client, content);

#ifdef ISCOMFITOR
   azy_content_dump(content, 0);
#endif

   if (!client->net->http.res.http_code)
     {
        client->net->http.res.http_code = 200;
        client->net->http.res.http_msg = azy_net_http_msg_get(200);
     }
   if (client->session_id)
     {
        char idstr[48];
        snprintf(idstr, sizeof(idstr), "sessid=%s;", client->session_id);
        azy_net_header_set(client->net, "Set-Cookie", idstr);
     }

   client->net->type = AZY_NET_TYPE_RESPONSE;
   azy_net_message_length_set(client->net, content->length);
   header = azy_net_header_create(client->net);
   EINA_SAFETY_ON_NULL_GOTO(header, error);
   azy_net_header_reset(client->net);
   INFO("Sending response for method: '%s'", content->method);


   char buf[64];
   snprintf(buf, sizeof(buf), "SENDING:\n<<<<<<<<<<<<<\n%%.%is%%.%llis\n<<<<<<<<<<<<<", eina_strbuf_length_get(header), content->length);
   INFO(buf, eina_strbuf_string_get(header), content->buffer);


   EINA_SAFETY_ON_TRUE_GOTO(!ecore_con_client_send(client->net->conn, eina_strbuf_string_get(header), eina_strbuf_length_get(header)), error);
   INFO("Send [1/2] complete! %i bytes queued for sending.", eina_strbuf_length_get(header));
   EINA_SAFETY_ON_TRUE_GOTO(!ecore_con_client_send(client->net->conn, content->buffer, content->length), error);
   INFO("Send [2/2] complete! %lli bytes queued for sending.", content->length);
   ecore_con_client_flush(client->net->conn);

error:
   eina_strbuf_free(header);
   azy_content_free(content);

}

static Eina_Bool
_azy_server_client_rpc(Azy_Server_Client *client,
                        Azy_Net_Transport  t)
{
   DBG("(client=%p)", client);
   Azy_Content *content;

   content = azy_content_new(NULL);
   EINA_SAFETY_ON_NULL_GOTO(content, error);

   if (!azy_content_unserialize_request(content, t, (char *)client->net->buffer, client->net->size))
     azy_content_error_faultmsg_set(content, -1, "Unserialize request failure.");
   else
     _azy_server_client_method_run(client, content);

error:
   free(client->net->buffer);
   client->net->buffer = NULL;
   client->net->size = 0;

   if (!content)
     return EINA_FALSE;

   azy_content_serialize_response(content, t);
   azy_net_transport_set(client->net, t);
   _azy_server_client_send(client, content);

   return EINA_TRUE;
}

static Eina_Bool
_azy_server_client_handler_request(Azy_Server_Client *client)
{
   DBG("(client=%p)", client);

   if ((client->net->type != AZY_NET_TYPE_GET) && (client->net->type != AZY_NET_TYPE_POST) &&
       (client->net->type != AZY_NET_TYPE_PUT))
     return ECORE_CALLBACK_RENEW;

   if (client->net->type == AZY_NET_TYPE_GET)
     {
        _azy_server_client_download(client);
        return ECORE_CALLBACK_RENEW;
     }
   if (client->net->type == AZY_NET_TYPE_PUT)
     {
        _azy_server_client_upload(client);
        return ECORE_CALLBACK_RENEW;
     }
   else
     {
        client->net->transport = azy_events_net_transport_get(azy_net_header_get(client->net, "content-type"));
        switch (client->net->transport)
          {
           case AZY_NET_JSON:
           case AZY_NET_XML:
             _azy_server_client_rpc(client, client->net->transport);
             break;

           case AZY_NET_TEXT:
           case AZY_NET_HTML:
           default:
             /* FIXME: this isn't supported yet but probably should be somehow? */
             azy_events_connection_kill(client->net->conn, EINA_TRUE, error501);
             _azy_server_client_free(client);
             break;

          }
     }

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_azy_server_client_handler_data(Azy_Server_Client          *client,
                                 int                          type,
                                 Ecore_Con_Event_Client_Data *ev)
{
   int offset = 0;
   void *data = (ev) ? ev->data : NULL;
   int len = (ev) ? ev->size : 0;
   static unsigned char *overflow;
   static long long int overflow_length;

   DBG("(client=%p, ev=%p, data=%p)", client, ev, (ev) ? ev->data : NULL);
   client->net->nodata = EINA_FALSE;

#ifdef ISCOMFITOR
   char buf[64];
   snprintf(buf, sizeof(buf), "RECEIVED:\n<<<<<<<<<<<<<\n%%.%is\n<<<<<<<<<<<<<", len);
   INFO(buf, data);
#endif

   if (!client->net->size)
     {
        client->net->buffer = overflow;
        client->net->size = overflow_length;
        INFO("%s: Set recv size to %lli from overflow", client->ip, client->net->size);
        
        /* returns offset where http header line ends */
         if (!(offset = azy_events_type_parse(client->net, type, data, len)) && ev)
           return azy_events_connection_kill(client->net->conn, EINA_TRUE, NULL);
         else if (!offset && overflow)
           {
              client->net->buffer = NULL;
              client->net->size = 0;
              INFO("%s: Set recv size to %lli, storing overflow of %lli", client->ip, client->net->size, overflow_length);
              return EINA_TRUE;
           }
         else
           {
              overflow = NULL;
              overflow_length = 0;
           }
     }
   
   if (!client->net->headers_read) /* if headers aren't done being read, keep reading them */
     {
        if (!azy_events_header_parse(client->net, data, len, offset) && ev)
          return azy_events_connection_kill(client->net->conn, EINA_TRUE, error500);
     }
   else
     {   /* otherwise keep appending to buffer */
        unsigned char *tmp;
        
        if (client->net->size + len > client->net->http.content_length)
          tmp = realloc(client->net->buffer, client->net->http.content_length);
        else
          tmp = realloc(client->net->buffer, client->net->size + len);

        EINA_SAFETY_ON_NULL_RETURN_VAL(tmp, ECORE_CALLBACK_RENEW);
        
        client->net->buffer = tmp;

        if (client->net->size + len > client->net->http.content_length)
          {
             overflow_length = (client->net->size + len) - client->net->http.content_length;
             memcpy(client->net->buffer + client->net->size, data, len - overflow_length);
             overflow = malloc(overflow_length);
             if (!overflow)
               {
                  ERR("alloc failure, losing %lli bytes", overflow_length);
                  _azy_server_client_handler_request(client);
                  return ECORE_CALLBACK_RENEW;
               }
             memcpy(overflow, data + (len - overflow_length), overflow_length);
             WARN("%s: Extra content length of %lli! Set recv size to %lli (previous %lli)",
                  client->ip, overflow_length, client->net->size + len - overflow_length, client->net->size);
             client->net->size += len - overflow_length;

          }
        else
          {
             memcpy(client->net->buffer + client->net->size, data, len);
             client->net->size += len;

             INFO("%s: Incremented recv size to %lli (+%i)", client->ip, client->net->size, len);
          }
     }

   if (client->net->overflow)
     {
        overflow = client->net->overflow;
        overflow_length = client->net->overflow_length;
        client->net->overflow = NULL;
        client->net->overflow_length = 0;
     }

   if (!client->net->headers_read)
     return ECORE_CALLBACK_RENEW;

   if (client->net->size >= client->net->http.content_length)
      _azy_server_client_handler_request(client);

   if (overflow && (!client->net->buffer))
     {
        WARN("%s: Calling %s again to try using %lli bytes of overflow data...", client->ip, __PRETTY_FUNCTION__, overflow_length);
        _azy_server_client_handler_data(client, type, NULL);
        if (!overflow)
          WARN("%s: Overflow has been successfully used!", client->ip);
        else
          WARN("%s: Overflow could not be used, storing %lli bytes for next event.", client->ip, overflow_length);
     }

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_azy_server_client_handler_del(Azy_Server_Client         *client,
                                int                         type __UNUSED__,
                                Ecore_Con_Event_Client_Del *ev)
{
   DBG("(client=%p)", client);
   INFO("Client %s has disconnected!", ecore_con_client_ip_get(ev->client));
   if (client->net->timer)
     ecore_timer_del(client->net->timer);
   _azy_server_client_free(client);

   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
azy_server_client_add_handler(Azy_Server                *server,
                               int                         type __UNUSED__,
                               Ecore_Con_Event_Client_Add *ev)
{
   DBG("(server=%p)", server);

   INFO("Client %s has connected!", ecore_con_client_ip_get(ev->client));
   ecore_con_client_timeout_set(ev->client, 1);
   _azy_server_client_new(server, ev->client);

   return ECORE_CALLBACK_RENEW;
}

