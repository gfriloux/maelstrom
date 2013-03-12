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
#include <errno.h>

#define MAX_HEADER_SIZE 8092

static inline unsigned char *
_azy_events_skip_blank(const unsigned char *p, int64_t *len)
{
   if ((!p) || ((*len) < 1) || (!isspace(p[0]))) return (unsigned char*)p;
   do
     {
        p++, (*len)--;
     } while (((*len) > 0) && isspace(p[0]));
   return (unsigned char*)p;
}

static unsigned int
_azy_events_valid_header_name(const char  *start,
                              unsigned int len)
{
   while ((*start != ':') && (len--))
     {
        if ((!isalnum(*start)) && (*start != '-'))
          return 0;

        start++;
     }

   if (*start == ':') return len;
   return 0;
}

static int
_azy_events_valid_response(Azy_Net *net,
                           const unsigned char *header,
                           int len)
{
   const unsigned char *start;
   unsigned char *p;
   int code;

   if ((len < 16) || strncmp((char*)header, "HTTP/1.", sizeof("HTTP/1.") - 1)) return 0;
   start = header;
   start += 7; len -= 7;

   switch (start[0])
     {
      case '0':
        net->proto = AZY_NET_PROTOCOL_HTTP_1_0;
        break;
      case '1':
        net->proto = AZY_NET_PROTOCOL_HTTP_1_1;
        break;
      default:
        return 0;
     }
   start++, len--;

   while ((len > 1) && (start[0] == ' '))
     start++; len--;
   if ((len < 7) || (!isdigit(start[0]))) return 0;

   errno = 0;
   code = strtol((char*)start, (char**)&p, 10);
   if (errno || (code < 1) || (code > 999) || (p[0] != ' ')) return 0;
   switch (net->proto)
     {
      case AZY_NET_PROTOCOL_HTTP_1_0:
        INFO("HTTP/1.0 RESPONSE %"PRIi32, code);
        break;
      case AZY_NET_PROTOCOL_HTTP_1_1:
        INFO("HTTP/1.1 RESPONSE %"PRIi32, code);
        break;
      default: break;
     }
   net->http.res.http_code = code;
   len -= (p - start); start += (p - start);

   while ((len > 1) && (start[0] == ' '))
     start++; len--;
   if (len < 3) return 0;
   for (p = (unsigned char*)start; len; len--)
     {
        if ((p[0] == '\r') || (p[0] == '\n'))
          {
             if (p == start) return 0;
             net->http.res.http_msg = eina_stringshare_add_length((char*)start, p - start);
             break;
          }
        if (!isprint(p[0])) return 0;
        if (len > 1) p++;
     }
   if (!net->http.res.http_msg) return 0;
   net->type = AZY_NET_TYPE_RESPONSE;
   return (int)(p - header);
}

