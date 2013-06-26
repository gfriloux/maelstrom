#include "shotgun_private.h"
#include "cencode.h"
#include "cdecode.h"

char *
shotgun_base64_encode(const unsigned char *string, size_t len, size_t *size)
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
        *size = retlen[0] + retlen[1] - 2;
     }
   else
     *size = retlen[0] + retlen[1] - 1;

   return ret;
}

unsigned char *
shotgun_base64_decode(const char *string, size_t len, size_t *size)
{
   base64_decodestate s;
   unsigned char *ret = NULL;
   size_t retlen;

   if ((len < 1) || (!string)) return NULL;

   if (!(ret = malloc(sizeof(char) * (size_t)((double)len / (double)(4 / 3)))))
     return NULL;
   base64_init_decodestate(&s);
   retlen = base64_decode_block((char*)string, len, ret, &s);
   *size = retlen;

   return ret;
}


void
shotgun_strtohex(unsigned char *digest, size_t len, char *ret)
{
   char hexchars[] = "0123456789abcdef";
   size_t x, y;
   for (x = y = 0; x < len; x++, y++)
     {
        ret[x++] = hexchars[digest[y] >> 4];
        ret[x] = hexchars[digest[y] & 15];
     }
   ret[len] = 0;
}

/* the following is based on public domain
 * hmac md5 code at http://tools.ietf.org/html/rfc2104#ref-MD5
 */
void
shotgun_md5_hmac_encode(unsigned char *digest, const char *string, size_t size, const void *key, size_t ksize)
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

EAPI const char *
shotgun_sha1_buffer(const unsigned char *data, size_t len)
{
   return sha1_buffer(data, len);
}

const char *
shotgun_strtohtml(const char *s)
{
   unsigned int block = 64,
                i,
                j,
                k;
   char *s1;
   size_t s_len,
          s1_len,
          c_len;
   Eina_Bool found;
   struct _Html
   {
      const char s,
                 *h;
      unsigned int l;
   };
   struct _Html html[] = {
      { '<', "&lt;", 4 },
      { '>', "&gt;", 4 },
      { '\'', "&apos;", 6 },
      { '\"', "&quot;", 6 },
      { '&', "&amp;", 5 },
      { 0, NULL, 0 }
   };

   EINA_SAFETY_ON_NULL_RETURN_VAL(s, NULL);

   s_len = strlen(s);
   s1_len = s_len + block;
   c_len = 0;

   s1 = calloc(1, s1_len);

   for (i=j=0; i < s_len; i++,j++)
     {
        found = EINA_FALSE;

        for (k=0; html[k].s; k++)
          {
             if (s[i] != html[k].s)
               continue;

             if ((c_len + html[k].l) >= s1_len)
               {
                  s1 = realloc(s1, s1_len + block);
                  s1_len += block;
               }
             strcpy(s1 + j, html[k].h);
             found = EINA_TRUE;
             j += html[k].l - 1;
             c_len += html[k].l;
             break;
          }

        if (found)
          continue;

        if ((s1_len - c_len) < 2)
          {
             s1 = realloc(s1, s1_len + block);
             s1_len += block;
          }
        s1[j] = s[i];
        c_len++;
     }
   s1[c_len] = 0;
   return s1;
}
