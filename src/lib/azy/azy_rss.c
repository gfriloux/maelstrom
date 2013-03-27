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
 * @defgroup Azy_Rss RSS Functions
 * @brief Functions which affect #Azy_Rss objects
 * @{
 */

static Eina_Mempool *rss_mempool = NULL;

static Eet_Data_Descriptor *rss_edd = NULL;
static Eet_Data_Descriptor *rss_union_edd = NULL;
static Eet_Data_Descriptor *rss_union_rss_edd = NULL;
static Eet_Data_Descriptor *rss_union_atom_edd = NULL;
static Eet_Data_Descriptor *rss_contact_edd = NULL;
static Eet_Data_Descriptor *rss_category_edd = NULL;
static Eet_Data_Descriptor *rss_link_edd = NULL;

static const char *const rss_mapping[] = {"rss", "atom"};

const char *
_azy_rss_eet_union_type_get(const void *data, Eina_Bool *unknow)
{
   const Azy_Rss *rss = data;

   if (unknow) *unknow = EINA_FALSE;

   if (rss->atom < 2) return rss_mapping[rss->atom];
   if (unknow) *unknow = EINA_TRUE;

   return NULL;
}

Eina_Bool
_azy_rss_eet_union_type_set(const char *type, void *data, Eina_Bool unknow)
{
   Eina_Bool *atom = data;
   int i;

   if (unknow) return EINA_FALSE;

   for (i = 0; i < 2; ++i)
     if (!strcmp(rss_mapping[i], type))
       {
          *atom = i;
          return EINA_TRUE;
       }

   return EINA_FALSE;
}

#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(rss_contact_edd, Azy_Rss_Contact, #name, name, EET_T_##type)
static void
_azy_rss_contact_edd_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Azy_Rss_Contact);
   rss_contact_edd = eet_data_descriptor_stream_new(&eddc);
   ADD(name, INLINED_STRING);
   ADD(uri, INLINED_STRING);
   ADD(email, INLINED_STRING);
#undef ADD
}

#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(rss_category_edd, Azy_Rss_Category, #name, name, EET_T_##type)
static void
_azy_rss_category_edd_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Azy_Rss_Category);
   rss_category_edd = eet_data_descriptor_stream_new(&eddc);
   ADD(category, INLINED_STRING);
   ADD(domain, INLINED_STRING);
#undef ADD
}

#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(rss_link_edd, Azy_Rss_Link, #name, name, EET_T_##type)
static void
_azy_rss_link_edd_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Azy_Rss_Link);
   rss_link_edd = eet_data_descriptor_stream_new(&eddc);
   ADD(title, INLINED_STRING);
   ADD(href, INLINED_STRING);
   ADD(rel, INLINED_STRING);
   ADD(type, INLINED_STRING);
   ADD(hreflang, INLINED_STRING);
   ADD(length, ULONG_LONG);
#undef ADD
}


#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(rss_union_rss_edd, Azy_Rss_Type_Rss, #name, name, EET_T_##type)
static void
_azy_rss_rss_edd_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Azy_Rss_Type_Rss);
   rss_union_rss_edd = eet_data_descriptor_stream_new(&eddc);
   ADD(link, INLINED_STRING);
   ADD(skipdays, UINT);
   ADD(skiphours, ULONG_LONG);
   ADD(ttl, UINT);
   ADD(image.url, INLINED_STRING);
   ADD(image.title, INLINED_STRING);
   ADD(image.link, INLINED_STRING);
   ADD(image.desc, INLINED_STRING);
   ADD(image.w, INT);
   ADD(image.h, INT);
#undef ADD
}

#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(rss_union_atom_edd, Azy_Rss_Type_Atom, #name, name, EET_T_##type)
static void
_azy_rss_atom_edd_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Azy_Rss_Type_Atom);
   rss_union_atom_edd = eet_data_descriptor_stream_new(&eddc);
   ADD(id, INLINED_STRING);
   ADD(rights, INLINED_STRING);
   ADD(logo, INLINED_STRING);
   EET_DATA_DESCRIPTOR_ADD_LIST(rss_union_atom_edd, Azy_Rss_Type_Atom, "contributors", contributors, azy_rss_contact_edd_get());
   EET_DATA_DESCRIPTOR_ADD_LIST(rss_union_atom_edd, Azy_Rss_Type_Atom, "authors", authors, azy_rss_contact_edd_get());
   EET_DATA_DESCRIPTOR_ADD_LIST(rss_union_atom_edd, Azy_Rss_Type_Atom, "atom_links", atom_links, azy_rss_link_edd_get());
