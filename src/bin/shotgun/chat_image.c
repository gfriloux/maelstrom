#include "ui.h"


static Evas_Object *
_chat_conv_image_provider(Image *i, Evas_Object *obj __UNUSED__, Evas_Object *tt)
{
   Evas_Object *ret, *ic, *lbl;
   int w, h, cw, ch;
   char *buf;
   size_t len;

   DBG("(i=%p)", i);
   if ((!i) || (!i->buf)) goto error;

   w = h = cw = ch = 0;
   ic = elm_icon_add(tt);
   if (!elm_image_memfile_set(ic, eina_binbuf_string_get(i->buf), eina_binbuf_length_get(i->buf), NULL, NULL))
     {
        /* an unloadable image is a useless image! */
        i->cl->image_list = eina_inlist_remove(i->cl->image_list, EINA_INLIST_GET(i));
        eina_hash_del_by_key(i->cl->images, i->addr);
        evas_object_del(ic);
        goto error;
     }
   evas_object_show(ic);
   ret = elm_box_add(tt);
   elm_box_homogeneous_set(ret, EINA_FALSE);
   EXPAND(ret);
   FILL(ret);

   lbl = elm_label_add(tt);
   elm_object_text_set(lbl, i->addr);
   elm_box_pack_end(ret, lbl);
   evas_object_show(lbl);

   elm_box_pack_end(ret, ic);

   elm_win_screen_size_get(tt, NULL, NULL, &cw, &ch);
   elm_win_screen_constrain_set(tt, EINA_TRUE);
   elm_image_object_size_get(ic, &w, &h);
   elm_image_resizable_set(ic, 0, 0);
   elm_image_smooth_set(ic, 1);
   if (elm_image_animated_available_get(ic))
     {
        elm_image_animated_set(ic, EINA_TRUE);
        elm_image_animated_play_set(ic, EINA_TRUE);
     }
   {
      float sc = 0;
      if ((float)w / (float)cw >= 0.6)
        sc = ((float)cw * 0.6) / (float)w;
      else if ((float)h / (float)ch >= 0.6)
        sc = ((float)ch * 0.6) / (float)h;
      if (sc) elm_object_scale_set(ic, sc);
   }
   lbl = elm_label_add(tt);
   len = (i->cl->settings->browser ? strlen(i->cl->settings->browser) : 0) + 64;
   if (len > 32000)
     buf = malloc(len);
   else
     buf = alloca(len);
   snprintf(buf, len, "%s%s%s"
            "Right click link to copy to clipboard",
            i->cl->settings->browser ? "Left click link to open with \"" : "",
            i->cl->settings->browser ?: "",
            i->cl->settings->browser ? "\"<ps>" : "");

   elm_object_text_set(lbl, buf);
   if (len > 32000) free(buf);
   elm_box_pack_end(ret, lbl);
   evas_object_show(lbl);
   return ret;
error:
   ret = elm_bg_add(tt);
   elm_bg_color_set(ret, 0, 0, 0);
   evas_object_show(ret);
   ret = elm_label_add(tt);
   {
      len = strlen(i->addr) + (i->cl->settings->browser ? strlen(i->cl->settings->browser) : 0) + 64;
      if (len > 32000)
        buf = malloc(len);
      else
        buf = alloca(len);
      snprintf(buf, len, "%s<ps>"
               "%s%s%s"
               "Right click link to copy to clipboard",
               i->addr,
               i->cl->settings->browser ? "Left click link to open with \"" : "",
               i->cl->settings->browser ?: "",
               i->cl->settings->browser ? "\"<ps>" : "");
      elm_object_text_set(ret, buf);
      if (len > 32000) free(buf);
   }
   return ret;
}

static int
_chat_image_sort_cb(Image *a, Image *b)
{
   long long diff;
   diff = a->timestamp - b->timestamp;
   if (diff < 0) return -1;
   if (!diff) return 0;
   return 1;
}

