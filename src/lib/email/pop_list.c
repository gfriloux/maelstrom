#include "email_private.h"

Email_Operation *
email_pop3_stat(Email *e, Email_Stat_Cb cb, const void *data)
{
   Email_Operation *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);

   op = email_op_new(e, EMAIL_POP_OP_STAT, cb, data);
   if (!e->current)
     {
        e->current = EMAIL_POP_OP_STAT;
        email_write(e, "STAT\r\n", 6);
     }
   return op;
}

Email_Operation *
email_pop3_list(Email *e, Email_List_Cb cb, const void *data)
{
   Email_Operation *op;
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);

   op = email_op_new(e, EMAIL_POP_OP_LIST, cb, data);
   if (!e->current)
     {
        e->current = EMAIL_POP_OP_LIST;
        email_write(e, "LIST\r\n", 6);
     }
   return op;
}

Email_Operation *
email_pop3_rset(Email *e, Email_Cb cb, const void *data)
{
   Email_Operation *op;
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);

   op = email_op_new(e, EMAIL_POP_OP_RSET, cb, data);
   if (!e->current)
     {
        e->current = EMAIL_POP_OP_RSET;
        email_write(e, EMAIL_POP3_RSET, sizeof(EMAIL_POP3_RSET) - 1);
     }
   return op;
}

Email_Operation *
email_pop3_delete(Email *e, unsigned int id, Email_Cb cb, const void *data)
{
   char buf[64];
   Email_Operation *op;
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);

   op = email_op_new(e, EMAIL_POP_OP_DELE, cb, data);
   if (!e->current)
     {
        e->current = EMAIL_POP_OP_DELE;
        snprintf(buf, sizeof(buf), EMAIL_POP3_DELE, id);
        email_write(e, buf, strlen(buf));
     }
   else
     op->opdata = (uintptr_t*)(unsigned long)id;
   return op;
}

Email_Operation *
email_pop3_retrieve(Email *e, unsigned int id, Email_Retr_Cb cb, const void *data)
{
   char buf[64];
   Email_Operation *op;
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);

   op = email_op_new(e, EMAIL_POP_OP_RETR, cb, data);
   if (!e->current)
     {
        e->current = EMAIL_POP_OP_RETR;
        snprintf(buf, sizeof(buf), EMAIL_POP3_RETR, id);
        email_write(e, buf, strlen(buf));
     }
   else
     op->opdata = (uintptr_t*)(unsigned long)id;
   return op;
}

Eina_Bool
email_pop3_stat_read(Email *e, const unsigned char *recvbuf, size_t size)
{
   Email_Stat_Cb cb;
   Email_Operation *op;
   int num;
   size_t len;

   op = eina_list_data_get(e->ops);
   cb = op->cb;
   if ((!email_op_pop_ok((const unsigned char *)recvbuf, size)) ||
       (sscanf((char*)recvbuf, "+OK %u %zu", &num, &len) != 2))
     {
        ERR("Error with STAT");
        if (cb && (!op->deleted)) cb(op, 0, 0);
        return EINA_TRUE;
     }
   INF("STAT returned %u messages (%zu octets)", num, len);
   if (cb && (!op->deleted)) cb(op, num, len);
   return EINA_TRUE;
}

Eina_Bool
email_pop3_list_read(Email *e, Ecore_Con_Event_Server_Data *ev)
{
   Email_List_Cb cb;
   Email_Operation *op;
   Eina_List *list = NULL;
   Email_List_Item_Pop3 *it;
   const char *p, *n;
   const unsigned char *data;
   size_t size;

   op = eina_list_data_get(e->ops);
   cb = op->cb;
   if ((!e->buf) && (!email_op_pop_ok(ev->data, ev->size)))
     {
        ERR("Error with LIST");
        if (cb && (!op->deleted)) cb(op, NULL);
        return EINA_TRUE;
     }
   if (e->buf)
     {
        eina_binbuf_append_length(e->buf, ev->data, ev->size);
        data = eina_binbuf_string_get(e->buf);
        size = eina_binbuf_length_get(e->buf);
     }
   else
     {
        data = ev->data;
        size = ev->size;
     }
   for (n = (char*)memchr(data + 3, '\n', size - 3), size -= (n - (char*)data);
        n && (size > 1);
        p = n, n = (char*)memchr(p, '\n', size - 1), size -= (p - (char*)data))
      {
         it = calloc(1, sizeof(Email_List_Item_Pop3));
         if (sscanf(++n, "%u %zu", &it->id, &it->size) != 2)
           {
              free(it);
              break;
           }
         INF("Message %u: %zu octets", it->id, it->size);
         list = eina_list_append(list, it);
      }
   if (n[0] == '.')
     {
        Eina_Bool tofree = EINA_TRUE;
        INF("LIST returned %u messages", eina_list_count(list));
        if (cb && (!op->deleted)) tofree = !!cb(op, list);
        if (tofree)
          {
             EINA_LIST_FREE(list, it)
               free(it);
          }
        if (e->buf)
          {
             eina_binbuf_free(e->buf);
             e->buf = NULL;
          }
     }
   else if (!e->buf)
     {
        e->buf = eina_binbuf_new();
        eina_binbuf_append_length(e->buf, (const unsigned char*)(n), size - (n - (char*)data));
     }
   return EINA_TRUE;
}
