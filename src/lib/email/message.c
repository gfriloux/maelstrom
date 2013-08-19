#include "email_private.h"
#include <math.h>

#define MESSAGE_BOUNDARY "--ASDF8AYWE0G8H31_//123R"

static int
part_sort_cb(const Email_Message_Part *a, const Email_Message_Part *b)
{
   if (a->num > b->num) return -1;
   if (a->num < b->num) return 1;
   return 0;
}

static Eina_Bool
_email_message_headers(const Eina_Hash *h EINA_UNUSED, const char *key, const char *data, Eina_Strbuf *buf)
{
   if (!strcasecmp(key, "content-type")) return EINA_TRUE;
   eina_strbuf_append_printf(buf, "%s: %s\r\n", key, data);
   return EINA_TRUE;
}

void
_message_part_serialize(Eina_Strbuf *buf, const Email_Message_Part *part, Eina_Bool is_content)
{
   const Eina_List *l;
   const Email_Message_Part *subpart;

   eina_strbuf_append_printf(buf, "Content-Type: %s;%s%s\r\n", part->content_type ?: "text/plain", part->charset ? " charset=" : "", part->charset ?: "");
   if (part->encoding)
     eina_strbuf_append_printf(buf, "Content-Transfer-Encoding: %s;\r\n", email_util_encoding_string_get(part->encoding));
   if (part->name)
     eina_strbuf_append_printf(buf, "Content-Disposition: attachment; filename=%s\r\n", part->name);
   else if (is_content)
     eina_strbuf_append(buf, "Content-Disposition: inline\r\n");
   eina_strbuf_append_length(buf, CRLF, CRLFLEN);
   if (part->content)
     {
        eina_strbuf_append_length(buf, (char*)EBUF(part->content), EBUFLEN(part->content));
        eina_strbuf_append_length(buf, CRLF, CRLFLEN);
     }
   if (!part->parts) return;
   eina_strbuf_append_printf(buf, "Content-Type: multipart/%s; boundary=\""MESSAGE_BOUNDARY"%p\"\r\n\r\n", part->attachments ? "mixed" : "alternative", part);
   EINA_LIST_FOREACH(part->parts, l, subpart)
     {
        eina_strbuf_append_printf(buf, "--"MESSAGE_BOUNDARY "%p" CRLF, part);
        _message_part_serialize(buf, subpart, 0);
     }
   eina_strbuf_append_printf(buf, MESSAGE_BOUNDARY"%p--" CRLF, part);
}