Eina_Error
_chat_image_complete(Azy_Client *cli, Azy_Content *content, Eina_Binbuf *buf)
{
   Image *i = azy_client_data_get(cli);
   int status;
   const char *h;
   Azy_Net *net = azy_content_net_get(content);

   status = azy_net_code_get(net);
   DBG("%i code for image: %s", status, azy_net_uri_get(net));
   if (i->buf) eina_binbuf_free(i->buf);
   i->buf = buf;
   if (status != 200)
     {
        if (i->buf) eina_binbuf_free(i->buf);
        i->buf = NULL;
        if (++i->tries < IMAGE_FETCH_TRIES)
          {
             Azy_Client_Call_Id id;

             id = azy_client_blank(i->client, AZY_NET_TYPE_GET, NULL, NULL, NULL);
             if (id)
               azy_client_callback_set(i->client, id, (Azy_Client_Transfer_Complete_Cb)_chat_image_complete);
             else
               {
                  ERR("fetch retry failed: img(%s)!", i->addr);
                  ui_eet_dummy_add(i->addr);
                  i->dummy = EINA_TRUE;
                  azy_client_free(i->client);
                  i->client = NULL;
               }
          }
        return ECORE_CALLBACK_RENEW;
     }
   h = azy_net_header_get(net, "content-type");
   if (h)
     {
        if (strncasecmp(h, "image/", 6))
          {
             ui_eet_dummy_add(i->addr);
             i->dummy = EINA_TRUE;
             if (i->buf) eina_binbuf_free(i->buf);
             i->buf = NULL;
             if (i->client) azy_client_free(i->client);
             i->client = NULL;
          }
     }
   if (status != 200)
     {
        i->cl->image_list = eina_inlist_remove(i->cl->image_list, EINA_INLIST_GET(i));
        eina_hash_del_by_key(i->cl->images, i->addr);
        return ECORE_CALLBACK_RENEW;
     }
   i->timestamp = (unsigned long long)ecore_time_unix_get();
   if (!i->dummy)
     {
        if (ui_eet_image_add(i->addr, i->buf, i->timestamp) == 1)
          i->cl->image_size += eina_binbuf_length_get(i->buf);
        if (i->client) azy_client_free(i->client);
        i->client = NULL;
        chat_image_cleanup(i->cl);
     }
   if (i->cl->dbus_image == i)
     {
        Elm_Entry_Anchor_Info e;

        memset(&e, 0, sizeof(Elm_Entry_Anchor_Info));
        e.name = i->addr;
        chat_conv_image_show(i->cl, NULL, &e);
     }
   return ECORE_CALLBACK_RENEW;
}

void
chat_conv_image_show(void *data, Evas_Object *obj, Elm_Entry_Anchor_Info *ev)
{
   Image *i = NULL;
   Contact *c = data;
   Contact_List *cl = data;
   const char *url;

   if (obj)
     {
        if (!c) return;
        cl = c->list;
     }
   else
     obj = cl->win;

   if (cl->dbus_image)
     {
        url = ev->name;
        ev->name = cl->dbus_image->addr;
        chat_conv_image_hide(NULL, cl->win, ev);
        ev->name = url;
     }
   DBG("anchor in: '%s' (%i, %i)", ev->name, ev->x, ev->y);
   i = eina_hash_find(cl->images, ev->name);
   if (i && i->buf)
     elm_object_tooltip_content_cb_set(obj, (Elm_Tooltip_Content_Cb)_chat_conv_image_provider, i, NULL);
   else
     {
        char *buf;
        size_t len;

        len = strlen(ev->name) + (cl->settings->browser ? strlen(cl->settings->browser) : 0) + 64;
        if (len > 32000)
          buf = malloc(len);
        else
          buf = alloca(len);
        snprintf(buf, len, "%s<ps>"
                 "%s%s%s"
                 "Right click link to copy to clipboard",
                 ev->name,
                 cl->settings->browser ? "Left click link to open with \"" : "",
                 cl->settings->browser ?: "",
                 cl->settings->browser ? "\"<ps>" : "");
        elm_object_tooltip_text_set(obj, buf);
        if (len > 32000) free(buf);
     }
   elm_object_tooltip_window_mode_set(obj, EINA_TRUE);
   elm_object_tooltip_show(obj);
}

