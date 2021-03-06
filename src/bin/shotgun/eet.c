#include "ui.h"
#include <Eet.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_PAM
# include <security/pam_appl.h>
# include <security/pam_misc.h>
#endif

static Eina_Bool _ui_eet_init = EINA_FALSE;

typedef struct UI_Eet_Auth
{
   const char *username;
   const char *domain;
   const char *resource;
   const char *server;
} UI_Eet_Auth;

typedef struct Shotgun_Presence
{
   const char *desc;
   Shotgun_User_Status status;
   int priority;
} Shotgun_Presence;

static Eet_Data_Descriptor *
eet_ss_edd_new(void)
{
   Eet_Data_Descriptor *edd;
   Eet_Data_Descriptor_Class eddc;
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Shotgun_Settings);
   edd = eet_data_descriptor_stream_new(&eddc);
#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Shotgun_Settings, #name, name, EET_T_##type)

   ADD(settings_exist, UCHAR);
   ADD(disable_notify, UCHAR);
   ADD(enable_chat_focus, UCHAR);
   ADD(enable_chat_promote, UCHAR);
   ADD(enable_chat_newselect, UCHAR);
   ADD(enable_chat_typing, UCHAR);
   ADD(enable_chat_noresource, UCHAR);
   ADD(disable_chat_status_entry, UCHAR);
   ADD(enable_account_info, UCHAR);
   ADD(enable_last_account, UCHAR);
   ADD(enable_logging, UCHAR);
   ADD(enable_illume, UCHAR);
   ADD(disable_reconnect, UCHAR);
   ADD(enable_presence_save, UCHAR);
   ADD(disable_list_status, UCHAR);
   ADD(enable_list_sort_alpha, UCHAR);
   ADD(enable_list_offlines, UCHAR);
   ADD(enable_global_otr, UCHAR);
   ADD(enable_mail_notifications, UCHAR);
   ADD(disable_window_on_message, UCHAR);

   ADD(chat_w, INT);
   ADD(chat_h, INT);
   ADD(chat_panes, DOUBLE);
   ADD(list_w, INT);
   ADD(list_h, INT);

   ADD(browser_SETTING, INT);
   ADD(browser, INLINED_STRING);
#undef ADD
   return edd;
}

static Eet_Data_Descriptor *
eet_auth_edd_new(void)
{
   Eet_Data_Descriptor *edd;
   Eet_Data_Descriptor_Class eddc;
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, UI_Eet_Auth);
   edd = eet_data_descriptor_stream_new(&eddc);
#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(edd, UI_Eet_Auth, #name, name, EET_T_##type)

   ADD(username, INLINED_STRING);
   ADD(domain, INLINED_STRING);
   ADD(server, INLINED_STRING);
#undef ADD
   return edd;
}

static Eet_Data_Descriptor *
eet_userinfo_edd_new(Eina_Bool old)
{
   Eet_Data_Descriptor *edd;
   Eet_Data_Descriptor_Class eddc, eddc2;
   /* FIXME: remove compat stuff in a couple weeks */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Contact_Info);
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc2, Shotgun_User_Info);
   if (old)
     edd = eet_data_descriptor_stream_new(&eddc2);
   else
     edd = eet_data_descriptor_stream_new(&eddc);
#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Contact_Info, #name, name, EET_T_##type)

   ADD(full_name, INLINED_STRING);
   ADD(photo.type, INLINED_STRING);
   ADD(photo.sha1, INLINED_STRING);
   if (!old)
     ADD(after, INLINED_STRING);
#undef ADD
   return edd;
}

static Eet_Data_Descriptor *
eet_presence_edd_new(void)
{
   Eet_Data_Descriptor *edd;
   Eet_Data_Descriptor_Class eddc;
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Shotgun_Presence);
   edd = eet_data_descriptor_stream_new(&eddc);
#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Shotgun_Presence, #name, name, EET_T_##type)

   ADD(desc, INLINED_STRING);
   ADD(status, UINT);
   ADD(priority, INT);
#undef ADD
   return edd;
}

