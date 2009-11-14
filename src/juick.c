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

#ifndef PURPLE_PLUGINS
#define PURPLE_PLUGINS
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>
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

#define WEBPAGE "http://juick.com/"
#define NS_JUICK_MESSAGES "http://juick.com/query#messages"
#define NS_JUICK_USERS "http://juick.com/query#users"

#define PREF_PREFIX "/plugins/core/juick-plugin"
#define PREF_IS_HIGHLIGHTING_TAGS PREF_PREFIX "/is_highlighting_tags"
#define PREF_IS_SHOW_MAX_MESSAGE PREF_PREFIX "/is_show_max_message"
#define PREF_IS_SHOW_JUICK PREF_PREFIX "/is_show_juick"
#define PREF_IS_INSERT_ONLY PREF_PREFIX "/is_insert_only"

const gchar *IMAGE_PREFIX = "http://i.juick.com/p";

static GHashTable *ht_signal_handlers = NULL;   /* <text_buffer, handler_id> */

static void
add_warning_message(GString *output, gchar *src, int tag_max)
{
	const char *MORETAGSNOTPLACING = _("\n<font color=\"red\">more juick " \
		"tags will not placing due to reach their maximum count: %d " \
		"in the single message\n</font>");
	gchar *s, *s1;
	gboolean is_show_max_message = purple_prefs_get_bool(
						PREF_IS_SHOW_MAX_MESSAGE);

	purple_debug_info(DBGID, "%s\n", __FUNCTION__);

	s = purple_markup_strip_html(src);
	s1 = g_utf8_strchr(s, -1, '\n');
	if (s1) {
		s1 = g_utf8_next_char(s1);
		g_string_append_len(output, s, s1 - s);
		if (is_show_max_message)
			g_string_append_printf(output,
					MORETAGSNOTPLACING, tag_max);
		g_string_append(output, s1);
	} else if (s)
		g_string_append(output, s);
	g_free(s);
}

static void
highlighting_juick_message_tags(GString *output, gchar **current,
						int *tag_count)
{
	gchar *p = *current, *prev = NULL, *msgid, old_char;

	purple_debug_info(DBGID, "%s\n", __FUNCTION__);

	if (!*p)
		return;

	msgid = p;
	prev = g_utf8_prev_char(p);
	while (*p && g_unichar_isspace(g_utf8_get_char(prev)) &&
						*p == '*')
	{
		p = g_utf8_next_char(p);
		while (*p && !g_unichar_isspace(g_utf8_get_char(p)) &&
							*p != '<')
			p = g_utf8_next_char(p);
		if (*p) {
			prev = p;
			p = g_utf8_next_char(p);
		}
	}
	if (*p) {
		p = g_utf8_prev_char(p);
		old_char = *p;
		*p = '\0';
		g_string_append_printf(output,
			"<font color=\"grey\">%s</font>", msgid);
		++*tag_count; ++*tag_count;
		*p = old_char;
		*current = p;
	}
}

static gboolean
get_juick_tag(gchar *text, gchar **result)
{
	gchar *p = text;
	gboolean is_reply = FALSE;

	if (text == NULL) {
		*result = NULL;
		return FALSE;
	}

	p = g_utf8_next_char(p);
	if (*text == '@') {
		while (*p && (g_unichar_isalnum(g_utf8_get_char(p)) ||
			*p == '-' || *p == '_' || *p == '.' || *p == '@'))
			p = g_utf8_next_char(p);
	} else if (*text == '#') {
		while (*p && (g_unichar_isdigit(g_utf8_get_char(p)) ||
								*p == '/')) {
			if (*p == '/')
				is_reply = TRUE;
			p = g_utf8_next_char(p);
		}
	}
	*result = p;
	return is_reply;
}

static gboolean
make_juick_tag(GString *output, const gchar *account_user, gchar **current,
							int *tag_count)
{
	const char *JUICK_TAG =
		"<a href=\"j://q?account=%s&reply=%c&body=%s\">%s</a>";
	gchar *p = *current, *prev = NULL, *msgid, old_char, reply = '\0';
	int result = FALSE;
	gboolean is_reply;

//	purple_debug_info(DBGID, "%s\n", __FUNCTION__);

	prev = p;
	reply = *prev;
	is_reply = get_juick_tag(prev, &p);
	if (is_reply)
		reply = '#';
	if (p == g_utf8_next_char(prev)) {
		g_string_append_unichar(output, g_utf8_get_char(prev));
	} else if (*p) {
		msgid = prev;
		old_char = *p;
		*p = '\0';
		g_string_append_printf(output, JUICK_TAG, account_user, reply,
								msgid, msgid);
		++*tag_count; ++*tag_count;
		*p = old_char;
		if (*p == ':' && reply == '@')
			result = TRUE;
	} else {
		if (reply != '\0')
			g_string_append_printf(output, JUICK_TAG, account_user,
							reply, prev, prev);
		else
			g_string_append(output, prev);
	}
	*current = p;
	return result;
}

