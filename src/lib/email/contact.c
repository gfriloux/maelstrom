#include "email_private.h"

static void
_email_contact_free(Email_Contact *ec)
{
   eina_stringshare_del(ec->address);
   eina_stringshare_del(ec->name);
   free(ec);
}

Email_Contact *
email_contact_new(const char *address)
{
   Email_Contact *ec;
   EINA_SAFETY_ON_NULL_RETURN_VAL(address, NULL);

   ec = calloc(1, sizeof(Email_Contact));
   EINA_SAFETY_ON_NULL_RETURN_VAL(ec, NULL);
   ec->address = eina_stringshare_add(address);
   ec->refcount = 1;
   return ec;
}

Email_Contact *
email_contact_ref(Email_Contact *ec)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ec, NULL);
   ec->refcount++;
   return ec;
}

void
email_contact_free(Email_Contact *ec)
{
   if (!ec) return;

   if (--ec->refcount) return;
   _email_contact_free(ec);
}

void
email_contact_name_set(Email_Contact *ec, const char *name)
{
   EINA_SAFETY_ON_NULL_RETURN(ec);
   eina_stringshare_replace(&ec->name, name);
}

void
email_contact_address_set(Email_Contact *ec, const char *address)
{
   EINA_SAFETY_ON_NULL_RETURN(ec);
   eina_stringshare_replace(&ec->address, address);
}

const char *
email_contact_name_get(Email_Contact *ec)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ec, NULL);
   return ec->name;
}

const char *
email_contact_address_get(Email_Contact *ec)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ec, NULL);
   return ec->address;
}
