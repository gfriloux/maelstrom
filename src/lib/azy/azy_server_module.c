/*
 * Copyright 2010, 2011, 2012, 2013 Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "azy_private.h"

typedef struct
{
   const void  *data;
   Eina_Free_Cb free_func;
} Azy_Server_Module_Param;

static void
azy_server_module_param_free_(Azy_Server_Module_Param *param)
{
   if (!param) return;
   if (param->free_func) param->free_func((void *)param->data);
   free(param);
}

/**
 * @defgroup Azy_Server_Module Server Module Functions
 * @brief Functions which affect #Azy_Server_Module objects
 * @{
 */

/**
 * @brief Return the data received from a client
 *
 * This function returns the received data from a server module (client).
 * This data is set only when clients have called HTTP PUT, and will be handled by
 * the __upload__ directive in the server.
 * @param module The server module (NOT NULL)
 * @return The module's received data
 */
Azy_Net_Data *
azy_server_module_recv_get(Azy_Server_Module *module)
{
   if (!AZY_MAGIC_CHECK(module, AZY_MAGIC_SERVER_MODULE))
     {
        AZY_MAGIC_FAIL(module, AZY_MAGIC_SERVER_MODULE);
        return NULL;
     }
   return &module->recv;
}

/**
 * @brief Return the private data of a server module
 *
 * This function returns the private data of a server module.
 * This data is set only in the server module definition function,
 * and has the value specified in the __attrs__ section of the module
 * in a .azy file.
 * @param module The server module (NOT NULL)
 * @return The module's data
 */
void *
azy_server_module_data_get(Azy_Server_Module *module)
{
   if (!AZY_MAGIC_CHECK(module, AZY_MAGIC_SERVER_MODULE))
     {
        AZY_MAGIC_FAIL(module, AZY_MAGIC_SERVER_MODULE);
        return NULL;
     }

   return module->data;
}

/**
 * @brief Returns whether a module has stored params
 *
 * This function can be used to determine whether params from previous
 * method runs are currently stored. It is used by the parser.
 * @param module The module (NOT NULL)
 * @return EINA_TRUE if params are stored, else EINA_FALSE
 */
Eina_Bool
azy_server_module_params_exist(Azy_Server_Module *module)
{
   if (!AZY_MAGIC_CHECK(module, AZY_MAGIC_SERVER_MODULE))
     {
        AZY_MAGIC_FAIL(module, AZY_MAGIC_SERVER_MODULE);
        return EINA_FALSE;
     }
   return !!module->params;
}

/**
 * @brief Set a param to a module
 *
 * This function sets a method call param named @p name to a module. It is used by the parser.
 * @param module The module (NOT NULL)
 * @param name The param name (NOT NULL)
 * @param value The param value
 * @param free_func The function to free @p value
 * @return EINA_TRUE on success, else EINA_FALSE
 */
Eina_Bool
azy_server_module_param_set(Azy_Server_Module *module, const char *name, const void *value, Eina_Free_Cb free_func)
{
   Azy_Server_Module_Param *param, *old;
   if (!AZY_MAGIC_CHECK(module, AZY_MAGIC_SERVER_MODULE))
     {
        AZY_MAGIC_FAIL(module, AZY_MAGIC_SERVER_MODULE);
        return EINA_FALSE;
     }
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!name[0], EINA_FALSE);
   if (!module->params) module->params = eina_hash_string_djb2_new((Eina_Free_Cb)azy_server_module_param_free_);
   param = calloc(1, sizeof(Azy_Server_Module_Param));
   EINA_SAFETY_ON_NULL_RETURN_VAL(param, EINA_FALSE);
   param->data = value;
   param->free_func = free_func;

   old = eina_hash_set(module->params, name, param);
   if (old) azy_server_module_param_free_(old);
   return EINA_TRUE;
}

/**
 * @brief Get a param from a module
 *
 * This function gets a previously set method call param
 * named @p name from module @p module. It is used by the parser.
 * @param module The module (NOT NULL)
 * @param name The param name (NOT NULL)
 * @return The param value, NULL on failure
 */
