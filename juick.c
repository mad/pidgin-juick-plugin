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
#include <string.h>

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
#include <gtksound.h>

#define DBGID "juick"
#define JUICK_JID "juick@juick.com"
#define JUBO_JID "jubo@nologin.ru"
#define NS_JUICK_MESSAGE "http://juick.com/query#messages"

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
	int i = 0, j = 0, ai = 0, tag_count = 0, tag_max = 92;

	purple_debug_info(DBGID, "%s\n", __FUNCTION__);

	if( (!strcmp(who, JUICK_JID) && 
             !strcmp(who, JUBO_JID)) || 
                (flags & PURPLE_MESSAGE_SYSTEM) ) 
	{
		return FALSE;
	}

	output = g_string_new("");
	src = *displaying;

	account_user = purple_account_get_username(account);
	// now search message text and look for things to highlight
	prev_char = src[i];
	while(src[i] != '\0') {
		//purple_debug_info(DBGID, "prev_char = %c, src[i] == %c\n", prev_char, src[i]);
		if (src[i] == '<')
			tag_count++;
		if (tag_count > tag_max) {
			msgid = purple_markup_strip_html(&src[i]);
			g_string_append_printf(output, "%s", msgid);
			free(msgid);
			break;			
		}
		if( (i == 0 || isspace(prev_char) || prev_char == '>' || prev_char == '(') && (src[i] == '@' || src[i] == '#') )
		{
			prev_char = src[i];
			i++;
			j = i;
			if (prev_char == '@') {
				while ( (src[j] != '\0') && (isalnum(src[j]) || src[j] == '-'|| src[j] == '@' || src[j] == '.')) 
					j++;
			} else if (prev_char == '#') {
				while ( (src[j] != '\0') && (isdigit(src[j]) || src[j] == '/') ) {
					if (src[j] == '/') 
						prev_char = '#';
					j++;
				}
			}
			if (i == j) {
				g_string_append_c(output, prev_char);
				continue;
			}
			old_char = src[j];
			src[j] = '\0';
			msgid = &src[i - 1];
			g_string_append_printf(output, 
			  "<a href=\"j://q?account=%s&reply=%c&body=%s\">%s</a>",
			  account_user, prev_char, msgid, msgid);
			tag_count++; tag_count++;
			src[j] = old_char;
			if (src[j] == ':') 
				ai = 0;
			i = j;
			prev_char = src[i - 1];
			continue;
		} else if (isspace(prev_char) && ai == 0 && src[i] == ':') {
			
		} /*else if (isspace(prev_char) && ai == 2 && src[i] == '*') {
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
			tag_count++; tag_count++;
			src[j] = old_char;
			i = j;
			prev_char = src[i - 1];
			continue;
		}*/
		g_string_append_c(output, src[i]);
		prev_char = src[i];
		i++; ai++;
	}
	free(*displaying);
	(*displaying) = g_string_free(output, FALSE);
	return FALSE;

}

