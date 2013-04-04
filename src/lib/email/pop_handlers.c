#include "email_private.h"

static void
next_pop(Email *e)
{
   char buf[64];
   Email_Operation *op;

   if (e->buf) return;
   op = email_op_pop(e);
   if (!op) return;
   switch (e->current)
     {
      case EMAIL_POP_OP_STAT:
        email_write(e, EMAIL_POP3_STAT, sizeof(EMAIL_POP3_STAT) - 1);
        break;
      case EMAIL_POP_OP_LIST:
        email_write(e, EMAIL_POP3_LIST, sizeof(EMAIL_POP3_LIST) - 1);
        break;
      case EMAIL_POP_OP_RSET:
        email_write(e, EMAIL_POP3_RSET, sizeof(EMAIL_POP3_RSET) - 1);
        break;
      case EMAIL_POP_OP_DELE:
        snprintf(buf, sizeof(buf), EMAIL_POP3_DELE, (unsigned int)(uintptr_t)op->opdata);
        op->opdata = NULL;
        email_write(e, buf, strlen(buf));
        break;
      case EMAIL_POP_OP_RETR:
        snprintf(buf, sizeof(buf), EMAIL_POP3_RETR, (unsigned int)(uintptr_t)op->opdata);
        op->opdata = NULL;
        email_write(e, buf, strlen(buf));
        break;
      case EMAIL_POP_OP_QUIT:
        email_write(e, EMAIL_POP3_QUIT, sizeof(EMAIL_POP3_QUIT) - 1);
        break;
      default:
        break;
     }
}

Eina_Bool
upgrade_pop(Email *e, int type EINA_UNUSED, Ecore_Con_Event_Server_Upgrade *ev)
{
   if (e != ecore_con_server_data_get(ev->server)) return ECORE_CALLBACK_PASS_ON;

   e->state++;
   e->secure = 1;
   email_login_pop(e, NULL);
   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
data_pop(Email *e, int type EINA_UNUSED, Ecore_Con_Event_Server_Data *ev)
{
   char *recvbuf;

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
        email_login_pop(e, ev);
        return ECORE_CALLBACK_RENEW;
     }

   if (!e->current) return ECORE_CALLBACK_RENEW;

   switch (e->current)
     {
      case EMAIL_POP_OP_STAT:
        if (!email_pop3_stat_read(e, ev->data, ev->size)) return ECORE_CALLBACK_RENEW;
        break;
      case EMAIL_POP_OP_LIST:
        if (!email_pop3_list_read(e, ev)) return ECORE_CALLBACK_RENEW;
        break;
      case EMAIL_POP_OP_RETR:
        if (!email_pop3_retr_read(e, ev)) return ECORE_CALLBACK_RENEW;
        break;
      case EMAIL_POP_OP_DELE:
      case EMAIL_POP_OP_QUIT:
      {
         Email_Cb cb;
         Email_Operation *op;

         op = eina_list_data_get(e->ops);
         if (!email_op_pop_ok(ev->data, ev->size))
           {
              if (e->current == EMAIL_POP_OP_DELE) ERR("Error with DELE");
              else ERR("Error with QUIT");
           }
         else
           {
              if (e->current == EMAIL_POP_OP_DELE) INF("DELE successful");
              else INF("QUIT");
           }
         cb = op->cb;
         if (cb && (!op->deleted)) cb(op);
         if (e->current == EMAIL_POP_OP_QUIT) ecore_con_server_del(e->svr);
         break;
      }
      default:
        break;
     }
   next_pop(e);
   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
error_pop(Email *e EINA_UNUSED, int type EINA_UNUSED, Ecore_Con_Event_Server_Error *ev EINA_UNUSED)
{
   ERR("Error");
   return ECORE_CALLBACK_RENEW;
}
