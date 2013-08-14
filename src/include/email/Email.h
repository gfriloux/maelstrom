#ifndef EMAIL_H
#define EMAIL_H

#include <Eina.h>

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif /* ! _WIN32 */

typedef struct Email Email;
typedef struct Email_Message Email_Message;
typedef struct Email_Contact Email_Contact;
typedef struct Email_Message_Part Email_Message_Part;
typedef struct Email_Operation Email_Operation;
typedef struct Email_Imap4_Mailbox_Info Email_Imap4_Mailbox_Info;
typedef struct Email_Imap4_Namespace_Extension Email_Imap4_Namespace_Extension;
typedef struct Email_Imap4_Namespace Email_Imap4_Namespace;


typedef enum
{
   EMAIL_TYPE_NONE,
   EMAIL_TYPE_POP3,
   EMAIL_TYPE_IMAP4,
   EMAIL_TYPE_SMTP,
   EMAIL_TYPE_LAST,
} Email_Type;

typedef enum
{
   EMAIL_MESSAGE_CONTACT_TYPE_NONE,
   EMAIL_MESSAGE_CONTACT_TYPE_TO,
   EMAIL_MESSAGE_CONTACT_TYPE_CC,
   EMAIL_MESSAGE_CONTACT_TYPE_BCC,
   EMAIL_MESSAGE_CONTACT_TYPE_LAST
} Email_Message_Contact_Type;

typedef enum
{
   EMAIL_MESSAGE_PART_ENCODING_NONE,
   EMAIL_MESSAGE_PART_ENCODING_7BIT,
   EMAIL_MESSAGE_PART_ENCODING_8BIT,
   EMAIL_MESSAGE_PART_ENCODING_BINARY,
   EMAIL_MESSAGE_PART_ENCODING_BASE64,
   EMAIL_MESSAGE_PART_ENCODING_QUOTED_PRINTABLE,
   EMAIL_MESSAGE_PART_ENCODING_LAST,
} Email_Message_Part_Encoding;

typedef enum
{
   EMAIL_OPERATION_STATUS_NONE, /**< for non-imap servers, this means operation failed */
   EMAIL_OPERATION_STATUS_OK,
   EMAIL_OPERATION_STATUS_NO,
   EMAIL_OPERATION_STATUS_BAD,
   EMAIL_OPERATION_STATUS_LAST
} Email_Operation_Status;

typedef enum
{
   EMAIL_IMAP4_NAMESPACE_PERSONAL,
   EMAIL_IMAP4_NAMESPACE_USER,
   EMAIL_IMAP4_NAMESPACE_SHARED,
   EMAIL_IMAP4_NAMESPACE_LAST
} Email_Imap4_Namespace_Type;

typedef enum
{
   EMAIL_IMAP4_MAILBOX_ATTRIBUTE_HASCHILDREN = (1 << 0),
   EMAIL_IMAP4_MAILBOX_ATTRIBUTE_HASNOCHILDREN = (1 << 1),
   EMAIL_IMAP4_MAILBOX_ATTRIBUTE_MARKED = (1 << 2),
   EMAIL_IMAP4_MAILBOX_ATTRIBUTE_NOINFERIORS = (1 << 3),
   EMAIL_IMAP4_MAILBOX_ATTRIBUTE_NOSELECT = (1 << 4),
   EMAIL_IMAP4_MAILBOX_ATTRIBUTE_UNMARKED = (1 << 5),
   EMAIL_IMAP4_MAILBOX_ATTRIBUTE_ITERATE = 6,
} Email_Imap4_Mailbox_Attribute;