void xmlnode_received_cb(PurpleConnection *gc, xmlnode **packet)
{
	xmlnode *node, *bodyn, *tagn;
	gchar *body = NULL, *bodyup = NULL, *tags = NULL, *tag = NULL, 
	      *s = NULL, *midrid = NULL, *comment = NULL;
	const char *uname, *mid, *rid, *mood, *ts, *from, *replies, *replyto;
	GString *output = NULL;
	PurpleConversation *conv;
	PurpleMessageFlags flags;
	int i = 0;
	gboolean first = TRUE;

	purple_debug_info(DBGID, "%s\n", __FUNCTION__);

	node = xmlnode_get_child(*packet, "query");
	if (node) {
		node = xmlnode_get_child(node, "juick");
	} else 
		node = xmlnode_get_child(*packet, "juick");

	from = xmlnode_get_attrib(*packet, "from");

	output = g_string_new("");
	while (node) {
		bodyn = xmlnode_get_child(node, "body");
		if (bodyn) {
			body = xmlnode_get_data_unescaped(bodyn);
			uname = xmlnode_get_attrib(node, "uname");
			mid = xmlnode_get_attrib(node, "mid");
			rid = xmlnode_get_attrib(node, "rid");
			ts = xmlnode_get_attrib(node, "ts");
			replies = xmlnode_get_attrib(node, "replies");
			mood = xmlnode_get_attrib(node, "mood");
			replyto = xmlnode_get_attrib(node, "replyto");
			if (replyto) {

			}
			tagn = xmlnode_get_child(node, "tag");
			while (tagn) {
				tag = xmlnode_get_data(tagn);
				if (tag) {
					if (tags) {
						s = g_strdup_printf("%s *%s", tags, tag);
						g_free(tags);
						tags = s;
					} else {
						tags = g_strdup_printf("*%s", tag);
					}
				}
				g_free(tag);
				tagn = xmlnode_get_next_twin(tagn);
			}
			if (tags && mood)
				s = g_strdup_printf("%s mood: %s<br/>", tags, mood);
			else if (tags)
				s = g_strdup_printf("%s<br/>", tags);
			else if (mood)
				s = g_strdup_printf("mood: %s<br/>", mood);
			else
				s = g_strdup_printf("<br/>");
			g_free(tags);
			if (rid)
				midrid = g_strdup_printf("%s/%s", mid, rid);
			else
				midrid = g_strdup_printf("%s", mid);
			bodyup = xmlnode_get_data(xmlnode_get_child(*packet, "body"));
			if (bodyup) {
				if (replyto)
					comment = strchr(bodyup, '>');
				i = 0;
				if (bodyup && bodyup[0] != '@') {
					while (!isspace(bodyup[i]) || isblank(bodyup[i]) || bodyup[i + 1] == '>')
						i++;
					bodyup[i + 1] = '\0';
				}
			}
			if (bodyup && i != 0 && !replyto)
				g_string_prepend(output, bodyup);
			g_free(bodyup);
			if (replyto && comment)
				g_string_append_printf(output, "%s @%s reply to %s: %s<br/>%s%s<br/>#%s", ts, uname, replyto, s, comment, body, midrid);
			else
				g_string_append_printf(output, "%s @%s: %s%s<br/>#%s", ts, uname, s, body, midrid);
			g_free(s);
			g_free(body);
			if (first) {
				i = 0;
				while (midrid[i]) {
					if (midrid[i] == '/') {
						midrid[i] = '#';
						break;
					}
					i++;
				}
				g_string_append_printf(output, " http://juick.com/%s<br/>", midrid);
				if (replies)
					g_string_append_printf(output, "Replies (%s)<br/>", replies);
			} else
				g_string_append(output, "<br/>");
			g_free(midrid);
		}
		node = xmlnode_get_next_twin(node);
		first = FALSE;
	}
	flags = PURPLE_MESSAGE_RECV | PURPLE_MESSAGE_NOTIFY;
	if (output->len != 0) {
		conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, gc->account, from);
		body = g_string_free(output, FALSE);
//		purple_sound_play_event(PURPLE_SOUND_FIRST_RECEIVE, gc->account);
		purple_conv_im_write(PURPLE_CONV_IM(conv), conv->name, body, flags, time(NULL));
//		g_free(body);
		xmlnode_free(*packet);
		*packet = NULL;
	} else {
		g_string_free(output, TRUE);
		node = xmlnode_get_child(*packet, "error");
		if (node && from && ((strcmp(from, JUICK_JID) == 0) || (strcmp(from, JUBO_JID) == 0))) {
			conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, gc->account, from);
			body = g_strdup_printf("error %s", xmlnode_get_attrib(node, "code"));
			purple_conv_im_write(PURPLE_CONV_IM(conv), conv->name, body, flags, time(NULL));
//			g_free(body);
		}
	}
}

static void
message_do(PurpleConnection *gc, const char *msgid, gboolean rid)
{
	xmlnode *iq, *query;

	iq = xmlnode_new("iq");
	xmlnode_set_attrib(iq, "type", "get");
	xmlnode_set_attrib(iq, "to", JUICK_JID);
	xmlnode_set_attrib(iq, "id", "123");

	query = xmlnode_new_child(iq, "query");
	xmlnode_set_namespace(query, NS_JUICK_MESSAGE);
	if (msgid) {
		xmlnode_set_attrib(query, "mid", msgid);
		if (rid)
			xmlnode_set_attrib(query, "rid", "*");
	}

	purple_signal_emit(purple_connection_get_prpl(gc), "jabber-sending-xmlnode", gc, &iq);
	if (iq != NULL)
		xmlnode_free(iq);
}

