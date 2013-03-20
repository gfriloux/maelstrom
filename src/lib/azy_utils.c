/*
 * Copyright 2010, 2011, 2012 Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "cencode.h"
#include "cdecode.h"
#include "azy_private.h"

#ifdef _WIN32
# ifdef HAVE_RPCDCE_H
#  include <Rpcdce.h>
#endif
#endif

/* length of a uuid */
#define UUID_LEN 36
/**
 * @defgroup Azy_Utils Utilities
 * @brief Functions which provide utility
 * @{
 */
/**
 * @brief Base64 encode a string of known length
 * @param string The string to encode
 * @param len The length of the string
 * @return Allocated base64 encoded string or NULL on error
 * This calls base64 encode functions to encode @p string, allocating
 * memory for the encoded string.
 */
char *
azy_base64_encode(const char *string,
                  double      len)
{
   base64_encodestate s;
   char *ret = NULL;
   size_t retlen[2];

   if ((len < 1) || (!string)) return NULL;

   if (!(ret = malloc(sizeof(char) * ((((len + 2) - ((size_t)(len + 2) % 3)) / 3) * 4) + 4)))
     return NULL;
   base64_init_encodestate(&s);
   retlen[0] = base64_encode_block(string, len, ret, &s);
   retlen[1] = base64_encode_blockend(ret + retlen[0], &s);
   ret[retlen[0] + retlen[1]] = '\0';

   return ret;
}

/**
 * @brief Base64 decode a string of known length
 * @param string The string to decode
 * @param len The length of the string
 * @return Allocated decoded string or NULL on error
 * This calls base64 decode functions to decode @p string, allocating
 * memory for the decoded string.
 */
char *
azy_base64_decode(const char *string,
                  size_t         len)
{
   base64_decodestate s;
   unsigned char *ret = NULL;
   size_t retlen;

   if ((len < 1) || (!string)) return NULL;

   if (!(ret = malloc(sizeof(char) * (size_t)((double)len / (double)(4 / 3)) + 1)))
     return NULL;
   base64_init_decodestate(&s);
   retlen = base64_decode_block(string, len, ret, &s);
   ret[retlen] = '\0';

   return (char*)ret;
}

/**
 * @brief Find a string of known length in a larger string of known length
 * @param big The large string
 * @param small The string to find
 * @param big_len The length of @p big
 * @param small_len The length of @p small
 * @return Pointer to the first occurrence of @p small, or NULL on error/failure
 * This can be considered strnstr, a utility function for finding a bounded string
 * in another bounded string.  It compares using unsigned char, however, so non-ascii
 * data can be found as well.
 */
unsigned char *
azy_memstr(const unsigned char *big,
           const unsigned char *small,
           size_t               big_len,
           size_t               small_len)
{
   unsigned char *x = (unsigned char *)big;

   if ((!big) || (!small) || (big_len < 1) || (small_len < 1) || (big_len < small_len))
     return NULL;

   for (; big_len >= small_len; x++, big_len--)
     if (!memcmp(x, small, small_len))
       return x;
   return NULL;
}

/**
 * @brief Read a UUID from /proc/sys/kernel/random/uuid and stringshare it
 * @return The stringshared uuid or NULL on error
 * This function is used to return a stringshared random UUID.  UUIDS are
 * Universally Unique IDentifiers, strings of 36 characters such as:
 * 550e8400-e29b-41d4-a716-446655440000
 */
const char *
azy_uuid_new(void)
{
   const char *ret = NULL;

#ifdef _WIN32
# ifdef HAVE_RPCDCE_H
   UUID u;
   RPC_CSTR buf;

   switch (UuidCreateSequential(&u))
     {
      case RPC_S_OK:
      case RPC_S_UUID_LOCAL_ONLY:
      case RPC_S_UUID_NO_ADDRESS:
        if (UuidToString(&u, &buf)) return NULL;

        ret = eina_stringshare_add(buf);
        RpcStringFree(buf);
      default:
        break;
     }
# endif
#endif

#ifdef __linux__
   char uuid[UUID_LEN + 1];
   FILE *f;
   if (!(f = fopen("/proc/sys/kernel/random/uuid", "r")))
     return NULL;

   if (fgets(uuid, UUID_LEN + 1, f))
     ret = eina_stringshare_add_length(uuid, UUID_LEN);

   fclose(f);
#endif
   return ret;
}

/**
 * @brief Parse a content-type string into an #Azy_Net_Transport
 * @param content_type The text string representing the content-type
 * @return The matching #Azy_Net_Transport, or AZY_NET_TRANSPORT_UNKNOWN on failure
 */
Azy_Net_Transport
azy_transport_get(const char *content_type)
{
   const char *c = NULL;;
   DBG("(content_type='%s')", content_type);
   if (!content_type)
     return AZY_NET_TRANSPORT_TEXT;

   if (!strncasecmp(content_type, "text/", 5))
     c = content_type + 5;
   else if (!strncasecmp(content_type, "application/", 12))
     c = content_type + 12;

   if (!c) return AZY_NET_TRANSPORT_UNKNOWN;

   if (!strncasecmp(c, "xml", 3))
     return AZY_NET_TRANSPORT_XML;

   if (c[0] == 'j')
     {
        if (!strncasecmp(c + 1, "son", 3))
          return AZY_NET_TRANSPORT_JSON;
        if (!strncasecmp(c + 1, "avascript", 9))
          return AZY_NET_TRANSPORT_JAVASCRIPT;
        return AZY_NET_TRANSPORT_UNKNOWN;
     }

   if (!strncasecmp(c, "eet", 3))
     return AZY_NET_TRANSPORT_EET;

   if (!strncasecmp(c, "plain", 5))
     return AZY_NET_TRANSPORT_TEXT;

   if (!strncasecmp(c, "html", 4))
     return AZY_NET_TRANSPORT_HTML;

   if (!strncasecmp(c, "atom+xml", 8))
     return AZY_NET_TRANSPORT_ATOM;

   return AZY_NET_TRANSPORT_UNKNOWN;
}

/** @} */
