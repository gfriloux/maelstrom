#include "email_private.h"

static const char *mail_flags[] =
{
   "\\Answered",
   "\\Deleted",
   "\\Draft",
   "\\Flagged",
   NULL,
   "\\Seen",
   NULL,
};

void
imap_func_message_write(Email_Operation *op, Email_Message *msg, const char *mbox, Email_Imap4_Mail_Flag flags)
{
   char buf[4096];
   unsigned int flag;

   op->opdata = email_message_serialize(msg);
   if (flags)
     {
        Eina_Bool first = EINA_TRUE;
        snprintf(buf, sizeof(buf), "APPEND %s (", mbox);
        email_imap_write(op->e, op, buf, 0);
        for (flag = 0; flag < EMAIL_IMAP4_MAIL_FLAG_ITERATE; flag++)
          if (flags & (1 << flag))
            {
               if (!first) email_write(op->e, " ", 1);
               if (mail_flags[flag])
                 {
                    first = EINA_FALSE;
                    email_write(op->e, mail_flags[flag], 0);
                 }
            }
        snprintf(buf, sizeof(buf), ") {%zu%s}" CRLF, ESBUFLEN(op->opdata), op->e->features.imap.LITERALPLUS ? "+" : "");
        email_write(op->e, buf, 0);
     }   
   else
     {
        snprintf(buf, sizeof(buf), "APPEND %s () {%zu%s}" CRLF, mbox, ESBUFLEN(op->opdata), op->e->features.imap.LITERALPLUS ? "+" : "");
        email_imap_write(op->e, op, buf, 0);
     }
   if (!op->e->features.imap.LITERALPLUS) return;
   // http://tools.ietf.org/html/rfc2088
   email_write(op->e, ESBUF(op->opdata), ESBUFLEN(op->opdata));
   email_write(op->e, CRLF, CRLFLEN);
   eina_strbuf_free(op->opdata);
   op->opdata = NULL;
}

Eina_Stringshare *
email_imap4_mbox_get(const Email *e)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!email_is_imap(e), NULL);
   return e->protocol.imap.mboxname;
}

void
email_imap4_mailboxinfo_free(Email_Imap4_Mailbox_Info *info)
{
   if (!info) return;
   if (info->expunge) eina_inarray_free(info->expunge);
   if (info->fetch) eina_inarray_free(info->fetch);
   free(info);
}

const Eina_Inarray *
email_imap4_namespaces_get(const Email *e, Email_Imap4_Namespace_Type type)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(type >= EMAIL_IMAP4_NAMESPACE_LAST, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!email_is_imap(e), NULL);
   return e->features.imap.namespaces[type];
}

Email_Operation *
email_imap4_lsub(Email *e, const char *reference, const char *mbox, Email_List_Cb cb, const void *data)
{
   char buf[4096];
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, EINA_FALSE);

   op = email_op_new(e, EMAIL_IMAP4_OP_LSUB, cb, data);
   snprintf(buf, sizeof(buf), "LSUB \"%s\" \"%s\"" CRLF, reference ?: "", mbox ?: "");
   if (!email_is_blocked(e))
     email_imap_write(e, op, buf, 0);
   else
     op->opdata = strdup(buf);
   return op;
}

Email_Operation *
email_imap4_list(Email *e, const char *reference, const char *mbox, Email_List_Cb cb, const void *data)
{
   char buf[4096];
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, EINA_FALSE);

   if ((!reference) && (mbox && (mbox[0] == '*') && (!mbox[1])))
     {
        ERR("Cowardly refusing to fetch all mailboxes; mbox should be '%%' here!");
        return EINA_FALSE;
     }
   op = email_op_new(e, EMAIL_IMAP4_OP_LIST, cb, data);
   snprintf(buf, sizeof(buf), "LIST \"%s\" \"%s\"" CRLF, reference ?: "", mbox ?: "");
   if (!email_is_blocked(e))
     email_imap_write(e, op, buf, 0);
   else
     op->opdata = strdup(buf);
   return op;
}

Email_Operation *
email_imap4_select(Email *e, const char *mbox, Email_Imap4_Mailbox_Info_Cb cb, const void *data)
{
   char buf[4096];
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(mbox, NULL);

   op = email_op_new(e, EMAIL_IMAP4_OP_SELECT, cb, data);
   snprintf(buf, sizeof(buf), "SELECT %s" CRLF, mbox);
   eina_stringshare_replace(&e->protocol.imap.mboxname, mbox);
   if (!email_is_blocked(e))
     email_imap_write(e, op, buf, 0);
   else
     op->opdata = strdup(buf);
   return op;
}

