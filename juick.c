/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#include <string.h>
#endif

#include "internal.h"

#include "version.h"
#include "debug.h"

#include "pidgin.h"

#include <debug.h>
#include <core.h>
#include <conversation.h>
#include <gtkconv.h>
#include <gtkprefs.h>
#include <gtkutils.h>
#include "gtkplugin.h"

#define DBGID "juick"

static const char *juick_jid = "juick@juick.com";
int id_last_reply = 0;

char *global_account_id = NULL;

static gboolean
markup_msg(PurpleAccount *account, const char *who, char **displaying,
           PurpleConversation *conv, PurpleMessageFlags flags)
{
  char *t, *tmp;
  char *startnew, *new;
  char *start, *end;
  int i = 0, tag_num = 0;

  if(!strstr(who, juick_jid))
    return FALSE;

  global_account_id = (const gchar *)purple_account_get_username(conv->account);
  tmp = purple_markup_strip_html(*displaying);
  t = purple_markup_linkify(tmp);
  g_free(tmp);

  new = (char *) malloc(strlen(t) * 20);
  memset(new, 0, strlen(t) * 20);
  startnew = new;

  // XXX: use regex ?
  while(*t) {
    // mark #ID
    if(*t == '#'
       && (*(t - 1) == ' ' || *(t - 1) == '\n' || i == 0)) {
      start = t;
      do {
        t++;
      } while(isdigit(*t) || *t == '/');
      end = t;
      // i dont know max len
      if((end - start) > 1
         && (end - start) < 20
         && (*t == ' ' || *t == '\n' || *t == 0 )) {
        strcat(new, "<A HREF=\"j:q?body=");
        strncat(new, start, end - start);
        strcat(new, "+\">");
        strncat(new, start, end - start);
        strcat(new, "</A>");
        i += 25 + (end - start)*2;
        continue;
      } else {
        t = start;
      }
      // Mark @username
    } else if(*t == '@'
              && (*(t - 1) == ' ' || *(t - 1) == '\n')) {
      start = t;
      do {
        t++;
        // XXX: @user@domain.com or ONLY @username ?
      } while(isalpha(*t) || isdigit(*t) || *t == '-');
      end = t;
      if((end - start) > 1
         && (end - start) < 20) {
        strcat(new, "<A HREF=\"j:q?body=");
        strncat(new, start, end - start);
        strcat(new, "\">");
        strncat(new, start, end - start);
        strcat(new, "</A>");
        i += 24 + (end - start)*2;
        continue;
      } else {
        t = start;
      }
      // Mark *tag
      // XXX: mark ONLY first tag in msg
    } else if(*t == '*'
              && *(t - 1) == ' '
              && *(t - 2) == ':') {
      start = t;
      tag_num = 0;
      do {
        if(*(t + 1) == '*') {
          new[i] = *t;
          i++;
          t++;
          start = t;
        } else if(*t != '*') {
          break;
        }
        do {
          t++;
        } while(*t != ' '
                && *t != '\n'
                && *t != 0);
        end = t;
        // i dont know max len
        if((end - start) > 1
           && (end - start) < 40) {
          strcat(new, "<FONT COLOR=\"#999999\"><I>");
          strncat(new, start, end - start);
          strcat(new, "</I></FONT>");
          i += 36 + end - start;
          tag_num++;
        } else {
          t = start;
          break;
        }
      } while(*t != '\n'
              && tag_num < 5);
      if(t != start) {
        continue;
      }
      // Mark *bold*
    } else if(*t == '*'
              && (*(t - 1) == ' ' || *(t - 1) == '\n')) {
      start = t;
      do {
        t++;
      } while(*t != ' '
              && *t != '\n'
              && *t != '*'
              && *t != 0);
      if(*t != '*') {
        end = start;
      } else {
        end = t;
      }
      if((end - start) > 1
         && (end - start) < 40
         && (*(t + 1) == ' ' || *(t + 1) == '\n' || *(t + 1) == 0)) {
        // skip *
        start++;
        strcat(new, "<B>");
        strncat(new, start, end - start);
        strcat(new, "</B>");
        i += 7 + end - start;
        // skip *
        t++;
        continue;
      } else {
        t = start;
      }
      // Mark /italic/
    } else if(*t == '/'
              && (*(t - 1) == ' ' || *(t - 1) == '\n')) {
      start = t;
      do {
        t++;
      } while(*t != ' '
              && *t != '\n'
              && *t != '/'
              && *t != 0);
      if(*t != '/') {
        end = start;
      } else {
        end = t;
      }
      if((end - start) > 1
         && (end - start) < 40
         && (*(t + 1) == ' ' || *(t + 1) == '\n' || *(t + 1) == 0)) {
        // skip /
        start++;
        strcat(new, "<I>");
        strncat(new, start, end - start);
        strcat(new, "</I>");
        i += 7 + end - start;
        // skip /
        t++;
        continue;
      } else {
        t = start;
      }
      // Mark _underline_
    } else if(*t == '_'
              && (*(t - 1) == ' ' || *(t - 1) == '\n')) {
      start = t;
      do {
        t++;
      } while(*t != ' '
              && *t != '\n'
              && *t != '_'
              && *t != 0);
      if(*t != '_') {
        end = start;
      } else {
        end = t;
      }
      if((end - start) > 1
         && (end - start) < 40
         && (*(t + 1) == ' ' || *(t + 1) == '\n' || *(t + 1) == 0)) {
        // skip _
        start++;
        strcat(new, "<U>");
        strncat(new, start, end - start);
        strcat(new, "</U>");
        i += 7 + end - start;
        // skip _
        t++;
        continue;
      } else {
        t = start;
      }
    }
    new[i] = *t;
    i++;
    t++;
  }
  t = *displaying;
  new = startnew;

  *displaying = g_strdup_printf("%s", new);
  free(new);
  g_free(t);

  return FALSE;
}

static gboolean juick_uri_handler(const char *proto, const char *cmd, GHashTable *params)
{
  PurpleAccount *account = NULL;
  PurpleConversation *conv = NULL;
  PidginConversation *gtkconv;
  gchar *body = NULL;

  purple_debug_info(DBGID, "juick_uri_handler\n");

  account = purple_accounts_find(global_account_id, "prpl-jabber");

  purple_debug_info(DBGID, "account %d\n", account);

  if (g_ascii_strcasecmp(proto, "j") == 0) {
    body = g_hash_table_lookup(params, "body");
    if (body && account) {
      conv = purple_find_conversation_with_account
	(PURPLE_CONV_TYPE_ANY, "juick@juick.com", account);
      gtkconv = PIDGIN_CONVERSATION(conv);
      gtk_text_buffer_insert_at_cursor(gtkconv->entry_buffer, body, -1);
      gtk_widget_grab_focus(GTK_WIDGET(gtkconv->entry));
      return TRUE;
    }
  }
  return FALSE;
}

static gboolean
intercept_sent(PurpleAccount *account, const char *who, char **message, void* pData)
{
  char *msg;

  if(!strstr(who, juick_jid))
    return FALSE;

  if (message == NULL || *message == NULL || **message == '\0')
    return FALSE;

  msg = *message;

  while(*msg == ' ' || *msg == '\n' || *msg == '\t') msg++;

  if(*msg == '#') {
    msg++;
    id_last_reply = atoi(msg);
      purple_debug_misc("purple-juick", "Juick-plugin: mem last reply %d\n",
                        id_last_reply);
  }
  return TRUE;
}

static void
cmd_button_cb(GtkButton *button, PidginConversation *gtkconv)
{
  PurpleConversation *conv = gtkconv->active_conv;
  PurpleAccount *account;
  PurpleConnection *connection;
  const char *label;
  const char *name;

  char tmp[80];

  name = purple_conversation_get_name(conv);
  account = purple_conversation_get_account(conv);
  connection = purple_conversation_get_gc(conv);
  label = gtk_button_get_label(button);

  if(strcmp(label, "last reply") == 0) {
    purple_debug_misc("purple-juick", "Juick-plugin: last reply button %d\n",
                      id_last_reply);
    if(id_last_reply) {
      sprintf(tmp, "purple-url-handler \"xmpp:juick@juick.com?message;body=%%23%d+\"&",
              id_last_reply);
      system(tmp);
    }
  } else if(strcmp(label, "last msg") == 0) {
    serv_send_im(connection, juick_jid, "#", PURPLE_MESSAGE_SEND);
  }
}