#undef ADD
}

static void *
_azy_rss_mempool_alloc(size_t size)
{
   Azy_Rss *rss;

   rss = eina_mempool_calloc(rss_mempool, size);
   AZY_MAGIC_SET(rss, AZY_MAGIC_RSS);
   rss->refcount = 1;
   return rss;
}

static void
_azy_rss_mempool_free(void *mem)
{
   Azy_Rss *rss = mem;
   AZY_MAGIC_SET(rss, AZY_MAGIC_NONE);
   eina_mempool_free(rss_mempool, mem);
}

static void
_azy_rss_edd_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Azy_Rss);
   eddc.func.mem_alloc = _azy_rss_mempool_alloc;
   eddc.func.mem_free = _azy_rss_mempool_free;
   rss_edd = eet_data_descriptor_stream_new(&eddc);

   eddc.version = EET_DATA_DESCRIPTOR_CLASS_VERSION;
   eddc.func.type_get = _azy_rss_eet_union_type_get;
   eddc.func.type_set = _azy_rss_eet_union_type_set;
   rss_union_edd = eet_data_descriptor_stream_new(&eddc);

#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(rss_edd, Azy_Rss, #name, name, EET_T_##type)

   EET_DATA_DESCRIPTOR_ADD_MAPPING(rss_union_edd, "rss", rss_union_rss_edd);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(rss_union_edd, "atom", rss_union_atom_edd);
   ADD(title, INLINED_STRING);
   ADD(img_url, INLINED_STRING);
   ADD(generator, INLINED_STRING);
   ADD(desc, INLINED_STRING);
   ADD(updated, ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_LIST(rss_edd, Azy_Rss, "categories", categories, azy_rss_category_edd_get());
   EET_DATA_DESCRIPTOR_ADD_LIST(rss_edd, Azy_Rss, "items", items, azy_rss_item_edd_get());
   EET_DATA_DESCRIPTOR_ADD_UNION(rss_edd, Azy_Rss, "data", data, atom, rss_union_edd);
#undef ADD
}

Eina_Bool
azy_rss_init(const char *type)
{
   rss_mempool = eina_mempool_add(type, "Azy_Rss", NULL, sizeof(Azy_Rss), 64);
   if (rss_mempool)
     {
        eet_init();
        _azy_rss_category_edd_init();
        _azy_rss_contact_edd_init();
        _azy_rss_link_edd_init();
        azy_rss_item_init(type);
        _azy_rss_rss_edd_init();
        _azy_rss_atom_edd_init();
        _azy_rss_edd_init();
        return EINA_TRUE;
     }

   if (strcmp(type, "pass_through")) return azy_rss_init("pass_through");
   azy_rss_item_shutdown();
   EINA_LOG_CRIT("Unable to set up mempool!");
   return EINA_FALSE;
}

void
azy_rss_shutdown(void)
{
   azy_rss_item_shutdown();

   eet_data_descriptor_free(rss_contact_edd);
   eet_data_descriptor_free(rss_category_edd);
   eet_data_descriptor_free(rss_link_edd);
   eet_data_descriptor_free(rss_union_edd);
   eet_data_descriptor_free(rss_union_rss_edd);
   eet_data_descriptor_free(rss_union_atom_edd);
   eet_data_descriptor_free(rss_edd);
   rss_edd = NULL;
   rss_union_edd = NULL;
   rss_union_rss_edd = NULL;
   rss_union_atom_edd = NULL;
   rss_contact_edd = NULL;
   rss_category_edd = NULL;
   rss_link_edd = NULL;
   eet_shutdown();
   eina_mempool_del(rss_mempool);
   rss_mempool = NULL;
}

