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

#include "../res/repeat_on.xpm"
#include "../res/repeat_off.xpm"
#include "replyto.h"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <locale.h>

static SylPluginInfo info = {
  N_(PLUGIN_NAME),
  "0.1.0",
  "HAYASHI Kentaro",
  N_(PLUGIN_DESC)
};

static gchar* g_copyright = N_("ReplyTo is distributed under 2-Clause BSD license.\n"
"\n"
"Copyright (C) 2012 HAYASHI Kentaro <kenhys@gmail.com>"
"\n"
"replyto contains following resource.\n"
                               "\n");

static gboolean g_enable = FALSE;


static GtkWidget *g_plugin_on = NULL;
static GtkWidget *g_plugin_off = NULL;
static GtkWidget *g_onoff_switch = NULL;
static GtkTooltips *g_tooltip = NULL;

static ReplyToOption g_opt;

void plugin_load(void)
{
  syl_init_gettext(REPLYTO, "lib/locale");
  
  debug_print(gettext("ReplyToNotify notify support Plug-in"));
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

  GtkWidget *mainwin = syl_plugin_main_window_get();
  GtkWidget *statusbar = syl_plugin_main_window_get_statusbar();
  GtkWidget *plugin_box = gtk_hbox_new(FALSE, 0);

  GdkPixbuf* on_pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)control_repeat_blue);
  g_plugin_on=gtk_image_new_from_pixbuf(on_pixbuf);
    
  GdkPixbuf* off_pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)control_repeat);
  g_plugin_off=gtk_image_new_from_pixbuf(off_pixbuf);

  gtk_box_pack_start(GTK_BOX(plugin_box), g_plugin_on, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(plugin_box), g_plugin_off, FALSE, FALSE, 0);
    
  g_tooltip = gtk_tooltips_new();
    
  g_onoff_switch = gtk_button_new();
  gtk_button_set_relief(GTK_BUTTON(g_onoff_switch), GTK_RELIEF_NONE);
  GTK_WIDGET_UNSET_FLAGS(g_onoff_switch, GTK_CAN_FOCUS);
  gtk_widget_set_size_request(g_onoff_switch, 20, 20);

  gtk_container_add(GTK_CONTAINER(g_onoff_switch), plugin_box);
  g_signal_connect(G_OBJECT(g_onoff_switch), "clicked",
                   G_CALLBACK(exec_replyto_onoff_cb), mainwin);
  gtk_box_pack_start(GTK_BOX(statusbar), g_onoff_switch, FALSE, FALSE, 0);

  gtk_widget_show_all(g_onoff_switch);
  gtk_widget_hide(g_plugin_on);

  info.name = g_strdup(_(PLUGIN_NAME));
  info.description = g_strdup(_(PLUGIN_DESC));

  g_opt.rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, REPLYTORC, NULL);
  g_opt.rcfile = g_key_file_new();

  if (g_key_file_load_from_file(g_opt.rcfile, g_opt.rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL)){
    g_opt.startup_flg = GET_RC_BOOLEAN(REPLYTO, "startup");
    debug_print("startup:%s", g_opt.startup_flg ? "true" : "false");

    if (g_opt.startup_flg != FALSE){
      g_enable=TRUE;
      gtk_widget_hide(g_plugin_off);
      gtk_widget_show(g_plugin_on);
      gtk_tooltips_set_tip(g_tooltip, g_onoff_switch,
                           _("ReplyTo is enabled. Click the icon to disable plugin."),
                           NULL);
    } else {
      g_enable=FALSE;
      gtk_widget_hide(g_plugin_on);
      gtk_widget_show(g_plugin_off);
      gtk_tooltips_set_tip(g_tooltip, g_onoff_switch,
                           _("ReplyTo is disalbed. Click the icon to enable plugin."),
                           NULL);
    }

  } else {
      /**/
      g_opt.startup_flg = FALSE;

  }
}

void plugin_unload(void)
{
  g_free(g_opt.rcpath);
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
  debug_print("[DEBUG] init_done_cb");
}

static void app_exit_cb(GObject *obj, gpointer data)
{
}

static void app_force_exit_cb(GObject *obj, gpointer data)
{
}

static void compose_created_cb(GObject *obj, gpointer compose)
{
  gchar *text;

  g_print("test: %p: compose created (%p)\n", obj, compose);

  /* rewrite To: entry */
  syl_plugin_compose_entry_set(compose, g_opt.to, 0);
}

