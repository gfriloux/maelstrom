#include "email_private.h"

static int email_init_count = 0;

int email_log_dom = -1;
EAPI int EMAIL_EVENT_CONNECTED = 0;
EAPI int EMAIL_EVENT_DISCONNECTED = 0;

static Eina_Bool
disc(Email *e, int type __UNUSED__, Ecore_Con_Event_Server_Del *ev)
{
   if (e != ecore_con_server_data_get(ev->server)) return ECORE_CALLBACK_PASS_ON;

   if (e->secure && e->flags && (!e->state))
     {
        /* ssl requested, not supported on base connection */
        if (e->pop3)
          e->svr = ecore_con_server_connect(ECORE_CON_REMOTE_CORK, e->addr, EMAIL_POP3_PORT, e);
        else if (e->smtp)
          e->svr = ecore_con_server_connect(ECORE_CON_REMOTE_CORK, e->addr, EMAIL_SMTP_PORT, e);
        else if (e->imap)
          e->svr = ecore_con_server_connect(ECORE_CON_REMOTE_CORK, e->addr, EMAIL_SMTP_PORT, e);
        e->flags = 0;
        return ECORE_CALLBACK_RENEW;
     }
   INF("Disconnected");
   e->svr = NULL;
   e->state = 0;
   e->smtp_state = 0;
   e->internal_state = 0;
   if (e->buf) eina_binbuf_free(e->buf);
   e->buf = NULL;
   if (e->deleted) email_free(e);
   else ecore_event_add(EMAIL_EVENT_DISCONNECTED, e, email_fake_free, NULL);
   return ECORE_CALLBACK_RENEW;
}

void
email_fake_free(void *d __UNUSED__, void *e __UNUSED__)
{}

int
email_init(void)
{
   if (email_init_count++) return email_init_count;

   eina_init();
   ecore_init();
   ecore_con_init();

   /* real men don't accept failure as a possibility */
   email_log_dom = eina_log_domain_register("email", EINA_COLOR_YELLOW);
   EMAIL_EVENT_CONNECTED = ecore_event_type_new();
   EMAIL_EVENT_DISCONNECTED = ecore_event_type_new();
   return email_init_count;
}

void
email_shutdown(void)
{
   if (--email_init_count) return;

   ecore_con_shutdown();
   ecore_shutdown();

   eina_log_domain_unregister(email_log_dom);
   if (_email_contacts_hash) eina_hash_free(_email_contacts_hash);
   _email_contacts_hash = NULL;

   eina_shutdown();
}

Email *
email_new(const char *username, const char *password, void *data)
{
   Email *e;

   e = calloc(1, sizeof(Email));
   if (username) e->username = eina_stringshare_add(username);
   if (password) e->password = strdup(password);
   e->data = data;
   return e;
}

void
email_free(Email *e)
{
   char *str;
   Email_List_Item *it;

   if (!e) return;

   eina_stringshare_del(e->username);
   free(e->password);
   if (e->svr)
     {
        ecore_con_server_del(e->svr);
        e->deleted = EINA_TRUE;
        return;
     }
   eina_stringshare_del(e->addr);
   if (e->buf) eina_binbuf_free(e->buf);
   EINA_LIST_FREE(e->certs, str)
     free(str);
   eina_list_free(e->ops);
   eina_list_free(e->op_ids);
   eina_list_free(e->cbs);
   switch (e->current)
     {
      case EMAIL_OP_LIST:
        EINA_LIST_FREE(e->ev, it)
          free(it);
      default:
        break;
     }
   if (e->pop3 && e->features.pop_features.apop_str)
     eina_binbuf_free(e->features.pop_features.apop_str);
   ecore_event_handler_del(e->h_data);
   ecore_event_handler_del(e->h_del);
   ecore_event_handler_del(e->h_error);
   ecore_event_handler_del(e->h_upgrade);
   free(e);
}