typedef enum
{
   EMAIL_IMAP4_MAIL_FLAG_ANSWERED = (1 << 0),
   EMAIL_IMAP4_MAIL_FLAG_DELETED = (1 << 1),
   EMAIL_IMAP4_MAIL_FLAG_DRAFT = (1 << 2),
   EMAIL_IMAP4_MAIL_FLAG_FLAGGED = (1 << 3),
   EMAIL_IMAP4_MAIL_FLAG_RECENT = (1 << 4),
   EMAIL_IMAP4_MAIL_FLAG_SEEN = (1 << 5),
   EMAIL_IMAP4_MAIL_FLAG_STAR = (1 << 6),
   EMAIL_IMAP4_MAIL_FLAG_ITERATE = 7,
} Email_Imap4_Mail_Flag;

typedef enum
{
   EMAIL_IMAP4_MAILBOX_ACCESS_NONE = 0,
   EMAIL_IMAP4_MAILBOX_ACCESS_READONLY,
   EMAIL_IMAP4_MAILBOX_ACCESS_READWRITE,
} Email_Imap4_Mailbox_Access;

typedef enum
{
   EMAIL_IMAP4_MAILBOX_RIGHT_NONE = 0,
   EMAIL_IMAP4_MAILBOX_RIGHT_LOOKUP = (1 << 0),
   EMAIL_IMAP4_MAILBOX_RIGHT_READ = (1 << 1),
   EMAIL_IMAP4_MAILBOX_RIGHT_SEEN = (1 << 2),
   EMAIL_IMAP4_MAILBOX_RIGHT_WRITE = (1 << 3),
   EMAIL_IMAP4_MAILBOX_RIGHT_INSERT = (1 << 4),
   EMAIL_IMAP4_MAILBOX_RIGHT_POST = (1 << 5),
   EMAIL_IMAP4_MAILBOX_RIGHT_CREATE = (1 << 6),
   EMAIL_IMAP4_MAILBOX_RIGHT_DELETE_MBOX = (1 << 7),
   EMAIL_IMAP4_MAILBOX_RIGHT_DELETE_MSG = (1 << 8),
   EMAIL_IMAP4_MAILBOX_RIGHT_EXPUNGE = (1 << 9),
   EMAIL_IMAP4_MAILBOX_RIGHT_ADMIN = (1 << 10),
   EMAIL_IMAP4_MAILBOX_RIGHT_ITERATE = 11,
} Email_Imap4_Mailbox_Rights;

typedef enum
{
   EMAIL_IMAP4_FETCH_TYPE_NONE = 0,
   EMAIL_IMAP4_FETCH_TYPE_ALL,
   EMAIL_IMAP4_FETCH_TYPE_FULL,
   EMAIL_IMAP4_FETCH_TYPE_FAST,
   EMAIL_IMAP4_FETCH_TYPE_ENVELOPE,
   EMAIL_IMAP4_FETCH_TYPE_FLAGS,
   EMAIL_IMAP4_FETCH_TYPE_INTERNALDATE,
   EMAIL_IMAP4_FETCH_TYPE_BODYSTRUCTURE,
   EMAIL_IMAP4_FETCH_TYPE_UID,
   EMAIL_IMAP4_FETCH_TYPE_RFC822,
   EMAIL_IMAP4_FETCH_TYPE_BODY,
   EMAIL_IMAP4_FETCH_TYPE_LAST,
} Email_Imap4_Fetch_Type;

#define EMAIL_IMAP4_UTIL_FETCH_INFOS(...) (Email_Imap4_Fetch_Info *[]){__VA_ARGS__, NULL}, 0

/* covers ALL to UID */
#define EMAIL_IMAP4_UTIL_FETCH_BASIC(TYPE) &(Email_Imap4_Fetch_Info){.type = EMAIL_IMAP4_FETCH_TYPE_##TYPE}