void *
azy_server_module_param_get(Azy_Server_Module *module, const char *name)
{
   Azy_Server_Module_Param *param;
   if (!AZY_MAGIC_CHECK(module, AZY_MAGIC_SERVER_MODULE))
     {
        AZY_MAGIC_FAIL(module, AZY_MAGIC_SERVER_MODULE);
        return NULL;
     }
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(module->params, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!name[0], NULL);

   param = eina_hash_find(module->params, name);
   EINA_SAFETY_ON_NULL_RETURN_VAL(param, NULL);
   return (void *)param->data;
}

/**
 * @brief Return the #Azy_Net object of the current or last
 * module's connection
 *
 * This function is used to return the current module's network information,
 * allowing parsing of headers.
 * If there is no current network information (Chunked transfer), then it will
 * return last module's network structure.
 * @param module The server module (NOT NULL)
 * @return The #Azy_Net object
 */
Azy_Net *
azy_server_module_net_get(Azy_Server_Module *module)
{
   if (!AZY_MAGIC_CHECK(module, AZY_MAGIC_SERVER_MODULE))
     {
        AZY_MAGIC_FAIL(module, AZY_MAGIC_SERVER_MODULE);
        return NULL;
     }
   EINA_SAFETY_ON_NULL_RETURN_VAL(module->client, NULL);

   return module->client->current ? module->client->current : module->client->net;
}

/**
 * @brief Return the #Azy_Content object of the current module's connection
 *
 * This function is used to return the current module's return content object,
 * allowing manipulation of the return value.
 * @note This should only be used on a suspended module.
 * @param module The server module (NOT NULL)
 * @return The #Azy_Content object
 */
Azy_Content *
azy_server_module_content_get(Azy_Server_Module *module)
{
   if (!AZY_MAGIC_CHECK(module, AZY_MAGIC_SERVER_MODULE))
     {
        AZY_MAGIC_FAIL(module, AZY_MAGIC_SERVER_MODULE);
        return NULL;
     }
   return module->content;
}

/**
 * @brief Return the #Azy_Server_Module_Def of a server with a given name
 *
 * This function finds the #Azy_Server_Module_Def with @p name in @p server and
 * returns it.
 * @param server The server object (NOT NULL)
 * @param name The #Azy_Server_Module_Def's name (NOT NULL)
 * @return The #Azy_Server_Module_Def, or NULL on failure
 */
Azy_Server_Module_Def *
azy_server_module_def_find(Azy_Server *server,
                           const char *name)
{
   if (!AZY_MAGIC_CHECK(server, AZY_MAGIC_SERVER))
     {
        AZY_MAGIC_FAIL(server, AZY_MAGIC_SERVER);
        return NULL;
     }
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   if (!server->module_defs) return NULL;

   return eina_hash_find(server->module_defs, name);
}

/**
 * @brief Add a module to the server object
 *
 * This function adds @p module to @p server.  After calling this,
 * the module should not be freed until the server has stopped running.
 * @param server The server object (NOT NULL)
 * @param module The module definition (NOT NULL)
 * @return EINA_TRUE on success, else EINA_FALSE
 */
Eina_Bool
azy_server_module_add(Azy_Server *server,
                      Azy_Server_Module_Def *module)
{
   if (!AZY_MAGIC_CHECK(server, AZY_MAGIC_SERVER))
     {
        AZY_MAGIC_FAIL(server, AZY_MAGIC_SERVER);
        return EINA_FALSE;
     }
   if (!module) return EINA_FALSE;
   if (server->module_defs)
     {
        if (azy_server_module_def_find(server, module->name))
          /* avoid adding same module twice */
          return EINA_TRUE;
     }
   else
     server->module_defs = eina_hash_string_superfast_new(NULL);
   if (!server->module_defs) return EINA_FALSE;

   INFO("Adding new module: '%s'", module->name);
   return eina_hash_add(server->module_defs, module->name, module);
}

/**
 * @brief Remove a module from the server object
 *
 * This function removes @p module from @p server.  Once a module
 * has been removed, its methods can no longer be called.
 * Note that this function only removes the module from the server's list
 * and does not actually free the module.
 * @param server The server object (NOT NULL)
 * @param module The module definition (NOT NULL)
 * @return EINA_TRUE on success, else EINA_FALSE
 */
