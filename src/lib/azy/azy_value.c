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

/**
 * @defgroup Azy_Value Low-level serialization
 * @brief Functions which provide a generic struct to represent RPC
 * @{
 */


static void *
_ops_malloc(const Eina_Value_Struct_Operations *ops EINA_UNUSED, const Eina_Value_Struct_Desc *desc)
{
   Azy_Value_Struct_Desc *edesc = (Azy_Value_Struct_Desc*)desc;
   edesc->refcount++;
   //DBG("%p refcount=%d", edesc, edesc->refcount);
   return malloc(desc->size);
}

static void
_ops_free(const Eina_Value_Struct_Operations *ops EINA_UNUSED, const Eina_Value_Struct_Desc *desc, void *memory)
{
   Azy_Value_Struct_Desc *edesc = (Azy_Value_Struct_Desc*) desc;
   edesc->refcount--;
   free(memory);
   //DBG("%p refcount=%d", edesc, edesc->refcount);
   if (edesc->refcount <= 0)
     {
        unsigned i;
        for (i = 0; i < edesc->base.member_count; i++)
          eina_stringshare_del((char *)edesc->base.members[i].name);
        free((Eina_Value_Struct_Member *)edesc->base.members);
        free(edesc);
     }
}

static Eina_Value_Struct_Operations operations =
{
   EINA_VALUE_STRUCT_OPERATIONS_VERSION,
   _ops_malloc,
   _ops_free,
   NULL,
   NULL,
   NULL
};

static Eina_Bool  azy_value_list_multi_line_get_(const Eina_Value *v);


/* returns EINA_TRUE if a struct/array object requires multiple lines to print */
static Eina_Bool
azy_value_list_multi_line_get_(const Eina_Value *val)
{
   const Eina_Value_Type *type;

   if (!val) return EINA_FALSE;
   type = eina_value_type_get(val);
   if (type == EINA_VALUE_TYPE_ARRAY)
     {
        unsigned int x, total = eina_value_array_count(val);
        if (total > 8) return EINA_TRUE;
        for (x = 0; x < total; x++)
          {
             Eina_Value m;

             eina_value_array_value_get(val, x, &m);
             if (azy_value_multi_line_get_(&m, 35)) return EINA_TRUE;
             eina_value_flush(&m);
          }
        return EINA_FALSE;
     }
   else if (type == EINA_VALUE_TYPE_STRUCT)
     {
        Eina_Value_Struct st;
        unsigned int x;

        eina_value_pget(val, &st);
        if (st.desc->member_count > 5) return EINA_TRUE;
        for (x = 0; x < st.desc->member_count; x++)
          {
             Eina_Value m;

             eina_value_struct_value_get(val, st.desc->members[x].name, &m);
             if (azy_value_multi_line_get_(&m, 25)) return EINA_TRUE;
             eina_value_flush(&m);
          }
        return EINA_FALSE;
     }

   return EINA_FALSE;
}

/* returns EINA_TRUE if the line requires multiple lines to print */
Eina_Bool
azy_value_multi_line_get_(const Eina_Value *val,
                          unsigned int max_strlen)
{
   const Eina_Value_Type *type;

   type = eina_value_type_get(val);
   if (type == EINA_VALUE_TYPE_STRUCT)
     {
        Eina_Value_Struct st;

        eina_value_pget(val, &st);
        return !!st.desc->member_count;
     }
   if (type == EINA_VALUE_TYPE_ARRAY)
     {
        return !!eina_value_array_count(val);
     }
   if ((type == EINA_VALUE_TYPE_STRING) || (type == EINA_VALUE_TYPE_STRINGSHARE))
     {
        char *str;
        eina_value_get(val, &str);
        return eina_strlen_bounded(str, max_strlen + 1) > max_strlen;
     }
   return EINA_FALSE;
}

/**
 * @brief Return the type of an #Eina_Value
 *
 * This function is used to return the type of value in
 * an #Eina_Value, mapped to an int for use in switches.
 * @param val The #Eina_Value struct (NOT NULL)
 * @return The #Azy_Value_Type, or AZY_VALUE_LAST on error
 */
Azy_Value_Type
azy_value_util_type_get(const Eina_Value *val)
{
   const Eina_Value_Type *type;

   EINA_SAFETY_ON_NULL_RETURN_VAL(val, AZY_VALUE_LAST);
   type = eina_value_type_get(val);
   if (type == EINA_VALUE_TYPE_ARRAY) return AZY_VALUE_ARRAY;
   if (type == EINA_VALUE_TYPE_STRUCT) return AZY_VALUE_STRUCT;
   if (type == EINA_VALUE_TYPE_STRING) return AZY_VALUE_BASE64;
   if (type == EINA_VALUE_TYPE_STRINGSHARE) return AZY_VALUE_STRING;
   if (type == EINA_VALUE_TYPE_UCHAR) return AZY_VALUE_BOOL;
   if (type == EINA_VALUE_TYPE_INT) return AZY_VALUE_INT;
   if (type == EINA_VALUE_TYPE_DOUBLE) return AZY_VALUE_DOUBLE;
   if (type == EINA_VALUE_TYPE_TIMESTAMP) return AZY_VALUE_TIME;
   return AZY_VALUE_LAST;
}