/* RFC822 utils */
#define EMAIL_IMAP4_UTIL_FETCH_RFC822 &(Email_Imap4_Fetch_Info){.type = EMAIL_IMAP4_FETCH_TYPE_RFC822, .u.rfc822 = {0, 0, 0}}
#define EMAIL_IMAP4_UTIL_FETCH_RFC822_HEADER &(Email_Imap4_Fetch_Info){.type = EMAIL_IMAP4_FETCH_TYPE_RFC822, .u.rfc822 = {1, 0, 0}}
#define EMAIL_IMAP4_UTIL_FETCH_RFC822_SIZE &(Email_Imap4_Fetch_Info){.type = EMAIL_IMAP4_FETCH_TYPE_RFC822, .u.rfc822 = {0, 1, 0}}
#define EMAIL_IMAP4_UTIL_FETCH_RFC822_TEXT &(Email_Imap4_Fetch_Info){.type = EMAIL_IMAP4_FETCH_TYPE_RFC822, .u.rfc822 = {0, 0, 1}}

/* body utils */
#define EMAIL_IMAP4_UTIL_BODY_PARTS(...) (unsigned int[]){__VA_ARGS__, 0}
#define EMAIL_IMAP4_UTIL_BODY_FIELDS(...) (const char *[]){__VA_ARGS__, NULL}

#define EMAIL_IMAP4_UTIL_FETCH_BODY(PEEK, START, END, UTIL_PARTS) \
  &(Email_Imap4_Fetch_Info){.type = EMAIL_IMAP4_FETCH_TYPE_BODY, .u.body = {UTIL_PARTS, 0, {START, END}, NULL, 0, 0, PEEK}}
#define EMAIL_IMAP4_UTIL_FETCH_BODY_TEXT(PEEK, START, END, UTIL_PARTS) \
  &(Email_Imap4_Fetch_Info){.type = EMAIL_IMAP4_FETCH_TYPE_BODY, .u.body = {UTIL_PARTS, 0, {START, END}, NULL, 0, EMAIL_IMAP4_FETCH_BODY_TYPE_TEXT, PEEK}}
#define EMAIL_IMAP4_UTIL_FETCH_BODY_MIME(PEEK, START, END, UTIL_PARTS) \
  &(Email_Imap4_Fetch_Info){.type = EMAIL_IMAP4_FETCH_TYPE_BODY, .u.body = {UTIL_PARTS, 0, {START, END}, NULL, 0, EMAIL_IMAP4_FETCH_BODY_TYPE_MIME, PEEK}}
#define EMAIL_IMAP4_UTIL_FETCH_BODY_HEADER(PEEK, START, END, UTIL_PARTS) \
  &(Email_Imap4_Fetch_Info){.type = EMAIL_IMAP4_FETCH_TYPE_BODY, .u.body = {UTIL_PARTS, 0, {START, END}, NULL, 0, EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER, PEEK}}
#define EMAIL_IMAP4_UTIL_FETCH_BODY_HEADER_FIELDS(PEEK, START, END, UTIL_PARTS, UTIL_FIELDS) \
  &(Email_Imap4_Fetch_Info){.type = EMAIL_IMAP4_FETCH_TYPE_BODY, .u.body = {UTIL_PARTS, 0, {START, END}, UTIL_FIELDS, 0, EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER_FIELDS, PEEK}}
#define EMAIL_IMAP4_UTIL_FETCH_BODY_HEADER_FIELDS_NOT(PEEK, START, END, UTIL_PARTS, UTIL_FIELDS) \
  &(Email_Imap4_Fetch_Info){.type = EMAIL_IMAP4_FETCH_TYPE_BODY, .u.body = {UTIL_PARTS, 0, {START, END}, UTIL_FIELDS, 0, EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER_FIELDS_NOT, PEEK}}

typedef enum
{
   EMAIL_IMAP4_FETCH_BODY_TYPE_NONE = 0,
   EMAIL_IMAP4_FETCH_BODY_TYPE_TEXT,
   EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER,
   EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER_FIELDS,
   EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER_FIELDS_NOT,
   EMAIL_IMAP4_FETCH_BODY_TYPE_MIME,
   EMAIL_IMAP4_FETCH_BODY_TYPE_LAST,
} Email_Imap4_Fetch_Body_Type;