static void
userinfo_add(Shotgun_Auth *auth, Evas_Object *img, Contact_Info *info)
{
   char buf[1024];
   const char *jid;
   void *img_data;
   Eet_Data_Descriptor *edd;
   Eet_File *ef = shotgun_data_get(auth);
   int w, h;
   Eina_Bool success;

   jid = shotgun_jid_get(auth);
   edd = eet_userinfo_edd_new(EINA_FALSE);
   snprintf(buf, sizeof(buf), "%s/%s", jid, info->jid);
   success = eet_data_write_cipher(ef, edd, buf, shotgun_password_get(auth), info, 0);
   eet_data_descriptor_free(edd);
   if (!success)
     {
        ERR("Failed to write userinfo for %s!", info->jid);
        eet_data_descriptor_free(edd);
        return;
     }
   INF("Wrote encrypted userinfo for %s to disk", info->jid);
   if ((!info->photo.data) || (!img)) return;
   snprintf(buf, sizeof(buf), "%s/%s/img", jid, info->jid);
   img_data = evas_object_image_data_get(img, EINA_FALSE);
   if (img_data)
     {
        evas_object_image_size_get(img, &w, &h);
        eet_data_image_write(ef, buf, img_data, w, h, evas_object_image_alpha_get(img), 5, 100, 0);
        info->photo.size = w * h * sizeof(int);
        INF("Wrote contact image %"PRIu64" bytes", w * h * sizeof(int));
     }
   eet_sync(ef);
   return;
}


static Contact_Info *
userinfo_get(Shotgun_Auth *auth, const char *jid)
{
   char buf[1024];
   const char *me;
   Eet_Data_Descriptor *edd;
   Contact_Info *ci;
   Shotgun_User_Info *info;
   Eet_File *ef = shotgun_data_get(auth);
   unsigned int w, h;

   edd = eet_userinfo_edd_new(EINA_FALSE);
   me = shotgun_jid_get(auth);
   snprintf(buf, sizeof(buf), "%s/%s", me, jid);
   ci = eet_data_read_cipher(ef, edd, buf, shotgun_password_get(auth));
   eet_data_descriptor_free(edd);
   if (ci)
     {
        if (ci->photo.sha1)
          INF("Read encrypted userinfo for %s from disk with image %s", jid, ci->photo.sha1);
        else
          INF("Read encrypted userinfo for %s from disk", jid);
     }
   else
     {
        edd = eet_userinfo_edd_new(EINA_TRUE);
        info = eet_data_read_cipher(ef, edd, buf, shotgun_password_get(auth));
        eet_data_descriptor_free(edd);
        if (info)
          {
             INF("Old format userinfo detected for %s, updating", jid);
             ci = realloc(info, sizeof(Contact_Info));
          }
        else
          INF("Userinfo for %s does not exist", jid);
     }
   snprintf(buf, sizeof(buf), "%s/%s/img", me, jid);
   if (ci)
     {
        if (eet_data_image_header_read(ef, buf, &w, &h, NULL, NULL, NULL, NULL))
          ci->photo.size = w * h * sizeof(int);
     }
   return ci;
}

static void
userinfo_thread_done(Contact *c, Ecore_Thread *et)
{
   Contact_Info *ci;
   c->info_thread = NULL;

   if (c->info_img) evas_object_del(c->info_img);
   c->info_img = NULL;
   if (et && c->list_item) c->list->list_item_update[c->list->mode](c->list_item);
   ci = ecore_thread_local_data_find(et, "info");
   if (ci && ci->dead)
     {
        if (ci == c->info) c->info = NULL;
        contact_info_free(NULL, ci);
        if (!c->info) return;
     }
   if (c->dead)
     {
        contact_free(c);
        return;
     }
   if ((c->cur && c->cur->vcard && ci &&
       ((ci->photo.sha1 != c->cur->photo) ||
        (c->cur->photo && (!ci->photo.size)))))
     {
        INF("VCARD for %s not current; fetching.", c->base->jid);
        if (contact_vcard_request(c)) return;
        CRI("VCARD REQUESTED WHILE VCARD REQUEST IN PROGRESS! BUG!!!!");
     }
   c->vcard_request = EINA_FALSE;
}

static void
userinfo_thread_cancel(Contact *c, Ecore_Thread *et)
{
   Contact_Info *ci;
   c->info_thread = NULL;
   ci = ecore_thread_local_data_find(et, "info");
   if (ci && (ci != c->info))
     {
        if (ci->dead) contact_info_free(NULL, ci);
     }
   c->vcard_request = EINA_FALSE;
   if (c->dead) contact_free(c);
}

static void
userinfo_thread_new(Contact *c, Ecore_Thread *et EINA_UNUSED)
{
   userinfo_add(c->list->account, c->info_img, c->info);
}