Eina_Bool
azy_server_module_del(Azy_Server *server,
                      Azy_Server_Module_Def *module)
{
   DBG("server=%p, module=%p", server, module);
   if (!AZY_MAGIC_CHECK(server, AZY_MAGIC_SERVER))
     {
        AZY_MAGIC_FAIL(server, AZY_MAGIC_SERVER);
        return EINA_FALSE;
     }
   EINA_SAFETY_ON_NULL_RETURN_VAL(module, EINA_FALSE);
   return eina_hash_del_by_key(server->module_defs, module->name);
}

/**
 * @brief Remove a module from the server object by name
 *
 * This function removes the module of name @p from @p server.  Once a module
 * has been removed, its methods can no longer be called.
 * Note that this function only removes the module from the server's list
 * and does not actually free the module.
 * @param server The server object (NOT NULL)
 * @param name The module's name (NOT NULL)
 * @return EINA_TRUE on success or module not found, else EINA_FALSE
 */
Eina_Bool
azy_server_module_name_del(Azy_Server *server,
                           const char *name)
{
   DBG("server=%p, name=%s", server, name);
   if (!AZY_MAGIC_CHECK(server, AZY_MAGIC_SERVER))
     {
        AZY_MAGIC_FAIL(server, AZY_MAGIC_SERVER);
        return EINA_FALSE;
     }
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, EINA_FALSE);
   return eina_hash_del_by_key(server->module_defs, name);
}

/**
 * @brief Create a new module definition with the given name
 *
 * This function creates a blank #Azy_Server_Module_Def with @p name.
 * @param name The name of the module (NOT NULL)
 * @return The new #Azy_Server_Module_Def, or NULL on failure
 */
Azy_Server_Module_Def *
azy_server_module_def_new(const char *name)
{
   Azy_Server_Module_Def *def;

   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);

   def = calloc(1, sizeof(Azy_Server_Module_Def));
   EINA_SAFETY_ON_NULL_RETURN_VAL(def, NULL);

   def->name = eina_stringshare_add(name);
   AZY_MAGIC_SET(def, AZY_MAGIC_SERVER_MODULE_DEF);
   return def;
}

/**
 * @brief Free the given #Azy_Server_Module_Def
 *
 * This function frees the given #Azy_Server_Module_Def, and should only
 * be called after the module will no longer be used.
 * @param def The #Azy_Server_Module_Def to free
 */
void
azy_server_module_def_free(Azy_Server_Module_Def *def)
{
   if (!def) return;
   if (!AZY_MAGIC_CHECK(def, AZY_MAGIC_SERVER_MODULE_DEF))
     {
        AZY_MAGIC_FAIL(def, AZY_MAGIC_SERVER_MODULE_DEF);
        return;
     }

   eina_stringshare_del(def->name);
   eina_hash_free(def->methods);
   if (def->module) eina_module_free(def->module);

   AZY_MAGIC_SET(def, AZY_MAGIC_NONE);
   free(def);
}

/**
 * @brief Set the __init__ and __shutdown__ callback functions for a #Azy_Server_Module_Def
 *
 * This function sets the callbacks called upon module load and module shutdown for @p def.
 * @param def The module definition (NOT NULL)
 * @param init The callback function to call upon module init
 * @param sd The callback function to call upon module shutdown
 */
void
azy_server_module_def_init_shutdown_set(Azy_Server_Module_Def *def,
                                        Azy_Server_Module_Cb init,
                                        Azy_Server_Module_Shutdown_Cb sd)
{
   if (!AZY_MAGIC_CHECK(def, AZY_MAGIC_SERVER_MODULE_DEF))
     {
        AZY_MAGIC_FAIL(def, AZY_MAGIC_SERVER_MODULE_DEF);
        return;
     }
   def->init = init;
   def->shutdown = sd;
}

/**
 * @brief Set the __pre__ and __post__ callback functions for a #Azy_Server_Module_Def
 *
 * This function sets the callbacks called before and after method calls for @p def.
 * @param def The module definition (NOT NULL)
 * @param pre The callback function to call immediately before method calls
 * @param post The callback function to call immediately after method calls
 */
