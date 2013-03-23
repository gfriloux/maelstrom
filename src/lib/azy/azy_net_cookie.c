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
        ck2->last_used = lround(ecore_time_unix_get());
        return ck2;
     }
   ck->last_used = ck->created = lround(ecore_time_unix_get());
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
azy_net_cookie_send_list_generate(Eina_Strbuf *buf, const Eina_List *cookies)
{
   const Eina_List *l;
   Azy_Net_Cookie *ck;

   EINA_SAFETY_ON_NULL_RETURN(buf);
   EINA_SAFETY_ON_NULL_RETURN(cookies);
   eina_strbuf_append(buf, "Cookie: ");
   EINA_LIST_FOREACH(cookies, l, ck)
     {
        ck->last_used = lround(ecore_time_unix_get());
        eina_strbuf_append_printf(buf, "%s=%s%s", ck->name, ck->value ?: "", l->next ? ";" : "");
     }
   eina_strbuf_append(buf, "\r\n");
}

void
azy_net_cookie_set_list_generate(Eina_Strbuf *buf, const Eina_List *cookies)
{
   const Eina_List *l;
   Azy_Net_Cookie *ck;

   EINA_SAFETY_ON_NULL_RETURN(buf);
   EINA_SAFETY_ON_NULL_RETURN(cookies);
   EINA_LIST_FOREACH(cookies, l, ck)
     {
        eina_strbuf_append(buf, "Set-Cookie: ");
        eina_strbuf_append_printf(buf, "%s=%s", ck->name, ck->value ?: "");
        if (ck->expires)
          {
             if (ck->max_age)
               eina_strbuf_append_printf(buf, "; max-age=%ld", ck->expires - lround(ecore_time_unix_get()));
             else
               {
                  char b[128];
                  struct tm *tm;

                  tm = localtime(&ck->expires);
                  strftime(b, sizeof(b), "%a, %d-%b-%Y %H:%M:%S", tm);
                  eina_strbuf_append_printf(buf, "; expires=%s", b);
               }
          }
        if (ck->domain)
          eina_strbuf_append_printf(buf, "; domain=%s", ck->domain);
        if (ck->path)
          eina_strbuf_append_printf(buf, "; path=%s", ck->path);
        if (ck->flags & AZY_NET_COOKIE_HTTPONLY)
          eina_strbuf_append(buf, "; HttpOnly");
        if (ck->flags & AZY_NET_COOKIE_SECURE)
          eina_strbuf_append(buf, "; Secure");
        eina_strbuf_append(buf, "\r\n");
     }
}

void
azy_net_cookie_append(Azy_Net *net, Azy_Net_Cookie *ck, Eina_Bool send_type)
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
   if (send_type)
     net->http.send_cookies = eina_list_append(net->http.send_cookies, ck);
   else
     net->http.set_cookies = eina_list_append(net->http.set_cookies, ck);
}

void
azy_net_cookie_send_list_apply(Azy_Net *net, const Eina_List *cookies)
{
   const Eina_List *l;
   Azy_Net_Cookie *ck;

   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return;
     }
   EINA_SAFETY_ON_NULL_RETURN(cookies);
   EINA_LIST_FOREACH(cookies, l, ck)
     {
        if (ck->flags & AZY_NET_COOKIE_HOSTONLY)
          {
             if (ck->domain != net->http.req.host) continue;
          }
        else
          if (!azy_util_domain_match(net->http.req.host, ck->domain)) continue;
        ck->refcount++;
        net->http.send_cookies = eina_list_sorted_insert(net->http.send_cookies, (Eina_Compare_Cb)_azy_net_cookie_sort_cb, ck);
     }
}

void
azy_net_cookie_set_list_clear(Azy_Net *net)
{
   Azy_Net_Cookie *ck;

   DBG("(net=%p)", net);
   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return;
     }
   EINA_LIST_FREE(net->http.set_cookies, ck)
     azy_net_cookie_free(ck);
}

void
azy_net_cookie_send_list_clear(Azy_Net *net)
{
   Azy_Net_Cookie *ck;

   DBG("(net=%p)", net);
   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return;
     }
   EINA_LIST_FREE(net->http.send_cookies, ck)
     azy_net_cookie_free(ck);
}

const Eina_List *
azy_net_cookie_send_list_get(const Azy_Net *net)
{
   DBG("(net=%p)", net);
   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return NULL;
     }
   return net->http.send_cookies;
}

const Eina_List *
azy_net_cookie_set_list_get(const Azy_Net *net)
{
   DBG("(net=%p)", net);
   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return NULL;
     }
   return net->http.set_cookies;
}

Eina_List *
azy_net_cookie_send_list_steal(Azy_Net *net)
{
   Eina_List *ret;
   DBG("(net=%p)", net);
   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return NULL;
     }
   ret = net->http.send_cookies;
   net->http.send_cookies = NULL;
   return ret;
}

Eina_List *
azy_net_cookie_set_list_steal(Azy_Net *net)
{
   Eina_List *ret;
   DBG("(net=%p)", net);
   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return NULL;
     }
   ret = net->http.set_cookies;
   net->http.set_cookies = NULL;
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
azy_net_cookie_parse(const Azy_Net *net, char *txt)
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
             ck->expires = azy_util_date_parse(start, &p);
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
   if (net && (!ck->domain))
     {
        ck->domain = eina_stringshare_ref(net->http.req.host);
        ck->flags |= AZY_NET_COOKIE_HOSTONLY;
     }
   return _azy_net_cookie_hash_add(ck);
error:
   azy_net_cookie_free(ck);
   return NULL;
}