Eina_Strbuf *
email_message_serialize(const Email_Message *msg)
{
   Eina_Strbuf *buf;
   Eina_List *l;
   Email_Contact *ec;
   time_t date;
   char timebuf[1024];
   struct tm *t;
/*
   From: Michael Blumenkrantz <michael.blumenkrantz@gmail.com>
   To: Michael Blumenkrantz <michael.blumenkrantz@gmail.com>
   Subject: test
   Message-ID: <20110701200521.12e0b66c@darc.ath.cx>
   X-Mailer: Claws Mail 3.7.9 (GTK+ 2.24.3; i686-pc-linux-gnu)
   Mime-Version: 1.0
   Content-Type: multipart/mixed; boundary="MP_/6MDEYc2h_pXILCyZXK5yi=w"

   --MP_/6MDEYc2h_pXILCyZXK5yi=w
   Content-Type: text/plain; charset=US-ASCII
   Content-Transfer-Encoding: 8bit
   Content-Disposition: inline

   testsetstst
   --MP_/6MDEYc2h_pXILCyZXK5yi=w
   Content-Type: application/x-gzip
   Content-Transfer-Encoding: base64
   Content-Disposition: attachment; filename=azy_dl_up.tar.gz
*/

   buf = eina_strbuf_new();
   date = msg->date;
   if (!date) date = lround(ecore_time_unix_get());
   t = localtime(&date);
   strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %T %z (%Z)", t);
   eina_strbuf_append_printf(buf, "Date: %s\r\n", timebuf);
   EINA_LIST_FOREACH(msg->from, l, ec)
     {
        if (ec->name)
          eina_strbuf_append_printf(buf, "From: %s <%s>%s", ec->name, ec->address, l->next ? "," : "\r\n");
        else
          eina_strbuf_append_printf(buf, "From: %s%s", ec->address, l->next ? "," : "\r\n");
     }
   EINA_LIST_FOREACH(msg->sender, l, ec)
     {
        if (ec->name)
          eina_strbuf_append_printf(buf, "Sender: %s <%s>%s", ec->name, ec->address, l->next ? "," : "\r\n");
        else
          eina_strbuf_append_printf(buf, "Sender: %s%s", ec->address, l->next ? "," : "\r\n");
     }
   EINA_LIST_FOREACH(msg->reply_to, l, ec)
     {
        if (ec->name)
          eina_strbuf_append_printf(buf, "Reply-To: %s <%s>%s", ec->name, ec->address, l->next ? "," : "\r\n");
        else
          eina_strbuf_append_printf(buf, "Reply-To: %s%s", ec->address, l->next ? "," : "\r\n");
     }
   EINA_LIST_FOREACH(msg->to, l, ec)
     {
        if (ec->name)
          eina_strbuf_append_printf(buf, "To: %s <%s>%s", ec->name, ec->address, l->next ? "," : "\r\n");
        else
          eina_strbuf_append_printf(buf, "To: %s%s", ec->address, l->next ? "," : "\r\n");
     }
   EINA_LIST_FOREACH(msg->cc, l, ec)
     {
        if (ec->name)
          eina_strbuf_append_printf(buf, "Cc: %s <%s>%s", ec->name, ec->address, l->next ? "," : "\r\n");
        else
          eina_strbuf_append_printf(buf, "Cc: %s%s", ec->address, l->next ? "," : "\r\n");
     }
   EINA_LIST_FOREACH(msg->bcc, l, ec)
     {
        if (ec->name)
          eina_strbuf_append_printf(buf, "Bcc: %s <%s>%s", ec->name, ec->address, l->next ? "," : "\r\n");
        else
          eina_strbuf_append_printf(buf, "Bcc: %s%s", ec->address, l->next ? "," : "\r\n");
     }
   if (msg->subject) eina_strbuf_append_printf(buf, "Subject: %s\r\n", msg->subject);
   if (msg->in_reply_to) eina_strbuf_append_printf(buf, "In-Reply-To: %s\r\n", msg->in_reply_to);
   if (msg->msgid) eina_strbuf_append_printf(buf, "Message-Id: %s\r\n", msg->msgid);
   if (msg->headers) eina_hash_foreach(msg->headers, (Eina_Hash_Foreach)_email_message_headers, buf);
   eina_strbuf_append_printf(buf, "MIME-Version: %1.1f\r\n", (msg->mimeversion >= 1.0) ? msg->mimeversion : 1.0);
   if (msg->parts)
     {
        Email_Message_Part *part;

        eina_strbuf_append_printf(buf, "Content-Type: multipart/%s; boundary=\""MESSAGE_BOUNDARY"%p\"\r\n\r\n", msg->attachments ? "mixed" : "alternative", msg);
        if (msg->content)
          {
             eina_strbuf_append_printf(buf, "--"MESSAGE_BOUNDARY "%p" CRLF, msg);
             _message_part_serialize(buf, msg->content, 1);
          }
        EINA_LIST_FOREACH(msg->parts, l, part)
          {
             eina_strbuf_append_printf(buf, "--"MESSAGE_BOUNDARY "%p" CRLF, msg);
             _message_part_serialize(buf, part, 0);
          }
        eina_strbuf_append_printf(buf, MESSAGE_BOUNDARY"%p--" CRLF, msg);
     }
   else if (msg->content)
     _message_part_serialize(buf, msg->content, 0);
   if (!email_is_imap(msg->owner))
     eina_strbuf_append(buf, "\r\n.\r\n"); //smtp requires CRLF.CRLF trailer
   return buf;
}

Email_Message *
email_message_new(void)
{
   Email_Message *msg;
   msg = calloc(1, sizeof(Email_Message));
   if (!msg) return NULL;
   msg->mimeversion = 1.0;
   return msg;
}

