#include "ui.h"

static int
_chat_image_sort_cb(Link *a, Link *b)
{
   long long diff;
   diff = a->timestamp - b->timestamp;
   if (diff < 0) return -1;
   if (!diff) return 0;
   return 1;
}

void
chat_conv_image_show(void *data, Evas_Object *obj EINA_UNUSED, Elm_Entry_Anchor_Info *ev)
{
   Link *i;
   Contact *c = data;
   int x, y;

   if (!c) return;

   DBG("anchor in: '%s' (%i, %i)", ev->name, ev->x, ev->y);
#ifdef HAVE_DBUS
   i = eina_hash_find(c->list->images, ev->name);
   if (!i) return;
   elm_win_screen_position_get(c->chat_window->win, &x, &y);
   ui_dbus_link_mousein(i, ev->x + x, ev->y + y);
#endif
}

void
chat_conv_link_hide(Contact *c, Evas_Object *obj EINA_UNUSED, Elm_Entry_Anchor_Info *ev)
{
   int x, y;
   Link *i;

   if (ev) DBG("anchor out: '%s' (%i, %i)", ev->name, ev->x, ev->y);
#ifdef HAVE_DBUS
   i = eina_hash_find(c->list->images, ev->name);
   if (!i) return;
   elm_win_screen_position_get(c->chat_window->win, &x, &y);
   ui_dbus_link_mouseout(i, ev->x + x, ev->y + y);
#endif
   
}

void
chat_link_add(Contact_List *cl, const char *url)
{
   Link *i;
   unsigned long long t;

   t = (unsigned long long)ecore_time_unix_get();

   i = eina_hash_find(cl->images, url);
   if (i)
     {
        i->timestamp = t;
        cl->image_list = eina_inlist_promote(cl->image_list, EINA_INLIST_GET(i));
        return;
     }
   i = calloc(1, sizeof(Link));
   i->timestamp = t;
   i->cl = cl;
   i->addr = url;
   eina_hash_add(cl->images, url, i);
   cl->image_list = eina_inlist_sorted_insert(cl->image_list, EINA_INLIST_GET(i), (Eina_Compare_Cb)_chat_image_sort_cb);
#ifdef HAVE_DBUS
   ui_dbus_link_detect(i);
#endif
}

void
chat_link_free(Link *i)
{
   if (!i) return;
   i->cl->image_list = eina_inlist_remove(i->cl->image_list, EINA_INLIST_GET(i));
#ifdef HAVE_DBUS
   ui_dbus_signal_link(i->cl, i->addr, EINA_TRUE, EINA_FALSE);
#endif
   eina_stringshare_del(i->addr);
   free(i);
}
