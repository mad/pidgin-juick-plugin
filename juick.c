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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>

#include <glib.h>

// fetch_url
#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#include <internal.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif
#define MAX_LEN 1024

// pidgin
#include <plugin.h>
#include <imgstore.h>
#include <account.h>
#include <core.h>
#include <debug.h>
#include <gtkconv.h>
#include <util.h>
#include <version.h>
#include <gtkplugin.h>
#include <gtkimhtml.h>
#include <gtkutils.h>
#include <gtknotify.h>
#include <conversation.h>

#define MAX_PATH 256
#define DBGID "juick"

#define PREF_PREFIX "/plugins/gtk/juick-plugin"
#define PREF_USE_AVATAR PREF_PREFIX "/avatar-p"
#define PREF_USE_ID_PLUS PREF_PREFIX "/id_plus"

struct _PurpleStoredImage
{
  int id;
  guint8 refcount;
  size_t size;		/**< The image data's size.	*/
  char *filename;	/**< The filename (for the UI)	*/
  gpointer data;	/**< The image data.		*/
};

struct _JuickAvatar
{
  int id;
  char *name;
};

typedef struct _JuickAvatar JuickAvatar;

static gchar *juick_avatar_dir;
static const char *juick_jid = "juick@juick.com";
int id_last_reply = 0;
char *global_account_id = NULL;

static GHashTable *avatar_store;

static int juick_smile_add_fake(char *);

static void disconnect_prefs_callbacks(GtkObject *, gpointer );
static void toggle_avatar(GtkWidget *, gpointer );
static void toggle_id(GtkWidget *, gpointer );
static GtkWidget * get_config_frame(PurplePlugin *);

static gchar* juick_avatar_url_extract(const gchar *);
static gchar* juick_make_avatar_dir();
static void juick_avatar_init();
static gboolean juick_avatar_exist_p(gchar *);
static void juick_download_avatar(gchar *);
static gchar *fetch_url(const gchar *, const gchar *, int *);

static gboolean
markup_msg(PurpleAccount *account, const char *who, char **displaying,
           PurpleConversation *conv, PurpleMessageFlags flags)
{
  char *t, *ttmp;
  char *startnew, *new;
  char *start, *end;
  char imgbuf[64];
  int i = 0, tag_num = 0, use_avatar, use_id_plus,
    tag_max = 0;
  char maybe_path[MAX_PATH];
  JuickAvatar *javatar, *tmpavatar;

  if(!strstr(who, juick_jid))
    return FALSE;

  global_account_id = purple_account_get_username(conv->account);
  t = purple_markup_strip_html(*displaying);
  ttmp = t;

  new = (char *) malloc(strlen(t) * 20);
  memset(new, 0, strlen(t) * 20);
  startnew = new;
  purple_debug_info(DBGID, "starting markup message\n");

  use_avatar = purple_prefs_get_int(PREF_USE_AVATAR);
  use_id_plus = purple_prefs_get_int(PREF_USE_ID_PLUS);

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
	if(use_id_plus) {
	  strcat(new, "+\">");
	} else {
	  strcat(new, " \">");
	}
        strncat(new, start, end - start);
        strcat(new, "</A>");
	i += 25 + (end - start)*2;
	tag_max += 2;
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
	// display avatar only by owner message
	if (use_avatar && *t == ':') {
	  javatar = g_new(JuickAvatar, 1);
	  javatar->name = (char *) malloc(20);
	  strncpy(javatar->name, start + 1, end - start - 1);
	  javatar->name[end - start - 1] = 0;
	  tmpavatar = g_hash_table_lookup(avatar_store, javatar->name);
	  if (!tmpavatar) {
	    juick_download_avatar(javatar->name);
	    sprintf(maybe_path, "%s/%s.png", juick_avatar_dir, javatar->name);
	    purple_debug_info(DBGID, "path to image %s\n", maybe_path);
	    javatar->id = juick_smile_add_fake(maybe_path);
	    g_hash_table_insert(avatar_store, javatar->name, javatar);
	  } else {
	    javatar = tmpavatar;
	  }
	  g_snprintf(imgbuf, sizeof(imgbuf), "<img id=\"%d\"> ", javatar->id);
	  purple_debug_info(DBGID, "add avatar %s\n", imgbuf);
	  strcat(new, imgbuf);
	  i += strlen(imgbuf);
	  tag_max += 1;
	}
        strcat(new, "<A HREF=\"j:q?body=");
        strncat(new, start, end - start);
        strcat(new, "\">");
        strncat(new, start, end - start);
        strcat(new, "</A>");
        i += 24 + (end - start)*2;
	tag_max += 2;
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
	  tag_max += 4;
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
	tag_max += 2;
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
	tag_max += 2;
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
	tag_max += 2;
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

  if (tag_max < 100) {
    t = *displaying;
    new = startnew;
    *displaying = g_strdup_printf("%s", new);
    free(new);
    g_free(ttmp);
    g_free(t);
  } else {
    g_free(ttmp);
    free(new);
  }

  return FALSE;
}