static gboolean
juick_on_displaying(PurpleAccount *account, const char *who,
	   char **displaying, PurpleConversation *conv,
	   PurpleMessageFlags flags)
{
	GString *output;
	gchar *p, *prev, *src;
	const gchar *account_user;
	int i = 3, tag_count = 0, tag_max = 92;
	gboolean begin = TRUE, b = FALSE;
	gboolean is_highlighting_tags = purple_prefs_get_bool(
			PREF_IS_HIGHLIGHTING_TAGS);

	purple_debug_info(DBGID, "%s\n", __FUNCTION__);

	if( (!g_str_has_prefix(who, JUICK_JID) &&
		!g_str_has_prefix(who, JUBO_JID)) ||
                (flags & PURPLE_MESSAGE_SYSTEM) )
	{
		return FALSE;
	}

	src = *displaying;
	output = g_string_sized_new(strlen(src));

	account_user = purple_account_get_username(account);
	// now search message text and look for things to highlight
	p = src;
	prev = p;
	while(*p) {
		if (*p == '<')
			tag_count++;
		if (tag_count > tag_max) {
			add_warning_message(output, p, tag_max);
			break;
		}
		if (p != src) {
			prev = g_utf8_prev_char(p);
			b = g_unichar_isspace(g_utf8_get_char(prev)) ||
				*prev == '>' || *prev == '(';
		}
		if(((*p == '@'|| *p == '#') && b) || (*p == '@' && begin))
		{
			if (make_juick_tag(output, account_user, &p,
								&tag_count))
				i = 0;
			else
				i = 3;
			begin = FALSE;
			continue;
		} else if (i == 2 && is_highlighting_tags &&
				g_unichar_isspace(g_utf8_get_char(prev)) &&
								*p == '*')
		{
			highlighting_juick_message_tags(output, &p,&tag_count);
			continue;
		}
		g_string_append_unichar(output, g_utf8_get_char(p));
		p = g_utf8_next_char(p);
		i++;
	}
	free(*displaying);
	*displaying = g_string_free(output, FALSE);
	// purple_debug_error(DBGID, "%s\n", (*displaying));
	return FALSE;
}

static char
*date_reformat(const char *field)
{
	gchar *tmp, *tmp1;
	time_t t;

	if (field == NULL)
		return NULL;

	tmp = g_strdup(field);
	purple_util_chrreplace(tmp, ' ', 'T');
	tmp1 = g_strdup_printf("%sZ", tmp);
	t = purple_str_to_time(tmp, TRUE, NULL, NULL, NULL);

	g_free(tmp); g_free(tmp1);
	return g_strdup(purple_date_format_long(localtime(&t)));
}

static void
body_reformat(GString *output, xmlnode *node, gboolean first)
{
	xmlnode *n, *bodyupn = NULL, *tagn;
	const char *uname, *mid, *rid, *mood, *ts, *replies, *replyto, *attach;
	gchar *body = NULL, *bodyup = NULL, *next = NULL, *tags = NULL,
	      *tag = NULL, *s = NULL, *s1, *tagsmood = NULL, *midrid = NULL,
	      *comment = NULL, *ts_ = NULL, *url = NULL, old_char = '\0';

	purple_debug_info(DBGID, "%s\n", __FUNCTION__);

	g_return_if_fail(node != NULL);
	g_return_if_fail(output != NULL);

	n = xmlnode_get_child(node, "body");
	body = xmlnode_get_data(n);
#if PURPLE_VERSION_CHECK(2, 6, 0)
	// Strangely, in the Help says
	// "xmlnode_get_data Gets (escaped) data from a node".
	// But we need to execute the special function
	// to ensure that the text was actually escaped
	// 2.6 need escape because of xhtml-im
	s = g_markup_escape_text(body, strlen(body));
	g_free(body); body = s;
#endif
	uname = xmlnode_get_attrib(node, "uname");
	mid = xmlnode_get_attrib(node, "mid");
	rid = xmlnode_get_attrib(node, "rid");
	ts = xmlnode_get_attrib(node, "ts");
	if (ts) {
		ts_ = date_reformat(ts);
	}
	replies = xmlnode_get_attrib(node, "replies");
	mood = xmlnode_get_attrib(node, "mood");
	replyto = xmlnode_get_attrib(node, "replyto");
	attach = xmlnode_get_attrib(node, "attach");
	tagn = xmlnode_get_child(node, "tag");
	// purple_debug_info(DBGID, "Make tags\n");
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
	// purple_debug_info(DBGID, "Join tags and mood\n");
	if (tags && mood)
		tagsmood = g_strdup_printf(" %s mood: %s", tags, mood);
	else if (tags)
		tagsmood = g_strdup_printf(" %s", tags);
	else if (mood)
		tagsmood = g_strdup_printf(" mood: %s", mood);
	g_free(tags);
	if (rid && mid)
		midrid = g_strdup_printf("%s/%s", mid, rid);
	else if (mid)
		midrid = g_strdup_printf("%s", mid);
	n = node->parent;
	if (n)
		bodyupn = xmlnode_get_child(n, "body");
	purple_debug_info(DBGID, "Make comment and extra information\n");
	if (bodyupn) {
		bodyup = xmlnode_get_data(bodyupn);
		if (bodyup && replyto)
			comment = g_utf8_strchr(bodyup, strlen(bodyup), '>');
		if (bodyup && *bodyup != '@') {
			if (comment)
				next = g_utf8_next_char(comment);
			else
				next = g_utf8_next_char(bodyup);
			while (g_unichar_isprint(g_utf8_get_char(next)))
				next = g_utf8_next_char(next);
			if (*next) {
				old_char = *next;
				*next = '\0';
			}
		}
	}
	if (attach && mid && !g_str_has_prefix(body, IMAGE_PREFIX))
		url = g_strdup_printf("%s/%s.%s\n", IMAGE_PREFIX,
							mid, attach);
	purple_debug_info(DBGID, "Join all strings\n");
	if (uname && replyto)
		s = g_strconcat("@", uname, _(": reply to "), replyto, NULL);
	else if (uname)
		s = g_strconcat("@", uname, ":", NULL);
	if (comment) {
		s1 = g_strconcat(s, "\n", comment, NULL);
		g_free(s);
		s = s1;
	}
	g_string_append_printf(output, "%s %s%s\n%s%s\n#%s",
			       ts_ ? ts_ : "",
			       s ? s : "",
			       tagsmood ? tagsmood : "",
			       url ? url : "", body,
			       midrid ? midrid : "");
	g_free(s);
	g_free(tagsmood);
	g_free(ts_);
	g_free(body);
	g_free(url);
	purple_debug_info(DBGID, "Add prefix or suffix to the message\n");
	if (bodyup && next && *next == '\0') {
		if (!replyto) {
			s = g_strdup_printf("%s\n", bodyup);
			g_string_prepend(output, s);
			g_free(s);
		}
		*next = old_char;
	}
	g_free(bodyup);
	if (midrid != NULL) {
		purple_util_chrreplace(midrid, '/', '#');
		g_string_append_printf(output, " http://juick.com/%s\n",
								   midrid);
	}
	if (first) {
		if (replies)
			g_string_append_printf(output, _("Replies (%s)\n"),
								  replies);
		if (rid && strcmp(rid, "1") && !replyto)
			g_string_prepend(output,
				_("Continued discussion\n"));
	} else
		g_string_append(output, "\n");
	g_free(midrid);
}