static int
_azy_events_valid_request(Azy_Net *net,
                          const unsigned char *header,
                          int len)
{
   char *p = NULL, *uri = NULL;
   const unsigned char *start, *path_start;
   int orig_len;

   DBG("(net=%p, header=%p, len=%i)", net, header, len);
   if (len < 16) return 0;

   start = header;;
   switch (start[0])
     {
      case 'H':
        if (strncmp((char*)start + 1, "EAD", 3)) return 0;
        return EINA_TRUE; /* FIXME: still unsupported */
      case 'G':
        if (strncmp((char*)start + 1, "ET", 2)) return 0;
        net->type = AZY_NET_TYPE_GET;
        start += 3; len -= 3;
        break;
      case 'P':
        if (!strncmp((char*)start + 1, "OST", 3))
          {
             net->type = AZY_NET_TYPE_POST;
             start += 4; len -= 4;
          }
        else if (!strncmp((char*)start + 1, "UT", 2))
          {
             net->type = AZY_NET_TYPE_PUT;
             start += 3; len -= 3;
          }
        else return 0;
        break;
      default:
        return 0;
     }
   if (start[0] != ' ') return 0;
   start++; len--;
   path_start = start;
   orig_len = len;
   for (; len; start++)
     {
        const unsigned char *end;
        for (end = start; len; len--)
          {
             switch (end[0])
               {
                case '\\':
                case ';':
                case '#':
                case '[':
                case ']':
                  /* reserved chars http://tools.ietf.org/html/rfc3986 */
                  return 0;
                case '%':
                  /* must be 3 char escape code + 8 char http version */
                  if (len < 11) return 0;
                  {
                     char code[3];
                     long codechar;

                     errno = 0;
                     code[0] = end[1];
                     code[1] = end[2];
                     code[2] = 0;
                     codechar = strtol(code, NULL, 16);
                     /* invalid escape code */
                     if (errno || (codechar < 32) || (codechar > 126)) return 0;
                     if (!uri)
                       {
                          uri = alloca(orig_len);
                          memcpy(uri, start, end - start);
                          p = uri + (end - start);
                       }
                     else
                       {
                          memcpy(p, start, end - start);
                          p += (end - start);
                       }
                     p[0] = (char)codechar;
                     p++;
                     if (codechar == '\\')
                       {
                          /* escape backslash */
                          p[0] = '\\';
                          p++;
                       }
                     /* new start */
                     len -= 3; start = end;
                     goto out;
                  }
                case '\r':
                case '\n':
                  {
                     unsigned int eo = end - header;
                     if (end - header < 14) return 0;
                     end--;
                     switch (end[0])
                       {
                        case '0':
                          net->proto = AZY_NET_PROTOCOL_HTTP_1_0;
                          break;
                        case '1':
                          net->proto = AZY_NET_PROTOCOL_HTTP_1_1;
                          break;
                        default:
                          return 0;
                       }
                     end -= 8;
                     if (strncmp((char*)end, " HTTP/1.", sizeof(" HTTP/1.") - 1)) return 0;
                     for (; (len < orig_len) && (end[0] == ' '); end--, len++);
                     if ((end - path_start < 0) || (end[0] == ' ')) return 0;
                     end++; /* copy up to the space */
                     if (uri)
                       {
                          memcpy(p, start, end - start);
                          net->http.req.http_path = eina_stringshare_add_length(uri, (p - uri) + (end - start));
                       }
                     else
                       net->http.req.http_path = eina_stringshare_add_length((char*)start, end - start);
                     INFO("Requested URI: '%s'", net->http.req.http_path);
                     return (int)eo;
                  }
                default:
                  break;
               }
             if (len > 1) end++;
          }
out:
        continue;
     }
   return 0;
}

static Eina_Bool
_azy_events_valid_header_value(const char  *name,
                               unsigned int len)
{
   while (len--)
     {
        if ((!isprint(*name)) && (!isspace(*name)))
          return EINA_FALSE;

        name++;
     }

   return EINA_TRUE;
}

int
azy_events_type_parse(Azy_Net *net, int type, const unsigned char *header, int64_t len)
{
   const unsigned char *start = NULL;
   size_t size;

   DBG("(net=%p, header=%p, len=%"PRIi64")", net, header, len);
   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return 0;
     }

   if (net->buffer)
     {
        unsigned char *buf_start;

        /* previous buffer */
        size = (eina_binbuf_length_get(net->buffer) + len > MAX_HEADER_SIZE) ? MAX_HEADER_SIZE : eina_binbuf_length_get(net->buffer) + len;
        buf_start = alloca(size);
        /* grab and combine buffers */
        if (header)
          {
             memcpy(buf_start, eina_binbuf_string_get(net->buffer), size);
             if (eina_binbuf_length_get(net->buffer) < size)
               memcpy(buf_start + eina_binbuf_length_get(net->buffer), header, size - eina_binbuf_length_get(net->buffer));
             len = size;
          }
        else
          {
             memcpy(buf_start, eina_binbuf_string_get(net->buffer), size);
             len = size;
          }

        start = buf_start;
        start = _azy_events_skip_blank(start, &len);
     }
   else
     {
        /* copy pointer */
        start = header;
        /* skip all spaces/newlines/etc and decrement len */
        start = _azy_events_skip_blank(start, &len);
     }

   if (!start) return 0;

   /* some clients are dumb and send leading cr/nl/etc */
   start = _azy_events_skip_blank(start, &len);

   if (type == ECORE_CON_EVENT_CLIENT_DATA)
     return _azy_events_valid_request(net, start, len);
   return _azy_events_valid_response(net, start, len);
}

