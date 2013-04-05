#include "email_private.h"

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
   op = email_op_new(e, EMAIL_IMAP_OP_LIST, cb, data);
   snprintf(buf, sizeof(buf), "LIST \"%s\" \"%s\"" CRLF, reference ?: "", mbox ?: "");
   if (!e->current)
     {
        e->current = EMAIL_IMAP_OP_LIST;
        email_imap_write(e, op, buf, 0);
        e->protocol.imap.current = op->opnum;
     }
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

   op = email_op_new(e, EMAIL_IMAP_OP_SELECT, cb, data);
   snprintf(buf, sizeof(buf), "SELECT %s" CRLF, mbox);
   if (!e->current)
     {
        e->current = EMAIL_IMAP_OP_SELECT;
        email_imap_write(e, op, buf, 0);
        e->protocol.imap.current = op->opnum;
     }
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

   op = email_op_new(e, EMAIL_IMAP_OP_EXAMINE, cb, data);
   snprintf(buf, sizeof(buf), "EXAMINE %s" CRLF, mbox);
   if (!e->current)
     {
        e->current = EMAIL_IMAP_OP_EXAMINE;
        email_imap_write(e, op, buf, 0);
        e->protocol.imap.current = op->opnum;
     }
   else
     op->opdata = strdup(buf);
   return op;
}
