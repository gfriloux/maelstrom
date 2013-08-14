#ifndef EMAIL_PRIVATE_H
#define EMAIL_PRIVATE_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#elif defined __GNUC__
#define alloca __builtin_alloca
#elif defined _AIX
#define alloca __alloca
#else
#include <stddef.h>
void *alloca (size_t);
#endif

#include <Ecore.h>
#include <Ecore_Con.h>
#include "Email.h"
#include <inttypes.h>
#include <ctype.h>
#include "match.h"
#include "md5.h"

#define DBG(...)            EINA_LOG_DOM_DBG(email_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(email_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(email_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(email_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(email_log_dom, __VA_ARGS__)

extern int email_log_dom;

#define EMAIL_POP3_PORT 110
#define EMAIL_POP3S_PORT 995

#define EMAIL_SMTP_PORT 25
#define EMAIL_SMTPS_PORT 465

#define EMAIL_IMAP4_PORT 143
#define EMAIL_IMAPS_PORT 993
#define EMAIL_IMAP4_SSL_PORT 585 //< wtf is this used for???

#define EMAIL_POP3_LIST "LIST\r\n"
#define EMAIL_POP3_STAT "STAT\r\n"
#define EMAIL_POP3_RSET "RSET\r\n"
#define EMAIL_POP3_DELE "DELE %"PRIu32"\r\n"
#define EMAIL_POP3_RETR "RETR %"PRIu32"\r\n"

#define EMAIL_POP3_QUIT "QUIT\r\n"

#define EMAIL_IMAP4_LOGOUT "LOGOUT\r\n"
#define EMAIL_IMAP4_NOOP "NOOP\r\n"
#define EMAIL_IMAP4_CLOSE "CLOSE\r\n"
#define EMAIL_IMAP4_EXPUNGE "EXPUNGE\r\n"
#define EMAIL_IMAP4_NAMESPACE "NAMESPACE\r\n"

#define EMAIL_SMTP_FROM "MAIL FROM: <%s>\r\n"
#define EMAIL_SMTP_TO "RCPT TO: <%s>\r\n"
#define EMAIL_SMTP_DATA "DATA\r\n"

#define EMAIL_STARTTLS "STARTTLS\r\n"
#define EMAIL_AUTHLOGIN "AUTH LOGIN\r\n"
#define CRLF "\r\n"
#define CRLFLEN 2

#define EBUF(X)             ((X) ? eina_binbuf_string_get(X) : NULL)
#define EBUFLEN(X)          ((X) ? eina_binbuf_length_get(X) : 0)

#define ESBUF(X)            ((X) ? eina_strbuf_string_get(X) : NULL)
#define ESBUFLEN(X)         ((X) ? eina_strbuf_length_get(X) : 0)

#define EMAIL_RETURN_ERROR -1 /*< protocol error */
#define EMAIL_RETURN_EAGAIN 0 /*< current parser needs more data */
#define EMAIL_RETURN_DONE 1 /*< parser successfully parsed all data */

typedef enum
{
   EMAIL_STATE_INIT,
   EMAIL_STATE_SSL,
   EMAIL_STATE_USER,
   EMAIL_STATE_PASS,
   EMAIL_STATE_CONNECTED
} Email_State;

typedef enum
{
   EMAIL_SMTP_STATE_NONE,
   EMAIL_SMTP_STATE_FROM,
   EMAIL_SMTP_STATE_TO,
   EMAIL_SMTP_STATE_DATA,
   EMAIL_SMTP_STATE_BODY,
} Email_Smtp_State;

typedef enum
{
   EMAIL_POP_OP_STAT = 1,
   EMAIL_POP_OP_LIST,
   EMAIL_POP_OP_RSET,
   EMAIL_POP_OP_DELE,
   EMAIL_POP_OP_RETR,
   EMAIL_POP_OP_QUIT,
} Email_Pop_Op;

typedef enum
{
   EMAIL_SMTP_OP_SEND = 1,
} Email_Smtp_Op;

typedef enum
{
   EMAIL_IMAP4_OP_CAPABILITY = 1,
   EMAIL_IMAP4_OP_NAMESPACE,
   EMAIL_IMAP4_OP_LOGIN,
   EMAIL_IMAP4_OP_LIST,
   EMAIL_IMAP4_OP_SELECT,
   EMAIL_IMAP4_OP_EXAMINE,
   EMAIL_IMAP4_OP_NOOP,
   EMAIL_IMAP4_OP_CLOSE,
   EMAIL_IMAP4_OP_EXPUNGE,
   EMAIL_IMAP4_OP_CREATE,
   EMAIL_IMAP4_OP_DELETE,
   EMAIL_IMAP4_OP_RENAME,
   EMAIL_IMAP4_OP_SUBSCRIBE,
   EMAIL_IMAP4_OP_UNSUBSCRIBE,
   EMAIL_IMAP4_OP_LSUB,
   EMAIL_IMAP4_OP_APPEND,
   EMAIL_IMAP4_OP_FETCH,
   EMAIL_IMAP4_OP_LOGOUT,
} Email_Imap4_Op;

