/*
 * Copyright 2010, 2011, 2012, 2013 Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
 */

#ifndef AZY_PRIV_H
#define AZY_PRIV_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <inttypes.h>
#include <time.h>
#include <Ecore.h>
#include <Ecore_Con.h>
#include <Azy.h>
#include "match.h"

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#elif defined __GNUC__
#define alloca __builtin_alloca
#elif defined _AIX
#define alloca __alloca
#else
#include <stddef.h>
void *alloca(size_t);
#endif

#define AZY_SERVER_TYPE                0x0f

#define AZY_MAGIC_SERVER               0x31337
#define AZY_MAGIC_SERVER_CLIENT        0x31338
#define AZY_MAGIC_SERVER_MODULE        0x31339
#define AZY_MAGIC_SERVER_MODULE_DEF    0x31340
#define AZY_MAGIC_SERVER_MODULE_METHOD 0x31341
#define AZY_MAGIC_CLIENT               0x31342
#define AZY_MAGIC_NET                  0x31343
#define AZY_MAGIC_VALUE                0x31344
#define AZY_MAGIC_NET_COOKIE           0x31345
#define AZY_MAGIC_CONTENT              0x31346
#define AZY_MAGIC_CLIENT_DATA_HANDLER  0x31347
#define AZY_MAGIC_RSS                  0x66442
#define AZY_MAGIC_RSS_ITEM             0x66443

#define AZY_MAGIC_NONE                 0x1234fedc
#define AZY_MAGIC                      Azy_Magic __magic
#define AZY_MAGIC_SET(d, m)   (d)->__magic = (m)
#define AZY_MAGIC_CHECK(d, m) ((d) && ((d)->__magic == (m)))
#define AZY_MAGIC_FAIL(d, m)  _azy_magic_fail((d), (d) ? (d)->__magic : 0, (m), __PRETTY_FUNCTION__)

#ifndef __GNUC__
# define __PRETTY_FUNCTION__ __FILE__
#endif
#ifndef MIN
# define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

extern int azy_log_dom;
extern int azy_rpc_log_dom;

#define DBG(...)            EINA_LOG_DOM_DBG(azy_log_dom, __VA_ARGS__)
#define INFO(...)           EINA_LOG_DOM_INFO(azy_log_dom, __VA_ARGS__)
#define WARN(...)           EINA_LOG_DOM_WARN(azy_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(azy_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(azy_log_dom, __VA_ARGS__)

#define RPC_DBG(...)        if (azy_rpc_log_dom != -1) EINA_LOG_DOM_DBG(azy_rpc_log_dom, __VA_ARGS__)
#define RPC_INFO(...)       if (azy_rpc_log_dom != -1) EINA_LOG_DOM_INFO(azy_rpc_log_dom, __VA_ARGS__)
#define RPC_WARN(...)       if (azy_rpc_log_dom != -1) EINA_LOG_DOM_WARN(azy_rpc_log_dom, __VA_ARGS__)
#define RPC_ERR(...)        if (azy_rpc_log_dom != -1) EINA_LOG_DOM_ERR(azy_rpc_log_dom, __VA_ARGS__)
#define RPC_CRI(...)        if (azy_rpc_log_dom != -1) EINA_LOG_DOM_CRIT(azy_rpc_log_dom, __VA_ARGS__)

#ifndef strdupa
# define strdupa(str)       strcpy(alloca(strlen(str) + 1), str)
#endif

#ifndef strndupa
# define strndupa(str, len) strncpy(alloca(len + 1), str, len)
#endif

#define EBUF(X)             ((X) ? eina_binbuf_string_get(X) : NULL)
#define EBUFLEN(X)          ((X) ? eina_binbuf_length_get(X) : 0)

#define ESBUF(X)            ((X) ? eina_strbuf_string_get(X) : NULL)
#define ESBUFLEN(X)         ((X) ? eina_strbuf_length_get(X) : 0)

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
extern Eina_Error AZY_ERROR_XML_UNSERIAL;

typedef struct Azy_Client_Handler_Data Azy_Client_Handler_Data;
typedef unsigned int                   Azy_Magic;

struct Azy_Net_Cookie
{
   AZY_MAGIC;
   unsigned int         refcount;
   Eina_Stringshare    *domain;
   Eina_Stringshare    *path;
   Eina_Stringshare    *name;
   Eina_Stringshare    *value;
   time_t               created;
   time_t               last_used;
   time_t               expires;
   Azy_Net_Cookie_Flags flags;
   Eina_Bool            max_age : 1; // if expires is actually max-age
   Eina_Bool            in_hash : 1;
};