static void
userinfo_thread_get(Contact *c, Ecore_Thread *et EINA_UNUSED)
{
   c->info = userinfo_get(c->list->account, c->base->jid);
}

Eina_Bool
ui_eet_init(Shotgun_Auth *auth)
{
   const char *home;
   char buf[4096];
   Eet_File *ef;

   if (_ui_eet_init) return EINA_TRUE;
   if (!util_configdir_create()) return EINA_FALSE;
   home = util_configdir_get();
   eet_init();

   if (shotgun_data_get(auth))
     {
        eet_shutdown();
        goto out;
     }
   snprintf(buf, sizeof(buf), "%s/shotgun.eet", home);
   ef = eet_open(buf, EET_FILE_MODE_READ_WRITE);
   if (!ef) goto error;
   shotgun_data_set(auth, ef);
out:
   _ui_eet_init = EINA_TRUE;
   return EINA_TRUE;
error:
   ERR("Could not open account info file!");
   eet_shutdown();
   return EINA_FALSE;
}

void
ui_eet_shutdown(Shotgun_Auth *auth)
{
   if (!auth) return;
   eet_close(shotgun_data_get(auth));
   eet_shutdown();
}

Shotgun_Auth *
_ui_eet_auth_get(Eet_File *ef, const char *jid)
{
   Eet_Data_Descriptor *edd;
   UI_Eet_Auth *a;
   Shotgun_Auth *auth;
   char buf[4096], *p;
   int size;

   edd = eet_auth_edd_new();
   a = eet_data_read(ef, edd, jid);
   eet_data_descriptor_free(edd);
   if (!a)
     {
        eet_close(ef);
        return NULL;
     }
   auth = shotgun_new(a->server, a->username, a->domain);
   /* FIXME: use resource */
   if (auth)
     {
        snprintf(buf, sizeof(buf), "%s/pw", shotgun_jid_get(auth));
        p = eet_read_cipher(ef, buf, &size, shotgun_jid_get(auth));
        if (p) shotgun_password_set(auth, p);
        free(p);
        shotgun_data_set(auth, ef);
     }
   else
     {
        ERR("Could not create auth info!");
        eet_close(ef);
     }
   eina_stringshare_del(a->server);
   eina_stringshare_del(a->username);
   eina_stringshare_del(a->domain);
   eina_stringshare_del(a->resource);
   free(a);
   return auth;
}

Shotgun_Auth *
ui_eet_auth_get(const char *name, const char *domain)
{
   Eet_File *ef;
   char *jid, buf[4096];
   const char *home;
   int size;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(!util_configdir_create(), NULL);
   eet_init();
   home = util_configdir_get();

   snprintf(buf, sizeof(buf), "%s/shotgun.eet", home);
   ef = eet_open(buf, EET_FILE_MODE_READ_WRITE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ef, NULL);

   jid = eet_read(ef, "last_account", &size);
   if ((!jid) || (jid[size - 1]))
     {
        jid = NULL;
        if (!name)
          {
             eet_close(ef);
             INF("last_account not set");
             return NULL;
          }
        if (domain)
          snprintf(buf, sizeof(buf), "%s@%s", name, domain);
        else
          strncpy(buf, name, sizeof(buf));
     }
   else
     {
        strncpy(buf, jid, sizeof(buf));
        free(jid);
     }
   return _ui_eet_auth_get(ef, buf);
}

void
ui_eet_auth_set(Shotgun_Auth *auth, Shotgun_Settings *ss, Eina_Bool use_auth)
{
   Eet_File *ef;
   const char *s, *jid;
   char buf[1024];
   Eet_Data_Descriptor *edd;
   UI_Eet_Auth a;

   if (!ss) return;
   ef = shotgun_data_get(auth);
   jid = shotgun_jid_get(auth);
   a.username = shotgun_username_get(auth);
   a.domain = shotgun_domain_get(auth);
   a.resource = shotgun_resource_get(auth);
   a.server = shotgun_servername_get(auth);

   if (ss->enable_last_account)
     eet_write(ef, "last_account", jid, strlen(jid) + 1, 0);
   else
     eet_delete(ef, "last_account");

   snprintf(buf, sizeof(buf), "%s/pw", jid);
   if (!ss->enable_account_info)
     {
        eet_delete(ef, buf);
        return;
     }
   /* FIXME: list for multiple accounts */
   edd = eet_auth_edd_new();

   eet_data_write(ef, edd, jid, &a, 0);
   eet_data_descriptor_free(edd);
   if (!use_auth)
     {
        s = shotgun_password_get(auth);
        if (!s) return;
        /* feeble attempt at ciphering, but at least it isn't plaintext */
        eet_write_cipher(ef, buf, s, strlen(s) + 1, 0, jid);
        return;
     }
#ifdef HAVE_PAM
   pam_handle_t *p;
   struct pam_conv pc;

   if (pam_start("shotgun", user, conv, &p) != PAM_SUCCESS)
     {
        ERR("Could not start PAM session! Password not saved!");
        return;
     }

#else
   CRI("PAM support not detected! Unable to store password!");
   return;
#endif
}

