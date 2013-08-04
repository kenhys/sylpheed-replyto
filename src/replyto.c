/* -*- coding: utf-8-unix -*-
 *
 * ReplyTo extension plug-in
 *  -- 
 * Copyright (c) 2012, HAYASHI Kentaro <kenhys@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "defs.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n-lib.h>
#include <locale.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <sys/stat.h>

#include "config.h"
#include "copying.h"
#include "sylmain.h"
#include "mainwindow.h"
#include "plugin.h"
#include "procmsg.h"
#include "procmime.h"
#include "procheader.h"
#include "utils.h"
#include "alertpanel.h"
#include "prefs_common.h"
#include "foldersel.h"


#include "sylplugin_factory.h"
#include "replyto.h"

#include "../res/repeat_on.xpm"
#include "../res/repeat_off.xpm"

#define PLUGIN_NAME N_("ReplyTo - reply extention plug-in for Sylpheed")
#define PLUGIN_DESC N_("reply to plug-in for Sylpheed")

static SylPluginInfo info = {
  N_(PLUGIN_NAME),
  VERSION,
  "HAYASHI Kentaro",
  N_(PLUGIN_DESC)
};

static ReplyToOption SYLPF_OPTION;

static void compose_created_cb(GObject *obj, gpointer compose);
static void summaryview_menu_popup_cb(GObject *obj, GtkItemFactory *ifactory,
                                      gpointer data);
static void init_done_cb(GObject *obj, gpointer data);
static void app_exit_cb(GObject *obj, gpointer data);
static void app_force_exit_cb(GObject *obj, gpointer data);

static void prefs_ok_cb(GtkWidget *widget, gpointer data);
static void prefs_cancel_cb(GtkWidget *widget, gpointer data);

static void exec_replyto_who_cb(void);
static void exec_replyto_menu_cb(void);

static void messageview_show_cb(GObject *obj, gpointer msgview,
				MsgInfo *msginfo, gboolean all_headers);
static GSList* get_replyto_header_list(MsgInfo *msginfo,
                                       gchar **reply_to);

static GtkWidget *create_config_main_page(GtkWidget *notebook, GKeyFile *pkey);
static void callback_dummy(GtkWidget *widget, gpointer data);

#define REPLYTO_POPUP_MENU _("/Reply to/_reply to who?")
#define REPLYTO_MENU _("/Message/Reply to/_reply to Who?")

void plugin_load(void)
{
  sylpf_init_gettext(REPLYTO, "lib/locale");
  
  debug_print(gettext("ReplyTo support Plug-in"));
  debug_print(dgettext("ReplyTo", "ReplyTo support Plug-in"));

  syl_plugin_add_menuitem("/Tools", NULL, NULL, NULL);
  syl_plugin_add_menuitem("/Tools", _("ReplyTo settings [replyto]"), exec_replyto_menu_cb, NULL);
  syl_plugin_add_menuitem("/Message/Reply to", _("reply to who?"), exec_replyto_who_cb, NULL);

  g_signal_connect(syl_app_get(), "init-done", G_CALLBACK(init_done_cb), NULL);
  g_signal_connect(syl_app_get(), "app-exit", G_CALLBACK(app_exit_cb), NULL);
  g_signal_connect(syl_app_get(), "app-force-exit", G_CALLBACK(app_force_exit_cb), NULL);

  syl_plugin_signal_connect("messageview-show",
                            G_CALLBACK(messageview_show_cb), NULL);
  syl_plugin_signal_connect("compose-created",
                            G_CALLBACK(compose_created_cb), NULL);
  syl_plugin_signal_connect("summaryview-menu-popup",
                            G_CALLBACK(summaryview_menu_popup_cb), NULL);

  syl_plugin_add_factory_item("<SummaryView>", REPLYTO_POPUP_MENU,
                              exec_replyto_who_cb, NULL);

  setup_plugin_onoff_switch(&SYLPF_OPTION,
                            G_CALLBACK(callback_dummy),
                            (const char **)control_repeat_blue,
                            (const char **)control_repeat);

  info.name = g_strdup(_(PLUGIN_NAME));
  info.description = g_strdup(_(PLUGIN_DESC));

  SYLPF_OPTION.replyto_flg = FALSE;
}


void plugin_unload(void)
{
  g_free(SYLPF_OPTION.rcpath);
}

SylPluginInfo *plugin_info(void)
{
  return &info;
}

gint plugin_interface_version(void)
{
  /* sylpheed 3.2 or later since r3005 */
  return 0x0109;
}

