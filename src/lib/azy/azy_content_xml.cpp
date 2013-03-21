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
#include "azy_private.h"
#include <errno.h>

#include "pugixml.hpp"
#include <iterator>

using namespace pugi;

static void azy_value_serialize_xml(xml_node node, const Eina_Value *val);
static Eina_Value *azy_value_deserialize_xml(xml_node node);

struct xml_memory_writer : xml_writer
{
   char  *buffer;
   size_t capacity;

   size_t result;

   xml_memory_writer() : buffer(0), capacity(0), result(0)
   {
   }

   xml_memory_writer(char *buffer,
                     size_t capacity) : buffer(buffer), capacity(capacity), result(0)
   {
   }

   size_t
   written_size() const
   {
      return result < capacity ? result : capacity;
   }

   virtual void
   write(const void *data,
         size_t size)
   {
      if (result < capacity)
        {
           size_t chunk = (capacity - result < size) ? capacity - result : size;
           memcpy(buffer + result, data, chunk);
        }

      result += size;
   }
};

static char *
azy_content_xmlnode_to_buf(xml_node node,
                           int64_t *len)
{
   xml_memory_writer counter;
   char *buffer;

   node.print(counter);
   buffer = static_cast<char *>(malloc(counter.result + 1));
   xml_memory_writer writer(buffer, counter.result);
   node.print(writer);
   buffer[writer.written_size()] = 0;
   *len = static_cast<int64_t> (writer.written_size());

   return buffer;
}

static inline const Eina_Value_Type *
_azy_value_type_get(const char *type)
{
   switch (type[0]) /* yaaay micro-optimizing! */
     {
      case 'b':
        if (!strcmp(type + 1, "oolean")) return EINA_VALUE_TYPE_UCHAR;
        if (!strcmp(type + 1, "ase64")) return EINA_VALUE_TYPE_STRING;
        break;
      case 'i':
        if ((!strcmp(type + 1, "nt")) || (!strcmp(type + 1, "4"))) return EINA_VALUE_TYPE_INT;
        break;
      case 's':
        if (!strcmp(type + 1, "tring")) return EINA_VALUE_TYPE_STRINGSHARE;
        if (!strcmp(type + 1, "truct")) return EINA_VALUE_TYPE_STRUCT;
        break;
      case 'd':
        if (!strcmp(type + 1, "ouble")) return EINA_VALUE_TYPE_DOUBLE;
        if (!strcmp(type + 1, "ateTime.iso8601")) return EINA_VALUE_TYPE_TIMESTAMP;
        break;
      case 'a':
        if (!strcmp(type + 1, "rray")) return EINA_VALUE_TYPE_ARRAY;
        break;
      default:
        break;
     }
   return NULL;
}

static Eina_Value *
azy_value_deserialize_basic_xml(xml_node node, Eina_Value *arr)
{
   const char *name;

   if (node.empty()) return NULL;

   name = node.name();
   if ((!name) || (!name[0])) return NULL;

   _azy_value_type_get(name);
   switch (name[0]) /* yaaay micro-optimizing! */
     {
      case 'b':
        if (std::distance(node.begin(), node.end()) != 1)
          return NULL;

        if (!strcmp(name + 1, "oolean"))
          {
             int x;

             x = strtol(node.child_value(), NULL, 10);
             if ((x != 0) && (x != 1)) return NULL;
             if (arr)
               {
                  eina_value_array_append(arr, x);
                  return (Eina_Value*)1;
               }
             return azy_value_util_bool_new(x);
          }
        if (!strcmp(name + 1, "ase64"))
          {
             if (arr)
               {
                  eina_value_array_append(arr, node.child_value());
                  return (Eina_Value*)1;
               }
             return azy_value_util_base64_new(node.child_value());
          }
        break;
      case 'i':
        if (std::distance(node.begin(), node.end()) != 1)
          return NULL;

        if ((!strcmp(name + 1, "nt")) || (!strcmp(name + 1, "4")))
          {
             int x;

             errno = 0;
             x = strtol(node.child_value(), NULL, 10);
             if (errno) return NULL;
             if (arr)
               {
                  eina_value_array_append(arr, x);
                  return (Eina_Value*)1;
               }
             return azy_value_util_int_new(x);
          }
        break;
      case 's':
        if (!strcmp(name + 1, "tring"))
          {
             if (std::distance(node.begin(), node.end()) > 1)
               return NULL;
             if (arr)
               {
                  eina_value_array_append(arr, node.child_value());
                  return (Eina_Value*)1;
               }
             return azy_value_util_string_new(node.child_value());
          }
        break;
      case 'd':
        if (std::distance(node.begin(), node.end()) != 1)
          return NULL;

        if (!strcmp(name + 1, "ouble"))
          {
             double x;

             errno = 0;
             x = strtod(node.child_value(), NULL);
             if (errno) return NULL;
             if (arr)
               {
                  eina_value_array_append(arr, x);
                  return (Eina_Value*)1;
               }
             return azy_value_util_double_new(x);
          }
        if (!strcmp(name + 1, "ateTime.iso8601"))
          {
             if (arr)
               {
                  struct tm tim;
                  time_t t;

                  if (!strptime(node.child_value(), "%Y%m%dT%H:%M:%S", &tim)) break;
                  t = mktime(&tim);
                  eina_value_array_append(arr, t);
                  return (Eina_Value*)1;
               }
             return azy_value_util_time_string_new(node.child_value());
          }

      default:
        break;
     }

   return NULL;
}