static void
juick_download_avatar(char *uname)
{
  FILE *fp;
  gchar img_path[MAX_PATH];

  gchar avatar_url[64];
  gchar *avatar_id;
  gchar *result;
  int len;

  purple_debug_info(DBGID, "checking %s\n", uname);
  if (!juick_avatar_exist_p(uname)) {
    purple_debug_info(DBGID, "trying download %s\n", uname);
    sprintf(avatar_url, "%s/", uname);
    result = fetch_url("juick.com", avatar_url, &len);
    avatar_id = juick_avatar_url_extract(result);

    g_free(result);

    if (avatar_id) {
      purple_debug_info(DBGID, "founding avatar id %s\n", avatar_id);
      sprintf(avatar_url, "as/%s", avatar_id);
      result = fetch_url("i.juick.com", avatar_url, &len);
      sprintf(img_path, "%s/%s.png", juick_avatar_dir, uname);

      purple_debug_info(DBGID, "saving avatar to %s (%d bytes)\n",
			img_path, len);
      fp = fopen(img_path, "wb");
      fwrite(result, len, 1, fp);
      fclose(fp);

      g_free(result);
      g_free(avatar_id);
    }
  }
}

static gchar *
juick_avatar_url_extract(const gchar *body)
{
  gchar *q, *p;

  p = strstr(body, "http://i.juick.com/a/");

  if (p && ((q = strchr(p, '"')) != NULL)) {
    p = strstr(p, "a/");
    p += 2;
    return g_strndup(p, q - p);
  }
  return NULL;
}

static gboolean
juick_avatar_exist_p(char *uname)
{
  struct stat sb;
  char avatar_dir[MAX_PATH];

  sprintf(avatar_dir, "%s/%s.png", juick_avatar_dir, uname);

  if (stat(avatar_dir, &sb) == -1)
    return FALSE;
  return TRUE;
}