static Azy_Net_Transfer_Encoding
_azy_events_transfer_encoding(const char *value)
{
   /* FIXME: chunk-extension???
    * http://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html#sec3.6.1
    */
   if (!strncasecmp(value, "chunked", sizeof("chunked") - 1)) return AZY_NET_TRANSFER_ENCODING_CHUNKED;
   return AZY_NET_TRANSFER_ENCODING_NONE;
}

static Eina_Strbuf *
_azy_events_separator_parse(const unsigned char *start, int64_t len, const unsigned char *r)
{
   const char *s = NULL;

   if (*r == '\r')
     {
        unsigned char *x;
        if ((x = memchr(start, '\n', len)))
          {
             if ((x - r) > 0)
               s = "\r\n";
             else
               { /* we currently have \n\r: b64 encoding can leave a trailing \n
                  * so we have to check for an extra \n
                  */
                   if ((x - r < 0) && ((unsigned int)(r + 1 - start) < len) && (r[1] == '\n')) /* \n\r\n */
                     {
                        if (((unsigned int)(r + 2 - start) < len) && (r[2] == '\r')) /* \n\r\n\r */
                          {
                             if (((unsigned int)(r + 3 - start) < len) && (r[3] == '\n'))
                               /* \n\r\n\r\n oh hey I'm gonna stop here before it gets too insane */
                               s = "\r\n";
                             else
                               s = "\n\r";
                          }
                        else
                          s = "\r\n";
                     }
                   else
                     s = "\n\r";
               }
          }
        else
          s = "\r";
     }
   else
     s = "\n";


   return eina_strbuf_manage_new(strdup(s));
}

static void
_azy_events_header_add(Azy_Net *net, const char *key, const char *value)
{
   INFO("Found header: key='%s'", key);
   INFO("Found header: value='%s'", value);
   if (!strcasecmp(key, "transfer-encoding"))
     {
        net->transfer_encoding = _azy_events_transfer_encoding(value);
        switch (net->transfer_encoding)
          {
           case AZY_NET_TRANSFER_ENCODING_CHUNKED:
             INFO("TRANSFER ENCODING: CHUNKED");
             break;
           default: break;
          }
        net->http.content_length = -1;
     }
   else if (!strcasecmp(key, "content-length"))
     {
        if (net->transfer_encoding)
          INFO("Ignoring content-length: transfer encoding is set :(");
        else
          net->http.content_length = strtol((const char *)value, NULL, 10);
     }
   else
     azy_net_header_set(net, key, value);
}

static unsigned char *
_azy_events_headers_parser(Azy_Net *net, unsigned char *start, int64_t *length, int line_len)
{
   unsigned char *r, *p;
   int64_t len;
   const char *s;
   unsigned int slen;

   if (!line_len) return start;

   r = start + line_len;
   len = *length;
   p = start;
   slen = eina_strbuf_length_get(net->separator);
   s = eina_strbuf_string_get(net->separator);
   while (len && r)
     {
        unsigned char *ptr, *semi = p;

        if (line_len > MAX_HEADER_SIZE)
          {
             WARN("Ignoring unreasonably large header starting with:\n %.32s\n", p);
             goto skip_header;
          }
        semi += (line_len - _azy_events_valid_header_name((const char *)p, line_len));
        if (semi == p) goto skip_header;

        ptr = semi + 1;
        while ((isspace(*ptr)) && (ptr - p < line_len))
          ptr++;

        if (_azy_events_valid_header_value((const char *)ptr, line_len - (ptr - p)))
          {
             p[semi - p] = 0;
             ptr[line_len - (ptr - p)] = 0;
             _azy_events_header_add(net, (char*)p, (char*)ptr);
          }

skip_header:
        len -= line_len + slen;
        if (len < slen)
          break;
        p = r + slen;
        /* double separator: STOP */
        if (!strncmp((char*)p, s, slen))
          {
             INFO("HEADERS PARSED!");
             net->headers_read = EINA_TRUE;
             break;
          }
        r = azy_memstr(p, (const unsigned char *)s, len, slen);
        if (r)
          line_len = r - p;
        /* FIXME: to be fully 1.1 compliant, lines without a colon
         * be filtered and checked to see if is a continuing header
         * from the previous line
         */
     }
   *length = len;
   return p;
}