static xmlnode *
make_message(const gchar *from, const gchar *to, const gchar *body)
{
	xmlnode *node, *n;

	node = xmlnode_new("message");
	xmlnode_set_attrib(node, "type", "chat");
	xmlnode_set_attrib(node, "from", from);
	xmlnode_set_attrib(node, "to", to);
	xmlnode_set_attrib(node, "id", "123");

#if PURPLE_VERSION_CHECK(2, 6, 0)
	/* xhtml-im work with gmail.com
	 * and work with 'hide new conversation' */
	gchar *s, *s1;
	xmlnode *n1;
	n = xmlnode_new_child(node, "html");
	xmlnode_set_namespace(n, "http://jabber.org/protocol/xhtml-im");
	s1 = purple_strreplace(body, "\n", "<br/>");
	s = g_strconcat("<body>", s1, "</body>", NULL);
	n1 = xmlnode_from_str(s, -1);
	g_free(s); g_free(s1);
	xmlnode_insert_child(n, n1);
#else
	/* xhtml-im don't work with 'hide new conversation' in pidgin 2.4, 2.5
	 * jabber:client don't good work with gmail.com,
	 * but work with 2.4, 2.5 pidgin */
	n = xmlnode_new_child(node, "body");
	xmlnode_set_namespace(n, "jabber:client");
	xmlnode_insert_data(n, body, -1);
#endif

	return node;
}

static void
xmlnode_received_cb(PurpleConnection *gc, xmlnode **packet)
{
	xmlnode *node;
	const char *from;
	gchar *s = NULL;
	GString *output = NULL;
	gboolean first = TRUE;

	purple_debug_info(DBGID, "%s\n", __FUNCTION__);

	if (!*packet)
		return;

	node = xmlnode_get_child(*packet, "query");
	if (node) {
		node = xmlnode_get_child(node, "juick");
	} else
		node = xmlnode_get_child(*packet, "juick");

	from = xmlnode_get_attrib(*packet, "from");

	output = g_string_new("");
	while (node) {
		body_reformat(output, node, first);
		node = xmlnode_get_next_twin(node);
		first = FALSE;
	}
	if (output->len != 0) {
		s = g_string_free(output, FALSE);
		node = make_message(from,
				xmlnode_get_attrib(*packet, "to"), s);
		g_free(s);
		xmlnode_free(*packet);
		*packet = node;
	} else {
		g_string_free(output, TRUE);
		node = xmlnode_get_child(*packet, "error");
		if (node && from &&
			(!strcmp(from, JUICK_JID) ||
			 !strcmp(from, JUBO_JID))) {
			s = g_strdup_printf("error %s", xmlnode_get_attrib(node,
								       "code"));
			node = make_message(from,
					xmlnode_get_attrib(*packet, "to"), s);
			g_free(s);
			xmlnode_free(*packet);
			*packet = node;
		}
	}
}

typedef enum
{
	IQ_POST,
	IQ_POST_REPLIES,
	IQ_NEW_MESSAGES,
	IQ_FRIENDS,
	IQ_SUBSCRIBERS
} IqStatus;