static void
create_juick_button_pidgin(PidginConversation *gtkconv)
{
  GtkWidget *last_reply_button, *last_button;
  PurpleConversation *conv;
  const char *who;

  conv = gtkconv->active_conv;
  who = purple_conversation_get_name(conv);

  if(!strstr(who, juick_jid))
    return;

  last_reply_button = g_object_get_data(G_OBJECT(gtkconv->toolbar), "last_reply_button");
  last_button = g_object_get_data(G_OBJECT(gtkconv->toolbar), "last_button");

  if (last_reply_button || last_button)
    return;

  last_reply_button = gtk_button_new_with_label(_("last reply"));
  last_button = gtk_button_new_with_label(_("last msg"));

  g_signal_connect(G_OBJECT(last_reply_button), "clicked",
                   G_CALLBACK(cmd_button_cb), gtkconv);
  g_signal_connect(G_OBJECT(last_button), "clicked",
                   G_CALLBACK(cmd_button_cb), gtkconv);

  gtk_box_pack_start(GTK_BOX(gtkconv->toolbar), last_reply_button,
                     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(gtkconv->toolbar), last_button,
                     FALSE, FALSE, 0);

  gtk_widget_show(last_reply_button);
  gtk_widget_show(last_button);

  g_object_set_data(G_OBJECT(gtkconv->toolbar), "last_reply_button", last_reply_button);
  g_object_set_data(G_OBJECT(gtkconv->toolbar), "last_button", last_button);

}

static void
remove_juick_button_pidgin(PidginConversation *gtkconv)
{
  GtkWidget *last_reply_button = NULL, *last_button = NULL;

  last_reply_button = g_object_get_data(G_OBJECT(gtkconv->toolbar), "last_reply_button");

  if (last_reply_button) {
    gtk_widget_destroy(last_reply_button);
    gtk_widget_destroy(last_button);
    g_object_set_data(G_OBJECT(gtkconv->toolbar), "last_reply_button", NULL);
    g_object_set_data(G_OBJECT(gtkconv->toolbar), "last_button", NULL);
  }
}

static void
conversation_displayed_cb(PidginConversation *gtkconv)
{
  GtkWidget *last_reply_button = NULL;

  last_reply_button = g_object_get_data(G_OBJECT(gtkconv->toolbar),
                                        "last_reply_button");
  if (last_reply_button == NULL) {
    create_juick_button_pidgin(gtkconv);
  }
}

static gboolean
window_keypress_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  PidginConversation *gtkconv;
  PurpleConversation *conv;
  PurpleConnection *connection;
  char tmp[80];

  gtkconv  = (PidginConversation *)data;
  conv = gtkconv->active_conv;
  connection = purple_conversation_get_gc(conv);

  if (event->state & GDK_CONTROL_MASK) {
    // XXX: locale ?
    switch (event->keyval) {
    case 's':
    case 'S':
      // this cyrilic 'Ы' 'ы'
    case 1753:
    case 1785:
      purple_debug_misc("purple-juick", "Juick-plugin: \\C-s pressed\n");
      serv_send_im(connection, juick_jid, "#", PURPLE_MESSAGE_SEND);
      break;
    case 'r':
    case 'R':
      // this cyrrilic 'К' 'к'
    case 1739:
    case 1771:
      purple_debug_misc("purple-juick", "Juick-plugin: \\C-r pressed\n");
      if(id_last_reply) {
        sprintf(tmp, "purple-url-handler \"xmpp:juick@juick.com?message;body=%%23%d+\"&",
                id_last_reply);
        system(tmp);
      }
      break;
    }
  }
  return FALSE;
}

static gboolean
add_key_handler_cb(PurpleConversation *conv)
{
  PidginConversation *gtkconv;

  gtkconv = PIDGIN_CONVERSATION(conv);

  /* Intercept keystrokes from the menu items */
  g_signal_connect(G_OBJECT(gtkconv->entry), "key_press_event",
                   G_CALLBACK(window_keypress_cb), gtkconv);
  return FALSE;
}

