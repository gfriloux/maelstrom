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

static Eina_Mempool *rss_item_mempool = NULL;
static Eet_Data_Descriptor *rss_item_edd = NULL;
static Eet_Data_Descriptor *rss_item_union_edd = NULL;
static Eet_Data_Descriptor *rss_item_union_rss_edd = NULL;
static Eet_Data_Descriptor *rss_item_union_atom_edd = NULL;

#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(rss_item_union_rss_edd, Azy_Rss_Item_Type_Rss, #name, name, EET_T_##type)
static void
_azy_rss_item_rss_edd_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Azy_Rss_Item_Type_Rss);
   rss_item_union_rss_edd = eet_data_descriptor_stream_new(&eddc);
   ADD(link, INLINED_STRING);
   ADD(guid, INLINED_STRING);
   ADD(comment_url, INLINED_STRING);
   ADD(author, INLINED_STRING);
   ADD(content, INLINED_STRING);
   ADD(content_encoded, INLINED_STRING);
   ADD(enclosure.url, INLINED_STRING);
   ADD(enclosure.length, ULONG_LONG);
   ADD(enclosure.type, INLINED_STRING);
   ADD(permalink, UCHAR);
#undef ADD
}

#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(rss_item_union_atom_edd, Azy_Rss_Item_Type_Atom, #name, name, EET_T_##type)
static void
_azy_rss_item_atom_edd_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Azy_Rss_Item_Type_Atom);
   rss_item_union_atom_edd = eet_data_descriptor_stream_new(&eddc);
   ADD(rights, INLINED_STRING);
   ADD(id, INLINED_STRING);
   ADD(icon, INLINED_STRING);
   ADD(updated, ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_LIST(rss_item_union_atom_edd, Azy_Rss_Item_Type_Atom, "contributors", contributors, azy_rss_contact_edd_get());
   EET_DATA_DESCRIPTOR_ADD_LIST(rss_item_union_atom_edd, Azy_Rss_Item_Type_Atom, "authors", authors, azy_rss_contact_edd_get());
   EET_DATA_DESCRIPTOR_ADD_LIST(rss_item_union_atom_edd, Azy_Rss_Item_Type_Atom, "atom_links", atom_links, azy_rss_link_edd_get());
#undef ADD
}

static void *
_azy_rss_item_mempool_alloc(size_t size)
{
   Azy_Rss_Item *item = eina_mempool_calloc(rss_item_mempool, size);
   AZY_MAGIC_SET(item, AZY_MAGIC_RSS_ITEM);
   return item;
}

static void
_azy_rss_item_mempool_free(void *mem)
{
   Azy_Rss_Item *item = mem;
   AZY_MAGIC_SET(item, AZY_MAGIC_NONE);
   eina_mempool_free(rss_item_mempool, mem);
}

static void
_azy_rss_item_edd_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Azy_Rss_Item);
   eddc.func.mem_alloc = _azy_rss_item_mempool_alloc;
   eddc.func.mem_free = _azy_rss_item_mempool_free;
   rss_item_edd = eet_data_descriptor_stream_new(&eddc);

   eddc.version = EET_DATA_DESCRIPTOR_CLASS_VERSION;
   eddc.func.type_get = _azy_rss_eet_union_type_get;
   eddc.func.type_set = _azy_rss_eet_union_type_set;
   rss_item_union_edd = eet_data_descriptor_stream_new(&eddc);

#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(rss_item_edd, Azy_Rss_Item, #name, name, EET_T_##type)

   EET_DATA_DESCRIPTOR_ADD_MAPPING(rss_item_union_edd, "rss", rss_item_union_rss_edd);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(rss_item_union_edd, "atom", rss_item_union_atom_edd);
   ADD(read, UCHAR);
   ADD(title, INLINED_STRING);
   ADD(uuid, INLINED_STRING);
   ADD(desc, INLINED_STRING);
   ADD(published, ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_LIST(rss_item_edd, Azy_Rss_Item, "categories", categories, azy_rss_category_edd_get());
   EET_DATA_DESCRIPTOR_ADD_UNION(rss_item_edd, Azy_Rss_Item, "data", data, atom, rss_item_union_edd);
#undef ADD
}

