#include "ui.h"
#ifdef HAVE_DBUS

#include <Eldbus.h>
#define NOTIFY_OBJECT    "/org/freedesktop/Notifications"
#define NOTIFY_BUS       "org.freedesktop.Notifications"
#define NOTIFY_INTERFACE "org.freedesktop.Notifications"

static Eldbus_Connection *ui_dbus_conn = NULL;
static Eldbus_Service_Interface *ui_core_iface = NULL;
static Eldbus_Object *ui_e_object = NULL;

typedef enum
{
   UI_DBUS_SIGNAL_MESSAGE_NEW,
   UI_DBUS_SIGNAL_MESSAGE_SELF_NEW,
   UI_DBUS_SIGNAL_STATUS_CHANGE,
   UI_DBUS_SIGNAL_STATUS_CHANGE_SELF,
   UI_DBUS_SIGNAL_LINK_NEW,
   UI_DBUS_SIGNAL_LINK_SELF_NEW,
   UI_DBUS_SIGNAL_LINK_DEL,
   UI_DBUS_SIGNAL_CONNECTED
} UI_Dbus_Signals;

static const Eldbus_Signal core_signals[] =
{
   [UI_DBUS_SIGNAL_MESSAGE_NEW] = {"new_msg", ELDBUS_ARGS({"s", "jid"}, {"s", "message"}), 0},
   [UI_DBUS_SIGNAL_MESSAGE_SELF_NEW] = {"new_msg_self", ELDBUS_ARGS({"s", "jid"}, {"s", "message"}), 0},
   [UI_DBUS_SIGNAL_STATUS_CHANGE] = {"status",
     ELDBUS_ARGS({"s", "jid"}, {"s", "resource"}, {"s", "description"}, {"u", "Shotgun_User_Status"},
                {"u", "Shotgun_Presence_Type"}, {"i", "priority"}), 0},
   [UI_DBUS_SIGNAL_STATUS_CHANGE_SELF] = {"status_self",
     ELDBUS_ARGS({"s", "description"}, {"u", "Shotgun_User_Status"}, {"i", "priority"}), 0},
   [UI_DBUS_SIGNAL_LINK_NEW] = {"link", ELDBUS_ARGS({"s", "url"}), 0},
   [UI_DBUS_SIGNAL_LINK_SELF_NEW] = {"link_self", ELDBUS_ARGS({"s", "url"}), 0},
   [UI_DBUS_SIGNAL_LINK_DEL] = {"link_del", ELDBUS_ARGS({"s", "url"}), 0},
   [UI_DBUS_SIGNAL_CONNECTED] = {"connected", ELDBUS_ARGS({"b", "connection_state"}), 0},
   {NULL, NULL, 0}
};

/////////////////// NOTIFY ///////////////////////////////////////

static Eina_Bool ui_notify_markup = EINA_FALSE;
static Eldbus_Object *ui_notify_object = NULL;

static void
_notify_capabilities_cb(void *data EINA_UNUSED, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   char *val;
   Eldbus_Message_Iter *arr;

   if (eldbus_message_error_get(msg, NULL, NULL)) return;
   if (!eldbus_message_arguments_get(msg, "as", &arr)) return;

   while (eldbus_message_iter_get_and_next(arr, 's', &val))
     if (!strcmp(val, "body-markup"))
       {
          ui_notify_markup = EINA_TRUE;
          break;
       }
}

static void
_notify_clear(void)
{
   if (!ui_notify_object) return;
   eldbus_object_unref(ui_notify_object);
   ui_notify_object = NULL;
}

static void
_notify_refresh(void)
{
   Eldbus_Message *msg;

   _notify_clear();
   ui_notify_object = eldbus_object_get(ui_dbus_conn, NOTIFY_BUS, NOTIFY_OBJECT);
   msg = eldbus_message_method_call_new(NOTIFY_BUS, NOTIFY_OBJECT, NOTIFY_INTERFACE, "GetCapabilities");
   eldbus_object_send(ui_notify_object, msg, _notify_capabilities_cb, NULL, -1);
}

static void
_notify_nameowner_change_cb(void *data EINA_UNUSED, const char *bus EINA_UNUSED, const char *old_id EINA_UNUSED, const char *new_id)
{
   if ((!new_id) || (!new_id[0]))
     _notify_clear();
   else
     _notify_refresh();
}