static int
juick_smile_add_fake(char *filename)
{
  char *filedata;
  size_t size;
  int id;
  GError *error = NULL;
  PurpleStoredImage *img;

  if (!g_file_get_contents(filename, &filedata, &size, &error)) {
    purple_debug_info(DBGID, "g_file_get error\n");
    g_error_free(error);

    return 0;
  }

  id = purple_imgstore_add_with_id(filedata, size, filename);
  img = purple_imgstore_find_by_id(id);

  return id;
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
      purple_debug_misc(DBGID, "Juick-plugin: mem last reply %d\n",
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
    purple_debug_misc(DBGID, "Juick-plugin: last reply button %d\n",
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

  last_reply_button = gtk_button_new_with_label("last reply");
  last_button = gtk_button_new_with_label("last msg");

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
      purple_debug_misc(DBGID, "Juick-plugin: \\C-s pressed\n");
      serv_send_im(connection, juick_jid, "#", PURPLE_MESSAGE_SEND);
      break;
    case 'r':
    case 'R':
      // this cyrrilic 'К' 'к'
    case 1739:
    case 1771:
      purple_debug_misc(DBGID, "Juick-plugin: \\C-r pressed\n");
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

static gchar *fetch_url(const gchar *host, const gchar *url, int *res_len)
{
  //struct hostent *phe;

  struct sockaddr_in sin;
  int s, len, len_respose;
  gchar *body_send;
  gchar *body_recv;
  gchar *body_response;

  s = socket(AF_INET, SOCK_STREAM, 0);
  if(s == -1) {
    perror("socket");
    return NULL;
  }

  // XXX: a little bit faster
  /*
   * phe = gethostbyname(host);
   * if(phe == NULL) {
   *   perror("gethostbyname");
   *   return NULL;
   * }
   */

  sin.sin_family = AF_INET;
  // sin.sin_addr.s_addr = ((struct in_addr*)phe->h_addr_list[0])->s_addr;;
  sin.sin_addr.s_addr = inet_addr("65.99.239.251");
  sin.sin_port = htons(80);

  if(connect(s, (struct sockaddr*)&sin, sizeof(struct sockaddr_in))) {
    perror("connect");
    return NULL;
  }

  body_send = (gchar *)g_malloc(MAX_LEN);
  body_recv = (gchar *)g_malloc(MAX_LEN);
  body_response = (gchar *)g_malloc(MAX_LEN);

  //  memset(body_response, 0, MAX_LEN);
  len_respose = 0;

  sprintf(body_send,
	  "GET /%s HTTP/1.1\r\n"
	  "Host: %s\r\n"
	  "Connection: close\r\n"
	  "Accept: */*\r\n\r\n", url, host);

  send(s, body_send, strlen(body_send), 0);

  int new_len = MAX_LEN;
  gchar *tmp;

  do {
    len = recv(s, body_recv, MAX_LEN, 0);
    len_respose += len;
    if (len_respose > new_len) {
      new_len = new_len * 2;
      body_response = (gchar *) g_realloc(body_response, new_len );
    }
    memcpy(body_response + (len_respose - len), body_recv, len);
  } while (len);

  // remove headers
  tmp = strstr(body_response, "\r\n\r\n");
  len_respose -= (tmp - body_response + 4);
  memcpy(body_response, tmp + 4, len_respose);


  g_free(body_recv);
  g_free(body_send);
  close(s);

  *res_len = len_respose;
  return body_response;
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

static char*
juick_make_avatar_dir()
{
  struct stat sb;
  char *home;
  char *maybe_dir = (char *) malloc(MAX_PATH);

#ifdef _WIN32
  home = getenv("APPDATA");
  sprintf(maybe_dir, "%s/tmp-juick/", home);
#else
  home = getenv("HOME");
  strcpy(maybe_dir, "/tmp/pidgin-juick/");
#endif

  if (home) {
    sprintf(maybe_dir, "%s/.purple/juick-avatars", home);
    if (stat(maybe_dir, &sb) == -1) {
#ifdef _WIN32
      mkdir(maybe_dir);
#else
      mkdir(maybe_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
    }
    return maybe_dir;
  }
  // aren`t HOME ?
#ifdef _WIN32
  mkdir(maybe_dir);
#else
  mkdir(maybe_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

  return maybe_dir;
}

static void
juick_avatar_init()
{
  avatar_store = g_hash_table_new(g_int_hash, g_str_equal);
  juick_avatar_dir = juick_make_avatar_dir();
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
  GList *convs = purple_get_conversations();
  PidginConversation *gtk_conv_handle = pidgin_conversations_get_handle();
  void *conv_handle = purple_conversations_get_handle();

  juick_avatar_init();

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

static GtkWidget *
get_config_frame(PurplePlugin *plugin)
{
  GtkWidget *ret;
  GtkWidget *frame;
  int f;
  GtkWidget *vbox, *hbox, *button;

  ret = gtk_vbox_new(FALSE, PIDGIN_HIG_CAT_SPACE);
  gtk_container_set_border_width(GTK_CONTAINER(ret), PIDGIN_HIG_BORDER);

  f = purple_prefs_get_int(PREF_USE_AVATAR);

  frame = pidgin_make_frame(ret, "Settings");
  vbox = gtk_vbox_new(FALSE, PIDGIN_HIG_BOX_SPACE);
  gtk_box_pack_start(GTK_BOX(frame), vbox, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(FALSE, PIDGIN_HIG_BOX_SPACE);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  button = gtk_check_button_new_with_label("Use Avatar ?");
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  if (f)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  g_signal_connect(G_OBJECT(button), "clicked",
		   G_CALLBACK(toggle_avatar), NULL);
  g_signal_connect(GTK_OBJECT(ret), "destroy",
		   G_CALLBACK(disconnect_prefs_callbacks), plugin);

  button = gtk_check_button_new_with_label("Add + after id?");
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  if (f)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  g_signal_connect(G_OBJECT(button), "clicked",
		   G_CALLBACK(toggle_id), NULL);
  g_signal_connect(GTK_OBJECT(ret), "destroy",
		   G_CALLBACK(disconnect_prefs_callbacks), plugin);


  gtk_widget_show_all(ret);
  return ret;
}

static void
toggle_id(GtkWidget *widget, gpointer data)
{
  int f;

  f = purple_prefs_get_int(PREF_USE_ID_PLUS);
  f = (f ? 0 : 1);
  purple_prefs_set_int(PREF_USE_ID_PLUS, f);
}

static void
toggle_avatar(GtkWidget *widget, gpointer data)
{
  int f;

  f = purple_prefs_get_int(PREF_USE_AVATAR);
  f = (f ? 0 : 1);
  purple_prefs_set_int(PREF_USE_AVATAR, f);
}

static void
disconnect_prefs_callbacks(GtkObject *object, gpointer data)
{
  PurplePlugin *plugin = (PurplePlugin *)data;

  purple_prefs_disconnect_by_handle(plugin);
}

static PidginPluginUiInfo ui_info =
  {
    get_config_frame,
    0, /* page_num (reserved) */

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
    "Adds some color and button for juick bot.",        /**< description */
    "owner.mad.epa@gmail.com",                          /**< author */
    "http://github.com/mad/pidgin-juick-plugin",        /**< homepage */
    plugin_load,                                        /**< load */
    plugin_unload,                                      /**< unload */
    NULL,                                               /**< destroy */
    &ui_info,                                           /**< ui_info        */
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
  purple_prefs_add_none(PREF_PREFIX);

  purple_prefs_add_int(PREF_USE_AVATAR, 0);
  purple_prefs_add_int(PREF_USE_ID_PLUS, 0);
}

PURPLE_INIT_PLUGIN(juick, init_plugin, info)