void
azy_server_module_def_pre_post_set(Azy_Server_Module_Def *def,
                                   Azy_Server_Module_Pre_Cb pre,
                                   Azy_Server_Module_Content_Cb post)
{
   if (!AZY_MAGIC_CHECK(def, AZY_MAGIC_SERVER_MODULE_DEF))
     {
        AZY_MAGIC_FAIL(def, AZY_MAGIC_SERVER_MODULE_DEF);
        return;
     }
   def->pre = pre;
   def->post = post;
}

/**
 * @brief Set the __download__ and __upload__ callback functions for a #Azy_Server_Module_Def
 *
 * This function sets the callbacks called before and after method calls for @p def.
 * @param def The module definition (NOT NULL)
 * @param download The callback function to call for HTTP GET requests
 * @param upload The callback function to call for HTTP PUT requests
 */
void
azy_server_module_def_download_upload_set(Azy_Server_Module_Def *def,
                                          Azy_Server_Module_Cb download,
                                          Azy_Server_Module_Cb upload)
{
   if (!AZY_MAGIC_CHECK(def, AZY_MAGIC_SERVER_MODULE_DEF))
     {
        AZY_MAGIC_FAIL(def, AZY_MAGIC_SERVER_MODULE_DEF);
        return;
     }
   def->download = download;
   def->upload = upload;
}

/**
 * @brief Set the __fallback__ callback function for a #Azy_Server_Module_Def
 *
 * This function sets the callback that is called any time a user attempts
 * to call an undefined rpc method.
 * @param def The module definition (NOT NULL)
 * @param fallback The callback function to call when an undefined method is requested
 */
void
azy_server_module_def_fallback_set(Azy_Server_Module_Def *def,
                                   Azy_Server_Module_Content_Cb fallback)
{
   if (!AZY_MAGIC_CHECK(def, AZY_MAGIC_SERVER_MODULE_DEF))
     {
        AZY_MAGIC_FAIL(def, AZY_MAGIC_SERVER_MODULE_DEF);
        return;
     }
   def->fallback = fallback;
}

/**
 * @brief Add a method to a module
 *
 * This function adds a callable rpc method to module @p def.  After adding,
 * @p method should be considered as belonging to @p def until the module is unloaded.
 * @param def The module definition (NOT NULL)
 * @param method The method to add (NOT NULL)
 */
void
azy_server_module_def_method_add(Azy_Server_Module_Def *def,
                                 Azy_Server_Module_Method *method)
{
   Azy_Server_Module_Method *old;
   if (!AZY_MAGIC_CHECK(def, AZY_MAGIC_SERVER_MODULE_DEF))
     {
        AZY_MAGIC_FAIL(def, AZY_MAGIC_SERVER_MODULE_DEF);
        return;
     }

   if (!AZY_MAGIC_CHECK(method, AZY_MAGIC_SERVER_MODULE_METHOD))
     {
        AZY_MAGIC_FAIL(method, AZY_MAGIC_SERVER_MODULE_METHOD);
        return;
     }
   if (!def->methods) def->methods = eina_hash_string_superfast_new((Eina_Free_Cb)azy_server_module_method_free);
   old = eina_hash_set(def->methods, method->name, method);
   if (old) azy_server_module_method_free(old);
}

/**
 * @brief Remove a method from a module
 *
 * This function removes a callable rpc method from module @p def.  After
 * removal, @p method will no longer be callable.
 * @note This does not free the method object.
 * @param def The module definition (NOT NULL)
 * @param method The method to remove (NOT NULL)
 * @return EINA_TRUE on success, else EINA_FALSE
 */
Eina_Bool
azy_server_module_def_method_del(Azy_Server_Module_Def *def,
                                 Azy_Server_Module_Method *method)
{
   if (!AZY_MAGIC_CHECK(def, AZY_MAGIC_SERVER_MODULE_DEF))
     {
        AZY_MAGIC_FAIL(def, AZY_MAGIC_SERVER_MODULE_DEF);
        return EINA_FALSE;
     }

   if (!AZY_MAGIC_CHECK(method, AZY_MAGIC_SERVER_MODULE_METHOD))
     {
        AZY_MAGIC_FAIL(method, AZY_MAGIC_SERVER_MODULE_METHOD);
        return EINA_FALSE;
     }
   return !!eina_hash_set(def->methods, method->name, NULL);
}