static void
_notify_nameowner_cb(void *data EINA_UNUSED, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   const char *error_name, *error_text;

   if (eldbus_message_error_get(msg, &error_name, &error_text))
     ERR("%s: %s", error_name, error_text);
   else
     _notify_refresh();
}

#if 0
static Eina_Bool
_notify_image_serialize(Eldbus_Message_Iter *sub, Evas_Object *img)
{
   int x, y, w = 0, h = 0, channels = 4, bits_per_sample = 8;
   unsigned char *d, *imgdata, *data;
   int rowstride, img_rowstride;
   int *s;
   size_t data_len;
   Eina_Bool has_alpha = EINA_TRUE;
   Eldbus_Message_Iter *entry, *st;
   
   evas_object_image_size_get(img, &w, &h);
   if ((w <= 0) || (h <= 0)) return EINA_FALSE;
   imgdata = evas_object_image_data_get(img, EINA_FALSE);
   if (!imgdata) return EINA_FALSE;
   
   data = malloc(4 * w * h);
   img_rowstride = 4 * w;
   
   rowstride = evas_object_image_stride_get(img);
   for (y = 0; y < h; y++)
     {
        s = (int *)(imgdata + (y * rowstride));
        d = data + (y * img_rowstride);
        
        for (x = 0; x < w; x++, s++)
          {
             *d++ = (*s >> 16) & 0xff;
             *d++ = (*s >> 8) & 0xff;
             *d++ = (*s) & 0xff;
             *d++ = (*s >> 24) & 0xff;
          }
     }
   evas_object_image_data_set(img, imgdata);
   data_len = ((h - 1) * img_rowstride) + (w * (((channels * bits_per_sample) + 7) / 8));

   eldbus_message_iter_arguments_append(sub, "s", "image-data");
   st = eldbus_message_iter_container_new(entry, 'v', NULL);
   eldbus_message_iter_arguments_append(st, "iiibii", w, h, img_rowstride, has_alpha, bits_per_sample, channels, &entry);
   eldbus_message_iter_fixed_array_get(st, "y", data, data_len

   if (dbus_message_iter_open_container(iter, DBUS_TYPE_STRUCT, NULL, &sub))
     {
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &(img->width));
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &(img->height));
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &(img->rowstride));
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_BOOLEAN, &(img->has_alpha));
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &(img->bits_per_sample));
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &(img->channels));
        if (dbus_message_iter_open_container(&sub, DBUS_TYPE_ARRAY, "y", &arr))
          {
             dbus_message_iter_append_fixed_array(&arr, DBUS_TYPE_BYTE, &(img->data), data_len);
             dbus_message_iter_close_container(&sub, &arr);
   if (dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry))
     {
        dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
        if (dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, type_str, &variant))
          {
             func(&variant, data);
             dbus_message_iter_close_container(&entry, &variant);
          }
        else
          {
             ERR("dbus_message_iter_open_container() failed");
          }
        dbus_message_iter_close_container(iter, &entry);
}
#endif
/////////////////// METHODS ///////////////////////////////////////


static Eldbus_Message *
_dbus_quit_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Contact_List *cl = eldbus_service_object_data_get(iface, "contact_list");
   shotgun_disconnect(cl->account);
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_dbus_link_list_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Contact_List *cl = eldbus_service_object_data_get(iface, "contact_list");
   Link *i;
   Eldbus_Message *reply;
   Eldbus_Message_Iter *iter, *arr;

   reply = eldbus_message_method_return_new(msg);
   iter = eldbus_message_iter_get(reply);
   arr = eldbus_message_iter_container_new(iter, 'a', "s");

   EINA_INLIST_FOREACH(cl->image_list, i)
     eldbus_message_iter_basic_append(arr, 's', i->addr);
   eldbus_message_iter_container_close(iter, arr);
   return reply;
}