typedef struct
{
   Email_Imap4_Fetch_Type type;
   union
   {
      struct
      {
         Eina_Bool header : 1;
         Eina_Bool size : 1;
         Eina_Bool text : 1;
      } rfc822;
      struct
      {
         unsigned int *parts;
         unsigned int part_count;
         size_t range[2];

         const char **fields;
         unsigned int fields_count;
         Email_Imap4_Fetch_Body_Type type;
         Eina_Bool peek : 1;
      } body;
   } u;
} Email_Imap4_Fetch_Info;

typedef struct
{
   unsigned int id;
   size_t size;
} Email_List_Item_Pop3; //messages

typedef struct
{
   Eina_Stringshare *name;
   Email_Imap4_Mailbox_Attribute attributes;
} Email_List_Item_Imap4; //mailboxes

typedef struct
{
   Email *e;
   unsigned int uid;
   unsigned long internaldate;
   Email_Imap4_Mail_Flag flags;
   Eina_Strbuf *boundary;
   Eina_Hash *params;
   Email_Message *msg;
   Eina_Inarray *part_nums;
   size_t size;
   Email_Imap4_Fetch_Body_Type type;
} Email_Imap4_Message;

struct Email_Imap4_Mailbox_Info
{
   Email *e;
   Email_Imap4_Mail_Flag flags;
   Email_Imap4_Mail_Flag permanentflags;
   Email_Imap4_Mailbox_Access access;
   Email_Imap4_Mailbox_Rights rights;
   unsigned int exists;
   Eina_Inarray *expunge; // unsigned int
   Eina_Inarray *fetch; // Email_Imap4_Message
   unsigned int recent;
   unsigned int unseen;
   unsigned long long uidvalidity;
   unsigned long long uidnext;
};

struct Email_Imap4_Namespace_Extension
{
   Eina_Stringshare *name; /**< extension name */
   Eina_Inarray *flags; /**< array of Eina_Stringshare flags */
};

struct Email_Imap4_Namespace
{
   Eina_Stringshare *prefix; /**< prefix required to access mailboxes in this namespace */
   Eina_Stringshare *delim; /**< delimiter used to separate mailbox hierarchies in this namespace */
   Eina_Hash *extensions; /**< extension flags for the namespace (name:#Email_Imap4_Namespace_Extension) */
};

/* return EINA_TRUE if the list data should be automatically freed */
typedef Eina_Bool (*Email_List_Cb)(Email_Operation *, Eina_List */* Email_List_Item */);
typedef Eina_Bool (*Email_Imap4_Mailbox_Info_Cb)(Email_Operation *, Email_Imap4_Mailbox_Info *);
typedef Eina_Bool (*Email_Imap4_Fetch_Cb)(Email_Operation *, Eina_Inarray * /* Email_Imap4_Message */);
typedef Eina_Bool (*Email_Retr_Cb)(Email_Operation *, Eina_Binbuf *);
typedef Eina_Bool (*Email_Send_Cb)(Email_Operation *, Email_Message *, Eina_Bool);
typedef void (*Email_Stat_Cb)(Email_Operation *, unsigned int, size_t);
typedef void (*Email_Cb)(Email_Operation *, Email_Operation_Status);

EAPI extern int EMAIL_EVENT_CONNECTED;
EAPI extern int EMAIL_EVENT_DISCONNECTED;

EAPI extern int EMAIL_EVENT_MAILBOX_STATUS; /**< sends Email_Imap4_Mailbox_Info */

EAPI int email_init(void);
EAPI void email_shutdown(void);
EAPI Email *email_new(const char *username, const char *password, void *data);
EAPI void email_free(Email *e);
EAPI void email_data_set(Email *e, const void *data);
EAPI void *email_data_get(const Email *e);
EAPI void email_cert_add(Email *e, const char *file);