struct Azy_Content
{
   AZY_MAGIC;
   void                 *data;
   const char           *method;
   Eina_List            *params;
   Eina_Value           *retval;
   void                 *ret;
   Azy_Client_Call_Id    id;
   Azy_Net              *recv_net;
   Azy_Content_Retval_Cb retval_cb;

   Eina_Binbuf          *buffer;

   Eina_Bool             error_set : 1;
   Eina_Bool             ret_is_rss : 1;
   Eina_Error            errcode; //internal code
   int                   faultcode; //code to actually report
   const char           *faultmsg; //if non-null, message to reply with instead of message associated with errcode
};

typedef struct Azy_Rss_Type_Rss
{
   /* rss format only */
   Eina_Stringshare *link;
   Eina_Stringshare *desc;
   time_t lastbuilddate;
   unsigned int skipdays; // bitshift per day
   unsigned long long skiphours; // bitshift per hour
   unsigned int ttl; // in minutes
   struct
   {
      Eina_Stringshare *url;
      Eina_Stringshare *title;
      Eina_Stringshare *link;
      Eina_Stringshare *desc;
      int w, h;
   } image;
} Azy_Rss_Type_Rss;

typedef struct Azy_Rss_Type_Atom
{
   /* atom format only */
   Eina_Stringshare *id;
   Eina_Stringshare *subtitle;
   Eina_Stringshare *rights;
   Eina_Stringshare *logo;
   time_t      updated;
   Eina_List  *contributors; //Azy_Rss_Contact
   Eina_List  *authors; //Azy_Rss_Contact
   Eina_List  *atom_links; //Azy_Rss_Link
} Azy_Rss_Type_Atom;

struct Azy_Rss
{
   AZY_MAGIC;
   Eina_Bool   atom; /* true if item is Azy_Rss_Atom */
   unsigned int refcount;
   Eina_Stringshare *title;
   Eina_Stringshare *img_url;
   Eina_Stringshare *generator;
   Eina_List  *categories; //Azy_Rss_Category
   Eina_List  *items; //Azy_Rss_Item

   union
   {
      Azy_Rss_Type_Rss rss;
      Azy_Rss_Type_Atom atom;
   } data;
};

typedef struct Azy_Rss_Item_Type_Rss
{
   /* rss format only */
   Eina_Stringshare *link;
   Eina_Stringshare *desc;
   Eina_Stringshare *guid;
   Eina_Stringshare *comment_url;
   Eina_Stringshare *author;
   Eina_Stringshare *content;
   Eina_Stringshare *content_encoded;
   struct
   {
      Eina_Stringshare *url;
      size_t length;
      Eina_Stringshare *type;
   } enclosure;
   Eina_Bool permalink; //guid is permalink (guid is a link)
} Azy_Rss_Item_Type_Rss;

typedef struct Azy_Rss_Item_Type_Atom
{
   /* atom format only */
   Eina_Stringshare *rights;
   Eina_Stringshare *summary;
   Eina_Stringshare *id;
   Eina_Stringshare *icon;
   time_t   updated;
   Eina_List  *contributors; //Azy_Rss_Contact
   Eina_List  *authors; //Azy_Rss_Contact
   Eina_List  *atom_links; //Azy_Rss_Link
} Azy_Rss_Item_Type_Atom;

struct Azy_Rss_Item
{
   AZY_MAGIC;
   Eina_Bool   atom; /* true if item is Azy_Rss_Atom */
   Eina_Bool read; /* whether item is marked as read */
   Eina_Stringshare *title;
   Eina_List  *categories; //Azy_Rss_Category
   time_t published;

   union
   {
      Azy_Rss_Item_Type_Rss rss;
      Azy_Rss_Item_Type_Atom atom;
   } data;
};

struct Azy_Net
{
   AZY_MAGIC;
   unsigned int      refcount;
   void             *conn;
   Eina_Bool         server_client : 1;

   Eina_Binbuf      *buffer;
   size_t            progress;  //for tracking current call progress in RAW mode
   Eina_Binbuf      *overflow;
   Eina_Strbuf      *separator; // \r\n, \n, \n\r, etc

   Ecore_Timer      *timer;

   Azy_Net_Type      type;
   Azy_Net_Transport transport;
   Azy_Net_Protocol  proto;

   struct
   {
      struct
      {
         Eina_Stringshare *http_path;
         Eina_Stringshare *host;
      } req;

