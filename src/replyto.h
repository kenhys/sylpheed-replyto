/*
 * ReplyTo -- reply to plug-in for Sylpheed
 *
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

#ifndef __REPLYTO_H__
#define __REPLYTO_H__

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <locale.h>

#define REPLYTO "replyto"
#define REPLYTORC "replytorc"

#define _(String) dgettext("replyto", String)
#define N_(String) gettext_noop(String)
#define gettext_noop(String) (String)

#define PLUGIN_NAME N_("ReplyTo - reply extention plug-in for Sylpheed")
#define PLUGIN_DESC N_("reply to plug-in for Sylpheed")

struct _ReplyToOption {
  /* full path to replytorc */
  gchar *rcpath;
  /* rcfile */
  GKeyFile *rcfile;

  gboolean startup_flg;
  gchar *to;

  GtkWidget *startup;

  GtkWidget *combo;
  
  MsgInfo *msginfo;
};

typedef struct _ReplyToOption ReplyToOption;

static void init_done_cb(GObject *obj, gpointer data);
static void app_exit_cb(GObject *obj, gpointer data);
static void app_force_exit_cb(GObject *obj, gpointer data);

static void prefs_ok_cb(GtkWidget *widget, gpointer data);

static void exec_replyto_cb(GObject *obj, FolderItem *item, const gchar *file, guint num);
static void exec_replyto_who_cb(void);
static void exec_replyto_menu_cb(void);
static void exec_replyto_onoff_cb(void);
GtkWidget *create_config_main_page(GtkWidget *notebook, GKeyFile *pkey);
GtkWidget *create_config_about_page(GtkWidget *notebook, GKeyFile *pkey);

static void command_path_clicked(GtkWidget *widget, gpointer data);
static void compose_created_cb(GObject *obj, gpointer compose);
static void messageview_show_cb(GObject *obj, gpointer msgview,
				MsgInfo *msginfo, gboolean all_headers);
static void summaryview_menu_popup_cb(GObject *obj, GtkItemFactory *ifactory,
                                      gpointer data);

#define GET_RC_BOOLEAN(section, keyarg) g_key_file_get_boolean(g_opt.rcfile, section, keyarg, NULL)
#define SET_RC_BOOLEAN(section, keyarg,valarg) g_key_file_set_boolean(g_opt.rcfile, section, keyarg, valarg)

#define ALIGN_TOP 3
#define ALIGN_BOTTOM 3
#define ALIGN_LEFT 6
#define ALIGN_RIGHT 6
#define BOX_SPACE 6

#endif /* __REPLYTO_H__ */
