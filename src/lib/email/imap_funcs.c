#include "email_private.h"

static const char *fetch_body_types[] =
{
   [EMAIL_IMAP4_FETCH_BODY_TYPE_TEXT] = "TEXT",
   [EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER] = "HEADER",
   [EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER_FIELDS] = "HEADER.FIELDS",
   [EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER_FIELDS_NOT] = "HEADER.FIELDS.NOT",
   [EMAIL_IMAP4_FETCH_BODY_TYPE_MIME] = "MIME",
};

static const char *fetch_types[] =
{
   [EMAIL_IMAP4_FETCH_TYPE_ALL] = "ALL",
   [EMAIL_IMAP4_FETCH_TYPE_FULL] = "FULL",
   [EMAIL_IMAP4_FETCH_TYPE_FAST] = "FAST",
   [EMAIL_IMAP4_FETCH_TYPE_ENVELOPE] = "ENVELOPE",
   [EMAIL_IMAP4_FETCH_TYPE_FLAGS] = "FLAGS",
   [EMAIL_IMAP4_FETCH_TYPE_INTERNALDATE] = "INTERNALDATE",
   [EMAIL_IMAP4_FETCH_TYPE_BODYSTRUCTURE] = "BODYSTRUCTURE",
   [EMAIL_IMAP4_FETCH_TYPE_UID] = "UID",
   [EMAIL_IMAP4_FETCH_TYPE_RFC822] = "RFC822",
   [EMAIL_IMAP4_FETCH_TYPE_BODY] = "BODY",
};

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
email_imap4_message_free(Email_Imap4_Message *im)
{
   if (!im) return;
   if (im->boundary) eina_strbuf_free(im->boundary);
   eina_inarray_free(im->part_nums);
   email_message_free(im->msg);
   eina_hash_free(im->params);
   free(im);
}

void
email_imap4_mailboxinfo_free(Email_Imap4_Mailbox_Info *info)
{
   if (!info) return;
   if (info->expunge) eina_inarray_free(info->expunge);
   if (info->fetch)
     {
        Email_Imap4_Message *im;

        EINA_INARRAY_FOREACH(info->fetch, im)
          email_message_free(im->msg);
     }
   eina_inarray_free(info->fetch);
   free(info);
}

Email_Operation_Status
email_operation_imap4_status_get(const Email_Operation *op)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(op, 0);
   return op->e->protocol.imap.status;
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
   EINA_SAFETY_ON_NULL_RETURN_VAL(e->protocol.imap.mboxname, NULL);

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
   EINA_SAFETY_ON_NULL_RETURN_VAL(e->protocol.imap.mboxname, NULL);

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