static Eldbus_Message *
_dbus_link_open_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Contact_List *cl = eldbus_service_object_data_get(iface, "contact_list");
   const char *url;

   if (eldbus_message_arguments_get(msg, "s", &url))
     {
        if (url && url[0]) chat_link_open(cl, url);
     }
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_dbus_list_get_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Contact_List *cl = eldbus_service_object_data_get(iface, "contact_list");
   Eina_List *l;
   Contact *c;
   Eldbus_Message *reply;
   Eldbus_Message_Iter *iter, *arr;

   reply = eldbus_message_method_return_new(msg);
   iter = eldbus_message_iter_get(reply);
   arr = eldbus_message_iter_container_new(iter, 'a', "s");

   EINA_LIST_FOREACH(cl->users_list, l, c)
     {
        if (c->cur && c->cur->status && c->base->subscription)
          eldbus_message_iter_basic_append(arr, 's', c->base->jid);
     }
   eldbus_message_iter_container_close(iter, arr);
   return reply;
}

static Eldbus_Message *
_dbus_list_get_all_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Contact_List *cl = eldbus_service_object_data_get(iface, "contact_list");
   Eina_List *l;
   Contact *c;
   Eldbus_Message *reply;
   Eldbus_Message_Iter *iter, *arr;

   reply = eldbus_message_method_return_new(msg);
   iter = eldbus_message_iter_get(reply);
   arr = eldbus_message_iter_container_new(iter, 'a', "s");

   EINA_LIST_FOREACH(cl->users_list, l, c)
     eldbus_message_iter_basic_append(arr, 's', c->base->jid);
   eldbus_message_iter_container_close(iter, arr);
   return reply;
}

static Eldbus_Message *
_dbus_contact_status_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Contact_List *cl = eldbus_service_object_data_get(iface, "contact_list");
   Eldbus_Message *reply;
   Contact *c;
   char *name, *s;
   const char *error_name, *error_text;

   if (eldbus_message_error_get(msg, &error_name, &error_text))
     {
        ERR("%s: %s", error_name, error_text);
        goto error;
     }
   if (!eldbus_message_arguments_get(msg, "s", &name)) goto error;
   if ((!name) || (!name[0])) goto error;
   s = strchr(name, '/');
   if (s) name = strndupa(name, s - name);
   c = eina_hash_find(cl->users, name);
   if (!c) goto error;
   reply = eldbus_message_method_return_new(msg);
   eldbus_message_arguments_append(reply, "sui", c->description, c->status, c->priority);
   return reply;
error:
   reply = eldbus_message_error_new(msg, "org.shotgun.contact.invalid", "Contact specified was invalid or not found!");
   return reply;
}

static Eldbus_Message *
_dbus_contact_send_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Contact_List *cl = eldbus_service_object_data_get(iface, "contact_list");
   Eldbus_Message *reply;
   Contact *c;
   char *p, *name, *s;
   Shotgun_Message_Status st;
   Eina_Bool ret = EINA_FALSE;
   const char *error_name, *error_text;

   if (eldbus_message_error_get(msg, &error_name, &error_text))
     {
        ERR("%s: %s", error_name, error_text);
        goto error;
     }
   reply = eldbus_message_method_return_new(msg);
   if (!eldbus_message_arguments_get(msg, "ssu", &name, &s, &st)) goto error;
   if ((!name) || (!name[0]) || (!s)) goto error;
   p = strchr(name, '/');
   if (p) name = strndupa(name, p - name);
   c = eina_hash_find(cl->users, name);
   if (!c) goto error;
   ret = shotgun_message_send(c->base->account, contact_jid_send_get(c), s, st, c->xhtml_im);
error:
   eldbus_message_arguments_append(reply, "b", ret);
   return reply;
}

static Eldbus_Message *
_dbus_contact_send_echo_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Contact_List *cl = eldbus_service_object_data_get(iface, "contact_list");
   Eldbus_Message *reply;
   Contact *c;
   char *p, *name, *s;
   Shotgun_Message_Status st;
   Eina_Bool ret = EINA_FALSE;
   const char *error_name, *error_text;

   if (eldbus_message_error_get(msg, &error_name, &error_text))
     {
        ERR("%s: %s", error_name, error_text);
        goto error;
     }
   reply = eldbus_message_method_return_new(msg);
   if (!eldbus_message_arguments_get(msg, "ssu", &name, &s, &st)) goto error;
   if ((!name) || (!name[0]) || (!s)) goto error;
   p = strchr(name, '/');
   if (p) name = strndupa(name, p - name);
   c = eina_hash_find(cl->users, name);
   if (!c) goto error;

   ret = shotgun_message_send(c->base->account, contact_jid_send_get(c), s, st, c->xhtml_im);
   if (ret)
     chat_message_insert(c, "me", s, EINA_TRUE);