Eina_Bool
azy_events_header_parse(Azy_Net       *net,
                        unsigned char *event_data,
                        size_t         event_len,
                        int            offset)
{
   unsigned char *r = NULL, *p = NULL, *start = NULL, *buf_start = NULL;
   unsigned char *data = (event_data) ? event_data + offset : NULL;
   int64_t len = (event_len) ? event_len - offset : 0;
   int64_t prev_size = 0;

   DBG("(net=%p, event_data=%p, len=%zu, offset=%i)", net, event_data, event_len, offset);
   if (!AZY_MAGIC_CHECK(net, AZY_MAGIC_NET))
     {
        AZY_MAGIC_FAIL(net, AZY_MAGIC_NET);
        return EINA_FALSE;
     }
   if (net->headers_read)
     return EINA_TRUE;
   EINA_SAFETY_ON_TRUE_RETURN_VAL((!net->buffer) && (!data), EINA_FALSE);

   if (net->buffer)
     {
        if (event_data && (azy_rpc_log_dom >= 0))
          {
             char buf[64];
             snprintf(buf, sizeof(buf), "STORED:\n<<<<<<<<<<<<<\n%%.%"PRIi64"s\n<<<<<<<<<<<<<", eina_binbuf_length_get(net->buffer));
             RPC_INFO(buf, eina_binbuf_string_get(net->buffer));
             snprintf(buf, sizeof(buf), "RECEIVED:\n<<<<<<<<<<<<<\n%%.%"PRIi64"s\n<<<<<<<<<<<<<", len - offset);
             RPC_INFO(buf, data);
          }
        /* previous buffer */
        /* alloca should be safe here because ecore_con reads at most 64k
         * and even if no headers were found previously, the entire
         * buffer would not be copied
         */
        buf_start = alloca(len + eina_binbuf_length_get(net->buffer) - offset);
        /* grab and combine buffers */
        if (event_data)
          {
             memcpy(buf_start, eina_binbuf_string_get(net->buffer) + offset, eina_binbuf_length_get(net->buffer) - offset);
             memcpy(buf_start + eina_binbuf_length_get(net->buffer), event_data, len);
          }
        else
          memcpy(buf_start, eina_binbuf_string_get(net->buffer) + offset, eina_binbuf_length_get(net->buffer) - offset);

        prev_size = eina_binbuf_length_get(net->buffer);
        len += prev_size - offset;
        eina_binbuf_free(net->buffer);
        net->buffer = NULL;
        start = buf_start;
        start = _azy_events_skip_blank(start, &len);
     }
   else
   /* only current buffer plus possible net->overflow */
     {
        if (azy_rpc_log_dom != -1)
          {
             char buf[64];
             snprintf(buf, sizeof(buf), "RECEIVED:\n<<<<<<<<<<<<<\n%%.%"PRIi64"s\n<<<<<<<<<<<<<", len - offset);
             RPC_INFO(buf, data);
          }
       /* copy pointer */
        start = data;
        /* skip all spaces/newlines/etc and decrement len */
        start = _azy_events_skip_blank(start, &len);
     }

   if ((!len) && (event_len - offset > 0)) /* only blanks were passed, assume http separator */
     {
        net->headers_read = EINA_TRUE;
        return EINA_TRUE;
     }
   /* apparently this can happen? */
   EINA_SAFETY_ON_NULL_RETURN_VAL(start, EINA_FALSE);
   /* find a header or append to buffer */
   if ((!(r = memchr(start, '\r', len)) && !(r = memchr(start, '\n', len)))) /* append to a buffer and use net->overflow */
     {
        if (!net->buffer)
          {
             net->buffer = eina_binbuf_new();
             net->progress = 0;
          }
        azy_events_recv_progress(net, start, len);
        return EINA_TRUE;
     }

   net->separator = _azy_events_separator_parse(start, len, r);

   p = _azy_events_headers_parser(net, start, &len, r - start);

   p = _azy_events_skip_blank(p, &len);

   if (!net->headers_read)
     return EINA_TRUE;

   if (!net->http.content_length) net->http.content_length = -1;
   if (len)
     {
        int64_t rlen;
        /* if we get here, we need to append to the buffers */

        if (net->http.content_length > 0)
          {
             if (len > net->http.content_length)
               {
                  rlen = net->http.content_length;
                  net->overflow = eina_binbuf_new();
                  eina_binbuf_append_length(net->overflow, p + rlen, len - rlen);
                  WARN("Extra content length of %"PRIi64"!", eina_binbuf_length_get(net->overflow));
#ifdef ISCOMFITOR
                if (azy_rpc_log_dom >= 0)
                  {
                     int64_t x;
                     unsigned char *buf;
                     RPC_INFO("OVERFLOW:\n<<<<<<<<<<<<<");
                     buf = eina_binbuf_string_get(net->overflow);
                     for (x = 0; x < eina_binbuf_length_get(net->overflow); x++)
                       putc(buf[x], stdout);
                     fflush(stdout);
                  }
#endif
               }
             else
               rlen = len;
          }
        else
          /* this shouldn't be possible unless someone is violating spec */
          rlen = len;

        INFO("Set recv size to %"PRIi64" (previous %"PRIi64")", rlen, prev_size);
        net->buffer = eina_binbuf_new();
        net->progress = 0;
        azy_events_recv_progress(net, p, rlen);
     }

   return EINA_TRUE;
}

