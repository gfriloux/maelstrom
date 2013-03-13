/*
 * Copyright 2010, 2011, 2012, 2013 Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
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

#include "azy_private.h"
#include <ctype.h>
#include <math.h>

typedef enum
{
   AZY_COOKIE_FIELD_EXPIRES,
   AZY_COOKIE_FIELD_MAX_AGE,
   AZY_COOKIE_FIELD_DOMAIN,
   AZY_COOKIE_FIELD_PATH,
   AZY_COOKIE_FIELD_HTTPONLY,
   AZY_COOKIE_FIELD_SECURE,
   AZY_COOKIE_FIELD_EXTENSION
} Azy_Cookie_Field;

typedef enum
{
   AZY_DATE_FIELD_IGNORE = -1,
   AZY_DATE_FIELD_FAILURE = 0,
   AZY_DATE_FIELD_TIME = (1 << 0),
   AZY_DATE_FIELD_YEAR = (1 << 1),
   AZY_DATE_FIELD_MONTH = (1 << 2),
   AZY_DATE_FIELD_DAY = (1 << 3)
} Azy_Date_Field;

static const char *const months[] =
{
   "jan",
   "feb",
   "mar",
   "apr",
   "may",
   "jun",
   "jul",
   "aug",
   "sep",
   "oct",
   "nov",
   "dec"
};

static const char *const cookie_avs[] =
{
   [AZY_COOKIE_FIELD_EXPIRES] = "expires",
   [AZY_COOKIE_FIELD_MAX_AGE] = "max-age",
   [AZY_COOKIE_FIELD_DOMAIN] = "domain",
   [AZY_COOKIE_FIELD_PATH] = "path",
   [AZY_COOKIE_FIELD_HTTPONLY] = "httponly",
   [AZY_COOKIE_FIELD_SECURE] = "secure"
};

static unsigned int cookie_av_lens[] =
{
   [AZY_COOKIE_FIELD_EXPIRES] = sizeof("expires") - 1,
   [AZY_COOKIE_FIELD_MAX_AGE] = sizeof("max-age") - 1,
   [AZY_COOKIE_FIELD_DOMAIN] = sizeof("domain") - 1,
   [AZY_COOKIE_FIELD_PATH] = sizeof("path") - 1,
   [AZY_COOKIE_FIELD_HTTPONLY] = sizeof("httponly") - 1,
   [AZY_COOKIE_FIELD_SECURE] = sizeof("secure") - 1,
};

static Eina_Hash *_azy_cookies = NULL;

static inline int
_azy_net_cookie_sort_time_cb(time_t a, time_t b)
{
   if (a < b) return -1;
   if (a > b) return 1;
   return 0;
}

static int
_azy_net_cookie_sort_cb(Azy_Net_Cookie *a, Azy_Net_Cookie *b)
{
   size_t alen, blen;

   if ((!a->path) && (!b->path))
     return _azy_net_cookie_sort_time_cb(a->created, b->created);
   if (!a->path) return -1;
   if (!b->path) return 1;
   alen = strlen(a->path);
   blen = eina_strlen_bounded(b->path, alen);
   if (blen < alen) return 1;
   if (b->path[blen]) return -1;
   return 0;
}

static Azy_Cookie_Field
_azy_cookie_tokenize(const char *field)
{
   Azy_Cookie_Field x = AZY_COOKIE_FIELD_EXPIRES;

   for (; x < AZY_COOKIE_FIELD_EXTENSION; x++)
     if (!strncasecmp(field, cookie_avs[x], cookie_av_lens[x])) return x;
   return x;
}

static inline Eina_Bool
_azy_date_char_is_delimiter(char c)
{
   return (c == 0x09) ||
          ((c >= 0x20) && (c <= 0x2F)) ||
          ((c >= 0x3B) && (c <= 0x40)) ||
          ((c >= 0x5B) && (c <= 0x60)) ||
          ((c >= 0x7B) && (c <= 0x7E))
   ;
}

static inline Azy_Date_Field
_azy_date_field_tokenize(char *start, char **end, Azy_Date_Field exclude)
{
   char *p = start;

/*
 * I feel like I should make a separate file for parsing this

   http://tools.ietf.org/html/rfc6265
   cookie-date     = *delimiter date-token-list *delimiter
   date-token-list = date-token *( 1*delimiter date-token )
   date-token      = 1*non-delimiter

   delimiter       = %x09 / %x20-2F / %x3B-40 / %x5B-60 / %x7B-7E
   non-delimiter   = %x00-08 / %x0A-1F / DIGIT / ":" / ALPHA / %x7F-FF
   non-digit       = %x00-2F / %x3A-FF

   day-of-month    = 1*2DIGIT ( non-digit *OCTET )
   month           = ( "jan" / "feb" / "mar" / "apr" /
                       "may" / "jun" / "jul" / "aug" /
                       "sep" / "oct" / "nov" / "dec" ) *OCTET
   year            = 2*4DIGIT ( non-digit *OCTET )
   time            = hms-time ( non-digit *OCTET )
   hms-time        = time-field ":" time-field ":" time-field
   time-field      = 1*2DIGIT
 */
   while (!_azy_date_char_is_delimiter(p[0]))
     p++;
   *end = p;
   if (isdigit(start[0]))
     {
        switch (p - start)
          {
           case 1:
           case 2:
             if (!(exclude & AZY_DATE_FIELD_DAY))
               return AZY_DATE_FIELD_DAY;
             return AZY_DATE_FIELD_IGNORE;

           case 4:
             if (!(exclude & AZY_DATE_FIELD_YEAR))
               return AZY_DATE_FIELD_YEAR;
             return AZY_DATE_FIELD_IGNORE;

           case 8:
             if (start[2] == ':') // hmstime...hopefully
               {
                  char *pp = start;

                  for (p = start, pp++; isdigit(pp[0]); pp++) ;
                  if ((pp - p != 2) || (pp[0] != ':')) goto error;
                  for (pp++, p = pp; isdigit(pp[0]); pp++) ;
                  if ((pp - p != 2) || (pp[0] != ':')) goto error;
                  for (pp++, p = pp; isdigit(pp[0]); pp++) ;
                  if ((pp - p != 2) || (pp != *end)) goto error;
                  return AZY_DATE_FIELD_TIME;
               }

           default: goto error;
          }
     }
   if (!(exclude & AZY_DATE_FIELD_MONTH))
     {
        if (p - start == 3)
          {
             int x;

             /* lowercase here to make memcmp work later */
             for (x = 0; x < 3; x++)
               start[x] = tolower(start[x]);
             return AZY_DATE_FIELD_MONTH;
          }
     }
   return AZY_DATE_FIELD_IGNORE;