error:
   eldbus_message_arguments_append(reply, "b", ret);
   return reply;
}

static Eldbus_Message *
_dbus_contact_info_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Contact_List *cl = eldbus_service_object_data_get(iface, "contact_list");
   char *name, *s;
   const char *d;
   Contact *c;
   Eldbus_Message *reply;
   const char *error_name, *error_text;

   if (eldbus_message_error_get(msg, &error_name, &error_text))
     {
        ERR("%s: %s", error_name, error_text);
        goto error;
     }
   if (!eldbus_message_arguments_get(msg, "s", &name)) goto error;
   if ((!name) || (!name[0])) goto error;
   s = strchr(name, '/');
   if (s) name = strndupa(name, s - name);
   c = eina_hash_find(cl->users, name);
   if (!c) goto error;
   reply = eldbus_message_method_return_new(msg);
   if (c->cur && c->cur->photo)
     {
        size_t size = sizeof(char) * (strlen(shotgun_jid_get(cl->account)) + strlen(c->base->jid) + 6);
        s = alloca(size);
        snprintf(s, size, "%s/%s/img", shotgun_jid_get(cl->account), c->base->jid);
     }
   else s = "";
   d = contact_name_get(c);
   eldbus_message_arguments_append(reply, "ssui", d, s, c->status, c->priority);
   return reply; /* get icon name from eet file */
error:
   reply = eldbus_message_error_new(msg, "org.shotgun.contact.invalid", "Contact specified was invalid or not found!");
   return reply;
}

static Eldbus_Message *
_dbus_contact_icon_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   Contact_List *cl = eldbus_service_object_data_get(iface, "contact_list");
   char *name, *s;
   Contact *c;
   Eldbus_Message *reply;
   const char *error_name, *error_text;

   if (eldbus_message_error_get(msg, &error_name, &error_text))
     {
        ERR("%s: %s", error_name, error_text);
        goto error;
     }
   if (!eldbus_message_arguments_get(msg, "s", &name)) goto error;
   if ((!name) || (!name[0])) goto error;
   s = strchr(name, '/');
   if (s) name = strndupa(name, s - name);
   c = eina_hash_find(cl->users, name);
   if (!c) goto error;
   reply = eldbus_message_method_return_new(msg);
   if (c->cur && c->cur->photo)
     {
        size_t size = sizeof(char) * (strlen(shotgun_jid_get(cl->account)) + strlen(c->base->jid) + 6);
        s = alloca(size);
        snprintf(s, size, "%s/%s/img", shotgun_jid_get(cl->account), c->base->jid);
     }
   else s = "";
   eldbus_message_arguments_append(reply, "s", s);
   return reply; /* get icon name from eet file */
error:
   reply = eldbus_message_error_new(msg, "org.shotgun.contact.invalid", "Contact specified was invalid or not found!");
   return reply;
}

void
ui_dbus_signal_message_self(Contact_List *cl EINA_UNUSED, const char *jid, const char *s)
{
   eldbus_service_signal_emit(ui_core_iface, UI_DBUS_SIGNAL_MESSAGE_SELF_NEW, jid, s);
}

void
ui_dbus_signal_message(Contact_List *cl EINA_UNUSED, Contact *c, Shotgun_Event_Message *msg)
{
   eldbus_service_signal_emit(ui_core_iface, UI_DBUS_SIGNAL_MESSAGE_NEW, c->base->jid, msg->msg);
}

void
ui_dbus_signal_status(Contact *c, Shotgun_Event_Presence *pres)
{
   const char *res, *desc;

   res = strrchr(pres->jid, '/');
   if (res) res++;
   else res = "/";
   desc = pres->description ?: "";
   eldbus_service_signal_emit(ui_core_iface, UI_DBUS_SIGNAL_STATUS_CHANGE, 
     c->base->jid, res, desc, pres->status, pres->type, pres->priority);
}

