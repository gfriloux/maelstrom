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

static Azy_Client_Call_Id azy_client_send_id__ = 0;

int AZY_CLIENT_DISCONNECTED;
int AZY_CLIENT_CONNECTED;
int AZY_CLIENT_RETURN;
int AZY_CLIENT_RESULT;
int AZY_CLIENT_ERROR;

/**
 * @brief Allocate a new client object
 * This function creates a new client object for use in connecting to a
 * server.
 * @return The new client, or #NULL on failure
 */
Azy_Client *
azy_client_new(void)
{
   Azy_Client *client;

   if (!(client = calloc(1, sizeof(Azy_Client))))
     return NULL;

   client->add = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD, (Ecore_Event_Handler_Cb)_azy_client_handler_add, client);
   client->del = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, (Ecore_Event_Handler_Cb)_azy_client_handler_del, client);

   AZY_MAGIC_SET(client, AZY_MAGIC_CLIENT);
   return client;
}

/**
 * @brief Get the data previously associated with a client
 * This function retrieves the data previously set to @p client
 * with azy_client_data_set.
 * @param client The client object (NOT #NULL)
 * @return The data, or #NULL on error
 */
void *
azy_client_data_get(Azy_Client *client)
{
   DBG("(client=%p)", client);
   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return NULL;
     }
   return client->data;
}

/**
 * @brief Set the data previously associated with a client
 * This function sets the data associated with @p client to @p data
 * for retrieval with azy_client_data_get.
 * @param client The client object (NOT #NULL)
 * @param data The data to associate
 */
void
azy_client_data_set(Azy_Client *client, const void *data)
{
   DBG("(client=%p)", client);
   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return;
     }
   client->data = (void*)data;
}

/**
 * @brief Set the connection info for a client
 * This function sets the server address and port for a client to
 * connect to.  The address can be either an ip string (ipv6 supported)
 * or a web address.
 * @param client The client object (NOT #NULL)
 * @param addr The server's address (NOT #NULL)
 * @param port The port on the server (-1 < port < 65536)
 * @return #EINA_TRUE on success, else #EINA_FALSE
 */
Eina_Bool
azy_client_host_set(Azy_Client *client,
                     const char  *addr,
                     int          port)
{
   DBG("(client=%p)", client);
   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return EINA_FALSE;
     }
   if ((!addr) || (port < 0) || (port > 65536))
     return EINA_FALSE;

   if (client->addr)
     eina_stringshare_del(client->addr);
   client->addr = eina_stringshare_add(addr);
   client->port = port;
   
   return EINA_TRUE;
}

/**
 * @brief Get the address of the server that the client connects to
 * This function returns the address string of the server that @p client
 * connects to.  The returned string is stringshared but still
 * belongs to the client object.
 * @param client The client object (NOT #NULL)
 * @return The address string, or #NULL on failure
 */
const char *
azy_client_addr_get(Azy_Client *client)
{
   DBG("(client=%p)", client);
   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return NULL;
     }

   return client->addr;
}

/**
 * @brief Set the address of the server that the client connects to
 * This function sets the address string of the server that @p client
 * connects to.
 * @param client The client object (NOT #NULL)
 * @return The address string
 */
Eina_Bool
azy_client_addr_set(Azy_Client *client, const char *addr)
{
   DBG("(client=%p)", client);
   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return EINA_FALSE;
     }
   EINA_SAFETY_ON_NULL_RETURN_VAL(addr, EINA_FALSE);

   client->addr = eina_stringshare_add(addr);
   return EINA_TRUE;
}

/**
 * @brief Get the port that the client connects to
 * This function returns the port number on the server that @p client
 * connects to.
 * @param client The client object (NOT #NULL)
 * @return The port number, or -1 on failure
 */
int
azy_client_port_get(Azy_Client *client)
{
   DBG("(client=%p)", client);
   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return -1;
     }

   return client->port;
}

