#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "internal.h"

#include "version.h"

#include "pidgin.h"

#include "gtkconv.h"
#include "gtkplugin.h"

static const char *juick = "juick@juick.com";

static gboolean
displaying_msg(PurpleAccount *account, const char *who, char **displaying,
	       PurpleConversation *conv, PurpleMessageFlags flags)
{
  char *t;
  char *startnew, *new;
  char *start, *end;
  int i = 0;

  if(!strstr(who, juick))
    return FALSE;

  t = *displaying;

  /* FIXME: mb realloc ? */

  new = (char *) malloc(strlen(t) * 10);
  memset(new, 0, strlen(t) * 10);
  startnew = new;

  while(*t) {
    if(*t == '#') {
      start = t;
      do {
	t++;
      } while(isdigit(*t));
      if(*t == 0)
	end = --t;
      end = t;
      if((end - start) > 1 && (end - start) < 20) {
      	strcat(new, "<B>");
      	strncat(new, start, end - start);
      	strcat(new, "</B>");
	i += 7 + end - start;
	continue;
      } else {
	t = start;
      }
    } else if(*t == '@') {
      start = t;
      do {
	t++;
      } while(*t != ' ' && *t != ':' && *t != 0);
      if(*t == 0)
	end = --t;
      end = t;
      if((end - start) > 1 && (end - start) < 30) {
      	strcat(new, "<FONT COLOR=\"BLUE\">");
      	strncat(new, start, end - start);
      	strcat(new, "</FONT>");
	i += 26 + end - start;
	continue;
      } else {
	t = start;
      }
    } else  if(*t == '*') {
      start = t;
      do {
	t++;
      } while(*t != ' ' && *t != ':' && *t != 0);
      if(*t == 0)
	end = --t;
      end = t;
      if((end - start) > 1  && (end - start) < 30) {
      	strcat(new, "<FONT COLOR=\"#999999\"><I>");
      	strncat(new, start, end - start);
      	strcat(new, "</I></FONT>");
	i += 36 + end - start;
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

static void
cmd_button_cb(GtkButton *button, PidginConversation *gtkconv)
{
  PurpleConversation *conv = gtkconv->active_conv;
  PurpleAccount *account;
  PurpleConnection *connection;
  const char *label;
  const char *name;

  name = purple_conversation_get_name(conv);
  account = purple_conversation_get_account(conv);
  connection = purple_conversation_get_gc(conv);
  label = gtk_button_get_label(button);

  if(strcmp(label, "Pop blogs") == 0) {
    serv_send_im(connection, juick, "@", PURPLE_MESSAGE_SEND);
  } else if(strcmp(label, "Last") == 0) {
    serv_send_im(connection, juick, "#", PURPLE_MESSAGE_SEND);
  }
}

static void
create_send_button_pidgin(PidginConversation *gtkconv)
{
  GtkWidget *send_button, *last_button;

  send_button = g_object_get_data(G_OBJECT(gtkconv->toolbar), "send_button");
  last_button = g_object_get_data(G_OBJECT(gtkconv->toolbar), "last_button");

  if (send_button || last_button)
    return;

  send_button = gtk_button_new_with_label(_("Pop blogs"));
  last_button = gtk_button_new_with_label(_("Last"));

  g_signal_connect(G_OBJECT(send_button), "clicked",
		   G_CALLBACK(cmd_button_cb), gtkconv);
  g_signal_connect(G_OBJECT(last_button), "clicked",
		   G_CALLBACK(cmd_button_cb), gtkconv);

  gtk_box_pack_start(GTK_BOX(gtkconv->toolbar), send_button,
		     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(gtkconv->toolbar), last_button,
		     FALSE, FALSE, 0);

  gtk_widget_show(send_button);
  gtk_widget_show(last_button);

  g_object_set_data(G_OBJECT(gtkconv->toolbar), "send_button", send_button);
  g_object_set_data(G_OBJECT(gtkconv->toolbar), "last_button", last_button);

}

static void
remove_send_button_pidgin(PidginConversation *gtkconv)
{
  GtkWidget *send_button = NULL, *last_button = NULL;

  send_button = g_object_get_data(G_OBJECT(gtkconv->toolbar), "send_button");
  last_button = g_object_get_data(G_OBJECT(gtkconv->toolbar), "last_button");

  if (send_button) {
    gtk_widget_destroy(send_button);
    gtk_widget_destroy(last_button);
    g_object_set_data(G_OBJECT(gtkconv->toolbar), "send_button", NULL);
    g_object_set_data(G_OBJECT(gtkconv->toolbar), "last_button", NULL);
  }
}

static void
conversation_displayed_cb(PidginConversation *gtkconv)
{
  GtkWidget *send_button = NULL;

  send_button = g_object_get_data(G_OBJECT(gtkconv->lower_hbox),
				  "send_button");
  if (send_button == NULL) {
    create_send_button_pidgin(gtkconv);
  }
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
  GList *convs = purple_get_conversations();
  void *gtk_conv_handle = pidgin_conversations_get_handle();

  purple_signal_connect(gtk_conv_handle, "displaying-im-msg", plugin,
			PURPLE_CALLBACK(displaying_msg), NULL);

  purple_signal_connect(gtk_conv_handle, "conversation-displayed", plugin,
			PURPLE_CALLBACK(conversation_displayed_cb), NULL);

  while (convs) {

    PurpleConversation *conv = (PurpleConversation *)convs->data;

    /* Setup Send button */
    if (PIDGIN_IS_PIDGIN_CONVERSATION(conv)) {
      create_send_button_pidgin(PIDGIN_CONVERSATION(conv));
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
      remove_send_button_pidgin(PIDGIN_CONVERSATION(conv));
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