static void
send_iq(PurpleConnection *gc, const gchar *msgid, IqStatus iq_status)
{
#if PURPLE_VERSION_CHECK(2, 6, 0)
	xmlnode *iq, *query;

	iq = xmlnode_new("iq");
	xmlnode_set_attrib(iq, "type", "get");
	xmlnode_set_attrib(iq, "to", JUICK_JID);
	xmlnode_set_attrib(iq, "id", "123");

	query = xmlnode_new_child(iq, "query");
	switch(iq_status) {
		case IQ_POST:
			xmlnode_set_namespace(query, NS_JUICK_MESSAGES);
			if (msgid)
				xmlnode_set_attrib(query, "mid", msgid);
			break;
		case IQ_POST_REPLIES:
			xmlnode_set_namespace(query, NS_JUICK_MESSAGES);
			if (msgid) {
				xmlnode_set_attrib(query, "mid", msgid);
				xmlnode_set_attrib(query, "rid", "*");
			}
			break;
		case IQ_NEW_MESSAGES:
			xmlnode_set_namespace(query, NS_JUICK_MESSAGES);
			break;
		case IQ_FRIENDS: case IQ_SUBSCRIBERS:
			xmlnode_set_namespace(query, NS_JUICK_USERS);
			break;
	}
	purple_signal_emit(purple_connection_get_prpl(gc),
		"jabber-sending-xmlnode", gc, &iq);
	if (iq != NULL)
		xmlnode_free(iq);
#else
	PurplePluginProtocolInfo *prpl_info = NULL;
	const char *STANZA = "<iq to='%s' id='123' type='get'>" \
			      "<query xmlns='%s' %%s/></iq>";
	gchar *text = NULL, *s = NULL, *s1 = NULL;

	if (gc)
		prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl);

	if (!prpl_info || prpl_info->send_raw == NULL)
		return;
	switch(iq_status) {
		case IQ_POST:
			if (!msgid)
				return;
			s = g_strdup_printf(STANZA, JUICK_JID, NS_JUICK_MESSAGES);
			s1 = g_strdup_printf(" mid='%s' ", msgid);
			text = g_strdup_printf(s, s1);
			break;
		case IQ_POST_REPLIES:
			if (!msgid)
				return;
			s = g_strdup_printf(STANZA, JUICK_JID, NS_JUICK_MESSAGES);
			s1 = g_strdup_printf(" mid='%s' rid='*' ", msgid);
			text = g_strdup_printf(s, s1);
			break;
		case IQ_NEW_MESSAGES:
			s = g_strdup_printf(STANZA, JUICK_JID, NS_JUICK_MESSAGES);
			text = g_strdup_printf(s, " ");
			break;
		case IQ_FRIENDS: case IQ_SUBSCRIBERS:
			text = g_strdup_printf(STANZA, JUICK_JID, NS_JUICK_USERS);
			break;
	}
	prpl_info->send_raw(gc, text, strlen(text));
	g_free(text);
	g_free(s); g_free(s1);
#endif
}

static gchar *
str_post_add_prefix(const gchar *prefix, const gchar *str)
{
	gchar *s, *s1;
	if ((s = strchr(str, '/')) == NULL)
		return g_strconcat(prefix, str, NULL);
	else {
		s = g_strndup(str, s - str);
		s1 = g_strconcat(prefix, s, NULL);
		g_free(s);
		return s1;
	}
}

static void
change_text_in_buffer(GtkTextBuffer *buffer, const gchar* text,
							gboolean at_start)
{
	GtkTextIter start, end, iter;
	gchar *str, *p, *first;
	int i = 0;

	gtk_text_buffer_get_bounds(buffer, &start, &end);
	str = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
	if (str == NULL) {
		gtk_text_buffer_insert_at_cursor(buffer, text, -1);
		return;
	}
	if (!at_start) {
		gtk_text_buffer_get_iter_at_mark(buffer, &iter,
					gtk_text_buffer_get_insert(buffer));
		i = gtk_text_iter_get_offset(&iter);
	}
	p = str + i;
	if (*p != '#' && *p != '@' && i != 0)
		p = g_utf8_prev_char(p);
	while(p != str) {
		if (!g_unichar_isdigit(g_utf8_get_char(p)) && *p != '/')
			break;
		p = g_utf8_prev_char(p);
	}

	first = p;
	get_juick_tag(first, &p);

	if (p == NULL || p == g_utf8_next_char(first)) {
		gtk_text_buffer_insert_at_cursor(buffer, text, -1);
	} else {
		gtk_text_buffer_get_iter_at_offset(buffer, &start,
					g_utf8_pointer_to_offset(str, first));
		gtk_text_buffer_get_iter_at_offset(buffer, &end,
					g_utf8_pointer_to_offset(str, p));
		gtk_text_buffer_delete(buffer, &start, &end);
		gtk_text_buffer_insert(buffer, &start, text, strlen(text) - 1);
	}
}

