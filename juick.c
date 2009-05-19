#ifdef HAVE_CONFIG_H
#include <config.h>
#include <string.h>
#endif

#include "internal.h"

#include "version.h"
#include "debug.h"

#include "pidgin.h"

#include "gtkconv.h"
#include "gtkplugin.h"

static const char *juick = "juick@juick.com";
int id_last_reply = 0;

static gboolean
markup_msg(PurpleAccount *account, const char *who, char **displaying,
	   PurpleConversation *conv, PurpleMessageFlags flags)
{
  char *t, *tmp;
  char *startnew, *new;
  char *start, *end;
  int i = 0;

  if(!strstr(who, juick))
    return FALSE;

  tmp = purple_markup_strip_html(*displaying);
  t = purple_markup_linkify(tmp);
  g_free(tmp);

  new = (char *) malloc(strlen(t) * 20);
  memset(new, 0, strlen(t) * 20);
  startnew = new;

  // XXX: use regexp ?
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
        strcat(new, "<A HREF=\"xmpp:juick@juick.com?message;body=");
        strncat(new, start, end - start);
        strcat(new, "+\">");
        strncat(new, start, end - start);
        strcat(new, "</A>");
        i += 50 + (end - start)*2;
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
	 && (end - start) < 17) {
        strcat(new, "<A HREF=\"xmpp:juick@juick.com?message;body=");
        strncat(new, start, end - start);
        strcat(new, "+\">");
        strncat(new, start, end - start);
        strcat(new, "</A>");
        i += 50 + (end - start)*2;
        continue;
      } else {
        t = start;
      }
      // Mark *tag
      // XXX: mark ONLY first tag in msg
    } else if(*t == '*'
	      && *(t - 1) == '\n'
	      && (*(t - 2) == ':' || (*(t - 2) == '\n'
				      && *(t - 3) == ')'))) {
      start = t;
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
        continue;
      } else {
        t = start;
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

static gboolean
intercept_sent(PurpleAccount *account, const char *who, char **message, void* pData)
{
  char *msg;

  if(!strstr(who, juick))
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
    serv_send_im(connection, juick, "#", PURPLE_MESSAGE_SEND);
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

  if(!strstr(who, juick))
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
      serv_send_im(connection, juick, "#", PURPLE_MESSAGE_SEND);
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
    PURPLE_MAJOR_VERSION,                           /**< major version */
    PURPLE_MINOR_VERSION,                           /**< minor version */
    PURPLE_PLUGIN_STANDARD,                         /**< type */
    PIDGIN_PLUGIN_TYPE,                             /**< ui_requirement */
    0,                                              /**< flags */
    NULL,                                           /**< dependencies */
    PURPLE_PRIORITY_DEFAULT,                        /**< priority */

    "gtkjuick",                                     /**< id */
    N_("juick"),                                    /**< name */
    N_("0.1"),                                      /**< version */
    N_("Adds some color and button to juick@juick.com."),         /**< summary */
    N_("Adds some color and button to juick@juick.com."),         /**< description */
    "owner.mad.epa@gmail.com",              /**< author */
    PURPLE_WEBSITE,                                 /**< homepage */
    plugin_load,                                    /**< load */
    plugin_unload,                                  /**< unload */
    NULL,                                           /**< destroy */
    NULL,                                           /**< ui_info */
    NULL,                                           /**< extra_info */
    NULL,                                           /**< prefs_info */
    NULL,                                           /**< actions */

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