void
chat_conv_image_hide(Contact *c EINA_UNUSED, Evas_Object *obj, Elm_Entry_Anchor_Info *ev)
{
   if (ev) DBG("anchor out: '%s' (%i, %i)", ev->name, ev->x, ev->y);
   elm_object_tooltip_unset(obj);
}

void
chat_image_add(Contact_List *cl, const char *url)
{
   Image *i;
   unsigned long long t;

   t = (unsigned long long)ecore_time_unix_get();

   i = eina_hash_find(cl->images, url);
   if (i)
     {
        if (i->buf)
          {
             i->timestamp = t;
             ui_eet_image_ping(url, i->timestamp);
          }
        else
          {
             /* randomly deleted during cache pruning */
             i->buf = ui_eet_image_get(url, t);
             cl->image_size += eina_binbuf_length_get(i->buf);
          }
        cl->image_list = eina_inlist_promote(cl->image_list, EINA_INLIST_GET(i));
        chat_image_cleanup(i->cl);
        return;
     }
   if (ui_eet_dummy_check(url)) return;
   if (cl->settings->disable_image_fetch) return;
   i = calloc(1, sizeof(Image));
   i->buf = ui_eet_image_get(url, t);
   if (i->buf)
     cl->image_size += eina_binbuf_length_get(i->buf);
   else
     {
        i->client = azy_client_util_connect(url);
        if (i->client)
          {
             Azy_Client_Call_Id id;

             azy_client_data_set(i->client, i);
             id = azy_client_blank(i->client, AZY_NET_TYPE_GET, NULL, NULL, NULL);
             if (id)
               azy_client_callback_set(i->client, id, (Azy_Client_Transfer_Complete_Cb)_chat_image_complete);
             else
               {
                  azy_client_free(i->client);
                  i->client = NULL;
               }
          }
        if (!i->client)
          {
             /* don't even know how to deal with this */
             ERR("IMAGE FETCHING FAILURE: %s", url);
             free(i);
             return;
          }
     }
   i->cl = cl;
   i->addr = url;
   eina_hash_add(cl->images, url, i);
   cl->image_list = eina_inlist_sorted_insert(cl->image_list, EINA_INLIST_GET(i), (Eina_Compare_Cb)_chat_image_sort_cb);
   chat_image_cleanup(i->cl);
}

void
chat_image_free(Image *i)
{
   if (!i) return;
   i->cl->image_list = eina_inlist_remove(i->cl->image_list, EINA_INLIST_GET(i));
   if (i->cl->dbus_image == i)
     {
        chat_conv_image_hide(NULL, i->cl->win, NULL);
        i->cl->dbus_image = NULL;
     }
   if (i->client) azy_client_free(i->client);
   if (i->buf) eina_binbuf_free(i->buf);
   ui_dbus_signal_link(i->cl, i->addr, EINA_TRUE, EINA_FALSE);
   eina_stringshare_del(i->addr);
   free(i);
}

void
chat_image_cleanup(Contact_List *cl)
{
   Image *i;

   if (!cl->settings->allowed_image_size) return;
   if (cl->settings->allowed_image_size < (cl->image_size / 1024 / 1024)) return;
   EINA_INLIST_FOREACH(cl->image_list, i)
     {
        if (!i->buf) continue;
        /* only free the buffers here to avoid having to deal with multiple list entries */
        cl->image_size -= eina_binbuf_length_get(i->buf);
        eina_binbuf_free(i->buf);
        i->buf = NULL;
        if (cl->settings->allowed_image_size < (cl->image_size / 1024 / 1024))
          return;
     }
}