void
email_message_free(Email_Message *msg)
{
   Email_Contact *ec;
   Email_Message_Part *part;

   if (!msg) return;
   msg->deleted = 1;
   if (msg->sending) return;
   eina_stringshare_del(msg->subject);
   eina_stringshare_del(msg->in_reply_to);
   eina_stringshare_del(msg->msgid);
   email_message_part_free(msg->content);
   EINA_LIST_FREE(msg->parts, part)
     email_message_part_free(part);
   EINA_LIST_FREE(msg->from, ec)
     email_contact_free(ec);
   EINA_LIST_FREE(msg->sender, ec)
     email_contact_free(ec);
   EINA_LIST_FREE(msg->to, ec)
     email_contact_free(ec);
   EINA_LIST_FREE(msg->cc, ec)
     email_contact_free(ec);
   EINA_LIST_FREE(msg->bcc, ec)
     email_contact_free(ec);
   free(msg);
}

Email_Operation *
email_message_send(Email *e, Email_Message *msg, Email_Send_Cb cb, const void *data)
{
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(msg->deleted, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(msg->owner && (msg->owner != e), NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg->to, NULL);

   msg->owner = e;
   op = email_op_new(e, EMAIL_SMTP_OP_SEND, cb, data);
   op->opdata = msg;
   if (!email_is_blocked(e)) send_smtp(e);
   return op;
}

void
email_message_contact_add(Email_Message *msg, Email_Contact *ec, Email_Message_Contact_Type type)
{
   EINA_SAFETY_ON_NULL_RETURN(msg);
   EINA_SAFETY_ON_NULL_RETURN(ec);

   switch (type)
     {
      case EMAIL_MESSAGE_CONTACT_TYPE_TO:
        msg->to = eina_list_append(msg->to, email_contact_ref(ec));
        break;
      case EMAIL_MESSAGE_CONTACT_TYPE_CC:
        msg->cc = eina_list_append(msg->to, email_contact_ref(ec));
        break;
      case EMAIL_MESSAGE_CONTACT_TYPE_BCC:
        msg->bcc = eina_list_append(msg->to, email_contact_ref(ec));
        break;
      default: break;
     }
}

void
email_message_contact_del(Email_Message *msg, Email_Contact *ec, Email_Message_Contact_Type type)
{
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(msg);
   EINA_SAFETY_ON_NULL_RETURN(ec);

   switch (type)
     {
      case EMAIL_MESSAGE_CONTACT_TYPE_TO:
        msg->to = eina_list_remove(msg->to, ec);
        break;
      case EMAIL_MESSAGE_CONTACT_TYPE_CC:
        msg->cc = eina_list_remove(msg->to, ec);
        break;
      case EMAIL_MESSAGE_CONTACT_TYPE_BCC:
        msg->bcc = eina_list_remove(msg->to, ec);
        break;
      default:
        l = eina_list_data_find_list(msg->to, ec);
        if (l)
          {
             msg->to = eina_list_remove_list(msg->to, l);
             email_contact_free(ec);
          }
        l = eina_list_data_find_list(msg->cc, ec);
        if (l)
          {
             msg->to = eina_list_remove_list(msg->cc, l);
             email_contact_free(ec);
          }
        l = eina_list_data_find_list(msg->bcc, ec);
        if (l)
          {
             msg->to = eina_list_remove_list(msg->bcc, l);
             email_contact_free(ec);
          }
     }
   email_contact_free(ec);
}

void
email_message_from_add(Email_Message *msg, Email_Contact *ec)
{
   EINA_SAFETY_ON_NULL_RETURN(msg);
   EINA_SAFETY_ON_NULL_RETURN(ec);
   if (!eina_list_data_find(msg->from, ec))
     msg->from = eina_list_append(msg->from, email_contact_ref(ec));
}

void
email_message_from_del(Email_Message *msg, Email_Contact *ec)
{
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(msg);
   EINA_SAFETY_ON_NULL_RETURN(ec);
   l = eina_list_data_find_list(msg->from, ec);
   if (!l) return;
   msg->from = eina_list_remove_list(msg->from, l);
   email_contact_free(ec);
}

void
email_message_sender_add(Email_Message *msg, Email_Contact *ec)
{
   EINA_SAFETY_ON_NULL_RETURN(msg);
   if (ec) email_contact_ref(ec);
   if (!eina_list_data_find(msg->sender, ec))
     msg->sender = eina_list_append(msg->sender, email_contact_ref(ec));
}

void
email_message_sender_del(Email_Message *msg, Email_Contact *ec)
{
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(msg);
   EINA_SAFETY_ON_NULL_RETURN(ec);
   l = eina_list_data_find_list(msg->sender, ec);
   if (!l) return;
   msg->sender = eina_list_remove_list(msg->sender, l);
   email_contact_free(ec);
}

unsigned long
email_message_date_get(const Email_Message *msg)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, 0);
   return msg->date;
}

