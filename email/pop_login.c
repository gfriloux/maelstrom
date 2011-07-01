#include "email_private.h"
#include "md5.h"

void
email_login_pop(Email *e, Ecore_Con_Event_Server_Data *ev)
{
   char *buf;
   size_t size;

   switch (e->state)
     {
      case EMAIL_STATE_SSL:
        if (!email_op_ok(ev->data, ev->size))
          {
             ERR("Could not create secure connection!");
             ecore_con_server_del(ev->server);
             return;
          }
        ecore_con_ssl_server_upgrade(e->svr, ECORE_CON_USE_MIXED);
        ecore_con_ssl_server_verify_basic(e->svr);
        e->flags = ECORE_CON_USE_MIXED;
        return;
      case EMAIL_STATE_INIT:
        if (!email_op_ok(ev->data, ev->size))
          {
             ERR("Not a POP3 server!");
             ecore_con_server_del(ev->server);
             return;
          }
        if (ev->size > 20)
          {
             const unsigned char *end;

             end = memrchr(ev->data + 3, '>', ev->size - 3);
             if (end)
               {
                  const unsigned char *start;

                  start = memrchr(ev->data + 3, '<', end - (unsigned char*)ev->data);
                  if (start)
                    {
                       e->features.pop_features.apop = EINA_TRUE;
                       e->features.pop_features.apop_str = eina_binbuf_new();
                       eina_binbuf_append_length(e->features.pop_features.apop_str, start, end - start + 1);
                    }
               }
          }
        if (e->secure && (!e->flags))
          {
             email_write(e, "STLS\r\n", sizeof("STLS\r\n") - 1);
             e->state++;
             return;
          }
        e->state = EMAIL_STATE_USER;
        ev = NULL;
      case EMAIL_STATE_USER:
        if (!ev)
          {
             unsigned char digest[16];
             char md5buf[33];

             if (!e->features.pop_features.apop)
               {
                  INF("Beginning AUTH PLAIN");
                  size = sizeof(char) * (sizeof("USER ") - 1 + sizeof("\r\n") - 1 + strlen(e->username)) + 1;
                  buf = alloca(size);
                  snprintf(buf, size, "USER %s\r\n", e->username);
                  email_write(e, buf, size - 1);
                  return;
               }
             INF("Beginning AUTH APOP");
             e->state++;
             eina_binbuf_append_length(e->features.pop_features.apop_str, (unsigned char*)e->password, strlen(e->password));

             md5_buffer((char*)eina_binbuf_string_get(e->features.pop_features.apop_str), eina_binbuf_length_get(e->features.pop_features.apop_str), digest);
             email_md5_digest_to_str(digest, md5buf);
             size = sizeof(char) * (sizeof("APOP ") - 1 + sizeof("\r\n") - 1 + strlen(e->username)) + sizeof(md5buf);
             buf = alloca(size);
             snprintf(buf, size, "APOP %s %s\r\n", e->username, md5buf);
             email_write(e, buf, size - 1);
             return;
          }
        if (!email_op_ok(ev->data, ev->size))
          {
             ERR("Username invalid!");
             ecore_con_server_del(e->svr);
             return;
          }
        size = sizeof(char) * (sizeof("PASS ") - 1 + sizeof("\r\n") - 1 + strlen(e->password)) + 1;
        buf = alloca(size);
        snprintf(buf, size, "PASS %s\r\n", e->password);
        DBG("Sending password");
        ecore_con_server_send(e->svr, buf, size - 1);
        e->state++;
        return;
      case EMAIL_STATE_PASS:
        if (!email_op_ok(ev->data, ev->size))
          {
             ERR("Credentials invalid!");
             ecore_con_server_del(e->svr);
             return;
          }
        INF("Logged in successfully!");
        e->state++;
        ecore_event_add(EMAIL_EVENT_CONNECTED, e, (Ecore_End_Cb)email_fake_free, NULL);
      default:
        break;
     }
}

Eina_Bool
email_quit_pop(Email *e, Ecore_Cb cb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(e->state != EMAIL_STATE_CONNECTED, EINA_FALSE);

   if (!e->current)
     {
        e->current = EMAIL_OP_QUIT;
        email_write(e, "QUIT\r\n", 6);
     }
   else
     e->ops = eina_list_append(e->ops, (uintptr_t*)EMAIL_OP_QUIT);
   e->cbs = eina_list_append(e->cbs, cb);
   return EINA_TRUE;
}