      struct
      {
         Eina_Stringshare *http_msg;
         int         http_code;
      } res;
      Eina_List                *set_cookies; /* Azy_Net_Cookie; Set-Cookie: cookies*/
      Eina_List                *send_cookies; /* Azy_Net_Cookie; Cookie: cookies */
      Eina_Hash                *headers;
      Eina_Hash                *post_headers;
      Eina_Binbuf              *post_headers_buf; // headers after last chunk
      int64_t                   content_length;
      Azy_Net_Transfer_Encoding transfer_encoding;
      size_t                    chunk_size;
   } http;
   Eina_Bool         nodata : 1;
   Eina_Bool         buffer_stolen : 1;
   Eina_Bool headers_read : 1;
   Eina_Bool need_chunk_size : 1; // waiting for size of next chunk for transfer encoding
};

struct Azy_Server
{
   AZY_MAGIC;
   unsigned int         refcount;
   Ecore_Con_Server    *server;
   Ecore_Event_Handler *add;
   const char          *addr;
   int                  port;
   unsigned long int    clients;

   struct
   {
      Eina_Bool  secure : 1;
      Eina_List *cert_files;
   } security;

   Eina_Hash           *module_defs;
};

typedef struct Azy_Server_Client
{
   AZY_MAGIC;
   Ecore_Event_Handler *del;
   Ecore_Event_Handler *data;
   Ecore_Event_Handler *upgrade;

   Azy_Net             *net;
   Azy_Net             *current;
   Azy_Server          *server;
   Eina_List           *modules;
   Azy_Server_Module   *upgrading_module;
   Eina_Binbuf         *overflow;

   Eina_Bool            handled : 1;
   Eina_Bool            dead : 1;
   Eina_Bool            resuming : 1;
   Eina_Bool            executing : 1;

   Eina_Bool            suspend : 1;
   Eina_List           *suspended_nets; /* Azy_Net* */
   Azy_Net             *resume;
   Azy_Content         *resume_rpc;
   Eina_Bool            resume_ret : 1;

   const char          *ip;
} Azy_Server_Client;

typedef enum
{
   AZY_SERVER_MODULE_STATE_INIT,
   AZY_SERVER_MODULE_STATE_PRE,
   AZY_SERVER_MODULE_STATE_METHOD,
   AZY_SERVER_MODULE_STATE_POST,
   AZY_SERVER_MODULE_STATE_ERR
} Azy_Server_Module_State;

struct Azy_Server_Module
{
   AZY_MAGIC;
   void                   *data;
   Azy_Server_Module_Def  *def;
   Azy_Content            *content;
   Azy_Server_Client      *client;
   Azy_Net_Data            recv;
   Azy_Net                *new_net;
   Eina_Hash              *params;
   Eina_Bool               suspend : 1;
   Eina_Bool               run_method : 1;
   Eina_Bool               post : 1;
   Eina_Bool               rewind : 1;
   Eina_Bool               rewind_now : 1;
   Eina_Bool               executing : 1;
   Azy_Server_Module_State state;
};

struct Azy_Client
{
   AZY_MAGIC;
   unsigned int refcount;
   void                *data;
   Azy_Net             *net;
   Eina_Binbuf         *overflow;

   Ecore_Event_Handler *add;
   Ecore_Event_Handler *del;
   Ecore_Event_Handler *recv;
   Ecore_Event_Handler *upgrade;

   Eina_List           *conns;
   Eina_Hash           *callbacks;
   Eina_Hash           *free_callbacks;

   const char          *addr;
   int                  port;
   Eina_Bool            secure : 1;
   Eina_Bool            connected : 1;
   Eina_Bool            upgraded : 1;
};

struct Azy_Client_Handler_Data
{
   AZY_MAGIC;
   Azy_Client_Call_Id id;
   Azy_Net_Type       type;
   Azy_Client        *client;
   Azy_Net           *recv;
   Eina_Stringshare  *method;
   Eina_Stringshare  *uri;
   Azy_Content_Cb     callback;  //callback set to convert from Eina_Value to Return_Type
   void              *content_data;
   Eina_Strbuf       *send;
   Eina_Bool          redirect : 1;
};

struct Azy_Server_Module_Def
{
   AZY_MAGIC;
   const char                   *name;
   int                           data_size;
   double                        version;
   Azy_Server_Module_Cb          init;
   Azy_Server_Module_Shutdown_Cb shutdown;
   Azy_Server_Module_Pre_Cb      pre;
   Azy_Server_Module_Content_Cb  post;
   Azy_Server_Module_Content_Cb  fallback;
   Azy_Server_Module_Cb          download;
   Azy_Server_Module_Cb          upload;
   Eina_Hash                    *methods;
   Eina_Module                  *module;
};