Email_Operation *
email_imap4_fetch(Email *e, unsigned int range[2], Email_Imap4_Fetch_Info **infos, unsigned int info_count, Email_Imap4_Fetch_Cb cb, const void *data)
{
   Email_Operation *op = NULL;
   Eina_Strbuf *sbuf;
   Eina_Bool body = EINA_FALSE;
   char buf[4096];
   unsigned int x;
   unsigned int num = info_count ?: UINT_MAX;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(range, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(infos, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e->protocol.imap.mboxname, NULL);

   if (range[0] == range[1])
     {
        if (range[0])
          snprintf(buf, sizeof(buf), "FETCH %u (", range[0]);
        else
          snprintf(buf, sizeof(buf), "FETCH 1:* (");
     }
   else
     {
        if (range[0] && range[1])
          snprintf(buf, sizeof(buf), "FETCH %u:%u (", range[0], range[1]);
        else if (range[0])
          snprintf(buf, sizeof(buf), "FETCH %u:* (", range[0]);
        else
          snprintf(buf, sizeof(buf), "FETCH *:%u (", range[1]);
     }
   sbuf = eina_strbuf_manage_new(strdup(buf));
   for (x = 0; x < num; x++)
     {
        if (!infos[x])
          {
             EINA_SAFETY_ON_TRUE_GOTO(!x, error);
             break;
          }
        switch (infos[x]->type)
          {
           case EMAIL_IMAP4_FETCH_TYPE_ALL:
           case EMAIL_IMAP4_FETCH_TYPE_FULL:
           case EMAIL_IMAP4_FETCH_TYPE_FAST:
             if (info_count != 1)
               {
                  ERR("EMAIL_IMAP4_FETCH_TYPE_%s CANNOT BE USED WITH OTHER FETCH TYPES!", fetch_types[infos[x]->type]);
                  goto error;
               }
             eina_strbuf_append(sbuf, fetch_types[infos[x]->type]);
             break;
           case EMAIL_IMAP4_FETCH_TYPE_BODYSTRUCTURE:
           case EMAIL_IMAP4_FETCH_TYPE_ENVELOPE:
           case EMAIL_IMAP4_FETCH_TYPE_FLAGS:
           case EMAIL_IMAP4_FETCH_TYPE_INTERNALDATE:
           case EMAIL_IMAP4_FETCH_TYPE_UID:
             if (x) eina_strbuf_append_char(sbuf, ' ');
             eina_strbuf_append(sbuf, fetch_types[infos[x]->type]);
             break;
           case EMAIL_IMAP4_FETCH_TYPE_RFC822:
             if (x) eina_strbuf_append_char(sbuf, ' ');
             eina_strbuf_append(sbuf, fetch_types[infos[x]->type]);
             {
                Eina_Bool used = EINA_FALSE;
                if (infos[x]->u.rfc822.header)
                  {
                     eina_strbuf_append(sbuf, ".HEADER");
                     used = EINA_TRUE;
                  }
                if (infos[x]->u.rfc822.size)
                  {
                     if (used)
                       {
                          eina_strbuf_append_char(sbuf, ' ');
                          eina_strbuf_append(sbuf, fetch_types[infos[x]->type]);
                       }
                     eina_strbuf_append(sbuf, ".SIZE");
                     used = EINA_TRUE;
                  }
                if (infos[x]->u.rfc822.text)
                  {
                     if (used)
                       {
                          eina_strbuf_append_char(sbuf, ' ');
                          eina_strbuf_append(sbuf, fetch_types[infos[x]->type]);
                       }
                     eina_strbuf_append(sbuf, ".TEXT");
                  }
             }
             break;
           case EMAIL_IMAP4_FETCH_TYPE_BODY:
             if (body)
               {
                  ERR("ONLY ONE BODY TYPE ALLOWED PER FETCH!");
                  goto error;
               }
             body = EINA_TRUE;
             if (x) eina_strbuf_append_char(sbuf, ' ');
             eina_strbuf_append(sbuf, fetch_types[infos[x]->type]);
             if (infos[x]->u.body.peek) eina_strbuf_append(sbuf, ".PEEK");
             {
                Eina_Bool need_brace = EINA_TRUE;
                unsigned int p;
                unsigned int pnum = infos[x]->u.body.part_count ?: UINT_MAX;

                if (infos[x]->u.body.parts)
                  {
                     for (p = 0; p < pnum; p++)
                       {
                          if (!infos[x]->u.body.parts[p])
                            {
                               pnum = p;
                               break;
                            }
                          if (need_brace) need_brace = !eina_strbuf_append_char(sbuf, '[');
                          else eina_strbuf_append_char(sbuf, '.');
                          eina_strbuf_append_printf(sbuf, "%u", infos[x]->u.body.parts[p]);
                       }
                  }
                else
                  pnum = 0;
                switch (infos[x]->u.body.type)
                  {
                   case EMAIL_IMAP4_FETCH_BODY_TYPE_NONE: break;
                   case EMAIL_IMAP4_FETCH_BODY_TYPE_MIME:
                     if (!pnum)
                       {
                          ERR("MUST HAVE PART NUMBERS TO USE MIME BODY FETCHING!");
                          goto error;
                       }
                   case EMAIL_IMAP4_FETCH_BODY_TYPE_TEXT:
                   case EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER:
                   case EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER_FIELDS:
                   case EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER_FIELDS_NOT:
                     if (need_brace) need_brace = !eina_strbuf_append_char(sbuf, '[');
                     else eina_strbuf_append_char(sbuf, '.');
                     eina_strbuf_append(sbuf, fetch_body_types[infos[x]->u.body.type]);
                     if ((infos[x]->u.body.type != EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER_FIELDS_NOT) &&
                         (infos[x]->u.body.type != EMAIL_IMAP4_FETCH_BODY_TYPE_HEADER_FIELDS))
                       break;
                     if (!infos[x]->u.body.fields_count)
                       {
                          ERR("MUST HAVE FIELD NAMES IN ORDER TO FETCH THEM!");
                          goto error;
                       }
                     eina_strbuf_append(sbuf, " (");
                     {
                        Eina_Bool need_space = EINA_FALSE;
                        unsigned int fnum = infos[x]->u.body.fields_count ?: UINT_MAX;

                        for (p = 0; p < fnum; p++)
                          {
                             if (!infos[x]->u.body.fields[p]) break;
                             if (need_space) eina_strbuf_append_char(sbuf, ' ');
                             need_space = eina_strbuf_append(sbuf, infos[x]->u.body.fields[p]);
                          }
                     }
                     eina_strbuf_append_char(sbuf, ')');
                     break;
                   default:
                     ERR("UNKNOWN FETCH BODY TYPE!");
                     goto error;
                  }
                if (!need_brace) eina_strbuf_append_char(sbuf, ']');
                if (infos[x]->u.body.range[1] > infos[x]->u.body.range[0])
                  eina_strbuf_append_printf(sbuf, "<%zu.%zu>", infos[x]->u.body.range[0], infos[x]->u.body.range[1]);
             }
             break;
           default:
             ERR("UNKNOWN FETCH TYPE! IGNORING!");
             continue;
          }
     }
   eina_strbuf_append(sbuf, ")\r\n");
   op = email_op_new(e, EMAIL_IMAP4_OP_FETCH, cb, data);
   if (email_is_blocked(e))
     op->opdata = eina_strbuf_string_steal(sbuf);
   else
     email_imap_write(e, op, ESBUF(sbuf), ESBUFLEN(sbuf));
error:
   eina_strbuf_free(sbuf);
   return op;
}
