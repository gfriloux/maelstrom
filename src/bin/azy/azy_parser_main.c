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
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Getopt.h>

#include "azy.h"

/* codegen helpers */

#define EL(i, fmt, args ...) \
  line_fprintf(i, fmt "\n", ## args)

#define E(i, fmt, args ...) \
  line_fprintf(i, fmt, ## args)

#define NL \
  line_fprintf(0, "\n")

#define S(args ...) \
  eina_stringshare_printf(args)

#define OPEN(fmt, args ...) {                                       \
     f = fopen(current_file = S(fmt, ## args), "w");                \
     if (!f) {                                                      \
          fprintf(stderr, "Can't open output file for writing.\n"); \
          exit(1); }                                                \
}                                                                   \
  current_line = 1

#define ORIG_LINE() \
  EL(0, "#line %d \"%s\"", current_line + 1, current_file)

#define GET_NAME(func)                                 \
  if (s->stub_##func && s->stub_##func[0])             \
    E(0, "%s%s%s_module_" #func, name, sep, s->name);  \
  else                                                 \
    E(0, "NULL")

#define STUB(s)                                                 \
  do {                                                          \
       if (s)                                                   \
         {                                                      \
            EL(0, "#line %d \"%s\"", s ## _line + 1, azy_file); \
            EL(0, "%s", s);                                     \
            ORIG_LINE();                                        \
         }                                                      \
    } while(0)

static int current_line;
static const char *current_file = NULL;

static Azy_Model *azy;
static Eina_Bool client_headers = EINA_FALSE;
static Eina_Bool client_impl = EINA_FALSE;
static Eina_Bool common_headers = EINA_FALSE;
static Eina_Bool common_impl = EINA_FALSE;
static Eina_Bool server_impl = EINA_FALSE;
static Eina_Bool server_headers = EINA_FALSE;
static Eina_Bool azy_gen = EINA_FALSE;
static Eina_Bool hash_funcs = EINA_FALSE;
static Eina_Bool isnull_funcs = EINA_FALSE;
static Eina_Bool print_funcs = EINA_FALSE;
static Eina_Bool eq_funcs = EINA_FALSE;
static Eina_Bool suspend_funcs = EINA_FALSE;
static char *out_dir = ".";
static char *azy_file;
static FILE *f;
static Eina_Stringshare *i, *b, *d, *c, *e, *ti, *b64;
static const char *sep;
static const char *name;

static const Ecore_Getopt opts = {
   "Azy_Parser",
   "Azy_Parser file.azy -o destination_directory/",
   "1.0alpha",
   "(C) 2010, 2011, 2012 Mike Blumenkrantz, previously others, see AUTHORS",
   "LGPL",
   "Parse an azy file into *.{c,h} files\n\n",
   1,
   {
      ECORE_GETOPT_STORE_STR('m', "modes", "Parser modes: all, server-impl, server-headers,\n"
                                           "\t\t\tclient-impl, client-headers,\n"
                                           "\t\t\tcommon-impl, common-headers"),
      ECORE_GETOPT_STORE_STR('o', "output", "Output directory (default is .)"),
      ECORE_GETOPT_STORE_TRUE('d', "debug", "Print debugging output"),
      ECORE_GETOPT_STORE_TRUE('H', "Hash", "Do not generate hash functions"),
      ECORE_GETOPT_STORE_TRUE('n', "null", "Do not generate isnull functions"),
      ECORE_GETOPT_STORE_TRUE('p', "print", "Do not generate print functions"),
      ECORE_GETOPT_STORE_TRUE('e', "eq", "Do not generate eq functions"),
      ECORE_GETOPT_STORE_TRUE('s', "suspend", "Suspend methods by default"),
      ECORE_GETOPT_VERSION('V', "version"),
      ECORE_GETOPT_COPYRIGHT('R', "copyright"),
      ECORE_GETOPT_LICENSE('L', "license"),
      ECORE_GETOPT_HELP('h', "help"),
      ECORE_GETOPT_SENTINEL
   }
};

extern void azy_parser_Trace(FILE *,
                             char *);
static void
line_fprintf(unsigned int indent,
             const char *fmt,
             ...)
{
   unsigned int x;
   va_list ap;

   for (x = 0; x < indent; x++)
     fprintf(f, "  ");

   va_start(ap, fmt);
   vfprintf(f, fmt, ap);
   va_end(ap);

   for (x = 0; x < strlen(fmt); x++)
     if (fmt[x] == '\n')
       current_line++;
}

static void
gen_type_marshalizers(Azy_Typedef *t,
                      Eina_Bool header)
{
   Eina_List *l;
   Azy_Struct_Member *m;
   unsigned int x = 0;

   if (header)
     {
        if (t->type == TD_STRUCT)
          {
             EL(0, "Eina_Value *%s(const %s azy_user_type) EINA_WARN_UNUSED_RESULT;", t->march_name,
                ((t->ctype == i) || (t->ctype == b)) ? "int32_t" : t->ctype);
             EL(0, "Eina_Bool %s(const Eina_Value *value_struct, %s* azy_user_type);",
                t->demarch_name, t->ctype);
          }
        else
          {
             if (t->type == TD_BASE) return;
             EL(0, "Eina_Value *%s(const %s azy_user_type) EINA_WARN_UNUSED_RESULT;",
                t->march_name, t->ctype);
             EL(0, "Eina_Bool %s(const Eina_Value *value_array, %s* azy_user_type);",
                t->demarch_name, t->ctype);
          }
        return;
     }

   if (t->type == TD_STRUCT)
     {
        /* marshalizers */
        EL(0, "Eina_Value *%s(const %s azy_user_type)", t->march_name,
           ((t->ctype == i) || (t->ctype == b)) ? "int32_t" : t->ctype);
        EL(0, "{");
        EL(1, "Eina_Value *value_struct = NULL;");
        EL(1, "size_t offset = 0;");
        EL(1, "Eina_Value_Struct_Member *members = NULL;");
        EL(1, "Eina_Value *val = NULL;");
        EL(1, "Eina_Value_Struct_Desc *st_desc;");

        NL;
        EL(1, "if (!azy_user_type) return NULL;");
        NL;

        EL(1, "st_desc = azy_value_util_struct_desc_new();");
        EL(1, "members = malloc(%u * sizeof(Eina_Value_Struct_Member));", eina_list_count(t->struct_members));
        EL(1, "st_desc->members = members;");
        EL(1, "st_desc->member_count = %u;", eina_list_count(t->struct_members));

        EINA_LIST_FOREACH(t->struct_members, l, m)
          {
             EL(1, "members[%u].name = eina_stringshare_add(\"%s\");", x, m->strname ? m->strname : m->name);
             EL(1, "offset = azy_value_util_type_offset(%s, offset);", m->type->etype);
             EL(1, "members[%u].offset = offset;", x);
             EL(1, "offset += azy_value_util_type_size(%s);", m->type->etype);
             EL(1, "members[%u].type = %s;", x, m->type->etype);
             x++;
          }
        EL(1, "st_desc->size = offset;");
        EL(1, "value_struct = eina_value_struct_new(st_desc);");
        NL;

        EINA_LIST_FOREACH(t->struct_members, l, m)
          {
             EL(1, "val = %s(azy_user_type->%s);", m->type->march_name, m->name);
             EL(1, "eina_value_struct_value_set(value_struct, \"%s\", val);", m->strname ? m->strname : m->name);
             EL(1, "eina_value_free(val);");
          }


        EL(1, "return value_struct;");
        EL(0, "}");
        NL;

        /* demarshalizers */
        EL(0, "Eina_Bool %s(const Eina_Value *value_struct, %s *azy_user_type)",
           t->demarch_name, t->ctype);
        EL(0, "{");
        EL(1, "%s azy_user_type_tmp = NULL;", t->ctype);
        EL(1, "Eina_Value val;");
        /* hack to find eldbus-mangled struct members */
        EL(1, "unsigned int arg = 0;");
        EL(1, "Eina_Bool found = EINA_FALSE;");
        NL;
        EL(1, "EINA_SAFETY_ON_NULL_RETURN_VAL(azy_user_type, EINA_FALSE);");
        EL(1, "EINA_SAFETY_ON_NULL_RETURN_VAL(value_struct, EINA_FALSE);");
        NL;
        EL(1, "azy_user_type_tmp = %s_new();", t->cname);

        EINA_LIST_FOREACH(t->struct_members, l, m)
          {
             NL;
             EL(1, "if(eina_value_type_get(value_struct) != EINA_VALUE_TYPE_STRUCT) found = EINA_FALSE;");
             EL(1, "else if ((!arg) && eina_value_struct_value_get(value_struct, \"%s\", &val))", m->strname ? m->strname : m->name);
             EL(2, "found = EINA_TRUE;");
             EL(1, "else");
             EL(1, "{");
             EL(2, "char buf[128];");
             EL(2, "snprintf(buf, sizeof(buf), \"arg%%d\", arg++);");
             EL(2, "if (eina_value_struct_value_get(value_struct, buf, &val))");
             EL(3, "found = EINA_TRUE;");
             EL(2, "else arg--;");
             EL(1, "}");

             if (m->type->ctype == c) //eldbus does NOT stringshare. bastard.
               {
                  EL(1, "if (!found || eina_value_type_get(&val) != EINA_VALUE_TYPE_STRINGSHARE)");
                  EL(1, "{");
                  EL(2, "EINA_LOG_WARN(\"%s is not found or not in true type set to empty string.\");", m->name);
                  EL(2, "azy_user_type_tmp->%s = eina_stringshare_add(\"\");", m->name);
                  EL(1, "}");
                  EL(1, "else");
                  EL(1, "{");
                  EL(2, "if (arg)");
                  EL(2, "{");
                  EL(3, "char *str = NULL;");
                  NL;
                  EL(3, "if (eina_value_get(&val, &str))");
                  EL(4, "azy_user_type_tmp->%s = eina_stringshare_add(str);", m->name);
                  EL(2, "}");
                  EL(2, "else");
                  EL(3, "%s(&val, &azy_user_type_tmp->%s);", m->type->demarch_name, m->name);
                  EL(2, "eina_value_flush(&val);");
                  EL(1, "}");
               }
             else if (m->type->ctype == d)
               {
                  EL(1, "if (!found)");
                  EL(1, "{");
                  EL(2, "EINA_LOG_WARN(\"%s is not found set to 0.\");", m->name);
                  EL(2, "azy_user_type_tmp->%s = 0;", m->name);
                  EL(1, "}");
                  EL(1, "else if(eina_value_type_get(&val) == EINA_VALUE_TYPE_DOUBLE || "
                                 "eina_value_type_get(&val) == EINA_VALUE_TYPE_FLOAT)");
                  EL(2, "%s(&val, &azy_user_type_tmp->%s);", m->type->demarch_name, m->name);
                  EL(1, "else if(eina_value_type_get(&val) == EINA_VALUE_TYPE_INT || "
                                 "eina_value_type_get(&val) == EINA_VALUE_TYPE_LONG || "
                                 "eina_value_type_get(&val) == EINA_VALUE_TYPE_INT64)");
                  EL(1, "{");
                  EL(2, "int int_to_double;");
                  EL(2, "%s(&val, &int_to_double);", m->type->demarch_name);
                  EL(2, "azy_user_type_tmp->%s = (double) int_to_double;", m->name);
                  EL(1, "}");
                  EL(1, "else");
                  EL(1, "{");
                  EL(2, "EINA_LOG_WARN(\"%s not in true type set to 0.\");", m->name);
                  EL(2, "azy_user_type_tmp->%s = 0;", m->name);
                  EL(1, "}");
               }
             else if (m->type->ctype == i)
               {
                  EL(1, "if (!found)");
                  EL(1, "{");
                  EL(2, "EINA_LOG_WARN(\"%s is not found set to 0.\");", m->name);
                  EL(2, "azy_user_type_tmp->%s = 0;", m->name);
                  EL(1, "}");
                  EL(1, "else if(eina_value_type_get(&val) == EINA_VALUE_TYPE_INT || "
                                 "eina_value_type_get(&val) == EINA_VALUE_TYPE_LONG || "
                                 "eina_value_type_get(&val) == EINA_VALUE_TYPE_INT64)");
                  EL(2, "%s(&val, &azy_user_type_tmp->%s);", m->type->demarch_name, m->name);
                  EL(1, "else");
                  EL(1, "{");
                  EL(2, "EINA_LOG_WARN(\"%s not in true type set to 0.\");", m->name);
                  EL(2, "azy_user_type_tmp->%s = 0;", m->name);
                  EL(1, "}");
               }
             else if (m->type->ctype == b)
               {
                  EL(1, "if (!found)");
                  EL(1, "{");
                  EL(2, "EINA_LOG_WARN(\"%s is not found set to EINA_FALSE.\");", m->name);
                  EL(2, "azy_user_type_tmp->%s = EINA_FALSE;", m->name);
                  EL(1, "}");
                  EL(1, "else");
                  EL(1, "{");
                  EL(2, "%s(&val, &azy_user_type_tmp->%s);", m->type->demarch_name, m->name);
                  EL(2, "if(azy_user_type_tmp->%s < 0 || azy_user_type_tmp->%s > 1)", m->name, m->name);
                  EL(2, "{");
                  EL(3, "EINA_LOG_WARN(\"%s not in true type set to EINA_FALSE.\");", m->name);
                  EL(3, "azy_user_type_tmp->%s = EINA_FALSE;", m->name);
                  EL(2, "}");
                  EL(1, "}");
               }
             else
               {
                  EL(1, "if(!found)");
                  EL(2, "EINA_LOG_WARN(\"%s is not found.\");", m->name);
                  EL(1, "%s(&val, &azy_user_type_tmp->%s);", m->type->demarch_name, m->name);
                  EL(1, "eina_value_flush(&val);");
               }

             if(eina_list_next(l)) EL(1, "found = EINA_FALSE;");
          }

        NL;
        EL(1, "*azy_user_type = azy_user_type_tmp;");
        EL(1, "return EINA_TRUE;");
        EL(0, "}");
        NL;
     }
   else if (t->type == TD_ARRAY)
     {
        /* marshalizers */
        const char *type;
        if ((t->item_type->ctype == i) || (t->item_type->ctype == b) || (t->item_type->ctype == ti))
          type = "int32_t *";
        else if (t->item_type->ctype == d)
          type = "double *";
        else
          type = t->item_type->ctype;
        EL(0, "Eina_Value *%s(const %s azy_user_type)", t->march_name, t->ctype);
        EL(0, "{");
        EL(1, "const Eina_List *l;");
        if (t->item_type->type == TD_ARRAY)
          EL(1, "Eina_Value_Array arr;");
        else if (t->item_type->type == TD_STRUCT)
          EL(1, "Eina_Value_Struct st;");
        EL(1, "%s v;", type);
        EL(1, "Eina_Value *value_array = eina_value_array_new(%s, 0);", t->item_type->etype);
        NL;
        EL(1, "EINA_LIST_FOREACH(azy_user_type, l, v)");
        EL(1, "{");
        if ((t->item_type->type == TD_ARRAY) || (t->item_type->type == TD_STRUCT))
          {
             if (t->item_type->ctype == d)
               EL(2, "Eina_Value *item_value = %s(*v);", t->item_type->march_name);
             else if (!t->item_type->free_func)
               EL(2, "Eina_Value *item_value = %s((long)(intptr_t)v);", t->item_type->march_name);
             else
               EL(2, "Eina_Value *item_value = %s((%s)v);", t->item_type->march_name, t->item_type->ctype);
          }

        NL;
        if (t->item_type->type == TD_ARRAY)
          {
             EL(2, "eina_value_get(item_value, &arr);");
             EL(2, "eina_value_array_append(value_array, arr);");
             EL(2, "eina_value_free(item_value);");
          }
        else if (t->item_type->type == TD_STRUCT)
          {
             EL(2, "eina_value_get(item_value, &st);");
             EL(2, "eina_value_array_append(value_array, st);");
             EL(2, "eina_value_free(item_value);");
          }
        else
          EL(2, "eina_value_array_append(value_array, v);");
        EL(1, "}");
        NL;
        EL(1, "return value_array;");
        EL(0, "}");
        NL;

        /* demarshalizers */
        EL(0, "Eina_Bool %s(const Eina_Value *value_array, %s* azy_user_type)", t->demarch_name, t->ctype);
        EL(0, "{");
        EL(1, "Eina_List *azy_user_type_tmp = NULL;");
        EL(1, "unsigned int x, total;");
        NL;
        EL(1, "EINA_SAFETY_ON_NULL_RETURN_VAL(azy_user_type, EINA_FALSE);");
        NL;
        EL(1, "if ((!value_array) || (eina_value_type_get(value_array) != EINA_VALUE_TYPE_ARRAY))");
        EL(2, "return EINA_FALSE;");
        NL;
        EL(1, "total = eina_value_array_count(value_array);");
        EL(1, "for (x = 0; x < total; x++)");
        EL(1, "{");
        if (t->item_type->ctype == d)
          EL(2, "double *d;");
        EL(2, "%s item_value = %s;", t->item_type->ctype, t->item_type->cnull);
        EL(2, "Eina_Value val;");
        NL;
        EL(2, "if (eina_value_array_value_get(value_array, x, &val))");
        EL(2, "{");
        EL(3, "%s(&val, &item_value);", t->item_type->demarch_name);
        EL(3, "eina_value_flush(&val);");
        EL(2, "}");
        NL;

        if ((t->item_type->ctype == i) || (t->item_type->ctype == ti))
          EL(2, "azy_user_type_tmp = eina_list_append(azy_user_type_tmp, (intptr_t*)(long)item_value);");
        else if (t->item_type->ctype == b)
          EL(2, "azy_user_type_tmp = eina_list_append(azy_user_type_tmp, (intptr_t*)(long)(item_value));");
        else if (t->item_type->ctype == d)
          {
             EL(2, "d = malloc(sizeof(double));");
             EL(2, "*d = item_value;");
             EL(2, "azy_user_type_tmp = eina_list_append(azy_user_type_tmp, d);");
          }
        else
          EL(2, "azy_user_type_tmp = eina_list_append(azy_user_type_tmp, item_value);");

        EL(1, "}");
        NL;
        EL(1, "*azy_user_type = azy_user_type_tmp;");
        EL(1, "return EINA_TRUE;");
        EL(0, "}");
        NL;
     }
}

static void
gen_type_eq(Azy_Typedef *t,
            Eina_Bool def)
{
   Eina_List *l;
   Azy_Struct_Member *m;

   if (def)
     {
        if ((t->type != TD_STRUCT) && (t->type != TD_ARRAY))
          return;
        EL(0, "/** @brief Check whether all the values of @p a are equal to @p b */");
        if (t->type == TD_STRUCT)
          EL(0, "Eina_Bool %s_eq(%s a, %s b);", t->cname, t->ctype, t->ctype);
        else if (t->type == TD_ARRAY)
          EL(0, "Eina_Bool %s(%s a, %s b);", t->eq_func, t->ctype, t->ctype);

        return;
     }

   if (t->type == TD_STRUCT)
     {
        EL(0, "Eina_Bool %s_eq(%s a, %s b)", t->cname, t->ctype, t->ctype);
        EL(0, "{");
        EL(1, "if (a == b)");
        EL(2, "return EINA_TRUE;");
        EL(1, "if ((!a) || (!b))");
        EL(2, "return EINA_FALSE;");

        EINA_LIST_FOREACH(t->struct_members, l, m)
          {
             if (m->type->eq_func)
               {
                  EL(1, "if (!%s(a->%s, b->%s))", m->type->eq_func, m->name, m->name);
               }
             else
               EL(1, "if (a->%s != b->%s)", m->name, m->name);

             EL(2, "return EINA_FALSE;");
          }

        NL;
        EL(1, "return EINA_TRUE;");
        EL(0, "}");
        NL;
     }
   else if (t->type == TD_ARRAY)
     {
        EL(0, "Eina_Bool %s(%s a, %s b)", t->eq_func, t->ctype, t->ctype);
        EL(0, "{");
        EL(1, "Eina_List *y, *z;");
        NL;
        EL(1, "if (a == b)");
        EL(2, "return EINA_TRUE;");
        EL(1, "if ((!a) || (!b))");
        EL(2, "return EINA_FALSE;");
        EL(1, "for (y = a, z = b;y && z; y = y->next, z = z->next)");
        EL(1, "{");

        if (t->item_type->eq_func)
          EL(2, "if (!%s(y->data, z->data))", t->item_type->eq_func);
        else
          {
             if ((t->item_type->ctype == i) || (t->item_type->ctype == b))
               EL(2, "if ((intptr_t)(y->data) != (intptr_t)(z->data))");
             else if (t->item_type->ctype == d)
               EL(2, "if (*(double*)(y->data) != *(double*)(z->data))");
             else
               EL(2, "if ((%s)(y->data) != (%s)(z->data))", t->item_type->ctype, t->item_type->ctype);
          }

        EL(3, "return EINA_FALSE;");
        EL(1, "}");
        NL;
        EL(1, "return EINA_TRUE;");
        EL(0, "}");
        NL;
     }
}

static void
gen_type_isnull(Azy_Typedef *t,
                Eina_Bool def)
{
   Eina_List *l;
   Azy_Struct_Member *m;

   if (def)
     {
        if (t->type != TD_STRUCT) return;

        EL(0, "/** @brief Check whether all the values of @p a are NULL */");
        EL(0, "Eina_Bool %s_isnull(%s a);", t->cname, t->ctype);
        return;
     }

   if (t->type == TD_STRUCT)
     {
        EL(0, "Eina_Bool %s_isnull(%s a)", t->cname, t->ctype);
        EL(0, "{");
        EL(1, "if (!a)");
        EL(2, "return EINA_TRUE;");

        EINA_LIST_FOREACH(t->struct_members, l, m)
          {
             if (m->type->isnull_func)
               EL(1, "if (%s(a->%s))", m->type->isnull_func, m->name);
             else
               EL(1, "if (a->%s != %s)", m->name, m->type->cnull);

             EL(2, "return EINA_FALSE;");
          }

        NL;
        EL(1, "return EINA_TRUE;");
        EL(0, "}");
        NL;
     }
}

static void
gen_type_hash(Azy_Typedef *t,
              Eina_Bool def)
{
   Eina_List *l;
   Eina_Bool need_err = EINA_FALSE;
   Azy_Struct_Member *m;

   if (t->type != TD_STRUCT) return;

   if (def)
     {
        EL(0, "/** @brief Create, from a hash table of strings, a %s */", t->cname);
        EL(0, "%s%s_hash(Eina_Hash *h);", t->ctype, t->cname);
        return;
     }

   EL(0, "%s%s_hash(Eina_Hash *h)", t->ctype, t->cname);
   EL(0, "{");
   EL(1, "%snew;", t->ctype);
   NL;
   EL(1, "EINA_SAFETY_ON_NULL_RETURN_VAL(h, NULL);");
   NL;
   EL(1, "new = %s_new();", t->cname);
   EL(1, "EINA_SAFETY_ON_NULL_RETURN_VAL(new, NULL);");
   EINA_LIST_FOREACH(t->struct_members, l, m)
     {
        if (m->type->hash_func)
          {
             EL(1, "new->%s = %s(eina_hash_find(h, \"%s\"));", m->name, m->type->hash_func, m->strname ? m->strname : m->name);
             EL(1, "EINA_SAFETY_ON_NULL_GOTO(new->%s, error);", m->name);
          }
        else
          {
             if (m->type->ctype == b)
               {
                  EL(1, "EINA_SAFETY_ON_TRUE_GOTO(!azy_str_to_bool_(eina_hash_find(h, \"%s\"), &new->%s), error);", m->name, m->name);
                  need_err = EINA_TRUE;
               }
             else if (m->type->ctype == d)
               {
                  EL(1, "EINA_SAFETY_ON_TRUE_GOTO(!azy_str_to_double_(eina_hash_find(h, \"%s\"), &new->%s), error);", m->name, m->name);
                  need_err = EINA_TRUE;
               }
             else if (m->type->ctype == i)
               {
                  EL(1, "EINA_SAFETY_ON_TRUE_GOTO(!azy_str_to_int_(eina_hash_find(h, \"%s\"), &new->%s), error);", m->name, m->name);
                  need_err = EINA_TRUE;
               }
             else if (m->type->ctype == c)
               {
                  EL(1, "EINA_SAFETY_ON_TRUE_GOTO(!azy_str_to_str_(eina_hash_find(h, \"%s\"), &new->%s), error);", m->name, m->name);
                  need_err = EINA_TRUE;
               }
          }
     }
   EL(1, "return new;");
   if (need_err)
     {
        NL;
        EL(0, "error:");
        EL(1, "%s(new);", t->free_func);
        EL(1, "return NULL;");
     }
   EL(0, "}");
   NL;
}

static void
gen_type_print(Azy_Typedef *t,
               Eina_Bool def)
{
   Eina_List *l;
   Azy_Struct_Member *m;

   if (def)
     {
        if (t->type == TD_STRUCT)
          {
             EL(0, "/** @brief Print, indenting @p indent times @p pre, a %s */", t->cname);
             EL(0, "void %s_print(const char *pre, int indent, const %sa);", t->cname, t->ctype);
          }
        else if (t->type == TD_ARRAY)
          {
             EL(0, "/** @brief Print, indenting @p indent times @p pre, an array of %s */", t->cname);
             EL(0, "void %s(const char *pre, int indent, const Eina_List *a);", t->print_func);
          }
        return;
     }

   if (t->type == TD_STRUCT)
     {
        EL(0, "void %s_print(const char *pre, int indent, const %sa)", t->cname, t->ctype);
        EL(0, "{");
        EL(1, "int i;");
        EL(1, "if (!a)");
        EL(2, "return;");
        EL(1, "if (!pre) pre = \"\\t\";");

        EINA_LIST_FOREACH(t->struct_members, l, m)
          {
             EL(1, "for (i = 0; i < indent; i++)");
             EL(2, "printf(\"%%s\", pre);");
             if (m->type->print_func)
               {
                  EL(1, "printf(\"%s:\\n\");", m->name);
                  EL(1, "%s(pre, indent + 1, a->%s);", m->type->print_func, m->name);
                  EL(1, "printf(\"\\n\");");
               }
             else
               {
                  if (m->type->ctype == b)
                    EL(1, "printf(\"%s: %s\\n\", (a->%s) ? \"yes\" : \"no\");", m->name, m->type->fmt_str, m->name);
                  else
                    EL(1, "printf(\"%s: %s\\n\", a->%s);", m->name, m->type->fmt_str, m->name);
               }
          }
        EL(0, "}");
        NL;
     }
   else if (t->type == TD_ARRAY)
     {
        EL(0, "void %s(const char *pre, int indent, const Eina_List *a)", t->print_func);
        EL(0, "{");
        EL(1, "const Eina_List *l;");
        if ((t->item_type->ctype == i) || (t->item_type->ctype == b) || (t->item_type->ctype == ti))
          EL(1, "intptr_t *t;");
        else if  (t->item_type->ctype == d)
          EL(1, "double *t;");
        else
          EL(1, "%s t;", t->item_type->ctype);
        NL;
        EL(1, "if (!a)");
        EL(2, "return;");
        EL(1, "EINA_LIST_FOREACH(a, l, t)");
        if (t->item_type->print_func)
          EL(2, "%s(pre, indent + 1, t);", t->item_type->print_func);
        else
          {
             EL(1, "{");
             EL(2, "int i;");
             EL(2, "for (i = 0; i < indent; i++)");
             EL(3, "printf(\"%%s\", pre);");
             if ((t->item_type->ctype == i) || (t->item_type->ctype == ti))
               EL(2, "printf(\"%%\"PRIdPTR\", \", (intptr_t)t);");
             else if (t->item_type->ctype == b)
               EL(2, "printf(\"%s, \", ((intptr_t)t) ? \"yes\" : \"no\");", t->item_type->fmt_str);
             else if (t->item_type->ctype == d)
               EL(2, "printf(\"%s, \", *(double*)t);", t->item_type->fmt_str);
             else
               EL(2, "printf(\"%s, \", t);", t->item_type->fmt_str);
             EL(1, "}");
          }
        EL(0, "}");
        NL;
     }
}

static void
gen_type_copyfree(Azy_Typedef *t,
                  Eina_Bool def)
{
   Eina_List *l;
   Azy_Struct_Member *m;

   if (def)
     {
        if (t->type == TD_STRUCT || t->type == TD_ARRAY)
          {
             EL(0, "/** @brief Free a #%s */", t->ctype);
             EL(0, "void %s(%s val);", t->free_func, t->ctype);
             EL(0, "/** @brief Copy a #%s */", t->ctype);
             EL(0, "%s%s(%sorig);", t->ctype, t->copy_func, t->ctype);
          }
        return;
     }

   if (t->type == TD_STRUCT)
     {
        /* free */
         EL(0, "void %s(%s val)", t->free_func, t->ctype);
         EL(0, "{");
         EL(1, "if (!val)");
         EL(2, "return;");
         NL;

         EINA_LIST_FOREACH(t->struct_members, l, m)
           {
              if (m->type->free_func)
                EL(1, "%s(val->%s);", m->type->free_func, m->name);
           }

         EL(1, "free(val);");
         EL(0, "}");
         NL;

         /* copy */
         EL(0, "%s %s(%s orig)", t->ctype, t->copy_func, t->ctype);
         EL(0, "{");
         EL(1, "%s copy;", t->ctype);
         NL;
         EL(1, "if (!orig)");
         EL(2, "return NULL;");
         NL;
         EL(1, "copy = %s_new();", t->cname);

         EINA_LIST_FOREACH(t->struct_members, l, m)
           {
              if (m->type->copy_func)
                EL(1, "copy->%s = %s(orig->%s);", m->name, m->type->copy_func, m->name);
              else
                EL(1, "copy->%s = orig->%s;", m->name, m->name);
           }

         NL;
         EL(1, "return copy;");
         EL(0, "}");
         NL;
     }
   else if (t->type == TD_ARRAY)
     {
        /* free */
         EL(0, "void %s(Eina_List *val)", t->free_func);
         EL(0, "{");
         if (t->item_type->free_func)
           EL(1, "%s t;", t->item_type->ctype);
         else if (t->item_type->cname == d)
           EL(1, "double *t;");
         EL(1, "if (!val) return;");

         if (t->item_type->free_func || (t->item_type->cname == d))
           {
              EL(1, "EINA_LIST_FREE(val, t)");
              EL(2, "%s(t);", t->item_type->free_func ? t->item_type->free_func : "free");
           }
         else
           EL(1, "eina_list_free(val);");
         EL(0, "}");
         NL;

         /* copy */
         EL(0, "Eina_List *%s(Eina_List *orig)", t->copy_func);
         EL(0, "{");
         EL(1, "Eina_List *copy = NULL;");
         if (t->item_type->copy_func || (t->item_type->cname == d))
           {
              EL(1, "Eina_List *l;");
              if (t->item_type->cname == d)
                EL(1, "double *t, *d;");
              else
                EL(1, "%s t;", t->item_type->ctype);
              NL;

              EL(1, "EINA_LIST_FOREACH(orig, l, t)");
              if (t->item_type->cname == d)
                {
                   EL(2, "{");
                   EL(3, "d = malloc(sizeof(double));");
                   EL(3, "*d = *t;");
                   EL(3, "copy = eina_list_append(copy, d);");
                   EL(2, "}");
                }
              else
                EL(2, "copy = eina_list_append(copy, %s((%s)t));", t->item_type->copy_func, t->item_type->ctype);
           }
         else
           EL(1, "copy = eina_list_clone(orig);");

         NL;
         EL(1, "return copy;");
         EL(0, "}");
         NL;
     }
}

static void
gen_type_defs(Eina_List *types)
{
   Eina_List *j, *l;
   Azy_Typedef *t;

   EINA_LIST_FOREACH(types, j, t)
     {
        if (t->type != TD_STRUCT)
          continue;
        EL(0, "typedef struct %s %s;", t->cname, t->cname);
     }

   NL;

   EINA_LIST_FOREACH(types, j, t)
     {
        Azy_Struct_Member *m;

        if (t->type != TD_STRUCT)
          continue;
        if (t->doc)
          EL(0, "%s", t->doc);

        EL(0, "struct %s\n{", t->cname);

        EINA_LIST_FOREACH(t->struct_members, l, m)
          {
             EL(1, "%s %s; /* %s */", m->type->ctype, m->name,
                (m->type->type == TD_ARRAY) ? m->type->cname : "");
          }

        EL(0, "};");
        NL;
        EL(0, "static inline %s%s_new(void)", t->ctype, t->cname);
        EL(0, "{");
        EL(1, "return calloc(1, sizeof(%s));", t->cname);
        EL(0, "}");
        NL;
     }
}

static void
gen_marshalizers(Eina_Bool header)
{
   Eina_List *j;
   Azy_Typedef *t;

   EINA_LIST_FOREACH(azy->types, j, t)
     {
        if (t->type == TD_BASE) continue;
        gen_type_marshalizers(t, header);
     }
}

static void
gen_errors_header(Eina_Bool azy_types)
{
   Eina_List *j;
   Azy_Error_Code *err;

   if (!azy->errors) return;

   if (azy_types)
     {
        EL(0, "Eina_Bool azy_err_faultcode_set(Azy_Content *content, Eina_Error code);");
        return;
     }

   EINA_LIST_FOREACH(azy->errors, j, err)
     {
        EL(0, "extern Eina_Error %s;", err->cname);
        EL(0, "extern int %s_code;", err->cname);
     }
   NL;

   EL(0, "void azy_err_init(void);");
   EL(0, "int azy_err_faultcode_get(Eina_Error code);");
}

static void
gen_errors_impl(Eina_Bool azy_types)
{
   Eina_List *j;
   Azy_Error_Code *err;

   if (!azy->errors) return;

   if (azy_types)
     {
        EL(0, "Eina_Bool azy_err_faultcode_set(Azy_Content *content, Eina_Error code)");
        EL(0, "{");
        EL(1, "EINA_SAFETY_ON_NULL_RETURN_VAL(content, EINA_FALSE);");
        EL(1, "EINA_SAFETY_ON_TRUE_RETURN_VAL(!code, EINA_FALSE);");
        NL;
        EINA_LIST_FOREACH(azy->errors, j, err)
          {
             EL(1, "if (code == %s)", err->cname);
             EL(2, "{");
             EL(3, "azy_content_error_faultcode_set(content, code, %s_code);", err->cname);
             EL(3, "return EINA_TRUE;");
             EL(2, "}");
          }
        EL(1, "fprintf(stderr, \"Error code %%u not found!\\n\", code);");
        EL(1, "return EINA_FALSE;");
        EL(0, "}");
        NL;
        return;
     }

   EINA_LIST_FOREACH(azy->errors, j, err)
     {
        EL(0, "Eina_Error %s;", err->cname);
        EL(0, "int %s_code = %i;", err->cname, err->code);
        EL(0, "const char %s_str[] = \"%s\";", err->cname, err->msg);
        NL;
     }

   EL(0, "void azy_err_init(void)");
   EL(0, "{");
   EINA_LIST_FOREACH(azy->errors, j, err)
     EL(1, "%s = eina_error_msg_static_register(%s_str);", err->cname, err->cname);
   EL(0, "}");
   NL;



   EL(0, "int azy_err_faultcode_get(Eina_Error code)");
   EL(0, "{");
   EL(1, "EINA_SAFETY_ON_TRUE_RETURN_VAL(!code, EINA_FALSE);");
   NL;
   EINA_LIST_FOREACH(azy->errors, j, err)
     {
        EL(1, "if (code == %s)", err->cname);
        EL(2, "return %s_code;", err->cname);
     }
   EL(1, "fprintf(stderr, \"Error code %%u not found!\\n\", code);");
   EL(1, "return -1;");
   EL(0, "}");
   NL;
}

static void
gen_common_headers(void)
{
   Eina_List *j;
   Azy_Typedef *t;

   OPEN("%s/%s%sCommon.h", out_dir, name, sep);

   EL(0, "#ifndef %s_Common_H", (azy->name) ? azy->name : "AZY");
   EL(0, "#define %s_Common_H", (azy->name) ? azy->name : "AZY");
   NL;

   EL(0, "#ifdef HAVE_CONFIG_H");
   EL(0, "# include \"config.h\"");
   EL(0, "#endif");
   EL(0, "#include <Eina.h>");
   NL;
   EL(0, "Eina_Bool azy_str_to_bool_(const char *d, Eina_Bool *ret);");
   EL(0, "Eina_Bool azy_str_to_str_(const char *d, const char **ret);");
   EL(0, "Eina_Bool azy_str_to_int_(const char *d, int *ret);");
   EL(0, "Eina_Bool azy_str_to_double_(const char *d, double *ret);");
   gen_errors_header(EINA_FALSE);
   gen_type_defs(azy->types);
   EINA_LIST_FOREACH(azy->types, j, t)
     {
        gen_type_copyfree(t, EINA_TRUE);
        gen_type_eq(t, EINA_TRUE);
        gen_type_print(t, EINA_TRUE);
        gen_type_isnull(t, EINA_TRUE);
//        gen_type_hash(t, EINA_TRUE);
     }
   EL(0, "#endif");
   fclose(f);

   OPEN("%s/%s%sCommon_Azy.h", out_dir, name, sep);

   EL(0, "#ifndef %s_Common_AZY_H", (azy->name) ? azy->name : "AZY");
   EL(0, "#define %s_Common_AZY_H", (azy->name) ? azy->name : "AZY");
   NL;

   EL(0, "#ifdef HAVE_CONFIG_H");
   EL(0, "# include \"config.h\"");
   EL(0, "#endif");
   EL(0, "#include <Eina.h>");
   EL(0, "#include <Azy.h>");
   EL(0, "#include \"%s%sCommon.h\"", name, sep);

   NL;
   gen_marshalizers(EINA_TRUE);
   NL;
   gen_errors_header(EINA_TRUE);

   EL(0, "#endif");
   fclose(f);
}

static void
gen_common_impl(void)
{
   Eina_List *j;
   Azy_Typedef *t;

   OPEN("%s/%s%sCommon.c", out_dir, name, sep);
   EL(0, "#ifdef HAVE_CONFIG_H");
   EL(0, "# include \"config.h\"");
   EL(0, "#endif");
   EL(0, "#include \"%s%sCommon.h\"", name, sep);
   EL(0, "#include <string.h>");
   EL(0, "#include <inttypes.h>");
   EL(0, "#include <errno.h>");
   EL(0, "#include <Azy.h>");
   NL;

   EL(0, "Eina_Bool");
   EL(0, "azy_str_to_bool_(const char *d, Eina_Bool *ret)");
   EL(0, "{");
   EL(1, "*ret = EINA_FALSE;");
   EL(1, "if (d && (*d == '1')) *ret = EINA_TRUE;");
   EL(1, "return EINA_TRUE;");
   EL(0, "}");
   NL;
   EL(0, "Eina_Bool");
   EL(0, "azy_str_to_str_(const char *d, const char **ret)");
   EL(0, "{");
   EL(1, "*ret = NULL;");
   EL(1, "if (!d) return EINA_TRUE;");
   EL(1, "*ret = eina_stringshare_add(d);");
   EL(1, "return EINA_TRUE;");
   EL(0, "}");
   NL;
   EL(0, "Eina_Bool");
   EL(0, "azy_str_to_int_(const char *d, int *ret)");
   EL(0, "{");
   EL(1, "errno = 0;");
   EL(1, "*ret = 0;");
   EL(1, "if (!d) return EINA_TRUE;");
   EL(1, "*ret = strtol(d, NULL, 10);");
   EL(1, "if (errno)");
   EL(2, "{");
   EL(0, "#ifdef ERR");
   EL(3, "fprintf(stderr, \"Error converting %%s to int: '%%s'\", d, strerror(errno));");
   EL(0, "#endif");
   EL(3, "return EINA_FALSE;");
   EL(2, "}");
   EL(1, "return EINA_TRUE;");
   EL(0, "}");
   NL;
   EL(0, "Eina_Bool");
   EL(0, "azy_str_to_double_(const char *d, double *ret)");
   EL(0, "{");
   EL(1, "errno = 0;");
   EL(1, "*ret = 0.0;");
   EL(1, "if (!d) return EINA_TRUE;");
   EL(1, "*ret = strtod(d, NULL);");
   EL(1, "if (errno)");
   EL(2, "{");
   EL(0, "#ifdef ERR");
   EL(3, "fprintf(stderr, \"Error converting %%s to double: '%%s'\", d, strerror(errno));");
   EL(0, "#endif");
   EL(3, "return EINA_FALSE;");
   EL(2, "}");
   EL(1, "return EINA_TRUE;");
   EL(0, "}");

   EINA_LIST_FOREACH(azy->types, j, t)
     {
        gen_type_copyfree(t, EINA_FALSE);
        gen_type_eq(t, EINA_FALSE);
        gen_type_print(t, EINA_FALSE);
        gen_type_isnull(t, EINA_FALSE);
//        gen_type_hash(t, EINA_FALSE);
     }

   gen_errors_impl(EINA_FALSE);
   fclose(f);


   OPEN("%s/%s%sCommon_Azy.c", out_dir, name, sep);
   EL(0, "#include \"%s%sCommon_Azy.h\"", name, sep);
   NL;
   gen_marshalizers(EINA_FALSE);
   gen_errors_impl(EINA_TRUE);
   fclose(f);
}

static void
gen_server_headers(Azy_Server_Module *s)
{
   Eina_List *j, *k;
   Azy_Method_Param *p;
   Azy_Method *method;

   OPEN("%s/%s%s%s.azy_server.h", out_dir, name, sep, s->name);
   EL(0, "#ifndef %s_%s_AZY_SERVER_H", (azy->name) ? azy->name : "AZY", s->name);
   EL(0, "#define %s_%s_AZY_SERVER_H", (azy->name) ? azy->name : "AZY", s->name);
   NL;

   EL(0, "#include <Azy.h>");
   EL(0, "#include \"%s%sCommon.h\"", name, sep);
   if (s->stub_header)
     {
        EL(0, "%s", s->stub_header);
        NL;
     }
   NL;

   if (s->doc)
     EL(0, "%s", s->doc);
   else
     {
        EL(0,
           "/** Implementation specific module data.");
        EL(0, " */ ");
     }

   EL(0, "typedef struct %s%s%s_Module", name, sep, s->name);
   EL(0, "{");
   /* FIXME: does this need to be output at all if there is no attrs stub? */
   EL(0, "%s", s->stub_attrs ? s->stub_attrs : "");
   EL(0, "} %s%s%s_Module;", name, sep, s->name);
   NL;

   EL(0, "#define %s%s%s_module_data_get(Azy_Server_Module) (%s%s%s_Module*)azy_server_module_data_get((Azy_Server_Module))",
      name, sep, s->name, name, sep, s->name);

   if (s->stub_init && s->stub_init[0])
     {
        EL(0, "/** Module constructor.");
        EL(0, " * ");
        EL(0, " * @param module Module object.");
        EL(0, " * ");
        EL(0, " * @return EINA_TRUE if all is ok.");
        EL(0, " */ ");
        EL(0, "Eina_Bool %s%s%s_module_init(Azy_Server_Module *module);", name, sep, s->name);
        NL;
     }

   if (s->stub_shutdown && s->stub_shutdown[0])
     {
        EL(0, "/** Module destructor.");
        EL(0, " * ");
        EL(0, " * @param module Module object.");
        EL(0, " */ ");
        EL(0, "void %s%s%s_module_shutdown(Azy_Server_Module *module);", name, sep, s->name);
        NL;
     }

   if (s->stub_pre && s->stub_pre[0])
     {
        EL(0, "/** Pre-call hook.");
        EL(0, " * ");
        EL(0, " * @param module Module object.");
        EL(0, " * @param net Network object to use for sending.");
        EL(0, " * ");
        EL(0,
           " * @return EINA_TRUE if you want to continue execution of the call.");
        EL(0, " */ ");
        EL(0, "Eina_Bool %s%s%s_module_pre(Azy_Server_Module *module, Azy_Net *net);", name, sep, s->name);
        NL;
     }

   if (s->stub_post && s->stub_post[0])
     {
        EL(0, "/** Post-call hook.");
        EL(0, " * ");
        EL(0, " * @param module Module object.");
        EL(0, " * @param content Received content object.");
        EL(0, " * ");
        EL(0,
           " * @return EINA_TRUE if you want to continue execution of the call.");
        EL(0, " */ ");
        EL(0, "Eina_Bool %s%s%s_module_post(Azy_Server_Module *module, Azy_Content *content);", name, sep, s->name);
        NL;
     }

   if (s->stub_fallback && s->stub_fallback[0])
     {
        EL(0,
           "/** Fallback hook. (for undefined methods)");
        EL(0, " * ");
        EL(0, " * @param module Module object.");
        EL(0, " * @param content Received content object.");
        EL(0, " * ");
        EL(0,
           " * @return EINA_TRUE if you handled the call.");
        EL(0, " */ ");
        EL(0, "Eina_Bool %s%s%s_module_fallback(Azy_Server_Module *module, Azy_Content *content);", name, sep, s->name);
        NL;
     }

   if (s->stub_download && s->stub_download[0])
     {
        EL(0, "/** Download hook.");
        EL(0, " * ");
        EL(0, " * @param module Module object.");
        EL(0, " * ");
        EL(0,
           " * @return EINA_TRUE if you want to continue execution of the call.");
        EL(0, " */ ");
        EL(0, "Eina_Bool %s%s%s_module_download(Azy_Server_Module *module);", name, sep, s->name);
        NL;
     }

   if (s->stub_upload && s->stub_upload[0])
     {
        EL(0, "/** Upload hook.");
        EL(0, " * ");
        EL(0, " * @param module Module object.");
        EL(0, " * ");
        EL(0,
           " * @return EINA_TRUE if you want to continue execution of the call.");
        EL(0, " */ ");
        EL(0, "Eina_Bool %s%s%s_module_upload(Azy_Server_Module *module);", name, sep, s->name);
        NL;
     }

   EINA_LIST_FOREACH(s->methods, j, method)
     {
        if (method->doc && method->doc[0])
          EL(0, "%s", method->doc);
        else
          {
             EL(0, "/** ");
             EL(0, " * ");
             EL(0,
                " * @param module Module object.");

             EINA_LIST_FOREACH(method->params, k, p)
               {
                  EL(0, " * @param %s", p->name);
               }

             EL(0, " * ");
             EL(0, " * @return A valid call id, or 0 on failure");
             EL(0, " */ ");
          }

        if (suspend_funcs)
          E(0, "void %s%s%s_module_%s(Azy_Server_Module *module",
            name, sep, s->name,
            method->name);
        else
          E(0, "%s %s%s%s_module_%s(Azy_Server_Module *module",
            method->return_type->ctype, name, sep, s->name,
            method->name);

        EINA_LIST_FOREACH(method->params, k, p)
          {
             E(0, ", %s %s", p->type->ctype, p->name);
          }

        EL(0, ", Azy_Content* error_);");
        NL;
     }
   EL(0, "Azy_Server_Module_Def* %s%s%s_module_def();",
      name, sep, s->name);
   EL(0, "#endif");
   fclose(f);
}

static void
gen_server_impl(Azy_Server_Module *s)
{
   Eina_List *j, *k;
   Azy_Method *method;
   Azy_Method_Param *p;

   OPEN("%s/%s%s%s.azy_server.c", out_dir, name, sep, s->name);
   if (s->stub_header)
     {
        STUB(s->stub_header);
        NL;
     }
   EL(0, "#include <Azy.h>");
   EL(0, "#include \"%s%sCommon.h\"", name, sep);
   EL(0, "#include \"%s%sCommon_Azy.h\"", name, sep);
   EL(0, "#include \"%s%s%s.azy_server.h\"", name, sep, s->name);
   NL;

   EL(0, "static Azy_Server_Module_Def *module_def = NULL;");
   NL;
   NL;
/* ************************STUBS************************* */
   if (s->stub_init && s->stub_init[0])
     {
        EL(0, "Eina_Bool %s%s%s_module_init(Azy_Server_Module *module)", name, sep, s->name);
        EL(0, "{");
/* attempt to evade even more compile warnings at the expense of slightly slower runtime.
 * worth iiiiiiiiiiiiiiiiiiiiit
 */
        if (strstr(s->stub_init, "data_"))
          EL(1, "%s%s%s_Module *data_ = azy_server_module_data_get(module);", name, sep, s->name);

        if (strstr(s->stub_init, "net_"))
          EL(1, "Azy_Net* net_ = azy_server_module_net_get(module);");
        STUB(s->stub_init);
        NL;
        EL(1, "return EINA_TRUE;");
        EL(0, "}");
        NL;
     }

   if (s->stub_shutdown && s->stub_shutdown[0])
     {
        EL(0, "void %s%s%s_module_shutdown(Azy_Server_Module *module)", name, sep, s->name);
        EL(0, "{");

        if (strstr(s->stub_shutdown, "data_"))
          EL(1, "%s%s%s_Module* data_ = azy_server_module_data_get(module);", name, sep, s->name);

        if (strstr(s->stub_shutdown, "net_"))
          EL(1, "Azy_Net* net_ = azy_server_module_net_get(module);");
        STUB(s->stub_shutdown);
        NL;
        EL(0, "}");
        NL;
     }

   if (s->stub_pre && s->stub_pre[0])
     {
        EL(0, "Eina_Bool %s%s%s_module_pre(Azy_Server_Module *module, Azy_Net *net)", name, sep, s->name);
        EL(0, "{");

        if (strstr(s->stub_pre, "data_"))
          EL(1, "%s%s%s_Module* data_ = azy_server_module_data_get(module);", name, sep, s->name);

        if (strstr(s->stub_pre, "net_"))
          EL(1, "Azy_Net* net_ = azy_server_module_net_get(module);");
        if (suspend_funcs)
          {
             EL(1, "if (azy_server_module_active_get(module))");
             EL(2, "azy_server_module_events_suspend(module);");
          }
        STUB(s->stub_pre);
        EL(1, "if (azy_server_module_events_suspended_get(module))");
        EL(2, "return EINA_TRUE;");
        NL;
        EL(1, "return EINA_TRUE;");
        EL(0, "}");
        NL;
     }

   if (s->stub_post && s->stub_post[0])
     {
        EL(0, "Eina_Bool %s%s%s_module_post(Azy_Server_Module *module, Azy_Content *content)", name, sep, s->name);
        EL(0, "{");

        if (strstr(s->stub_post, "data_"))
          EL(1, "%s%s%s_Module* data_ = azy_server_module_data_get(module);", name, sep, s->name);

        if (strstr(s->stub_post, "net_"))
          EL(1, "Azy_Net* net_ = azy_server_module_net_get(module);");
        if (!strstr(s->stub_post, "content"))
          EL(1, "(void)content;");
        if (suspend_funcs)
          {
             EL(1, "if (azy_server_module_active_get(module))");
             EL(2, "azy_server_module_events_suspend(module);");
          }
        STUB(s->stub_post);
        EL(1, "if (azy_server_module_events_suspended_get(module))");
        EL(2, "return EINA_TRUE;");
        NL;
        EL(1, "return EINA_TRUE;");
        EL(0, "}");
        NL;
     }

   if (s->stub_fallback && s->stub_fallback[0])
     {
        EL(0, "Eina_Bool %s%s%s_module_fallback(Azy_Server_Module *module, Azy_Content *content)", name, sep, s->name);
        EL(0, "{");

        if (strstr(s->stub_fallback, "data_"))
          EL(1, "%s%s%s_Module* data_ = azy_server_module_data_get(module);", name, sep, s->name);

        if (strstr(s->stub_fallback, "net_"))
          EL(1, "Azy_Net* net_ = azy_server_module_net_get(module);");
        if (!strstr(s->stub_fallback, "content"))
          EL(1, "(void)content;");
        if (suspend_funcs)
          {
             EL(1, "if (azy_server_module_active_get(module))");
             EL(2, "azy_server_module_events_suspend(module);");
          }
        STUB(s->stub_fallback);
        EL(1, "if (azy_server_module_events_suspended_get(module))");
        EL(2, "return EINA_TRUE;");
        NL;
        EL(1, "return EINA_TRUE;");
        EL(0, "}");
        NL;
     }

   if (s->stub_download && s->stub_download[0])
     {
        EL(0, "Eina_Bool %s%s%s_module_download(Azy_Server_Module *module)", name, sep, s->name);
        EL(0, "{");

        if (strstr(s->stub_download, "data_"))
          EL(1, "%s%s%s_Module* data_ = azy_server_module_data_get(module);", name, sep, s->name);

        if (strstr(s->stub_download, "net_"))
          EL(1, "Azy_Net* net_ = azy_server_module_net_get(module);");
        if (suspend_funcs)
          {
             EL(1, "if (azy_server_module_active_get(module))");
             EL(2, "azy_server_module_events_suspend(module);");
          }
        STUB(s->stub_download);
        EL(1, "return EINA_TRUE;");
        EL(0, "}");
        NL;
     }

   if (s->stub_upload && s->stub_upload[0])
     {
        EL(0, "Eina_Bool %s%s%s_module_upload(Azy_Server_Module *module)", name, sep, s->name);
        EL(0, "{");

        if (strstr(s->stub_upload, "data_"))
          EL(1, "%s%s%s_Module* data_ = azy_server_module_data_get(module);", name, sep, s->name);

        if (strstr(s->stub_upload, "net_"))
          EL(1, "Azy_Net* net_ = azy_server_module_net_get(module);");
        if (suspend_funcs)
          {
             EL(1, "if (azy_server_module_active_get(module))");
             EL(2, "azy_server_module_events_suspend(module);");
          }
        STUB(s->stub_upload);
        EL(1, "return EINA_TRUE;");
        EL(0, "}");
        NL;
     }

   EINA_LIST_FOREACH(s->methods, j, method)
     {
        if (suspend_funcs)
          E(0, "void %s%s%s_module_%s(Azy_Server_Module *module", name, sep, s->name, method->name);
        else
          E(0, "%s %s%s%s_module_%s(Azy_Server_Module *module", method->return_type->ctype, name, sep, s->name, method->name);

        EINA_LIST_FOREACH(method->params, k, p)
          {
             E(0, ", %s %s", p->type->ctype, p->name);
          }

        EL(0, ", Azy_Content *error_)");
        EL(0, "{");

        if ((method->stub_impl) && (strstr(method->stub_impl, "data_")))
          EL(1, "%s%s%s_Module *data_ = azy_server_module_data_get(module);", name, sep, s->name);
        if (!suspend_funcs)
          EL(1, "%s retval = %s;", method->return_type->ctype, method->return_type->cnull);

        if (method->stub_impl)
          {
             if (!strstr(method->stub_impl, "error_"))
               EL(1, "(void)error_;");
             STUB(method->stub_impl);
          }
        else
          EL(1, "azy_content_error_faultmsg_set(error_, -1, \"Method is not implemented. (%s)\");", method->name);
        if (!suspend_funcs)
          EL(1, "return retval;");
        EL(0, "}");
        NL;
     }
   NL;
   /********************* NOT STUBS *********************/
   EINA_LIST_FOREACH(s->methods, j, method)
     {
        int n = 0;

        EL(0, "static Eina_Bool method_%s(Azy_Server_Module *module, Azy_Content *content)", method->name);
        EL(0, "{");
        if (!suspend_funcs)
          {
             EL(1, "%s azy_return_module = %s;", method->return_type->ctype, method->return_type->cnull);
             EL(1, "Eina_Value *azy_return_value;");
          }
        EINA_LIST_FOREACH(method->params, k, p)
          EL(1, "%s %s = %s;", p->type->ctype, p->name, p->type->cnull);

        NL;
        EL(1, "EINA_SAFETY_ON_NULL_RETURN_VAL(module, EINA_FALSE);");
        EL(1, "EINA_SAFETY_ON_NULL_RETURN_VAL(content, EINA_FALSE);");

        NL;
        EL(1,
           "if (eina_list_count(azy_content_params_get(content)) != %d) /* number of parameters taken by call */",
           eina_list_count(method->params));
        EL(1, "{");
        EL(2,
           "azy_content_error_faultmsg_set(content, -1, \"Invalid number of parameters passed to stub function!\");");
        EL(2, "return EINA_FALSE;");
        EL(1, "}");

        if (method->params)
          {
             EL(1, "if (azy_server_module_params_exist(module))");
             EL(2, "{");
             EINA_LIST_FOREACH(method->params, k, p)
               {
                  if (p->type->ctype == d)
                    {
                       EL(3, "double *%s_ptr = NULL;", p->name);
                       EL(3, "%s_ptr = azy_server_module_param_get(module, \"%s\");", p->name, p->name);
                       EL(3, "if (!%s_ptr)", p->name);
                       EL(4, "{");
                       EL(5, "azy_content_error_faultmsg_set(content, -1, \"Stub parameter value demarshalization failed. (%s:%s)\");",
                          method->name, p->name);
                       EL(5, "return EINA_FALSE;");
                       EL(4, "}");
                       EL(3, "%s = *%s_ptr;", p->name, p->name);
                    }
                  else if (p->type->ctype == b)
                    {
                       EL(3, "if (azy_server_module_param_get(module, \"%s\"))", p->name);
                       EL(4, "%s = EINA_TRUE;", p->name);
                    }
                  else
                    {
                       if (p->type->ctype == i)
                         EL(3, "%s = (intptr_t)azy_server_module_param_get(module, \"%s\");", p->name, p->name);
                       else
                         EL(3, "%s = azy_server_module_param_get(module, \"%s\");", p->name, p->name);
                    }
               }
             EL(2, "}");
             EL(1, "else");
             EL(2, "{");
             EINA_LIST_FOREACH(method->params, k, p)
               {
                  EL(3, "if (!%s(azy_content_param_get(content, %d), &%s))", p->type->demarch_name, n++, p->name);
                  EL(4, "{");
                  EL(5, "azy_content_error_faultmsg_set(content, -1, \"Stub parameter value demarshalization failed. (%s:%s)\");",
                     method->name, p->name);
                  EL(5, "return EINA_FALSE;");
                  EL(4, "}");
               }
             EL(2, "}");
             NL;
          }
        EL(1, "azy_content_retval_cb_set(content, (Azy_Content_Retval_Cb)%s);", method->return_type->march_name);
        NL;
        if (suspend_funcs)
          {
             EL(1, "azy_server_module_events_suspend(module);");
          }
        else
          E(1, "azy_return_module = ");
        E(suspend_funcs ? 1 : 0, "%s%s%s_module_%s(module", name, sep, s->name, method->name);

        EINA_LIST_FOREACH(method->params, k, p)
          {
             E(0, ", %s", p->name);
          }
        EL(0, ", content);");
        EL(1, "if (azy_content_error_is_set(content))");
        if (method->return_type->free_func && (!suspend_funcs))
          {
	     EL(2, "{");
             EL(3, "%s(azy_return_module);", method->return_type->free_func);
	  }
        EL(3, "return EINA_FALSE;");
        if (method->return_type->free_func && (!suspend_funcs))
          {
        EL(2, "}");
	  }
        NL;
        EL(1, "if (azy_server_module_events_suspended_get(module))");
        EL(2, "{");
        EL(3, "if (!azy_server_module_params_exist(module))");
        EL(4, "{");
        EINA_LIST_FOREACH(method->params, k, p)
          {
             if (p->type->ctype == d)
               {
                  EL(5, "double *%s_ptr", p->name);
                  EL(5, "%s_ptr = malloc(sizeof(double));", p->name);
                  EL(5, "*%s_ptr = %s", p->name);
                  EL(5, "azy_server_module_param_set(module, \"%s\", %s_ptr, Array_double_free);", p->name, p->name);
               }
             else if (p->type->ctype == i)
               EL(5, "azy_server_module_param_set(module, \"%s\", (intptr_t*)%s, NULL);", p->name, p->name);
             else if (p->type->ctype == b)
               EL(5, "azy_server_module_param_set(module, \"%s\", (intptr_t*)!!%s, NULL);", p->name, p->name);
             else
               EL(5, "azy_server_module_param_set(module, \"%s\", (void*)%s, (Eina_Free_Cb)%s);", p->name, p->name, p->type->free_func ? p->type->free_func : "NULL");
          }
        EL(4, "}");
        EL(3, "return EINA_TRUE;");
        EL(2, "}");
        NL;
        EL(1, "if (!azy_content_retval_get(content))");
        EL(2, "{");
        if (!suspend_funcs)
          {
             EL(3, "azy_return_value = %s(azy_return_module);", method->return_type->march_name);
             EL(3, "if (!azy_return_value)");
          }
        EL(4, "azy_content_error_faultmsg_set(content, -1, \"Stub return value marshalization failed. (%s)\");", method->name);
        if (!suspend_funcs)
          {
             EL(3, "else");
             EL(4, "azy_content_retval_set(content, azy_return_value);");
          }
        NL;

        if (method->return_type->free_func && (!suspend_funcs))
          EL(3, "%s(azy_return_module);", method->return_type->free_func);
        EL(2, "}");

        EL(1, "if (!azy_server_module_params_exist(module))");
        EL(2, "{");
        EINA_LIST_FOREACH(method->params, k, p)
          {
             if ((!p->pass_ownership) && p->type->free_func)
               EL(3, "%s(%s);", p->type->free_func, p->name);
          }
        EL(2, "}");

        EL(1, "return !azy_content_error_is_set(content);");
        EL(0, "}");
        NL;
     }

   NL;

   EL(0, "Azy_Server_Module_Def *\n%s%s%s_module_def(void)", name, sep, s->name);
   EL(0, "{");
   EL(1, "Azy_Server_Module_Method *method;");
   NL;
   EL(1, "if (module_def) return module_def;");
   NL;
   EL(1, "module_def = azy_server_module_def_new(\"%s%s%s\");", (azy->name && azy->name[0]) ? azy->name : "", sep[0] ? "." : "", s->name);
   EL(1, "EINA_SAFETY_ON_NULL_RETURN_VAL(module_def, NULL);");
   NL;

   E(1, "azy_server_module_def_init_shutdown_set(module_def, ");
   GET_NAME(init);
   E(0, ", ");
   GET_NAME(shutdown);
   EL(0, ");");

   E(1, "azy_server_module_def_pre_post_set(module_def, ");
   GET_NAME(pre);
   E(0, ", ");
   GET_NAME(post);
   EL(0, ");");

   E(1, "azy_server_module_def_download_upload_set(module_def, ");
   GET_NAME(download);
   E(0, ", ");
   GET_NAME(upload);
   EL(0, ");");

   E(1, "azy_server_module_def_fallback_set(module_def, ");
   GET_NAME(fallback);
   EL(0, ");");
   if (s->version) EL(1, "azy_server_module_def_version_set(module_def, %g);", s->version);
   EL(1, "azy_server_module_size_set(module_def, sizeof(%s%s%s_Module));", name, sep, s->name);
   EINA_LIST_FOREACH(s->methods, j, method)
     {
        EL(1, "method = azy_server_module_method_new(\"%s\", method_%s);", method->name, method->name);
        EL(1, "EINA_SAFETY_ON_NULL_GOTO(method, error);");
        EL(1, "azy_server_module_def_method_add(module_def, method);");
     }
   NL;
   EL(1, "return module_def;");
   EL(0, "error:");
   EL(2, "azy_server_module_def_free(module_def);");
   EL(1, "return NULL;");
   EL(0, "}");
   fclose(f);
}

static void
gen_client_headers(Azy_Server_Module *s)
{
   Eina_List *j, *k;
   Azy_Method *method;
   Azy_Method_Param *p;

   OPEN("%s/%s%s%s.azy_client.h", out_dir, name, sep, s->name);
   EL(0, "#ifndef %s_%s_AZY_CLIENT_H", (azy->name) ? azy->name : "AZY", s->name);
   EL(0, "#define %s_%s_AZY_CLIENT_H", (azy->name) ? azy->name : "AZY", s->name);
   NL;

   EL(0, "#include \"%s%sCommon_Azy.h\"", name, sep);
   NL;

   EINA_LIST_FOREACH(s->methods, j, method)
     {
        EL(0, "/** ");
        EL(0, " * ");
        EL(0, " * @param cli Client object.");

        EINA_LIST_FOREACH(method->params, k, p)
          {
             EL(0, " * @param %s", p->name);
          }

        EL(0, " * @param error_ Error content (cannot be NULL).");
        EL(0, " *");
        EL(0, " * @return ");
        EL(0, " */ ");

        E(0, "Azy_Client_Call_Id %s%s%s_%s(Azy_Client* cli", name, sep, s->name, method->name);

        EINA_LIST_FOREACH(method->params, k, p)
          {
             E(0, ", %s%s %s", !strcmp(p->type->ctype, "char*") ? "const " : "", p->type->ctype, p->name);
          }

        EL(0, ", Azy_Content *error_, const void *data);");
        NL;
     }

   EL(0, "#endif");
   fclose(f);
}

static void
gen_client_impl(Azy_Server_Module *s)
{
   Eina_List *j, *k;
   Azy_Method *method;
   Azy_Method_Param *p;

   OPEN("%s/%s%s%s.azy_client.c", out_dir, name, sep, s->name);
   EL(0, "#include <Azy.h>");
   EL(0, "#include \"%s%sCommon_Azy.h\"", name, sep);
   EL(0, "#include \"%s%s%s.azy_client.h\"", name, sep, s->name);
   NL;

   EINA_LIST_FOREACH(s->methods, j, method)
     {
        E(0, "Azy_Client_Call_Id %s%s%s_%s(Azy_Client* cli", name, sep, s->name, method->name);

        EINA_LIST_FOREACH(method->params, k, p)
          {
             E(0, ", %s%s %s", !strcmp(p->type->ctype, "char*") ? "const " : "", p->type->ctype, p->name);
          }

        EL(0, ", Azy_Content *error_, const void *data)");
        EL(0, "{");
        EL(1, "Azy_Client_Call_Id retval = 0;");

        if (method->params)
          EL(1, "Eina_Value *param_value;");

        EL(1, "Azy_Content *content;");
        EL(1, "Azy_Net *net;");
        EL(1, "Azy_Net_Transport tr = AZY_NET_TRANSPORT_XML;");
        NL;
        EL(1, "EINA_SAFETY_ON_NULL_RETURN_VAL(cli, retval);");
        EL(1, "EINA_SAFETY_ON_NULL_RETURN_VAL(error_, retval);");
        NL;
        EL(1, "content = azy_content_new(\"%s%s%s.%s\");", (azy->name && azy->name[0]) ? azy->name : "", (azy->name && azy->name[0]) ? "." : "", s->name, method->name);
        EL(1, "if (!content) return 0;");
        EL(1, "if (data) azy_content_data_set(content, data);");

        EINA_LIST_FOREACH(method->params, k, p)
          {
             NL;
             EL(1, "param_value = %s(%s);", p->type->march_name, p->name);
             EL(1, "if (!param_value)");
             EL(1, "{");
             EL(2, "azy_content_error_faultmsg_set(error_, AZY_CLIENT_ERROR_MARSHALIZER, "
                   "\"Call parameter value marshalization failed (param=%s).\");", p->name);
             EL(2, "azy_content_free(content);");
             EL(2, "return retval;");
             EL(1, "}");
             EL(1, "azy_content_param_add(content, param_value);");
          }

        NL;
        EL(1, "net = azy_client_net_get(cli);");
        EL(1, "if (azy_net_transport_get(net) != AZY_NET_TRANSPORT_UNKNOWN)");
        EL(2, "tr = azy_net_transport_get(net);");
        EL(1, "retval = azy_client_call(cli, content, tr, (Azy_Content_Cb)%s);", method->return_type->demarch_name);
        EL(1, "EINA_SAFETY_ON_TRUE_GOTO(!retval, error);");
        if (method->return_type->free_func)
          EL(1, "azy_client_callback_free_set(cli, retval, (Eina_Free_Cb)%s);", method->return_type->free_func);

        NL;
        EL(0, "error:");
        EL(1, "azy_content_error_copy(content, error_);");
        EL(1, "azy_content_free(content);");
        EL(1, "return retval;");
        EL(0, "}");
        NL;
     }
   fclose(f);
}

static void
azy_write(void)
{
   Azy_Server_Module *s;
   Eina_List *l;

   if (common_headers)
     gen_common_headers();

   if (common_impl)
     gen_common_impl();

   EINA_LIST_FOREACH(azy->modules, l, s)
     {
        if (client_headers)
          gen_client_headers(s);

        if (client_impl)
          gen_client_impl(s);

        if (server_headers)
          gen_server_headers(s);

        if (server_impl)
          gen_server_impl(s);
     }
#if 0
   if (azy_gen)
     {
        OPEN("%s/%s.azy", out_dir, azy->name);

        EL(0, "/* Generated AZY */");
        NL;

        EL(0, "namespace %s;", azy->name);
        NL;

        EINA_LIST_FOREACH(azy->errors, j, e)
          {
             EL(0, "error %-22s = %d;", e->name, e->code);
          }

        EINA_LIST_FOREACH(azy->types, j, t)
          {
             if (t->type == TD_STRUCT)
               {
                  NL;
                  EL(0, "struct %s", t->name);
                  EL(0, "{");

                  EINA_LIST_FOREACH(t->struct_members, k, m)
                    {
                       EL(1, "%-24s %s;",
                          azy_typedef_azy_name(m->type), m->name);
                    }

                  EL(0, "}");
               }
          }

        EINA_LIST_FOREACH(azy->modules, j, s)
          {
             NL;
             EL(0, "module %s", s->name);
             EL(0, "{");

             EINA_LIST_FOREACH(s->errors, j, e)
               {
                  EL(1, "error %-18s = %d;", e->name, e->code);
               }

             if (s->errors)
               NL;

             s->methods = eina_list_sort(s->methods, eina_list_count(s->methods), (Eina_Compare_Cb)azy_method_compare);

             EINA_LIST_FOREACH(s->methods, j, method)
               {
                  E(1, "%-24s %-25s(",
                    azy_typedef_azy_name(method->return_type), method->name);

                  EINA_LIST_FOREACH(method->params, k, p)
                    {
                       if (k == method->params)
                         EL(0, "%-15s %s%s", azy_typedef_azy_name(p->type), p->name, k->next ? "," : ");");
                       else
                         EL(0, "%-55s%-15s %s%s", "", azy_typedef_azy_name(p->type), p->name, k->next ? "," : ");");
                    }

                  if (!method->params)
                    EL(0, ");");
               }

             EL(0, "}");
          }
     }

   /* end */

   fclose(f);
#endif
}

int
main(int argc,
     char *argv[])
{
   Eina_Bool debug = EINA_FALSE;
   Eina_Bool exit_option = EINA_FALSE;
   Eina_Bool err;
   int args, mode;
   char *modes = "all";

   Ecore_Getopt_Value values[] =
   {
      ECORE_GETOPT_VALUE_STR(modes),
      ECORE_GETOPT_VALUE_STR(out_dir),
      ECORE_GETOPT_VALUE_BOOL(debug),
      ECORE_GETOPT_VALUE_BOOL(hash_funcs),
      ECORE_GETOPT_VALUE_BOOL(isnull_funcs),
      ECORE_GETOPT_VALUE_BOOL(print_funcs),
      ECORE_GETOPT_VALUE_BOOL(eq_funcs),
      ECORE_GETOPT_VALUE_BOOL(suspend_funcs),
      ECORE_GETOPT_VALUE_BOOL(exit_option),
      ECORE_GETOPT_VALUE_BOOL(exit_option),
      ECORE_GETOPT_VALUE_BOOL(exit_option),
      ECORE_GETOPT_VALUE_BOOL(exit_option)
   };

   ecore_init();
   ecore_app_args_set(argc, (const char **)argv);
   args = ecore_getopt_parse(&opts, values, argc, argv);
   if (args < 0)
     return 1;

   if (exit_option)
     return 0;

   if (!argv[args])
     {
        printf("You must specify the .azy file.\n");
        return 1;
     }

   if (debug) azy_parser_Trace(stdout, "Azy_Parser: ");
   err = EINA_FALSE;
   i = eina_stringshare_add("int");
   b = eina_stringshare_add("Eina_Bool");
   d = eina_stringshare_add("double");
   c = eina_stringshare_add("Eina_Stringshare *");
   e = eina_stringshare_add("Eina_List *");
   ti = eina_stringshare_add("time_t");
   b64 = eina_stringshare_add("base64");

   client_headers = strstr(modes, "all") || strstr(modes, "client-headers");
   client_impl = strstr(modes, "all") || strstr(modes, "client-impl");
   common_headers = strstr(modes, "all") || strstr(modes, "common-headers");
   common_impl = strstr(modes, "all") || strstr(modes, "common-impl");
   server_impl = strstr(modes, "all") || strstr(modes, "server-impl");
   server_headers = strstr(modes, "all") || strstr(modes, "server-headers");
   azy_gen = strstr(modes, "azy") || 0;

   mode = client_headers | client_impl | common_headers | common_impl | server_headers | server_impl;

   if (!mode)
     {
        printf("You have not specified a valid parsing mode!\n");
        exit(1);
     }

   for (; args < argc; args++)
     {
        Eina_Bool type = EINA_FALSE; /* EINA_FALSE for azy */
        azy_file = argv[args];
        if (eina_str_has_extension(azy_file, ".azy")) type = EINA_FALSE;
        if (!type)
          azy = azy_parse_file_azy(azy_file, &err);
        if ((!azy) || (err))
          {
             printf("Error parsing file!\n");
             exit(1);
          }
        sep = (azy->name) ? "_" : "";
        name = (azy->name) ? azy->name : "";

        if (!type)
          azy_write();
     }
   if (debug)
     printf("azy-parser: Done!!\n");
   return 0;
}