static void replyto_who_ok_cb(GtkWidget *widget, gpointer data)
{
  g_opt.to = gtk_combo_box_get_active_text(g_opt.combo);

  MainWindow *mainwin = syl_plugin_main_window_get();
  
  /* emulate reply button clicked! */
  gtk_signal_emit_by_name(mainwin->reply_btn, "clicked");
  
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
    g_key_file_load_from_file(g_opt.rcfile, g_opt.rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL);

    gboolean flg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_opt.startup));
    SET_RC_BOOLEAN(REPLYTO, "startup", flg);
    debug_print("startup:%s\n", flg ? "true" : "false");

    gsize sz;
    gchar *buf=g_key_file_to_data(g_opt.rcfile, &sz, NULL);
    g_file_set_contents(g_opt.rcpath, buf, sz, NULL);
    
    gtk_widget_destroy(GTK_WIDGET(data));
}
static void prefs_cancel_cb(GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
}
static void prefs_test_cb(GtkWidget *widget, gpointer data)
{
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
  gtk_widget_set_size_request(window, 400, 300);
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

  int i , j;
  g_opt.combo = gtk_combo_box_new_text();
  if (g_opt.msginfo) {
    debug_print("[DEBUG] msginfo:%p\n", g_opt.msginfo);
    gchar *msg_path = procmsg_get_message_file_path(g_opt.msginfo);
    GSList *hlist = procheader_get_header_list_from_file(msg_path);
    if (hlist) {
      debug_print("[DEBUG] hlist:%p\n", hlist);
      for (i = 0; i < g_slist_length(hlist); i++) {
        Header *header = (Header *)g_slist_nth_data(hlist, i);
        if (header && header->name && header->body) {
          for (j = 0; j < 5; j++) {
            if (strcmp(header->name, reply_to_list[j]) == 0) {
              gtk_combo_box_append_text(g_opt.combo, header->body);
            }
          }
          g_print("%s:%s\n", header->name, header->body);
        }
      }
    }
  }
  gtk_box_pack_start(GTK_BOX(vbox), g_opt.combo, FALSE, FALSE, 0);
  
  gtk_widget_show_all(window);
  
}

static void messageview_show_cb(GObject *obj, gpointer msgview,
				MsgInfo *msginfo, gboolean all_headers)
{
  debug_print("[DEBUG] messageview_show_cb\n");
  debug_print("[DEBUG] msginfo:%p\n", msginfo);
  g_opt.msginfo = msginfo;
  debug_print("[DEBUG] msginfo:%p\n", g_opt.msginfo);
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
    create_config_main_page(notebook, g_opt.rcfile);
    /* about, copyright tab */
    create_config_about_page(notebook, g_opt.rcfile);

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
    if (g_key_file_load_from_file(g_opt.rcfile, g_opt.rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL)){
      g_opt.startup_flg = GET_RC_BOOLEAN(REPLYTO, "startup");
      debug_print("startup:%s\n", g_opt.startup_flg ? "true" : "false");
      if (g_opt.startup_flg){
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_opt.startup), TRUE);
      }

    }else{
      /* default settings */
      g_opt.startup_flg = FALSE;
    }
 
    gtk_widget_show(window);
}

static GtkWidget *create_config_main_page(GtkWidget *notebook, GKeyFile *pkey)
{
  debug_print("create_config_main_page\n");
  if (notebook == NULL){
    return NULL;
  }
  /* startup */
  if (pkey!=NULL){
  }
  GtkWidget *vbox = gtk_vbox_new(FALSE, 0);

  /**/
  GtkWidget *startup_align = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(startup_align), ALIGN_TOP, ALIGN_BOTTOM, ALIGN_LEFT, ALIGN_RIGHT);

  GtkWidget *startup_frm = gtk_frame_new(_("Startup Option"));
  GtkWidget *startup_frm_align = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(startup_frm_align), ALIGN_TOP, ALIGN_BOTTOM, ALIGN_LEFT, ALIGN_RIGHT);


  g_opt.startup = gtk_check_button_new_with_label(_("Enable plugin on startup."));
  gtk_container_add(GTK_CONTAINER(startup_frm_align), g_opt.startup);
  gtk_container_add(GTK_CONTAINER(startup_frm), startup_frm_align);
  gtk_container_add(GTK_CONTAINER(startup_align), startup_frm);

  gtk_widget_show(g_opt.startup);


  /**/
  gtk_box_pack_start(GTK_BOX(vbox), startup_align, FALSE, FALSE, 0);

  GtkWidget *general_lbl = gtk_label_new(_("General"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, general_lbl);
  gtk_widget_show_all(notebook);
  return NULL;

}