error:
   *end = strchr(start, ';');
   return AZY_DATE_FIELD_FAILURE;
}

static time_t
_azy_cookie_expires_parse(char *start, char **end)
{
   Azy_Date_Field found = 0;
   char *p;
   struct tm tm;
   int x;

   memset(&tm, 0, sizeof(tm));
   tm.tm_isdst = -1;
   while (start[0] != ';')
     {
        //eg. Wed, 11-Sep-2013 11:51:23 GMT
        switch (_azy_date_field_tokenize(start, &p, found))
          {
           case AZY_DATE_FIELD_IGNORE: break;

           case AZY_DATE_FIELD_TIME:
             x = strtol(start, NULL, 10);
             if ((x < 0) || (x > 23)) goto error;
             tm.tm_hour = x;
             start += 3;
             x = strtol(start, NULL, 10);
             if ((x < 0) || (x > 59)) goto error;
             tm.tm_min = x;
             start += 3;
             x = strtol(start, NULL, 10);
             if ((x < 0) || (x > 59)) goto error;
             tm.tm_sec = x;
             found |= AZY_DATE_FIELD_TIME;
             break;

           case AZY_DATE_FIELD_YEAR:
             x = strtol(start, NULL, 10);
             if (p - start == 2)
               {
                  if ((x >= 70) && (x <= 99)) x += 1900;
                  else if ((x >= 0) && (x <= 69))
                    x += 2000;
                  else return 0;
               }
             else
               {
                  if (x < 1601) goto error;
               }
             tm.tm_year = x - 1900;
             found |= AZY_DATE_FIELD_YEAR;
             break;

           case AZY_DATE_FIELD_MONTH:
             for (x = 0; x < 12; x++)
               if (!memcmp(start, months[x], 3))
                 {
                    tm.tm_mon = x;
                    found |= AZY_DATE_FIELD_MONTH;
                    break;
                 }
             break;

           case AZY_DATE_FIELD_DAY:
             x = strtol(start, NULL, 10);
             if ((x < 1) || (x > 31)) goto error;
             tm.tm_mday = x;
             found |= AZY_DATE_FIELD_DAY;
             break;

           case AZY_DATE_FIELD_FAILURE:
           default: goto error;
          }
        if (found ==
            (AZY_DATE_FIELD_TIME | AZY_DATE_FIELD_YEAR | AZY_DATE_FIELD_MONTH | AZY_DATE_FIELD_DAY)
            )
          break;
        start = p;
        while (_azy_date_char_is_delimiter(start[0]))
          start++;
     }
   if (found ==
       (AZY_DATE_FIELD_TIME | AZY_DATE_FIELD_YEAR | AZY_DATE_FIELD_MONTH | AZY_DATE_FIELD_DAY)
       )
     return mktime(&tm);
error:
   *end = strchr(start, ';');
   return 0;
}

