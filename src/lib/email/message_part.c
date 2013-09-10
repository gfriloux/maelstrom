#include "email_private.h"

Email_Message_Part *
email_message_part_new(const char *name, const char *content_type)
{
   Email_Message_Part *part;

   part = calloc(1, sizeof(Email_Message_Part));
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   part->name = eina_stringshare_add(name);
   part->content_type = eina_stringshare_add(content_type);
   return part;
}

Email_Message_Part *
email_message_part_text_new(const char *text, size_t len)
{
   Email_Message_Part *part;

   part = calloc(1, sizeof(Email_Message_Part));
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   if (text)
     {
        part->content = eina_binbuf_new();
        eina_binbuf_append_length(part->content, (void*)text, len ?: strlen(text));
     }
   part->content_type = eina_stringshare_add("text/plain");
   return part;
}

Email_Message_Part *
email_message_part_ref(Email_Message_Part *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   part->refcount++;
   return part;
}

Email_Message_Part *
email_message_part_new_from_file(const char *filename, const char *content_type)
{
   Email_Message_Part *part;
   Eina_File *f;
   void *file;
   unsigned char *buf;
   size_t size;

   EINA_SAFETY_ON_NULL_RETURN_VAL(filename, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!filename[0], NULL);

   f = eina_file_open(filename, EINA_TRUE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(f, NULL);
   file = eina_file_map_all(f, EINA_FILE_SEQUENTIAL);
   if (!file)
     {
        ERR("mmap of %s failed!", filename);
        goto error;
     }
   part = calloc(1, sizeof(Email_Message_Part));
   if (!part) goto munmap;
   size = eina_file_size_get(f);
   buf = malloc(size);
   if (!buf)
     {
        free(part);
        goto munmap;
     }
   part->refcount = 1;
   memcpy(buf, file, size);
   part->content = eina_binbuf_manage_new_length(buf, size);
   part->name = eina_stringshare_add(filename);
   part->content_type = eina_stringshare_add(content_type);
   eina_file_map_free(f, file);
   eina_file_close(f);
   return part;

munmap:
   ERR("mem alloc failed!");
   eina_file_map_free(f, file);
error:
   eina_file_close(f);
   return NULL;
}

void
email_message_part_free(Email_Message_Part *part)
{
   Email_Message_Part *subpart;

   if (!part) return;
   if (part->refcount) part->refcount--;
   if (part->refcount) return;

   eina_stringshare_del(part->name);
   eina_stringshare_del(part->content_type);
   eina_stringshare_del(part->charset);
   if (part->content) eina_binbuf_free(part->content);
   EINA_LIST_FREE(part->parts, subpart)
     email_message_part_free(subpart);
   free(part);
}

size_t
email_message_part_size_get(const Email_Message_Part *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, 0);
   return part->size;
}

void
email_message_part_name_set(Email_Message_Part *part, const char *name)
{
   EINA_SAFETY_ON_NULL_RETURN(part);
   eina_stringshare_replace(&part->name, name);
}

Eina_Stringshare *
email_message_part_name_get(Email_Message_Part *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   return part->name;
}

unsigned int
email_message_part_number_get(const Email_Message_Part *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, 0);
   return part->num;
}

void
email_message_part_number_set(Email_Message_Part *part, unsigned int num)
{
   EINA_SAFETY_ON_NULL_RETURN(part);
   EINA_SAFETY_ON_TRUE_RETURN(!num);
   part->num = num;
}

void
email_message_part_content_type_set(Email_Message_Part *part, const char *content_type)
{
   EINA_SAFETY_ON_NULL_RETURN(part);
   eina_stringshare_replace(&part->content_type, content_type);
}

Eina_Stringshare *
email_message_part_content_type_get(Email_Message_Part *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   return part->content_type;
}

void
email_message_part_part_add(Email_Message_Part *part, Email_Message_Part *subpart)
{
   EINA_SAFETY_ON_NULL_RETURN(part);
   EINA_SAFETY_ON_NULL_RETURN(subpart);
   part->parts = eina_list_append(part->parts, email_message_part_ref(subpart));
   part->attachments += !!subpart->name;
}

void
email_message_part_part_del(Email_Message_Part *part, Email_Message_Part *subpart)
{
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(part);
   EINA_SAFETY_ON_NULL_RETURN(subpart);
   l = eina_list_data_find_list(part->parts, subpart);
   if (!l) return;
   part->parts = eina_list_remove_list(part->parts, l);
   part->attachments -= !!subpart->name;
   email_message_part_free(subpart);
}

const Eina_List *
email_message_part_parts_get(const Email_Message_Part *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   return part->parts;
}

Eina_Bool
email_message_part_content_set(Email_Message_Part *part, const void *content, size_t size)
{
   unsigned char *buf;
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, EINA_FALSE);

   if (part->content) eina_binbuf_free(part->content);
   part->content = NULL;
   if (!content) return EINA_TRUE;
   buf = malloc(size);
   memcpy(buf, content, size);
   part->content = eina_binbuf_manage_new_length(buf, size);
   return EINA_TRUE;
}

Eina_Bool
email_message_part_content_manage(Email_Message_Part *part, void *content, size_t size)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, EINA_FALSE);

   if (part->content) eina_binbuf_free(part->content);
   part->content = NULL;
   if (!content) return EINA_TRUE;
   part->content = eina_binbuf_manage_new_length(content, size);
   return !!part->content;
}

const Eina_Binbuf *
email_message_part_content_get(Email_Message_Part *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   return part->content;
}

Eina_Binbuf *
email_message_part_content_steal(Email_Message_Part *part)
{
   Eina_Binbuf *buf;
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   buf = part->content;
   part->content = NULL;
   return buf;
}

Email_Message_Part_Encoding
email_message_part_encoding_get(const Email_Message_Part *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, EMAIL_MESSAGE_PART_ENCODING_NONE);
   return part->encoding;
}

void
email_message_part_encoding_set(Email_Message_Part *part, Email_Message_Part_Encoding encoding)
{
   EINA_SAFETY_ON_NULL_RETURN(part);
   EINA_SAFETY_ON_TRUE_RETURN(encoding >= EMAIL_MESSAGE_PART_ENCODING_LAST);
   EINA_SAFETY_ON_TRUE_RETURN(!encoding);
   part->encoding = encoding;
}

void
email_message_part_charset_set(Email_Message_Part *part, const char *charset)
{
   EINA_SAFETY_ON_NULL_RETURN(part);
   eina_stringshare_replace(&part->charset, charset);
}

Eina_Stringshare *
email_message_part_charset_get(const Email_Message_Part *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   return part->charset;
}