/* for simple flag parsing */
typedef void (*Email_Flag_Set_Cb)(Email *, unsigned int);

#if (EINA_VERSION_MAJOR == 1) && (EINA_VERSION_MINOR < 8)
# define eina_list_last_data_get(X) eina_list_data_get(eina_list_last(X))
#endif

struct Email_Message
{
   Email *owner;
   unsigned int sending; //wait until after send to delete
   void *data;

   Eina_List *to;
   Eina_List *cc;
   Eina_List *bcc;
   Eina_List *sender; /* Sender: */
   Eina_List *reply_to; /* Sender: */
   Eina_List *from; /* Email_Contact * */ /* From: X,Y,Z */
   Eina_Stringshare *subject;
   Eina_Stringshare *in_reply_to;
   Eina_Stringshare *msgid;

   Email_Message_Part *content;
   Eina_List *parts;
   unsigned int attachments;


   double mimeversion;
   unsigned long date;

   Eina_Hash *headers;
   Eina_Bool deleted : 1; //user deleted message
};

struct Email_Message_Part
{
   unsigned int refcount;
   unsigned int num;
   Eina_Stringshare *name;
   Eina_Stringshare *content_type;
   Eina_Stringshare *charset;
   Email_Message_Part_Encoding encoding;

   Eina_List *parts;

   Eina_Binbuf *content;
   size_t size; //possible to have size without content
   unsigned int lines; //number of lines in text/plain parts
   unsigned int attachments;
};

struct Email_Contact
{
   unsigned int refcount;
   Eina_Stringshare *address;
   Eina_Stringshare *routing_address; //wtf is this???
   Eina_Stringshare *name;
};

struct Email_Operation
{
   Email *e;
   unsigned int opnum;
   unsigned int optype;
   void *opdata;
   void *userdata;
   void *cb;
   char *mbox; //target mbox for imap APPENDs
   unsigned int flags;
   Eina_Bool deleted : 1;
   Eina_Bool sent : 1;
   Eina_Bool allows_cont : 1; //imap operation that allows/requires continuation
   Eina_Bool blocking : 1;
};

struct Email
{
   Ecore_Con_Server *svr;
   const char *addr;
   int flags;

   Email_State state;
   Eina_Binbuf *buf;

   const char *username;
   char *password;
   void *data;
   Eina_List *certs;

   unsigned int current;
   Eina_List *ops;
   void *ev;

   Ecore_Event_Handler *h_data, *h_del, *h_error, *h_upgrade;

   unsigned int opcount; //current max operation number

   union
   {
      struct
      {
         Email_Smtp_State state;
         unsigned int internal_state;
      } smtp;
      struct
      {
         unsigned int current; //current operation number
         int state; //for various parsers to save their states; < 0 when we're parsing a resp line
         Email_Operation_Status status; //status of current op
         Email_Imap4_Mailbox_Info *mbox; //info can be updated with untagged data at any time
         Eina_Stringshare *mboxname; //currently SELECT/EXAMINE mailbox name
         Eina_List *blockers;
         Eina_Bool caps : 1; //whether capabilities have been parsed yet
         Eina_Bool resp : 1; //whether respcode for current line has been parsed yet
      } imap;
   } protocol;

   union
   {
      struct
      {
         Eina_Binbuf *apop_str;
         Eina_Bool apop : 1;
      } pop;
      struct
      {
         const char *domain;
         Eina_Bool ssl : 1;
         Eina_Bool pipelining : 1;
         Eina_Bool eightbit : 1;
         size_t size;

         Eina_Bool cram : 1;
         Eina_Bool login : 1;
         Eina_Bool plain : 1;
      } smtp;
      struct
      {
         Eina_Bool AUTH_CRAM_MD5 : 1;
         Eina_Bool AUTH_GSSAPI : 1;
         Eina_Bool AUTH_NTLM : 1;
         Eina_Bool AUTH_PLAIN : 1;

         Eina_Bool ACL : 1;
         Eina_Bool CHILDREN : 1;
         Eina_Bool IDLE : 1;
         Eina_Bool IMAP4 : 1;
         Eina_Bool IMAP4rev1 : 1;
         Eina_Bool LITERALPLUS : 1;
         Eina_Bool LOGINDISABLED : 1;
         Eina_Bool NAMESPACE : 1;
         Eina_Bool QUOTA : 1;
         Eina_Bool SORT : 1;
         Eina_Bool UIDPLUS : 1;

         Eina_Inarray *namespaces[EMAIL_IMAP4_NAMESPACE_LAST];
      } imap;
   } features;
   Email_Type type;
   Eina_Bool upgrade : 1;
   Eina_Bool secure : 1;
   Eina_Bool deleted : 1;
   Eina_Bool need_crlf : 1; //protocol is searching for next CRLF
};

