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
#include <gtk/gtk.h>

#include "alertpanel.h"

#define REPLYTO "replyto"
#define REPLYTORC "replytorc"

#define SYLPF_ID replyto
#define SYLPF_OPTION replyto_option

#define SYLPF_FUNC(arg) replyto ## _ ## arg

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

  GtkWidget *plugin_on;
  GtkWidget *plugin_off;
  GtkWidget *plugin_switch;
  GtkTooltips *plugin_tooltip;

  gboolean plugin_enabled;

  gchar *to;

  GtkWidget *combo;
  MsgInfo *msginfo;

  gboolean replyto_flg;
};

typedef struct _ReplyToOption ReplyToOption;

GtkWidget *create_config_main_page(GtkWidget *notebook, GKeyFile *pkey);
GtkWidget *create_config_about_page(GtkWidget *notebook, GKeyFile *pkey);
void setup_plugin_onoff_switch(ReplyToOption *option,
                               GCallback callback_func,
                               const char **on_xpm,
                               const char **off_xpm);
void update_plugin_onoff_status(ReplyToOption *option,
                                gboolean onoff,
                                const char *title,
                                const char *message,
                                const char *tooltip);

#define GET_RC_BOOLEAN(section, keyarg) g_key_file_get_boolean(option.rcfile, section, keyarg, NULL)
#define SET_RC_BOOLEAN(section, keyarg,valarg) g_key_file_set_boolean(option.rcfile, section, keyarg, valarg)

#define ALIGN_TOP 3
#define ALIGN_BOTTOM 3
#define ALIGN_LEFT 6
#define ALIGN_RIGHT 6
#define BOX_SPACE 6

#endif /* __REPLYTO_H__ */