void
ui_dbus_signal_status_self(Contact_List *cl)
{
   const char *desc;
   Shotgun_User_Status st;
   int priority;

   desc = shotgun_presence_get(cl->account, &st, &priority);
   desc = desc ?: "";

   eldbus_service_signal_emit(ui_core_iface, UI_DBUS_SIGNAL_STATUS_CHANGE_SELF, desc, st, priority);
}

void
ui_dbus_signal_connect_state(Contact_List *cl)
{
   Eina_Bool state;

   state = (shotgun_connection_state_get(cl->account) == SHOTGUN_CONNECTION_STATE_CONNECTED);

   eldbus_service_signal_emit(ui_core_iface, UI_DBUS_SIGNAL_CONNECTED, state);
}

void
ui_dbus_signal_link(Contact_List *cl EINA_UNUSED, const char *url, Eina_Bool del, Eina_Bool self)
{
   UI_Dbus_Signals sig;

   if (del)
     sig = UI_DBUS_SIGNAL_LINK_DEL;
   else if (self)
     sig = UI_DBUS_SIGNAL_LINK_SELF_NEW;
   else
     sig = UI_DBUS_SIGNAL_LINK_NEW;
   eldbus_service_signal_emit(ui_core_iface, sig, url);
}

static const Eldbus_Method core_methods[] =
{
      { "quit", NULL, NULL, _dbus_quit_cb, 0},
      {NULL, NULL, NULL, NULL, 0}
};

static const Eldbus_Method link_methods[] =
{
      { "list", NULL, ELDBUS_ARGS({"as", "urls"}), _dbus_link_list_cb, 0},
      { "open", ELDBUS_ARGS({"s", "url_to_open"}), NULL, _dbus_link_open_cb, 0},
      {NULL, NULL, NULL, NULL, 0}
};

static const Eldbus_Method list_methods[] =
{
      { "get", NULL, ELDBUS_ARGS({"as", "jid_array"}), _dbus_list_get_cb, 0},
      { "get_all", NULL, ELDBUS_ARGS({"as", "jid_array"}), _dbus_list_get_all_cb, 0},
      {NULL, NULL, NULL, NULL, 0}
};

static const Eldbus_Method contact_methods[] =
{
      { "status", ELDBUS_ARGS({"s", "jid"}),
        ELDBUS_ARGS({"s", "description"}, {"u", "Shotgun_User_Status"}, {"i", "priority"}), _dbus_contact_status_cb, 0},
      { "icon", ELDBUS_ARGS({"s", "jid"}), ELDBUS_ARGS({"s", "eet_icon_key"}), _dbus_contact_icon_cb, 0},
      { "info", ELDBUS_ARGS({"s", "jid"}),
        ELDBUS_ARGS({"s", "display_name"}, {"s", "eet_icon_key"}, {"u", "Shotgun_User_Status"}, {"i", "priority"}), _dbus_contact_info_cb, 0 },
      { "send",
        ELDBUS_ARGS({"s", "jid"}, {"s", "message"}, {"u", "Shotgun_Message_Status"}), ELDBUS_ARGS({"b", "success"}), _dbus_contact_send_cb, 0},
      { "send_echo",
        ELDBUS_ARGS({"s", "jid"}, {"s", "message"}, {"u", "Shotgun_Message_Status"}), ELDBUS_ARGS({"b", "success"}), _dbus_contact_send_echo_cb, 0},
      {NULL, NULL, NULL, NULL, 0}
};

static const Eldbus_Service_Interface_Desc core_desc =
{
   SHOTGUN_DBUS_METHOD_BASE ".core", core_methods, core_signals, NULL, NULL, NULL
};

static const Eldbus_Service_Interface_Desc link_desc =
{
   SHOTGUN_DBUS_METHOD_BASE ".link", link_methods, NULL, NULL, NULL, NULL
};

static const Eldbus_Service_Interface_Desc list_desc =
{
   SHOTGUN_DBUS_METHOD_BASE ".list", list_methods, NULL, NULL, NULL, NULL
};

static const Eldbus_Service_Interface_Desc contact_desc =
{
   SHOTGUN_DBUS_METHOD_BASE ".contact", contact_methods, NULL, NULL, NULL, NULL
};