Eina_Bool
azy_events_connection_kill(void       *conn,
                           Eina_Bool   server_client,
                           const char *msg)
{
   DBG("(conn=%p, server_client=%i, msg='%s')", conn, server_client, msg);
   if (msg)
     {
        if (server_client)
          ecore_con_client_send(conn, msg, strlen(msg));
        else
          ecore_con_server_send(conn, msg, strlen(msg));
     }

   if (server_client)
     ecore_con_client_del(conn);
   else
     ecore_con_server_del(conn);
   return ECORE_CALLBACK_RENEW;
}

inline void
azy_events_recv_progress(Azy_Net *net, void *data, size_t len)
{
   eina_binbuf_append_length(net->buffer, data, len);
   net->progress += len;
   INFO("(net=%p) PROGRESS: %zu/%"PRIi64, net, net->progress, net->http.content_length);
   if (azy_rpc_log_dom != -1) 
   {
      char buf[64];
      snprintf(buf, sizeof(buf), "RECEIVED:\n<<<<<<<<<<<<<\n%%.%"PRIi64"s\n<<<<<<<<<<<<<", len);
      RPC_INFO(buf, data);
   }
}

inline Eina_Bool
azy_events_length_overflows(int64_t current, int64_t max)
{
   if (max < 1) return EINA_FALSE;
   return current > max;
}

inline void
azy_events_transfer_progress_event(const Azy_Client_Handler_Data *hd, size_t size)
{
   Azy_Event_Transfer_Progress *dse;

   dse = malloc(sizeof(Azy_Event_Transfer_Progress));
   EINA_SAFETY_ON_NULL_RETURN(dse);
   dse->id = hd->id;
   dse->size = size;
   dse->client = hd->client;
   dse->net = hd->recv;
   ecore_event_add(AZY_EVENT_TRANSFER_PROGRESS, dse, NULL, NULL);
}

void
_azy_event_handler_fake_free(void *data  __UNUSED__,
                             void *data2 __UNUSED__)
{}

void
azy_fake_free(void *data __UNUSED__)
{}

