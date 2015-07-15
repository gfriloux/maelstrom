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
#include "cJSON.h"
#include <math.h>

static Eina_Value *azy_value_deserialize_json(cJSON *object);
static cJSON *azy_value_serialize_json(const Eina_Value *val);

static inline const Eina_Value_Type *
_azy_value_type_get(cJSON *object)
{
   switch (object->type)
     {
        case cJSON_False:
        case cJSON_True:
          return EINA_VALUE_TYPE_UCHAR;
        case cJSON_Number:
        {
         double d = object->valuedouble;

         //this is some crazy from cJSON.c for int detection
         if ((fabs(((double)object->valueint) - d) <= __DBL_EPSILON__) && (d <= __INT_MAX__) && (d >= -__INT_MAX__ - 1))
           return EINA_VALUE_TYPE_INT;
         return EINA_VALUE_TYPE_DOUBLE;
        }
        case cJSON_String: return EINA_VALUE_TYPE_STRINGSHARE;
        case cJSON_Array: return EINA_VALUE_TYPE_ARRAY;
        case cJSON_Object: return EINA_VALUE_TYPE_STRUCT;
        default: break;
     }
   return NULL;
}

static cJSON *
azy_value_serialize_basic_json(const Eina_Value *val)
{
   switch (azy_value_util_type_get(val))
     {
      case AZY_VALUE_INT:
        {
           int int_val = -1;
           eina_value_get(val, &int_val);
           return cJSON_CreateNumber(int_val);
        }

      case AZY_VALUE_TIME:
        {
           time_t t;
           struct tm *tim;
           char buf[1024];

           eina_value_get(val, &t);
           tim = localtime(&t);
           strftime(buf, sizeof(buf), "%Y%m%dT%H:%M:%S", tim);
           return cJSON_CreateString(buf);
        }

      case AZY_VALUE_STRING:
      case AZY_VALUE_BASE64:
        {
           const char *str_val;
           eina_value_get(val, &str_val);
           return cJSON_CreateString(str_val);
        }

      case AZY_VALUE_BOOL:
        {
           Eina_Bool bool_val;
           eina_value_get(val, &bool_val);

           if (bool_val)
             return cJSON_CreateTrue();

           return cJSON_CreateFalse();
        }

      case AZY_VALUE_DOUBLE:
        {
           double dbl_val = -1;
           eina_value_get(val, &dbl_val);
           return cJSON_CreateNumber(dbl_val);
        }
      default: break;
     }

   return NULL;
}

static cJSON *
azy_value_serialize_struct_json(const Eina_Value *val)
{
   Eina_Value_Struct st;
   cJSON *object;
   unsigned int x;

   eina_value_pget(val, &st);
   object = cJSON_CreateObject();
   for (x = 0; x < st.desc->member_count; x++)
     {
        Eina_Value m;

        eina_value_struct_value_get(val, st.desc->members[x].name, &m);
        cJSON_AddItemToObject(object, st.desc->members[x].name, azy_value_serialize_json(&m));
        eina_value_flush(&m);
     }
   return object;
}

static cJSON *
azy_value_serialize_array_json(const Eina_Value *val)
{
   unsigned int x, total;
   cJSON *object;

   total = eina_value_array_count(val);
   object = cJSON_CreateArray();
   for (x = 0; x < total; x++)
     {
        Eina_Value m;

        eina_value_array_value_get(val, x, &m);
        cJSON_AddItemToArray(object, azy_value_serialize_json(&m));
        eina_value_flush(&m);
     }
   return object;
}

static cJSON *
azy_value_serialize_json(const Eina_Value *val)
{
   const Eina_Value_Type *type;

   EINA_SAFETY_ON_NULL_RETURN_VAL(val, NULL);

   type = eina_value_type_get(val);
   if (type == EINA_VALUE_TYPE_ARRAY)
     return azy_value_serialize_array_json(val);
   if (type == EINA_VALUE_TYPE_STRUCT)
     return azy_value_serialize_struct_json(val);
   else if (type)
     return azy_value_serialize_basic_json(val);
   return cJSON_CreateNull();
}

static Eina_Value *
azy_value_deserialize_basic_json(cJSON *object, Eina_Value *arr)
{
   switch (object->type)
     {
      case cJSON_False:
      case cJSON_True:
        if (!arr) return azy_value_util_bool_new(object->valueint);
        eina_value_array_append(arr, object->valueint);
        return (Eina_Value*)1;

      case cJSON_Number:
      {
         double d = object->valuedouble;

         //this is some crazy from cJSON.c for int detection
         if ((fabs(((double)object->valueint) - d) <= __DBL_EPSILON__) && (d <= __INT_MAX__) && (d >= -__INT_MAX__ - 1))
           {
              if (!arr) return azy_value_util_int_new(object->valueint);
              eina_value_array_append(arr, object->valueint);
           }
         else
           {
              if (!arr) return azy_value_util_double_new(object->valuedouble);
              eina_value_array_append(arr, object->valuedouble);
           }
         return (Eina_Value*)1;
      }

      case cJSON_String:
        if (!arr) return azy_value_util_string_new(object->valuestring);
        eina_value_array_append(arr, object->valuestring);
        return (Eina_Value*)1;

      default: break;
     }
   return NULL;
}