/**
 * @brief Set the port that the client connects to
 * This function sets the port number on the server that @p client
 * connects to.
 * @param client The client object (NOT #NULL)
 * @param port The port number (-1 < port < 65536)
 * @return #EINA_TRUE on success, or #EINA_FALSE on failure
 */
Eina_Bool
azy_client_port_set(Azy_Client *client, int port)
{
   DBG("(client=%p)", client);
   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return EINA_FALSE;
     }
   if ((port < 0) || (port > 65535))
     return EINA_FALSE;

   client->port = port;
   return EINA_TRUE;
}

/**
 * @brief Connect the client to its server
 * This function begins the connection process for @p client to its
 * previously set server.  This will return EINA_FALSE immediately if an error occurs.
 * @param client The client object (NOT #NULL)
 * @param secure If #EINA_TRUE, TLS will be used in the connection
 * @return #EINA_TRUE if successful, or #EINA_FALSE on failure
 */
Eina_Bool
azy_client_connect(Azy_Client *client,
                    Eina_Bool    secure)
{
   DBG("(client=%p)", client);
   Ecore_Con_Server *svr;
   int flags = ECORE_CON_REMOTE_NODELAY;

   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return EINA_FALSE;
     }
   if ((client->connected) || (!client->addr) || (!client->port))
     return EINA_FALSE;

   client->secure = !!secure;

   if (secure) flags |= ECORE_CON_USE_MIXED;

   if (!(svr = ecore_con_server_connect(flags, client->addr, client->port, NULL)))
     return EINA_FALSE;

   ecore_con_server_data_set(svr, client);

   ecore_con_server_timeout_set(svr, 1);

   client->net = azy_net_new(svr);

   return EINA_TRUE;
}

/**
 * @brief Get the network object associated with the client
 * This function returns the #Azy_Net object associated with @p client
 * which is used for all outgoing data transmissions.  From here, azy_net
 * namespaced functions can be called as normal upon the returned object.
 * Note that the returned object belongs to the client, and will only exist
 * if the client is connected.
 * @param client The client object (NOT #NULL)
 * @return The #Azy_Net object, or #NULL on failure
 */
Azy_Net *
azy_client_net_get(Azy_Client *client)
{
   DBG("(client=%p)", client);
   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return NULL;
     }

   return client->net;
}

/**
 * @brief Check whether a client is connected
 * This function returns true only when the client is connected.
 * @param client The client (NOT #NULL)
 * @return #EINA_TRUE if the client is connected, else #EINA_FALSE
 */
Eina_Bool
azy_client_connected_get(Azy_Client *client)
{
   DBG("(client=%p)", client);
   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return EINA_FALSE;
     }
   return client->connected;
}

/**
 * @brief Close a client's connection
 * This function is the opposite of azy_client_connect, it
 * terminates an existing connection.
 * @param client The client (NOT #NULL)
 */
void
azy_client_close(Azy_Client *client)
{
   DBG("(client=%p)", client);

   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return;
     }
   EINA_SAFETY_ON_FALSE_RETURN(client->connected);
   EINA_SAFETY_ON_NULL_RETURN(client->net);
   
   ecore_con_server_del(client->net->conn);

   azy_net_free(client->net);
   client->net = NULL;

   client->connected = EINA_FALSE;
}

/**
 * @brief Set a callback for an #Azy_Client_Call_Id
 * This function is used to setup a callback to be called for the response of
 * a transmission with @p id, overriding (disabling) the AZY_CLIENT_RESULT event
 * for that call.  If a previous callback was set for @p id, this will overwrite it.
 * @param client The client (NOT #NULL)
 * @param id The transmission id (> 0)
 * @param callback The callback to use (NOT #NULL)
 * @return #EINA_TRUE on success, or #EINA_FALSE on failure
 */
Eina_Bool
azy_client_callback_set(Azy_Client *client,
                         Azy_Client_Call_Id id,
                         Azy_Client_Return_Cb callback)
{
   DBG("(client=%p, id=%u)", client, id);

   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return EINA_FALSE;
     }
   EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(id < 1, EINA_FALSE);

   if (!client->callbacks)
     client->callbacks = eina_hash_int32_new(NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(client->callbacks, EINA_FALSE);

   return eina_hash_add(client->callbacks, &id, callback);
}

/**
 * @brief Set a callback to free the retval struct of @p id
 * This function should not be called by users.
 * @param client The client
 * @param id The transmission id
 * @param callback The free callback
 */
Eina_Bool
azy_client_callback_free_set(Azy_Client *client,
                              Azy_Client_Call_Id id,
                              Ecore_Cb callback)
{
   DBG("(client=%p, id=%u)", client, id);

   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return EINA_FALSE;
     }
   EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(id < 1, EINA_FALSE);

   if (!client->free_callbacks)
     client->free_callbacks = eina_hash_int32_new(NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(client->free_callbacks, EINA_FALSE);

   return eina_hash_add(client->free_callbacks, &id, callback);
}

/**
 * @brief Make a method call using a connected client
 * This function is used to make a method call on @p client as defined in
 * @p content, using content-type defined by @p transport and the deserialization
 * function specified by @p cb.  This should generally not be used by users, as azy_parser
 * will automatically generate the correct calls from a .azy file.
 * @param client The client (NOT #NULL)
 * @param content The content containing the method name and parameters (NOT #NULL)
 * @param transport The content-type (xml/json/etc) to use
 * @param cb The deserialization callback to use for the response (NOT #NULL)
 * @return The #Azy_Client_Call_Id of the transmission, to be used with azy_client_callback_set,
 * or 0 on failure
 */
Azy_Client_Call_Id
azy_client_call(Azy_Client       *client,
                 Azy_Content      *content,
                 Azy_Net_Transport transport,
                 Azy_Content_Cb    cb)
{
   Eina_Strbuf *msg;
   Azy_Client_Handler_Data *handler_data;

   DBG("(client=%p, net=%p, content=%p)", client, client->net, content);

   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return 0;
     }
   EINA_SAFETY_ON_NULL_RETURN_VAL(client->net, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(content, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(content->method, 0);

   if (!client->connected)
     {
        ERR("Can't perform RPC on closed connection.");
        return 0;
     }

   INFO("New method call: '%s'", content->method);

   while (++azy_client_send_id__ < 1);

   content->id = azy_client_send_id__;

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
   INFO("Send [1/2] complete! %zu bytes queued for sending.", eina_strbuf_length_get(msg));
   eina_strbuf_free(msg);
   msg = NULL;

   EINA_SAFETY_ON_TRUE_GOTO(!ecore_con_server_send(client->net->conn, content->buffer, content->length), error);
   INFO("Send [2/2] complete! %lli bytes queued for sending.", content->length);
   ecore_con_server_flush(client->net->conn);
 
   handler_data = calloc(1, sizeof(Azy_Client_Handler_Data));
   EINA_SAFETY_ON_NULL_RETURN_VAL(handler_data, 0);
   handler_data->client = client;
   handler_data->method = eina_stringshare_ref(content->method);
   handler_data->callback = cb;
   handler_data->content_data = content->data;

   handler_data->id = azy_client_send_id__;
   AZY_MAGIC_SET(handler_data, AZY_MAGIC_CLIENT_DATA_HANDLER);
   if (!client->conns)
     {
        client->recv =  ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA,
                                                (Ecore_Event_Handler_Cb)_azy_client_handler_data, handler_data);
        ecore_con_server_data_set(client->net->conn, client);
     }
     
   client->conns = eina_list_append(client->conns, handler_data);

   DBG("(client=%p, net=%p, content=%p, handler_data=%p)", client, client->net, content, handler_data);
   return azy_client_send_id__;
error:
   if (msg)
     eina_strbuf_free(msg);
   return 0;
}

/**
 * @brief Send arbitrary data to a connected server
 * This function is used to send arbitrary data to a connected server using @p client.
 * It will automatically generate the http header based on the content-length and other
 * data specified in the client's #Azy_Net object.
 * @param client The client (NOT #NULL)
 * @param data The data to send (NOT #NULL)
 * @param length The length (in bytes) of the data (> 0)
 * @return The #Azy_Client_Call_Id of the transmission, to be used with azy_client_callback_set,
 * or 0 on failure
 */