Eina_Bool
email_connect_pop3(Email *e, Eina_Bool secure, const char *addr)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->imap || e->smtp, EINA_FALSE);
   e->pop3 = EINA_TRUE;
   e->flags = secure ? ECORE_CON_USE_MIXED : 0;
   e->secure = !!secure;
   eina_stringshare_replace(&e->addr, addr);
   e->svr = ecore_con_server_connect(ECORE_CON_REMOTE_CORK | e->flags, addr, e->secure ? EMAIL_POP3S_PORT : EMAIL_POP3_PORT, e);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e->svr, EINA_FALSE);
   if (e->secure) ecore_con_ssl_server_verify_basic(e->svr);
   e->h_del = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, (Ecore_Event_Handler_Cb)disc, e);
   e->h_data = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, (Ecore_Event_Handler_Cb)data_pop, e);
   e->h_error = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ERROR, (Ecore_Event_Handler_Cb)error_pop, e);
   e->h_upgrade = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_UPGRADE, (Ecore_Event_Handler_Cb)upgrade_pop, e);
   return EINA_TRUE;
}

Eina_Bool
email_connect_smtp(Email *e, Eina_Bool secure, const char *addr, const char *from_domain)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->imap || e->pop3, EINA_FALSE);
   e->smtp = EINA_TRUE;
   e->flags = secure ? ECORE_CON_USE_MIXED : 0;
   e->secure = !!secure;
   eina_stringshare_replace(&e->addr, addr);
   eina_stringshare_replace(&e->features.smtp_features.domain, from_domain);
   e->svr = ecore_con_server_connect(ECORE_CON_REMOTE_CORK | e->flags, addr, EMAIL_SMTP_PORT, e);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e->svr, EINA_FALSE);
   if (e->secure) ecore_con_ssl_server_verify_basic(e->svr);
   e->h_del = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, (Ecore_Event_Handler_Cb)disc, e);
   e->h_data = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, (Ecore_Event_Handler_Cb)data_smtp, e);
   e->h_error = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ERROR, (Ecore_Event_Handler_Cb)error_smtp, e);
   e->h_upgrade = ecore_event_handler_add(ECORE_CON_EVENT_SERVER_UPGRADE, (Ecore_Event_Handler_Cb)upgrade_smtp, e);
   return EINA_TRUE;
}

Eina_Bool
email_quit(Email *e, Email_Cb cb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, EINA_FALSE);

   if (!e->current)
     {
        e->current = EMAIL_OP_QUIT;
        email_write(e, EMAIL_QUIT, sizeof(EMAIL_QUIT) - 1);
     }
   e->ops = eina_list_append(e->ops, (uintptr_t*)EMAIL_OP_QUIT);
   e->cbs = eina_list_append(e->cbs, cb);
   return EINA_TRUE;
}

void
email_cert_add(Email *e, const char *file)
{
   e->certs = eina_list_append(e->certs, strdup(file));
   ecore_con_ssl_server_verify(e->svr);
}

void
email_data_set(Email *e, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(e);
   e->data = data;
}

void *
email_data_get(Email *e)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   return e->data;
}

const Eina_List *
email_queue_get(Email *e)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
   return e->ops;
}

Eina_Bool
email_op_cancel(Email *e, unsigned int op_number)
{
   Eina_List *l, *ids, *op_l;
   uintptr_t *op;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(op_number > eina_list_count(e->ops), EINA_FALSE);

   op_l = eina_list_nth_list(e->ops, op_number - 1);
   if (((uintptr_t)op_l->data != EMAIL_OP_DELE) && ((uintptr_t)op_l->data != EMAIL_OP_RETR))
     /* no op id to remove, so this is easy */
     goto out;
   ids = e->op_ids;
   EINA_LIST_FOREACH(e->ops, l, op)
     {
        if (l == op_l) break;
        if (((uintptr_t)op == EMAIL_OP_DELE) || ((uintptr_t)op == EMAIL_OP_RETR))
          ids = ids->next;
     }
   e->op_ids = eina_list_remove_list(e->op_ids, l);
out:
   e->ops = eina_list_remove_list(e->ops, op_l);
   e->cbs = eina_list_remove_list(e->cbs, eina_list_nth_list(e->cbs, op_number - 1));
   return EINA_TRUE;
}