static void init_done_cb(GObject *obj, gpointer data)
{
}

static void app_exit_cb(GObject *obj, gpointer data)
{
}

static void app_force_exit_cb(GObject *obj, gpointer data)
{
}

static void compose_created_cb(GObject *obj, gpointer compose)
{
  if (SYLPF_OPTION.replyto_flg) {
    /* rewrite To: entry */
    syl_plugin_compose_entry_set(compose, SYLPF_OPTION.to, 0);

    SYLPF_OPTION.replyto_flg = FALSE;
  }
}

static void replyto_who_ok_cb(GtkWidget *widget, gpointer data)
{
  MainWindow *mainwin;

  SYLPF_OPTION.to = gtk_combo_box_get_active_text(GTK_COMBO_BOX(SYLPF_OPTION.combo));

  mainwin = syl_plugin_main_window_get();

  SYLPF_OPTION.replyto_flg = TRUE;

  /* emulate reply button clicked! */
  gtk_signal_emit_by_name(GTK_OBJECT(mainwin->reply_btn), "clicked");

  gtk_widget_destroy(GTK_WIDGET(data));
}

static void replyto_who_cancel_cb(GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(data));
}

/**
 *
 */
static void prefs_ok_cb(GtkWidget *widget, gpointer data)
{
  gsize sz;
  gchar *buf;

  g_key_file_load_from_file(SYLPF_OPTION.rcfile, SYLPF_OPTION.rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL);

  buf=g_key_file_to_data(SYLPF_OPTION.rcfile, &sz, NULL);
  g_file_set_contents(SYLPF_OPTION.rcpath, buf, sz, NULL);
    
  gtk_widget_destroy(GTK_WIDGET(data));
}
static void prefs_cancel_cb(GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(data));
}

static gchar *reply_to_list[] = {
  "To", "From", "Cc", "Reply-To"
};


enum {
  COLUMN_STRING,
  COLUMN_INT,
  N_COLUMNS,
};

static void exec_replyto_who_cb(void)
{
  // show window
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *ok_btn;
  GtkWidget *cancel_btn;
  GtkWidget *confirm_area;
  GtkWidget *hbox;
  GtkWidget *label;
  int i;
  gchar *first_entry;
  GSList *header_list;
  Header *header;
  
  first_entry = NULL;
  header_list = NULL;
  if (SYLPF_OPTION.msginfo) {
    SYLPF_DEBUG_PTR("msginfo", SYLPF_OPTION.msginfo);
    header_list = get_replyto_header_list(SYLPF_OPTION.msginfo, &first_entry);
    
    SYLPF_DEBUG_PTR("header_list", header_list);
    SYLPF_DEBUG_PTR("first_entry", first_entry);
    if (!header_list || !first_entry) {
      return;
    }
    if (!g_slist_length(header_list)) {
      return;
    }
  }

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(window), 8);
  gtk_widget_set_size_request(window, 400, 100);
  gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_modal(GTK_WINDOW(window), TRUE);
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, FALSE);
  gtk_widget_realize(window);

  vbox = gtk_vbox_new(FALSE, 6);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  confirm_area = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(confirm_area), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(confirm_area), 6);

  ok_btn = gtk_button_new_from_stock(GTK_STOCK_OK);
  GTK_WIDGET_SET_FLAGS(ok_btn, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(confirm_area), ok_btn, FALSE, FALSE, 0);
  gtk_widget_show(ok_btn);

  cancel_btn = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  GTK_WIDGET_SET_FLAGS(cancel_btn, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(confirm_area), cancel_btn, FALSE, FALSE, 0);
  gtk_widget_show(cancel_btn);

  g_signal_connect(G_OBJECT(ok_btn), "clicked",
                   G_CALLBACK(replyto_who_ok_cb), window);
  g_signal_connect(G_OBJECT(cancel_btn), "clicked",
                   G_CALLBACK(replyto_who_cancel_cb), window);

  gtk_box_pack_end(GTK_BOX(vbox), confirm_area, FALSE, FALSE, 0);
  gtk_widget_show(confirm_area);

  hbox = gtk_hbox_new(FALSE, 6);
  gtk_widget_show(hbox);
  label = gtk_label_new(_("To:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(2, 24, 0)
  SYLPF_OPTION.combo = gtk_combo_box_text_new();

  for (i = 0; i < g_slist_length(header_list); i++) {
    header = (Header *)g_slist_nth_data(header_list, i);
    if (header && header->name && header->body) {
      if (strcasecmp(header->body, first_entry) == 0) {
        gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(SYLPF_OPTION.combo),
                                        header->body);
      } else {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(SYLPF_OPTION.combo),
                                       header->body);
      }
    }
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(SYLPF_OPTION.combo), 0);
#else
  SYLPF_OPTION.combo = gtk_combo_box_new();