/**
 * @brief Create a new method object with specified name and callback
 *
 * This function creates a new method object with stringshared @p name and
 * callback @p cb.
 * @param name The name of the method
 * @param cb The callback of the method
 * @return The new #Azy_Server_Module_Method object, or NULL on failure
 */
Azy_Server_Module_Method *
azy_server_module_method_new(const char *name,
                             Azy_Server_Module_Content_Cb cb)
{
   Azy_Server_Module_Method *method;

   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);

   method = calloc(1, sizeof(Azy_Server_Module_Method));
   EINA_SAFETY_ON_NULL_RETURN_VAL(method, NULL);

   method->name = eina_stringshare_add(name);
   method->method = cb;

   AZY_MAGIC_SET(method, AZY_MAGIC_SERVER_MODULE_METHOD);
   return method;
}

/**
 * @brief Free a method object
 *
 * This function frees a method object.  After calling, the method will no
 * longer be callable.  This function must only be called AFTER
 * azy_server_module_def_method_del to avoid undefined methods remaining
 * in the module's method list after they've been freed.
 * @param method The method to free (NOT NULL)
 */
void
azy_server_module_method_free(Azy_Server_Module_Method *method)
{
   if (!AZY_MAGIC_CHECK(method, AZY_MAGIC_SERVER_MODULE_METHOD))
     {
        AZY_MAGIC_FAIL(method, AZY_MAGIC_SERVER_MODULE_METHOD);
        return;
     }

   AZY_MAGIC_SET(method, AZY_MAGIC_NONE);
   eina_stringshare_del(method->name);
   free(method);
}

/**
 * @brief Get the version of a module
 *
 * This function returns the version number of a module as set with
 * azy_server_module_def_version_set or the __version__() directive in a .azy file.
 * @param m The module (NOT NULL)
 * @return The version of the module, or -1.0 on failure
 */
double
azy_server_module_version_get(Azy_Server_Module *m)
{
   if (!AZY_MAGIC_CHECK(m, AZY_MAGIC_SERVER_MODULE))
     {
        AZY_MAGIC_FAIL(m, AZY_MAGIC_SERVER_MODULE);
        return -1.0;
     }

   return m->def->version;
}

/**
 * @brief Set the version of a module
 *
 * This function sets the version number of a server module.
 * @param def The module def (NOT NULL)
 * @param version The version number of the module
 */
void
azy_server_module_def_version_set(Azy_Server_Module_Def *def,
                                  double version)
{
   if (!AZY_MAGIC_CHECK(def, AZY_MAGIC_SERVER_MODULE_DEF))
     {
        AZY_MAGIC_FAIL(def, AZY_MAGIC_SERVER_MODULE_DEF);
        return;
     }

   def->version = version;
}

/**
 * @brief Return the size of the private data of a module
 *
 * This function is equivalent to calling sizeof(Azy_Server_Module).
 * It returns the total size of the __attrs__ section of a module.
 * @param def The module def (NOT NULL)
 * @return The size of the module, or -1 on failure
 */
int
azy_server_module_def_size_get(Azy_Server_Module_Def *def)
{
   if (!AZY_MAGIC_CHECK(def, AZY_MAGIC_SERVER_MODULE_DEF))
     {
        AZY_MAGIC_FAIL(def, AZY_MAGIC_SERVER_MODULE_DEF);
        return -1;
     }

   return def->data_size;
}

/**
 * @brief Set the size of the private data of a module
 *
 * This function should never be called by users.
 * @param def The module def (NOT NULL)
 * @param size The size of the module
 * @return EINA_TRUE on success, else EINA_FALSE
 */
Eina_Bool
azy_server_module_size_set(Azy_Server_Module_Def *def,
                           int size)
{
   if (!AZY_MAGIC_CHECK(def, AZY_MAGIC_SERVER_MODULE_DEF))
     {
        AZY_MAGIC_FAIL(def, AZY_MAGIC_SERVER_MODULE_DEF);
        return EINA_FALSE;
     }

   def->data_size = size;
   return EINA_TRUE;
}