static void
send_link(PurpleConversation *conv, const gchar *send, const gchar *body,
							const gchar reply)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	PurpleConvIm *convim = PURPLE_CONV_IM(conv);
	PurpleConnection *gc = purple_conversation_get_gc(
			PIDGIN_CONVERSATION(conv)->active_conv);
	gboolean is_insert_only = purple_prefs_get_bool(PREF_IS_INSERT_ONLY);
	gchar *s = NULL, *s1, *text = NULL;

	text = g_strconcat(body, " ", NULL);

	if ((send && !strcmp(send, "'i'")) || (is_insert_only && !send)) {
		change_text_in_buffer(gtkconv->entry_buffer, text, TRUE);
		g_free(text);
		gtk_widget_grab_focus(GTK_WIDGET(gtkconv->entry));
		return;
	}
	if (send && !strcmp(send, "'ir'")) {
		change_text_in_buffer(gtkconv->entry_buffer, text, FALSE);
		g_free(text);
		gtk_widget_grab_focus(GTK_WIDGET(gtkconv->entry));
		return;
	}
	if (send && !strcmp(send, "'rec'")) {
		s = str_post_add_prefix("! ", body);
		purple_conv_im_send(convim, s);
		g_free(s);
		g_free(text);
		gtk_widget_grab_focus(GTK_WIDGET(gtkconv->entry));
		return;
	}
	if (send && !strcmp(send, "'sub'")) {
		s = str_post_add_prefix("S ", body);
		purple_conv_im_send(convim, s);
		g_free(s);
		g_free(text);
		return;
	}
	if (send && !strcmp(send, "'usub'")) {
		s = str_post_add_prefix("U ", body);
		purple_conv_im_send(convim, s);
		g_free(s);
		g_free(text);
		return;
	}
	if (send && !strcmp(send, "'web'")) {
		s = purple_strreplace(body, "/", "#");
		s1 = g_strconcat(WEBPAGE, s + 1, NULL);
		purple_notify_uri(NULL, s1);
		g_free(s); g_free(s1);
		g_free(text);
		return;
	}
	if (reply == '#') {
		if ((send && !strcmp(send, "'s'")) ||
						(!is_insert_only && !send)) {
			send_iq(gc, body + 1, IQ_POST);
			send_iq(gc, body + 1, IQ_POST_REPLIES);
			if (send)
				change_text_in_buffer(gtkconv->entry_buffer,
								text, TRUE);
			else
				gtk_text_buffer_set_text(
						gtkconv->entry_buffer, text,-1);
		}
	} else if (reply == '@') {
		s = g_strdup_printf("%s+", body);
		if (send && !strcmp(send, "'ui'")) {
			purple_conv_im_send(convim, body);
		} else if (send && !strcmp(send, "'up'")) {
			purple_conv_im_send(convim, s);
		} else if ((send && !strcmp(send, "'s'")) ||
							!is_insert_only) {
			purple_conv_im_send(convim, body);
			purple_conv_im_send(convim, s);
		}
		g_free(s);
	} else
		gtk_text_buffer_insert_at_cursor(
				gtkconv->entry_buffer, text, -1);
	gtk_widget_grab_focus(GTK_WIDGET(gtkconv->entry));
	g_free(text);
}