Azy_Client_Call_Id
azy_client_send(Azy_Client   *client,
                 unsigned char *data,
                 int            length)
{
   Eina_Strbuf *msg;
   Azy_Client_Handler_Data *handler_data;

      if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return 0;
     }
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
   INFO("Send [1/2] complete! %zi bytes queued for sending.", eina_strbuf_length_get(msg));
   eina_strbuf_free(msg);
   msg = NULL;

   EINA_SAFETY_ON_TRUE_GOTO(!ecore_con_server_send(client->net->conn, data, length), error);
   INFO("Send [2/2] complete! %i bytes queued for sending.", length);
   ecore_con_server_flush(client->net->conn);

   EINA_SAFETY_ON_TRUE_RETURN_VAL(!(handler_data = calloc(1, sizeof(Azy_Client_Handler_Data))), 0);

   if (!client->conns)
     {
        client->recv =  ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA,
                                                (Ecore_Event_Handler_Cb)_azy_client_handler_data, handler_data);
        ecore_con_server_data_set(client->net->conn, client);
     }

   handler_data->client = client;
   AZY_MAGIC_SET(handler_data, AZY_MAGIC_CLIENT_DATA_HANDLER);

   while (++azy_client_send_id__ < 1);

   handler_data->id = azy_client_send_id__;
   client->conns = eina_list_append(client->conns, handler_data);
   
   return azy_client_send_id__;

error:
   if (msg)
     eina_strbuf_free(msg);
   return 0;
}

/**
 * @brief Validate a transmission attempt
 * This function is used to check both the #Azy_Client_Call_Id and the #Azy_Content
 * of an azy_client_call or azy_client_send attempt, and will additionally set
 * an #Azy_Client_Return_Cb and log the calling function name upon failure.
 * Note that this function also calls azy_content_error_reset.
 * Also note: THIS FUNCTION IS MEANT TO BE USED IN A MACRO!!!!
 * @param cli The client (NOT #NULL)
 * @param err_content The content used to make the call which may contain an error (NOT #NULL)
 * @param ret The call id
 * @param cb The callback to set for @p ret (NOT #NULL)
 * @param func The function name of the calling function
 * @return This function returns #EINA_TRUE only if the call was successful and @p cb was set, else #EINA_FALSE
 */
Eina_Bool
azy_client_call_checker(Azy_Client *cli, Azy_Content *err_content, Azy_Client_Call_Id ret, Azy_Client_Return_Cb cb, const char *func)
{
   DBG("(cli=%p, cb=%p, func='%s')", cli, cb, func);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cli, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(err_content, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, EINA_FALSE);
   if (!ret) return EINA_FALSE;

   if (azy_content_error_is_set(err_content))
     {
        ERR("%s:\n%s", func ? func : "<calling function not specified>", azy_content_error_message_get(err_content));
        azy_content_error_reset(err_content);
        return EINA_FALSE;
     }

   azy_content_error_reset(err_content);
   if (!azy_client_callback_set(cli, ret, cb))
     {
        CRI("Could not set callback from %s!", func ? func : "<calling function not specified>");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Free an #Azy_Client
 * This function frees a client and ALL associated data.  If called
 * on a connected client, azy_client_close will be called and then the client
 * will be freed.
 * @param client The client (NOT #NULL)
 */
void
azy_client_free(Azy_Client *client)
{
   DBG("(client=%p)", client);

   if (!AZY_MAGIC_CHECK(client, AZY_MAGIC_CLIENT))
     {
        AZY_MAGIC_FAIL(client, AZY_MAGIC_CLIENT);
        return;
     }

   if (client->connected)
     azy_client_close(client);
   AZY_MAGIC_SET(client, AZY_MAGIC_NONE);
   if (client->addr)
     eina_stringshare_del(client->addr);
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