/**
 * @brief Send data to a client
 *
 * This function is used to queue arbitrary data to send to a client through its module.  It will automatically
 * generate all http header strings from @p net (if provided) including the content-length (based on @p data).
 * @param module The client's #Azy_Server_Module object (NOT NULL)
 * @param net An #Azy_Net object containing http information to use
 * @param data The data to send
 * @return EINA_TRUE on success, else EINA_FALSE
 */
Eina_Bool
azy_server_module_send(Azy_Server_Module *module,
                       Azy_Net *net,
                       const Azy_Net_Data *data)
{
   Eina_Strbuf *header;
   char chunk_size[20];
   Eina_Binbuf *chunk_data;
   Eina_Bool nullify = EINA_FALSE;

   if (!AZY_MAGIC_CHECK(module, AZY_MAGIC_SERVER_MODULE))
     {
        AZY_MAGIC_FAIL(module, AZY_MAGIC_SERVER_MODULE);
        return EINA_FALSE;
     }

   if (net)
     {
        if (!module->client->current)
          {
             module->client->current = net;
             nullify = EINA_TRUE;
          }

        if (net->headers_sent)
          goto post_header;

        Eina_Bool s;
        if ((data) && (net->http.transfer_encoding != AZY_NET_TRANSFER_ENCODING_CHUNKED))
          azy_net_content_length_set(net, data->size);
        if (!net->http.res.http_code)
          azy_net_code_set(net, 200);  /* OK */
        azy_net_type_set(net, AZY_NET_TYPE_RESPONSE);
        EINA_SAFETY_ON_TRUE_RETURN_VAL(!(header = azy_net_header_create(net)), EINA_FALSE);
        EINA_SAFETY_ON_NULL_RETURN_VAL(module->client->current->conn, EINA_FALSE);
        s = !!ecore_con_client_send(module->client->current->conn, eina_strbuf_string_get(header), eina_strbuf_length_get(header));
        eina_strbuf_free(header);
        if (!s)
          {
             ERR("Could not queue header for sending!");
             return EINA_FALSE;
          }
        net->headers_sent = EINA_TRUE;
     }

post_header:

   if ((!net) || (net->http.transfer_encoding != AZY_NET_TRANSFER_ENCODING_CHUNKED))
     {
        if (!data || !data->data) return EINA_TRUE;

        EINA_SAFETY_ON_NULL_RETURN_VAL(module->client->current->conn, EINA_FALSE);
        EINA_SAFETY_ON_TRUE_RETURN_VAL(!ecore_con_client_send(module->client->current->conn, data->data, data->size), EINA_FALSE);
        goto post_send;
     }

   if (!data || !data->data)
     {
        EINA_SAFETY_ON_NULL_RETURN_VAL(module->client->current->conn, EINA_FALSE);
        EINA_SAFETY_ON_TRUE_RETURN_VAL(!ecore_con_client_send(module->client->current->conn, "0\r\n\r\n", 5), EINA_FALSE);
        goto post_send;
     }

   net->refcount++;
   sprintf((char *)chunk_size, "%" PRIx64 "\r\n", data->size);
   chunk_data = eina_binbuf_new();
   eina_binbuf_append_length(chunk_data, (unsigned char *)chunk_size, strlen(chunk_size));
   eina_binbuf_append_length(chunk_data, data->data, data->size);
   eina_binbuf_append_length(chunk_data, (unsigned char *)"\r\n", 2);
   EINA_SAFETY_ON_NULL_RETURN_VAL(module->client->current->conn, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!ecore_con_client_send(module->client->current->conn,
                                  eina_binbuf_string_get(chunk_data), eina_binbuf_length_get(chunk_data)), EINA_FALSE);
   eina_binbuf_free(chunk_data);


post_send:
   if (nullify) module->client->current = NULL;
   return EINA_TRUE;
}

/**
 * @brief Upgrade a client's connection to SSL/TLS
 *
 * This function begins the SSL handshake process on connected client represented by @p module.
 * An AZY_EVENT_SERVER_CLIENT_UPGRADE event will be emitted on success, and EINA_FALSE will be
 * returned immediately on failure.
 * @param module The client object (NOT NULL)
 * @return #EINA_TRUE if successful, or #EINA_FALSE on failure
 */
Eina_Bool
azy_server_module_upgrade(Azy_Server_Module *module)
{
   DBG("(module=%p)", module);

   if (!AZY_MAGIC_CHECK(module, AZY_MAGIC_SERVER_MODULE))
     {
        AZY_MAGIC_FAIL(module, AZY_MAGIC_SERVER_MODULE);
        return EINA_FALSE;
     }
   if (module->client->dead) return EINA_FALSE;

   if (!ecore_con_ssl_client_upgrade(module->client->net->conn, ECORE_CON_USE_MIXED)) return EINA_FALSE;
   module->client->upgrading_module = module;
   return EINA_TRUE;
}

/**
 * @brief Return the state of an #Azy_Server_Module object
 * The return value of this function represents the connection state of the associated client.
 * @param module The module (NOT NULL)
 * @return EINA_TRUE if the client is connected, else EINA_FALSE
 */
Eina_Bool
azy_server_module_active_get(Azy_Server_Module *module)
{
   if (!AZY_MAGIC_CHECK(module, AZY_MAGIC_SERVER_MODULE))
     {
        AZY_MAGIC_FAIL(module, AZY_MAGIC_SERVER_MODULE);
        return EINA_FALSE;
     }

   return !module->client->dead;
}

/**
 * @brief Load a library and run a function from it which returns an #Azy_Server_Module_Def
 * This function loads @p file as an Eina_Module. If @p modname is specified, it attempts to call
 * modname() from the loaded module to create an #Azy_Server_Module_Def. If @p is NULL, the following
 * shell script formula will be used to generate a function name:
 * shell$ echo "$(basename $file | cut -d'.' -f1)_module_def"
 * @param file The file to load as a module (NOT NULL)
 * @param modname The name of the function to call of type #Azy_Server_Module_Def_Cb
 * @return On success, the loaded #Azy_Server_Module_Def, else NULL.
 */
Azy_Server_Module_Def *
azy_server_module_def_load(const char *file,
                           const char *modname)
{
   Eina_Module *m;
   Azy_Server_Module_Def *ret;
   const char *name;
   char buf[4096];
   Azy_Server_Module_Def_Cb cb;

   EINA_SAFETY_ON_NULL_RETURN_VAL(file, NULL);

   if (modname) name = modname;
   else /* attempt to autodetect module name */
     {
        const char *s, *d;

        s = strrchr(file, '/');
        EINA_SAFETY_ON_NULL_RETURN_VAL(s, NULL);

        d = strchr(++s, '.');
        EINA_SAFETY_ON_NULL_RETURN_VAL(d, NULL);
        EINA_SAFETY_ON_TRUE_RETURN_VAL(d - s + sizeof("_module_def") > sizeof(buf), NULL);
        snprintf(buf, d - s + sizeof("_module_def"), "%s_module_def", s);
        name = buf;
     }

   m = eina_module_new(file);
   EINA_SAFETY_ON_NULL_RETURN_VAL(m, NULL);

   EINA_SAFETY_ON_TRUE_GOTO(!eina_module_load(m), err);
   cb = (Azy_Server_Module_Def_Cb)eina_module_symbol_get(m, name);
   EINA_SAFETY_ON_TRUE_GOTO(!cb, err);
   ret = cb();
   EINA_SAFETY_ON_TRUE_GOTO(!ret, err);
   ret->module = m;
   return ret;

err:
   eina_module_free(m);
   return NULL;
}

/**
 * @brief Disconnect a client
 *
 * This function is used to terminate connection with the client.
 * @param module The client's #Azy_Server_Module object (NOT NULL)
 */
void
azy_server_module_close(Azy_Server_Module *module)
{
   EINA_SAFETY_ON_NULL_RETURN(module->client->net);
   EINA_SAFETY_ON_NULL_RETURN(module->client->net->conn);

   ecore_con_client_del(module->client->net->conn);
}
/** @} */
