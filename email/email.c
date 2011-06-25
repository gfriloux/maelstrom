#include "email_private.h"

static int email_init_count = 0;

int email_log_dom = -1;
int EMAIL_EVENT_CONNECTED = 0;

static Eina_Bool
upgrade_pop(Email *e, int type __UNUSED__, Ecore_Con_Event_Server_Upgrade *ev)
{
   if (e != ecore_con_server_data_get(ev->server)) return ECORE_CALLBACK_PASS_ON;

   e->state++;
   email_login_pop(e, NULL);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
data_pop(Email *e, int type __UNUSED__, Ecore_Con_Event_Server_Data *ev)
{
   char *recv;

   if (e != ecore_con_server_data_get(ev->server))
     {
        DBG("Event mismatch");
        return ECORE_CALLBACK_PASS_ON;
     }

   recv = alloca(ev->size + 1);
   memcpy(recv, ev->data, ev->size);
   recv[ev->size] = 0;
   DBG("Receiving %i bytes:\n%s", ev->size, recv);

   if (e->state < EMAIL_STATE_CONNECTED)
     {
        email_login_pop(e, ev);
        return ECORE_CALLBACK_RENEW;
     }

   if (!e->ops) return ECORE_CALLBACK_RENEW;
   
   switch ((uintptr_t)e->ops->data)
     {
      case EMAIL_OP_STAT:
      {
         Email_Stat_Cb cb;
         int num;
         size_t size;

         cb = e->stat_cbs->data;
         e->stat_cbs = eina_list_remove_list(e->stat_cbs, e->stat_cbs);
         e->ops = eina_list_remove_list(e->ops, e->ops);
         if ((!email_op_ok(ev->data, ev->size)) ||
             (sscanf(recv, "+OK %u %zu", &num, &size) != 2))
           {
              ERR("Error with STAT");
              cb(e, 0, 0);
              return ECORE_CALLBACK_RENEW;
           }
         cb(e, num, size);
      }
      default:
        break;
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
disc(Email *e, int type __UNUSED__, Ecore_Con_Event_Server_Del *ev)
{
   if (e != ecore_con_server_data_get(ev->server)) return ECORE_CALLBACK_PASS_ON;

   if (e->secure && e->flags && (!e->state))
     {
        /* ssl requested, not supported on base connection */
        e->svr = ecore_con_server_connect(ECORE_CON_REMOTE_NODELAY, e->addr, EMAIL_POP3_PORT, e);
        e->flags = 0;
        return ECORE_CALLBACK_RENEW;
     }
   INF("Disconnected");
   e->svr = NULL;
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
error_pop(Email *e __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Error *ev __UNUSED__)
{
   ERR("Error");
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
   eina_log_domain_level_set("email", EINA_LOG_LEVEL_DBG);
   eina_log_domain_level_set("ecore_con", EINA_LOG_LEVEL_DBG);
   EMAIL_EVENT_CONNECTED = ecore_event_type_new();
   return email_init_count;
}

Email *
email_new(const char *username, const char *password, void *data)
{
   Email *e;

   e = calloc(1, sizeof(Email));
   e->username = eina_stringshare_add(username);
   e->password = strdup(password);
   e->data = data;
   return e;
}

Eina_Bool
email_connect_pop3(Email *e, Eina_Bool secure, const char *addr)
{
   e->flags = secure ? ECORE_CON_USE_MIXED : 0;
   e->secure = !!secure;
   e->pop3 = EINA_TRUE;
   eina_stringshare_replace(&e->addr, addr);
   e->svr = ecore_con_server_connect(ECORE_CON_REMOTE_NODELAY | e->flags, addr, e->secure ? EMAIL_POP3S_PORT : EMAIL_POP3_PORT, e);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e->svr, EINA_FALSE);
   if (e->secure) ecore_con_ssl_server_verify_basic(e->svr);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, (Ecore_Event_Handler_Cb)disc, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, (Ecore_Event_Handler_Cb)data_pop, e);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ERROR, (Ecore_Event_Handler_Cb)error_pop, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_UPGRADE, (Ecore_Event_Handler_Cb)upgrade_pop, e);
   return EINA_TRUE;
}

void
email_cert_add(Email *e, const char *file)
{
   e->certs = eina_list_append(e->certs, strdup(file));
   ecore_con_ssl_server_verify(e->svr);
}