/*
 * @brief Create a new #Azy_Rss object
 *
 * This function creates a new #Azy_Rss object, returning NULL on failure.
 * @return The new object, or NULL on failure
 */
Azy_Rss *
azy_rss_new(void)
{
   Azy_Rss *rss;

   rss = eina_mempool_calloc(rss_mempool, sizeof(Azy_Rss));
   EINA_SAFETY_ON_NULL_RETURN_VAL(rss, NULL);

   AZY_MAGIC_SET(rss, AZY_MAGIC_RSS);
   rss->refcount = 1;
   return rss;
}

Azy_Rss_Category *
azy_rss_category_new(void)
{
   return calloc(1, sizeof(Azy_Rss_Category));
}

void
azy_rss_category_free(Azy_Rss_Category *cat)
{
   if (!cat) return;
   eina_stringshare_del(cat->category);
   eina_stringshare_del(cat->domain);
   free(cat);
}

/**
 * @brief Free an #Azy_Rss_Contact object
 *
 * This function frees an #Azy_Rss_Contact object.
 * @param c The object (NOT NULL)
 */
void
azy_rss_contact_free(Azy_Rss_Contact *c)
{
   if (!c) return;

   eina_stringshare_del(c->name);
   eina_stringshare_del(c->uri);
   eina_stringshare_del(c->email);
   free(c);
}

/**
 * @brief Free an #Azy_Rss_Link object
 *
 * This function frees an #Azy_Rss_Link object.
 * @param li The object (NOT NULL)
 */
void
azy_rss_link_free(Azy_Rss_Link *li)
{
   if (!li) return;
   eina_stringshare_del(li->href);
   eina_stringshare_del(li->rel);
   eina_stringshare_del(li->type);
   eina_stringshare_del(li->hreflang);
   eina_stringshare_del(li->title);
   free(li);
}

/**
 * @brief Free an #Azy_Rss object
 *
 * This function frees an #Azy_Rss object and all #Azy_Rss_Item subobjects.
 * @param rss The rss object (NOT NULL)
 */
void
azy_rss_free(Azy_Rss *rss)
{
   void *item;

   if (!rss) return;
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return;
     }
   if (rss->refcount) rss->refcount--;
   if (rss->refcount) return;
   eina_stringshare_del(rss->title);
   eina_stringshare_del(rss->desc);
   eina_stringshare_del(rss->img_url);
   EINA_LIST_FREE(rss->categories, item)
     azy_rss_category_free(item);
   eina_stringshare_del(rss->generator);
   if (rss->atom)
     {
        eina_stringshare_del(rss->data.atom.rights);
        eina_stringshare_del(rss->data.atom.id);
        eina_stringshare_del(rss->data.atom.logo);
        EINA_LIST_FREE(rss->data.atom.contributors, item)
          azy_rss_contact_free(item);
        EINA_LIST_FREE(rss->data.atom.authors, item)
          azy_rss_contact_free(item);
        EINA_LIST_FREE(rss->data.atom.atom_links, item)
          azy_rss_link_free(item);
     }
   else
     {
        eina_stringshare_del(rss->data.rss.link);
        eina_stringshare_del(rss->data.rss.image.url);
        eina_stringshare_del(rss->data.rss.image.title);
        eina_stringshare_del(rss->data.rss.image.link);
        eina_stringshare_del(rss->data.rss.image.desc);
     }
   EINA_LIST_FREE(rss->items, item)
     azy_rss_item_free(item);

   AZY_MAGIC_SET(rss, AZY_MAGIC_NONE);

   eina_mempool_free(rss_mempool, rss);
}

/**
 * @brief Increase the refcount of an rss object to prevent its deletion
 * @param rss The object to ref (NOT NULL)
 */
void
azy_rss_ref(Azy_Rss *rss)
{
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return;
     }
   rss->refcount++;
}

Eet_Data_Descriptor *
azy_rss_contact_edd_get(void)
{
   return rss_contact_edd;
}

Eet_Data_Descriptor *
azy_rss_category_edd_get(void)
{
   return rss_category_edd;
}

