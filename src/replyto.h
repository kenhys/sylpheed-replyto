/*
 * ReplyTo -- reply to plug-in for Sylpheed
 * Copyright (C) 2012 HAYASHI Kentaro
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
  
  GtkWidget *startup;
};

typedef struct _ReplyToOption ReplyToOption;

static void init_done_cb(GObject *obj, gpointer data);
static void app_exit_cb(GObject *obj, gpointer data);
static void app_force_exit_cb(GObject *obj, gpointer data);

static gchar *myprocmsg_get_message_file_path(MsgInfo *msginfo);
static void prefs_ok_cb(GtkWidget *widget, gpointer data);

static void exec_replyto_cb(GObject *obj, FolderItem *item, const gchar *file, guint num);
static void exec_replyto_who_cb(void);
static void exec_replyto_menu_cb(void);
static void exec_replyto_onoff_cb(void);
static GtkWidget *create_config_main_page(GtkWidget *notebook, GKeyFile *pkey);
static GtkWidget *create_config_about_page(GtkWidget *notebook, GKeyFile *pkey);

static void command_path_clicked(GtkWidget *widget, gpointer data);
static void compose_created_cb(GObject *obj, gpointer compose);
static void inc_start_cb(GObject *obj, PrefsAccount *ac);
static void inc_finished_cb(GObject *obj, gint new_messages);
static void messageview_show_cb(GObject *obj, gpointer msgview,
				MsgInfo *msginfo, gboolean all_headers);

#define GET_RC_BOOLEAN(section, keyarg) g_key_file_get_boolean(g_opt.rcfile, section, keyarg, NULL)
#define SET_RC_BOOLEAN(section, keyarg,valarg) g_key_file_set_boolean(g_opt.rcfile, section, keyarg, valarg)

#define ALIGN_TOP 3
#define ALIGN_BOTTOM 3
#define ALIGN_LEFT 6
#define ALIGN_RIGHT 6
#define BOX_SPACE 6

#endif /* __REPLYTO_H__ */