void
ui_eet_userinfo_update(Shotgun_Auth *auth, const char *jid, Contact_Info *ci)
{
   char buf[1024];
   const char *me;
   Eet_Data_Descriptor *edd;
   Eet_File *ef = shotgun_data_get(auth);

   me = shotgun_jid_get(auth);
   edd = eet_userinfo_edd_new(EINA_FALSE);
   snprintf(buf, sizeof(buf), "%s/%s", me, jid);
   if (eet_data_write_cipher(ef, edd, buf, shotgun_password_get(auth), ci, 0))
     INF("Updated userinfo for %s", jid);
   eet_data_descriptor_free(edd);
}

void
ui_eet_userinfo_fetch(Contact *c, Eina_Bool new)
{
   Ecore_Thread_Cb cb = (Ecore_Thread_Cb)(new ? userinfo_thread_new : userinfo_thread_get);
   if (c->info_thread) return;
   if (new)
     {
        c->info = realloc(c->info, sizeof(Contact_Info));
        c->info->after = eina_stringshare_ref(c->after);
     }
   c->info_thread = ecore_thread_run((Ecore_Thread_Cb)cb, (Ecore_Thread_Cb)userinfo_thread_done,
                                     (Ecore_Thread_Cb)userinfo_thread_cancel, c);
   ecore_thread_local_data_add(c->info_thread, "info", c->info, NULL, EINA_FALSE);
}

Shotgun_Settings *
ui_eet_settings_get(Shotgun_Auth *auth)
{
   Eet_Data_Descriptor *edd;
   Eet_File *ef = shotgun_data_get(auth);
   Shotgun_Settings *ss = shotgun_settings_get(auth);

   if (ss) return ss;
   edd = eet_ss_edd_new();
   ss = eet_data_read(ef, edd, "settings");
   eet_data_descriptor_free(edd);
   shotgun_settings_set(auth, ss);
   return ss;
}

void
ui_eet_settings_set(Shotgun_Auth *auth, Shotgun_Settings *ss)
{
   Eet_Data_Descriptor *edd;
   Eet_File *ef = shotgun_data_get(auth);

   if (!ss) return;
   edd = eet_ss_edd_new();
   eet_data_write(ef, edd, "settings", ss, 0);
   eet_data_descriptor_free(edd);
}

void
ui_eet_presence_set(Shotgun_Auth *auth)
{
   Eet_Data_Descriptor *edd;
   Eet_File *ef = shotgun_data_get(auth);
   Shotgun_Presence p;
   char buf[4096];

   if (snprintf(buf, sizeof(buf), "%s/presence", shotgun_jid_get(auth)) < 1) return;
   edd = eet_presence_edd_new();
   p.desc = shotgun_presence_get(auth, &p.status, &p.priority);
   eet_data_write(ef, edd, buf, &p, 0);
   eet_data_descriptor_free(edd);
}

Eina_Bool
ui_eet_presence_get(Shotgun_Auth *auth)
{
   Eet_Data_Descriptor *edd;
   Eet_File *ef = shotgun_data_get(auth);
   Shotgun_Presence *p;
   char buf[4096];

   if (snprintf(buf, sizeof(buf), "%s/presence", shotgun_jid_get(auth)) < 1) return EINA_FALSE;
   edd = eet_presence_edd_new();
   p = eet_data_read(ef, edd, buf);
   eet_data_descriptor_free(edd);
   shotgun_presence_desc_manage(auth, p->desc);
   shotgun_presence_priority_set(auth, p->priority);
   shotgun_presence_status_set(auth, p->status);
   free(p);
   return EINA_TRUE;
}
