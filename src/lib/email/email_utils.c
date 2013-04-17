#include "email_private.h"
#include "cencode.h"
#include "cdecode.h"
#include "md5.h"

static const char *const encoding_strings[] =
{
   [EMAIL_MESSAGE_PART_ENCODING_7BIT] = "7BIT",
   [EMAIL_MESSAGE_PART_ENCODING_8BIT] = "8BIT",
   [EMAIL_MESSAGE_PART_ENCODING_BINARY] = "BINARY",
   [EMAIL_MESSAGE_PART_ENCODING_BASE64] = "BASE64",
   [EMAIL_MESSAGE_PART_ENCODING_QUOTED_PRINTABLE] = "QUOTED-PRINTABLE",
};

char *
email_base64_encode(const unsigned char *string, size_t len, size_t *size)
{
   base64_encodestate s;
   char *ret = NULL;
   size_t retlen[2];

   if ((len < 1) || (!string)) return NULL;

   if (!(ret = malloc(sizeof(char) * ((((len + 2) - ((size_t)(len + 2) % 3)) / 3) * 4) + 4)))
     return NULL;
   base64_init_encodestate(&s);
   retlen[0] = base64_encode_block((char*)string, len, ret, &s);
   retlen[1] = base64_encode_blockend(ret + retlen[0], &s);
   ret[retlen[0] + retlen[1]] = '\0';
   if (ret[retlen[0] + retlen[1] - 1] == '\n')
     {
        ret[retlen[0] + retlen[1] - 1] = '\0';
        if (size) *size = retlen[0] + retlen[1] - 1;
     }
   else if (size)
     *size = retlen[0] + retlen[1];

   return ret;
}

unsigned char *
email_base64_decode(const char *string, size_t len, size_t *size)
{
   base64_decodestate s;
   unsigned char *ret = NULL;
   size_t retlen;

   if ((len < 1) || (!string)) return NULL;

   if (!(ret = malloc(sizeof(char) * (size_t)((double)len / (double)(4 / 3)))))
     return NULL;
   base64_init_decodestate(&s);
   retlen = base64_decode_block((char*)string, len, ret, &s);
   if (size) *size = retlen;

   return ret;
}

void
email_md5_digest_to_str(unsigned char *digest, char *ret)
{
   char hexchars[] = "0123456789abcdef";
   unsigned int x, y;
   for (x = y = 0; x < 33; x++, y++)
     {
        ret[x++] = hexchars[digest[y] >> 4];
        ret[x] = hexchars[digest[y] & 15];
     }
   ret[32] = 0;
}

/* the following is based on public domain
 * hmac md5 code at http://tools.ietf.org/html/rfc2104#ref-MD5
 */
void
email_md5_hmac_encode(unsigned char *digest, const char *string, size_t size, const void *key, size_t ksize)
{
   md5_ctx ctx;
   unsigned char opad[65] = {0}, ipad[65] = {0};
   int i;
   /* if key is longer than 64 bytes reset it to key=MD5(key) */
   if (ksize > 64)
     {
        md5_buffer(key, ksize, digest);
        key = digest;
        ksize = 16;
     }

   /*
    * the HMAC_MD5 transform looks like:
    *
    * MD5(K XOR opad, MD5(K XOR ipad, text))
    *
    * where K is an n byte key
    * ipad is the byte 0x36 repeated 64 times
    * opad is the byte 0x5c repeated 64 times
    * and text is the data being protected
    */

   /* start out by storing key in pads */
   memcpy(ipad, key, ksize);
   memcpy(opad, key, ksize);

   /* XOR key with ipad and opad values */
   for (i = 0; i < 64; i++) {
           ipad[i] ^= 0x36;
           opad[i] ^= 0x5c;
   }
   /*
    * perform inner MD5
    */
   md5_init_ctx(&ctx);                  /* init context for 1st
                                         * pass */
   md5_process_bytes(ipad, 64, &ctx);      /* start with inner pad */
   md5_process_bytes(string, size, &ctx); /* then text of datagram */
   md5_finish_ctx(&ctx, digest);          /* finish up 1st pass */
   /*
    * perform outer MD5
    */
   md5_init_ctx(&ctx);                   /* init context for 2nd
                                         * pass */
   md5_process_bytes(opad, 64, &ctx);     /* start with outer pad */
   md5_process_bytes(digest, 16, &ctx);     /* then results of 1st
                                         * hash */
   md5_finish_ctx(&ctx, digest);          /* finish up 2nd pass */
}

///////////////////////////////////////////////////////////////////

void
email_util_inarray_stringshare_free(Eina_Inarray *arr)
{
   Eina_Stringshare *s;

   if (!arr) return;
   EINA_INARRAY_FOREACH(arr, s)
     eina_stringshare_del(s);
   eina_inarray_free(arr);
}

const char *
email_util_encoding_string_get(Email_Message_Part_Encoding encoding)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!encoding, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(encoding >= EMAIL_MESSAGE_PART_ENCODING_LAST, NULL);
   return encoding_strings[encoding];
}

Email_Message_Part_Encoding
email_util_encoding_type_get(const char *uppercase)
{
   unsigned int x;

   if (!uppercase) return EMAIL_MESSAGE_PART_ENCODING_NONE;
   if (!uppercase[0]) return EMAIL_MESSAGE_PART_ENCODING_NONE;

   for (x = EMAIL_MESSAGE_PART_ENCODING_7BIT; x < EMAIL_MESSAGE_PART_ENCODING_LAST; x++)
     if (!strcmp(encoding_strings[x], uppercase)) break;
   return x;
}