Eet_Data_Descriptor *
azy_rss_link_edd_get(void)
{
   return rss_link_edd;
}

Eet_Data_Descriptor *
azy_rss_edd_get(void)
{
   return rss_edd;
}

/**
 * @brief Retrieve the list of items in an rss object
 *
 * This function returns a list of #Azy_Rss_Item objects belonging to @p rss
 * @param rss The #Azy_Rss (NOT NULL)
 * @return An #Eina_List of #Azy_Rss_Item objects
 */
const Eina_List *
azy_rss_items_get(const Azy_Rss *rss)
{
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return NULL;
     }
   return rss->items;
}

/**
 * @brief Steal the list of items in an rss object
 *
 * This function returns a list of #Azy_Rss_Item objects belonging to @p rss,
 * separating the list from @p rss. The list must then be manually freed.
 * @param rss The #Azy_Rss (NOT NULL)
 * @return An #Eina_List of #Azy_Rss_Item objects
 */
Eina_List *
azy_rss_items_steal(Azy_Rss *rss)
{
   Eina_List *ret;
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return NULL;
     }
   ret = rss->items;
   rss->items = NULL;
   return ret;
}

/**
 * @brief Retrieve the list of authors of an rss object
 *
 * This function returns a list of #Azy_Rss_Contact objects belonging to @p rss
 * @param rss The #Azy_Rss (NOT NULL)
 * @return An #Eina_List of #Azy_Rss_Contact objects
 */
const Eina_List *
azy_rss_authors_get(const Azy_Rss *rss)
{
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return NULL;
     }
   return rss->data.atom.authors;
}

/**
 * @brief Retrieve the list of contributors of an rss object
 *
 * This function returns a list of #Azy_Rss_Contact objects belonging to @p rss
 * @param rss The #Azy_Rss (NOT NULL)
 * @return An #Eina_List of #Azy_Rss_Contact objects
 */
const Eina_List *
azy_rss_contributors_get(const Azy_Rss *rss)
{
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return NULL;
     }
   return rss->data.atom.contributors;
}

/**
 * @brief Retrieve the list of links from an rss object
 *
 * This function returns a list of #Azy_Rss_Link objects belonging to @p rss
 * @param rss The #Azy_Rss (NOT NULL)
 * @return An #Eina_List of #Azy_Rss_Link objects
 */
const Eina_List *
azy_rss_links_get(const Azy_Rss *rss)
{
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return NULL;
     }
   return rss->data.atom.atom_links;
}

/**
 * @brief Retrieve the list of categories from an rss object
 *
 * This function returns a list of strings belonging to @p rss
 * @param rss The #Azy_Rss (NOT NULL)
 * @return An #Eina_List of stringshared strings
 */
const Eina_List *
azy_rss_categories_get(const Azy_Rss *rss)
{
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return NULL;
     }
   return rss->categories;
}

/**
 * @brief Return whether a feed is an atom feed
 *
 * This function returns true if the feed is an rss atom feed,
 * otherwise it should be assumed to be an rss2 feed.
 * @param rss The #Azy_Rss (NOT NULL)
 * @return EINA_TRUE if feed is an atom feed, else EINA_FALSE
 */
Eina_Bool
azy_rss_atom_get(const Azy_Rss *rss)
{
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return EINA_FALSE;
     }
   return rss->atom;
}

/**
 * @brief Retrieve the skipdays from an rss object
 *
 * This function returns a bitwise ORed number of the days which the
 * rss feed is not updated (eg. (1<<0) flag for skipping the first day of the week)
 * @param rss The #Azy_Rss (NOT NULL)
 * @return The skipdays
 */
unsigned int
azy_rss_skipdays_get(const Azy_Rss *rss)
{
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return 0;
     }
   EINA_SAFETY_ON_TRUE_RETURN_VAL(rss->atom, 0);
   return rss->data.rss.skipdays;
}

/**
 * @brief Retrieve the skipdays from an rss object
 *
 * This function returns the TTL (time-to-live) for the rss feed
 * data. This value is the time for which the feed data should be cached
 * before refreshing.
 * @param rss The #Azy_Rss (NOT NULL)
 * @return The TTL
 */