static gboolean
juick_uri_handler(const char *proto, const char *cmd, GHashTable *params)
{
	PurpleAccount *account = NULL;
	PurpleConversation *conv = NULL;
	PidginConversation *gtkconv;
	PurpleConnection *gc;
	PurplePluginProtocolInfo *prpl_info;
	gchar *body = NULL, *account_user = NULL, *reply = NULL, *send = NULL;

	purple_debug_info(DBGID, "juick_uri_handler %s\n", proto);

	if (g_ascii_strcasecmp(proto, "j") == 0) {
		body = g_hash_table_lookup(params, "body");
		account_user = g_hash_table_lookup(params, "account");
		reply = g_hash_table_lookup(params, "reply");
		send = g_hash_table_lookup(params, "send");
		account = purple_accounts_find(account_user, "prpl-jabber");
		if (body && account) {
			conv = purple_conversation_new (PURPLE_CONV_TYPE_IM, 
								account, JUICK_JID);
			purple_conversation_present(conv);
			gtkconv = PIDGIN_CONVERSATION(conv);
			gc = purple_conversation_get_gc(gtkconv->active_conv);
			prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl);
			if (reply[0] == '#') {
				if (!send) {
					message_do(gc, body + 1, FALSE);
					message_do(gc, body + 1, TRUE);
					gtk_text_buffer_set_text(gtkconv->entry_buffer, body, -1);
				} else if (send[0] == 'p')
					gtk_text_buffer_insert_at_cursor(gtkconv->entry_buffer, body, -1);
			} else if (reply[0] == '@') {
				reply = g_strdup_printf("%s+", body);
				if (send && g_strrstr(send, "ip")) {
					gtk_text_buffer_insert_at_cursor(gtkconv->entry_buffer, body, -1);
				} else if (send && send[0] == 'i'){
					serv_send_im(gc, JUICK_JID, body, PURPLE_MESSAGE_SEND);
				} else if (send && send[0] == 'p') {
					serv_send_im(gc, JUICK_JID, reply, PURPLE_MESSAGE_SEND);
				} else {
					serv_send_im(gc, JUICK_JID, body, PURPLE_MESSAGE_SEND);
					serv_send_im(gc, JUICK_JID, reply, PURPLE_MESSAGE_SEND);
				}
				g_free(reply);
			} else 
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

void menu_activate_cb(GtkMenuItem *menuitem, GtkIMHtmlLink *link)
{
        const gchar * url = gtk_imhtml_link_get_url(link);

        purple_debug_info(DBGID, "%s called\n", __FUNCTION__);

        purple_got_protocol_handler_uri(url);
}

void menu_insert_activate_cb(GtkMenuItem *menuitem, GtkIMHtmlLink *link)
{
        const gchar * url = gtk_imhtml_link_get_url(link);
	gchar *murl = NULL;

        purple_debug_info(DBGID, "%s called\n", __FUNCTION__);

	murl = g_strdup_printf("%s&send=ip", url);
        purple_got_protocol_handler_uri(murl);
	g_free(murl);
}

void menu_user_info_activate_cb(GtkMenuItem *menuitem, GtkIMHtmlLink *link)
{
        const gchar * url = gtk_imhtml_link_get_url(link);
	gchar *murl = NULL;

        purple_debug_info(DBGID, "%s called\n", __FUNCTION__);

	murl = g_strdup_printf("%s&send=i", url);
        purple_got_protocol_handler_uri(murl);
	g_free(murl);
}

void menu_user_posts_activate_cb(GtkMenuItem *menuitem, GtkIMHtmlLink *link)
{
        const gchar * url = gtk_imhtml_link_get_url(link);
	gchar *murl = NULL;

        purple_debug_info(DBGID, "%s called\n", __FUNCTION__);

	murl = g_strdup_printf("%s&send=p", url);
        purple_got_protocol_handler_uri(murl);
	g_free(murl);
}

static gboolean juick_context_menu(GtkIMHtml * imhtml, GtkIMHtmlLink * link, 
                                                           GtkWidget * menu)
{
	GtkWidget *item, *img;
        const gchar *url = gtk_imhtml_link_get_url(link);

        purple_debug_info(DBGID, "%s called\n", __FUNCTION__);

	img = gtk_image_new_from_stock(GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_MENU);
	if (g_strrstr(url, "&reply=@")) {
		item = gtk_image_menu_item_new_with_mnemonic("See _user info and posts");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), img);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_activate_cb), link);
		item = gtk_menu_item_new_with_mnemonic("_Insert");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_insert_activate_cb), link);
		item = gtk_menu_item_new_with_mnemonic("See user _info");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_user_info_activate_cb), link);
		item = gtk_menu_item_new_with_mnemonic("See user _posts");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_user_posts_activate_cb), link);
	} else if (g_strrstr(url, "&reply=#")) {
		item = gtk_image_menu_item_new_with_mnemonic("See _replies");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), img);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_activate_cb), link);
		item = gtk_menu_item_new_with_mnemonic("_Insert");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_user_posts_activate_cb), link);
	} else {
		return FALSE;
	}

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

	void *jabber_handle = purple_plugins_find_with_id("prpl-jabber");
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

	/* Jabber signals */
	if (jabber_handle) 
		purple_signal_connect(jabber_handle, "jabber-receiving-xmlnode", plugin,
		                      PURPLE_CALLBACK(xmlnode_received_cb), NULL);
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