static Eina_Value *
azy_value_deserialize_struct_json(cJSON *object)
{
   Eina_Array *st_members, *st_values;
   unsigned int offset = 0, z = 0;
   Eina_Value *value_st = NULL;
   Eina_Value_Struct_Member *members;
   Eina_Value_Struct_Desc *st_desc;
   const char *name;
   cJSON *child;

   st_desc = azy_value_util_struct_desc_new();
   st_members = eina_array_new(1);
   st_values = eina_array_new(1);

   for (child = object->child; child; child = child->next)
     {
        Eina_Value_Struct_Member *m;
        const Eina_Value_Type *type;
        Eina_Value *v;

        type = _azy_value_type_get(child);
        if (!type) goto end;
        name = child->string;
        m = (Eina_Value_Struct_Member*)calloc(1, sizeof(Eina_Value_Struct_Member));
        m->name = eina_stringshare_add(name);
        offset = azy_value_util_type_offset(type, offset);
        m->offset = offset;
        offset += azy_value_util_type_size(type);
        m->type = type;
        eina_array_push(st_members, m);

        v = azy_value_deserialize_json(child);
        if (!v) goto end;
        eina_array_push(st_values, v);
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
        eina_value_struct_value_set(value_st, members[z].name, v);
        eina_value_free(v);
     }

end:
   eina_array_free(st_members);
   eina_array_free(st_values);
   return value_st;
}

static Eina_Value *
azy_value_deserialize_array_json(cJSON *object)
{
   Eina_Value *arr;
   const Eina_Value_Type *type;
   int x = 0;
   cJSON *child;

   child = cJSON_GetArrayItem(object, 0);
   if (!child)
     return eina_value_array_new(EINA_VALUE_TYPE_STRING, 0);

   type = _azy_value_type_get(child);
   arr = eina_value_array_new(type, 0);
   for (; x < cJSON_GetArraySize(object); x++)
     {
        if (x) child = cJSON_GetArrayItem(object, x);
        if (type == EINA_VALUE_TYPE_ARRAY)
          {
             Eina_Value_Array inner_array;
             Eina_Value *data = azy_value_deserialize_array_json(child);
             if (!data) goto error;
             eina_value_get(data, &inner_array);
             eina_value_array_append(arr, inner_array);
          }
        else if (type == EINA_VALUE_TYPE_STRUCT)
          {
             Eina_Value_Struct st;
             Eina_Value *data = azy_value_deserialize_struct_json(child);
             if (!data) goto error;
             eina_value_get(data, &st);
             eina_value_array_append(arr, st);
          }
        else
          {
             if (!azy_value_deserialize_basic_json(child, arr)) goto error;
          }
     }
   return arr;
error:
   eina_value_free(arr);
   return NULL;
}

static Eina_Value *
azy_value_deserialize_json(cJSON *object)
{
   if (!object) return NULL;

   switch (object->type)
     {
      case cJSON_Object:
        return azy_value_deserialize_struct_json(object);
      case cJSON_Array:
        return azy_value_deserialize_array_json(object);
      default:
        return azy_value_deserialize_basic_json(object, NULL);
     }
   return NULL;
}

Eina_Bool
azy_content_serialize_request_json(Azy_Content *content)
{
   Eina_List *l;
   Eina_Value *v;
   cJSON *object, *params;
   char *msg;

   if ((!content) || (content->buffer))
     return EINA_FALSE;

   object = cJSON_CreateObject();
   cJSON_AddStringToObject(object, "method", content->method);

   if (content->params)
     {
        params = cJSON_CreateArray();
        EINA_LIST_FOREACH(content->params, l, v)
          cJSON_AddItemToArray(params, azy_value_serialize_json(v));

        cJSON_AddItemToObject(object, "params", params);
     }
   cJSON_AddNumberToObject(object, "id", content->id);

   if (eina_log_domain_level_check(azy_log_dom, EINA_LOG_LEVEL_DBG))
     msg = cJSON_Print(object);
   else
     msg = cJSON_PrintUnformatted(object);

   if(!msg) goto free_object;
   azy_content_buffer_set_(content, (unsigned char *)msg, strlen(msg));

   cJSON_Delete(object);
   return EINA_TRUE;

free_object:
   cJSON_Delete(object);
   return EINA_FALSE;
}

char *
azy_content_serialize_json(Eina_Value *ev)
{
   cJSON *json;
   char *data;

   json = azy_value_serialize_json(ev);
   data = cJSON_Print(json);

   cJSON_Delete(json);
   return data;
}

