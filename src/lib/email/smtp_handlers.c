#include "email_private.h"


static void
next_smtp(Email *e)
{
   Email_Operation *op;

   e->protocol.smtp.state = 0;
   e->protocol.smtp.internal_state = 0;

   op = eina_list_data_get(e->ops);
   if (op->optype == EMAIL_SMTP_OP_SEND)
     {
        Email_Message *msg = op->opdata;

        msg->sending--;
        if (!msg->sending)
          {
             msg->owner = NULL;
             if (msg->deleted) email_message_free(msg);
          }
        op = email_op_pop(e);
     }
   if (!op) return;
   switch (e->current)
     {
      case EMAIL_SMTP_OP_SEND:
        if (!send_smtp(e))
          {
             Email_Send_Cb cb = op->cb;
             if (cb && (!op->deleted)) cb(op, op->opdata, EINA_FALSE);
             next_smtp(e);
          }
        break;
      case EMAIL_POP_OP_QUIT:
        email_write(e, EMAIL_POP3_QUIT, sizeof(EMAIL_POP3_QUIT) - 1);
      default:
        break;
     }
}

Eina_Bool
send_smtp(Email *e)
{
   char *buf;
   size_t size;
   Email_Message *msg;
   Email_Operation *op;
   Email_Contact *ec;
   Eina_Strbuf *bbuf;

   e->current = EMAIL_SMTP_OP_SEND;
   op = eina_list_data_get(e->ops);
   msg = op->opdata;
   switch (e->protocol.smtp.state)
     {
      case EMAIL_SMTP_STATE_NONE:
        msg->sending++;
        e->protocol.smtp.state++;
      case EMAIL_SMTP_STATE_FROM:
        if ((!msg->from) && (!msg->sender))
          {
             char buf2[1024];

             /* Do not concat smtp domain name if its already in username */
             if (strstr(e->username, e->features.smtp.domain))
               snprintf(buf2, sizeof(buf2), "%s", e->username);
             else
               snprintf(buf2, sizeof(buf2), "%s@%s", e->username, e->features.smtp.domain);
             msg->sender = eina_list_append(msg->sender, email_contact_new(buf2));
          }
        ec = eina_list_data_get(msg->sender);
        size = sizeof(char) * (sizeof(EMAIL_SMTP_FROM) + strlen(ec->address)) - 2;
        buf = alloca(size);
        snprintf(buf, size, EMAIL_SMTP_FROM, ec->address);
        email_write(e, buf, size - 1);
        e->protocol.smtp.state++;
        e->protocol.smtp.internal_state = 0;
        break;
      case EMAIL_SMTP_STATE_TO:
        ec = eina_list_nth(msg->to, e->protocol.smtp.internal_state++);
        if (!ec)
          {
             e->protocol.smtp.state++;
             e->protocol.smtp.internal_state = 0;
             return send_smtp(e);
          }
        size = sizeof(char) * (sizeof(EMAIL_SMTP_TO) + strlen(ec->address)) - 2;
        buf = alloca(size);
        snprintf(buf, size, EMAIL_SMTP_TO, ec->address);
        email_write(e, buf, size - 1);
        break;
      case EMAIL_SMTP_STATE_DATA:
        email_write(e, EMAIL_SMTP_DATA, sizeof(EMAIL_SMTP_DATA) - 1);
        e->protocol.smtp.state++;
        e->protocol.smtp.internal_state = 0;
        break;
      default:
        bbuf = email_message_serialize(msg);
        e->protocol.smtp.state++;
        if (bbuf)
          {
             email_write(e, eina_strbuf_string_get(bbuf), eina_strbuf_length_get(bbuf));
             eina_strbuf_free(bbuf);
          }
        else
          return EINA_FALSE;
     }
   return EINA_TRUE;
}