EAPI Email *email_operation_email_get(const Email_Operation *op);
EAPI void *email_operation_data_get(const Email_Operation *op);
EAPI void email_operation_data_set(Email_Operation *op, const void *data);
EAPI const char *email_operation_status_message_get(const Email_Operation *op);
EAPI Eina_Bool email_operation_cancel(Email *e, Email_Operation *op);
EAPI void email_operation_blocking_set(Email_Operation *op);

EAPI const Eina_List */* Email_Operation */email_operations_get(Email *e);

EAPI void email_pop3_set(Email *e);
EAPI void email_imap4_set(Email *e);
EAPI void email_smtp_set(Email *e, const char *from_domain);
EAPI Eina_Bool email_connect(Email *e, const char *host, Eina_Bool secure);

EAPI Email_Operation *email_quit(Email *e, Email_Cb cb, const void *data);
EAPI Email_Operation *email_pop3_stat(Email *e, Email_Stat_Cb cb, const void *data);
EAPI Email_Operation *email_pop3_list(Email *e, Email_List_Cb cb, const void *data);
EAPI Email_Operation *email_pop3_rset(Email *e, Email_Cb cb, const void *data);
EAPI Email_Operation *email_pop3_delete(Email *e, unsigned int id, Email_Cb cb, const void *data);
EAPI Email_Operation *email_pop3_retrieve(Email *e, unsigned int id, Email_Retr_Cb cb, const void *data);

