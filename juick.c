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
#endif

#include <ctype.h>
#include <glib.h>

// libpurple
#include <core.h>
#include <debug.h>
#include <plugin.h>
#include <pluginpref.h>
#include <version.h>

// pidgin
#include <gtkplugin.h>
#include <gtkimhtml.h>
#include <gtkconv.h>

#define DBGID "juick"
#define JUICK_JID "juick@juick.com"
#define JUBO_JID "jubo@nologin.ru"
#define PREF_PREFIX "/plugins/core/juick-plugin"
#define PREF_USE_AVATAR PREF_PREFIX "/avatar-p"
#define PREF_USE_ID_PLUS PREF_PREFIX "/id_plus"

static gboolean
juick_on_displaying(PurpleAccount *account, const char *who,
	   char **displaying, PurpleConversation *conv,
	   PurpleMessageFlags flags)
{
	GString *output;
	gchar prev_char, old_char, *src, *msgid;
	const gchar *account_user;
	int i = 0, j = 0, ai = 0;

	purple_debug_info(DBGID, "%s\n", __FUNCTION__);

	if( (!g_ascii_strcasecmp(who, JUICK_JID) && 
             !g_ascii_strcasecmp(who, JUBO_JID)) || 
                (flags & PURPLE_MESSAGE_SYSTEM) ) 
	{
		return FALSE;
	}

	output = g_string_new("");
	src = purple_markup_strip_html(*displaying);
	purple_debug_info(DBGID, "%s\n", *displaying);

	account_user = purple_account_get_username(account);
	// now search message text and look for things to highlight
	prev_char = src[i];
	while(src[i] != '\0') {
		//purple_debug_info(DBGID, "prev_char = %c, src[i] == %c\n", prev_char, src[i]);
		if( (i == 0 || isspace(prev_char) || prev_char == '>') && (src[i] == '@' || src[i] == '#') )
		{
			prev_char = src[i];
			i++;
			j = i;
			if (prev_char == '@') {
				while ( (src[j] != '\0') && (isalnum(src[j]) || src[j] == '-') )
					j++;
			} else if (prev_char == '#') {
				while ( (src[j] != '\0') && (isdigit(src[j]) || src[j] == '/' || src[j] == '+') )
					j++;
			}
			if (i == j) {
				g_string_append_c(output, prev_char);
				continue;
			}
			old_char = src[j];
			src[j] = '\0';
			msgid = &src[i - 1];
			g_string_append_printf(output, "<a href=\"j://q?account=%s&body=%s\">%s</a>", account_user, msgid, msgid);
			src[j] = old_char;
			if (src[j] == ':') 
				ai = 0;
			i = j;
			prev_char = src[i - 1];
		} else if (isspace(prev_char) && ai == 2 && src[i] == '*') {
			j = i;
			while (src[j] != '\0' && isspace(prev_char) && src[j] == '*') {
				j++;
				while (!isspace(src[j]) && src[j] != '<')
					j++;
				prev_char = src[j];
				if (src[j] != '\0')
					j++;
			}
			j--;
			old_char = src[j];
			src[j] = '\0';
			msgid = &src[i - 1];
			g_string_append_printf(output, "<font color=\"grey\">%s</font>", msgid);
			src[j] = old_char;
			i = j;
			prev_char = src[i - 1];
		} else {
			g_string_append_c(output, src[i]);
			prev_char = src[i];
			i++; ai++;
		}
	}
	g_free(src);
	free(*displaying);
	src = g_string_free(output, FALSE);
	//purple_debug_info(DBGID, "%s\n", src);
	(*displaying) = purple_markup_linkify(src);
	g_free(src);
	return FALSE;

}
static gboolean
juick_uri_handler(const char *proto, const char *cmd, GHashTable *params)
{
	PurpleAccount *account = NULL;
	PurpleConversation *conv = NULL;
	PidginConversation *gtkconv;
	gchar *body = NULL, *account_user = NULL;
//	char *account_id = g_hash_table_lookup(params, "account");


	purple_debug_info(DBGID, "juick_uri_handler %s\n", proto);

//	account = purple_accounts_find(account_id, "prpl-jabber");

	if (g_ascii_strcasecmp(proto, "j") == 0) {
		body = g_hash_table_lookup(params, "body");
		account_user = g_hash_table_lookup(params, "account");
		account = purple_accounts_find(account_user, "prpl-jabber");
		if (body && account) {
			conv = purple_find_conversation_with_account 
			 (PURPLE_CONV_TYPE_ANY, JUICK_JID, account);
			if (!conv) {
				conv = purple_conversation_new (PURPLE_CONV_TYPE_IM, 
								account, JUICK_JID);
			}
			purple_conversation_present(conv);
			gtkconv = PIDGIN_CONVERSATION(conv);
			gtk_text_buffer_insert_at_cursor(gtkconv->entry_buffer, body, -1);
			gtk_widget_grab_focus(GTK_WIDGET(gtkconv->entry));
			return TRUE;
		}
	}
	return FALSE;
}