#endif

  gtk_box_pack_start(GTK_BOX(hbox), SYLPF_OPTION.combo, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  
  gtk_widget_show_all(window);
  
}

static GSList* get_replyto_header_list(MsgInfo *msginfo,
                                       gchar **reply_to)
{
  gchar *msg_path;
  GSList *header_list;
  GSList *reply_list;
  Header *header;
  int i, j, last_index;
  
#define SYLPF_FUNC_NAME "get_replyto_header_list"
  SYLPF_START_FUNC;

  *reply_to = NULL;
  msg_path = procmsg_get_message_file_path(msginfo);
  if (!msg_path) {
    SYLPF_WARN_MSG("missing MsgInfo file path");
    return NULL;
  }

  header_list = procheader_get_header_list_from_file(msg_path);
  g_free(msg_path);
  if (!header_list) {
    SYLPF_WARN_MSG("missing header list from message file");
    return NULL;
  }
  
  SYLPF_DEBUG_STR("msg_path", msg_path);
  SYLPF_DEBUG_PTR("header_list", header_list);

  reply_list = NULL;
  last_index = (sizeof(reply_to_list)/sizeof(gchar*))-1;

  for (i = 0; i < g_slist_length(header_list); i++) {
    header = (Header *)g_slist_nth_data(header_list, i);
    if (header && header->name && header->body) {
      SYLPF_DEBUG_STR("header->name", header->name);
      for (j = last_index; j >= 0; j--) {
        if (strcasecmp(header->name, reply_to_list[j]) == 0) {
          SYLPF_DEBUG_STR("append header body as reply_list", header->body);
          reply_list = g_slist_append(reply_list,  header);
          *reply_to = header->body;
        }
      }
      debug_print("%s:%s\n", header->name, header->body);
    }
  }
  SYLPF_END_FUNC;
#undef SYLPF_FUNC_NAME
  return reply_list;
}

static void messageview_show_cb(GObject *obj, gpointer msgview,
				MsgInfo *msginfo, gboolean all_headers)
{
  debug_print("[DEBUG] messageview_show_cb\n");
  debug_print("[DEBUG] msginfo:%p\n", msginfo);
  SYLPF_OPTION.msginfo = msginfo;
  debug_print("[DEBUG] msginfo:%p\n", SYLPF_OPTION.msginfo);
}



static void exec_replyto_menu_cb(void)
{
  GtkWidget *dialog;
  gint response;

  SYLPF_START_FUNC;

  dialog = create_preference_dialog(&SYLPF_OPTION);

  gtk_widget_show_all(dialog);
  response = gtk_dialog_run(GTK_DIALOG(dialog));

  switch (response) {
  case GTK_RESPONSE_OK:
    save_preference_dialog(&option);
    break;
  case GTK_RESPONSE_CANCEL:
  default:
    break;
  }

  gtk_widget_destroy(dialog);

  SYLPF_END_FUNC;
}


static GtkWidget *create_preference_dialog (ReplyToOption *option)
{
  GtkWidget *dialog;
  GtkWidget *vbox, *hbox;
  GtkWidget *notebook;
  gpointer mainwin;
  GtkWidget *window;

  mainwin = syl_plugin_main_window_get();
  window = ((MainWindow*)mainwin)->window;

  dialog = gtk_dialog_new_with_buttons(_("ReplyTo"),
                                       GTK_WINDOW(window),
                                       GTK_DIALOG_MODAL,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_OK, GTK_RESPONSE_OK,
                                       NULL);

  sylpf_init_preference_dialog_size(dialog);

  vbox = gtk_vbox_new(FALSE, SYLPF_BOX_SPACE);
  hbox = gtk_hbox_new(TRUE, SYLPF_BOX_SPACE);

  gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), SYLPF_BOX_SPACE);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);

  gtk_container_add(GTK_CONTAINER(hbox), vbox);

  /* notebook */ 
  notebook = gtk_notebook_new();
  /* main tab */
  create_config_main_page(notebook, SYLPF_OPTION.rcfile);
  /* about, copyright tab */
  sylpf_append_config_about_page(notebook,
                                 option->rcfile,
                                 _("About"),
                                 _(PLUGIN_NAME),
                                 _(PLUGIN_DESC),
                                 _(copyright));

  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

  SYLPF_RETURN_VALUE(dialog);
}