EAPI Eina_Stringshare *email_imap4_mbox_get(const Email *e);
EAPI void email_imap4_message_free(Email_Imap4_Message *im);
EAPI void email_imap4_mailboxinfo_free(Email_Imap4_Mailbox_Info *info);
/* only useful from inside a direct imap callback */
EAPI Email_Operation_Status email_operation_imap4_status_get(const Email_Operation *op);
EAPI const Eina_Inarray /* Email_Imap_Namespace */ *email_imap4_namespaces_get(const Email *e, Email_Imap4_Namespace_Type type);
EAPI Email_Operation *email_imap4_list(Email *e, const char *reference, const char *mbox, Email_List_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_lsub(Email *e, const char *reference, const char *mbox, Email_List_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_select(Email *e, const char *mbox, Email_Imap4_Mailbox_Info_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_examine(Email *e, const char *mbox, Email_Imap4_Mailbox_Info_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_expunge(Email *e, Email_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_noop(Email *e);
EAPI Email_Operation *email_imap4_close(Email *e, Email_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_create(Email *e, const char *mbox, Email_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_delete(Email *e, const char *mbox, Email_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_rename(Email *e, const char *mbox, const char *newmbox, Email_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_subscribe(Email *e, const char *mbox, Email_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_unsubscribe(Email *e, const char *mbox, Email_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_append(Email *e, const char *mbox, Email_Message *msg, Email_Imap4_Mail_Flag flags, Email_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_fetch(Email *e, unsigned int range[2], Email_Imap4_Fetch_Info **infos, unsigned int info_count, Email_Imap4_Fetch_Cb cb, const void *data);

EAPI Email_Contact *email_contact_new(const char *address);
EAPI Email_Contact *email_contact_ref(Email_Contact *ec);
EAPI void email_contact_free(Email_Contact *ec);
EAPI void email_contact_name_set(Email_Contact *ec, const char *name);
EAPI void email_contact_address_set(Email_Contact *ec, const char *address);
EAPI const char *email_contact_name_get(Email_Contact *ec);
EAPI const char *email_contact_address_get(Email_Contact *ec);

EAPI Email_Message_Part *email_message_part_new(const char *name, const char *content_type);
EAPI Email_Message_Part *email_message_part_text_new(const char *text, size_t len);
EAPI Email_Message_Part *email_message_part_ref(Email_Message_Part *part);
EAPI Email_Message_Part *email_message_part_new_from_file(const char *filename, const char *content_type);
EAPI void email_message_part_free(Email_Message_Part *at);
EAPI size_t email_message_part_size_get(const Email_Message_Part *part);
EAPI void email_message_part_name_set(Email_Message_Part *part, const char *name);
EAPI Eina_Stringshare *email_message_part_name_get(Email_Message_Part *at);
EAPI Eina_Stringshare *email_message_part_content_type_get(Email_Message_Part *at);
EAPI Eina_Bool email_message_part_content_set(Email_Message_Part *at, const void *content, size_t size);
EAPI Eina_Bool email_message_part_content_manage(Email_Message_Part *at, void *content, size_t size);
EAPI const Eina_Binbuf *email_message_part_content_get(Email_Message_Part *at);
EAPI Eina_Binbuf *email_message_part_content_steal(Email_Message_Part *at);
EAPI Eina_Stringshare *email_message_part_charset_get(const Email_Message_Part *part);
EAPI void email_message_part_charset_set(Email_Message *msg, const char *charset);
EAPI void email_message_part_part_add(Email_Message_Part *part, Email_Message_Part *subpart);
EAPI void email_message_part_part_del(Email_Message_Part *part, Email_Message_Part *subpart);
EAPI const Eina_List *email_message_part_parts_get(const Email_Message_Part *part);

EAPI Email_Message * email_message_new(void);
EAPI void email_message_free(Email_Message *msg);
EAPI const Eina_List *email_message_parts_get(const Email_Message *msg);
EAPI const Email_Message_Part *email_message_content_get(const Email_Message *msg);
EAPI void email_message_content_set(Email_Message *msg, Email_Message_Part *content);
EAPI Email_Operation *email_message_send(Email *e, Email_Message *msg, Email_Send_Cb cb, const void *data);
EAPI void email_message_contact_add(Email_Message *msg, Email_Contact *ec, Email_Message_Contact_Type type);
EAPI void email_message_contact_del(Email_Message *msg, Email_Contact *ec, Email_Message_Contact_Type type);
EAPI void email_message_from_add(Email_Message *msg, Email_Contact *ec);
EAPI void email_message_from_del(Email_Message *msg, Email_Contact *ec);
EAPI void email_message_sender_add(Email_Message *msg, Email_Contact *ec);
EAPI void email_message_subject_set(Email_Message *msg, const char *subject);
EAPI Eina_Stringshare *email_message_msgid_get(const Email_Message *msg);
EAPI void email_message_msgid_set(Email_Message *msg, const char *msgid);
EAPI Eina_Stringshare *email_message_in_reply_to_get(const Email_Message *msg);
EAPI void email_message_in_reply_to_set(Email_Message *msg, const char *in_reply_to);
EAPI void email_message_header_set(Email_Message *msg, const char *name, const char *value);
EAPI const char *email_message_header_get(Email_Message *msg, const char *name);
EAPI double email_message_mime_version_get(Email_Message *msg);
EAPI void email_message_mime_version_set(Email_Message *msg, double version);
EAPI void email_message_part_add(Email_Message *msg, Email_Message_Part *at);
EAPI void email_message_part_del(Email_Message *msg, Email_Message_Part *at);
EAPI void email_message_data_set(Email_Message *msg, const void *data);
EAPI void * email_message_data_get(Email_Message *msg);
EAPI void email_message_date_set(Email_Message *msg, unsigned long date);
EAPI unsigned long email_message_date_get(const Email_Message *msg);
EAPI Eina_Strbuf *email_message_serialize(const Email_Message *msg);
EAPI Email *email_message_email_get(Email_Message *msg);

EAPI void email_util_inarray_stringshare_free(Eina_Inarray *arr);
EAPI const char *email_util_encoding_string_get(Email_Message_Part_Encoding encoding);
EAPI Email_Message_Part_Encoding email_util_encoding_type_get(const char *uppercase);
#endif

