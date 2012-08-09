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
#include <gtk/gtk.h>

#include <stdio.h>
#include <sys/stat.h>

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

#include "replyto.h"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <locale.h>

static SylPluginInfo info = {
  N_(PLUGIN_NAME),
  "0.2.0",
  "HAYASHI Kentaro",
  N_(PLUGIN_DESC)
};

ReplyToOption option;

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

#define REPLYTO_POPUP_MENU _("/Reply to/Reply to who?")
#define REPLYTO_MENU _("/Message/Reply to/Reply to Who?")

void plugin_load(void)
{
  syl_init_gettext(REPLYTO, "lib/locale");
  
  debug_print(gettext("ReplyTo support Plug-in"));
  debug_print(dgettext("ReplyTo", "ReplyTo support Plug-in"));

  syl_plugin_add_menuitem("/Tools", NULL, NULL, NULL);
  syl_plugin_add_menuitem("/Tools", _("ReplyTo Settings [replyto]"), exec_replyto_menu_cb, NULL);
  syl_plugin_add_menuitem("/Message/Reply to", _("Reply to Who?"), exec_replyto_who_cb, NULL);

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

  info.name = g_strdup(_(PLUGIN_NAME));
  info.description = g_strdup(_(PLUGIN_DESC));

}


void plugin_unload(void)
{
  g_free(option.rcpath);
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
  if (option.replyto_flg) {
    /* rewrite To: entry */
    syl_plugin_compose_entry_set(compose, option.to, 0);

    option.replyto_flg = FALSE;
  }
}

static void replyto_who_ok_cb(GtkWidget *widget, gpointer data)
{
  option.to = gtk_combo_box_get_active_text(GTK_COMBO_BOX(option.combo));

  MainWindow *mainwin = syl_plugin_main_window_get();

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
  g_key_file_load_from_file(option.rcfile, option.rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL);

  gboolean flg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(option.startup));
  SET_RC_BOOLEAN(REPLYTO, "startup", flg);
  debug_print("startup:%s\n", flg ? "true" : "false");

  gsize sz;
  gchar *buf=g_key_file_to_data(option.rcfile, &sz, NULL);
  g_file_set_contents(option.rcpath, buf, sz, NULL);
    
  gtk_widget_destroy(GTK_WIDGET(data));
}
static void prefs_cancel_cb(GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(data));
}

static gchar *reply_to_list[] = {
  "To", "From", "Cc", "List-Id", "Reply-To",
};


enum {
  COLUMN_STRING,
  COLUMN_INT,
  N_COLUMNS,
};