/**
 * @brief Check if an #Eina_Value is an RPC error
 *
 * This function checks to see if @p val is an RPC error
 * with a faultcode and faultstring, returning both if
 * it is.
 * Note that the faultstring returned is still owned by @p val.
 * @param val The #Azy_Value to check
 * @param errcode Pointer to store a faultcode in
 * @param errmsg Pointer to store a faultmsg in
 * @return EINA_FALSE if @p val is not an error value, else EINA_TRUE
 */
Eina_Bool
azy_value_util_retval_is_error(const Eina_Value *val, int *errcode, const char **errmsg)
{
   const Eina_Value_Struct_Member *c, *s;
   Eina_Value m;
   Eina_Value_Struct st;

   if ((azy_value_util_type_get(val) != AZY_VALUE_STRUCT) || (!errcode) || (!errmsg))
     return EINA_FALSE;

   eina_value_pget(val, &st);
   c = eina_value_struct_member_find(&st, "faultCode");
   s = eina_value_struct_member_find(&st, "faultString");

   if ((!c) && (!s)) return EINA_FALSE;

   if (s)
     {
        eina_value_struct_member_value_get(val, s, &m);
        eina_value_get(&m, errmsg);
        eina_value_flush(&m);
     }
   if (c)
     {
        if (s) eina_value_flush(&m);
        eina_value_struct_member_value_get(val, c, &m);
        eina_value_get(&m, errcode);
     }

   return EINA_TRUE;
}

/**
 * @brief Dump a value's contents into a string
 *
 * This function appends the values in @p v into #Eina_Strbuf @p string,
 * indenting @p indent spaces.  It calls itself recursively, dumping all sub-values
 * into @p string as well.
 * Note that base64 values are NOT decoded when dumping.
 * @param v The value to dump (NOT NULL)
 * @param string The #Eina_Strbuf to append to (NOT NULL)
 * @param indent The number of spaces to indent
 */
void
azy_value_util_dump(const Eina_Value *v,
               Eina_Strbuf *string,
               unsigned int indent)
{
   unsigned int x, total;
   Eina_Value val;
   char buf[256];
   Eina_Bool multi;

   EINA_SAFETY_ON_NULL_RETURN(v);
   EINA_SAFETY_ON_NULL_RETURN(string);

   memset(buf, ' ', MIN(indent * 2, sizeof(buf) - 1));

   switch (azy_value_util_type_get(v))
     {
      case AZY_VALUE_ARRAY:
        if (!eina_value_array_count(v))
          {
             eina_strbuf_append(string, "[]");
             break;
          }
        multi = azy_value_list_multi_line_get_(v);
        eina_strbuf_append(string, "[ ");
        total = eina_value_array_count(v);
        for (x = 0; x < total; x++)
          {
             eina_value_array_value_get(v, x, &val);
             if (multi) eina_strbuf_append_printf(string, "\n%s  ", buf);
             azy_value_util_dump(&val, string, indent + 1);
             if (multi)
               {
                  if (x + 1 < eina_value_array_count(v))
                    eina_strbuf_append_char(string, ',');
               }
             else
               eina_strbuf_append_printf(string, "%s ", (x + 1 < total) ? "," : "");
             eina_value_flush(&val);
          }
        if (multi)
          eina_strbuf_append_printf(string, "\n%s]", buf);
        else
          eina_strbuf_append_char(string, ']');
        break;

      case AZY_VALUE_STRUCT:
      {
         Eina_Value_Struct st;

         eina_value_pget(v, &st);
         if (!st.desc->member_count)
           {
              eina_strbuf_append(string, "{}");
              break;
           }
         multi = azy_value_list_multi_line_get_(v);
         eina_strbuf_append(string, "{ ");

         for (x = 0; x < st.desc->member_count; x++)
           {
              eina_value_struct_value_get(v, st.desc->members[x].name, &val);
              if (multi) eina_strbuf_append_printf(string, "\n%s  ", buf);
              azy_value_util_dump(&val, string, indent);
              if (multi)
                {
                   if (x + 1 < st.desc->member_count)
                     eina_strbuf_append_char(string, ',');
                }
              else
                eina_strbuf_append_printf(string, "%s ", (x + 1 < st.desc->member_count) ? "," : "");
              eina_value_flush(&val);
           }
         if (multi)
           eina_strbuf_append_printf(string, "\n%s}", buf);
         else
           eina_strbuf_append_char(string, '}');
         break;
      }

      case AZY_VALUE_INT:
      {
         int i;

         eina_value_get(v, &i);
         eina_strbuf_append_printf(string, "%d", i);
         break;
      }

      case AZY_VALUE_TIME:
      {
           time_t t;
           struct tm *tim;
           char tbuf[1024];

           eina_value_get(v, &t);
           tim = localtime(&t);
           strftime(tbuf, sizeof(tbuf), "%Y%m%dT%H:%M:%S", tim);
           eina_strbuf_append_printf(string, "\"%s\"", tbuf);
           break;
      }
      case AZY_VALUE_STRING:
      case AZY_VALUE_BASE64:
      {
         char *str;

         eina_value_get(v, &str);
         eina_strbuf_append_printf(string, "\"%s\"", str);
         break;
      }

      case AZY_VALUE_BOOL:
      {
         Eina_Bool b;

         eina_value_get(v, &b);
         eina_strbuf_append_printf(string, "%s", b ? "true" : "false");
         break;
      }

      case AZY_VALUE_DOUBLE:
      {
         double d;

         eina_value_get(v, &d);
         eina_strbuf_append_printf(string, "%g", d);
         break;
      }
      default: break;
     }
}

