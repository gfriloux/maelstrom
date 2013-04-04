#include "email_private.h"

Eina_Bool
email_imap4_list(Email *e, const char *reference, const char *mbox, Email_List_Cb cb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, EINA_FALSE);

   if ((!reference) && (mbox && (mbox[0] == '*') && (!mbox[1])))
     {
        ERR("Cowardly refusing to fetch all mailboxes; mbox should be '%%' here!");
        return EINA_FALSE;
     }
   e->cbs = eina_list_append(e->cbs, cb);
   if (!e->ops)
     {
        e->current = EMAIL_IMAP_OP_LIST;
        email_imap_write(e, "LIST \"", 6);
        if (reference && reference[0])
          email_write(e, reference, strlen(reference));
        email_write(e, "\" \"", 3);
        if (mbox && mbox[0])
          email_write(e, mbox, strlen(mbox));
        email_write(e, "\"" CRLF, 1 + CRLFLEN);
     }
   e->ops = eina_list_append(e->ops, (uintptr_t*)EMAIL_IMAP_OP_LIST);
   return EINA_TRUE;
}