static void exec_replyto_who_cb(void)
{
  // show window
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(window), 8);
  gtk_widget_set_size_request(window, 400, 100);
  gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_modal(GTK_WINDOW(window), TRUE);
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, FALSE);
  gtk_widget_realize(window);

  GtkWidget *vbox = gtk_vbox_new(FALSE, 6);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  GtkWidget *confirm_area = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(confirm_area), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(confirm_area), 6);


  GtkWidget *ok_btn = gtk_button_new_from_stock(GTK_STOCK_OK);
  GTK_WIDGET_SET_FLAGS(ok_btn, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(confirm_area), ok_btn, FALSE, FALSE, 0);
  gtk_widget_show(ok_btn);

  GtkWidget *cancel_btn = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  GTK_WIDGET_SET_FLAGS(cancel_btn, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(confirm_area), cancel_btn, FALSE, FALSE, 0);
  gtk_widget_show(cancel_btn);

  g_signal_connect(G_OBJECT(ok_btn), "clicked",
                   G_CALLBACK(replyto_who_ok_cb), window);
  g_signal_connect(G_OBJECT(cancel_btn), "clicked",
                   G_CALLBACK(replyto_who_cancel_cb), window);

  gtk_box_pack_end(GTK_BOX(vbox), confirm_area, FALSE, FALSE, 0);
  gtk_widget_show(confirm_area);

  GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
  gtk_widget_show(hbox);
  GtkWidget *label = gtk_label_new(_("To:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  int i , j;
  option.combo = gtk_combo_box_new_text();
  if (option.msginfo) {
    debug_print("[DEBUG] msginfo:%p\n", option.msginfo);
    gchar *msg_path = procmsg_get_message_file_path(option.msginfo);
    GSList *hlist = procheader_get_header_list_from_file(msg_path);
    if (hlist) {
      debug_print("[DEBUG] hlist:%p\n", hlist);
      for (i = 0; i < g_slist_length(hlist); i++) {
        Header *header = (Header *)g_slist_nth_data(hlist, i);
        if (header && header->name && header->body) {
          for (j = 0; j < 5; j++) {
            if (strcmp(header->name, reply_to_list[j]) == 0) {
              gtk_combo_box_append_text(GTK_COMBO_BOX(option.combo), header->body);
            }
          }
          g_print("%s:%s\n", header->name, header->body);
        }
      }
    }
  }
  gtk_box_pack_start(GTK_BOX(hbox), option.combo, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  
  gtk_widget_show_all(window);
  
}

static void messageview_show_cb(GObject *obj, gpointer msgview,
				MsgInfo *msginfo, gboolean all_headers)
{
  debug_print("[DEBUG] messageview_show_cb\n");
  debug_print("[DEBUG] msginfo:%p\n", msginfo);
  option.msginfo = msginfo;
  debug_print("[DEBUG] msginfo:%p\n", option.msginfo);
}



static void exec_replyto_menu_cb(void)
{
  /* show modal dialog */
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *confirm_area;
  GtkWidget *ok_btn;
  GtkWidget *cancel_btn;
#if DEBUG
  GtkWidget *test_btn;
#endif
    
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(window), 8);
  gtk_widget_set_size_request(window, 400, 300);
  gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_modal(GTK_WINDOW(window), TRUE);
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, FALSE);
  gtk_widget_realize(window);

  vbox = gtk_vbox_new(FALSE, 6);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  /* notebook */ 
  GtkWidget *notebook = gtk_notebook_new();
  /* main tab */
  /* about, copyright tab */
  create_config_about_page(notebook, option.rcfile);

  gtk_widget_show(notebook);
  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

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

#if DEBUG
  test_btn = gtk_button_new_from_stock(GTK_STOCK_NETWORK);
  GTK_WIDGET_SET_FLAGS(cancel_btn, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(confirm_area), test_btn, FALSE, FALSE, 0);
  gtk_widget_show(test_btn);
#endif
    
  gtk_widget_show(confirm_area);
	
  gtk_box_pack_end(GTK_BOX(vbox), confirm_area, FALSE, FALSE, 0);
  gtk_widget_grab_default(ok_btn);

  gtk_window_set_title(GTK_WINDOW(window), _("ReplyTo Settings [ReplyTo]"));

  g_signal_connect(G_OBJECT(ok_btn), "clicked",
                   G_CALLBACK(prefs_ok_cb), window);
  g_signal_connect(G_OBJECT(cancel_btn), "clicked",
                   G_CALLBACK(prefs_cancel_cb), window);
#if DEBUG
  g_signal_connect(G_OBJECT(test_btn), "clicked",
                   G_CALLBACK(prefs_test_cb), window);
#endif
    
  /* load settings */
  if (g_key_file_load_from_file(option.rcfile, option.rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL)) {
    option.startup_flg = GET_RC_BOOLEAN(REPLYTO, "startup");
    debug_print("startup:%s\n", option.startup_flg ? "true" : "false");
    if (option.startup_flg) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(option.startup), TRUE);
    }

  }else{
    /* default settings */
    option.startup_flg = FALSE;
  }
 
  gtk_widget_show(window);
}

static void summaryview_menu_popup_cb(GObject *obj, GtkItemFactory *ifactory,
                                      gpointer data)
{

  g_print("test: %p: summaryview menu popup\n", obj);

#if 0
  GtkWidget *widget;
  int i,j;
  gchar *msg_path = procmsg_get_message_file_path(option.msginfo);
  GSList *hlist = procheader_get_header_list_from_file(msg_path);
  if (hlist) {
    debug_print("[DEBUG] hlist:%p\n", hlist);
    for (i = 0; i < g_slist_length(hlist); i) {
      Header *header = (Header *)g_slist_nth_data(hlist, i);
      if (header && header->name && header->body) {
        for (j = 0; j < 5; j) {
          if (strcasecmp(header->name, reply_to_list[j]) == 0) {
          }
        }
        g_print("%s:%s\n", header->name, header->body);
      }
    }
  }
#endif
}