Eina_Bool
upgrade_smtp(Email *e, int type EINA_UNUSED, Ecore_Con_Event_Server_Upgrade *ev)
{
   char *buf;
   size_t size;

   if (e != ecore_con_server_data_get(ev->server)) return ECORE_CALLBACK_PASS_ON;

   e->state++;
   e->secure = 1;
   size = sizeof(char) * (sizeof("EHLO \r\n") + strlen(e->features.smtp.domain));

   buf = alloca(size);
   snprintf(buf, size, "EHLO %s\r\n", e->features.smtp.domain);
   email_write(e, buf, size - 1);
   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
data_smtp(Email *e, int type EINA_UNUSED, Ecore_Con_Event_Server_Data *ev)
{
   char *recvbuf;
   Email_Operation *op;
   Email_Send_Cb cb;
   Email_Cb qcb;
   Eina_Bool tofree = EINA_TRUE;

   if (e != ecore_con_server_data_get(ev->server))
     {
        DBG("Event mismatch");
        return ECORE_CALLBACK_PASS_ON;
     }

   if (eina_log_domain_level_check(email_log_dom, EINA_LOG_LEVEL_DBG))
     {
        recvbuf = alloca(ev->size + 1);
        memcpy(recvbuf, ev->data, ev->size);
        recvbuf[ev->size] = 0;
        DBG("Receiving %i bytes:\n%s", ev->size, recvbuf);
     }

   if (e->state < EMAIL_STATE_CONNECTED)
     {
        email_login_smtp(e, ev);
        return ECORE_CALLBACK_RENEW;
     }
   if (!e->current) return ECORE_CALLBACK_RENEW;

   op = eina_list_data_get(e->ops);
   cb = op->cb;
   qcb = op->cb;
   if (e->current == EMAIL_POP_OP_QUIT)
     {
        Eina_Bool success = EINA_FALSE;
        if ((ev->size < 3) || (memcmp(ev->data, "221", 3)))
          ERR("Could not QUIT properly!");
        else
          {
             ecore_con_server_del(e->svr);
             e->svr = NULL;
             success = EINA_TRUE;
          }
        if (qcb && (!op->deleted)) qcb(op, success);
        return ECORE_CALLBACK_RENEW;
     }
   switch (e->protocol.smtp.state)
     {
      case EMAIL_SMTP_STATE_BODY:
        if ((ev->size < 3) || (memcmp(ev->data, "354", 3)))
          {
             if (cb && (!op->deleted)) tofree = !!cb(op, op->opdata, EINA_FALSE);
             if (tofree) email_message_free(op->opdata);
             next_smtp(e);
             return ECORE_CALLBACK_RENEW;
          }
        if (!send_smtp(e))
          {
             if (cb && (!op->deleted)) tofree = !!cb(op, op->opdata, EINA_FALSE);
             if (tofree) email_message_free(op->opdata);
             next_smtp(e);
          }
        break;
      default:
        if ((ev->size < 3) || (memcmp(ev->data, "250", 3)))
          {
             if (cb && (!op->deleted)) tofree = !!cb(op, op->opdata, EINA_FALSE);
             if (tofree) email_message_free(op->opdata);
             next_smtp(e);
          }
        else if (e->protocol.smtp.state > EMAIL_SMTP_STATE_BODY)
          {
             if (cb && (!op->deleted)) tofree = !!cb(op, op->opdata, EINA_TRUE);
             if (tofree) email_message_free(op->opdata);
             next_smtp(e);
          }
        else
          {
             if (!send_smtp(e))
               {
             if (cb && (!op->deleted)) tofree = !!cb(op, op->opdata, EINA_FALSE);
             if (tofree) email_message_free(op->opdata);
                  next_smtp(e);
               }
          }
     }

   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
error_smtp(Email *e EINA_UNUSED, int type EINA_UNUSED, Ecore_Con_Event_Server_Error *ev EINA_UNUSED)
{
   ERR("Error");
   return ECORE_CALLBACK_RENEW;
}