static void
_azy_net_cookie_free(Azy_Net_Cookie *ck)
{
   eina_stringshare_del(ck->domain);
   eina_stringshare_del(ck->path);
   eina_stringshare_del(ck->name);
   eina_stringshare_del(ck->value);
   AZY_MAGIC_SET(ck, AZY_MAGIC_NONE);
   free(ck);
}

static inline void
_azy_net_cookie_hash_name(Azy_Net_Cookie *ck, char *buf)
{
   snprintf(buf, 4096, "%s;%s;%s", ck->name ? : "", ck->domain ? : "", ck->path ? : "");
}

static inline void
_azy_net_cookie_hash_del(Azy_Net_Cookie *ck, char *buf)
{
   if (!buf[0])
     _azy_net_cookie_hash_name(ck, buf);
   eina_hash_del_by_key(_azy_cookies, buf);
}

static inline void
_azy_net_cookie_update(Azy_Net_Cookie *ck, Azy_Net_Cookie *update)
{
   if (ck->value != update->value)
     {
        eina_stringshare_del(ck->value);
        ck->value = eina_stringshare_ref(update->value);
     }
   ck->expires = update->expires;
   ck->max_age = update->max_age;
   ck->flags = update->flags;
}

static inline Azy_Net_Cookie *
_azy_net_cookie_hash_add(Azy_Net_Cookie *ck)
{
   char buf[4096];
   Azy_Net_Cookie *ck2;

   if (ck->in_hash) return ck;
   _azy_net_cookie_hash_name(ck, buf);
   ck2 = eina_hash_find(_azy_cookies, buf);
   if (ck2)
     {
        _azy_net_cookie_update(ck, ck2);
        _azy_net_cookie_free(ck);
        return ck2;
     }
   ck->created = lround(ecore_time_unix_get());
   eina_hash_add(_azy_cookies, buf, ck);
   ck->in_hash = 1;
   return ck;
}

int
azy_net_cookie_init_(void)
{
   if (_azy_cookies) return 1;
   _azy_cookies = eina_hash_string_superfast_new(NULL);
   return !!_azy_cookies;
}

void
azy_net_cookie_shutdown_(void)
{
   if (!_azy_cookies) return;
   eina_hash_free_cb_set(_azy_cookies, (Eina_Free_Cb)_azy_net_cookie_free);
   eina_hash_free(_azy_cookies);
   _azy_cookies = NULL;
}

Azy_Net_Cookie *
azy_net_cookie_new(void)
{
   Azy_Net_Cookie *ck;

   ck = calloc(1, sizeof(Azy_Net_Cookie));
   EINA_SAFETY_ON_NULL_RETURN_VAL(ck, NULL);

   AZY_MAGIC_SET(ck, AZY_MAGIC_NET_COOKIE);
   return ck;
}

void
azy_net_cookie_free(Azy_Net_Cookie *ck)
{
   DBG("(ck=%p)", ck);

   if (!AZY_MAGIC_CHECK(ck, AZY_MAGIC_NET_COOKIE))
     {
        AZY_MAGIC_FAIL(ck, AZY_MAGIC_NET_COOKIE);
        return;
     }
   if (ck->refcount) ck->refcount--;
   if (ck->refcount) return;
   if (ck->in_hash)
     {
        char buf[4096];
        _azy_net_cookie_hash_name(ck, buf);
        eina_hash_del_by_key(_azy_cookies, buf);
     }
   _azy_net_cookie_free(ck);
}

void
azy_net_cookie_insert(Azy_Net *net, Azy_Net_Cookie *ck)
{
   DBG("(net=%p,ck=%p)", net, ck);
   if (!AZY_MAGIC_CHECK(ck, AZY_MAGIC_NET_COOKIE))
     {
        AZY_MAGIC_FAIL(ck, AZY_MAGIC_NET_COOKIE);
        return;
     }
   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return;
     }
   ck->refcount++;
   net->http.cookies = eina_list_sorted_insert(net->http.cookies, (Eina_Compare_Cb)_azy_net_cookie_sort_cb, ck);
}

void
azy_net_cookie_list_clear(Azy_Net *net)
{
   Azy_Net_Cookie *ck;

   DBG("(net=%p)", net);
   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return;
     }
   EINA_LIST_FREE(net->http.cookies, ck)
     azy_net_cookie_free(ck);
}

const Eina_List *
azy_net_cookie_list_get(const Azy_Net *net)
{
   DBG("(net=%p)", net);
   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return NULL;
     }
   return net->http.cookies;
}

Eina_List *
azy_net_cookie_list_steal(Azy_Net *net)
{
   Eina_List *ret;
   DBG("(net=%p)", net);
   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return NULL;
     }
   ret = net->http.cookies;
   net->http.cookies = NULL;
   return ret;
}

Eina_Stringshare *
azy_net_cookie_domain_get(const Azy_Net_Cookie *ck)
{
   DBG("(ck=%p)", ck);
   if (!AZY_MAGIC_CHECK(ck, AZY_MAGIC_NET_COOKIE))
     {
        AZY_MAGIC_FAIL(ck, AZY_MAGIC_NET_COOKIE);
        return NULL;
     }
   return ck->domain;
}

Eina_Stringshare *
azy_net_cookie_path_get(const Azy_Net_Cookie *ck)
{
   DBG("(ck=%p)", ck);
   if (!AZY_MAGIC_CHECK(ck, AZY_MAGIC_NET_COOKIE))
     {
        AZY_MAGIC_FAIL(ck, AZY_MAGIC_NET_COOKIE);
        return NULL;
     }
   return ck->path;
}

Azy_Net_Cookie_Flags
azy_net_cookie_flags_get(const Azy_Net_Cookie *ck)
{
   DBG("(ck=%p)", ck);
   if (!AZY_MAGIC_CHECK(ck, AZY_MAGIC_NET_COOKIE))
     {
        AZY_MAGIC_FAIL(ck, AZY_MAGIC_NET_COOKIE);
        return 0;
     }
   return ck->flags;
}

time_t
azy_net_cookie_expires(const Azy_Net_Cookie *ck)
{
   DBG("(ck=%p)", ck);
   if (!AZY_MAGIC_CHECK(ck, AZY_MAGIC_NET_COOKIE))
     {
        AZY_MAGIC_FAIL(ck, AZY_MAGIC_NET_COOKIE);
        return 0;
     }
   return ck->expires;
}

Azy_Net_Cookie *
azy_net_cookie_parse(char *txt)
{
   char *start, *p, *pp;
   Azy_Net_Cookie *ck;

   RPC_DBG("%s", txt);
   /* http://tools.ietf.org/html/rfc6265 */
   EINA_SAFETY_ON_TRUE_RETURN_VAL(isblank(txt[0]), NULL);
   start = p = strchr(txt, '=');
   /* invalid but permissible cookie */
   if (!p) return NULL;

   for (pp = p; (pp != txt) && (pp[-1] == ' '); pp--) ;

   /* invalid but also permissible cookie */
   if (pp == txt) return NULL;
   ck = azy_net_cookie_new();
   EINA_SAFETY_ON_NULL_RETURN_VAL(ck, NULL);
   ck->name = eina_stringshare_add_length(txt, pp - txt);
   start++;
   while (isspace(start[0]))
     start++;
   for (p = start; p[0] && (p[0] != ';'); p++)
     {
        char c = p[0];
        /* illegal cookie value characters */
        if (!(c == 0x21) &&
            !((c >= 0x21) && (c <= 0x2B)) &&
            !((c >= 0x2D) && (c <= 0x3A)) &&
            !((c >= 0x3C) && (c <= 0x5B)) &&
            !((c >= 0x5D) && (c <= 0x7E))
            )
          {
             ERR("COOKIE VALUE CONTAINS ILLEGAL CHARACTERS! SITE MUST BE SPANKED!!!");
             goto error;
          }
        if (c == ' ') break;
     }
   if (!p[0])
     {
        /* easy cookie. hooray! */
        ck->value = eina_stringshare_add(start);
        return ck;
     }
   if ((p[0] != ';') && (p[0] != ' '))
     {
        /* bad cookie! */
        ERR("INVALID COOKIE VALUE!");
        goto error;
     }
   if (p != start)
     ck->value = eina_stringshare_add_length(start, p - start);
   if (p[0] != ';') p = strchr(p, ';');

   while (p && p[0])
     {
        Azy_Cookie_Field field;

        start = p;
        while (isblank(start[0]))
          start++;
        if (start[0] != ';')
          {
             ERR("INVALID COOKIE FORMAT!");
             goto error;
          }
        start++;
        while (isblank(start[0]))
          start++;
        field = _azy_cookie_tokenize(start);
        switch (field)
          {
           case AZY_COOKIE_FIELD_EXTENSION:
             WARN("EXTENSION DATA! FIXME!");
             p = strchr(start, ';');
             continue;

           case AZY_COOKIE_FIELD_SECURE:
           case AZY_COOKIE_FIELD_HTTPONLY:
             ck->flags |= (field - AZY_COOKIE_FIELD_PATH);
             p = start + cookie_av_lens[field];
             continue;

           default:
             start += cookie_av_lens[field] + 1; // extra char for '='
          }
        while (isblank(start[0]))
          start++;
        if (start[0] == ';')
          {
             /* cookie field without value */
             start++;
             continue;
          }
        switch (field)
          {
             int t;

           case AZY_COOKIE_FIELD_EXPIRES:
             /* fml */
             p = strchr(start, ';');
             if (ck->max_age) continue;
             ck->expires = _azy_cookie_expires_parse(start, &p);
             ck->flags |= AZY_NET_COOKIE_PERSISTENT;
             break;

           case AZY_COOKIE_FIELD_MAX_AGE:
             p = strchr(start, ';');
             if ((start[0] != '-') && (!isdigit(start[0]))) continue;
             errno = 0;
             t = strtol(start, &p, 10);
             if (errno) continue;
             if (t < 1)
               /* this is supposed to be
                * "the earliest possible time that the client can represent"
                * not sure if that should be t=0 or what, so I'm using t=1
                * since that should have the same effect but allow us to
                * see that the expiry time has been set
                */
               ck->expires = 1;
             else
               ck->expires = lround(ecore_time_unix_get()) + t;
             ck->flags |= AZY_NET_COOKIE_PERSISTENT;
             ck->max_age = 1;
             continue;

           default:
             p = strchr(start, ';');
          }
        if (p) p[0] = 0;
        switch (field)
          {
           case AZY_COOKIE_FIELD_DOMAIN:
             /* FIXME: this will break for unicode and stuff */
             eina_str_tolower(&start);
             pp = start;
             while (pp[0] && (!isblank(pp[0])))
               pp++;
             eina_stringshare_replace_length(&ck->domain, start, pp - start);
             break;

           case AZY_COOKIE_FIELD_PATH:
             pp = start;
             while (pp[0] && (!isblank(pp[0])))
               pp++;
             eina_stringshare_replace_length(&ck->path, start, pp - start);
             break;

           default: break;
          }
        if (p) p[0] = ';';
     }
   if (!ck->expires) ck->expires = UINT_MAX;
   return _azy_net_cookie_hash_add(ck);
error:
   azy_net_cookie_free(ck);
   return NULL;
}

