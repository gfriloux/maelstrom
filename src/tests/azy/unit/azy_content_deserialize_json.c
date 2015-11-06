#include "t002_json.h"

#define ck_assert_double_eq(_a, _b) do { \
  double epsilon = 0.00001; \
  ck_assert_msg(_a >=  _b - epsilon && _a <= _b +  epsilon, "Assertion '%f==%f' failed", _a, _b); \
} while (0)

START_TEST(_azy_content_deserialize_json_int1)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_int\":3}";
   Eina_Value *ev;
   T002_Integer *ti;

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

// todo : when value is not the true type for exemple string in place of int,
// int must be equal to 0.
// In this case azy_value_to_T002_Integer must display a warning message.
START_TEST(_azy_content_deserialize_json_int2)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_int\":\"3\"}";
   Eina_Value *ev;
   T002_Integer *ti;

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

// todo : when value not exist in json ignore it.
// When value of int is not found, it must be equal to 0.
// In this case azy_value_to_T002_String must display a warning message.
START_TEST(_azy_content_deserialize_json_int3)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_integer\":3}";
   Eina_Value *ev;
   T002_Integer *ti;

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

// todo : when value is not the true type for exemple int in place of string,
// string must be equal to empty.
// In this case azy_value_to_T002_String must display a warning message and not segfault...
START_TEST(_azy_content_deserialize_json_string2)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_string\":3}";
   Eina_Value *ev;
   T002_String *ts;

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

// todo : when value not exist in json ignore it.
// When value of string is not found, it must be equal to "".
// In this case azy_value_to_T002_String must display a warning message.
START_TEST(_azy_content_deserialize_json_string3)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "{\"test_stringshare\":\"damien\"}";
   Eina_Value *ev;
   T002_String *ts;

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

// todo : when value is not the true type for exemple int in place of string,
// string must be equal to empty.
// In this case azy_value_to_Array_string must display a warning message and not segfault...
START_TEST(_azy_content_deserialize_json_array_string2)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "[\"string1\",2]";
   Eina_Value *ev;
   Eina_List *el = NULL,
             *el2;
   Eina_Stringshare *data;

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
   ck_assert_str_eq( (char *)eina_list_nth(el, 1), "");

   EINA_LIST_FOREACH(el, el2, data) eina_stringshare_del(data);
   azy_content_free(content);
}
END_TEST

START_TEST(_azy_content_deserialize_json_array_string3)
{
   Azy_Content *content;
   Eina_Bool r;
   const char *s = "";

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
   ck_assert_msg(!!ts->computers, "Error while getting ts->network.");
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

   content = azy_content_new(NULL);
   ck_assert_msg(!!content, "Error while allocating content.");

   r = azy_content_deserialize_json(content, s, strlen(s));
   ck_assert_msg(r, "Error while deserializing json.");

   ev = azy_content_retval_get(content);
   ck_assert_msg(!!ev, "Error, azy_content_retval_get must return NULL.");

   r = azy_value_to_T002_Server(ev, &ts);
   ck_assert_msg(!r, "Error, azy_value_to_Array_string must return EINA_FALSE.");
   ck_assert_msg(!ts, "Error, value of ts isn't NULL.");

   azy_content_free(content);
}
END_TEST

// todo : when value is not present in json init it and continue parsing.
// ->key: abc123
// ->initialize: yes
// ->connexions: 3
// ->hash:
// ->volume: 0
// ->network:
// ->->ip: (null)
// ->->gateway: (null)
// ->->netmask: (null)
// ->->dns: (null)
// ->computers:
// ->licence:
// Currently, the data after hash are not set.
// In this case azy_value_to_T002_Struct must display a warning message.
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
   ck_assert_msg(!!ts->computers, "Error while getting ts->network.");
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

   return tc_tests;
}