static void summaryview_menu_popup_cb(GObject *obj, GtkItemFactory *ifactory,
                                      gpointer data)
{
}

GtkWidget *create_config_main_page(GtkWidget *notebook, GKeyFile *pkey)
{
  GtkWidget *vbox;
  GtkWidget *startup_align;
  GtkWidget *label;
  
  debug_print("create_config_main_page\n");
  if (notebook == NULL) {
    return NULL;
  }
  /* startup */
  if (pkey!=NULL) {
  }
  vbox = gtk_vbox_new(FALSE, 0);

  /**/
  startup_align = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(startup_align), ALIGN_TOP, ALIGN_BOTTOM, ALIGN_LEFT, ALIGN_RIGHT);
#if 0

  startup_frm = gtk_frame_new(_("Startup Option"));
  startup_frm_align = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(startup_frm_align), ALIGN_TOP, ALIGN_BOTTOM, ALIGN_LEFT, ALIGN_RIGHT);


  option.startup = gtk_check_button_new_with_label(_("Enable plugin on startup."));
  gtk_container_add(GTK_CONTAINER(startup_frm_align), option.startup);
  gtk_container_add(GTK_CONTAINER(startup_frm), startup_frm_align);
  gtk_container_add(GTK_CONTAINER(startup_align), startup_frm);

  gtk_widget_show(option.startup);

#endif
  /**/
  gtk_box_pack_start(GTK_BOX(vbox), startup_align, FALSE, FALSE, 0);

  label = gtk_label_new(_("General"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
  gtk_widget_show_all(notebook);
  return NULL;
}

static void callback_dummy(GtkWidget *widget, gpointer data)
{
  update_plugin_onoff_status(NULL,
                             FALSE,
                             NULL,
                             NULL,
                             NULL);
}


static void setup_plugin_onoff_switch(ReplyToOption *option,
                                      GCallback callback_func,
                                      const char **on_xpm,
                                      const char **off_xpm)
{
  GtkWidget *statusbar = syl_plugin_main_window_get_statusbar();
  GtkWidget *plugin_box = gtk_hbox_new(FALSE, 0);

  GdkPixbuf* on_pixbuf = gdk_pixbuf_new_from_xpm_data(on_xpm);
    
  GdkPixbuf* off_pixbuf = gdk_pixbuf_new_from_xpm_data(off_xpm);

  option->plugin_on = gtk_image_new_from_pixbuf(on_pixbuf);

  option->plugin_off = gtk_image_new_from_pixbuf(off_pixbuf);

  gtk_box_pack_start(GTK_BOX(plugin_box), option->plugin_on, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(plugin_box), option->plugin_off, FALSE, FALSE, 0);
    
  option->plugin_tooltip = gtk_tooltips_new();
    
  option->plugin_switch = gtk_button_new();
  gtk_button_set_relief(GTK_BUTTON(option->plugin_switch), GTK_RELIEF_NONE);
  GTK_WIDGET_UNSET_FLAGS(option->plugin_switch, GTK_CAN_FOCUS);
  gtk_widget_set_size_request(option->plugin_switch, 20, 20);

  gtk_container_add(GTK_CONTAINER(option->plugin_switch), plugin_box);
  g_signal_connect(G_OBJECT(option->plugin_switch), "clicked",
                   G_CALLBACK(callback_func), option);
  gtk_box_pack_start(GTK_BOX(statusbar), option->plugin_switch, FALSE, FALSE, 0);

  gtk_widget_show_all(option->plugin_switch);
  gtk_widget_hide(option->plugin_off);
}

static void update_plugin_onoff_status(ReplyToOption *option,
                                       gboolean enabled,
                                       const char *title,
                                       const char *message,
                                       const char *tooltip)
{
  SYLPF_START_FUNC;

  option->plugin_enabled = enabled;

  if (title && message) {
    syl_plugin_alertpanel_message(title, message, ALERT_NOTICE);
  }

  if (enabled != FALSE) {
    gtk_widget_hide(option->plugin_off);
    gtk_widget_show(option->plugin_on);
    gtk_tooltips_set_tip(option->plugin_tooltip,
                         option->plugin_switch,
                         tooltip,
                         NULL);
  } else {
    gtk_widget_hide(option->plugin_on);
    gtk_widget_show(option->plugin_off);
    gtk_tooltips_set_tip(option->plugin_tooltip,
                         option->plugin_switch,
                         tooltip,
                         NULL);
  }
  SYLPF_END_FUNC;
}