#define DIFF(EXP) (unsigned int)(EXP)

static inline Eina_Bool
email_op_pop_ok(const unsigned char *data, int size)
{
   return !((size < 3) || (data[0] != '+') || strncasecmp((char*)data + 1, "OK", 2));
}

static inline void
email_write(Email *e, const void *data, size_t size)
{
   if (eina_log_domain_level_check(email_log_dom, EINA_LOG_LEVEL_DBG))
     DBG("Sending:\n%s", (char*)data);
   if (!size) size = strlen((char*)data);
   ecore_con_server_send(e->svr, data, size);
}

static inline Eina_Bool
email_is_pop(const Email *e)
{
   return e->type == EMAIL_TYPE_POP3;
}

static inline Eina_Bool
email_is_smtp(const Email *e)
{
   return e->type == EMAIL_TYPE_SMTP;
}

static inline Eina_Bool
email_is_imap(const Email *e)
{
   return e->type == EMAIL_TYPE_IMAP4;
}

/* use this for offset updates to ensure the current offset doesn't get overwritten */
static inline void
imap_offset_update(size_t *offset, size_t plus)
{
   *offset += plus;
}

static inline void
email_imap_write(Email *e, Email_Operation *op, const char *data, size_t size)
{
   char buf[64];

   if (!op)
     {
        snprintf(buf, sizeof(buf), "%u ", e->opcount++);
        email_write(e, buf, strlen(buf));
        email_write(e, data, size ?: strlen(data));
        return;
     }
   snprintf(buf, sizeof(buf), "%u ", op->opnum);
   email_write(e, buf, strlen(buf));
   if (data)
     email_write(e, data, size ?: strlen(data));
   else if (op->opdata)
     email_write(e, op->opdata, strlen(op->opdata));
   else
     CRI("WTF");
   op->sent = 1;
}

static inline Eina_Bool
email_is_blocked(const Email *e)
{
   if (email_is_pop(e) || email_is_smtp(e)) return eina_list_count(e->ops) > 1;
   if (e->protocol.imap.blockers)
     {
        Email_Operation *op = eina_list_data_get(e->ops);
        /* if there's a blocker queue, we are not blocked if the first blocker has not been sent */
        if (eina_list_data_get(e->protocol.imap.blockers) == op)
          return op->sent;
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

void auth_cram_md5(Email *e, const unsigned char *data, size_t size);

Eina_Bool upgrade_pop(Email *e, int type, Ecore_Con_Event_Server_Upgrade *ev);
Eina_Bool data_pop(Email *e, int type, Ecore_Con_Event_Server_Data *ev);
Eina_Bool error_pop(Email *e, int type, Ecore_Con_Event_Server_Error *ev );

Eina_Bool upgrade_imap(Email *e, int type, Ecore_Con_Event_Server_Upgrade *ev);
Eina_Bool data_imap(Email *e, int type, Ecore_Con_Event_Server_Data *ev);
Eina_Bool error_imap(Email *e, int type, Ecore_Con_Event_Server_Error *ev );

Eina_Bool upgrade_smtp(Email *e, int type, Ecore_Con_Event_Server_Upgrade *ev);
Eina_Bool data_smtp(Email *e, int type, Ecore_Con_Event_Server_Data *ev);
Eina_Bool error_smtp(Email *e, int type, Ecore_Con_Event_Server_Error *ev );

/* return EINA_TRUE if e->ops should be popped */
Eina_Bool email_pop3_stat_read(Email *e, const unsigned char *recv, size_t size);
Eina_Bool email_pop3_list_read(Email *e, Ecore_Con_Event_Server_Data *ev);
Eina_Bool email_pop3_retr_read(Email *e, Ecore_Con_Event_Server_Data *ev);

Eina_Bool send_smtp(Email *e);

void email_login_pop(Email *e, Ecore_Con_Event_Server_Data *ev);
void email_login_smtp(Email *e, Ecore_Con_Event_Server_Data *ev);
int email_login_imap(Email *e, const unsigned char *data, size_t size, size_t *offset);

void imap_func_message_write(Email_Operation *op, Email_Message *msg, const char *mbox, Email_Imap4_Mail_Flag flags);

void email_fake_free(void *d, void *e);
Email_Operation *email_op_new(Email *e, unsigned int type, void *cb, const void *data);
void email_op_free(Email_Operation *op);
Email_Operation *email_op_pop(Email *e);

char *email_base64_encode(const unsigned char *string, size_t len, size_t *size);
unsigned char *email_base64_decode(const char *string, size_t len, size_t *size);
void email_md5_digest_to_str(unsigned char *digest, char *ret);
void email_md5_hmac_encode(unsigned char *digest, const char *string, size_t size, const void *key, size_t ksize);
#endif