Eina_Bool
azy_content_serialize_response_json(Azy_Content *content)
{
   cJSON *object, *error;
   char *msg;

   if ((!content) || (content->buffer))
     return EINA_FALSE;

   if (content->error_set)
     {
        object = cJSON_CreateObject();
        cJSON_AddNullToObject(object, "result");

        error = cJSON_CreateObject();
        cJSON_AddNumberToObject(error, "code", content->faultcode);
        cJSON_AddStringToObject(error, "message", azy_content_error_message_get(content));

        cJSON_AddItemToObject(object, "error", error);
        cJSON_AddNumberToObject(object, "id", content->id);
     }
   else if (content->retval)
     {
        object = cJSON_CreateObject();
        cJSON_AddItemToObject(object, "result", azy_value_serialize_json(content->retval));
        cJSON_AddNullToObject(object, "error");
        cJSON_AddNumberToObject(object, "id", content->id);
     }
   else
     {
        DBG("No return found in %s!", __PRETTY_FUNCTION__);
        return EINA_FALSE;
     }

   if (eina_log_domain_level_check(azy_log_dom, EINA_LOG_LEVEL_DBG))
     msg = cJSON_Print(object);
   else
     msg = cJSON_PrintUnformatted(object);

   if(!msg) goto free_object;
   azy_content_buffer_set_(content, (unsigned char *)msg, strlen(msg));

   cJSON_Delete(object);
   return EINA_TRUE;

free_object:
   cJSON_Delete(object);
   return EINA_FALSE;
}

Eina_Bool
azy_content_deserialize_json(Azy_Content *content,
                             const char *buf,
                             ssize_t len  EINA_UNUSED)
{
   cJSON *object;

   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(content, EINA_FALSE);

   if (!(object = cJSON_Parse(buf)))
     {
        ERR("%s", eina_error_msg_get(AZY_ERROR_REQUEST_JSON_OBJECT));
        return EINA_FALSE;
     }

   content->retval = azy_value_deserialize_json(object);
   cJSON_Delete(object);
   return EINA_TRUE;
}

Eina_Bool
azy_content_deserialize_request_json(Azy_Content *content,
                                     const char *buf,
                                     ssize_t len  EINA_UNUSED)
{
   cJSON *object, *grab;
   int i;

   if ((!content) || (!buf))
     return EINA_FALSE;

   if (!(object = cJSON_Parse(buf)))
     {
        azy_content_error_code_set(content, AZY_ERROR_REQUEST_JSON_OBJECT);
        return EINA_FALSE;
     }

   if ((grab = cJSON_GetObjectItem(object, "id")))
     content->id = grab->valueint;

   if (!(grab = cJSON_GetObjectItem(object, "method")))
     {
        azy_content_error_code_set(content, AZY_ERROR_REQUEST_JSON_METHOD);
        cJSON_Delete(object);
        return EINA_FALSE;
     }

   content->method = eina_stringshare_add(grab->valuestring);

   grab = cJSON_GetObjectItem(object, "params");

   for (i = 0; grab && (i < cJSON_GetArraySize(grab)); i++)
     {
        Eina_Value *v;

        if (!(v = azy_value_deserialize_json(cJSON_GetArrayItem(grab, i))))
          {
             azy_content_error_faultmsg_set(content, -1, "Can't parse JSON-RPC request. Failed to deserialize parameter %d.", i);
             cJSON_Delete(object);
             return EINA_FALSE;
          }

        content->params = eina_list_append(content->params, v);
     }

   cJSON_Delete(object);
   return EINA_TRUE;
}

Eina_Bool
azy_content_deserialize_response_json(Azy_Content *content,
                                      const char *buf,
                                      ssize_t len  EINA_UNUSED)
{
   cJSON *object, *grab, *error;
   Eina_Value *ret;

   if ((!content) || (!buf))
     return EINA_FALSE;

   if (!(object = cJSON_Parse(buf)))
     {
        azy_content_error_code_set(content, AZY_ERROR_RESPONSE_JSON_OBJECT);
        return EINA_FALSE;
     }

   if ((grab = cJSON_GetObjectItem(object, "id")))
     content->id = grab->valueint;

   if ((grab = cJSON_GetObjectItem(object, "error")) && (error = cJSON_GetObjectItem(grab, "code")))
     {
        int code;
        const char *msg = NULL;
        cJSON *obj;

        code = error->valueint;
        obj = cJSON_GetObjectItem(grab, "message");
        if (obj)
          msg = obj->valuestring;

        if (msg)
          {
             Eina_Error e;

             e = eina_error_find(msg);
             if (e)
               azy_content_error_faultcode_set(content, e, code);
             else
               azy_content_error_faultmsg_set(content, code, "%s", msg);
          }
        else
          azy_content_error_code_set(content, AZY_ERROR_RESPONSE_JSON_ERROR);

        cJSON_Delete(object);
        return EINA_TRUE;
     }

   grab = cJSON_GetObjectItem(object, "result");

   if ((!grab) || (grab->type == cJSON_NULL))
     {
        azy_content_error_code_set(content, AZY_ERROR_RESPONSE_JSON_NULL);
        cJSON_Delete(object);
        return EINA_FALSE;
     }

   if (!(ret = azy_value_deserialize_json(grab)))
     {
        azy_content_error_code_set(content, AZY_ERROR_RESPONSE_JSON_INVALID);
        cJSON_Delete(object);
        return EINA_FALSE;
     }

   azy_content_retval_set(content, ret);
   cJSON_Delete(object);
   return EINA_TRUE;
}