/** @} */
////////////////////////////////////////////

Eina_Value *
azy_value_util_int_new(int i)
{
   Eina_Value *v;

   v = eina_value_new(EINA_VALUE_TYPE_INT);
   if (v) eina_value_set(v, i);
   return v;
}

Eina_Value *
azy_value_util_double_new(double d)
{
   Eina_Value *v;

   v = eina_value_new(EINA_VALUE_TYPE_DOUBLE);
   if (v) eina_value_set(v, d);
   return v;
}

Eina_Value *
azy_value_util_bool_new(Eina_Bool b)
{
   Eina_Value *v;

   v = eina_value_new(EINA_VALUE_TYPE_UCHAR);
   if (v) eina_value_set(v, b);
   return v;
}

Eina_Value *
azy_value_util_base64_new(const char *b64)
{
   Eina_Value *v;

   v = eina_value_new(EINA_VALUE_TYPE_STRING);
   if (v) eina_value_set(v, b64);
   return v;
}

Eina_Value *
azy_value_util_string_new(const char *str)
{
   Eina_Value *v;

   v = eina_value_new(EINA_VALUE_TYPE_STRINGSHARE);
   if (v) eina_value_set(v, str);
   return v;
}

Eina_Value *
azy_value_util_time_new(time_t t)
{
   Eina_Value *v;

   v = eina_value_new(EINA_VALUE_TYPE_TIMESTAMP);
   if (v) eina_value_set(v, t);
   return v;
}

Eina_Value *
azy_value_util_time_string_new(const char *timestr)
{
   Eina_Value *v;
   struct tm tm;
   time_t t;

   if (!strptime(timestr, "%Y%m%dT%H:%M:%S", &tm)) return NULL;
   t = mktime(&tm);
   v = eina_value_new(EINA_VALUE_TYPE_TIMESTAMP);
   if (v) eina_value_set(v, t);
   return v;
}

Eina_Value *
azy_value_util_copy(const Eina_Value *val)
{
   Eina_Value *v;

   v = eina_value_new(eina_value_type_get(val));
   EINA_SAFETY_ON_NULL_RETURN_VAL(v, NULL);
   eina_value_copy(val, v);
   return v;
}

Eina_Bool
azy_value_util_string_copy(const Eina_Value *val, Eina_Stringshare **str)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(val, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(str, EINA_FALSE);
   if (!eina_value_get(val, str)) return EINA_FALSE;
   eina_stringshare_ref(*str);
   return EINA_TRUE;
}

Eina_Bool
azy_value_util_base64_copy(const Eina_Value *val, char **str)
{
   char *s;
   EINA_SAFETY_ON_NULL_RETURN_VAL(val, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(str, EINA_FALSE);
   if (!eina_value_get(val, &s)) return EINA_FALSE;
   *str = azy_util_strdup(s);
   return EINA_TRUE;
}

unsigned int
azy_value_util_type_offset(const Eina_Value_Type *type, unsigned base)
{
   unsigned size, padding;
   size = azy_value_util_type_size(type);
   if (!(base % size))
     return base;
   padding = abs(base - size);
   return base + padding;
}

size_t
azy_value_util_type_size(const Eina_Value_Type *type)
{
   if (type == EINA_VALUE_TYPE_INT) return sizeof(int32_t);
   if (type == EINA_VALUE_TYPE_UCHAR) return sizeof(unsigned char);
   if ((type == EINA_VALUE_TYPE_STRING) || (type == EINA_VALUE_TYPE_STRINGSHARE)) return sizeof(char*);
   if (type == EINA_VALUE_TYPE_TIMESTAMP) return sizeof(time_t);
   if (type == EINA_VALUE_TYPE_ARRAY) return sizeof(Eina_Value_Array);
   if (type == EINA_VALUE_TYPE_DOUBLE) return sizeof(double);
   if (type == EINA_VALUE_TYPE_STRUCT) return sizeof(Eina_Value_Struct);
   return 0;
}

Eina_Value_Struct_Desc *
azy_value_util_struct_desc_new(void)
{
   Azy_Value_Struct_Desc *st_desc;

   st_desc = calloc(1, sizeof(Azy_Value_Struct_Desc));
   EINA_SAFETY_ON_NULL_RETURN_VAL(st_desc, NULL);
   st_desc->base.version = EINA_VALUE_STRUCT_DESC_VERSION;
   st_desc->base.ops = &operations;
   return (Eina_Value_Struct_Desc*)st_desc;
}