Eina_Bool
azy_rss_item_init(const char *type)
{
   rss_item_mempool = eina_mempool_add(type, "Azy_Rss_Item", NULL, sizeof(Azy_Rss_Item), 64);
   if (rss_item_mempool)
     {
         _azy_rss_item_rss_edd_init();
         _azy_rss_item_atom_edd_init();
         _azy_rss_item_edd_init();
        return EINA_TRUE;
     }

   if (strcmp(type, "pass_through")) return azy_rss_item_init("pass_through");
   EINA_LOG_CRIT("Unable to set up mempool!");
   return EINA_FALSE;
}

void
azy_rss_item_shutdown(void)
{
   eet_data_descriptor_free(rss_item_union_atom_edd);
   eet_data_descriptor_free(rss_item_union_rss_edd);
   eet_data_descriptor_free(rss_item_union_edd);
   eet_data_descriptor_free(rss_item_edd);
   rss_item_edd = NULL;
   rss_item_union_edd = NULL;
   rss_item_union_rss_edd = NULL;
   rss_item_union_atom_edd = NULL;

   eina_mempool_del(rss_item_mempool);
   rss_item_mempool = NULL;
}

/**
 * @defgroup Azy_Rss_Item RSS item Functions
 * @brief Functions which affect #Azy_Rss_Item objects
 * @{
 */

/*
 * @brief Create a new #Azy_Rss_Item object
 *
 * This function creates a new #Azy_Rss_Item object, returning NULL on failure.
 * @return The new object, or NULL on failure
 */
Azy_Rss_Item *
azy_rss_item_new(void)
{
   Azy_Rss_Item *item;

   item = eina_mempool_calloc(rss_item_mempool, sizeof(Azy_Rss_Item));
   EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);

   AZY_MAGIC_SET(item, AZY_MAGIC_RSS_ITEM);
   return item;
}

void
azy_rss_item_append(Azy_Rss *rss, Azy_Rss_Item *item)
{
   item->uuid = azy_util_uuid_new();
   rss->items = eina_list_append(rss->items, item);
}

/**
 * @brief Free an #Azy_Rss_Item object
 *
 * This function frees an #Azy_Rss_Item object
 * @param item The rss item object
 */
void
azy_rss_item_free(Azy_Rss_Item *item)
{
   void *d;

   if (!item) return;
   if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM))
     {
        AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);
        return;
     }

   eina_stringshare_del(item->title);
   eina_stringshare_del(item->uuid);
   eina_stringshare_del(item->desc);
   EINA_LIST_FREE(item->categories, d)
     azy_rss_category_free(d);
   if (item->atom)
     {
        eina_stringshare_del(item->data.atom.rights);
        eina_stringshare_del(item->data.atom.id);
        eina_stringshare_del(item->data.atom.icon);
        EINA_LIST_FREE(item->data.atom.contributors, d)
          azy_rss_contact_free(d);
        EINA_LIST_FREE(item->data.atom.authors, d)
          azy_rss_contact_free(d);
        EINA_LIST_FREE(item->data.atom.atom_links, d)
          azy_rss_link_free(d);
     }
   else
     {
        eina_stringshare_del(item->data.rss.link);
        eina_stringshare_del(item->data.rss.guid);
        eina_stringshare_del(item->data.rss.comment_url);
        eina_stringshare_del(item->data.rss.author);
        eina_stringshare_del(item->data.rss.content);
        eina_stringshare_del(item->data.rss.content_encoded);
        eina_stringshare_del(item->data.rss.enclosure.url);
        eina_stringshare_del(item->data.rss.enclosure.type);
     }
   AZY_MAGIC_SET(item, AZY_MAGIC_NONE);

   eina_mempool_free(rss_item_mempool, item);
}

Eet_Data_Descriptor *
azy_rss_item_edd_get(void)
{
   return rss_item_edd;
}

/**
 * @brief Return whether an item is from an atom feed
 * @param item The item (NOT NULL)
 * @return EINA_TRUE if item's feed is atom, else EINA_FALSE for rss2
 */
Eina_Bool
azy_rss_item_atom_get(const Azy_Rss_Item *item)
{
   if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM))
     {
        AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);
        return EINA_FALSE;
     }
   return item->atom;
}

/**
 * @brief Return whether an item is marked as read
 * @param item The item (NOT NULL)
 * @return EINA_TRUE if item is marked read, else EINA_FALSE
 */
Eina_Bool
azy_rss_item_read_get(const Azy_Rss_Item *item)
{
   if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM))
     {
        AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);
        return EINA_FALSE;
     }
   return item->read;
}

/**
 * @brief Set whether an item is marked as read
 * @param item The item (NOT NULL)
 * @param is_read EINA_TRUE if item should be marked read, else EINA_FALSE
 */
void
azy_rss_item_read_set(Azy_Rss_Item *item, Eina_Bool is_read)
{
   if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM))
     {
        AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);
        return;
     }
   item->read = !!is_read;
}

/**
 * @brief Retrieve the list of authors of an item object
 *
 * This function returns a list of #Azy_Rss_Contact objects belonging to @p item
 * @param item The #Azy_Rss_Item (NOT NULL)
 * @return An #Eina_List of #Azy_Rss_Contact objects
 */
const Eina_List *
azy_rss_item_authors_get(const Azy_Rss_Item *item)
{
   if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM))
     {
        AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);
        return NULL;
     }
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!item->atom, NULL);
   return item->data.atom.authors;
}

/**
 * @brief Retrieve the list of contributors of an item object
 *
 * This function returns a list of #Azy_Rss_Contact objects belonging to @p item
 * @param item The #Azy_Rss_Item (NOT NULL)
 * @return An #Eina_List of #Azy_Rss_Contact objects
 */
const Eina_List *
azy_rss_item_contributors_get(const Azy_Rss_Item *item)
{
   if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM))
     {
        AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);
        return NULL;
     }
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!item->atom, NULL);
   return item->data.atom.contributors;
}

/**
 * @brief Retrieve the list of links from an item object
 *
 * This function returns a list of #Azy_Rss_Link objects belonging to @p item
 * @param item The #Azy_Rss_Item (NOT NULL)
 * @return An #Eina_List of #Azy_Rss_Link objects
 */
const Eina_List *
azy_rss_item_links_get(const Azy_Rss_Item *item)
{
   if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM))
     {
        AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);
        return NULL;
     }
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!item->atom, NULL);
   return item->data.atom.atom_links;
}

/**
 * @brief Retrieve the publish date from an item object
 *
 * @param item The #Azy_Rss_Item (NOT NULL)
 * @return The publish date in unixtime
 */
time_t
azy_rss_item_pubdate_get(const Azy_Rss_Item *item)
{
   if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM))
     {
        AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);
        return 0;
     }
   return item->published;
}

void
azy_rss_item_enclosure_get(const Azy_Rss_Item *item, Eina_Stringshare **url, Eina_Stringshare **content_type, size_t *length)
{
   if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM))
     {
        AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);
        return;
     }
   EINA_SAFETY_ON_TRUE_RETURN(item->atom);
   if (url) *url = item->data.rss.enclosure.url;
   if (content_type) *content_type = item->data.rss.enclosure.type;
   if (length) *length = item->data.rss.enclosure.length;
}

/**
 * @brief Retrieve the list of categories from an item object
 *
 * This function returns a list of Azy_Rss_Category belonging to @p item
 * @param item The #Azy_Rss_Item (NOT NULL)
 * @return An #Eina_List of Azy_Rss_Category strings
 */
const Eina_List *
azy_rss_item_categories_get(const Azy_Rss_Item *item)
{
   if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM))
     {
        AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);
        return NULL;
     }
   return item->categories;
}

Eina_Bool
azy_rss_item_guid_is_permalink(const Azy_Rss_Item *item)
{
   if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM))
     {
        AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);
        return EINA_FALSE;
     }
   EINA_SAFETY_ON_TRUE_RETURN_VAL(item->atom, EINA_FALSE);
   return item->data.rss.permalink;
}

#define DEF(NAME, CHECK, MEMBER) \
/**
   @brief Retrieve the NAME of an rss item object
   This function will return the NAME of @p item.  The NAME will be stringshared,
   but still belongs to @p item.
   @param item The #Azy_Rss object (NOT NULL)
   @return The NAME, or NULL on failure
 */                                                  \
  Eina_Stringshare *                                   \
  azy_rss_item_##NAME##_get(const Azy_Rss_Item *item)     \
  {                                                  \
     if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM)) \
       {                                             \
          AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);  \
          return NULL;                               \
       }                                             \
     CHECK                                             \
     return item->MEMBER;                              \
  }

DEF(title,, title)
DEF(uuid,, uuid)
DEF(desc,, desc)
DEF(link, EINA_SAFETY_ON_TRUE_RETURN_VAL(item->atom, NULL);, data.rss.link)
DEF(guid, EINA_SAFETY_ON_TRUE_RETURN_VAL(item->atom, NULL);, data.rss.guid)
DEF(comment_url, EINA_SAFETY_ON_TRUE_RETURN_VAL(item->atom, NULL);, data.rss.comment_url)
DEF(author, EINA_SAFETY_ON_TRUE_RETURN_VAL(item->atom, NULL);, data.rss.author)
DEF(content, EINA_SAFETY_ON_TRUE_RETURN_VAL(item->atom, NULL);, data.rss.content)
DEF(content_encoded, EINA_SAFETY_ON_TRUE_RETURN_VAL(item->atom, NULL);, data.rss.content_encoded)
DEF(rights, EINA_SAFETY_ON_TRUE_RETURN_VAL(!item->atom, NULL);, data.atom.rights)
DEF(id, EINA_SAFETY_ON_TRUE_RETURN_VAL(!item->atom, NULL);, data.atom.id)
DEF(icon, EINA_SAFETY_ON_TRUE_RETURN_VAL(!item->atom, NULL);, data.atom.icon)

#undef DEF

/**
 * @brief Print an rss item object
 *
 * This function will print an #Azy_Rss_Item object, optionally indenting
 * @p indent times using @p pre string.
 * @param pre String to indent with
 * @param indent Number of times to indent
 * @param item The rss item object (NOT NULL)
 */
void
azy_rss_item_print(const char *pre,
                   int indent,
                   const Azy_Rss_Item *item)
{
   int i;
   Eina_List *l;
   void *it;
   char buf[256];
   struct tm *t;

   if (!AZY_MAGIC_CHECK(item, AZY_MAGIC_RSS_ITEM))
     {
        AZY_MAGIC_FAIL(item, AZY_MAGIC_RSS_ITEM);
        return;
     }

   if (!pre) pre = "\t";

#define PRINT(X) do {                        \
       if (item->X)                          \
         {                                   \
            for (i = 0; i < indent; i++)     \
              printf("%s", pre);             \
            printf("%s: %s\n", #X, item->X); \
         }                                   \
  } while (0)

   PRINT(title);
   PRINT(desc);
   t = localtime(&item->published);
   strftime(buf, sizeof(buf), "%FT%TZ", t);
   for (i = 0; i < indent; i++)
     printf("%s", pre);
   printf("published: %s\n", buf);

#define INDENT(X) do {                   \
       if (item->X##s)                   \
         {                               \
            for (i = 0; i < indent; i++) \
              printf("%s", pre);         \
            printf("%s: \n", #X);        \
         }                               \
  }                                      \
  while (0)
   INDENT(categorie);
   EINA_LIST_FOREACH(item->categories, l, it)
     {
        azy_rss_category_print(pre, indent + 1, it);
        if (l->next) printf("\n");
     }
   if (item->atom)
     {
        PRINT(data.atom.rights);
        PRINT(data.atom.id);
        PRINT(data.atom.icon);

        t = localtime(&item->data.atom.updated);
        strftime(buf, sizeof(buf), "%FT%TZ", t);
        for (i = 0; i < indent; i++)
          printf("%s", pre);
        printf("updated: %s\n", buf);

        INDENT(data.atom.contributor);
        EINA_LIST_FOREACH(item->data.atom.contributors, l, it)
          {
             azy_rss_contact_print(pre, indent + 1, it);
             if (l->next) printf("\n");
          }
        INDENT(data.atom.author);
        EINA_LIST_FOREACH(item->data.atom.authors, l, it)
          {
             azy_rss_contact_print(pre, indent + 1, it);
             if (l->next) printf("\n");
          }
        INDENT(data.atom.atom_link);
        EINA_LIST_FOREACH(item->data.atom.atom_links, l, it)
          {
             azy_rss_link_print(pre, indent + 1, it);
             if (l->next) printf("\n");
          }
     }
   else
     {
        PRINT(data.rss.link);
        PRINT(data.rss.guid);
        PRINT(data.rss.comment_url);
        PRINT(data.rss.author);
        PRINT(data.rss.enclosure.url);
        PRINT(data.rss.enclosure.type);
     }
}

/** @} */