struct Azy_Server_Module_Method
{
   AZY_MAGIC;
   const char                  *name;
   Azy_Server_Module_Content_Cb method;
};


typedef struct _Azy_Value_Struct_Desc
{
   Eina_Value_Struct_Desc base;
   int refcount;
} Azy_Value_Struct_Desc;
#ifdef __cplusplus
extern "C" {
#endif

void             _azy_magic_fail(const void *d, Azy_Magic m, Azy_Magic req_m, const char *fname);

Eina_Bool        azy_rss_init(const char *);
Eina_Bool        azy_rss_item_init(const char *);
void             azy_rss_shutdown(void);
void             azy_rss_item_shutdown(void);

Eina_Bool        azy_value_multi_line_get_(const Eina_Value *v, unsigned int max_strlen);
int              azy_events_type_parse(Azy_Net *net, int type, const unsigned char *header, int64_t len);
Eina_Bool        azy_events_header_parse(Azy_Net *net, unsigned char *event_data, size_t event_len, int offset);
Eina_Bool        azy_events_connection_kill(void *conn, Eina_Bool server_client, const char *msg);

void             azy_events_recv_progress(Azy_Net *net, const void *data, size_t len);
inline void      azy_events_client_transfer_progress_event(const Azy_Client_Handler_Data *hd, size_t size);
void azy_events_client_transfer_complete_cleanup(Azy_Client *client, Azy_Content *content);
void azy_events_client_transfer_complete_event_free(Azy_Client *client, Azy_Event_Client_Transfer_Complete *cse);
inline void      azy_events_client_transfer_complete_event(const Azy_Client_Handler_Data *hd, Azy_Content *content);
inline Eina_Bool azy_events_length_overflows(int64_t current, int64_t max);
size_t           azy_events_transfer_decode(Azy_Net *net, unsigned char *start, int len);
inline Eina_Bool azy_events_chunks_done(const Azy_Net *net);
Eina_Binbuf     *azy_events_overflow_add(Azy_Net *net, const unsigned char *data, size_t len);

int              azy_net_cookie_init_(void);
void             azy_net_cookie_shutdown_(void);

Eina_Bool        _azy_client_handler_add(Azy_Client *client, int type, Ecore_Con_Event_Server_Add *add);
Eina_Bool        _azy_client_handler_del(Azy_Client *client, int type, Ecore_Con_Event_Server_Del *del);
Eina_Bool        _azy_client_handler_data(Azy_Client_Handler_Data *handler_data, int type, Ecore_Con_Event_Server_Data *ev);
Eina_Bool        _azy_client_handler_upgrade(Azy_Client_Handler_Data *hd, int type, Ecore_Con_Event_Server_Upgrade *ev);

Eina_Bool        azy_server_client_handler_add(Azy_Server *server, int type, Ecore_Con_Event_Client_Add *ev);
void _azy_event_handler_fake_free(Eina_Free_Cb cb, void *data);

Eina_Bool
                 azy_content_deserialize_json(Azy_Content *content, const char *buf, ssize_t len);

Azy_Rss         *azy_rss_new(void);
Azy_Rss_Item    *azy_rss_item_new(void);
Azy_Rss_Category *azy_rss_category_new(void);
void azy_rss_category_free(Azy_Rss_Category *cat);
const char *_azy_rss_eet_union_type_get(const void *data, Eina_Bool *unknow);
Eina_Bool _azy_rss_eet_union_type_set(const char *type, void *data, Eina_Bool unknow);

Eina_Bool        azy_content_serialize_request_xml(Azy_Content *content);
Eina_Bool        azy_content_serialize_response_xml(Azy_Content *content);
Eina_Bool        azy_content_deserialize_xml(Azy_Content *content, char *buf, ssize_t len);
Eina_Bool        azy_content_deserialize_request_xml(Azy_Content *content, char *buf, ssize_t len);
Eina_Bool        azy_content_deserialize_response_xml(Azy_Content *content, char *buf, ssize_t len);
Eina_Bool        azy_content_serialize_request_json(Azy_Content *content);
Eina_Bool        azy_content_serialize_response_json(Azy_Content *content);
Eina_Bool        azy_content_deserialize_request_json(Azy_Content *content, const char *buf, ssize_t len);
Eina_Bool        azy_content_deserialize_response_json(Azy_Content *content, const char *buf, ssize_t len);

Eina_Bool        azy_content_buffer_set_(Azy_Content *content, unsigned char *buffer, size_t length);
#ifdef __cplusplus
}
#endif
#endif
