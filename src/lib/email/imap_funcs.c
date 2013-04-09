#include "email_private.h"

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
   if (!email_is_blocked(e))
     email_imap_write(e, op, buf, 0);
   else
     op->opdata = strdup(buf);
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