static PurpleNotifyUiOps juick_ops;
static void *(*saved_notify_uri)(const char *uri);

static void * juick_notify_uri(const char *uri) {
  void * retval = NULL;

  if(strncmp(uri, "j:", 2) == 0) {
    purple_got_protocol_handler_uri(uri);
  } else {
    retval = saved_notify_uri(uri);
  }
  return retval;
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
  GList *convs = purple_get_conversations();
  PidginConversation *gtk_conv_handle = pidgin_conversations_get_handle();
  void *conv_handle = purple_conversations_get_handle();

  /* for markup */
  purple_signal_connect(gtk_conv_handle, "displaying-im-msg", plugin,
                        PURPLE_CALLBACK(markup_msg), NULL);

  /* for make button */
  purple_signal_connect(gtk_conv_handle, "conversation-displayed", plugin,
                        PURPLE_CALLBACK(conversation_displayed_cb), NULL);

  /* for mem last reply */
  purple_signal_connect(conv_handle, "sending-im-msg", plugin,
                        PURPLE_CALLBACK(intercept_sent), NULL);

  purple_signal_connect(conv_handle, "conversation-created",
                        plugin, PURPLE_CALLBACK(add_key_handler_cb), NULL);

  // XXX: purple_conversation_foreach (init_conversation); ?
  while (convs) {
    PurpleConversation *conv = (PurpleConversation *)convs->data;
    PidginConversation *gtkconv;

    gtkconv = PIDGIN_CONVERSATION(conv);
    /* Intercept keystrokes from the menu items */
    g_signal_connect(G_OBJECT(gtkconv->entry), "key_press_event",
                     G_CALLBACK(window_keypress_cb), gtkconv);

    /* Setup Send button */
    if (PIDGIN_IS_PIDGIN_CONVERSATION(conv)) {
      create_juick_button_pidgin(PIDGIN_CONVERSATION(conv));
    }
    convs = convs->next;
  }
  memcpy(&juick_ops, purple_notify_get_ui_ops(), sizeof(PurpleNotifyUiOps));
  saved_notify_uri = juick_ops.notify_uri;
  juick_ops.notify_uri = juick_notify_uri;
  purple_notify_set_ui_ops(&juick_ops);
  purple_signal_connect(purple_get_core(), "uri-handler", plugin, PURPLE_CALLBACK(juick_uri_handler), NULL);

  return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
  GList *convs = purple_get_conversations();

  while (convs) {
    PurpleConversation *conv = (PurpleConversation *)convs->data;
    /* Remove Send button */
    if (PIDGIN_IS_PIDGIN_CONVERSATION(conv)) {
      remove_juick_button_pidgin(PIDGIN_CONVERSATION(conv));
    }
    convs = convs->next;
  }
  return TRUE;
}

static PurplePluginInfo info =
  {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,                               /**< major version */
    PURPLE_MINOR_VERSION,                               /**< minor version */
    PURPLE_PLUGIN_STANDARD,                             /**< type */
    PIDGIN_PLUGIN_TYPE,                                 /**< ui_requirement */
    0,                                                  /**< flags */
    NULL,                                               /**< dependencies */
    PURPLE_PRIORITY_DEFAULT,                            /**< priority */

    "gtkjuick",                                         /**< id */
    N_("juick"),                                        /**< name */
    N_("0.1"),                                          /**< version */
    N_("Adds some color and button for juick bot."),    /**< summary */
    N_("Adds some color and button for juick bot."),    /**< description */
    "owner.mad.epa@gmail.com",                          /**< author */
    PURPLE_WEBSITE,                                     /**< homepage */
    plugin_load,                                        /**< load */
    plugin_unload,                                      /**< unload */
    NULL,                                               /**< destroy */
    NULL,                                               /**< ui_info        */
    NULL,                                               /**< extra_info */
    NULL,                                               /**< prefs_info */
    NULL,                                               /**< actions */
    /* padding */
    NULL,
    NULL,
    NULL,
    NULL
  };

static void
init_plugin(PurplePlugin *plugin)
{
}

PURPLE_INIT_PLUGIN(sendbutton, init_plugin, info)