unsigned int
azy_rss_ttl_get(const Azy_Rss *rss)
{
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return 0;
     }
   EINA_SAFETY_ON_TRUE_RETURN_VAL(rss->atom, 0);
   return rss->data.rss.ttl;
}

/**
 * @brief Retrieve the last update time from an rss object
 *
 * This function returns the last updated time for the rss feed
 * data. This value is the time at which the feed was last updated by the server
 * @param rss The #Azy_Rss (NOT NULL)
 * @return The time, as unixtime
 */
time_t
azy_rss_updated_get(const Azy_Rss *rss)
{
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return 0;
     }
   return rss->updated;
}

/**
 * @brief Retrieve the skiphours from an rss object
 *
 * This function returns a bitwise ORed number of the hours which the
 * rss feed is not updated (eg. (1<<0) flag for skipping the first hour of the day)
 * @param rss The #Azy_Rss (NOT NULL)
 * @return The skiphours
 */
unsigned long long
azy_rss_skiphours_get(const Azy_Rss *rss)
{
   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return 0;
     }
   EINA_SAFETY_ON_TRUE_RETURN_VAL(rss->atom, 0);
   return rss->data.rss.skiphours;
}

#define DEF(NAME, CHECK, MEMBER) \
/**
   @brief Retrieve the NAME of an rss object
   This function will return the NAME of @p rss.  The NAME will be stringshared,
   but still belongs to @p rss.
   @param rss The #Azy_Rss object (NOT NULL)
   @return The NAME, or NULL on failure
 */                                            \
  Eina_Stringshare *                                 \
  azy_rss_##NAME##_get(const Azy_Rss * rss)    \
  {                                            \
     if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS)) \
       {                                       \
          AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);  \
          return NULL;                         \
       }                                       \
     CHECK                                      \
     return rss->MEMBER;                         \
  }

DEF(title, , title)
DEF(img_url, , img_url)
DEF(generator, , generator)
DEF(desc,, desc)
DEF(link, EINA_SAFETY_ON_TRUE_RETURN_VAL(rss->atom, NULL);, data.rss.link)
DEF(rights, EINA_SAFETY_ON_TRUE_RETURN_VAL(!rss->atom, NULL);, data.atom.rights)
DEF(id, EINA_SAFETY_ON_TRUE_RETURN_VAL(!rss->atom, NULL);, data.atom.id)
DEF(logo, EINA_SAFETY_ON_TRUE_RETURN_VAL(!rss->atom, NULL);, data.atom.logo)

#undef DEF

/**
 * @brief Print an rss contact object
 *
 * This function will print an #Azy_Rss_Contact object,
 * optionally indenting @p indent times using @p pre string.
 * @param pre String to indent with
 * @param indent Number of times to indent
 * @param li The rss link object (NOT NULL)
 */
void
azy_rss_contact_print(const char *pre,
                      int indent,
                      const Azy_Rss_Contact *c)
{
   int i;
   if (!c) return;

   if (!pre) pre = "\t";

#define PRINT(X) do {                     \
       if (c->X)                          \
         {                                \
            for (i = 0; i < indent; i++)  \
              printf("%s", pre);          \
            printf("%s: %s\n", #X, c->X); \
         }                                \
  } while (0)

   PRINT(name);
   PRINT(uri);
   PRINT(email);
#undef PRINT
}

/**
 * @brief Print an rss category object
 *
 * This function will print an #Azy_Rss_Category object,
 * optionally indenting @p indent times using @p pre string.
 * @param pre String to indent with
 * @param indent Number of times to indent
 * @param li The rss category object (NOT NULL)
 */
void
azy_rss_category_print(const char *pre,
                   int indent,
                   const Azy_Rss_Category *cat)
{
   int i;
   if (!cat) return;

   if (!pre) pre = "\t";

#define PRINT(X) do {                      \
       if (cat->X)                          \
         {                                 \
            for (i = 0; i < indent; i++)   \
              printf("%s", pre);           \
            printf("%s: %s\n", #X, cat->X); \
         }                                 \
  } while (0)

   PRINT(domain);
   PRINT(category);
#undef PRINT
}