#if PURPLE_VERSION_CHECK(2, 6, 0)
static gboolean juick_url_clicked_cb(GtkIMHtml * imhtml, GtkIMHtmlLink * link)
{
        const gchar * url = gtk_imhtml_link_get_url(link);

        purple_debug_info(DBGID, "%s called\n", __FUNCTION__);
        purple_debug_info(DBGID, "url = %s\n", url);

        purple_got_protocol_handler_uri(url);

        return TRUE;
}

static gboolean juick_context_menu(GtkIMHtml * imhtml, GtkIMHtmlLink * link, 
                                                           GtkWidget * menu)
{
        purple_debug_info(DBGID, "%s called\n", __FUNCTION__);
        // Nothing yet T_T
        return TRUE;
}
#else
static PurpleNotifyUiOps juick_ops;
static void *(*saved_notify_uri)(const char *uri);

static void *juick_notify_uri(const char *uri) {
	void *retval = NULL;

	purple_debug_misc(DBGID, "Go url %s\n", uri);

	if(strncmp(uri, "j:", 2) == 0) {
		purple_got_protocol_handler_uri(uri);
	} else {
		retval = saved_notify_uri(uri);
	}
	return retval;
}
#endif

static PurplePluginPrefFrame * get_plugin_pref_frame(PurplePlugin *plugin)
{
        PurplePluginPrefFrame *frame;
        PurplePluginPref *ppref;

        frame = purple_plugin_pref_frame_new();

        ppref = purple_plugin_pref_new_with_name_and_label(PREF_USE_AVATAR, 
                                               ("Use Avatar (very slow)"));
        purple_plugin_pref_frame_add(frame, ppref);

        ppref = purple_plugin_pref_new_with_name_and_label(PREF_USE_ID_PLUS,
                                                        ("Add + after id"));
        purple_plugin_pref_frame_add(frame, ppref);

	return frame;	
}

gboolean plugin_load(PurplePlugin *plugin)
{
#if PURPLE_VERSION_CHECK(2, 6, 0)
	gtk_imhtml_class_register_protocol("j://", juick_url_clicked_cb, 
                                                    juick_context_menu);
#else
	memcpy(&juick_ops, purple_notify_get_ui_ops(), 
                           sizeof(PurpleNotifyUiOps));
	saved_notify_uri = juick_ops.notify_uri;
	juick_ops.notify_uri = juick_notify_uri;
	purple_notify_set_ui_ops(&juick_ops);
#endif
	purple_signal_connect(purple_get_core(), "uri-handler", plugin, 
                             PURPLE_CALLBACK(juick_uri_handler), NULL);

	purple_signal_connect(pidgin_conversations_get_handle(), 
"displaying-im-msg", plugin, PURPLE_CALLBACK(juick_on_displaying), NULL);

	return TRUE;
}

gboolean plugin_unload(PurplePlugin *plugin)
{
#if PURPLE_VERSION_CHECK(2, 6, 0)
	gtk_imhtml_class_register_protocol("j://", NULL, NULL);
#else
	juick_ops.notify_uri = saved_notify_uri;
	purple_notify_set_ui_ops(&juick_ops);
#endif
	purple_signal_disconnect(purple_get_core(), "uri-handler", plugin, 
                                      PURPLE_CALLBACK(juick_uri_handler));

	purple_signal_disconnect(purple_conversations_get_handle(), 
"displaying-im-msg", plugin, PURPLE_CALLBACK(juick_on_displaying));

	return TRUE;
}

static PurplePluginUiInfo prefs_info =
{
	get_plugin_pref_frame,
	0, /* page_num (reserved) */
	NULL, /* frame (reserved */
	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};


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
	"juick",                                            /**< name */
	"0.1",                                              /**< version */
	"Adds some color and button for juick bot.",        /**< summary */
	"Adds some color and button for juick bot.\nUnfortunately pidgin developers have decided that more than 100 tags may not be necessary when displaying the message",        /**< description */
	"owner.mad.epa@gmail.com",                          /**< author */
	"http://github.com/mad/pidgin-juick-plugin",        /**< homepage */
	plugin_load,                                        /**< load */
	plugin_unload,                                      /**< unload */
	NULL,                                               /**< destroy */
	NULL,                                               /**< ui_info        */
	NULL,                                               /**< extra_info */
	&prefs_info,                                        /**< prefs_info */
	NULL,                                               /**< actions */
		  					    /**< padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
	purple_prefs_add_none(PREF_PREFIX);
	purple_prefs_add_bool(PREF_USE_AVATAR, FALSE);
	purple_prefs_add_bool(PREF_USE_ID_PLUS, FALSE);
}

PURPLE_INIT_PLUGIN(juick, init_plugin, info)
