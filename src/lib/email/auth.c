#include "email_private.h"

void
auth_cram_md5(Email *e, const unsigned char *data, size_t size)
{
   char *b64, md5sum[33];
   unsigned char *bin, digest[16];
   int bsize;
   Eina_Strbuf *sbuf;

   bin = email_base64_decode((const char*)data, size, &bsize);
   email_md5_hmac_encode(digest, (char*)bin, bsize, e->password, strlen(e->password));
   free(bin);
   email_md5_digest_to_str(digest, md5sum);
   sbuf = eina_strbuf_new();
   eina_strbuf_append_printf(sbuf, "%s %s", e->username, md5sum);
   b64 = email_base64_encode(eina_strbuf_string_get(sbuf), eina_strbuf_length_get(sbuf), &bsize);
   eina_strbuf_free(sbuf);
   ecore_con_server_send(e->svr, b64, bsize);
   ecore_con_server_send(e->svr, CRLF, CRLFLEN);
   free(b64);
}
