#include "email_private.h"
#include "md5.h"

int
email_login_imap(Email *e, const unsigned char *data, size_t size, size_t *offset)
{
   switch (e->state)
     {
      case EMAIL_STATE_SSL:
        ecore_con_ssl_server_upgrade(e->svr, ECORE_CON_USE_MIXED);
        ecore_con_ssl_server_verify_basic(e->svr);
        e->flags = ECORE_CON_USE_MIXED;
        break;
      case EMAIL_STATE_INIT:
        if (e->upgrade && (!e->secure))
          {
             /* upgrade allowed, not using SSL yet */
             email_imap_write(e, NULL, EMAIL_STARTTLS, sizeof(EMAIL_STARTTLS) - 1);
             e->state++;
             return EMAIL_RETURN_DONE;
          }
        e->state = EMAIL_STATE_USER;
        if (!e->protocol.imap.caps)
          {
             e->current = EMAIL_IMAP_OP_CAPABILITY;
             email_imap_write(e, NULL, "CAPABILITY\r\n", sizeof("CAPABILITY\r\n") - 1);
             return EMAIL_RETURN_DONE;
          }
      case EMAIL_STATE_USER:
        if (!e->protocol.imap.caps)
          {
             CRI("WTFF");
             return EMAIL_RETURN_ERROR;
          }
        if (e->features.imap.LOGINDISABLED)
          {
             /* LOGINDISABLED means this server is useless to us, just disconnect and die */
             if (!e->secure)
               ERR("LOGINDISABLED hit after SSL usage blocked, probably should have used SSL!");
             ecore_con_server_del(e->svr);
             return EMAIL_RETURN_ERROR;
          }
        if (e->features.imap.AUTH_CRAM_MD5)
          {
             if (data)
               {
                  if (!memchr(data, '\r', size)) break;
                  auth_cram_md5(e, data, size);
                  imap_offset_update(offset, size);
                  e->state++;
                  break;
               }
             //called from imap_dispatch
             e->current = EMAIL_IMAP_OP_LOGIN;
             email_imap_write(e, NULL, "AUTHENTICATE CRAM-MD5\r\n", sizeof("AUTHENTICATE CRAM-MD5\r\n") - 1);
             break;
          }
        else if (e->features.imap.AUTH_PLAIN)
          {
             e->current = EMAIL_IMAP_OP_LOGIN;
             email_imap_write(e, NULL, "LOGIN ", 6);
             ecore_con_server_send(e->svr, e->username, strlen(e->username));
             ecore_con_server_send(e->svr, " ", 1);
             ecore_con_server_send(e->svr, e->password, strlen(e->password));
             ecore_con_server_send(e->svr, CRLF, CRLFLEN);
             e->state++;
          }
        break;
      case EMAIL_STATE_PASS:
        e->current = 0;
        switch (e->protocol.imap.status)
          {
           case EMAIL_IMAP_STATUS_OK: break;
           default:
             ecore_con_server_del(e->svr);
             /* FIXME: error event */
             ERR("Authentication rejected!");
             return EMAIL_RETURN_ERROR;
          }
        INF("Logged in successfully!");
        e->state++;
        imap_offset_update(offset, size);
        ecore_event_add(EMAIL_EVENT_CONNECTED, e, (Ecore_End_Cb)email_fake_free, NULL);
      default:
        break;
     }
   return EMAIL_RETURN_EAGAIN;
}