void
ui_dbus_init(Contact_List *cl)
{
   Eldbus_Service_Interface *iface;

   if (ui_dbus_conn) return;

   eldbus_init();

   ui_dbus_conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);

   ui_core_iface = iface = eldbus_service_interface_register(ui_dbus_conn, SHOTGUN_DBUS_PATH, &core_desc);
   eldbus_service_object_data_set(iface, "contact_list", cl);
   eldbus_name_request(ui_dbus_conn, SHOTGUN_DBUS_INTERFACE, ELDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE,
                      NULL, iface);

   iface = eldbus_service_interface_register(ui_dbus_conn, SHOTGUN_DBUS_PATH, &link_desc);
   eldbus_service_object_data_set(iface, "contact_list", cl);
   iface = eldbus_service_interface_register(ui_dbus_conn, SHOTGUN_DBUS_PATH, &list_desc);
   eldbus_service_object_data_set(iface, "contact_list", cl);
   iface = eldbus_service_interface_register(ui_dbus_conn, SHOTGUN_DBUS_PATH, &contact_desc);
   eldbus_service_object_data_set(iface, "contact_list", cl);
   eldbus_name_owner_changed_callback_add(ui_dbus_conn, NOTIFY_BUS, _notify_nameowner_change_cb, NULL, EINA_FALSE);
   eldbus_name_owner_get(ui_dbus_conn, NOTIFY_BUS, _notify_nameowner_cb, NULL);

   ui_e_object = eldbus_object_get(ui_dbus_conn, "org.enlightenment.wm.service", "/org/enlightenment/wm/RemoteObject");
}

void
ui_dbus_notify(Contact_List *cl, Evas_Object *img, const char *from, const char *message)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *iter, *sub;

   if (cl->settings->disable_notify) return;
   if (!ui_notify_object) return;

   msg = eldbus_message_method_call_new(NOTIFY_BUS, NOTIFY_OBJECT, NOTIFY_INTERFACE, "Notify");

   iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_append(iter, "susssas", "SHOTGUN!", 0, "shotgun", from, message, &sub);
   eldbus_message_iter_container_close(iter, sub);

   eldbus_message_iter_arguments_append(iter, "a{sv}", &sub);
#if 0
   _notify_image_serialize(sub, img);
#endif
   eldbus_message_iter_container_close(iter, sub);
   
   eldbus_message_iter_arguments_append(iter, "i", 5000); //timeout
   eldbus_object_send(ui_notify_object, msg, NULL, NULL, -1);
}

void
ui_dbus_link_detect(Link *i)
{
   Eldbus_Message *msg;

   msg = eldbus_message_method_call_new("org.enlightenment.wm.service", "/org/enlightenment/wm/RemoteObject", "org.enlightenment.wm.Teamwork", "LinkDetect");

   eldbus_message_arguments_append(msg, "su", i->addr, i->timestamp);
   eldbus_object_send(ui_e_object, msg, NULL, NULL, -1);
}

void
ui_dbus_link_mousein(Link *i, int x, int y)
{
   Eldbus_Message *msg;

   msg = eldbus_message_method_call_new("org.enlightenment.wm.service", "/org/enlightenment/wm/RemoteObject", "org.enlightenment.wm.Teamwork", "LinkMouseIn");

   eldbus_message_arguments_append(msg, "suii", i->addr, i->timestamp, x, y);
   eldbus_object_send(ui_e_object, msg, NULL, NULL, -1);
}

void
ui_dbus_link_mouseout(Link *i, int x, int y)
{
   Eldbus_Message *msg;

   msg = eldbus_message_method_call_new("org.enlightenment.wm.service", "/org/enlightenment/wm/RemoteObject", "org.enlightenment.wm.Teamwork", "LinkMouseOut");

   eldbus_message_arguments_append(msg, "suii", i->addr, i->timestamp, x, y);
   eldbus_object_send(ui_e_object, msg, NULL, NULL, -1);
}

void
ui_dbus_shutdown(Contact_List *cl)
{
   if (!cl) return;
   if (ui_dbus_conn) eldbus_connection_unref(ui_dbus_conn);
   ui_dbus_conn = NULL;
   ui_core_iface = NULL;
   ui_e_object = NULL;
   eldbus_shutdown();
}

#endif