Email_Operation *
email_imap4_examine(Email *e, const char *mbox, Email_Imap4_Mailbox_Info_Cb cb, const void *data)
{
   char buf[4096];
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(mbox, NULL);

   op = email_op_new(e, EMAIL_IMAP4_OP_EXAMINE, cb, data);
   snprintf(buf, sizeof(buf), "EXAMINE %s" CRLF, mbox);
   eina_stringshare_replace(&e->protocol.imap.mboxname, mbox);
   if (!email_is_blocked(e))
     email_imap_write(e, op, buf, 0);
   else
     op->opdata = strdup(buf);
   return op;
}

Email_Operation *
email_imap4_expunge(Email *e, Email_Cb cb, const void *data)
{
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);

   op = email_op_new(e, EMAIL_IMAP4_OP_EXPUNGE, cb, data);
   if (!email_is_blocked(e))
     email_imap_write(e, op, EMAIL_IMAP4_EXPUNGE, sizeof(EMAIL_IMAP4_EXPUNGE) - 1);
   return op;
}

Email_Operation *
email_imap4_noop(Email *e)
{
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);

   op = email_op_new(e, EMAIL_IMAP4_OP_NOOP, NULL, NULL);
   if (!email_is_blocked(e))
     email_imap_write(e, op, EMAIL_IMAP4_NOOP, sizeof(EMAIL_IMAP4_NOOP) - 1);
   return op;
}

Email_Operation *
email_imap4_close(Email *e, Email_Cb cb, const void *data)
{
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);

   op = email_op_new(e, EMAIL_IMAP4_OP_CLOSE, cb, data);
   if (!email_is_blocked(e))
     email_imap_write(e, op, EMAIL_IMAP4_CLOSE, sizeof(EMAIL_IMAP4_CLOSE) - 1);
   return op;
}

Email_Operation *
email_imap4_create(Email *e, const char *mbox, Email_Cb cb, const void *data)
{
   char buf[4096];
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(mbox, NULL);

   op = email_op_new(e, EMAIL_IMAP4_OP_CREATE, cb, data);
   snprintf(buf, sizeof(buf), "CREATE %s" CRLF, mbox);
   if (!email_is_blocked(e))
     email_imap_write(e, op, buf, 0);
   else
     op->opdata = strdup(buf);
   return op;
}

Email_Operation *
email_imap4_delete(Email *e, const char *mbox, Email_Cb cb, const void *data)
{
   char buf[4096];
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(mbox, NULL);

   op = email_op_new(e, EMAIL_IMAP4_OP_DELETE, cb, data);
   snprintf(buf, sizeof(buf), "DELETE %s" CRLF, mbox);
   if (!email_is_blocked(e))
     email_imap_write(e, op, buf, 0);
   else
     op->opdata = strdup(buf);
   return op;
}

Email_Operation *
email_imap4_rename(Email *e, const char *mbox, const char *newmbox, Email_Cb cb, const void *data)
{
   char buf[4096];
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(mbox, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(newmbox, NULL);

   op = email_op_new(e, EMAIL_IMAP4_OP_RENAME, cb, data);
   snprintf(buf, sizeof(buf), "RENAME %s %s" CRLF, mbox, newmbox);
   if (!email_is_blocked(e))
     email_imap_write(e, op, buf, 0);
   else
     op->opdata = strdup(buf);
   return op;
}

Email_Operation *
email_imap4_subscribe(Email *e, const char *mbox, Email_Cb cb, const void *data)
{
   char buf[4096];
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(mbox, NULL);

   op = email_op_new(e, EMAIL_IMAP4_OP_SUBSCRIBE, cb, data);
   snprintf(buf, sizeof(buf), "SUBSCRIBE %s" CRLF, mbox);
   if (!email_is_blocked(e))
     email_imap_write(e, op, buf, 0);
   else
     op->opdata = strdup(buf);
   return op;
}

Email_Operation *
email_imap4_unsubscribe(Email *e, const char *mbox, Email_Cb cb, const void *data)
{
   char buf[4096];
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(mbox, NULL);

   op = email_op_new(e, EMAIL_IMAP4_OP_UNSUBSCRIBE, cb, data);
   snprintf(buf, sizeof(buf), "UNSUBSCRIBE %s" CRLF, mbox);
   if (!email_is_blocked(e))
     email_imap_write(e, op, buf, 0);
   else
     op->opdata = strdup(buf);
   return op;
}

Email_Operation *
email_imap4_append(Email *e, const char *mbox, Email_Message *msg, Email_Imap4_Mail_Flag flags, Email_Cb cb, const void *data)
{
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(mbox, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(msg->owner && (msg->owner != e), NULL);

   op = email_op_new(e, EMAIL_IMAP4_OP_APPEND, cb, data);
   op->allows_cont = 1;
   if (!e->features.imap.LITERALPLUS)
     email_operation_blocking_set(op); // APPEND requires continuation data without LITERAL+
   if (email_is_blocked(e))
     {

        msg->sending++;
        msg->owner = e;
        op->opdata = msg;
        op->mbox = strdup(mbox);
        op->flags = flags;
     }
   else
     imap_func_message_write(op, msg, mbox, flags);
   return op;
}
