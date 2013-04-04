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
typedef struct Email_Attachment Email_Attachment;
typedef struct Email_Operation Email_Operation;

/* return EINA_TRUE if the list data should be automatically freed */
typedef Eina_Bool (*Email_List_Cb)(Email_Operation *, Eina_List */* Email_List_Item */);

typedef void (*Email_Stat_Cb)(Email_Operation *, unsigned int, size_t);
typedef void (*Email_Bool_Cb)(Email_Operation *, Eina_Bool);
typedef void (*Email_Retr_Cb)(Email_Operation *, Eina_Binbuf *);
typedef void (*Email_Send_Cb)(Email_Operation *, Email_Message *, Eina_Bool);
typedef void (*Email_Cb)(Email_Operation *);

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
   EMAIL_MESSAGE_CONTACT_TYPE_TO,
   EMAIL_MESSAGE_CONTACT_TYPE_CC,
   EMAIL_MESSAGE_CONTACT_TYPE_BCC
} Email_Message_Contact_Type;

typedef enum
{
   EMAIL_IMAP_MAILBOX_FLAG_HASCHILDREN = (1 << 0),
   EMAIL_IMAP_MAILBOX_FLAG_HASNOCHILDREN = (1 << 1),
   EMAIL_IMAP_MAILBOX_FLAG_MARKED = (1 << 2),
   EMAIL_IMAP_MAILBOX_FLAG_NOINFERIORS = (1 << 3),
   EMAIL_IMAP_MAILBOX_FLAG_NOSELECT = (1 << 4),
   EMAIL_IMAP_MAILBOX_FLAG_UNMARKED = (1 << 5),
} Email_Imap_Mailbox_Flag;

typedef struct
{
   unsigned int id;
   size_t size;
} Email_List_Item_Pop3; //messages

typedef struct
{
   Eina_Stringshare *name;
   Email_Imap_Mailbox_Flag flags;
} Email_List_Item_Imap4; //mailboxes

EAPI extern int EMAIL_EVENT_CONNECTED;
EAPI extern int EMAIL_EVENT_DISCONNECTED;

EAPI int email_init(void);
EAPI Email *email_new(const char *username, const char *password, void *data);
EAPI void email_free(Email *e);
EAPI void email_data_set(Email *e, const void *data);
EAPI void *email_data_get(const Email *e);
EAPI void email_cert_add(Email *e, const char *file);

EAPI Email *email_operation_email_get(const Email_Operation *op);
EAPI void *email_operation_data_get(const Email_Operation *op);
EAPI void email_operation_data_set(Email_Operation *op, const void *data);

EAPI const Eina_List */* Email_Operation */email_queue_get(Email *e);
EAPI Eina_Bool email_op_cancel(Email *e, Email_Operation *op);

EAPI void email_pop3_set(Email *e);
EAPI void email_imap4_set(Email *e);
EAPI void email_smtp_set(Email *e, const char *from_domain);
EAPI Eina_Bool email_connect(Email *e, const char *host, Eina_Bool secure);

EAPI Email_Operation *email_quit(Email *e, Email_Cb cb, const void *data);
EAPI Email_Operation * email_pop3_stat(Email *e, Email_Stat_Cb cb, const void *data);
EAPI Email_Operation * email_pop3_list(Email *e, Email_List_Cb cb, const void *data);
EAPI Email_Operation * email_pop3_rset(Email *e, Email_Cb cb, const void *data);
EAPI Email_Operation * email_pop3_delete(Email *e, unsigned int id, Email_Cb cb, const void *data);
EAPI Email_Operation * email_pop3_retrieve(Email *e, unsigned int id, Email_Retr_Cb cb, const void *data);

EAPI Email_Operation * email_imap4_list(Email *e, const char *reference, const char *mbox, Email_List_Cb cb, const void *data);
EAPI Email_Operation *email_imap4_select(Email *e, const char *mbox, Email_Bool_Cb cb, const void *data);

EAPI Email_Contact *email_contact_new(const char *address);
EAPI Email_Contact *email_contact_ref(Email_Contact *ec);
EAPI void email_contact_free(Email_Contact *ec);
EAPI void email_contact_name_set(Email_Contact *ec, const char *name);
EAPI void email_contact_address_set(Email_Contact *ec, const char *address);
EAPI const char *email_contact_name_get(Email_Contact *ec);
EAPI const char *email_contact_address_get(Email_Contact *ec);

EAPI Email_Attachment * email_attachment_new(const char *name, const char *content_type);
EAPI Email_Attachment * email_attachment_new_from_file(const char *filename, const char *content_type);
EAPI void email_attachment_free(Email_Attachment *at);
EAPI const char * email_attachment_name_get(Email_Attachment *at);
EAPI const char * email_attachment_content_type_get(Email_Attachment *at);
EAPI Eina_Bool email_attachment_content_set(Email_Attachment *at, const void *content, size_t size);
EAPI Eina_Bool email_attachment_content_steal(Email_Attachment *at, void *content, size_t size);
EAPI const unsigned char * email_attachment_content_get(Email_Attachment *at, size_t *size);

EAPI Email_Message * email_message_new(void);
EAPI void email_message_free(Email_Message *msg);
EAPI Email_Operation *email_message_send(Email *e, Email_Message *msg, Email_Send_Cb cb, const void *data);
EAPI void email_message_contact_add(Email_Message *msg, Email_Contact *ec, Email_Message_Contact_Type type);
EAPI void email_message_contact_del(Email_Message *msg, Email_Contact *ec);
EAPI void email_message_contact_del_by_address(Email_Message *msg, const char *address);
EAPI void email_message_from_add(Email_Message *msg, Email_Contact *ec);
EAPI void email_message_from_del(Email_Message *msg, Email_Contact *ec);
EAPI void email_message_sender_set(Email_Message *msg, Email_Contact *ec);
EAPI Email_Contact * email_message_sender_get(Email_Message *msg);
EAPI void email_message_subject_set(Email_Message *msg, const char *subject);
EAPI void email_message_subject_manage(Email_Message *msg, char *subject);
EAPI void email_message_content_set(Email_Message *msg, const char *content, size_t size);
EAPI void email_message_content_manage(Email_Message *msg, char *content, size_t size);
EAPI void email_message_header_set(Email_Message *msg, const char *name, const char *value);
EAPI const char * email_message_header_get(Email_Message *msg, const char *name);
EAPI const char * email_message_charset_get(Email_Message *msg);
EAPI void email_message_charset_set(Email_Message *msg, const char *charset);
EAPI double email_message_mime_version_get(Email_Message *msg);
EAPI void email_message_mime_version_set(Email_Message *msg, double version);
EAPI void email_message_attachment_add(Email_Message *msg, Email_Attachment *at);
EAPI void email_message_attachment_del(Email_Message *msg, Email_Attachment *at);
EAPI void email_message_data_set(Email_Message *msg, const void *data);
EAPI void * email_message_data_get(Email_Message *msg);
EAPI Email *email_message_email_get(Email_Message *msg);
#endif