static Eina_Value *
azy_value_deserialize_struct_xml(xml_node node)
{
   Eina_Array *st_members, *st_values;
   unsigned int offset = 0, z = 0;
   Eina_Value *value_st = NULL;
   Eina_Value_Struct_Member *members;
   Eina_Value_Struct_Desc *st_desc;
   const char *name;

   if (node.empty()) return NULL;

   if (std::distance(node.begin(), node.end()) < 1)
     return NULL;
   node = node.child("member");
   if (node.empty()) return NULL;

   st_desc = azy_value_util_struct_desc_new();
   st_members = eina_array_new(1);
   st_values = eina_array_new(1);

   for (xml_node it = node; it; it = it.next_sibling())
     {
        Eina_Value_Struct_Member *m;
        const Eina_Value_Type *type;
        xml_node child;
        Eina_Value *v;

        if ((std::distance(it.begin(), it.end()) != 2) || /* member */
            (std::distance(it.first_child().begin(), it.first_child().end()) != 1) || /* name/value */
            (std::distance(it.first_child().first_child().begin(), it.first_child().first_child().end()))) /* name */
          {
             ERR("%s", eina_error_msg_get(AZY_ERROR_XML_UNSERIAL));
             goto end;
          }
        child = it.child("value").first_child();
        if (child.empty())
          {
             ERR("%s", eina_error_msg_get(AZY_ERROR_XML_UNSERIAL));
             goto end;
          }
        type = _azy_value_type_get(child.name());
        if (!type)
          {
             ERR("%s", eina_error_msg_get(AZY_ERROR_XML_UNSERIAL));
             goto end;
          }
        name = it.child("name").child_value();
        if (!name)
          {
             ERR("%s", eina_error_msg_get(AZY_ERROR_XML_UNSERIAL));
             goto end;
          }
        m = (Eina_Value_Struct_Member*)calloc(1, sizeof(Eina_Value_Struct_Member));
        m->name = eina_stringshare_add(name);
        offset = azy_value_util_type_offset(type, offset);
        m->offset = offset;
        offset += azy_value_util_type_size(type);
        m->type = type;
        eina_array_push(st_members, m);

        v = azy_value_deserialize_xml(child);
        eina_array_push(st_values, v ?: (void*)1);
        z++;
     }
   if (!z)
     {
        free(st_desc);
        goto end;
     }

   members = (Eina_Value_Struct_Member*)malloc(eina_array_count(st_members) * sizeof(Eina_Value_Struct_Member));
   for (z = 0; z < eina_array_count(st_members); z++)
     {
        Eina_Value_Struct_Member *m = (Eina_Value_Struct_Member*)eina_array_data_get(st_members, z);
        members[z].name = m->name;
        members[z].offset = m->offset;
        members[z].type = m->type;
        free(m);
     }

   //setup
   st_desc->members = members;
   st_desc->member_count = eina_array_count(st_members);
   st_desc->size = offset;
   value_st = eina_value_struct_new(st_desc);

   //filling with data
   for (z = 0; z < eina_array_count(st_values); z++)
     {
        Eina_Value *v = (Eina_Value*)eina_array_data_get(st_values, z);
        if (v == (void*)1) continue;
        eina_value_struct_value_set(value_st, members[z].name, v);
        eina_value_free(v);
     }

end:
   eina_array_free(st_members);
   eina_array_free(st_values);
   return value_st;
}

static Eina_Value *
azy_value_deserialize_array_xml(xml_node node)
{
   Eina_Value *arr;
   const char *name;
   const Eina_Value_Type *type;

   if (node.empty()) return NULL;

   node = node.child("data");
   if (node.empty()) return NULL;
   if (std::distance(node.begin(), node.end()) < 1)
     return NULL;
   node = node.child("value");
   if (node.empty()) return NULL;

   name = node.first_child().name();
   if ((!name) || (!name[0])) return NULL; //incorrectly formed blank array
   type = _azy_value_type_get(name);
   arr = eina_value_array_new(type, 0);
   for (xml_node it = node; it; it = it.next_sibling())
     {
        if (std::distance(node.begin(), node.end()) != 1)
          {
             ERR("%s", eina_error_msg_get(AZY_ERROR_XML_UNSERIAL));
             goto error;
          }
        if (type == EINA_VALUE_TYPE_ARRAY)
          {
             Eina_Value_Array inner_array;
             Eina_Value *data = azy_value_deserialize_array_xml(it.first_child());
             if (!data) continue;
             eina_value_get(data, &inner_array);
             eina_value_array_append(arr, inner_array);
             eina_value_free(data);
          }
        else if (type == EINA_VALUE_TYPE_STRUCT)
           {
              Eina_Value_Struct st;
              Eina_Value *data = azy_value_deserialize_struct_xml(it.first_child());
              if (!data) continue;
              eina_value_get(data, &st);
              eina_value_array_append(arr, st);
              eina_value_free(data);
           }
        else
          {
             if (!azy_value_deserialize_basic_xml(it.first_child(), arr)) goto error;
          }
     }
   return arr;
error:
   eina_value_free(arr);
   return NULL;
}

static void
azy_value_serialize_basic_xml(xml_node node, const Eina_Value *val)
{
   char buf[128];
   xml_node n;

   switch (azy_value_util_type_get(val))
     {
      case AZY_VALUE_INT:
      {
         int x;

         eina_value_get(val, &x);
         snprintf(buf, sizeof(buf), "%d", x);
         node.append_child("int").append_child(node_pcdata).set_value(buf);
         return;
      }
      case AZY_VALUE_STRING:
      {
         const char *txt;
         eina_value_get(val, &txt);
         n = node.append_child("string");
         if (txt) n.append_child(node_pcdata).set_value(txt);
         return;
      }
      case AZY_VALUE_BASE64:
      {
         const char *txt;
         eina_value_get(val, &txt);
         n = node.append_child("base64");
         if (txt) n.append_child(node_pcdata).set_value(txt);
         return;
      }
      case AZY_VALUE_BOOL:
      {
         Eina_Bool x;

         eina_value_get(val, &x);
         node.append_child("boolean").append_child(node_pcdata).set_value(x ? "1" : "0");
         return;
      }
      case AZY_VALUE_DOUBLE:
      {
         double x;

         eina_value_get(val, &x);
         snprintf(buf, sizeof(buf), "%g", x);
         node.append_child("double").append_child(node_pcdata).set_value(buf);
         return;
      }

      case AZY_VALUE_TIME:
      {
         time_t t;
         struct tm *tim;

         eina_value_get(val, &t);
         tim = localtime(&t);
         strftime(buf, sizeof(buf), "%Y%m%dT%H:%M:%S", tim);
         node.append_child("dateTime.iso8601").append_child(node_pcdata).set_value(buf);
      }
      default: break;
     }
}

static void
azy_value_serialize_struct_xml(xml_node node, const Eina_Value *val)
{
   Eina_Value_Struct st;
   unsigned int x;

   eina_value_pget(val, &st);
   node = node.append_child("struct");
   for (x = 0; x < st.desc->member_count; x++)
     {
        xml_node member, value;
        Eina_Value m;

        member = node.append_child("member");
        member.append_child("name").append_child(node_pcdata).set_value(st.desc->members[x].name);
        value = member.append_child("value");
        eina_value_struct_value_get(val, st.desc->members[x].name, &m);
        azy_value_serialize_xml(value, &m);
        eina_value_flush(&m);
     }
}

static void
azy_value_serialize_array_xml(xml_node node, const Eina_Value *val)
{
   unsigned int x, total;

   node = node.append_child("array").append_child("data");
   total = eina_value_array_count(val);
   for (x = 0; x < total; x++)
     {
        Eina_Value m;

        eina_value_array_value_get(val, x, &m);
        azy_value_serialize_xml(node.append_child("value"), &m);
        eina_value_flush(&m);
     }
}


static void
azy_value_serialize_xml(xml_node node, const Eina_Value *val)
{
   EINA_SAFETY_ON_TRUE_RETURN(node.empty());
   EINA_SAFETY_ON_NULL_RETURN(val);

   if (eina_value_type_get(val) == EINA_VALUE_TYPE_ARRAY)
     azy_value_serialize_array_xml(node, val);
   if (eina_value_type_get(val) == EINA_VALUE_TYPE_STRUCT)
     azy_value_serialize_struct_xml(node, val);
   else
     azy_value_serialize_basic_xml(node, val);
}

Eina_Bool
azy_content_serialize_request_xml(Azy_Content *content)
{
   xml_document doc;
   xml_node node;
   Eina_List *l;
   void *val;
   unsigned char *buf;
   int64_t len;

   node = doc.append_child("methodCall");
   node.append_child("methodName").append_child(node_pcdata).set_value(content->method);
   if (content->params)
     node = node.append_child("params");

   EINA_LIST_FOREACH(content->params, l, val)
     {
        xml_node p;

        p = node.append_child("param").append_child("value");
        azy_value_serialize_xml(p, (Eina_Value*)val);
     }
   buf = (unsigned char *)azy_content_xmlnode_to_buf(doc, &len);
   content->buffer = eina_binbuf_manage_new_length(buf, len);
   return EINA_TRUE;
}

Eina_Bool
azy_content_serialize_response_xml(Azy_Content *content)
{
   xml_document doc;
   xml_node node, m;
   unsigned char *buf;
   int64_t len;

   node = doc.append_child("methodResponse");
   if (content->error_set)
     {
        char buf[16];

        node = node.append_child("fault").append_child("value").append_child("struct");
        m = node.append_child("member");
        m.append_child("name").append_child(node_pcdata).set_value("faultCode");
        snprintf(buf, sizeof(buf), "%i", content->faultcode);
        m.append_child("value").append_child("int").append_child(node_pcdata).set_value(buf);
        m = node.append_child("member");
        m.append_child("name").append_child(node_pcdata).set_value("faultString");
        m.append_child("value").append_child("string").append_child(node_pcdata).set_value(azy_content_error_message_get(content));
     }
   else
     {
        node = node.append_child("params").append_child("param").append_child("value");
        azy_value_serialize_xml(node, content->retval);
     }

   buf = (unsigned char *)azy_content_xmlnode_to_buf(doc, &len);
   content->buffer = eina_binbuf_manage_new_length(buf, len);
   return EINA_TRUE;
}

static Eina_Value *
azy_value_deserialize_xml(xml_node node)
{
   const Eina_Value_Type *type = _azy_value_type_get(node.name());
   if (type == EINA_VALUE_TYPE_ARRAY)
     return azy_value_deserialize_array_xml(node);
   if (type == EINA_VALUE_TYPE_STRUCT)
     return azy_value_deserialize_struct_xml(node);
   return azy_value_deserialize_basic_xml(node, NULL);
}

static Eina_Bool
azy_content_deserialize_request_xml(Azy_Content *content,
                                    xml_document &doc)
{
   xpath_node_set params, name;
   const char *mn;
   static xpath_query pquery("/methodCall/params/param/value");
   static xpath_query nquery("/methodCall/methodName");
   ptrdiff_t num;

   if ((!doc.first_child()) || (doc.first_child() != doc.last_child())) /* more than methodCall at root */
     return EINA_FALSE;

   num = std::distance(doc.first_child().begin(), doc.first_child().end());

   if ((num != 2) && (num != 1)) /* not params/methodName */
     return EINA_FALSE;

   name = nquery.evaluate_node_set(doc);
   if (name.empty())
     {
        azy_content_error_code_set(content, AZY_ERROR_REQUEST_XML_METHODNAME);
        return EINA_FALSE;
     }

   if (!name.first().node().first_child().first_child().empty()) /* that's an error */
     return EINA_FALSE;

   mn = name.first().node().child_value();
   if ((!mn) || (!mn[0])) return EINA_FALSE;
   content->method = eina_stringshare_add(mn);

   params = pquery.evaluate_node_set(doc);
   if (params.empty()) return EINA_TRUE;
   {
      xpath_node_set fc, fs;
      xml_node v, p;

      v = params.first().node(); /* <value> */
      p = v.parent(); /* <param> */
      if (std::distance(p.parent().begin(), p.parent().end()) != static_cast<int64_t>(params.size())) /* that's a paddlin */
        return EINA_FALSE;
      if (std::distance(p.begin(), p.end()) != 1) /* that's a paddlin */
        return EINA_FALSE;
      if (std::distance(v.begin(), v.end()) != 1) /* that's also a paddlin */
        return EINA_FALSE;

      for (xpath_node_set::const_iterator it = params.begin(); it != params.end(); ++it)
        {
           xpath_node n;
           Eina_Value *val;
           void *v;
           xml_node node;

           n = *it;

           if (std::distance(n.node().begin(), n.node().end()) != 1) return EINA_FALSE;
           val = azy_value_deserialize_xml(n.node().first_child());
           if (!val)
             {
                azy_content_error_faultmsg_set(content, -1,
                                               "Can't parse XML-RPC XML request. Failed to deserialize parameter %i.", 1 + std::distance(params.begin(), it));
                EINA_LIST_FREE(content->params, v)
                  eina_value_free((Eina_Value*)v);
                return EINA_FALSE;
             }
           azy_content_param_add(content, val);
        }
   }

   return EINA_TRUE;
}

static Eina_Bool
azy_content_deserialize_response_xml(Azy_Content *content,
                                     xml_document &doc)
{
   xpath_node_set params;
   static xpath_query pquery("/methodResponse/params/param/value");
   static xpath_query fcquery("/methodResponse/fault/value/struct/member/value/int");
   static xpath_query fsquery("/methodResponse/fault/value/struct/member/value/string");

   if ((!doc.first_child()) || (doc.first_child() != doc.last_child())) /* more than methodResponse at root */
     return EINA_FALSE;

   if (std::distance(doc.first_child().begin(), doc.first_child().end()) != 1) /* more than params/fault */
     return EINA_FALSE;

   params = pquery.evaluate_node_set(doc);
   if (params.empty())
     {
        xpath_node_set fc, fs;
        int c;
        const char *s = NULL;
        xml_node f;

        f = doc.first_child().first_child(); /* <fault> */
        if (f.empty() || (std::distance(f.begin(), f.end()) != 1)) /* again, a paddlin */
          return EINA_FALSE;
        f = f.first_child(); /* <value> */
        if (f.empty() || (std::distance(f.begin(), f.end()) != 1)) /* yet another paddlin */
          return EINA_FALSE;
        f = f.first_child(); /* <struct> */
        if (f.empty() || (std::distance(f.begin(), f.end()) != 2)) /* yet another paddlin */
          return EINA_FALSE;

        fc = fcquery.evaluate_node_set(doc);
        if (fc.empty())
          {
             azy_content_error_code_set(content, AZY_ERROR_RESPONSE_XML_FAULT);
             return EINA_FALSE;
          }
        f = fc.first().node(); /* <int> */
        if (std::distance(f.begin(), f.end()) != 1) /* more paddlins */
          return EINA_FALSE;
        f = f.parent(); /* <value> */
        if (std::distance(f.begin(), f.end()) != 1) /* some paddlins here too */
          return EINA_FALSE;

        fs = fsquery.evaluate_node_set(doc);
        if (fs.empty())
          {
             azy_content_error_code_set(content, AZY_ERROR_RESPONSE_XML_FAULT);
             return EINA_FALSE;
          }
        f = fs.first().node(); /* <string> */
        if (std::distance(f.begin(), f.end()) != 1) /* more paddlins */
          return EINA_FALSE;
        f = f.parent(); /* <value> */
        if (std::distance(f.begin(), f.end()) != 1) /* some paddlins here too */
          return EINA_FALSE;
        errno = 0;
        c = strtol(fc.first().node().child_value(), NULL, 10);
        if (errno) c = -1;
        if (!fs.empty()) s = fs.first().node().child_value();

        if (s)
          {
             Eina_Error e;

             e = eina_error_find(s);
             if (e) azy_content_error_faultcode_set(content, e, c);
             else
               azy_content_error_faultmsg_set(content, c, "%s", s);
          }
        else
          azy_content_error_faultmsg_set(content, c, "%s", "");
        return EINA_TRUE;
     }
   else if (params.size() > 1)
     {
        azy_content_error_code_set(content, AZY_ERROR_RESPONSE_XML_MULTI);
        return EINA_FALSE;
     }

   content->retval = azy_value_deserialize_xml(params.first().node().first_child());
   if (!content->retval)
     {
        azy_content_error_code_set(content, AZY_ERROR_RESPONSE_XML_RETVAL);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static Eina_Bool
azy_content_deserialize_rss_xml(Azy_Content *content,
                                xml_document &doc)
{
   xpath_node_set channel, img;
   Azy_Rss *rss;
   static xpath_query cquery("/rss/channel");
   static xpath_query iquery("/rss/channel/image/url");

   channel = cquery.evaluate_node_set(doc);

   if (channel.empty())
     {
        azy_content_error_code_set(content, AZY_ERROR_RESPONSE_XML_FAULT);
        return EINA_FALSE;
     }

   rss = azy_rss_new();
   if (!rss)
     {
        azy_content_error_code_set(content, AZY_ERROR_RESPONSE_XML_FAULT);
        return EINA_FALSE;
     }
   img = iquery.evaluate_node_set(doc);
   if (!img.empty())
     rss->img_url = eina_stringshare_add(img.first().node().child_value());

   for (xml_node::iterator it = channel.first().node().begin(); it != channel.first().node().end(); ++it)
     {
        xml_node n;
        const char *name;

        n = *it;
        name = n.name();

        if ((!rss->title) && (!strcmp(name, "title")))
          rss->title = eina_stringshare_add(n.child_value());
        else if ((!rss->link) && (!strcmp(name, "link")))
          rss->link = eina_stringshare_add(n.child_value());
        else if ((!rss->desc) && (!strcmp(name, "description")))
          rss->desc = eina_stringshare_add(n.child_value());
        else if (!strcmp(name, "item") && (!n.empty()))
          {
             Azy_Rss_Item *i;

             i = azy_rss_item_new();
             if (!i) goto error;

             for (xml_node::iterator x = n.begin(); x != n.end(); ++x)
               {
                  xml_node nn;

                  nn = *x;
                  name = nn.name();

                  if ((!i->title) && (!strcmp(name, "title")))
                    i->title = eina_stringshare_add(nn.child_value());
                  else if ((!i->link) && (!strcmp(name, "link")))
                    i->link = eina_stringshare_add(nn.child_value());
                  else if ((!i->desc) && (!strcmp(name, "description")))
                    i->desc = eina_stringshare_add(nn.child_value());
                  else if ((!i->author) && (!strcmp(name, "author")))
                    eina_stringshare_replace(&i->author, nn.child_value());
                  else if ((!i->author) && (!strcmp(name, "dc:creator")))
                    eina_stringshare_replace(&i->author, nn.child_value());
                  else if ((!i->date) && (!strcmp(name, "pubDate")))
                    i->date = eina_stringshare_add(nn.child_value());
                  else if ((!i->guid) && (!strcmp(name, "guid")))
                    i->guid = eina_stringshare_add(nn.child_value());
                  else if ((!i->comment_url) && (!strcmp(name, "comments")))
                    i->comment_url = eina_stringshare_add(nn.child_value());
                  else if ((!i->content) && (!strcmp(name, "content")))
                    i->content = eina_stringshare_add(nn.child_value());
                  else if ((!i->content_encoded) && (!strcmp(name, "content:encoded")))
                    i->content_encoded = eina_stringshare_add(nn.child_value());
               }
             rss->items = eina_list_append(rss->items, i);
          }
     }
   content->ret = rss;
   return EINA_TRUE;

error:
   azy_rss_free(rss);
   return EINA_FALSE;
}

static Azy_Rss_Link *
azy_content_deserialize_atom_xml_link(xml_node &node)
{
/*
   atomLink =
   element atom:link {
      atomCommonAttributes,
      attribute href { atomUri },
      attribute rel { atomNCName | atomUri }?,
      attribute type { atomMediaType }?,
      attribute hreflang { atomLanguageTag }?,
      attribute title { text }?,
      attribute length { text }?,
      undefinedContent
   }
 */
   Azy_Rss_Link *rl;

   rl = (Azy_Rss_Link *)calloc(1, sizeof(Azy_Rss_Link));
   EINA_SAFETY_ON_NULL_RETURN_VAL(rl, NULL);
   for (xml_attribute_iterator i = node.attributes_begin(); i != node.attributes_end(); ++i)
     {
        xml_attribute it;

        it = *i;

#define SET(X)                \
  if (!strcmp(it.name(), #X)) \
    rl->X = eina_stringshare_add(it.value())
        SET(href);
        else SET(rel);
        else SET(type);
        else SET(hreflang);
        else SET(title);
        else if (!strcmp(it.name(), "length"))
          rl->length = it.as_uint();
#undef SET
     }
   return rl;
}

static Azy_Rss_Contact *
azy_content_deserialize_atom_xml_contact(xml_node &node)
{
/*
   atomPersonConstruct =
   atomCommonAttributes,
   (element atom:name { text }
    & element atom:uri { atomUri }?
    & element atom:email { atomEmailAddress }?
    & extensionElement*)
 */
   Azy_Rss_Contact *c;

   c = (Azy_Rss_Contact *)calloc(1, sizeof(Azy_Rss_Contact));
   EINA_SAFETY_ON_NULL_RETURN_VAL(c, NULL);
   for (xml_node::iterator i = node.begin(); i != node.end(); ++i)
     {
        xml_node n;
        const char *name;

        n = *i;
        name = n.name();

#define SET(X)                        \
  if ((!c->X) && (!strcmp(name, #X))) \
    c->X = eina_stringshare_add(n.child_value())

        SET(name);
        else SET(uri);
        else SET(email);
#undef SET
     }
   return c;
}

static Azy_Rss_Item *
azy_content_deserialize_atom_xml_entry(xml_node &node)
{
/*
   atomEntry =
   element atom:entry {
      atomCommonAttributes,
      (atomAuthor*
       & atomCategory*
       & atomContent?
       & atomContributor*
       & atomId
       & atomLink*
       & atomPublished?
       & atomRights?
       & atomSource?
       & atomSummary?
       & atomTitle
       & atomUpdated
       & extensionElement*)
   }
 */
   Azy_Rss_Item *it;

   it = azy_rss_item_new();
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, NULL);
   it->atom = EINA_TRUE;
   for (xml_node::iterator i = node.begin(); i != node.end(); ++i)
     {
        xml_node n;
        const char *name;
        Azy_Rss_Contact *c;

        n = *i;
        name = n.name();

#define SET(X)                         \
  if ((!it->X) && (!strcmp(name, #X))) \
    it->X = eina_stringshare_add(n.child_value())

        SET(title);
        else SET(rights);
        else SET(summary);
        else SET(id);
        else SET(icon);
#undef SET
        else if (!strcmp(name, "category"))
          it->categories = eina_list_append(it->categories, eina_stringshare_add(n.attribute("term").value()));
        else if (!strcmp(name, "contributor"))
          {
             c = azy_content_deserialize_atom_xml_contact(n);
             if (c)
               it->contributors = eina_list_append(it->contributors, c);
          }
        else if (!strcmp(name, "author"))
          {
             c = azy_content_deserialize_atom_xml_contact(n);
             if (c)
               it->authors = eina_list_append(it->authors, c);
          }
        else if (!strcmp(name, "link"))
          {
             Azy_Rss_Link *rl;

             rl = azy_content_deserialize_atom_xml_link(n);
             if (rl) it->atom_links = eina_list_append(it->atom_links, rl);
          }
        else if (!strcmp(name, "updated"))
          strptime(n.child_value(), "%FT%TZ", &it->updated);
        else if (!strcmp(name, "published"))
          strptime(n.child_value(), "%FT%TZ", &it->published);
     }
   return it;
}

Eina_Bool
azy_content_deserialize_atom_xml(Azy_Content *content,
                                 xml_document &doc)
{
   xml_node node;
   xml_attribute attr;
   Azy_Rss *rss;

   rss = azy_rss_new();
   if (!rss)
     {
        azy_content_error_code_set(content, AZY_ERROR_RESPONSE_XML_FAULT);
        return EINA_FALSE;
     }
   rss->atom = EINA_TRUE;
   for (xml_node::iterator it = doc.first_child().begin(); it != doc.first_child().end(); ++it)
     {
        xml_node n;
        const char *name;
        Azy_Rss_Contact *c;

        n = *it;
        name = n.name();
/* http://tools.ietf.org/html/rfc4287
   atomFeed =
   element atom:feed {
      atomCommonAttributes,
      (atomAuthor*
       & atomCategory*
       & atomContributor*
       & atomGenerator?
       & atomIcon?
       & atomId
       & atomLink*
       & atomLogo?
       & atomRights?
       & atomSubtitle?
       & atomTitle
       & atomUpdated
       & extensionElement*),
      atomEntry*
   }
 */
#define SET(X)                          \
  if ((!rss->X) && (!strcmp(name, #X))) \
    rss->X = eina_stringshare_add(n.child_value())

        SET(title);
        else SET(rights);
        else SET(subtitle);
        else SET(generator);
        else SET(logo);
        else SET(id);
#undef SET
        else if (!strcmp(name, "category"))
          rss->categories = eina_list_append(rss->categories, eina_stringshare_add(n.attribute("term").value()));
        else if (!strcmp(name, "contributor"))
          {
             c = azy_content_deserialize_atom_xml_contact(n);
             if (c)
               rss->contributors = eina_list_append(rss->contributors, c);
          }
        else if (!strcmp(name, "author"))
          {
             c = azy_content_deserialize_atom_xml_contact(n);
             if (c)
               rss->authors = eina_list_append(rss->authors, c);
          }
        else if (!strcmp(name, "link"))
          {
             Azy_Rss_Link *rl;

             rl = azy_content_deserialize_atom_xml_link(n);
             if (rl) rss->atom_links = eina_list_append(rss->atom_links, rl);
          }
        else if (!strcmp(name, "icon"))
          rss->img_url = eina_stringshare_add(n.child_value());
        else if (!strcmp(name, "updated"))
          strptime(n.child_value(), "%FT%TZ", &rss->updated);
        else if (!strcmp(name, "entry") && (!n.empty()))
          {
             Azy_Rss_Item *i;

             i = azy_content_deserialize_atom_xml_entry(n);
             if (i) rss->items = eina_list_append(rss->items, i);
          }
     }
   content->ret = rss;
   return EINA_TRUE;
}

Eina_Bool
azy_content_deserialize_response_xml(Azy_Content *content,
                                     char *buf,
                                     ssize_t len)
{
   xml_document doc;

   if ((!content) || (!buf)) return EINA_FALSE;

   if (!doc.load_buffer_inplace(buf, len))
     {
        azy_content_error_code_set(content, AZY_ERROR_RESPONSE_XML_DOC);
        return EINA_FALSE;
     }

   return azy_content_deserialize_response_xml(content, doc);
}

Eina_Bool
azy_content_deserialize_request_xml(Azy_Content *content,
                                    char *buf,
                                    ssize_t len)
{
   xml_document doc;

   if ((!content) || (!buf)) return EINA_FALSE;

   if (!doc.load_buffer_inplace(buf, len))
     {
        azy_content_error_code_set(content, AZY_ERROR_RESPONSE_XML_DOC);
        return EINA_FALSE;
     }

   return azy_content_deserialize_request_xml(content, doc);
}

Eina_Bool
azy_content_deserialize_xml(Azy_Content *content,
                            char *buf,
                            ssize_t len)
{
   xml_document doc;
   xml_attribute attr;
   xml_node node;

   if ((!content) || (!buf)) return EINA_FALSE;

   if (!doc.load_buffer_inplace(buf, len))
     {
        azy_content_error_code_set(content, AZY_ERROR_RESPONSE_XML_DOC);
        return EINA_FALSE;
     }

   node = doc.first_child();
   if (node.empty())
     {
        azy_content_error_code_set(content, AZY_ERROR_RESPONSE_XML_FAULT);
        return EINA_FALSE;
     }
   switch (node.name()[0])
     {
      case 'r':
        if (!strcmp(node.name(), "rss"))
          return azy_content_deserialize_rss_xml(content, doc);
        break;

      case 'm':
        if (!strcmp(node.name(), "methodCall"))
          return azy_content_deserialize_request_xml(content, doc);
        else if (!strcmp(node.name(), "methodResponse"))
          return azy_content_deserialize_response_xml(content, doc);

      default:
        break;
     }
   attr = doc.first_child().attribute("xmlns");
   if ((!attr.empty()) && (!strcmp(attr.value(), "http://www.w3.org/2005/Atom")))
     return azy_content_deserialize_atom_xml(content, doc);
   azy_content_error_code_set(content, AZY_ERROR_RESPONSE_XML_FAULT);
   return EINA_FALSE;
}
