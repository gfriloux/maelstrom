#include "t002_json.h"

#define ck_assert_double_eq(_a, _b) do {                                                          \
  double epsilon = 0.00001;                                                                       \
  ck_assert_msg(_a >=  _b - epsilon && _a <= _b +  epsilon, "Assertion '%f==%f' failed", _a, _b); \
} while (0)

#define PRINT(_func, _test, _result) do {                                                                           \
  printf("\n\n\e[4mFunction : %s, line : %d\e[24m\n- test : %s\n- result : %s\n", _func, __LINE__, _test, _result); \
} while (0)


START_TEST(_azy_content_deserialize_json_int1)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_int\":3}";
   Eina_Value *ev;
   T002_Integer *ti;

   PRINT("_azy_content_deserialize_json_int1", "correct int data.", "test_int = 3.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_Integer(ev, &ti);
   ck_assert_msg(r, "Error while getting value of ti.");
   ck_assert_msg(!!ti, "Error, value of ti is NULL.");
   ck_assert_int_eq(ti->test_int, 3);

   T002_Integer_free(ti);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_int2)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_int\":\"3\"}";
   Eina_Value *ev;
   T002_Integer *ti;

   PRINT("_azy_content_deserialize_json_int2",
         "bad type of value for data test_int, string in place of int.",
         "tests_int = 0.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_Integer(ev, &ti);
   ck_assert_msg(r, "Error while getting value of ti.");
   ck_assert_msg(!!ti, "Error, value of ti is NULL.");
   ck_assert_int_eq(ti->test_int, 0);

   T002_Integer_free(ti);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_int3)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_integer\":3}";
   Eina_Value *ev;
   T002_Integer *ti;

   PRINT("_azy_content_deserialize_json_int3", "data test_int does not exit.",
         "\n-- test_int = 0,\n-- test_integer is ignored.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_Integer(ev, &ti);
   ck_assert_msg(r, "Error while getting value of ti.");
   ck_assert_msg(!!ti, "Error, value of ti is not NULL.");
   ck_assert_int_eq(ti->test_int, 0);

   T002_Integer_free(ti);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_int4)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "";

   PRINT("_azy_content_deserialize_json_int4",
         "json is not a in valid format, equal to empty string.",
         "azy_content_deserialize_json return an error.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(!r, "Error, azy_content_deserialize_json must return EINA_FALSE.");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_int5)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{}";
   Eina_Value *ev;

   PRINT("_azy_content_deserialize_json_int5", "json data is empty, equal to {}.",
         "azy_content_retval_get return NULL.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!ev, "Error, azy_content_retval_get must return NULL.");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_int6)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_int\":}";

   PRINT("_azy_content_deserialize_json_int6",
         "json is not in a valid format, value of data is not set.",
         "azy_content_deserialize_json return an error.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(!r, "Error, azy_content_deserialize_json must return EINA_FALSE.");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_double)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_double\":3.3}";
   Eina_Value *ev;
   T002_Double *td;

   PRINT("_azy_content_deserialize_json_double", "correct double data.",
         "test_double = 3.3.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_Double(ev, &td);
   ck_assert_msg(r, "Error while getting value of td.");
   ck_assert_msg(!!td, "Error, value of td is NULL.");
   ck_assert_double_eq(td->test_double, 3.3);

   T002_Double_free(td);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_string1)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_string\":\"damien\"}";
   Eina_Value *ev;
   T002_String *ts;

   PRINT("_azy_content_deserialize_json_string1", "correct string data.",
         "test_string = damien.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_String(ev, &ts);
   ck_assert_msg(r, "Error while getting value of ts.");
   ck_assert_msg(!!ts, "Error, value of ts is NULL.");
   ck_assert_str_eq(ts->test_string, "damien");

   T002_String_free(ts);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_string2)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_string\":3}";
   Eina_Value *ev;
   T002_String *ts;

   PRINT("_azy_content_deserialize_json_string2",
         "bad type of value for data test_string, int in place of string.",
         "test_string = \"\".");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_String(ev, &ts);
   ck_assert_msg(r, "Error while getting value of ts.");
   ck_assert_msg(!!ts, "Error, value of ts is NULL.");
   ck_assert_str_eq(ts->test_string, "");

   T002_String_free(ts);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_string3)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_stringshare\":\"damien\"}";
   Eina_Value *ev;
   T002_String *ts;

   PRINT("_azy_content_deserialize_json_string3", "data test_string does not exist.",
         "\n-- test_string = \"\",\n-- test_stringshare is ignored.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_String(ev, &ts);
   ck_assert_msg(r, "Error while getting value of ts.");
   ck_assert_msg(!!ts, "Error, value of ts is not NULL.");
   ck_assert_str_eq(ts->test_string, "");

   T002_String_free(ts);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_string4)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "";

   PRINT("_azy_content_deserialize_json_string4",
         "json is not a in valid format, equal to empty string.",
         "azy_content_deserialize_json return an error.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(!r, "Error, azy_content_deserialize_json must return EINA_FALSE.");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_string5)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{}";
   Eina_Value *ev;

   PRINT("_azy_content_deserialize_json_string5", "json data is empty, equal to {}.",
         "azy_content_retval_get return NULL.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!ev, "Error, azy_content_retval_get must return NULL.");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_string6)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_string\":}";

   PRINT("_azy_content_deserialize_json_string6",
         "json is not in a valid format, value of data is not set.",
         "azy_content_deserialize_json return an error.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(!r, "Error, azy_content_deserialize_json must return EINA_FALSE.");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_array_string1)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "[\"string1\",\"string2\"]";
   Eina_Value *ev;
   Eina_List *el = NULL,
             *el2;
   Eina_Stringshare *data;

   PRINT("_azy_content_deserialize_json_array_string1", "correct array string.",
         "\n-- eina_list_count = 2,\n-- first element = \"string1\","
         "\n-- second element = \"string2\".");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_Array_string(ev, &el);
   ck_assert_msg(r, "Error while getting eina list.");
   ck_assert_msg(!!el, "Error, value of el is NULL.");
   ck_assert_int_eq(eina_list_count(el), 2);
   ck_assert_str_eq( (char *)eina_list_nth(el, 0), "string1");
   ck_assert_str_eq( (char *)eina_list_nth(el, 1), "string2");

   EINA_LIST_FOREACH(el, el2, data) eina_stringshare_del(data);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_array_string2)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "[\"string1\",2,\"string3\"]";
   Eina_Value *ev;
   Eina_List *el = NULL,
             *el2;
   Eina_Stringshare *data;

   PRINT("_azy_content_deserialize_json_array_string2",
         "array contain data with a different type (string and int).",
         "\n-- eina_value must contain only one type, the reference type is the first "
         "type found (here string),"
         "\n-- data of type int is ignored,\n-- eina_list_count = 2,"
         "\n-- first element = \"string1\","
         "\n-- second element = \"string3\".");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_Array_string(ev, &el);
   ck_assert_msg(r, "Error while getting eina list.");
   ck_assert_msg(!!el, "Error, value of el is NULL.");
   ck_assert_int_eq(eina_list_count(el), 2);
   ck_assert_str_eq( (char *)eina_list_nth(el, 0), "string1");
   ck_assert_str_eq( (char *)eina_list_nth(el, 1), "string3");

   EINA_LIST_FOREACH(el, el2, data) eina_stringshare_del(data);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_array_string3)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "";

   PRINT("_azy_content_deserialize_json_array_string3",
         "json is not a in valid format, equal to empty string.",
         "azy_content_deserialize_json return an error.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(!r, "Error, azy_content_deserialize_json must return EINA_FALSE.");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_array_string4)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "[]";
   Eina_Value *ev;
   Eina_List *el = NULL;

   PRINT("_azy_content_deserialize_json_array_string4",
         "array is empty.", "eina_list = NULL.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_Array_string(ev, &el);
   ck_assert_msg(r, "Error while getting eina list.");
   ck_assert_msg(!el, "Error, value of el isn't NULL.");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_array_string5)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{}";
   Eina_Value *ev;
   Eina_List *el = NULL;

   PRINT("_azy_content_deserialize_json_array_string5",
         "json data is wrong, equal to {}.", "eina_list = NULL.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!ev, "Error, azy_content_retval_get must return NULL.");

   r = azy_value_to_Array_string(ev, &el);
   ck_assert_msg(!r, "Error, azy_value_to_Array_string must return EINA_FALSE.");
   ck_assert_msg(!el, "Error, value of el isn't NULL.");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_array_string6)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "[\"string1\",]";

   PRINT("_azy_content_deserialize_json_array_string6",
         "json is not in a valid format, value of one data is not set.",
         "azy_content_deserialize_json return an error.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(!r, "Error, azy_content_deserialize_json must return EINA_FALSE.");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_struct1)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{"
                      "\"key\":\"abc123\","
                      "\"initialize\":true,"
                      "\"connexions\":3,"
                      "\"hash\":\"blabla\","
                      "\"volume\":12.56,"
                      "\"network\": {"
                         "\"ip\":\"192.168.4.4\","
                         "\"gateway\":\"192.168.4.1\","
                         "\"netmask\":\"255.255.255.0\","
                         "\"dns\":\"192.168.4.1\""
                      "},"
                      "\"computers\": ["
                         "{"
                            "\"name\":\"sango\","
                            "\"ip\":\"192.168.4.145\""
                         "},"
                         "{"
                            "\"name\":\"nath\","
                            "\"ip\":\"192.168.4.146\""
                         "}"
                      "],"
                      "\"licence\":\"lic123\""
                   "}";
   Eina_Value *ev;
   T002_Server *ts;
   T002_Computer *tc;

   PRINT("_azy_content_deserialize_json_struct1", "correct struct data.",
         "all values are correct (int, double, boolean, string, array, struct).");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_Server(ev, &ts);
   ck_assert_msg(r, "Error while getting value of ts.");
   ck_assert_msg(!!ts, "Error, value of ts is NULL.");
   ck_assert_str_eq(ts->key, "abc123");
   ck_assert_int_eq(ts->initialize, EINA_TRUE);
   ck_assert_int_eq(ts->connexions, 3);
   ck_assert_str_eq(ts->hash, "blabla");
   ck_assert_double_eq(ts->volume, 12.56);
   ck_assert_msg(!!ts->network, "Error while getting ts->network.");
   ck_assert_str_eq(ts->network->ip, "192.168.4.4");
   ck_assert_str_eq(ts->network->gateway, "192.168.4.1");
   ck_assert_str_eq(ts->network->netmask, "255.255.255.0");
   ck_assert_str_eq(ts->network->dns, "192.168.4.1");
   ck_assert_msg(!!ts->computers, "Error while getting ts->computers.");
   ck_assert_int_eq(eina_list_count(ts->computers), 2);
   tc = eina_list_nth(ts->computers, 0);
   ck_assert_str_eq(tc->name, "sango");
   ck_assert_str_eq(tc->ip, "192.168.4.145");
   tc = eina_list_nth(ts->computers, 1);
   ck_assert_str_eq(tc->name, "nath");
   ck_assert_str_eq(tc->ip, "192.168.4.146");
   ck_assert_str_eq(ts->licence, "lic123");

   T002_Server_free(ts);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_struct2)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "";

   PRINT("_azy_content_deserialize_json_struct2",
         "json is not a in valid format, equal to empty string.",
         "azy_content_deserialize_json return an error.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(!r, "Error, azy_content_deserialize_json must return EINA_FALSE.");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_struct3)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{}";
   Eina_Value *ev;

   PRINT("_azy_content_deserialize_json_struct3", "json data is empty, equal to {}.",
         "azy_content_retval_get return NULL.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!ev, "Error, azy_content_retval_get must return NULL.");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_struct4)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "[]";
   Eina_Value *ev;
   T002_Server *ts;

   PRINT("_azy_content_deserialize_json_struct4",
         "json data is an invalid array equal to [].",
         "all values are set (int = 0, double = 0, boolean = EINA_FALSE, string = \"\","
         "array = [], struct = {}).");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error, azy_content_retval_get must return NULL.");

   r = azy_value_to_T002_Server(ev, &ts);
   ck_assert_msg(r, "Error while getting value of ts.");
   ck_assert_msg(!!ts, "Error, value of ts is NULL.");
   ck_assert_str_eq(ts->key, "");
   ck_assert_int_eq(ts->initialize, EINA_FALSE);
   ck_assert_int_eq(ts->connexions, 0);
   ck_assert_str_eq(ts->hash, "");
   ck_assert_double_eq(ts->volume, 0);
   ck_assert_msg(!!ts->network, "Error while getting ts->network.");
   ck_assert_str_eq(ts->network->ip, "");
   ck_assert_str_eq(ts->network->gateway, "");
   ck_assert_str_eq(ts->network->netmask, "");
   ck_assert_str_eq(ts->network->dns, "");
   ck_assert_msg(!ts->computers, "Error, ts->computers must be empty list.");
   ck_assert_int_eq(eina_list_count(ts->computers), 0);
   ck_assert_str_eq(ts->licence, "");

   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_struct5)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{"
                      "\"key\":\"abc123\","
                      "\"initialize\":true,"
                      "\"connexions\":3,"
                      "\"volume\":12.56,"
                      "\"network\": {"
                         "\"ip\":\"192.168.4.4\","
                         "\"gateway\":\"192.168.4.1\","
                         "\"netmask\":\"255.255.255.0\","
                         "\"dns\":\"192.168.4.1\""
                      "},"
                      "\"computers\": ["
                         "{"
                            "\"name\":\"sango\","
                            "\"ip\":\"192.168.4.145\""
                         "},"
                         "{"
                            "\"name\":\"nath\","
                            "\"ip\":\"192.168.4.146\""
                         "}"
                      "],"
                      "\"licence\":\"lic123\""
                   "}";
   Eina_Value *ev;
   T002_Server *ts;
   T002_Computer *tc;

   PRINT("_azy_content_deserialize_json_struct5", "data hash does not exist.",
         "if a data does not exist in middle of json, this data must be init and the "
         "next datas must be set to their values.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_Server(ev, &ts);
   ck_assert_msg(r, "Error while getting value of ts.");
   ck_assert_msg(!!ts, "Error, value of ts is NULL.");
   ck_assert_str_eq(ts->key, "abc123");
   ck_assert_int_eq(ts->initialize, EINA_TRUE);
   ck_assert_int_eq(ts->connexions, 3);
   ck_assert_str_eq(ts->hash, "");
   ck_assert_double_eq(ts->volume, 12.56);
   ck_assert_msg(!!ts->network, "Error while getting ts->network.");
   ck_assert_str_eq(ts->network->ip, "192.168.4.4");
   ck_assert_str_eq(ts->network->gateway, "192.168.4.1");
   ck_assert_str_eq(ts->network->netmask, "255.255.255.0");
   ck_assert_str_eq(ts->network->dns, "192.168.4.1");
   ck_assert_msg(!!ts->computers, "Error while getting ts->computers.");
   ck_assert_int_eq(eina_list_count(ts->computers), 2);
   tc = eina_list_nth(ts->computers, 0);
   ck_assert_str_eq(tc->name, "sango");
   ck_assert_str_eq(tc->ip, "192.168.4.145");
   tc = eina_list_nth(ts->computers, 1);
   ck_assert_str_eq(tc->name, "nath");
   ck_assert_str_eq(tc->ip, "192.168.4.146");
   ck_assert_str_eq(ts->licence, "lic123");

   T002_Server_free(ts);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_boolean1)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_boolean\":true}";
   Eina_Value *ev;
   T002_Boolean *tb;

   PRINT("_azy_content_deserialize_json_boolean1", "true boolean.",
         "test_boolean = EINA_TRUE.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_Boolean(ev, &tb);
   ck_assert_msg(r, "Error while getting value of tb.");
   ck_assert_msg(!!tb, "Error, value of tb is NULL.");
   ck_assert_int_eq(tb->test_boolean, EINA_TRUE);

   T002_Boolean_free(tb);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_boolean2)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_boolean\":false}";
   Eina_Value *ev;
   T002_Boolean *tb;

   PRINT("_azy_content_deserialize_json_boolean2", "false boolean.",
         "test_boolean = EINA_FALSE.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_Boolean(ev, &tb);
   ck_assert_msg(r, "Error while getting value of tb.");
   ck_assert_msg(!!tb, "Error, value of tb is NULL.");
   ck_assert_int_eq(tb->test_boolean, EINA_FALSE);

   T002_Boolean_free(tb);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_boolean3)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_boolean\":2}";
   Eina_Value *ev;
   T002_Boolean *tb;

   PRINT("_azy_content_deserialize_json_boolean3",
         "bad type of value for data test_boolean, int in place of boolean.",
         "test_boolean = EINA_FALSE.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_Boolean(ev, &tb);
   ck_assert_msg(r, "Error while getting value of tb.");
   ck_assert_msg(!!tb, "Error, value of tb is NULL.");
   ck_assert_int_eq(tb->test_boolean, EINA_FALSE);

   T002_Boolean_free(tb);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_boolean4)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_boolean\":\"\"}";
   Eina_Value *ev;
   T002_Boolean *tb;

   PRINT("_azy_content_deserialize_json_boolean4",
         "bad type of value for data test_boolean, string in place of boolean.",
         "test_boolean = EINA_FALSE.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_Boolean(ev, &tb);
   ck_assert_msg(r, "Error while getting value of tb.");
   ck_assert_msg(!!tb, "Error, value of tb is NULL.");
   ck_assert_int_eq(tb->test_boolean, EINA_FALSE);

   T002_Boolean_free(tb);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_boolean5)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_bool\":true}";
   Eina_Value *ev;
   T002_Boolean *tb;

   PRINT("_azy_content_deserialize_json_boolean5",
         "data test_boolean does not exit.",
         "\n--test_boolean = EINA_FALSE,\n--test_bool is ignored.");

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error while getting retval.");

   r = azy_value_to_T002_Boolean(ev, &tb);
   ck_assert_msg(r, "Error while getting value of tb.");
   ck_assert_msg(!!tb, "Error, value of tb is NULL.");
   ck_assert_int_eq(tb->test_boolean, EINA_FALSE);

   T002_Boolean_free(tb);
   azy_content_free(content);
}
END_TEST

TCase *
azy_content_deserialize_json_init()
{
   TCase *tc_tests = tcase_create("Tests azy_content_deserialize_json");

   tcase_add_test(tc_tests, _azy_content_deserialize_json_int1);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_int2);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_int3);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_int4);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_int5);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_int6);

   tcase_add_test(tc_tests, _azy_content_deserialize_json_double);

   tcase_add_test(tc_tests, _azy_content_deserialize_json_string1);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_string2);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_string3);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_string4);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_string5);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_string6);

   tcase_add_test(tc_tests, _azy_content_deserialize_json_array_string1);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_array_string2);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_array_string3);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_array_string4);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_array_string5);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_array_string6);

   tcase_add_test(tc_tests, _azy_content_deserialize_json_struct1);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_struct2);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_struct3);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_struct4);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_struct5);

   tcase_add_test(tc_tests, _azy_content_deserialize_json_boolean1);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_boolean2);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_boolean3);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_boolean4);
   tcase_add_test(tc_tests, _azy_content_deserialize_json_boolean5);

   return tc_tests;
}