/* about, copyright tab */
static GtkWidget *create_config_about_page(GtkWidget *notebook, GKeyFile *pkey)
{
  debug_print("create_config_about_page\n");
  if (notebook == NULL){
    return NULL;
  }
  GtkWidget *hbox = gtk_hbox_new(TRUE, 6);
  GtkWidget *vbox = gtk_vbox_new(FALSE, 6);

  GtkWidget *lbl = gtk_label_new(_("ReplyTo"));
  GtkWidget *desc = gtk_label_new(PLUGIN_DESC);

  /* copyright */
  GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);

  gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, TRUE, 6);
  gtk_box_pack_start(GTK_BOX(vbox), desc, FALSE, TRUE, 6);
  gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 6);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 6);

  GtkTextBuffer *tbuffer = gtk_text_buffer_new(NULL);
  gtk_text_buffer_set_text(tbuffer, _(g_copyright), strlen(g_copyright));
  GtkWidget *tview = gtk_text_view_new_with_buffer(tbuffer);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(tview), FALSE);
  gtk_container_add(GTK_CONTAINER(scrolled), tview);
    
  gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 6);
    
  /**/
  GtkWidget *general_lbl = gtk_label_new(_("About"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), hbox, general_lbl);
  gtk_widget_show_all(notebook);
  return NULL;
}

static void exec_replyto_onoff_cb(void)
{

    if (g_enable != TRUE){
        syl_plugin_alertpanel_message(_("ReplyTo"), _("ReplyTO plugin is enabled."), ALERT_NOTICE);
        g_enable=TRUE;
        gtk_widget_hide(g_plugin_off);
        gtk_widget_show(g_plugin_on);
        gtk_tooltips_set_tip
			(g_tooltip, g_onoff_switch,
			 _("ReplyTo is enabled. Click the icon to disable plugin."),
			 NULL);
    }else{
        syl_plugin_alertpanel_message(_("ReplyTo"), _("ReplyTo plugin is disabled."), ALERT_NOTICE);
        g_enable=FALSE;
        gtk_widget_hide(g_plugin_on);
        gtk_widget_show(g_plugin_off);
        gtk_tooltips_set_tip
			(g_tooltip, g_onoff_switch,
			 _("ReplyTo is disabled. Click the icon to enable plugin."),
			 NULL);
    }
}

void exec_replyto_cb(GObject *obj, FolderItem *item, const gchar *file, guint num)
{
    if (g_enable!=TRUE){
        debug_print("[DEBUG] disabled replyto plugin\n");
        return;
    }
    if (item->stype != F_NORMAL && item->stype != F_INBOX){
      debug_print("[DEBUG] not F_NORMAL and F_INBOX %d\n", item->stype);
      if (item->folder) {
        if (item->folder->klass) {
          debug_print("[DEBUG] item->name:%s FolderType:%d %d\n", item->name, item->folder->klass->type);
        }
      }
      return;
    }

    PrefsCommon *prefs_common = prefs_common_get();
    if (prefs_common->online_mode != TRUE){
        debug_print("[DEBUG] not online\n");
        return;
    }

    PrefsAccount *ac = (PrefsAccount*)account_get_default();
    g_return_if_fail(ac != NULL);

    /* check item->path for filter */
    g_print("%s\n", item->name);
    g_print("%s\n", item->path);

    MsgInfo *msginfo = folder_item_get_msginfo(item, num);
    debug_print("[DEBUG] flags:%08x UNREAD:%08x NEW:%08x MARKED:%08x ", msginfo->flags, MSG_UNREAD, MSG_NEW, MSG_MARKED);
    debug_print("[DEBUG] perm_flags:%08x \n", msginfo->flags.perm_flags);
    debug_print("[DEBUG] tmp_flags:%08x \n", msginfo->flags.tmp_flags);

    g_key_file_load_from_file(g_opt.rcfile, g_opt.rcpath, G_KEY_FILE_KEEP_COMMENTS, NULL);

#ifdef DEBUG
    debug_print("[DEBUG] item->path:%s\n", item->path);
#endif

}

static void command_path_clicked(GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog = gtk_file_chooser_dialog_new(NULL, NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
                                                  GTK_STOCK_OPEN,GTK_RESPONSE_ACCEPT,
                                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                  NULL);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT){
    gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(dialog));

    gtk_entry_set_text(GTK_ENTRY(data), filename);
    g_free (filename);
  }
  gtk_widget_destroy (dialog);
}

static void inc_start_cb(GObject *obj, PrefsAccount *ac)
{
}

static void inc_finished_cb(GObject *obj, gint new_messages)
{
  g_print("test: received %d new messages\n", new_messages);
}