/**
 * @brief Print an rss link object
 *
 * This function will print an #Azy_Rss_Link object,
 * optionally indenting @p indent times using @p pre string.
 * @param pre String to indent with
 * @param indent Number of times to indent
 * @param li The rss link object (NOT NULL)
 */
void
azy_rss_link_print(const char *pre,
                   int indent,
                   const Azy_Rss_Link *li)
{
   int i;
   if (!li) return;

   if (!pre) pre = "\t";

#define PRINT(X) do {                      \
       if (li->X)                          \
         {                                 \
            for (i = 0; i < indent; i++)   \
              printf("%s", pre);           \
            printf("%s: %s\n", #X, li->X); \
         }                                 \
  } while (0)

   PRINT(title);
   PRINT(href);
   PRINT(rel);
   PRINT(type);
   PRINT(hreflang);
#undef PRINT
   if (li->length)
     {
        for (i = 0; i < indent; i++)
          printf("%s", pre);
        printf("length: %zu\n", li->length);
     }
}

/**
 * @brief Print an rss object
 *
 * This function will print to stdout an #Azy_Rss object along with all sub-objects,
 * optionally indenting @p indent times using @p pre string.
 * @param pre String to indent with
 * @param indent Number of times to indent
 * @param rss The rss object (NOT NULL)
 */
void
azy_rss_print(const char *pre,
              int indent,
              const Azy_Rss *rss)
{
   int i;
   Eina_List *l;
   void *item;
   struct tm *t;
   char buf[1024];

   if (!AZY_MAGIC_CHECK(rss, AZY_MAGIC_RSS))
     {
        AZY_MAGIC_FAIL(rss, AZY_MAGIC_RSS);
        return;
     }

   if (!pre) pre = "\t";

#define PRINT(X) do {                       \
       if (rss->X)                          \
         {                                  \
            for (i = 0; i < indent; i++)    \
              printf("%s", pre);            \
            printf("%s: %s\n", #X, rss->X); \
         }                                  \
  } while (0)

   PRINT(title);
   PRINT(img_url);
   PRINT(generator);
   PRINT(desc);

   t = localtime(&rss->updated);
   strftime(buf, sizeof(buf), "%FT%TZ", t);
   for (i = 0; i < indent; i++)
     printf("%s", pre);
   printf("updated: %s\n", buf);
   EINA_LIST_FOREACH(rss->categories, l, item)
     {
        
        azy_rss_category_print(pre, indent + 1, item);
        if (l->next) printf("\n");
     }

   if (rss->atom)
     {
        PRINT(data.atom.rights);
        PRINT(data.atom.id);
        PRINT(data.atom.logo);

#define INDENT(X) do {                   \
       if (rss->X##s)                    \
         {                               \
            for (i = 0; i < indent; i++) \
              printf("%s", pre);         \
            printf("%s: \n", #X);        \
         }                               \
  }                                      \
  while (0)

        INDENT(data.atom.contributor);
        EINA_LIST_FOREACH(rss->data.atom.contributors, l, item)
          {
             azy_rss_contact_print(pre, indent + 1, item);
             if (l->next) printf("\n");
          }
        INDENT(data.atom.author);
        EINA_LIST_FOREACH(rss->data.atom.authors, l, item)
          {
             azy_rss_contact_print(pre, indent + 1, item);
             if (l->next) printf("\n");
          }
        INDENT(data.atom.atom_link);
        EINA_LIST_FOREACH(rss->data.atom.atom_links, l, item)
          {
             azy_rss_link_print(pre, indent + 1, item);
             if (l->next) printf("\n");
          }
     }
   else
     {
        PRINT(data.rss.link);
        PRINT(data.rss.image.url);
        PRINT(data.rss.image.title);
        PRINT(data.rss.image.link);
     }

   INDENT(item);
   EINA_LIST_FOREACH(rss->items, l, item)
     {
        azy_rss_item_print(pre, indent + 1, item);
        if (l->next) printf("\n");
     }
}

/** @} */