void
email_message_date_set(Email_Message *msg, unsigned long date)
{
   EINA_SAFETY_ON_NULL_RETURN(msg);
   msg->date = date;
}

Eina_Stringshare *
email_message_in_reply_to_get(const Email_Message *msg)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, 0);
   return msg->in_reply_to;
}

void
email_message_in_reply_to_set(Email_Message *msg, const char *in_reply_to)
{
   EINA_SAFETY_ON_NULL_RETURN(msg);
   eina_stringshare_replace(&msg->in_reply_to, in_reply_to);
}

Eina_Stringshare *
email_message_msgid_get(const Email_Message *msg)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, 0);
   return msg->msgid;
}

void
email_message_msgid_set(Email_Message *msg, const char *msgid)
{
   EINA_SAFETY_ON_NULL_RETURN(msg);
   eina_stringshare_replace(&msg->msgid, msgid);
}

void
email_message_subject_set(Email_Message *msg, const char *subject)
{
   EINA_SAFETY_ON_NULL_RETURN(msg);
   EINA_SAFETY_ON_NULL_RETURN(subject);

   eina_stringshare_replace(&msg->subject, subject);
}

void
email_message_header_set(Email_Message *msg, const char *name, const char *value)
{
   char *old;

   EINA_SAFETY_ON_NULL_RETURN(msg);
   EINA_SAFETY_ON_NULL_RETURN(name);

   if (!msg->headers) msg->headers = eina_hash_string_superfast_new(free);

   if (value)
     {
        old = eina_hash_set(msg->headers, name, strdup(value));
        free(old);
     }
   else
     eina_hash_del_by_key(msg->headers, name);
}

const char *
email_message_header_get(Email_Message *msg, const char *name)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, NULL);
   if (!msg->headers) return NULL;
   return eina_hash_find(msg->headers, name);
}

double
email_message_mime_version_get(Email_Message *msg)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, 0.0);
   return msg->mimeversion;
}

void
email_message_mime_version_set(Email_Message *msg, double version)
{
   EINA_SAFETY_ON_NULL_RETURN(msg);
   msg->mimeversion = version;
}

const Eina_List *
email_message_parts_get(const Email_Message *msg)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, NULL);
   return msg->parts;
}

const Email_Message_Part *
email_message_content_get(const Email_Message *msg)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, NULL);
   return msg->content;
}

void
email_message_content_set(Email_Message *msg, Email_Message_Part *content)
{
   EINA_SAFETY_ON_NULL_RETURN(msg);
   if (msg->content)
     {
        msg->attachments -= !!content->name;
        email_message_part_free(msg->content);
        msg->content = NULL;
     }
   if (!content) return;
   msg->content = email_message_part_ref(content);
   msg->attachments += !!content->name;
}

void
email_message_part_add(Email_Message *msg, Email_Message_Part *part)
{
   EINA_SAFETY_ON_NULL_RETURN(msg);
   EINA_SAFETY_ON_NULL_RETURN(part);
   if (part->num)
     msg->parts = eina_list_sorted_insert(msg->parts, (Eina_Compare_Cb)part_sort_cb, email_message_part_ref(part));
   else
     msg->parts = eina_list_append(msg->parts, email_message_part_ref(part));
   msg->attachments += !!part->name;
}

void
email_message_part_del(Email_Message *msg, Email_Message_Part *part)
{
   Eina_List *l;
   EINA_SAFETY_ON_NULL_RETURN(msg);
   EINA_SAFETY_ON_NULL_RETURN(part);
   l = eina_list_data_find_list(msg->parts, part);
   if (!l) return;
   msg->parts = eina_list_remove_list(msg->parts, l);
   msg->attachments -= !!part->name;
   email_message_part_free(part);
}

void
email_message_data_set(Email_Message *msg, const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(msg);
   msg->data = (void*)data;
}

void *
email_message_data_get(Email_Message *msg)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, NULL);
   return msg->data;
}

Email *
email_message_email_get(Email_Message *msg)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, NULL);
   return msg->owner;
}