static gboolean
juick_uri_handler(const char *proto, const char *cmd, GHashTable *params)
{
	PurpleAccount *account = NULL;
	PurpleConversation *conv = NULL;
	gchar *body = NULL, *account_user = NULL, *reply = NULL, *send = NULL;

	purple_debug_info(DBGID, "%s %s\n", __FUNCTION__, proto);

	if (g_ascii_strcasecmp(proto, "j") == 0) {
		body = g_hash_table_lookup(params, "body");
		account_user = g_hash_table_lookup(params, "account");
		reply = g_hash_table_lookup(params, "reply");
		send = g_hash_table_lookup(params, "send");
		account = purple_accounts_find(account_user, "prpl-jabber");
		if (body && account) {
			conv = purple_conversation_new(
				PURPLE_CONV_TYPE_IM, account, JUICK_JID);
			if (purple_prefs_get_bool(PREF_IS_SHOW_JUICK))
				// don't work in pidgin 2.4, 2.5
				purple_conversation_present(conv);
			send_link(conv, send, body, reply[0]);
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean
window_keypress_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	PurpleConversation *conv = (PurpleConversation *)data;
	PurpleConvIm *convim;
	PurpleConnection *gc;

	if (!g_str_has_prefix(conv->name, JUICK_JID))
		return FALSE;

	convim = PURPLE_CONV_IM(conv);
	gc = purple_conversation_get_gc(
			PIDGIN_CONVERSATION(conv)->active_conv);

	if (event->state & GDK_CONTROL_MASK)
		switch (event->keyval) {
			case GDK_3:
				purple_conv_im_send(convim, "#");
				break;
			case GDK_2:
				send_iq(gc, NULL, IQ_NEW_MESSAGES);
				break;
		}
	return FALSE;
}

static void
attach_to_conversation(gpointer data, gpointer user_data)
{
	PurpleConversation *conv = (PurpleConversation *) data;
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	gulong handler_id;

	if (!g_str_has_prefix(conv->name, JUICK_JID))
		return;

	handler_id = g_signal_connect(G_OBJECT(gtkconv->entry),
		"key_press_event", G_CALLBACK(window_keypress_cb), conv);
	g_hash_table_insert(ht_signal_handlers, gtkconv->entry,
						    (gpointer) handler_id);
	handler_id = g_signal_connect(G_OBJECT(gtkconv->imhtml),
		"key_press_event", G_CALLBACK(window_keypress_cb), conv);
	g_hash_table_insert(ht_signal_handlers, gtkconv->imhtml,
						    (gpointer) handler_id);
}

static void
detach_from_conversation(gpointer data, gpointer user_data)
{
	PurpleConversation *conv = (PurpleConversation *) data;
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	gulong handler_id;

	if (!g_str_has_prefix(conv->name, JUICK_JID))
		return;

	handler_id = (gulong) g_hash_table_lookup(ht_signal_handlers,
							gtkconv->entry);
	g_signal_handler_disconnect(gtkconv->entry, handler_id);
	g_hash_table_remove(ht_signal_handlers, gtkconv->entry);
	handler_id = (gulong) g_hash_table_lookup(ht_signal_handlers,
							gtkconv->imhtml);
	g_signal_handler_disconnect(gtkconv->imhtml, handler_id);
	g_hash_table_remove(ht_signal_handlers, gtkconv->imhtml);
}

static void
conversation_created_cb(PurpleConversation *conv)
{
	attach_to_conversation(conv, NULL);
}

static void
deleting_conversation_cb(PurpleConversation *conv)
{
	detach_from_conversation(conv, NULL);
}

#if PURPLE_VERSION_CHECK(2, 6, 0)
static gchar *
replace_or_insert(const gchar *string, const gchar *delimiter,
						const gchar *replacement)
{
	int i = 0;
	gchar *result = NULL, *s1 = NULL;
	gchar **s = g_strsplit(string, delimiter, 2);

	while (s[i] != NULL){
		if (i > 2)
			break;
		i++;
	}
	if (i == 2) {
		s1 = s[1] + 1;
		if (s1 && *s1 != '\0')
			s1 = strchr(s1, *s[1]) + 1;
		if (s1 && s[0])
			result = g_strconcat(s[0], delimiter, replacement,
								s1, NULL);
	} else if (i == 1) {
		result = g_strconcat(s[0], delimiter, replacement, NULL);
	}
	g_strfreev(s);
	return result;
}

typedef enum {
	JUICK_LINK_SHOW,
	JUICK_LINK_INSERT,
	JUICK_LINK_INSERT_WITH_REPLACE,
	JUICK_LINK_USER_INFO,
	JUICK_LINK_USER_POSTS,
	JUICK_LINK_SUBSCRIBE,
	JUICK_LINK_UNSUBSCRIBE,
	JUICK_LINK_WEBPAGE,
	JUICK_LINK_RECOMMEND
} JuickLinkStatus;

static void
process_link(const gchar *url, JuickLinkStatus status)
{
	gchar *murl = NULL;
	const gchar *send = "&send=";
//        const gchar *url = gtk_imhtml_link_get_url(link);

	purple_debug_info(DBGID, "%s, %d\n", __FUNCTION__, status);

	switch(status){
		case JUICK_LINK_SHOW:
			murl = replace_or_insert(url, send, "'s'");
			break;
		case JUICK_LINK_INSERT:
			murl = replace_or_insert(url, send, "'i'");
			break;
		case JUICK_LINK_INSERT_WITH_REPLACE:
			murl = replace_or_insert(url, send, "'ir'");
			break;
		case JUICK_LINK_USER_INFO:
			murl = replace_or_insert(url, send, "'ui'");
			break;
		case JUICK_LINK_USER_POSTS:
			murl = replace_or_insert(url, send, "'up'");
			break;
		case JUICK_LINK_SUBSCRIBE:
			murl = replace_or_insert(url, send, "'sub'");
			break;
		case JUICK_LINK_UNSUBSCRIBE:
			murl = replace_or_insert(url, send, "'usub'");
			break;
		case JUICK_LINK_WEBPAGE:
			murl = replace_or_insert(url, send, "'web'");
			break;
		case JUICK_LINK_RECOMMEND:
			murl = replace_or_insert(url, send, "'rec'");
			break;
	}
	if (murl != NULL)
		purple_got_protocol_handler_uri(murl);
	g_free(murl);
}

static void
menu_show_activate_cb(GtkMenuItem *menuitem, gchar *link)
{
	process_link(link, JUICK_LINK_SHOW);
}

static void
menu_insert_activate_cb(GtkMenuItem *menuitem, gchar *link)
{
	process_link(link, JUICK_LINK_INSERT);
}

static void
menu_insert_replace_activate_cb(GtkMenuItem *menuitem, gchar *link)
{
	process_link(link, JUICK_LINK_INSERT_WITH_REPLACE);
}

static void
menu_user_info_activate_cb(GtkMenuItem *menuitem, gchar *link)
{
	process_link(link, JUICK_LINK_USER_INFO);
}

static void
menu_user_posts_activate_cb(GtkMenuItem *menuitem, gchar *link)
{
	process_link(link, JUICK_LINK_USER_POSTS);
}

static void
menu_subscribe_activate_cb(GtkMenuItem *menuitem, gchar *link)
{
	process_link(link, JUICK_LINK_SUBSCRIBE);
}

static void
menu_unsubscribe_activate_cb(GtkMenuItem *menuitem, gchar *link)
{
	process_link(link, JUICK_LINK_UNSUBSCRIBE);
}

static void
menu_webpage_activate_cb(GtkMenuItem *menuitem, gchar *link)
{
	process_link(link, JUICK_LINK_WEBPAGE);
}

static void
menu_recommend_activate_cb(GtkMenuItem *menuitem, gchar *link)
{
	process_link(link, JUICK_LINK_RECOMMEND);
}

static gchar *
get_part_of_uri(const gchar *string, const gchar *delimiter)
{
	gchar *s, *s1;
	s = strstr(string, delimiter);
	s1 = strstr(s + 1, "&");
	if (s1)
		return g_strndup(s, s1 - s + strlen(delimiter));
	else
		return g_strdup(s + strlen(delimiter));

}

static gboolean
juick_context_menu(GtkWidget *menu, const gchar *url, const gchar *text)
{
	GtkWidget *item, *item_i = NULL, *item_s = NULL, *img;
	gboolean result = FALSE;
	gboolean is_insert_only = purple_prefs_get_bool(PREF_IS_INSERT_ONLY);
	gchar *s;

	img = gtk_image_new_from_stock(GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_MENU);
	if (g_strrstr(url, "&reply=@")) {
		item_s = gtk_image_menu_item_new_with_mnemonic(
						_("_See user info and posts"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_s);
		g_signal_connect(G_OBJECT(item_s), "activate",
			G_CALLBACK(menu_show_activate_cb), (gpointer)url);
		s = g_strdup_printf(_("_Insert/replace '%s ' at start"), text);
		item_i = gtk_image_menu_item_new_with_mnemonic(s);
		g_free(s);
		if (is_insert_only)
			gtk_image_menu_item_set_image(
					GTK_IMAGE_MENU_ITEM(item_i), img);
		else
			gtk_image_menu_item_set_image(
					GTK_IMAGE_MENU_ITEM(item_s), img);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_i);
		g_signal_connect(G_OBJECT(item_i), "activate",
			G_CALLBACK(menu_insert_activate_cb), (gpointer)url);

		s = g_strdup_printf(_("Insert/replace '%s ' at _cursor"), text);
		item = gtk_menu_item_new_with_mnemonic(s);
		g_free(s);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(menu_insert_replace_activate_cb),
								(gpointer)url);
		item = gtk_menu_item_new_with_mnemonic(_("See user _webpage"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
			G_CALLBACK(menu_webpage_activate_cb), (gpointer)url);
		item = gtk_menu_item_new_with_mnemonic(_("See user _info"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
			G_CALLBACK(menu_user_info_activate_cb), (gpointer)url);
		item = gtk_menu_item_new_with_mnemonic(_("See user _posts"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
			G_CALLBACK(menu_user_posts_activate_cb), (gpointer)url);
		s = g_strdup_printf(_("Subscribe _to %s"), text + 1);
		item = gtk_menu_item_new_with_mnemonic(s);
		g_free(s);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
			G_CALLBACK(menu_subscribe_activate_cb), (gpointer)url);
		s = g_strdup_printf(_("Unsubscribe _from %s"), text + 1);
		item = gtk_menu_item_new_with_mnemonic(s);
		g_free(s);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
			G_CALLBACK(menu_unsubscribe_activate_cb), (gpointer)url);
		result = TRUE;
	} else if (g_strrstr(url, "&reply=#")) {
		item_s = gtk_image_menu_item_new_with_mnemonic(
						_("_See replies to post"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_s);
		g_signal_connect(G_OBJECT(item_s), "activate",
			G_CALLBACK(menu_show_activate_cb),(gpointer)url);
		s = g_strdup_printf(_("_Insert/replace '%s ' at start"), text);
		item_i = gtk_image_menu_item_new_with_mnemonic(s);
		g_free(s);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_i);
		g_signal_connect(G_OBJECT(item_i), "activate",
			G_CALLBACK(menu_insert_activate_cb), (gpointer)url);
		if (is_insert_only)
			gtk_image_menu_item_set_image(
					GTK_IMAGE_MENU_ITEM(item_i), img);
		else
			gtk_image_menu_item_set_image(
					GTK_IMAGE_MENU_ITEM(item_s), img);
		s = g_strdup_printf(_("Insert/replace '%s ' at _cursor"), text);
		item = gtk_menu_item_new_with_mnemonic(s);
		g_free(s);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(menu_insert_replace_activate_cb),
								(gpointer)url);
		item = gtk_menu_item_new_with_mnemonic(_("See post _webpage"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
			G_CALLBACK(menu_webpage_activate_cb), (gpointer)url);
		item = gtk_menu_item_new_with_mnemonic(
						_("Subscribe _to this post"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
			G_CALLBACK(menu_subscribe_activate_cb), (gpointer)url);
		item = gtk_menu_item_new_with_mnemonic(
					_("Unsubscribe _from this post"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
			G_CALLBACK(menu_unsubscribe_activate_cb), (gpointer)url);
		item = gtk_menu_item_new_with_mnemonic(_("R_ecommend this post"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
			G_CALLBACK(menu_recommend_activate_cb), (gpointer)url);
		result = TRUE;
	}
	return result;
}

static gboolean
juick_url_clicked_cb(GtkIMHtml * imhtml, GtkIMHtmlLink * link)
{
        const gchar * url = gtk_imhtml_link_get_url(link);

        purple_debug_info(DBGID, "%s called\n", __FUNCTION__);
        purple_debug_info(DBGID, "url = %s\n", url);

        purple_got_protocol_handler_uri(url);

        return TRUE;
}

static gboolean
context_menu(GtkIMHtml *imhtml, GtkIMHtmlLink *link, GtkWidget *menu)
{
        const gchar *url = gtk_imhtml_link_get_url(link);
	const gchar *body = "&body=";
	gchar *text;
	gboolean result = FALSE;

        purple_debug_info(DBGID, "%s called\n", __FUNCTION__);

	text = get_part_of_uri(url, body);

	result = juick_context_menu(menu, url, text);

	g_free(text);
        return result;
}
#else
static PurpleNotifyUiOps juick_ops;
static void *(*saved_notify_uri)(const char *uri);

static void *juick_notify_uri(const char *uri) {
	void *retval = NULL;

	purple_debug_info(DBGID, "Go url %s\n", uri);

	if(strncmp(uri, "j:", 2) == 0) {
		purple_got_protocol_handler_uri(uri);
	} else {
		retval = saved_notify_uri(uri);
	}
	return retval;
}
#endif

static PurplePluginPrefFrame*
get_plugin_pref_frame(PurplePlugin *plugin)
{
        PurplePluginPrefFrame *frame;
        PurplePluginPref *ppref;

        frame = purple_plugin_pref_frame_new();

        ppref = purple_plugin_pref_new_with_name_and_label(
		PREF_IS_HIGHLIGHTING_TAGS, (
					_("Greyed out tags in the message")));
        purple_plugin_pref_frame_add(frame, ppref);
        ppref = purple_plugin_pref_new_with_name_and_label(
						PREF_IS_SHOW_MAX_MESSAGE,
                                            (_("Show max warning message")));
        purple_plugin_pref_frame_add(frame, ppref);
#if PURPLE_VERSION_CHECK(2, 6, 0)
        ppref = purple_plugin_pref_new_with_name_and_label(
		PREF_IS_SHOW_JUICK, (_("Show Juick conversation when click " \
					"on juick tag in other conversation")));
        purple_plugin_pref_frame_add(frame, ppref);
#endif
        ppref = purple_plugin_pref_new_with_name_and_label(
		PREF_IS_INSERT_ONLY, (_("Insert when left click, don't show")));
        purple_plugin_pref_frame_add(frame, ppref);

	return frame;
}

static gboolean
plugin_load(PurplePlugin *plugin)
{

	void *jabber_handle = purple_plugins_find_with_id("prpl-jabber");
	void *conv_handle = purple_conversations_get_handle();

#if PURPLE_VERSION_CHECK(2, 6, 0)
	gtk_imhtml_class_register_protocol("j://", juick_url_clicked_cb,
							    context_menu);
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
				"displaying-im-msg", plugin,
				PURPLE_CALLBACK(juick_on_displaying), NULL);

	purple_signal_connect(jabber_handle, "jabber-receiving-xmlnode", plugin,
				PURPLE_CALLBACK(xmlnode_received_cb), NULL);

	/* Create the hash table for signal handlers. */
	ht_signal_handlers = g_hash_table_new(g_direct_hash, g_direct_equal);

	/* Attach to current conversations. */
	g_list_foreach(purple_get_conversations(), attach_to_conversation,
									NULL);

	/* Connect signals for future conversations. */
	purple_signal_connect(conv_handle, "conversation-created", plugin,
				PURPLE_CALLBACK(conversation_created_cb), NULL);
	purple_signal_connect(conv_handle, "deleting-conversation", plugin,
			PURPLE_CALLBACK(deleting_conversation_cb), NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	void *jabber_handle = purple_plugins_find_with_id("prpl-jabber");
	void *conv_handle = purple_conversations_get_handle();

	/* Disconnect signals for future conversations. */
	purple_signal_disconnect(conv_handle, "conversation-created", plugin,
				PURPLE_CALLBACK(conversation_created_cb));
	purple_signal_disconnect(conv_handle, "deleting-conversation", plugin,
				PURPLE_CALLBACK(deleting_conversation_cb));

	/* Detach from current conversations. */
	g_list_foreach(purple_get_conversations(), detach_from_conversation,
									NULL);
	/* Destroy the hash table for signal handlets. */
	g_hash_table_destroy(ht_signal_handlers);

	purple_signal_disconnect(jabber_handle, "jabber-receiving-xmlnode",
			plugin, PURPLE_CALLBACK(xmlnode_received_cb));

	purple_signal_disconnect(pidgin_conversations_get_handle(),
			"displaying-im-msg", plugin,
			PURPLE_CALLBACK(juick_on_displaying));

	purple_signal_disconnect(purple_get_core(), "uri-handler", plugin,
                                      PURPLE_CALLBACK(juick_uri_handler));
#if PURPLE_VERSION_CHECK(2, 6, 0)
	gtk_imhtml_class_register_protocol("j://", NULL, NULL);
#else
	juick_ops.notify_uri = saved_notify_uri;
	purple_notify_set_ui_ops(&juick_ops);
#endif
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
	PURPLE_MAJOR_VERSION,                             /**< major version */
	PURPLE_MINOR_VERSION,                             /**< minor version */
	PURPLE_PLUGIN_STANDARD,                           /**< type */
	PIDGIN_PLUGIN_TYPE,                               /**< ui_requirement */
	0,                                                /**< flags */
	NULL,                                             /**< dependencies */
	PURPLE_PRIORITY_DEFAULT,                          /**< priority */

	"gtkjuick",                                       /**< id */
	N_("Juick"),                                          /**< name */
	"0.3",                                            /**< version */
	N_("Adds some color and button for juick bot."),  /**< summary */
	N_("Adds some color and button for juick bot.\n" \
		"Unfortunately pidgin developers have decided that more than " \
		"100 tags may not be necessary when " \
		"displaying the message"),                /**< description */
	"owner.mad.epa@gmail.com, pktfag@gmail.com",      /**< author */
	"http://github.com/mad/pidgin-juick-plugin",      /**< homepage */
	plugin_load,                                      /**< load */
	plugin_unload,                                    /**< unload */
	NULL,                                             /**< destroy */
	NULL,                                             /**< ui_info */
	NULL,                                             /**< extra_info */
	&prefs_info,                                      /**< prefs_info */
	NULL,                                             /**< actions */
							  /**< padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
#if ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */
	/* we have to translate it here as we are in a different textdomain */
	plugin->info->name = _(plugin->info->name);
	plugin->info->summary = _(plugin->info->summary);
	plugin->info->description = _(plugin->info->description);

	purple_prefs_add_none(PREF_PREFIX);
	purple_prefs_add_bool(PREF_IS_HIGHLIGHTING_TAGS, FALSE);
	purple_prefs_add_bool(PREF_IS_SHOW_MAX_MESSAGE, TRUE);
#if PURPLE_VERSION_CHECK(2, 6, 0)
	purple_prefs_add_bool(PREF_IS_SHOW_JUICK, TRUE);
	purple_prefs_add_bool(PREF_IS_INSERT_ONLY, FALSE);
#else
	purple_prefs_add_bool(PREF_IS_INSERT_ONLY, TRUE);
#endif
}

PURPLE_INIT_PLUGIN(juick, init_plugin, info)


// Local Variables:
// c-basic-offset: 8
// indent-tabs-mode: t
// End:
