/*
 * Copyright 2010 Mike Blumenkrantz <mike@zentific.com>
 */

#ifndef AZY_PRIV_H
#define AZY_PRIV_H

#include <Eina.h>
#include <Azy.h>

#define AZY_SERVER_TYPE 0x0f

#define AZY_MAGIC_SERVER 0x31337
#define AZY_MAGIC_SERVER_CLIENT 0x31338
#define AZY_MAGIC_SERVER_MODULE 0x31339
#define AZY_MAGIC_SERVER_MODULE_DEF 0x31340
#define AZY_MAGIC_SERVER_MODULE_METHOD 0x31341
#define AZY_MAGIC_CLIENT 0x31342
#define AZY_MAGIC_NET 0x31343
#define AZY_MAGIC_VALUE 0x31344
#define AZY_MAGIC_BLOB 0x31345
#define AZY_MAGIC_CONTENT 0x31346
#define AZY_MAGIC_CLIENT_DATA_HANDLER 0x31347

#define AZY_MAGIC_NONE 0x1234fedc
#define AZY_MAGIC                 Azy_Magic  __magic
#define AZY_MAGIC_SET(d, m)       (d)->__magic = (m)
#define AZY_MAGIC_CHECK(d, m)     ((d) && ((d)->__magic == (m)))
#define AZY_MAGIC_FAIL(d, m)  _azy_magic_fail((d), (d) ? (d)->__magic : 0, (m), __PRETTY_FUNCTION__)

#ifndef __GNUC__
# define __PRETTY_FUNCTION__ __FILE__
#endif


#define DBG(...)  EINA_LOG_DOM_DBG(azy_log_dom, __VA_ARGS__)
#define INFO(...) EINA_LOG_DOM_INFO(azy_log_dom, __VA_ARGS__)
#define WARN(...) EINA_LOG_DOM_WARN(azy_log_dom, __VA_ARGS__)
#define ERR(...)  EINA_LOG_DOM_ERR(azy_log_dom, __VA_ARGS__)
#define CRI(...)  EINA_LOG_DOM_CRIT(azy_log_dom, __VA_ARGS__)

extern Eina_Error AZY_ERROR_REQUEST_JSON_OBJECT;
extern Eina_Error AZY_ERROR_REQUEST_JSON_METHOD;
extern Eina_Error AZY_ERROR_REQUEST_JSON_PARAM;

extern Eina_Error AZY_ERROR_RESPONSE_JSON_OBJECT;
extern Eina_Error AZY_ERROR_RESPONSE_JSON_ERROR;
extern Eina_Error AZY_ERROR_RESPONSE_JSON_NULL;
extern Eina_Error AZY_ERROR_RESPONSE_JSON_INVALID;

extern Eina_Error AZY_ERROR_REQUEST_XML_DOC;
extern Eina_Error AZY_ERROR_REQUEST_XML_ROOT;
extern Eina_Error AZY_ERROR_REQUEST_XML_METHODNAME;
extern Eina_Error AZY_ERROR_REQUEST_XML_PARAM;

extern Eina_Error AZY_ERROR_RESPONSE_XML_DOC;
extern Eina_Error AZY_ERROR_RESPONSE_XML_ROOT;
extern Eina_Error AZY_ERROR_RESPONSE_XML_RETVAL;
extern Eina_Error AZY_ERROR_RESPONSE_XML_MULTI;
extern Eina_Error AZY_ERROR_RESPONSE_XML_FAULT;
extern Eina_Error AZY_ERROR_RESPONSE_XML_INVAL;
extern Eina_Error AZY_ERROR_RESPONSE_XML_UNSERIAL;

typedef struct _Azy_Client_Handler_Data Azy_Client_Handler_Data;
typedef unsigned int Azy_Magic;


struct _Azy_Content
{
   AZY_MAGIC;
   void               *data;
   const char         *method;
   Eina_List          *params;
   Azy_Value          *retval;
   void               *ret;
   Azy_Client_Call_Id  id;
   Azy_Net            *recv_net;

   unsigned char      *buffer;
   long long int       length;

   Eina_Bool           error_set;
   Eina_Error          errcode; //internal code
   int                 faultcode; //code to actually report
   const char         *faultmsg; //if non-null, message to reply with instead of message associated with errcode
};

struct _Azy_Net
{
   AZY_MAGIC;
   void              *conn;
   Eina_Bool          server_client : 1;

   long long int      size;
   unsigned char     *buffer;
   unsigned char      *overflow;
   long long int       overflow_length;

   Ecore_Timer       *timer;
   Eina_Bool          nodata : 1;

   Azy_Net_Type      type;
   Azy_Net_Transport transport;
   struct
   {
      struct
      {
         const char *http_path;
      } req;

      struct
      {
         const char *http_msg;
         int         http_code;
      } res;
      int        version;
      Eina_Hash *headers;
      long long int        content_length;
   } http;
   Eina_Bool headers_read : 1;
};

struct _Azy_Server
{
   AZY_MAGIC;
   Ecore_Con_Server    *server;
   Ecore_Event_Handler *add;

   struct
   {
      Eina_Bool secure : 1;
      Eina_Bool cert : 1;
   } security;

   Eina_List *module_defs;
};

typedef struct _Azy_Server_Client
{
   AZY_MAGIC;
   Ecore_Event_Handler *del;
   Ecore_Event_Handler *data;

   Azy_Net    *net;
   Azy_Server *server;
   Eina_List   *modules;

   const char          *session_id;
   const char          *ip;
} Azy_Server_Client;

struct _Azy_Server_Module
{
   AZY_MAGIC;
   void                   *data;
   Azy_Server_Module_Def *def;
   Azy_Content           *content;
   Azy_Server_Client     *client;
   double                  last_used;
};

struct _Azy_Value
{
   AZY_MAGIC;
   Azy_Value_Type     type;
   int                 ref;

   const char         *str_val;
   int                 int_val;
   double              dbl_val;
   Azy_Blob          *blob_val;

   Eina_List          *children;

   const char         *member_name;
   Azy_Value         *member_value;
};

struct _Azy_Client
{
   AZY_MAGIC;
   void        *data;
   Azy_Net    *net;

   Ecore_Event_Handler *add;
   Ecore_Event_Handler *del;
   Ecore_Event_Handler *recv;

   Eina_List           *conns;
   Eina_Hash           *callbacks;
   Eina_Hash           *free_callbacks;

   const char          *host;
   int                  port;
   const char          *session_id;
   int                  secure;

   Eina_Bool            connected : 1;
};

struct _Azy_Client_Handler_Data
{
   AZY_MAGIC;
   Azy_Client_Call_Id  id;
   Azy_Client         *client;
   Azy_Net            *recv;
   const char         *method;
   Azy_Content_Cb      callback; //callback set to convert from Azy_Value to Return_Type
   void               *content_data;
};


struct _Azy_Blob
{
   AZY_MAGIC;
   const char *buf;
   int         len;
   char        refs;
};

struct _Azy_Server_Module_Def
{
   AZY_MAGIC;
   const char *name;
   int         data_size;
   Azy_Server_Module_Cb init;
   Azy_Server_Module_Shutdown_Cb shutdown;
   Azy_Server_Module_Content_Cb pre;
   Azy_Server_Module_Content_Cb post;
   Azy_Server_Module_Content_Cb fallback;
   Azy_Server_Module_Cb download;
   Azy_Server_Module_Cb upload;
   Eina_List *methods;
};

struct _Azy_Server_Module_Method
{
   AZY_MAGIC;
   const char *name;
   Azy_Server_Module_Content_Cb method;
};

extern void _azy_magic_fail(const void *d, Azy_Magic m, Azy_Magic req_m, const char *fname);

Eina_Bool _azy_value_multi_line_get(Azy_Value *v,
                                      int                 max_strlen);
int azy_events_type_parse(Azy_Net            *net,
                           int                  type,
                           const unsigned char *header,
                           int                  len);
Eina_Bool azy_events_header_parse(Azy_Net      *net,
                                   unsigned char *event_data,
                                   size_t         event_len,
                                   int            offset);
Azy_Net_Transport azy_events_net_transport_get(const char *content_type);
Eina_Bool          azy_events_connection_kill(void             *conn,
                                               Eina_Bool         server_client,
                                               const char       *msg);

Eina_Bool _azy_client_handler_add(Azy_Client            *client,
                                   int type                        __UNUSED__,
                                   Ecore_Con_Event_Server_Add *add __UNUSED__);
Eina_Bool _azy_client_handler_del(Azy_Client            *client,
                                   int type                        __UNUSED__,
                                   Ecore_Con_Event_Server_Del *del __UNUSED__);
Eina_Bool _azy_client_handler_data(Azy_Client_Handler_Data *handler_data,
                                    int                               type,
                                    Ecore_Con_Event_Server_Data      *ev);

Eina_Bool azy_server_client_handler_add(Azy_Server                *server,
                                         int                         type,
                                         Ecore_Con_Event_Client_Add *ev);
void      _azy_event_handler_fake_free(void *data __UNUSED__, void *data2 __UNUSED__);
#endif
